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
#include "buildings/nobArcherTower.h"
#include "buildings/nobBaseWarehouse.h"
#include "factories/BuildingFactory.h"
#include "figures/nofArcher.h"
#include "worldFixtures/WorldWithGCExecution.h"
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
BOOST_TEST_DONT_PRINT_LOG_VALUE(Visibility)
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

namespace {
/// World with archer tower addon where the HQ has helpers (but no bows) for recruiting archers
template<class T_World>
struct ArcherTowerFixtureT : T_World
{
    using T_World::BuildRoad;
    using T_World::curPlayer;
    using T_World::ggs;
    using T_World::hqPos;
    using T_World::world;

    ArcherTowerFixtureT()
    {
        ggs.setSelection(AddonId::ARCHER_TOWER, 1);
        hq = world.template getSpecObj<nobBaseWarehouse>(hqPos);
        hq->addToInventory(PeopleCounts::make(Job::Helper, 10), true);
    }

    void addBows(unsigned num) { hq->addToInventory(GoodCounts::make(GoodType::Bow, num), true); }

    /// Creates a tower in the given distance east of the HQ and connects it with a road
    nobArcherTower* createTower(unsigned distance = 2)
    {
        const MapPoint towerPos = world.makeMapPoint(hqPos + Position(distance, 0));
        auto* tower = static_cast<nobArcherTower*>(
          BuildingFactory::createBuilding(world, BuildingType::ArcherTower, towerPos, curPlayer, Nation::Romans));
        BuildRoad(world.getNeighbour(towerPos, Direction::SouthEast), false,
                  std::vector<Direction>(distance, Direction::West));
        return tower;
    }

    nobBaseWarehouse* hq;
};
using ArcherTowerFixture = ArcherTowerFixtureT<WorldWithGCExecution1P>;
} // namespace

BOOST_AUTO_TEST_SUITE(ArcherTowerSuite)

BOOST_FIXTURE_TEST_CASE(TowerRequestsFourArchers, ArcherTowerFixture)
{
    addBows(MAX_ARCHERS_PER_TOWER);
    nobArcherTower* tower = createTower();
    BOOST_TEST(tower->getNumArchers() == MAX_ARCHERS_PER_TOWER);
    RTTR_EXEC_TILL(1500, tower->getNumPresentArchers() == MAX_ARCHERS_PER_TOWER);
    BOOST_TEST(tower->hasWorker());
    BOOST_TEST(!tower->isDoorOpen());
    for(unsigned slot = 0; slot < MAX_ARCHERS_PER_TOWER; ++slot)
    {
        BOOST_TEST_REQUIRE(tower->getArcher(slot));
        BOOST_TEST(tower->getSlotOf(*tower->getArcher(slot)) == slot);
    }
    BOOST_TEST(hq->getNumRealFigures(Job::Archer) == 0u);
    BOOST_TEST(hq->getNumRealWares(GoodType::Bow) == 0u);
}

BOOST_FIXTURE_TEST_CASE(NoArcherWithoutBow, ArcherTowerFixture)
{
    nobArcherTower* tower = createTower();
    RTTR_SKIP_GFS(500);
    BOOST_TEST(tower->getNumArchers() == 0u);
    BOOST_TEST(!tower->hasWorker());
    BOOST_TEST(tower->isDoorOpen());
    // Archers are still wanted and arrive once bows are available
    addBows(MAX_ARCHERS_PER_TOWER);
    RTTR_EXEC_TILL(3000, tower->getNumPresentArchers() == MAX_ARCHERS_PER_TOWER);
}

BOOST_FIXTURE_TEST_CASE(OnlyMissingSlotIsRequested, ArcherTowerFixture)
{
    addBows(MAX_ARCHERS_PER_TOWER);
    nobArcherTower* tower = createTower();
    RTTR_EXEC_TILL(1500, tower->getNumPresentArchers() == MAX_ARCHERS_PER_TOWER);
    tower->getArcher(1)->abrogate();
    BOOST_TEST(tower->getNumArchers() == MAX_ARCHERS_PER_TOWER - 1);
    BOOST_TEST(!tower->getArcher(1));
    BOOST_TEST(tower->hasWorker());
    BOOST_TEST(!tower->isDoorOpen());

    // Only one replacement is recruited although there are more bows
    addBows(3);
    RTTR_EXEC_TILL(3000, tower->getNumArchers() == MAX_ARCHERS_PER_TOWER);
    RTTR_SKIP_GFS(500);
    BOOST_TEST(tower->getNumArchers() == MAX_ARCHERS_PER_TOWER);
    BOOST_TEST(hq->getNumRealWares(GoodType::Bow) == 2u);
    BOOST_TEST_REQUIRE(tower->getArcher(1));
    BOOST_TEST(tower->getSlotOf(*tower->getArcher(1)) == 1u);
}

