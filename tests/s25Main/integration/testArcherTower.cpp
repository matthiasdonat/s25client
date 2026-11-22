// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AddonHelperFunctions.h"
#include "GlobalGameSettings.h"
#include "addons/const_addons.h"
#include "gameData/ArcherTowerConsts.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/JobConsts.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/JobTypes.h"
#include <s25util/Serializer.h>
#include <boost/test/unit_test.hpp>
#include <string>

BOOST_AUTO_TEST_SUITE(ArcherTowerSuite)

BOOST_AUTO_TEST_CASE(AddonRegistered)
{
    GlobalGameSettings ggs;
    BOOST_TEST_REQUIRE(ggs.getAddon(AddonId::ARCHER_TOWER) != nullptr);
    BOOST_TEST(ggs.getSelection(AddonId::ARCHER_TOWER) == 0u);
    BOOST_TEST(!isArcherTowerActive(ggs));
}

BOOST_AUTO_TEST_CASE(EnablingArcherEnablesWine)
{
    GlobalGameSettings ggs;
    ggs.setSelection(AddonId::ARCHER_TOWER, 1);
    BOOST_TEST(ggs.isEnabled(AddonId::WINE));
    BOOST_TEST(isArcherTowerActive(ggs));
}

BOOST_AUTO_TEST_CASE(DisablingWineKeepsArcherInactive)
{
    GlobalGameSettings ggs;
    ggs.setSelection(AddonId::ARCHER_TOWER, 1);
    ggs.setSelection(AddonId::WINE, 0);
    BOOST_TEST(!isArcherTowerActive(ggs));
}

BOOST_AUTO_TEST_CASE(SettingsSerializationKeepsExactState)
{
    GlobalGameSettings ggs;
    ggs.setSelection(AddonId::ARCHER_TOWER, 1);
    ggs.setSelection(AddonId::WINE, 0);
    Serializer ser;
    ggs.serialize(ser);

    GlobalGameSettings loaded;
    loaded.deserialize(ser);
    BOOST_TEST(loaded.isEnabled(AddonId::ARCHER_TOWER));
    BOOST_TEST(!loaded.isEnabled(AddonId::WINE));
    BOOST_TEST(!isArcherTowerActive(loaded));
}

BOOST_AUTO_TEST_CASE(ArcherTowerTables)
{
    const BuildingCost cost = BUILDING_COSTS[BuildingType::ArcherTower];
    BOOST_TEST(cost.boards == 3u);
    BOOST_TEST(cost.stones == 4u);
    BOOST_TEST((BUILDING_SIZE[BuildingType::ArcherTower] == BuildingQuality::House));
    BOOST_TEST_REQUIRE(BUILDING_NAMES[BuildingType::ArcherTower] != nullptr);
    BOOST_TEST(std::string(BUILDING_NAMES[BuildingType::ArcherTower]) != "");
    BOOST_TEST_REQUIRE(BUILDING_HELP_STRINGS[BuildingType::ArcherTower] != nullptr);
    BOOST_TEST(std::string(BUILDING_HELP_STRINGS[BuildingType::ArcherTower]) != "");
    BOOST_TEST(JOB_NAMES[Job::Archer] != "");
    BOOST_TEST((JOB_CONSTS[Job::Archer].tool == GoodType::Bow));

    const BldWorkDescription& desc = BLD_WORK_DESC[BuildingType::ArcherTower];
    BOOST_TEST_REQUIRE(static_cast<bool>(desc.job));
    BOOST_TEST((desc.job.value() == Job::Archer));
    BOOST_TEST_REQUIRE(desc.waresNeeded.size() == 1u);
    BOOST_TEST((desc.waresNeeded[0] == GoodType::Wine));
    BOOST_TEST(desc.numSpacesPerWare == ARCHER_TOWER_WINE_SPACES);

    BOOST_TEST(ARCHER_TOWER_RANGE == 7u);
    BOOST_TEST(MAX_ARCHERS_PER_TOWER == 4u);
    BOOST_TEST(ARCHER_TOWER_WINE_SPACES == 3u);
    BOOST_TEST(ARCHER_BOOST_SHOTS == 10u);
    BOOST_TEST(ARCHER_HIT_CHANCE_PERCENT == 70u);
    BOOST_TEST(ARCHER_SHOT_INTERVAL_GFS == 140u);
    BOOST_TEST(ARCHER_BOOSTED_SHOT_INTERVAL_GFS == 100u);
}

BOOST_AUTO_TEST_CASE(ArcherTowerProperties)
{
    BOOST_TEST(!BuildingProperties::isMilitary(BuildingType::ArcherTower));
    BOOST_TEST(BuildingProperties::isUsual(BuildingType::ArcherTower));
    BOOST_TEST(BuildingProperties::isValid(BuildingType::ArcherTower));
    BOOST_TEST(!isSoldier(Job::Archer));
}

BOOST_AUTO_TEST_CASE(ArcherTowerUnusedWithoutAddon)
{
    GlobalGameSettings ggs;
    BOOST_TEST(makeIsUnusedBuilding(ggs)(BuildingType::ArcherTower));
    BOOST_TEST(makeIsUnusedJob(ggs)(Job::Archer));
    ggs.setSelection(AddonId::ARCHER_TOWER, 1);
    BOOST_TEST(!makeIsUnusedBuilding(ggs)(BuildingType::ArcherTower));
    BOOST_TEST(!makeIsUnusedJob(ggs)(Job::Archer));
}

BOOST_AUTO_TEST_SUITE_END()
