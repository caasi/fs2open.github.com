#include <gtest/gtest.h>

#include "cfile/cfile.h"
#include "cmdline/cmdline.h"
#include "localization/localize.h"
#include "mod_table/mod_table.h"
#include "options/Option.h"
#include "options/OptionsManager.h"
#include "sound/audiostr.h"

#include "util/FSTestFixture.h"

// Test data (test_data/voice_language):
//   data/tables/zh-lcl.tbm      adds the language "Chinese" with the extension "tw"
//   data/voice/briefing/        foo.ogg, tw/foo.ogg, bar.ogg (no tw/bar.ogg)
//   data/voice/debriefing/      tw/9_onlytw.ogg (no 9_onlytw.ogg): a voice pack with no original
class VoiceLanguageTest : public test::FSTestFixture {
  public:
	VoiceLanguageTest() : test::FSTestFixture(INIT_MOD_TABLE | INIT_CFILE) { pushModDir("voice_language"); }

  protected:
	// Start from an empty list so that this test does not depend on the tests that ran before it. The base class
	// clears the list in lcl_close(), so put back the list of the earlier tests afterwards: a later test
	// (IntelParseTest) relies on German being at index LCL_GERMAN.
	SCP_vector<lang_info> _saved_languages;
	int _saved_current_lang = 0;
	bool _saved_in_game_options = false;

	void SetUp() override
	{
		_saved_languages = Lcl_languages;
		_saved_current_lang = Lcl_current_lang;
		_saved_in_game_options = Using_in_game_options;
		Lcl_languages.clear();
		test::FSTestFixture::SetUp();
	}

	void TearDown() override
	{
		// The voice language comes from three process-wide places. Clear all of them, so that nothing leaks
		// into the next test. mod_table_reset() in the base class clears the mod default.
		Cmdline_voice_lang.clear();
		options::OptionsManager::instance()->clearOverrides();
		options::OptionsManager::instance()->discardChanges();
		test::FSTestFixture::TearDown();
		Lcl_languages = _saved_languages;
		Lcl_current_lang = _saved_current_lang;
		Using_in_game_options = _saved_in_game_options;
	}

	static int chinese()
	{
		int lang = lcl_find_lang_index_by_name("Chinese");
		EXPECT_GE(lang, 0) << "zh-lcl.tbm was not read";
		return lang;
	}

	static SCP_string in_tw(const char* name) { return SCP_string("tw") + DIR_SEPARATOR_STR + name; }

	// the text language is English after SetUp(); this makes it Chinese for the rest of the test
	static void set_text_language_chinese() { lcl_set_language(chinese()); }

	static void set_player_voice_language(const char* name, const char* ext)
	{
		options::OptionsManager::instance()->setConfigValue("Audio.VoiceLanguage",
			std::unique_ptr<json_t>(json_pack("{ssss}", "name", name, "ext", ext)));
	}

	// "original" or "same-as-text", the two stored values that are not a language
	static void set_player_voice_choice(const char* value)
	{
		options::OptionsManager::instance()->setConfigValue("Audio.VoiceLanguage",
			std::unique_ptr<json_t>(json_string(value)));
	}

	static SCP_string option_display()
	{
		auto* opt = options::OptionsManager::instance()->getOptionByKey("Audio.VoiceLanguage");
		EXPECT_NE(opt, nullptr);
		return opt ? opt->getCurrentValueDescription().display : "";
	}
};

// The -voice_language flag is read when the command line is parsed, so it must be in place before SetUp().
class VoiceLanguageCmdlineTest : public VoiceLanguageTest {
  public:
	VoiceLanguageCmdlineTest()
	{
		addCommandlineArg("-voice_language");
		addCommandlineArg("original");
	}
};

TEST_F(VoiceLanguageTest, NothingSetFollowsTheTextLanguage)
{
	EXPECT_EQ(lcl_get_voice_lang_index(), LCL_ENGLISH);
	EXPECT_EQ(cf_localize_voice_filename("foo.ogg", CF_TYPE_ANY, NUM_AUDIO_EXT, audio_ext_list), "foo.ogg");

	set_text_language_chinese();
	EXPECT_EQ(lcl_get_voice_lang_index(), chinese());
	EXPECT_EQ(cf_localize_voice_filename("foo.ogg", CF_TYPE_ANY, NUM_AUDIO_EXT, audio_ext_list), in_tw("foo.ogg"));

	Using_in_game_options = true;
	EXPECT_EQ(option_display(), "Same as text language");
}

