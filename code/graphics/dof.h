#pragma once

#include "globalincs/pstypes.h"

// Far-only depth of field. The player's ship and the near part of the nearest capital ship stay
// sharp; the far part of that ship, everything behind it and (optionally) the sky are blurred.
// This is not a lens model: no physical lens blurs only behind a band. The pass is OpenGL only.

namespace graphics {

enum class DofMode { Off = 0, Cutscenes = 1, Always = 2 };

// Only ships at least this large can be the focus ship. Smaller ships are fighters and
// bombers, which stay sharp because the blur never reaches in front of the focus ship.
constexpr float DOF_FOCUS_MIN_RADIUS = 200.0f;

struct dof_candidate {
	float surface_dist; // distance from the eye to the ship's bounding sphere
	float radius;
	float zmax;         // view-space z of the far end of its bounding box
};

// Index of the focus ship: the nearest candidate (by surface_dist) with radius >= DOF_FOCUS_MIN_RADIUS
// that is not wholly behind the eye. -1 when there is none.
int dof_pick_focus(const SCP_vector<dof_candidate>& candidates);

// View-space z extent (along eye_fvec) of a box given by its 8 model-space corners.
void dof_view_z_extent(const vec3d corners[8], const vec3d& pos, const matrix& orient, const vec3d& eye_pos,
	const vec3d& eye_fvec, float* zmin, float* zmax);

struct dof_band {
	float start; // view z where the blur starts
	float ramp;  // metres over which the blur grows from 0 to the max blur
};

// start_frac picks the start inside [zmin, zmax]; the blur reaches tail * max blur at zmax.
// tail 0 means the ship and what is behind it stay sharp up to the far plane (only the sky blurs).
dof_band dof_far_band(float zmin, float zmax, float start_frac, float tail);

bool dof_mode_active(DofMode mode, bool in_cutscene);

// The renderer's view of the options for one frame.
struct dof_frame {
	dof_band band;
	float max_blur; // pixels
	float sky_blur; // scale of max_blur for pixels with no depth
};

// False when no blur runs this frame (option off, not a cutscene in Cutscenes mode, or no focus ship).
bool dof_get_frame(dof_frame* out);

// The OpenGL pass is the only implementation. Records that requirement for every DoF option, so the
// options dialog greys them out on other renderers and a launcher can say why.
void dof_set_requirements();

} // namespace graphics
