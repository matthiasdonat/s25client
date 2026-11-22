# Add archer tower (optional addon)

## What
New lobby addon "Enable archer tower" (`AddonId::ARCHER_TOWER = 0x01400000`), only effective together with the wine economy (enabling it also enables the wine addon).

- New building `BuildingType::ArcherTower` (house size, 3 boards + 4 stones, no territory, not attackable, burns down when its territory is lost like the catapult) with up to 4 archers (`Job::Archer`, recruited from a helper and a bow).
- Archers shoot at enemy soldiers and scouts within 7 fields (visible nodes only, no civilians, no soldiers in buildings, fights, at flags or on doors, no scouts on roads). 70% hit chance, damage 1, one shot every 7 s.
- Up to 3 wine are stored in the tower. One wine gives an archer 10 shots with a shot every 5 s. Wine is delivered like to the temple.
- An occupied tower reveals 7 fields around it.
- Target priority (nearest / weakest / strongest, scouts count as weakest) via new GameCommand `SetArcherTargetPriority` and window `iwArcherTower`. "Production off" stops the shooting.
- Graphics: reuses the lookout tower sprites, archers use the hunter sprites and shooting animation.

## Compatibility
- Savegame version 15 -> 16, game command version 2 -> 3. Old savegames load (tests with a v15 fixture). Replays of enum sized commands still parse.
- New GO types, `BuildingType`, `Job`, `GCType` and the `nofBuildingWorker` state are appended at the end.

## Not included / known limitations
- AI does not build or use the tower (it must not crash, covered by a test).
- Roof positions of the archers (`ARCHER_ROOF_OFFSETS`) are placeholders and the in-game graphics were not checked with the S2 data.
- No UI test for the new window (test fixtures have no game data).
- German translations live in the `external/languages` submodule.
- Soldiers fighting, waiting for a defender or at a building's flag are not shot at.

## Tests
`Test_integration` (`ArcherTowerSuite`, `GameCommandSuite/ChangeArcherTargetPriority`, `AttackSuite/RangedHit*`, `AIIgnoresArcherTowerAddon`), `Test_lua` constants. `Test_network` has a pre-existing flaky test (`TestServer_Works`).