BOOST_FIXTURE_TEST_CASE(DoorOpensWhenLastArcherIsLost, ArcherTowerFixture)
{
    addBows(1);
    nobArcherTower* tower = createTower();
    RTTR_EXEC_TILL(1500, tower->getNumPresentArchers() == 1u);
    BOOST_TEST(!tower->isDoorOpen());
    tower->getArcher(0)->abrogate();
    BOOST_TEST(!tower->hasWorker());
    BOOST_TEST(tower->isDoorOpen());
    // New archer closes the door again
    addBows(1);
    RTTR_EXEC_TILL(1500, tower->getNumPresentArchers() == 1u);
    BOOST_TEST(!tower->isDoorOpen());
}

BOOST_FIXTURE_TEST_CASE(DestroyedTowerReleasesArchers, ArcherTowerFixture)
{
    addBows(MAX_ARCHERS_PER_TOWER);
    nobArcherTower* tower = createTower();
    const MapPoint towerPos = tower->getPos();
    RTTR_EXEC_TILL(1500, tower->getNumPresentArchers() == MAX_ARCHERS_PER_TOWER);
    std::vector<const nofArcher*> archers;
    for(unsigned slot = 0; slot < MAX_ARCHERS_PER_TOWER; ++slot)
        archers.push_back(tower->getArcher(slot));
    this->DestroyBuilding(towerPos);
    for(const nofArcher* archer : archers)
        BOOST_TEST(archer->isWandering());
    // Nothing should break while the archers walk home or wander and no new archers are requested
    addBows(MAX_ARCHERS_PER_TOWER);
    RTTR_SKIP_GFS(1500);
    BOOST_TEST(hq->getNumRealWares(GoodType::Bow) == MAX_ARCHERS_PER_TOWER);
}

BOOST_FIXTURE_TEST_CASE(DestroyedTowerWithArchersOnTheWay, ArcherTowerFixture)
{
    nobArcherTower* tower = createTower();
    const MapPoint towerPos = tower->getPos();
    this->DestroyBuilding(towerPos);
    // Pending archer requests must be gone
    addBows(MAX_ARCHERS_PER_TOWER);
    RTTR_SKIP_GFS(1000);
    BOOST_TEST(hq->getNumRealWares(GoodType::Bow) == MAX_ARCHERS_PER_TOWER);
}

BOOST_FIXTURE_TEST_CASE(TowerOrdersUpToThreeWine, ArcherTowerFixture)
{
    addBows(MAX_ARCHERS_PER_TOWER);
    hq->addToInventory(GoodCounts::make(GoodType::Wine, 10), true);
    nobArcherTower* tower = createTower();
    RTTR_EXEC_TILL(3000, tower->getNumWares(0) == ARCHER_TOWER_WINE_SPACES);
    RTTR_SKIP_GFS(1000);
    BOOST_TEST(tower->getNumWares(0) == ARCHER_TOWER_WINE_SPACES);
    BOOST_TEST(hq->getNumRealWares(GoodType::Wine) == 10u - ARCHER_TOWER_WINE_SPACES);
}

BOOST_FIXTURE_TEST_CASE(WineGoesToTempleAndTower, ArcherTowerFixture)
{
    addBows(MAX_ARCHERS_PER_TOWER);
    hq->addToInventory(GoodCounts::make(GoodType::Wine, 10), true);
    hq->addToInventory(PeopleCounts::make(Job::TempleServant, 1), true);
    nobArcherTower* tower = createTower();
    const MapPoint templePos = hqPos - MapPoint(2, 0);
    const auto* temple = static_cast<nobUsual*>(
      BuildingFactory::createBuilding(world, BuildingType::Temple, templePos, curPlayer, Nation::Romans));
    BuildRoad(world.getNeighbour(templePos, Direction::SouthEast), false, std::vector<Direction>(2, Direction::East));
    RTTR_EXEC_TILL(4000, tower->getNumWares(0) == ARCHER_TOWER_WINE_SPACES && temple->getNumWares(0) > 0);
}

BOOST_AUTO_TEST_CASE(V15SaveStillDeliversWineToTemple)
{
    Savegame save;
    const auto game = loadV15Fixture(save);
    const auto& wineClients = game->world_.getPlayer(0).getClientBuildings(GoodType::Wine);
    BOOST_TEST(std::count(wineClients.begin(), wineClients.end(), BuildingType::Temple) == 1);
    BOOST_TEST(std::count(wineClients.begin(), wineClients.end(), BuildingType::ArcherTower) == 1);
}