TEST_F(VoiceLanguageTest, OptionValues)
{
	auto* opt = options::OptionsManager::instance()->getOptionByKey("Audio.VoiceLanguage");
	ASSERT_NE(opt, nullptr);

	// the two values that are not a language, then every language
	auto values = opt->getValidValues();
	ASSERT_EQ(values.size(), Lcl_languages.size() + 2);
	EXPECT_EQ(values[0].display, "Same as text language");
	EXPECT_EQ(values[1].display, "Original voice");
	EXPECT_EQ(values.back().display, "Chinese");
}

TEST_F(VoiceLanguageTest, MenuValuesRoundTrip)
{
	// the menu sets a value from the description that getValidValues() gave
	Using_in_game_options = true;
	set_text_language_chinese();
	auto* opt = options::OptionsManager::instance()->getOptionByKey("Audio.VoiceLanguage");
	ASSERT_NE(opt, nullptr);
	auto values = opt->getValidValues();

	opt->setValueDescription(values[1]);
	EXPECT_EQ(lcl_get_voice_lang_index(), LCL_VOICE_ORIGINAL);
	EXPECT_EQ(option_display(), "Original voice");

	opt->setValueDescription(values[0]);
	EXPECT_EQ(lcl_get_voice_lang_index(), chinese());
	EXPECT_EQ(option_display(), "Same as text language");

	opt->setValueDescription(values.back());
	EXPECT_EQ(lcl_get_voice_lang_index(), chinese());
	EXPECT_EQ(option_display(), "Chinese");
}

TEST_F(VoiceLanguageTest, VoiceLanguageApartFromTheTextLanguage)
{
	Using_in_game_options = true;
	set_player_voice_language("Chinese", "tw");

	EXPECT_EQ(lcl_get_voice_lang_index(), chinese());
	EXPECT_EQ(cf_localize_voice_filename("foo.ogg", CF_TYPE_ANY, NUM_AUDIO_EXT, audio_ext_list), in_tw("foo.ogg"));
	EXPECT_EQ(cf_localize_voice_filename("bar.ogg", CF_TYPE_ANY, NUM_AUDIO_EXT, audio_ext_list), "bar.ogg");

	// other localized files (fonts, SCPUI) keep the text language
	EXPECT_EQ(cf_localize_filename("foo.ogg", CF_TYPE_ANY, NUM_AUDIO_EXT, audio_ext_list), "foo.ogg");
}

TEST_F(VoiceLanguageTest, OriginalVoiceIgnoresTheTextLanguage)
{
	Using_in_game_options = true;
	set_player_voice_choice("original");
	set_text_language_chinese();

	EXPECT_EQ(option_display(), "Original voice");
	EXPECT_EQ(lcl_get_voice_lang_index(), LCL_VOICE_ORIGINAL);
	EXPECT_EQ(cf_localize_voice_filename("foo.ogg", CF_TYPE_ANY, NUM_AUDIO_EXT, audio_ext_list), "foo.ogg");
	EXPECT_EQ(cf_localize_filename("foo.ogg", CF_TYPE_ANY, NUM_AUDIO_EXT, audio_ext_list), in_tw("foo.ogg"));
}

TEST_F(VoiceLanguageTest, VoiceOnlyPack)
{
	// a voice pack with no translated text: English text, Chinese voice; a name with only the tw file exists
	Using_in_game_options = true;
	set_player_voice_language("Chinese", "tw");

	auto localized = cf_localize_voice_filename("9_onlytw", CF_TYPE_VOICE_DEBRIEFINGS, NUM_AUDIO_EXT, audio_ext_list);
	EXPECT_EQ(localized, in_tw("9_onlytw"));
	EXPECT_TRUE(cf_exists_full_ext(localized.c_str(), CF_TYPE_VOICE_DEBRIEFINGS, NUM_AUDIO_EXT, audio_ext_list));

	// the folder limits the lookup
	EXPECT_EQ(cf_localize_voice_filename("9_onlytw", CF_TYPE_VOICE_BRIEFINGS, NUM_AUDIO_EXT, audio_ext_list), "9_onlytw");
}

