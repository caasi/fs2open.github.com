// Spike: can a mod add Chinese as a new language, with no retail data?
#include <gtest/gtest.h>

#include "localization/localize.h"
#include "mod_table/mod_table.h"
#include "util/FSTestFixture.h"

class ZhSpikeTest : public test::FSTestFixture {
 public:
	ZhSpikeTest() : test::FSTestFixture(INIT_MOD_TABLE | INIT_CFILE) { pushModDir("zh_spike"); }

 protected:
	void SetUp() override { test::FSTestFixture::SetUp(); }
	void TearDown() override { test::FSTestFixture::TearDown(); }
};

TEST_F(ZhSpikeTest, languages) {
	EXPECT_TRUE(Unicode_text_mode);
	for (auto& l : Lcl_languages)
		std::cout << "lang: " << l.lang_name << " ext=" << l.lang_ext << "\n";
	EXPECT_GE(lcl_find_lang_index_by_name("Chinese"), 0);
	std::cout << "German index: " << lcl_find_lang_index_by_name("German") << "\n";
}

TEST_F(ZhSpikeTest, translate) {
	int zh = lcl_find_lang_index_by_name("Chinese");
	ASSERT_GE(zh, 0);
	lcl_xstr_close();
	lcl_set_language(zh);
	lcl_xstr_init();

	SCP_string out;
	lcl_ext_localize("XSTR(\"Alpha 1\", 3000)", out);
	EXPECT_EQ(out, "阿爾法一號");
	lcl_ext_localize("XSTR(\"Proceed to the jump node and await further orders.\", 3001)", out);
	EXPECT_EQ(out, "前往跳躍點，等候進一步命令。");
	lcl_ext_localize("XSTR(\"No id\", -1)", out);
	EXPECT_EQ(out, "No id");
}

#include "graphics/2d.h"
#include "graphics/software/font.h"
#include "parse/parselo.h"

class ZhWrapTest : public test::FSTestFixture {
 public:
	ZhWrapTest() : test::FSTestFixture(INIT_MOD_TABLE | INIT_CFILE | INIT_GRAPHICS | INIT_FONTS) {
		pushModDir("zh_spike");
		pushModDir("wrap");
	}

 protected:
	void SetUp() override { test::FSTestFixture::SetUp(); }
	void TearDown() override { test::FSTestFixture::TearDown(); }
};

static void dump(const char* label, const char* text, int max_w, int max_len) {
	SCP_vector<int> n;
	SCP_vector<const char*> p;
	int lines = split_str(text, max_w, n, p, max_len);
	std::cout << label << " lines=" << lines << "\n";
	for (int i = 0; i < lines; i++) {
		int w, h;
		gr_get_string_size(&w, &h, p[i], 1.0f, n[i]);
		std::cout << "  [" << SCP_string(p[i], n[i]) << "] bytes=" << n[i] << " px=" << w << "\n";
		EXPECT_EQ(n[i] % 3, 0) << "split inside a UTF-8 sequence";
	}
}

TEST_F(ZhWrapTest, wrap) {
	EXPECT_TRUE(Unicode_text_mode);
	std::cout << "fonts: " << font::FontManager::numberOfFonts() << "\n";
	font::set_font(0);
	const char* zh = "前往跳躍點等候進一步命令前往跳躍點等候進一步命令";
	dump("no space, 100px", zh, 100, INT_MAX);
	dump("no space, 1000px, max 10 bytes", zh, 1000, 10);
	dump("one space, 100px", "前往跳躍點 等候進一步命令前往跳躍點等候進一步命令", 100, INT_MAX);
}