namespace {
/// Tower far away from the HQ so that some nodes in range of the tower are not visible by other means.
/// Needs a bigger world than the default for a single player
struct FarTowerFixture : ArcherTowerFixtureT<WorldWithGCExecution<1, 40, 20>>
{
    static constexpr unsigned towerDistance = 6;

    FarTowerFixture() { ggs.exploration = Exploration::FogOfWar; }

    nobArcherTower* createFarTower() { return createTower(towerDistance); }

    /// Node at the maximum vision distance east of the tower
    MapPoint farPoint(const nobArcherTower& tower) const
    {
        return world.makeMapPoint(tower.getPos() + Position(VISUALRANGE_ARCHERTOWER, 0));
    }

    Visibility visibility(MapPoint pt) const { return world.getNode(pt).fow[curPlayer].visibility; }
};
} // namespace

BOOST_FIXTURE_TEST_CASE(TowerRevealsRadiusSevenWithFirstArcher, FarTowerFixture)
{
    addBows(1);
    nobArcherTower* tower = createFarTower();
    const MapPoint far = farPoint(*tower);
    BOOST_TEST_REQUIRE(visibility(far) != Visibility::Visible);
    RTTR_EXEC_TILL(2000, tower->getNumPresentArchers() == 1u);
    BOOST_TEST(visibility(far) == Visibility::Visible);
    const MapPoint tooFar = world.makeMapPoint(far + Position(1, 0));
    BOOST_TEST(visibility(tooFar) != Visibility::Visible);
}

BOOST_FIXTURE_TEST_CASE(VisionRemovedWhenLastArcherIsLost, FarTowerFixture)
{
    addBows(1);
    nobArcherTower* tower = createFarTower();
    const MapPoint far = farPoint(*tower);
    RTTR_EXEC_TILL(2000, tower->getNumPresentArchers() == 1u);
    BOOST_TEST_REQUIRE(visibility(far) == Visibility::Visible);
    tower->getArcher(0)->abrogate();
    BOOST_TEST(visibility(far) != Visibility::Visible);
}

BOOST_FIXTURE_TEST_CASE(VisionStaysWhenOneOfTwoLost, FarTowerFixture)
{
    addBows(2);
    nobArcherTower* tower = createFarTower();
    const MapPoint far = farPoint(*tower);
    RTTR_EXEC_TILL(3000, tower->getNumPresentArchers() == 2u);
    tower->getArcher(0)->abrogate();
    BOOST_TEST(visibility(far) == Visibility::Visible);
}

BOOST_FIXTURE_TEST_CASE(VisionRemovedWhenTowerIsDestroyed, FarTowerFixture)
{
    addBows(2);
    nobArcherTower* tower = createFarTower();
    const MapPoint far = farPoint(*tower);
    const MapPoint towerPos = tower->getPos();
    RTTR_EXEC_TILL(3000, tower->getNumPresentArchers() == 2u);
    BOOST_TEST_REQUIRE(visibility(far) == Visibility::Visible);
    this->DestroyBuilding(towerPos);
    BOOST_TEST(visibility(far) != Visibility::Visible);
}

BOOST_FIXTURE_TEST_CASE(SaveLoadKeepsSlots, ArcherTowerFixture)
{
    addBows(MAX_ARCHERS_PER_TOWER);
    nobArcherTower* tower = createTower();
    RTTR_EXEC_TILL(1500, tower->getNumPresentArchers() == MAX_ARCHERS_PER_TOWER);
    tower->getArcher(1)->abrogate();
    tower->getArcher(3)->abrogate();

    SerializedGameData sgd;
    sgd.makeSnapshot(*game);
    std::vector<PlayerInfo> players;
    players.emplace_back(world.getPlayer(0));
    Game game2(ggs, em.getCurrentGF(), players);
    MockLocalGameState lgs;
    sgd.readSnapshot(game2, lgs);
    game2.world_.initAfterLoad();

    const auto* loaded = game2.world_.getSpecObj<nobArcherTower>(tower->getPos());
    BOOST_TEST_REQUIRE(loaded);
    BOOST_TEST(loaded->getNumArchers() == 2u);
    BOOST_TEST(loaded->getArcher(0));
    BOOST_TEST(!loaded->getArcher(1));
    BOOST_TEST(loaded->getArcher(2));
    BOOST_TEST(!loaded->getArcher(3));
    BOOST_TEST(loaded->getSlotOf(*loaded->getArcher(2)) == 2u);
    BOOST_TEST(!loaded->isDoorOpen());
}

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
