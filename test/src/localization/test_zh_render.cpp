// Spike: draw Chinese text with the real OpenGL renderer and save a screenshot.
// Needs a display (DISPLAY=:0). Run only with --gtest_filter=ZhRender*.
#include <gtest/gtest.h>

#include "../../../freespace2/SDLGraphicsOperations.cpp"

#include "cmdline/cmdline.h"
#include "globalincs/alphacolors.h"
#include "graphics/2d.h"
#include "graphics/opengl/gropengl.h"
#include "graphics/render.h"
#include "graphics/software/font.h"
#include "mod_table/mod_table.h"
#include "parse/parselo.h"
#include "util/FSTestFixture.h"

class ZhRenderTest : public test::FSTestFixture {
 public:
	ZhRenderTest() : test::FSTestFixture(INIT_MOD_TABLE | INIT_CFILE) {
		pushModDir("zh_spike");
		pushModDir("wrap");
	}

 protected:
	void SetUp() override { test::FSTestFixture::SetUp(); }
	void TearDown() override { test::FSTestFixture::TearDown(); }
};

static int draw_wrapped(int x, int y, int w, const char* text) {
	SCP_vector<int> n;
	SCP_vector<const char*> p;
	int lines = split_str(text, w, n, p);
	int fh = gr_get_font_height();
	for (int i = 0; i < lines; i++)
		gr_string(x, y + i * (fh + 4), p[i], GR_RESIZE_NONE, 1.0f, n[i]);
	return y + lines * (fh + 4);
}

TEST_F(ZhRenderTest, screenshot) {
	ASSERT_TRUE(Unicode_text_mode);
	extern int Is_standalone;
	ImGui::CreateContext(); // freespace.cpp does this before gr_init
	Is_standalone = 0; // the test harness sets 1, which forces the Stub renderer
	ASSERT_TRUE(gr_init(std::make_unique<SDLGraphicsOperations>(), GraphicsAPI::OpenGL, 1024, 768));
	font::init();
	alpha_colors_init();

	for (int frame = 0; frame < 3; frame++) {
		gr_clear();
		gr_set_color_fast(&Color_grey);
		gr_rect(60, 60, 904, 2, GR_RESIZE_NONE);
		gr_rect(60, 520, 904, 2, GR_RESIZE_NONE);

		font::set_font(2);
		gr_set_color_fast(&Color_bright_white);
		gr_string(80, 80, "第一章：熱點", GR_RESIZE_NONE);

		font::set_font(0);
		gr_set_color_fast(&Color_bright_blue);
		int y = draw_wrapped(80, 140, 860,
			"阿爾法一號，前往跳躍點，等候進一步命令。偵測到席瓦人的訊號，"
			"所有戰機保持編隊，不要脫離護航目標。重複：不要脫離護航目標。");
		gr_set_color_fast(&Color_bright_green);
		draw_wrapped(80, y + 24, 240, "指揮部：收到。這一欄只有 240px 寬，用來看中文逐字斷行，以及 English words mixed in 的情況。");

		font::set_font(1);
		gr_set_color_fast(&Color_white);
		gr_string(80, 540, "FSO 81caf06 + spike/zh-i18n, OpenGL, Noto Sans TC 16/12/24", GR_RESIZE_NONE);
		if (frame == 2) {
			int w = gr_screen.max_w, h = gr_screen.max_h;
			SCP_vector<unsigned char> px(w * h * 3);
			glFinish();
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
			FILE* f = fopen("/dev/shm/zh_render.ppm", "wb");
			fprintf(f, "P6\n%d %d\n255\n", w, h);
			for (int r = h - 1; r >= 0; r--) fwrite(&px[r * w * 3], 1, w * 3, f);
			fclose(f);
		}
		gr_flip(false);
	}

	font::close();
	gr_close();
}
