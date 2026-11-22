# Archer Tower Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an optional lobby addon with a medium building ("Archer Tower", 4 archers) that shoots enemy soldiers and scouts within 7 fields, using wine as a fire-rate booster.

**Architecture:** `nobArcherTower : nobUsual` with four worker slots (slot 0 is `nobUsual::worker`, slots 1-3 live in the subclass) and a new worker `nofArcher : nofBuildingWorker`. Not a military building: no territory, not attackable, burns down when territory is lost. Shooting is event-driven like `nofCatapultMan`; damage reaches soldiers/scouts through a new ranged-hit API. Existing wares (Bow, Wine) and the LookoutTower sprite are reused.

**Tech Stack:** C++17, Boost.Test, CMake (Unix Makefiles, existing `build/` dir), clang-format, gettext (`_()`).

**Spec:** `docs/superpowers/specs/2026-11-22-archer-tower-design.md` (approved). Background and recon: `docs/superpowers/specs/2026-11-22-archer-tower-brainstorming.md`.

## Status (Stand 2026-11-22)

- [x] Task 1-13 erledigt und committet (Ledger: `.superpowers/sdd/2026-11-22-archer-tower/progress.md`, dort auch alle Rulings).
- [x] Task 14 erledigt: AI-Test, PR-Beschreibung (`docs/superpowers/archer-tower-pr-description.md`). `clang-format` ist gelaufen (`git clang-format`, nur auf geänderte Zeilen).
- [x] Abschlussreview: Selbstprüfung für Spec und KI; der Reviewer-Subagent läuft noch und konnte nicht abgebrochen werden (Befunde danach prüfen).
- Abweichungen: Task 12 ohne UI-Test (Test-Fixtures haben keine Spieldaten), Task 13 ohne manuelle Pruefung im Spiel (Dach-Offsets sind Platzhalter), Task 10 ohne statistischen Trefferchance-Test.

## Global Constraints

- Branch `feature/archer-tower`; one large PR to Return-To-The-Roots/s25client; commit per task (imperative one-line messages, repo style).
- Build must be warning-free (`RTTR_ENABLE_WERROR=ON`); format with `clang-format` (`make clangFormat`); no magic constants (named `constexpr`), `std::array`, doxygen one-liners on new functions; naming: lowerCamelCase methods/locals, members suffixed `_` in new classes.
- Tests for every feature (Boost.Test, small, RNG-robust). All new tests live in suite `ArcherTowerSuite` in `tests/s25Main/integration/testArcherTower.cpp` unless a task names another file.
- Range 7 fields (`calcDistance`); 4 archers; max 3 wine; 1 wine = 10 boosted shots; damage always 1; hit chance 70 %; shot interval 7 s (normal) / 5 s (boosted), converted with `durationToGfs` (140 / 100 GF); cost 3 boards + 4 stones; `BuildingQuality::House`; addon ID `0x01400000`; `currentGameDataVersion` 15 -> 16.
- Deterministic: state changes only in events / `GameCommand::execute`, randomness only via `RANDOM_*`.
- Soldiers inside buildings, soldiers in `noFighting`, passive soldiers walking to a building, and civilians are never targets.
- New enum values are appended at the end (`BuildingType`, `Job`, `GO_Type`, `GCType`, `nofBuildingWorker::State`).
- AI does not build the tower (not in `buildingsWanted`), but must not crash when it exists.
- Test commands: `cmake --build build --target Test_integration -j8` then `build/Contents/MacOS/Test_integration --run_test=ArcherTowerSuite`.

## Review Focus

1. Tower destroyed or archer lost while a shot event is pending: no dangling event, no hit applied, no assert.
2. Target dies, enters a building or starts a fight between aiming and impact: shot is dropped silently (task 9).
3. Armored enemy soldier hit: armor is lost first, hitpoints unchanged (task 8).
4. `ARCHER_TOWER` on with `WINE` off arriving via Lua, replay or network: tower is unavailable, nothing crashes (task 2).
5. Save/load while archers are mid-cooldown or boosted: slot index, boosted-shot counter and pending wine survive; v15 saves load with empty tower/archer data and still deliver wine to temples (tasks 4, 6, 10).

---

### Task 1: Baseline, doc commit, v15 save fixture

