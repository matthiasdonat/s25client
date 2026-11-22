// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AddonHelperFunctions.h"
#include "Game.h"
#include "GameCommands.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "PlayerInfo.h"
#include "Savegame.h"
#include "SerializedGameData.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobUsual.h"
#include "helpers/serializeEnums.h"
#include "worldFixtures/MockLocalGameState.h"
#include "gameTypes/VisualSettings.h"
#include "world/GameWorld.h"
#include "addons/const_addons.h"
#include "gameData/ArcherTowerConsts.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/JobConsts.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/JobTypes.h"
#include "test/testConfig.h"
#include <s25util/Serializer.h>
#include <boost/test/unit_test.hpp>
#include <string>

// LCOV_EXCL_START
BOOST_TEST_DONT_PRINT_LOG_VALUE(BuildingType)
// LCOV_EXCL_STOP

namespace {
class TestChangeBuildOrder : public gc::ChangeBuildOrder
{
public:
    TestChangeBuildOrder(bool custom, const BuildOrders& data) : gc::ChangeBuildOrder(custom, data) {}
};
class TestSetAllInventorySettings : public gc::SetAllInventorySettings
{
public:
    TestSetAllInventorySettings(MapPoint pt, bool isJob, std::vector<InventorySetting> states)
        : gc::SetAllInventorySettings(pt, isJob, std::move(states))
    {}
};

constexpr unsigned lastGameCommandVersionBeforeArcher = 2;

/// Loads the savegame of the last version without the archer tower
std::unique_ptr<Game> loadV15Fixture(Savegame& save)
{
    const auto path = rttr::test::rttrBaseDir / "tests" / "testData" / "savegames" / "v15-wine.sav";
    BOOST_TEST_REQUIRE(save.load(path, SaveGameDataToLoad::All));
    std::vector<PlayerInfo> players;
    for(unsigned i = 0; i < save.getNumPlayers(); ++i)
        players.emplace_back(save.getPlayer(i));
    auto game = std::make_unique<Game>(save.ggs, save.start_gf, players);
    MockLocalGameState lgs;
    save.sgd.readSnapshot(*game, lgs);
    // Everything was read, so the (shorter) old arrays were handled correctly
    BOOST_TEST(save.sgd.getBytesLeft() == 0u);
    game->world_.initAfterLoad();
    return game;
}
} // namespace

BOOST_AUTO_TEST_SUITE(ArcherTowerSuite)

BOOST_AUTO_TEST_CASE(LoadV15Savegame)
{
    Savegame save;
    const auto game = loadV15Fixture(save);
    const GamePlayer& player = game->world_.getPlayer(0);
    BOOST_TEST(player.getInventory().people[Job::Archer] == 0u);
    const auto* hq = game->world_.getSpecObj<nobBaseWarehouse>(player.getHQPos());
    BOOST_TEST_REQUIRE(hq);
    BOOST_TEST(hq->getNumRealFigures(Job::Archer) == 0u);
    BOOST_TEST(player.getBuildingRegister().getBuildings(BuildingType::ArcherTower).empty());
    BOOST_TEST(player.getBuildingRegister().getBuildings(BuildingType::Temple).size() == 1u);

    VisualSettings settings;
    player.fillVisualSettings(settings);
    BOOST_TEST(settings.build_order.back() == BuildingType::ArcherTower);
    BOOST_TEST(settings.build_order == GamePlayer::getStandardBuildOrder(), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(SaveLoadRoundTripV16)
{
    Savegame save;
    const auto game = loadV15Fixture(save);
    SerializedGameData first;
    first.makeSnapshot(*game);

    std::vector<PlayerInfo> players;
    for(unsigned i = 0; i < save.getNumPlayers(); ++i)
        players.emplace_back(save.getPlayer(i));
    Game game2(save.ggs, save.start_gf, players);
    MockLocalGameState lgs;
    first.readSnapshot(game2, lgs);
    game2.world_.initAfterLoad();
    SerializedGameData second;
    second.makeSnapshot(game2);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(second.getData(), second.getData() + second.getLength(), first.getData(),
                                    first.getData() + first.getLength());
}

BOOST_AUTO_TEST_CASE(V15ChangeBuildOrderCommandParses)
{
    BuildOrders order = GamePlayer::getStandardBuildOrder();
    gc::Deserializer current;
    TestChangeBuildOrder(true, order).serialize(current);
    // Old format: no entry for the (last) archer tower
    gc::Deserializer old(lastGameCommandVersionBeforeArcher);
    old.pushRawData(current.getData(), current.getLength() - 1);

    const auto cmd = gc::GameCommand::deserialize(old);
    BOOST_TEST(old.getBytesLeft() == 0u);
    Serializer reserialized;
    cmd->serialize(reserialized);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(reserialized.getData(), reserialized.getData() + reserialized.getLength(),
                                    current.getData(), current.getData() + current.getLength());
}

BOOST_AUTO_TEST_CASE(V15SetAllInventorySettingsCommandParses)
{
    const MapPoint pt(3, 4);
    std::vector<InventorySetting> states(helpers::NumEnumValues_v<Job>);
    for(unsigned i = 0; i + 1 < states.size(); ++i)
        states[i] = InventorySetting(EInventorySetting::Collect);
    gc::Deserializer current;
    TestSetAllInventorySettings(pt, true, states).serialize(current);
    gc::Deserializer old(lastGameCommandVersionBeforeArcher);
    old.pushRawData(current.getData(), current.getLength() - 1);

    const auto cmd = gc::GameCommand::deserialize(old);
    BOOST_TEST(old.getBytesLeft() == 0u);
    Serializer reserialized;
    cmd->serialize(reserialized);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(reserialized.getData(), reserialized.getData() + reserialized.getLength(),
                                    current.getData(), current.getData() + current.getLength());
}

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
