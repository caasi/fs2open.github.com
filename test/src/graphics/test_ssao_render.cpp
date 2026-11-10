// Spike: draw one FS2 demo ship through the deferred renderer with the GTAO
// spike off, on, and in the AO debug view, and save each frame as PPM.
// Needs a display (DISPLAY=:0) and the FS2 demo data in ~/ai/fs2-demo/data.
// Run only with --gtest_filter=SsaoRender*.
#include <gtest/gtest.h>

#include "../../../freespace2/SDLGraphicsOperations.cpp"

#include "cfile/cfile.h"
#include "cmdline/cmdline.h"
#include "graphics/2d.h"
#include "graphics/light.h"
#include "graphics/matrix.h"
#include "graphics/opengl/gropengl.h"
#include "lighting/lighting.h"
#include "lighting/lighting_profiles.h"
#include "math/vecmat.h"
#include "missionui/missionscreencommon.h"
#include "model/model.h"
#include "model/modelrender.h"
#include "render/3d.h"
#include "util/FSTestFixture.h"

class SsaoRenderTest : public test::FSTestFixture {
 public:
	SsaoRenderTest() : test::FSTestFixture(INIT_NONE) {}

 protected:
	void SetUp() override
	{
		test::FSTestFixture::SetUp();
		root = SCP_string(getenv("HOME")) + "/ai/fs2-demo/data/x"; // cfile wants something after the path
		ASSERT_EQ(cfile_init(root.c_str()), 0);
	}
	void TearDown() override
	{
		cfile_close();
		test::FSTestFixture::TearDown();
	}
	SCP_string root;
};

static void save_ppm(const char* path)
{
	int w = gr_screen.max_w, h = gr_screen.max_h;
	SCP_vector<unsigned char> px(w * h * 3);
	glFinish();
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
	FILE* f = fopen(path, "wb");
	fprintf(f, "P6\n%d %d\n255\n", w, h);
	for (int r = h - 1; r >= 0; r--)
		fwrite(&px[r * w * 3], 1, w * 3, f);
	fclose(f);
}

// Same sequence as obj_render_queue_all() inside game_render_frame(), for one model.
static void render_ship(int model_num)
{
	auto pm = model_get(model_num);
	float r = pm->rad;

	gr_reset_clip();
	gr_clear();
	gr_zbuffer_clear(TRUE);
	gr_scene_texture_begin();

	g3_start_frame(1);
	vec3d eye = vm_vec_new(r * 1.1f, r * 0.7f, -r * 1.6f);
	vec3d fwd;
	vm_vec_negate(&(fwd = eye));
	matrix view;
	vm_vector_2_matrix(&view, &fwd, nullptr, nullptr);
	g3_set_view_matrix(&eye, &view, 0.6f);
	gr_set_proj_matrix(Proj_fov, gr_screen.clip_aspect, Min_draw_distance, Max_draw_distance);
	gr_set_view_matrix(&Eye_position, &Eye_matrix);

	lighting_profiles::set_non_mission_profile non_mission;
	common_setup_room_lights();
	gr_set_ambient_light(170, 170, 170); // ambient-heavy, so AO has something to darken

	gr_deferred_lighting_begin(false);
	model_draw_list scene;
	scene.init();
	model_render_params info;
	info.set_detail_level_lock(0);
	info.set_flags(MR_NO_FOGGING);
	model_render_queue(&info, &scene, model_num, &vmd_identity_matrix, &vmd_zero_vector);
	scene.init_render();
	scene.render_all(ZBUFFER_TYPE_FULL);
	gr_zbuffer_set(ZBUFFER_TYPE_READ);
	gr_clear_states();
	gr_deferred_lighting_end();
	gr_deferred_lighting_finish();

	gr_end_view_matrix();
	gr_end_proj_matrix();
	g3_end_frame();
	gr_scene_texture_end();
}

TEST_F(SsaoRenderTest, ship)
{
	extern int Is_standalone;
	ImGui::CreateContext();
	Is_standalone = 0; // the test harness sets 1, which forces the Stub renderer
	ASSERT_TRUE(gr_init(std::make_unique<SDLGraphicsOperations>(), GraphicsAPI::OpenGL, 1024, 768));
	model_init();

	const char* pof = getenv("SSAO_POF") ? getenv("SSAO_POF") : "fighter2t-02.pof";
	int model_num = model_load(pof, nullptr, ErrorType::WARNING);
	ASSERT_GE(model_num, 0);

	struct { int spike; bool debug; const char* out; } runs[] = {
		{0, false, "/tmp/ssao_off.ppm"},
		{1, false, "/tmp/ssao_on.ppm"},
		{1, true, "/tmp/ssao_debug.ppm"},
	};
	for (auto& run : runs) {
		Cmdline_ssao_spike = run.spike;
		if (run.debug) setenv("FSO_SSAO_DEBUG", "1", 1); else unsetenv("FSO_SSAO_DEBUG");
		// the pass prints its GPU time every 300 frames
		const int frames = (run.spike && !run.debug) ? 310 : 4;
		for (int frame = 0; frame < frames; frame++) {
			render_ship(model_num);
			if (frame == frames - 1) save_ppm(run.out);
			gr_flip(false);
		}
	}
	unsetenv("FSO_SSAO_DEBUG");
	Cmdline_ssao_spike = 0;

	model_free_all();
	gr_close();
}