**Files:**
- Create: `tests/testData/savegames/v15-wine.sav` (+ `.license` file in the repo's REUSE style, see `tests/testData/200kGFs.rpl.license`)
- Test: `tests/s25Main/integration/testArcherTower.cpp` (created here, registered in `tests/s25Main/integration/CMakeLists.txt` if sources are listed explicitly)

**Interfaces:**
- Produces: later tasks load `tests/testData/savegames/v15-wine.sav` via the existing test-data helper used for `200kGFs.rpl` (find it with `grep -rn 200kGFs tests`).

- [x] **Step 1: Commit the design docs.** `git add docs/superpowers && git commit -m "Add archer tower design and planning docs"`
- [x] **Step 2: Build baseline.** `cmake --build build -j8`; run `Test_integration`, `Test_simple`, `Test_UI`, `Test_lua`. Expected: all pass. Record failures that exist before any change in the PR description draft.
- [x] **Step 3: Generate the v15 fixture on the unmodified code.** Write a throwaway test (do not commit it) modelled on `testSerialization.cpp:327-350` that builds a 1-player world with Wine addon enabled, a HQ, and a Temple, saves with `Savegame::save`, and copies the file to `tests/testData/savegames/v15-wine.sav`. Confirm the file header carries game data version 15.
- [x] **Step 4: Create `testArcherTower.cpp` with an empty suite** (`BOOST_AUTO_TEST_SUITE(ArcherTowerSuite)` / `END`) and make it build.
- [x] **Step 5: Commit** fixture + empty test file: `git commit -m "Add v15 savegame fixture and archer tower test suite"`

### Task 2: Addon and Wine dependency

**Files:**
- Create: `libs/s25main/addons/AddonArcherTower.h`
- Modify: `libs/s25main/addons/const_addons.h` (add `ARCHER_TOWER = 0x01400000`), `libs/s25main/addons/Addons.h`, `libs/s25main/GlobalGameSettings.cpp` (registration list and normalisation), `libs/s25main/AddonHelperFunctions.h/.cpp`
- Test: `tests/s25Main/integration/testArcherTower.cpp`

**Interfaces:**
- Produces: `class AddonArcherTower : public AddonBool` (name `_("Enable archer tower")`, group `AddonGroup::Economy`, default off); `bool isArcherTowerActive(const GlobalGameSettings&)` in `AddonHelperFunctions.h` (true only if `ARCHER_TOWER` and `WINE` are both enabled); extension of `makeIsUnusedBuilding` / `makeIsUnusedJob` so that `ArcherTower` / `Archer` are unused unless `isArcherTowerActive` (wired in Task 3 when the enum values exist).

- [x] **Step 1: Failing tests** (names): `AddonRegistered` (`ggs.getAddon(AddonId::ARCHER_TOWER)` exists, default status 0); `EnablingArcherEnablesWine` (`ggs.setSelection(AddonId::ARCHER_TOWER, 1)` => `ggs.isEnabled(AddonId::WINE)`); `DisablingWineKeepsArcherInactive` (after enabling both, `setSelection(WINE, 0)` => `isArcherTowerActive(ggs) == false`); `SettingsSerializationNormalises` (serialize a `GlobalGameSettings` with ARCHER on and WINE off, deserialize into a fresh one => WINE on).
- [x] **Step 2: Run to confirm failure** (unknown `AddonId::ARCHER_TOWER`).
- [x] **Step 3: Implement** the addon class, ID, include, registration in the `mp_list` in `GlobalGameSettings.cpp`; normalisation: in `GlobalGameSettings::setSelection`, `deserialize` and `loadSettings`, enabling `ARCHER_TOWER` also sets `WINE` (use one private helper, no duplication); `isArcherTowerActive` in `AddonHelperFunctions`. Also normalise in `iwAddons.cpp:107-162` so the Wine checkbox updates visibly, and in `LuaInterfaceSettings.cpp:111`.
- [x] **Step 4: Run** `ArcherTowerSuite` and `Test_UI` (`AddonWindow` counts addons dynamically). Expected: PASS.
- [x] **Step 5: Commit** `git commit -m "Add archer tower addon with wine dependency"`

### Task 3: Enum values, tables, constants, Lua

**Files:**
- Create: `libs/s25main/gameData/ArcherTowerConsts.h`
- Modify: `gameTypes/BuildingType.h` (append `ArcherTower`, update `maxEnumValue`), `gameTypes/JobTypes.h` (append `Archer` after `LeatherWorker`, update `maxEnumValue`), `gameData/BuildingConsts.h/.cpp` (`BUILDING_COSTS` `{3, 4}`, `BUILDING_SIZE` House, `BLD_WORK_DESC`, `BUILDING_SIGN_CONSTS`, `BUILDING_SMOKE_CONSTS` if sized, names/help strings, `DoorConsts.h` entries copied from `LookoutTower`), `gameData/JobConsts.cpp` (`{GoodType::Bow, 0, 0, 0}`, sprite `{false, 20}` = Hunter, name `gettext_noop("Archer")`), `world/GameWorldView.cpp:717-774` (`getBuildingRadius` case returning `ARCHER_TOWER_RANGE`), `lua/LuaInterfaceGame.cpp` (`ADD_LUA_CONST(ArcherTower)`, `ADD_LUA_CONST(Archer)`), `doc/lua/constants.md`, `AddonHelperFunctions.cpp` (unused filters), `ingameWindows/iwWares.cpp` (`JOB_DISPLAY_ORDER`), `Loader.cpp:216-224` (`getJobTex` alias Archer -> Hunter icon).
- Test: `testArcherTower.cpp`, `tests/s25Main/lua/testLuaConstants.cpp`, `tests/s25Main/integration/testBuildingRadius.cpp`

**Interfaces:**
- Produces in `ArcherTowerConsts.h` (namespace-free `constexpr`, header-only):
  `ARCHER_TOWER_RANGE = 7u`, `VISUALRANGE_ARCHERTOWER = ARCHER_TOWER_RANGE`, `MAX_ARCHERS_PER_TOWER = 4u`, `ARCHER_TOWER_WINE_SPACES = 3u`, `ARCHER_BOOST_SHOTS = 10u`, `ARCHER_HIT_CHANCE_PERCENT = 70u`, `ARCHER_SHOT_INTERVAL_GFS = durationToGfs(7s)`, `ARCHER_BOOSTED_SHOT_INTERVAL_GFS = durationToGfs(5s)`.
- `BLD_WORK_DESC[ArcherTower] = {Job::Archer, std::nullopt, WaresNeeded(GoodType::Wine), ARCHER_TOWER_WINE_SPACES}`.
- `BuildingProperties::isMilitary(ArcherTower) == false`, `isUsual == true`, `isValid == true`.

- [x] **Step 1: Failing tests:** `ArcherTowerTables` (`BUILDING_COSTS[ArcherTower]` equals boards 3 / stones 4; `BUILDING_SIZE` House; `BUILDING_NAMES`/`BUILDING_HELP_STRINGS` non-null and non-empty; `JOB_NAMES[Archer]` non-null; `JOB_CONSTS[Archer].tool == GoodType::Bow`; `ARCHER_SHOT_INTERVAL_GFS == 140`, `ARCHER_BOOSTED_SHOT_INTERVAL_GFS == 100`); `ArcherTowerProperties` (not military, usual, valid; `Job::Archer` is not `isSoldier`); `ArcherTowerUnusedWithoutAddon` (`makeIsUnusedBuilding(ggs)(ArcherTower)` true by default, false once ARCHER and WINE are on; same for `makeIsUnusedJob`); extend `getBuildingRadius` test (`== ARCHER_TOWER_RANGE`); extend Lua constants tests (`BLD_ARCHERTOWER`, `JOB_ARCHER` values equal to the enum).
- [x] **Step 2: Run to confirm failure.**
- [x] **Step 3: Implement** all modifications above. After appending the enum values grep for every exhaustive `switch` on `BuildingType`/`Job` and for `EnumArray` initialisers of the two enums (`grep -rn "BuildingType::LookoutTower\|Job::LeatherWorker" libs`) and fix each; compile with `-Werror`. Check `iwBuildingProductivities.cpp:20` (`allIcons`) and `iwBuildOrder.cpp` for hidden-building handling and include the tower the same way the Wine buildings are included.
- [x] **Step 4: Run** `Test_integration`, `Test_lua`, `Test_UI`, `Test_simple`. Expected: PASS (existing round-trip serialization tests stay green because both sides use the new sizes).
- [x] **Step 5: Commit** `git commit -m "Add archer tower building type, archer job and constants"`

### Task 4: Savegame version 16 and compatibility guards

**Files:**
- Modify: `SerializedGameData.cpp:112` (version 16 + changelog line `/// 16: archer tower addon, one new building type and one new job`), `AddonHelperFunctions.h` (add `numArcherTowerAddonBuildings = 1u`, `numArcherTowerAddonJobs = 1u`, only used for deserialization), `GamePlayer.cpp` (every enum-sized array written without length: `build_order`, `inventory.people`, `global_inventory.people`, distribution/transport/tool arrays touched by the new enum values), `BuildingRegister.cpp:38-50`, `buildings/nobBaseWarehouse.cpp:143-154,182-205`, `GameCommands.cpp` (`ChangeBuildOrder`, `SetAllInventorySettings`: guard via the deserializer's data version so old replays still parse).
- Test: `testArcherTower.cpp`, `testSerialization.cpp`

**Interfaces:**
- Consumes: fixture `tests/testData/savegames/v15-wine.sav` (Task 1).
- Produces: loading any version <= 15 data yields `Archer` inventory entries = 0 and `ArcherTower` appended last in the build order.

- [x] **Step 1: Failing tests:** `LoadV15Savegame` (loads the fixture into a world; asserts load succeeds, HQ and Temple exist, `getInventory().people[Job::Archer] == 0`, `getBuildOrder().back() == BuildingType::ArcherTower`, no leftover bytes in the serializer); `SaveLoadRoundTripV16` (save, load, compare build order and inventory arrays); `V15ReplayCommandsParse` (a serialized `ChangeBuildOrder` and `SetAllInventorySettings` written with the v15 array size deserialize without exception).
- [x] **Step 2: Run to confirm failure** (misread arrays / exception).
- [x] **Step 3: Implement** guards using the same pattern as the Wine/Leather code (`if(sgd.getGameDataVersion() < 16)` pop the shorter raw array, append the new defaults). Keep each guard small; do not touch the Wine/Leather branches.
- [x] **Step 4: Run** all four test binaries. Expected: PASS.
- [x] **Step 5: Commit** `git commit -m "Bump savegame version for archer tower"`

### Task 5: `nobArcherTower`, `nofArcher` skeleton, four worker slots

**Files:**
- Create: `buildings/nobArcherTower.h/.cpp`, `figures/nofArcher.h/.cpp`
- Modify: `gameTypes/GO_Type.h` (append `NobArcherTower`, `NofArcher`, update `maxEnumValue`), `SerializedGameData.cpp` (`RTTR_CREATE_GO` for both), `factories/BuildingFactory.cpp:22-31`, `factories/JobFactory.cpp` (case `Job::Archer` -> `nofArcher`; the `getGOT() == NobUsual` checks at `:79,92` must not misfire), `GamePlayer.cpp:474-479` (request `MAX_ARCHERS_PER_TOWER` jobs for this type), `buildings/nobUsual.h/.cpp` (virtual seams listed below), `figures/nofBuildingWorker.cpp:68` (`abrogateWorkplace` passes `this`), `world/GameWorldView.cpp:435` (occupancy label GOT list), `BuildingRegister.cpp` (tower stays in `buildings[ArcherTower]` as `nobUsual*`; deserialization uses `GO_Type::NobArcherTower` for this type).
- Test: `testArcherTower.cpp`

**Interfaces:**
- `class nobArcherTower : public nobUsual` with: `nobArcherTower(MapPoint pos, unsigned char player, Nation nation)`, `nobArcherTower(SerializedGameData&, unsigned obj_id)`, `GO_Type getGOT() const override`, `unsigned getNumArchers() const`, `nofArcher* getArcher(unsigned slot) const` (nullptr if empty), `unsigned getFreeSlot() const` (or `MAX_ARCHERS_PER_TOWER` when full), `void gotWorker(Job, noFigure&) override` (fills the next free slot, calls `handleEvent(0)` only for the first worker), `void workerLost(nofArcher&)` (frees only that slot, re-requests exactly one `Job::Archer`), `void DestroyBuilding() override` (calls `lostWork()` on every archer, `jobNotWanted(this, true)` for unfilled slots), `void serialize(SerializedGameData&) const override` (slots 1-3 and priority field added here; slot 0 stays in the base format), `bool hasWorker() const` stays true when at least one archer is present.
- `class nofArcher : public nofBuildingWorker`: `nofArcher(MapPoint, unsigned char player, nobArcherTower* workplace)`, `nofArcher(SerializedGameData&, unsigned obj_id)`, `unsigned getSlot() const`, `GO_Type getGOT() const final`; for now `areWaresAvailable()` returns false (stays in `WaitingForWaresOrProductionStopped`), `drawWorking` empty, `getCarryID` returns 0.
- Slot index is assigned by the tower and stored in the archer (serialized).

- [x] **Step 1: Failing tests:** `TowerRequestsFourArchers` (build tower on a 1-player world with Wine+Archer addons, HQ has 4 Helpers and 4 Bows: after completion `hqPos` warehouse sends four `nofArcher`, `getNumArchers() == 4` after they arrive); `NoArcherWithoutBow` (Bow = 0 => no archer is created, request stays pending); `OnlyMissingSlotIsRequested` (kill one archer: `jobs_wanted` gains exactly one `Archer` entry for this tower); `TowerBurnsOnLostTerritory` (enemy captures territory under the tower: all four archers get `isWandering()`, tower removed, no assert); `SaveLoadKeepsSlots` (round trip with 2 archers at slots 0 and 2, slot indices preserved); `FirstArcherClosesDoorOnce` (door state consistent after 4 arrivals / 1 loss).
- [x] **Step 2: Run to confirm failure.**
- [x] **Step 3: Implement** the two classes and the `nobUsual` seams: make `workerArrived` create the productivity event only if absent; make `gotWorker`, `workerLost`, `DestroyBuilding`, `hasWorker` virtual where the tower must differ (`nobUsual` has no other multi-worker user, so keep changes behaviour-neutral for existing buildings); Door: `closeDoor` once on first arrival, `openDoor` when the last archer leaves.
- [x] **Step 4: Run** `Test_integration` fully. Expected: PASS.
- [x] **Step 5: Commit** `git commit -m "Add archer tower building with four archer slots"`

### Task 6: Wine supply

**Files:**
- Modify: `GamePlayer.cpp:140-142` (add `distribution[GoodType::Wine].client_buildings.push_back(BuildingType::ArcherTower)` in the standard distribution), `GamePlayer.cpp` deserialization (for `getGameDataVersion() < 16` add the tower as Wine client when the archer addon is active), `buildings/nobUsual.cpp` (`consumeWares` unchanged; verify `calcDistributionPoints`)
- Test: `testArcherTower.cpp`

**Interfaces:**
- Consumes: `BLD_WORK_DESC[ArcherTower]` (Task 3), `nobArcherTower` (Task 5).
- Produces: `nobArcherTower` stores up to 3 wine through the standard `numWares[0]`; `nobUsual::getNumWares(0)`.

- [x] **Step 1: Failing tests:** `TowerOrdersUpToThreeWine` (HQ with 10 Wine: after some GFs `getNumWares(0) == 3`, never more); `WineGoesToTempleAndTower` (both buildings exist, wine delivered to both, none starves permanently with enough stock); `V15SaveStillDeliversWineToTemple` (load the v15 fixture with archer addon enabled, build a tower, wine arrives at the tower and the temple distribution list still contains `Temple`).
- [x] **Step 2: Run to confirm failure.**
- [x] **Step 3: Implement** the distribution client and the load-time patch. Do not add a row to `iwDistribution`.
- [x] **Step 4: Run** all test binaries. Expected: PASS.
- [x] **Step 5: Commit** `git commit -m "Supply wine to archer towers"`

### Task 7: Vision around the tower

**Files:**
- Modify: `buildings/nobArcherTower.cpp`, `figures/nofArcher.cpp` (`workplaceReached`, `workAborted`), `world/GameWorld.cpp:1066-1079` (visibility recalculation also considers `ArcherTower` buildings with `hasWorker()`)
- Test: `testArcherTower.cpp`

**Interfaces:**
- Consumes: `VISUALRANGE_ARCHERTOWER` (Task 3), `makeVisibleAroundPoint(pos, radius, player)`, `recalcVisibilitiesAroundPoint`.
- Produces: tower reveals `VISUALRANGE_ARCHERTOWER` around itself while `getNumArchers() >= 1`.

- [x] **Step 1: Failing tests:** `TowerRevealsRadiusSevenWithFirstArcher` (a node at distance 7 outside own territory is `Visible` after the first archer arrives, not before); `VisionRemovedWhenLastArcherLost` (node returns to non-visible after the last archer dies/tower is destroyed); `VisionStaysWhenOneOfTwoLost`.
- [x] **Step 2: Run to confirm failure.**
- [x] **Step 3: Implement** following `nofScout_LookoutTower::workplaceReached/workAborted` and the `LookoutTower` loop in `GameWorld.cpp`.
- [x] **Step 4: Run** `ArcherTowerSuite`, then all binaries. Expected: PASS.
- [x] **Step 5: Commit** `git commit -m "Give archer towers vision"`

### Task 8: Ranged hit API for soldiers and scouts

**Files:**
- Modify: `figures/nofActiveSoldier.h/.cpp` (new method next to `takeHit`), `figures/nofScout_Free.h/.cpp`
- Test: `tests/s25Main/integration/testArcherTower.cpp` (reuse fixtures from `testAttacking.cpp:72-195` where needed)

**Interfaces:**
- Produces: `bool nofActiveSoldier::isValidRangedTarget() const` (true only when alive, not in `noFighting`, not inside a building, no active free fight); `void nofActiveSoldier::takeRangedHit()` (one damage: armor first, else `--hitpoints`; at 0 hitpoints the soldier dies cleanly: aborts free fight, informs home building / attacked goal / defender bookkeeping the same way a lost fight does, then `die()`); `bool nofScout_Free::isValidRangedTarget() const`; `void nofScout_Free::killByRangedHit()` (dies, clears the scouted flag bookkeeping).

- [x] **Step 1: Failing tests:** `RangedHitRemovesArmorFirst`; `RangedHitReducesOneHitpoint` (rank Private, 3 HP -> 2); `RangedHitKillsLastHitpoint` (attacker walking to a building with 1 HP: soldier removed, skeleton placed, inventory count decreased, attacked building's attacker list consistent, no assert fired); `RangedHitOnAggressiveDefenderCleansUp`; `ScoutDiesOnRangedHit`; `FightingSoldierIsNotValidTarget`; `SoldierInsideBuildingIsNotValidTarget`.
- [x] **Step 2: Run to confirm failure.**
- [x] **Step 3: Implement** reusing the cleanup used by `lostFighting` / `informTargetsAboutCancelling` / `noFigure::die`; do not duplicate fight-cleanup logic, extract a shared private helper if needed.
- [x] **Step 4: Run** `Test_integration`. Expected: PASS (existing `AttackSuite` unchanged).
- [x] **Step 5: Commit** `git commit -m "Add ranged hit handling for soldiers and scouts"`

### Task 9: Target discovery and priority

**Files:**
- Create: `figures/ArcherTargeting.h/.cpp`
- Test: `testArcherTower.cpp`

**Interfaces:**
- Produces:
  - `enum class ArcherTargetPriority : uint8_t { Nearest, Weakest, Strongest }` with `maxEnumValue`.
  - `struct ArcherTarget { noFigure* figure; unsigned distance; unsigned rank; bool isScout; unsigned objectId; }`.
  - `std::vector<ArcherTarget> findArcherTargets(const GameWorldBase&, MapPoint towerPos, unsigned char player)` (enemy via `GamePlayer::isAttackable`, within `ARCHER_TOWER_RANGE`, node `fow[player].visibility == Visibility::Visible`, only `nofScout_Free` and `nofActiveSoldier` satisfying `isValidRangedTarget`).
  - `const ArcherTarget* chooseArcherTarget(const std::vector<ArcherTarget>&, ArcherTargetPriority)`: Nearest = min distance; Weakest = min rank with scouts first; Strongest = max rank with scouts last; ties by distance, then `objectId`.

- [x] **Step 1: Failing tests:** `FindsEnemySoldierAndScout`; `IgnoresOwnAndAlliedFigures`; `IgnoresCivilians` (carrier, builder, donkey); `IgnoresOutOfRange` (distance 8); `IgnoresFoggedFigures`; `IgnoresPassiveSoldiers`; `PriorityNearest`; `PriorityWeakestPrefersScout`; `PriorityStrongestPrefersHighRankAndScoutLast`; `TieBreakIsDeterministic` (same input twice gives same target).
- [x] **Step 2: Run to confirm failure.**
- [x] **Step 3: Implement** the two functions; scan with `getPointsInRadius` / `getFigures` (model: `nofActiveSoldier::tryFightingNearbyEnemy`, `nofHunter::getAnimalsInRange`).
- [x] **Step 4: Run** `ArcherTowerSuite`. Expected: PASS.
- [x] **Step 5: Commit** `git commit -m "Add archer target discovery and priority"`

### Task 10: Shooting state machine, wine boost, drawing

**Files:**
- Modify: `figures/nofBuildingWorker.h` (append states `ArcherCooldown` and `ArcherShooting`; update `maxEnumValue`; handle both in `lostWork` (`nofBuildingWorker.cpp:229-294`) and `draw`), `figures/nofArcher.h/.cpp`, `buildings/nobArcherTower.h/.cpp` (target priority field + serialization), `gameData/ArcherTowerConsts.h` (roof offsets)
- Test: `testArcherTower.cpp`

**Interfaces:**
- Consumes: `findArcherTargets`/`chooseArcherTarget` (Task 9), `takeRangedHit`/`killByRangedHit` (Task 8), `ARCHER_*` constants (Task 3), `nobUsual::waresAvailable()/consumeWares()`.
- Produces on `nobArcherTower`: `ArcherTargetPriority getTargetPriority() const`, `void setTargetPriority(ArcherTargetPriority)`; on `nofArcher`: `unsigned getBoostedShotsLeft() const`; roof offsets `ARCHER_ROOF_OFFSETS` as `helpers::EnumArray<std::array<DrawPoint, MAX_ARCHERS_PER_TOWER>, Nation>`.
- Behaviour: state `ArcherCooldown` waits `ARCHER_SHOT_INTERVAL_GFS` (or `ARCHER_BOOSTED_SHOT_INTERVAL_GFS` while `boostedShotsLeft_ > 0`), then searches a target; none => cooldown again with `startNotWorking`; a target and `boostedShotsLeft_ == 0` and `waresAvailable()` => `consumeWares()`, `boostedShotsLeft_ = ARCHER_BOOST_SHOTS`; shot: face the target direction, state `ArcherShooting` (Hunter-length animation event), at the end the target is re-validated (`isValidRangedTarget`, still in range and visible), then `RANDOM_RAND(100) < ARCHER_HIT_CHANCE_PERCENT` decides hit => `takeRangedHit` / `killByRangedHit`; `boostedShotsLeft_` is decremented per shot; target is stored as object id and re-resolved (no raw pointer saved). "Feuer einstellen" uses the existing production stop; `gotWareOrProductionAllowed` restarts the cooldown.

- [x] **Step 1: Failing tests:** `ShootsEverySevenSecondsWithoutWine` (enemy scout in range, no wine: kill/hit events happen at 140 GF spacing); `ShootsEveryFiveSecondsWithWine` (spacing 100 GF for 10 shots, wine count drops by 1, 11th shot spacing 140 and consumes a new wine if available); `HitChanceIsSeventyPercent` (fixed RNG seed, 1000 shots on a target dummy with unlimited HP: hits in [650, 750]); `OnlyOneDamagePerShot` (armored and unarmored); `ShotDroppedWhenTargetEntersBuilding` / `ShotDroppedWhenTargetDies` (no hit, no assert, next cooldown starts); `ProductionStopPreventsFiring`; `NoWineConsumedWithoutTarget`; `FourArchersSplitTargetsAccordingToPriority` (Weakest vs Strongest picks different figures); `TowerDestroyedMidShotIsClean` (destroy tower in `ArcherShooting`: events removed, no hit applied); `SaveLoadMidCooldownKeepsBoostCounter` (round trip while `boostedShotsLeft_ == 6`: counter, slot, remaining event time preserved).
- [x] **Step 2: Run to confirm failure.**
- [x] **Step 3: Implement** states and drawing: idle/aiming sprite `LOADER.getBobSprite(nation, Job::Hunter, dir, 0).drawForPlayer(...)`, shooting frames as `nofHunter::drawWorking` (`rom_bobs` 219+0..12 for East, `1686 + enumCast(dir + 2u) * 8 + frame` otherwise, sound 74 on last frame), no ground shadow, position `drawPt + ARCHER_ROOF_OFFSETS[nation][slot]`; start with placeholder offsets copied from the catapult `(-7, -19)` plus a per-slot spread, then tune visually in Task 13.
- [x] **Step 4: Run** all test binaries. Expected: PASS.
- [x] **Step 5: Commit** `git commit -m "Add archer shooting and wine boost"`

### Task 11: Target priority game command

**Files:**
- Modify: `GameCommand.h` (append `SetArcherTargetPriority` to `GCType`, update `maxEnumValue`), `GameCommands.h/.cpp`, `GameCommand.cpp` (`deserialize` case), `factories/GameCommandFactory.h/.cpp`, `ai/AIInterface.h/.cpp` (optional wrapper, only if trivial), `buildings/nobArcherTower.h/.cpp` (virtual copy `targetPriorityVirtual_` like `coinsDisabledVirtual`)
- Test: `tests/s25Main/integration/testGameCommands.cpp` (suite `GameCommandSuite`, fixture `WorldWithGCExecution2P`, model `ChangeCoinAccept` at `:636`)

**Interfaces:**
- Produces: `gc::SetArcherTargetPriority(MapPoint, ArcherTargetPriority)` with `execute` (owner check mandatory); `bool GameCommandFactory::SetArcherTargetPriority(MapPoint, ArcherTargetPriority)`; `nobArcherTower::setTargetPriorityVirtual(...)`, `getTargetPriorityVirtual()`.

- [x] **Step 1: Failing tests:** `SetArcherTargetPriorityChangesTower` (owner changes Nearest -> Weakest); `SetArcherTargetPriorityIgnoredForOtherPlayer`; `SetArcherTargetPriorityIgnoredOnWrongBuilding` (non-tower position); `SetArcherTargetPrioritySerialization` (command serialize/deserialize round trip, invalid enum value rejected).
- [x] **Step 2: Run to confirm failure.**
- [x] **Step 3: Implement** following `SetArmorAllowed` (`GameCommands.h:416-432`).
- [x] **Step 4: Run** `Test_integration`. Expected: PASS.
- [x] **Step 5: Commit** `git commit -m "Add archer target priority game command"`

### Task 12: Windows and build menus

**Files:**
- Create: `ingameWindows/iwArcherTower.h/.cpp` (derived from `iwBuilding`)
- Modify: `desktops/dskGameInterface.cpp:579-601` and `ingameWindows/iwBuildings.cpp:145-155` (open `iwArcherTower` for this type), `ingameWindows/iwAction.cpp:104-161` (tower in the "House" tab, hidden unless `isArcherTowerActive`), `iwBuildOrder.cpp`, `iwBuildingProductivities.cpp`, `iwBuilding.cpp` (hide the productivity bar for the tower like catapult/lookout at `:88-90`, adapt the "House unoccupied" text for partial occupancy), `world/GameWorldView.cpp:441-449` (hide productivity label)
- Test: `tests/s25Main/UI/testWindows.cpp`

**Interfaces:**
- `iwArcherTower(GameWorldView&, GameCommandFactory&, nobArcherTower*)`: three radio-style image buttons (Nearest / Weakest / Strongest) calling `gcFactory.SetArcherTargetPriority`, then `setTargetPriorityVirtual`; wine row via the existing `iwBuilding` ware display (3 slots); the existing button 6 is "hold fire / fire at will".

- [x] **Step 1: Failing tests:** `ArcherTowerWindowConstructs` (window for a placed tower constructs, has the three priority controls and the production button); `BuildMenuListsTowerOnlyWithAddon` (action window building list contains `ArcherTower` when active, not otherwise); existing `AddonWindow` and building-window tests stay green.
- [x] **Step 2: Run to confirm failure.**
- [x] **Step 3: Implement** windows and menu wiring.
- [x] **Step 4: Run** `Test_UI`, `Test_integration`. Expected: PASS.
- [x] **Step 5: Commit** `git commit -m "Add archer tower window and build menu entries"`

### Task 13: Graphics aliasing and visual tuning

**Files:**
- Modify: `Loader.cpp:152-169` (`getNationIcon`, `getBuildingTex`) and `:637-676` (`building_cache`): map `ArcherTower` to `LookoutTower` for sprite, shadow, skeleton, skeleton shadow, door and icon via one small helper `BuildingType getGraphicsBuildingType(BuildingType)`; `gameData/ArcherTowerConsts.h` (final `ARCHER_ROOF_OFFSETS`)
- Test: `testArcherTower.cpp`

**Interfaces:**
- Produces: `BuildingType getGraphicsBuildingType(BuildingType)` (identity except `ArcherTower -> LookoutTower`), declared where `Loader.cpp` and `noBuildingSite` code can both use it.

- [x] **Step 1: Failing test:** `GraphicsTypeAliasesLookoutTower` (identity for all other types, `ArcherTower -> LookoutTower`).
- [x] **Step 2: Run to confirm failure.**
- [x] **Step 3: Implement** the helper and use it at every `250 + 5*enumCast(type)` / `nationIcons_` lookup (grep `enumCast(bld)` and `enumCast(type)` in `Loader.cpp`, `noBuildingSite.cpp`, `FOWObjects.cpp`).
- [x] **Step 4: Manual check** in the real game with an S2 data install: start a game with Wine and Archer addons, build the tower on a 3-player map, check build menu icon, building sprite, construction site, burning/skeleton, door, four archers on the roof with distinct positions for all five nations, shooting animation facing the target, no ground shadow; adjust `ARCHER_ROOF_OFFSETS`. Record the observations in the PR description.
- [x] **Step 5: Run** all test binaries; **commit** `git commit -m "Reuse lookout tower graphics for archer tower"`

### Task 14: Full verification and PR preparation

**Files:**
- Modify: `external/languages` is a submodule and is not changed; check `rttr.pot` generation target only if the repo documents one (see `README.md`).
- Test: `tests/s25Main/integration/testAI.cpp` (or the existing AI/autoplay test; reuse its fixture)

- [x] **Step 1: Failing test:** `AIDoesNotCrashWithArcherAddon` (run an AI-vs-AI headless game for the same number of GFs as the existing AI test with `ARCHER_TOWER` and `WINE` enabled; additionally place a human-built tower for one AI-owned player); expect it to pass or reveal AI tables that need the new enum values (`AIPlayerJH.cpp:320-329` `bldToTest`, `BuildingPlanner.cpp`), fix with the smallest change that keeps the AI from building the tower.
- [x] **Step 2: Run the complete suites:** `ctest --output-on-failure` in `build`, plus `make clangFormat` followed by `git diff --exit-code`.
- [x] **Step 3: Write the PR description** (feature, addon id, savegame version bump, deviations from the original wish list: no catapult/flag attack, graphics reused, no AI, German translations pending in `external/languages`, test list, manual test notes, known limitations).
- [x] **Step 4: Commit** `git commit -m "Verify archer tower with AI and full test run"`
