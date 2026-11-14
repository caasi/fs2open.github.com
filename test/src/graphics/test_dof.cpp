#include <gtest/gtest.h>

#include "graphics/dof.h"
#include "math/vecmat.h"
#include "options/OptionsManager.h"

using namespace graphics;

TEST(Dof, PickFocusTakesNearestLargeShip)
{
	SCP_vector<dof_candidate> c = {
		{50.0f, 10.0f, 100.0f},    // fighter, nearer: too small
		{900.0f, 800.0f, 3000.0f}, // capital ship
		{400.0f, 250.0f, 1000.0f}, // corvette, nearest large ship
	};
	EXPECT_EQ(dof_pick_focus(c), 2);
}

TEST(Dof, PickFocusAcceptsExactMinRadius)
{
	SCP_vector<dof_candidate> c = {{100.0f, DOF_FOCUS_MIN_RADIUS, 500.0f}};
	EXPECT_EQ(dof_pick_focus(c), 0);
}

TEST(Dof, PickFocusSkipsShipBehindEye)
{
	// A ship behind the camera would put the band at z = 0 and blur the whole screen.
	SCP_vector<dof_candidate> c = {
		{10.0f, 500.0f, -20.0f},
		{2000.0f, 500.0f, 3000.0f},
	};
	EXPECT_EQ(dof_pick_focus(c), 1);
}

TEST(Dof, PickFocusNoneWhenNoLargeShip)
{
	EXPECT_EQ(dof_pick_focus({}), -1);
	SCP_vector<dof_candidate> c = {{10.0f, 20.0f, 100.0f}};
	EXPECT_EQ(dof_pick_focus(c), -1);
}

TEST(Dof, ViewZExtentOfRotatedBox)
{
	// Unit box (half size 1 x 1 x 100) 1000 m ahead, turned 90 degrees about y:
	// its long axis now lies along x, so the depth extent is only 1000 +- 1.
	vec3d corners[8];
	int i = 0;
	for (float x : {-1.0f, 1.0f})
		for (float y : {-1.0f, 1.0f})
			for (float z : {-100.0f, 100.0f})
				corners[i++] = vm_vec_new(x, y, z);

	matrix orient;
	angles a = {0.0f, 0.0f, PI_2};
	vm_angles_2_matrix(&orient, &a);

	const vec3d pos = vm_vec_new(0.0f, 0.0f, 1000.0f);
	const vec3d eye = vm_vec_new(0.0f, 0.0f, 0.0f);
	const vec3d fvec = vm_vec_new(0.0f, 0.0f, 1.0f);
	float zmin, zmax;
	dof_view_z_extent(corners, pos, vmd_identity_matrix, eye, fvec, &zmin, &zmax);
	EXPECT_NEAR(zmin, 900.0f, 0.01f);
	EXPECT_NEAR(zmax, 1100.0f, 0.01f);

	dof_view_z_extent(corners, pos, orient, eye, fvec, &zmin, &zmax);
	EXPECT_NEAR(zmin, 999.0f, 0.01f);
	EXPECT_NEAR(zmax, 1001.0f, 0.01f);
}

TEST(Dof, FarBandStartAndTail)
{
	// The spike's preferred setting: start at a third of the box, full blur at its far end.
	auto b = dof_far_band(1000.0f, 2500.0f, 1.0f / 3.0f, 1.0f);
	EXPECT_NEAR(b.start, 1500.0f, 0.01f);
	EXPECT_NEAR(b.ramp, 1000.0f, 0.01f);

	// Half tail: at zmax the blur is half of the max, so the ramp is twice as long.
	b = dof_far_band(1000.0f, 2500.0f, 1.0f / 3.0f, 0.5f);
	EXPECT_NEAR(b.ramp, 2000.0f, 0.01f);
}

TEST(Dof, FarBandEdgeCases)
{
	// Start at the far end: the ramp must stay positive, the shader divides by it.
	auto b = dof_far_band(1000.0f, 2500.0f, 1.0f, 1.0f);
	EXPECT_NEAR(b.start, 2500.0f, 0.01f);
	EXPECT_GE(b.ramp, 1.0f);

	// Tail 0: nothing at or behind the ship gets a visible blur.
	b = dof_far_band(1000.0f, 2500.0f, 0.0f, 0.0f);
	EXPECT_GT(b.ramp, 1.0e20f);

	// A box partly behind the eye starts no nearer than 1 m.
	b = dof_far_band(-300.0f, 200.0f, 0.0f, 1.0f);
	EXPECT_GE(b.start, 1.0f);
}

TEST(Dof, ModeLogic)
{
	EXPECT_FALSE(dof_mode_active(DofMode::Off, false));
	EXPECT_FALSE(dof_mode_active(DofMode::Off, true));
	EXPECT_FALSE(dof_mode_active(DofMode::Cutscenes, false));
	EXPECT_TRUE(dof_mode_active(DofMode::Cutscenes, true));
	EXPECT_TRUE(dof_mode_active(DofMode::Always, false));
	EXPECT_TRUE(dof_mode_active(DofMode::Always, true));
}

TEST(Dof, OptionsExistAndDefaultOff)
{
	auto* mgr = options::OptionsManager::instance();
	for (const char* key : {"Graphics.DepthOfField", "Graphics.DepthOfFieldMaxBlur", "Graphics.DepthOfFieldStart",
			 "Graphics.DepthOfFieldTail", "Graphics.DepthOfFieldSky"}) {
		EXPECT_NE(mgr->getOptionByKey(key), nullptr) << key;
	}
	dof_frame f;
	EXPECT_FALSE(dof_get_frame(&f));
}

TEST(Dof, EveryOptionRequiresOpenGL)
{
	auto* mgr = options::OptionsManager::instance();
	dof_set_requirements();
	for (const char* key : {"Graphics.DepthOfField", "Graphics.DepthOfFieldMaxBlur", "Graphics.DepthOfFieldStart",
			 "Graphics.DepthOfFieldTail", "Graphics.DepthOfFieldSky"}) {
		auto requirement = mgr->getRequirement(key);
		ASSERT_TRUE(requirement.has_value()) << key;
		EXPECT_EQ(requirement->first, "opengl") << key;
		EXPECT_FALSE(requirement->second.empty()) << key;
	}
	EXPECT_FALSE(mgr->getRequirement("Graphics.VSync").has_value());
	mgr->clearRequirements();
}