TEST_F(VoiceLanguageTest, EmptyLanguageListLeavesTheNameAlone)
{
	// before lcl_init(); also with "same as text", whose index is the text language's and so not negative
	Using_in_game_options = true;
	set_player_voice_choice("same-as-text");

	auto languages = Lcl_languages;
	Lcl_languages.clear();
	EXPECT_EQ(lcl_get_voice_lang_index(), LCL_VOICE_ORIGINAL);
	Lcl_languages = languages;
}

TEST_F(VoiceLanguageTest, PlayerSettingIgnoredWithoutInGameOptions)
{
	Using_in_game_options = false;
	set_player_voice_language("Chinese", "tw");

	EXPECT_EQ(lcl_get_voice_lang_index(), LCL_ENGLISH);
}

TEST_F(VoiceLanguageTest, ModDefault)
{
	Default_voice_language = "Chinese";
	EXPECT_EQ(lcl_get_voice_lang_index(), chinese());

	Using_in_game_options = false;
	EXPECT_EQ(lcl_get_voice_lang_index(), chinese());

	Default_voice_language = "original";
	set_text_language_chinese();
	EXPECT_EQ(lcl_get_voice_lang_index(), LCL_VOICE_ORIGINAL);
}

TEST_F(VoiceLanguageTest, PlayerChoiceBeatsTheModDefault)
{
	Default_voice_language = "Chinese";
	Using_in_game_options = true;

	// while the player has not chosen, the menu shows the mod default, which is what plays
	EXPECT_EQ(option_display(), "Chinese");
	EXPECT_EQ(lcl_get_voice_lang_index(), chinese());

	set_player_voice_language("English", "");
	EXPECT_EQ(lcl_get_voice_lang_index(), LCL_ENGLISH);

	set_player_voice_choice("same-as-text");
	EXPECT_EQ(lcl_get_voice_lang_index(), LCL_ENGLISH);

	set_player_voice_choice("original");
	EXPECT_EQ(lcl_get_voice_lang_index(), LCL_VOICE_ORIGINAL);
}

TEST_F(VoiceLanguageTest, StaleStoredValueFallsBackToTheModDefault)
{
	Default_voice_language = "Chinese";
	Using_in_game_options = true;

	// a language this mod does not have, a known name with another extension, and a hand-edited string
	set_player_voice_language("Klingon", "kl");
	EXPECT_EQ(lcl_get_voice_lang_index(), chinese());

	set_player_voice_language("English", "xx");
	EXPECT_EQ(lcl_get_voice_lang_index(), chinese());

	set_player_voice_choice("bogus");
	EXPECT_EQ(lcl_get_voice_lang_index(), chinese());
	EXPECT_EQ(option_display(), "Chinese");
}

TEST_F(VoiceLanguageCmdlineTest, FlagBeatsEverything)
{
	Default_voice_language = "Chinese";
	set_text_language_chinese();

	Using_in_game_options = false;
	EXPECT_EQ(lcl_get_voice_lang_index(), LCL_VOICE_ORIGINAL);

	// the options menu shows the flag as an override of the player setting
	Using_in_game_options = true;
	EXPECT_TRUE(options::OptionsManager::instance()->isOverridden("Audio.VoiceLanguage"));
	EXPECT_EQ(option_display(), "Original voice");
	EXPECT_EQ(lcl_get_voice_lang_index(), LCL_VOICE_ORIGINAL);

	// the override rejects a player choice, so remove it to store one; the flag still wins
	options::OptionsManager::instance()->clearOverrides();
	set_player_voice_language("Chinese", "tw");
	EXPECT_EQ(option_display(), "Chinese");
	EXPECT_EQ(lcl_get_voice_lang_index(), LCL_VOICE_ORIGINAL);
}

TEST_F(VoiceLanguageTest, UnknownNamesAreIgnored)
{
	// set after SetUp(): for an unknown name lcl_init() gives a Warning, which throws under unittests
	Cmdline_voice_lang = "Klingon";
	Default_voice_language = "Klingon";
	EXPECT_EQ(lcl_get_voice_lang_index(), LCL_ENGLISH);

	// an unknown flag falls through to the next source
	Using_in_game_options = true;
	set_player_voice_language("Chinese", "tw");
	EXPECT_EQ(lcl_get_voice_lang_index(), chinese());
}
