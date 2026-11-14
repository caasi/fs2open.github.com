#include "graphics/dof.h"

#include <algorithm>

#include "globalincs/systemvars.h"
#include "localization/localize.h"
#include "math/vecmat.h"
#include "model/model.h"
#include "object/object.h"
#include "options/Option.h"
#include "options/OptionsManager.h"
#include "playerman/player.h"
#include "render/3d.h"
#include "ship/ship.h"

namespace graphics {

static DofMode Dof_mode = DofMode::Off;
static int Dof_max_blur = 16;
static float Dof_start = 0.33f;
static float Dof_tail = 1.0f;
static float Dof_sky = 1.0f;
static DofQuality Dof_quality = DofQuality::High;

static const char* const Dof_option_keys[] = {"Graphics.DepthOfField", "Graphics.DepthOfFieldMaxBlur",
	"Graphics.DepthOfFieldStart", "Graphics.DepthOfFieldTail", "Graphics.DepthOfFieldSky", "Graphics.DepthOfFieldQuality"};

// coverity[GLOBAL_INIT_ORDER] -- safe; OptionBuilder::finish() uses Meyers singleton
static auto DofModeOption __UNUSED = options::OptionBuilder<DofMode>("Graphics.DepthOfField",
                     std::pair<const char*, int>{"Depth of field", -1},
                     std::pair<const char*, int>{"Blurs the far part of the nearest capital ship and everything behind it. "
                                                 "Your ship and nearby ships stay sharp (requires post-processing, OpenGL only)", -1})
                     .category(std::make_pair("Graphics", 1825))
                     .level(options::ExpertLevel::Advanced)
                     .values({{DofMode::Off, {"Off", -1}},
                              {DofMode::Cutscenes, {"Cutscenes only", -1}},
                              {DofMode::Always, {"Always", -1}}})
                     .default_val(DofMode::Off)
                     .bind_to(&Dof_mode)
                     .importance(54)
                     .finish();

// coverity[GLOBAL_INIT_ORDER] -- safe; OptionBuilder::finish() uses Meyers singleton
static auto DofMaxBlurOption __UNUSED = options::OptionBuilder<int>("Graphics.DepthOfFieldMaxBlur",
                     std::pair<const char*, int>{"Depth of field max blur", -1},
                     std::pair<const char*, int>{"Largest blur radius, in pixels", -1})
                     .category(std::make_pair("Graphics", 1825))
                     .level(options::ExpertLevel::Advanced)
                     .range(1, 32)
                     .default_val(16)
                     .bind_to(&Dof_max_blur)
                     .importance(53)
                     .flags({options::OptionFlags::RangeTypeInteger})
                     .finish();

// coverity[GLOBAL_INIT_ORDER] -- safe; OptionBuilder::finish() uses Meyers singleton
static auto DofStartOption __UNUSED = options::OptionBuilder<float>("Graphics.DepthOfFieldStart",
                     std::pair<const char*, int>{"Depth of field start", -1},
                     std::pair<const char*, int>{"Where the blur starts, as a part of the depth of the nearest capital ship "
                                                 "(0 = its near end, 1 = its far end)", -1})
                     .category(std::make_pair("Graphics", 1825))
                     .level(options::ExpertLevel::Advanced)
                     .range(0.0f, 1.0f)
                     .default_val(0.33f)
                     .bind_to(&Dof_start)
                     .importance(52)
                     .finish();

// coverity[GLOBAL_INIT_ORDER] -- safe; OptionBuilder::finish() uses Meyers singleton
static auto DofTailOption __UNUSED = options::OptionBuilder<float>("Graphics.DepthOfFieldTail",
                     std::pair<const char*, int>{"Depth of field tail blur", -1},
                     std::pair<const char*, int>{"Blur at the far end of the nearest capital ship, as a part of the max blur", -1})
                     .category(std::make_pair("Graphics", 1825))
                     .level(options::ExpertLevel::Advanced)
                     .range(0.0f, 1.0f)
                     .default_val(1.0f)
                     .bind_to(&Dof_tail)
                     .importance(51)
                     .finish();

// coverity[GLOBAL_INIT_ORDER] -- safe; OptionBuilder::finish() uses Meyers singleton
static auto DofSkyOption __UNUSED = options::OptionBuilder<float>("Graphics.DepthOfFieldSky",
                     std::pair<const char*, int>{"Depth of field sky blur", -1},
                     std::pair<const char*, int>{"Blur of the stars and background, as a part of the max blur", -1})
                     .category(std::make_pair("Graphics", 1825))
                     .level(options::ExpertLevel::Advanced)
                     .range(0.0f, 1.0f)
                     .default_val(1.0f)
                     .bind_to(&Dof_sky)
                     .importance(50)
                     .finish();

// coverity[GLOBAL_INIT_ORDER] -- safe; OptionBuilder::finish() uses Meyers singleton
static auto DofQualityOption __UNUSED = options::OptionBuilder<DofQuality>("Graphics.DepthOfFieldQuality",
                     std::pair<const char*, int>{"Depth of field quality", -1},
                     std::pair<const char*, int>{"Samples per pixel of the blur. Lower is faster; the blur of single stars shows a pattern of dots", -1})
                     .category(std::make_pair("Graphics", 1825))
                     .level(options::ExpertLevel::Advanced)
                     .values({{DofQuality::Low, {"Low", -1}},
                              {DofQuality::Medium, {"Medium", -1}},
                              {DofQuality::High, {"High", -1}}})
                     .default_val(DofQuality::High)
                     .bind_to(&Dof_quality)
                     .importance(49)
                     .finish();

int dof_samples(DofQuality quality)
{
	const int level = std::clamp(static_cast<int>(quality), 0, 2);
	return DOF_MAX_SAMPLES * (level + 1) / 3;
}

int dof_pick_focus(const SCP_vector<dof_candidate>& candidates)
{
	int best = -1;
	for (int i = 0; i < static_cast<int>(candidates.size()); i++) {
		const auto& c = candidates[i];
		if (c.radius < DOF_FOCUS_MIN_RADIUS || c.zmax <= 0.0f || !c.in_view || c.cloaked) {
			continue;
		}
		if (best < 0 || c.surface_dist < candidates[best].surface_dist) {
			best = i;
		}
	}
	return best;
}

bool dof_sphere_in_view(const vec3d& centre, float radius, float tan_half_fov_y, float aspect)
{
	if (centre.xyz.z + radius <= 0.0f) {
		return false;
	}
	// Signed distance to each side plane through the eye; outside when it is more than the radius.
	const float tan_half_fov_x = tan_half_fov_y * aspect;
	const float dx = (fabsf(centre.xyz.x) - centre.xyz.z * tan_half_fov_x) / sqrtf(1.0f + tan_half_fov_x * tan_half_fov_x);
	const float dy = (fabsf(centre.xyz.y) - centre.xyz.z * tan_half_fov_y) / sqrtf(1.0f + tan_half_fov_y * tan_half_fov_y);
	return dx <= radius && dy <= radius;
}

void dof_view_z_extent(const vec3d corners[8], const vec3d& pos, const matrix& orient, const vec3d& eye_pos,
	const vec3d& eye_fvec, float* zmin, float* zmax)
{
	*zmin = FLT_MAX;
	*zmax = -FLT_MAX;
	for (int i = 0; i < 8; i++) {
		vec3d world, to;
		vm_vec_unrotate(&world, &corners[i], &orient);
		vm_vec_add2(&world, &pos);
		vm_vec_sub(&to, &world, &eye_pos);
		const float z = vm_vec_dot(&to, &eye_fvec);
		*zmin = MIN(*zmin, z);
		*zmax = MAX(*zmax, z);
	}
}

dof_band dof_far_band(float zmin, float zmax, float start_frac, float tail)
{
	zmin = MAX(zmin, 1.0f);
	zmax = MAX(zmax, zmin);
	dof_band b;
	b.start = zmin + start_frac * (zmax - zmin);
	// The shader divides by the ramp; tail 0 means "no blur before the far plane".
	b.ramp = tail > 0.0f ? MAX(1.0f, (zmax - b.start) / tail) : 1.0e30f;
	return b;
}

bool dof_mode_active(DofMode mode, bool in_cutscene)
{
	return mode == DofMode::Always || (mode == DofMode::Cutscenes && in_cutscene);
}

// A cutscene is a scripted camera (set-camera sexp, Lua gr.setCamera, autopilot cinematic)
// or the cutscene bars (cutscene-bars sexp, autopilot).
static bool dof_in_cutscene()
{
	return (Viewer_mode & VM_FREECAMERA) || (Cutscene_bar_flags & CUB_CUTSCENE);
}

bool dof_get_frame(dof_frame* out)
{
	if (!dof_mode_active(Dof_mode, dof_in_cutscene())) {
		return false;
	}

	// ponytail: an asymmetric (VR) field of view skips the frustum test; every ship counts as in view there.
	const bool symmetric_fov = std::holds_alternative<float>(Proj_fov);
	const float tan_half_fov_y = symmetric_fov ? tanf(std::get<float>(Proj_fov) * 0.5f) : 0.0f;

	SCP_vector<dof_candidate> candidates;
	SCP_vector<std::pair<float, float>> extents;
	for (auto so : list_range(&Ship_obj_list)) {
		const object* o = &Objects[so->objnum];
		if (o == Player_obj || o->radius < DOF_FOCUS_MIN_RADIUS || o->flags[Object::Object_Flags::Should_be_dead]) {
			continue;
		}
		const polymodel* pm = model_get(Ship_info[Ships[o->instance].ship_info_index].model_num);
		float zmin, zmax;
		dof_view_z_extent(pm->bounding_box, o->pos, o->orient, Eye_position, Eye_matrix.vec.fvec, &zmin, &zmax);
		vec3d to, view;
		vm_vec_sub(&to, &o->pos, &Eye_position);
		vm_vec_rotate(&view, &to, &Eye_matrix);
		const bool in_view = !symmetric_fov || dof_sphere_in_view(view, o->radius, tan_half_fov_y, gr_screen.clip_aspect);
		const bool cloaked = Ships[o->instance].flags[Ship::Ship_Flags::Cloaked];
		candidates.push_back({vm_vec_dist(&o->pos, &Eye_position) - o->radius, o->radius, zmax, in_view, cloaked});
		extents.emplace_back(zmin, zmax);
	}

	const int focus = dof_pick_focus(candidates);
	if (focus < 0) {
		return false;
	}
	out->band = dof_far_band(extents[focus].first, extents[focus].second, Dof_start, Dof_tail);
	out->max_blur = static_cast<float>(Dof_max_blur);
	out->sky_blur = Dof_sky;
	out->samples = dof_samples(Dof_quality);
	return true;
}

void dof_set_requirements()
{
	for (const char* key : Dof_option_keys) {
		options::OptionsManager::instance()->setRequirement(key, "opengl",
			XSTR("Depth of field is only available with the OpenGL renderer.", -1));
	}
}

} // namespace graphics
