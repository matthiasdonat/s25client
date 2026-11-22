// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameData/GameConsts.h"
#include <array>
#include <chrono>

/// Maximum distance (in fields) to a target the archers of a tower shoot at
constexpr unsigned ARCHER_TOWER_RANGE = 7;
/// Radius (in fields) around an occupied tower that is made visible for its owner
constexpr unsigned VISUALRANGE_ARCHERTOWER = ARCHER_TOWER_RANGE;
/// Number of archers a tower holds
constexpr unsigned MAX_ARCHERS_PER_TOWER = 4;
/// Number of wine a tower can store
constexpr unsigned ARCHER_TOWER_WINE_SPACES = 3;
/// Number of fast shots one wine gives to an archer
constexpr unsigned ARCHER_BOOST_SHOTS = 10;
/// Chance (in percent) that a shot hits its target
constexpr unsigned ARCHER_HIT_CHANCE_PERCENT = 70;
/// Damage (in hitpoints) of a hit
constexpr unsigned ARCHER_HIT_DAMAGE = 1;
/// Duration of the shooting animation in game frames
constexpr unsigned ARCHER_SHOT_ANIMATION_GFS = 16;
/// Time between two searches for a target while there is none, in game frames
constexpr unsigned ARCHER_SEARCH_INTERVAL_GFS = 20;
/// Offsets (from the building position) where the archers of a tower stand on the roof, indexed by slot
constexpr std::array<std::array<int, 2>, MAX_ARCHERS_PER_TOWER> ARCHER_ROOF_OFFSETS{
  {{-7, -36}, {7, -36}, {-7, -30}, {7, -30}}};
/// Center of the wooden platform the archers stand on (relative to the building position) and half its size
constexpr std::array<int, 2> ARCHER_PLATFORM_CENTER{0, -32};
constexpr int ARCHER_PLATFORM_HALF_WIDTH = 15;
constexpr int ARCHER_PLATFORM_HALF_HEIGHT = 7;
/// Time between two shots of an archer without wine, in game frames
constexpr unsigned ARCHER_SHOT_INTERVAL_GFS = durationToGfs(std::chrono::seconds(7));
/// Time between two shots of an archer while the wine boost lasts, in game frames
constexpr unsigned ARCHER_BOOSTED_SHOT_INTERVAL_GFS = durationToGfs(std::chrono::seconds(5));
