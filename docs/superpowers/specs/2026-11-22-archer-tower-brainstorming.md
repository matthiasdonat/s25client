# Archer Tower (Bogenschützenturm) – Brainstorming-Stand

> **Status: Brainstorming läuft, Design NICHT freigegeben.** Dieses Dokument hält den Zwischenstand fest,
> damit die Arbeit später fortgesetzt werden kann. Es ist weder das fertige Design-Dokument
> noch ein Implementierungsplan. Es darf noch nichts implementiert werden.

- Datum: 2026-11-22
- Branch: `feature/archer-tower` (von `master`, Stand `b0f0ea98b`)
- Ziel: **möglicher Beitrag zum offiziellen Spiel (Upstream-PR an Return-To-The-Roots/s25client)**
- Workflow: Superpowers `brainstorming` → Approaches wählen → Design-Dokument freigeben → `writing-plans`

## 1. Ursprüngliche Spezifikation (vom Auftraggeber, unverändert übernommen)

### 1.1 Überblick
Neues Fernkampf-Militärgebäude **Bogenschützenturm** als optionales Lobby-Addon. Erweitert das
1v1-Nahkampfsystem um Fernkampf-Unterstützung und knüpft an das bestehende Addon **Weinwirtschaft (Wine)** an.

### 1.2 Addon
- Neues Addon `AddonArcherTower`, in den Lobby-Einstellungen aktivierbar.
- Zwingende Abhängigkeit: aktiviert `Wine` automatisch bzw. setzt es voraus.

### 1.3 Warenkette & Rekrutierung (nur bestehende Waren)
- Kein neues Produktionsgebäude, keine neuen Waren.
- Bogen `GD_BOW` = dasselbe Werkzeug wie beim Jäger (Schlosser `BLD_METALWORKS`).
- Munition: wie beim Jäger unbegrenzt, kein Pfeil-Item.
- Neuer Beruf `JOB_ARCHER`. Rekrutierung in `nobBaseWarehouse`: 1 Träger + 1 Bogen + 1 Wein (`GD_WINE`) statt Bier.
- „Beförderung“: Mit Wein im Turm höhere Feuerrate und Trefferchance, pro Wein für 10 Schüsse.

### 1.4 Gebäude `BLD_ARCHERTOWER`
- `BQ_HOUSE`, Kosten 3 Bretter + 4 Steine.
- Keine Grenzerweiterung (rein taktische Gebietskontrolle).
- Max. **4 Bogenschützen**, kein separater Nahkämpfer-Slot.
- Von feindlichen Katapulten beschießbar und zerstörbar.

### 1.5 Kampf & Grafik
- Jäger-Assets (`JOB_HUNTER`) wiederverwenden (Spielerfarbe). Auf den Zinnen ein animierter Schütze, der sich zum Ziel dreht.
- Nahkampf an der Fahne: Bogenschütze tritt vor die Tür, nutzt die Jäger-Schuss-Animation. **50 % Kampfkraft/HP** eines gleichrangigen Nahkämpfers.
- Fernkampf: Reichweite ca. **1,5× Sichtweite eines Spähers** (Luftlinie, ignoriert Flüsse/Mauern).
- Ziele: feindliche Soldaten und Späher, **keine** Zivilisten.
- Schaden: ohne Wein 1 Trefferpunkt, mit Wein 2. Trefferwahrscheinlichkeit wie beim Katapult.

### 1.6 UI
- Zielpriorität (Toggle): Nächstes | Schwächstes | Stärkstes Ziel.
- Feuerbefehl: Feuer frei / Feuer einstellen.

### 1.7 Technische Hinweise der Spec
- `nobArcherTower` erbt von `nobMilitary`, `Update()`-Schleife für Zielerfassung und Projektil-Spawning.
- `JOB_ARCHER` nutzt `GD_BOW` + `GD_WINE` in `nobBaseWarehouse`; Sprite-Mapping auf Jäger-Frames inkl. Spielerfarbe.
- Erweiterung `BuildingType`, `Job`, `SerializedGameData`, KI (`AIPlayerJH`, Werkzeug-Quote für `GD_BOW`).

## 2. Recon-Ergebnisse (Code-Stand `b0f0ea98b`)

Pfade relativ zu `libs/s25main/`, außer wo anders angegeben.

### 2.1 Addons
- Basis: `addons/Addon.h`, `addons/AddonBool.h`. Beispiel Wine: `addons/AddonWine.h` (ID `WINE = 0x01000000`).
- IDs in `addons/const_addons.h:44-93` (`ENUM_WITH_STRING(AddonId, ...)`), neueste ID derzeit `BUILDING_RADIUS = 0x01300000`.
- Registrierung: `addons/Addons.h` (Include) und `GlobalGameSettings.cpp:70-126` (`mp_list`). `libs/s25main/CMakeLists.txt` globt `addons/`, kein CMake-Edit nötig.
- Addon-Assets: `data/RTTR/assets/addons/<0xID>/` werden automatisch geladen (`Loader.cpp:1147-1169`).
- **Keine Abhängigkeits-/Ausschluss-Mechanik vorhanden.** Bisherige Addons prüfen Abhängigkeiten an der Verwendungsstelle
  (`NO_ARMOR_DEFAULT`/`ARMOR_CAPTURED_BLD` ohne `LEATHER` tun still nichts, `nobMilitary.cpp:75,1193`).
- Stellen für „Archer braucht Wine“: `iwAction.cpp:146-161`, `AddonHelperFunctions.cpp:11-57`, `world/GameWorld.cpp:131-146`,
  KI über `aii.canBuildBuildingtype`; UI-Normalisierung in `iwAddons.cpp:107-162`, `GlobalGameSettings::loadSettings`/`deserialize`, `LuaInterfaceSettings.cpp:111`.

### 2.2 Gebäudetyp (Berührungspunkte)
- Enum: `gameTypes/BuildingType.h:9-56` (aktuell max `HarborBuilding`; freier Slot `Nothing9`). Anhängen am Ende entspricht dem Wine/Leather-Vorbild.
- Tabellen (`EnumArray`, **ohne Längenprüfung**, kurze Initialisierung kompiliert still): `gameData/BuildingConsts.h/.cpp`
  (`BUILDING_COSTS`, `BUILDING_SIZE`, `BLD_WORK_DESC`, `BUILDING_SIGN_CONSTS`, `BUILDING_NAMES`, `BUILDING_HELP_STRINGS`; `nullptr`-String crasht).
- `gameData/BuildingProperties.cpp:8-19`: `militaryBldTypes` ist `std::array<BuildingType,4>` und **wirft beim Start „Invalid size“** bei einem 5. Typ.
- `factories/BuildingFactory.cpp:22-31`, `BuildingRegister.cpp`, `GamePlayer.cpp:468-498,521,2200-2204`, `world/GameWorldView.cpp:717-774` (`getBuildingRadius`, exhaustive switch).
- UI: `ingameWindows/iwAction.cpp`, `iwBuildings.cpp`, `iwBuildingProductivities.cpp` (hartes `std::array<BuildingType,30>`), `iwBuildOrder.cpp`, `desktops/dskGameInterface.cpp:579-601`.
- Grafik: Sprite-Index `250 + 5*Typ` in den S2-LST-Dateien (`Loader.cpp:152-169`, `658-670`); Icon `nationIcons_[nation]->get(enumCast(bld))`. Neue Sprites nur über Override-/Addon-Ordner möglich.
- KI: `ai/aijh/BuildingPlanner.cpp:106-191`, `AIPlayerJH.cpp:320-329,925-1000,2295-2370`, `AIConstruction.cpp:432-564`. Neues Militärgebäude würde sonst als „größtes“ gelten, wenn es im Enum zuletzt steht.
- Lua: `LuaInterfaceGame.cpp:26-105` (`ADD_LUA_CONST`), `doc/lua/constants.md`, `tests/s25Main/lua/testLuaConstants.cpp`.
- Übersetzungen: `external/languages/rttr.pot` (Submodul), `gettext_noop`/`_()` in den Quellen.

### 2.3 Beruf `JOB_ARCHER`
- `gameTypes/JobTypes.h:11-56`; **Soldaten-Jobs sind ein zusammenhängender Enum-Bereich** (`Private..General`), `Archer` müsste am Ende nach `LeatherWorker` stehen.
- `gameData/JobConsts.cpp:17-61` (Tabelle ohne `static_assert`; Jäger: `{GoodType::Bow, 0, 300, 5}`, Sprite `{false, 20}`).
- Rekrutierung: `nobBaseWarehouse::orderJob` / `tryRecruitJob` / `canRecruit` (`nobBaseWarehouse.cpp:269-302,1186-1217`); `tryRecruitJob` asserted „kein Soldat“.
- Soldaten werden getrennt in `handleRecrutingEvent` (`nobBaseWarehouse.cpp:501-549`) aus Schwert + Schild + **Bier** + Träger erzeugt.
- `JobFactory.cpp:55-128` (exhaustive switch), `Loader.cpp:216-224` (`getJobTex`), `noFigure.cpp:826-861` (`drawWalking`), `iwWares.cpp:97-115`, `iwShip.cpp:262-296`, `dskCredits.cpp:400-424`.
- `Tool::Bow` existiert bereits (`GoodTypes.h`), Defaults `GamePlayer.cpp:76-97`, Schlosser `nofMetalworker.cpp:99-206`; KI-Quote generisch in `AIPlayerJH.cpp:2295-2370`, Jäger-Deckel `BuildingPlanner.cpp:191-192`.
- Start-Inventar: `buildings/nobHQ.cpp:34-370`.

### 2.4 `nobMilitary` – harte Soldaten-Annahmen
- `buildings/nobMilitary.h:38-255`; Konstruktor-Switch auf 4 Größen (`nobMilitary.cpp:51-62`, sonst `RTTR_Assert(false)`).
- Arrays der Länge `NUM_MILITARY_BLDS = 4` (`gameData/MilitaryConsts.h`): `MILITARY_RADIUS`, `NUM_TROOPS`, `NUM_GOLDS`, `NUM_ARMOR`, `TROOPS_FLAG_OFFSET`, `BORDER_FLAG_OFFSET`, `BUILDING_ARMOR_SIGN_CONSTS`.
- `nobMilitary::gotWorker` (`nobMilitary.cpp:796-802`) asserted `nofPassiveSoldier`.
- `GamePlayer::addBuilding` (`GamePlayer.cpp:475-479`) fordert Arbeiter an, wenn `BLD_WORK_DESC.job` gesetzt und **nicht** `isSoldier(job)`.
- 5 Ränge fest verdrahtet: `NUM_SOLDIER_RANKS`, `HITPOINTS`/`HIT_MOMENT` (`static_assert`), `FIGHT_ANIMATIONS`, `SOLDIER_JOBS`, `chooseSoldier`, `nobBaseWarehouse.cpp:135-141`.
- Truppen kommen über `GamePlayer::orderTroops` → `nobBaseWarehouse::orderTroops` (`nobBaseWarehouse.cpp:926-964`).
- **Rangaufstieg läuft über Münzen**, nicht Wein/Bier (`wantCoins`, `searchCoins`, `prepareUpgrading`, `handleEvent(2)`, `nofPassiveSoldier::upgrade`).
- Alles `isMilitary()` ist angreifbar, besitzt Territorium, wird zu `nobMilitary*` gecastet (`nobBaseMilitary.cpp:83,169`, `nofAttacker.cpp:201,667`, `nofDefender.cpp:157`, `BuildingRegister.cpp:78-120`).
- Ein Militärgebäude **ohne Grenzerweiterung** ist ein Sonderfall: Territorium wird in `nobMilitary::recalcTerritory`/`MILITARY_RADIUS` berechnet. Verhalten bei Radius 0 ist ungeklärt.

### 2.5 Katapult-Referenz
- `figures/nofCatapultMan.cpp:97-287`: Ziele über `lookForMilitaryBuildings(pos, 3)`, Sichtbarkeit `fow[player].visibility == Visible`, `isAttackable`, nicht `isNewBuilt`.
- Trefferchance **hartcodiert 70 %** (`nofCatapultMan.cpp:219`), `CATAPULT_MAX_TARGET_RANGE = 13`, `CATAPULT_WAIT1_LENGTH = 1300` GF (`gameData/JobConsts.h:48-55`).
- Einschlag: `CatapultStone.cpp:87-119`; Schaden `nobMilitary::hitOfCatapultStone` (`nobMilitary.cpp:1509-1541`) tötet den **schwächsten** Soldaten in `troops`, gepanzerte überleben mit 30 %. Es gibt **keine Gebäude-HP**.

### 2.6 Kampf
- `nofSoldier::hitpoints` (protected, `HITPOINTS = {3,4,5,6,7}`, `MilitaryConsts.h:76`); einzige Dekrementierung `nofActiveSoldier::takeHit` (`nofActiveSoldier.cpp:349-360`), nur aus `noFighting` aufgerufen.
- Kampfablauf: `nodeObjs/noFighting.cpp` (Zweier-Kampf auf einem Knoten). Fahnenverteidiger: `nobBaseMilitary::callDefender` → `provideDefender` → `nofDefender`.
- Kampfsprites: `LOADER.fight_cache[nation][rank]` – **indiziert nach Rang**, ein neuer Typ braucht eigene Animationen oder Fallback (`Loader.cpp:750-793`, `noFighting.cpp:91,138`).
- **Kein Schaden-von-außen-Pfad**, kein Projektil-/Pfeil-Rendering. Einzig vorhandenes Projektil: `CatapultStone`.
- Sicheres Töten: `noFigure::die()` (`noFigure.cpp:875-887`); Aufräumen (`enemy`, `homeBld`, `attacked_goal`) muss der Aufrufer erledigen, sonst feuern Asserts (`nofActiveSoldier.h:104-108`).

### 2.7 Sicht / Späher / Jäger
- `VISUALRANGE_SCOUT = 3`, `VISUALRANGE_SOLDIER = 2`, `VISUALRANGE_LOOKOUTTOWER = 20` (`MilitaryConsts.h:82-93`); `SCOUT_RANGE = 16` ist die **Wanderreichweite** des Spähers, nicht seine Sicht (`nofScout_Free.cpp:117`).
- Jäger-Animationen: `figures/nofHunter.cpp:63-117` (Warten: `getBobSprite(.., Job::Hunter, dir, 0)`; Schuss: `rom_bobs` 219+0..12 für Ost, sonst `1686 + (dir+2)*8 + frame`).
- Job-Sprites: `JobConsts.cpp:80-99`, Cache `Loader.cpp:698-720`, Spielerfarbe via `drawForPlayer(pos, color)`.
- Figuren in Radius suchen: `World::getFigures`, `getPointsInRadius`, Vorlage `nofActiveSoldier::tryFightingNearbyEnemy` (`nofActiveSoldier.cpp:209-231`).

### 2.8 UI / GameCommands
- Muster für Gebäude-Toggles: `iwMilitaryBuilding.cpp:229-355` (Gold-Button) und `iwBuilding.cpp:172-211`; Virtual-State-Muster `coinsDisabledVirtual` in `nobMilitary`.
- Neues GameCommand: `GameCommand.h` (`GCType` am Ende anhängen), `GameCommands.h/.cpp`, `GameCommand.cpp` (`deserialize`), `factories/GameCommandFactory.h/.cpp`, optional `ai/AIInterface`. Besitzerprüfung `bld->getPlayer() == playerId` ist Pflicht.

### 2.9 Savegames / Netzwerk
- `build_order`, `distribution`, `inventory.people` sind `std::array`s **ohne Längenangabe** serialisiert. Neue Typen verlangen:
  Erhöhung `currentGameDataVersion` (`SerializedGameData.cpp:112`, aktuell 15) plus Changelog-Zeile und Guards `if(sgd.getGameDataVersion() < N ...)`
  wie bei Wine/Leather (`GamePlayer.cpp:271-319,342-358`, `BuildingRegister.cpp:42-46`, `nobBaseWarehouse.cpp:184-205`).
- Neue Figurenklasse: `GO_Type.h` (am Ende anhängen), `RTTR_CREATE_GO` in `SerializedGameData.cpp:119-193`.

### 2.10 Tests & Build
- Tests: Boost.Test, `tests/s25Main/integration/` (`testGameCommands.cpp`, `testAttacking.cpp`, `testSerialization.cpp`, `testBuilding.cpp`, `testBuildingRadius.cpp`, `testArmor.cpp`, `testFigures.cpp`), Fixtures in `tests/s25Main/worldFixtures/`. Es gibt **keine** Tests für Katapult-Code.
- Build-Verzeichnis `build/` ist konfiguriert (Unix Makefiles, Release, `RTTR_ENABLE_WERROR=ON`, `RTTR_Assert_Enabled=2`, `BUILD_TESTING=ON`, arm64, Boost 1.92, Bundle-Layout `build/Contents/MacOS/`). Binaries vorhanden, Aktualität unbekannt.
- Aufrufe: `cmake --build build --target Test_integration`, `build/Contents/MacOS/Test_integration --run_test=<Suite>/<Case>`, `ctest -R Test_integration`.

## 3. Konflikte zwischen Spec und Code

| # | Spec sagt | Code sagt | Folge |
|---|-----------|-----------|-------|
| 1 | `nobArcherTower : nobMilitary`, Besatzung aus `JOB_ARCHER` | `nobMilitary` hält `nofPassiveSoldier` mit 5 Rängen, 4 Größen, `gotWorker` castet hart | Archer ist kein einfacher Job; entweder Archer als Soldaten-Variante modellieren oder `nobMilitary` stark verallgemeinern |
| 2 | „Turm erhält automatisch Wein-Upgrades“ | Aufstieg nur über Münzen; Bier nur beim Rekrutieren | Wein-Verbrauch/Munition („10 Schüsse pro Wein“) ist komplett neue Logik (Lagerung, Anforderung, Anzeige, Serialisierung) |
| 3 | `Update()`-Schleife, Projektil-Spawning | Ereignisgesteuert (`EventManager`), kein Projektil-Rendering, kein Schaden von außen | Neues Event-Muster nach `nofCatapultMan`, neues Schadens-API, neue Pfeil-Zeichnung (optional) |
| 4 | Addon aktiviert/erfordert Wine | Keine Abhängigkeits-Mechanik | Eigene Normalisierung an mehreren Stellen oder minimaler neuer Mechanismus in `GlobalGameSettings` |
| 5 | Neues Gebäude `BLD_ARCHERTOWER` | Sprites nur aus S2-Daten (Index `250+5*Typ`) | Eigene Grafik nötig (Addon-Override-Ordner), im offiziellen Spiel mit passender Lizenz |
| 6 | Reichweite 1,5 × Späher-Sicht | `VISUALRANGE_SCOUT = 3` → ca. 4–5 Felder; Katapult 13, Lookout 20 | Wahrscheinlich gemeint ist etwas anderes; **zu klären** |
| 7 | „Kein Nahkämpfer-Slot“, 50 % HP | `HITPOINTS` fest pro Rang, `fight_cache` pro Rang | Eigene HP-Regel und Kampfanimation (Bogen) nötig |
| 8 | Grenzerweiterung: keine | `MILITARY_RADIUS[size]` steuert Territorium | Verhalten bei Radius 0 klären; Baubarkeit nur auf eigenem Gebiet |
| 9 | Schaden 1/2 HP | Soldaten haben 3–7 HP, Schaden nur über `noFighting` | Neuer externer Schadenspfad inkl. Kampf-/Marsch-/Fahnenzustände |

## 4. Folgen von „Beitrag zum offiziellen Spiel“

Für einen Upstream-PR gelten zusätzlich (aus `CONTRIBUTING.md` und den Recon-Ergebnissen):

- **Kompilierung ohne Warnungen** (`RTTR_ENABLE_WERROR=ON`), Formatierung mit `clang-format` (`make clangFormat`), `clang-tidy`.
- **Tests Pflicht** für jedes neue Feature (Boost.Test, klein und beschreibend, RNG-robust).
- Namenskonventionen: lowerCamelCase für Methoden/Variablen, Member mit Suffix `_`, **keine Magic Constants**, `std::array`, `static_assert`, Doxygen-Kommentare an neuen Funktionen.
- **Savegame- und Replay-Kompatibilität** ist praktisch Pflicht: Version erhöhen, Changelog, alte Spielstände laden.
- **Netzwerk-Determinismus:** alle Zustandsänderungen nur über `GameCommand::execute` bzw. Events; kein nicht-deterministischer Code.
- **KI-Unterstützung** (Baulogik, Werkzeugquote) sollte mindestens nicht abstürzen; gewünscht ist sinnvolles Verhalten.
- **Übersetzungen:** neue Strings über `_()`/`gettext_noop`, `.pot` im Submodul `external/languages` ist ein separates Repo.
- **Grafiken** müssen frei lizenziert sein; S2-Originalsprites dürfen nicht ausgeliefert werden. Lizenzvermerke (`.reuse`, `LICENSES/`).
- **Lua-Konstanten** und Dokumentation (`doc/lua/constants.md`) ergänzen.
- Ein Feature dieser Größe sollte vorab mit den Maintainern abgestimmt werden (Issue/Diskussion), bevor ein großer PR entsteht.
- Die Arbeit sollte in **kleine, einzeln reviewbare PRs** zerlegt werden (z. B. Addon-Gerüst, Gebäudetyp, Beruf, Fernkampf, UI, KI).

## 5. Offene Fragen (für die nächste Sitzung)

Reihenfolge nach Einfluss auf das Design:

1. **Architektur:** Archer als Soldaten-Variante in `nobMilitary` (Variante A), eigene Klasse neben `nobMilitary` (Variante B), oder `nobUsual`-basierte Lösung wie `LookoutTower` (Variante C)? Siehe Abschnitt 6.
2. **Reichweite:** Was ist mit „1,5-fache Sichtweite eines Spähers“ gemeint? Zahlenwert in Feldern (z. B. 5, 10, 13 wie Katapult)?
3. **Wein-Mechanik:** Wie kommt Wein ins Gebäude (Anforderung wie Münzen?), wie wird der Vorrat angezeigt, was passiert bei 0 Wein, gilt der Wein pro Turm oder pro Schütze, und wie ist „Beförderung“ zu verstehen?
4. **Gebäudegrafik:** Welche Grafik ist vorgesehen (vorhandenes Gebäude als Platzhalter, eigene Pixelgrafik, Addon-Override)? Für Upstream: Lizenz.
5. **Verteidigung:** Was passiert, wenn alle Bogenschützen tot sind – wird der Turm erobert wie normale Militärgebäude? Wer verteidigt, wenn der Turm nur Bogenschützen enthält? Zählt `Rang` für Bogenschützen?
6. **Ziele:** Sollen auch Soldaten im Nahkampf (`noFighting`) und Soldaten im Gebäude getroffen werden? Nur sichtbare Ziele (Fog of War)? Zielwahl bei „Schwächstes/Stärkstes“ nach Rang oder aktueller HP?
7. **Territorium:** Radius 0 erlaubt? Darf der Turm außerhalb eigener Grenzen gebaut werden (sonst nur im eigenen Gebiet)?
8. **Savegame-Kompatibilität:** Pflicht für alte Spielstände (mit Version-Guards) oder Savegame-Version anheben und alte Stände verwerfen?
9. **KI:** Soll die KI den Turm bauen und nutzen (Prototyp: nein, nur nicht abstürzen)?
10. **PR-Zuschnitt:** Ein großer PR oder mehrere (Gerüst/Gebäude/Beruf/Kampf/UI/KI)? Vorabstimmung mit Upstream gewünscht?
11. **Balancing:** Trefferchance „wie Katapult“ = 70 %, Wein-Bonus „höhere Trefferchance“ – konkreter Wert? Feuerrate in GF?

## 6. Entwurf der Approaches (noch nicht mit dem Auftraggeber besprochen)

> Nur Arbeitsentwurf, keine Entscheidung. Die Empfehlung ist unverbindlich.

**A. Archer als Soldat in `nobMilitary`.**
`nobArcherTower : nobMilitary`, Archer als Soldaten-Variante mit eigenem Rang/Job. `NUM_MILITARY_BLDS` auf 5, `militaryBldTypes` auf 5, Konstruktor-Switch erweitern, `nofPassiveSoldier`/`nofDefender` um Archer-Unterscheidung erweitern.
+ Eroberung, Angreifbarkeit, Katapult, Fahnenkampf, Territorium kommen weitgehend gratis.
− Größte Eingriffstiefe (`SOLDIER_JOBS`, `HITPOINTS`, `fight_cache`, KI-Pfade, Savegame). Viele Berührungspunkte für den Review.

**B. Eigene Klasse `nobArcherTower : nobBaseMilitary`, Archer als nicht-rangbasierte Figur.**
Wiederverwendung von Angriffs-/Verteidigungs-Schnittstelle (`nobBaseMilitary`), aber eigene Besatzungsverwaltung.
+ `nobMilitary` bleibt unangetastet, geringeres Regressionsrisiko.
− Viele `isMilitary()`-Stellen casten auf `nobMilitary`; Code duplizieren oder generalisieren. Eroberung und Territorium müssen selbst nachgebaut werden.

**C. `nobUsual`-Turm nach dem Muster `LookoutTower`/Katapult mit Archer als `nofBuildingWorker`.**
+ Kleinste Eingriffstiefe, nutzt vorhandene Worker-/Event-Muster, nur eine Schussmechanik.
− Kein Territorium, nicht angreifbar/erobbar, kein Fahnenkampf → weicht stark von der Spec ab (Abschnitte 1.4, 1.5).

**Vorläufige Einschätzung:** Die Spec verlangt Eroberbarkeit, Fahnenkampf, Katapult-Anfälligkeit und Territorium → A oder B.
A ist näher am Spec-Wortlaut („erbt von `nobMilitary`“), B ist sicherer. Entscheidung offen.

## 7. Grobe Zerlegung (Idee für spätere Pläne, nicht freigegeben)

1. Addon-Gerüst (`AddonArcherTower`, ID, Registrierung, Wine-Abhängigkeit, Tests, Übersetzung)
2. Neuer `BuildingType` + `Job` inkl. Savegame-Version, Tabellen, Lua, Build-UI, Platzhalter-Grafik
3. Rekrutierung (Bogen + Wein + Träger), Werkzeug-Einstellungen, KI-Werkzeugquote
4. Besatzungsmodell im Turm (je nach Architektur A/B)
5. Fernkampf: Zielerfassung, Event-Schleife, Trefferwurf, externer Schadenspfad, Wein-Munition
6. Fahnenverteidigung: 50 % HP, Bogenanimation
7. UI: Zielpriorität, Feuerbefehl, GameCommands, Gebäude-Fenster
8. Rendering: Schütze auf Zinnen, optional Pfeil-Animation
9. KI-Verhalten
10. Dokumentation, Lua-Doku, `.pot`, Lizenzen

## 8. Weiteres Vorgehen

1. Fragen aus Abschnitt 5 beantworten, zuerst 1, 2, 3 und 5.
2. Approaches (Abschnitt 6) besprechen, Variante wählen.
3. Größe bestimmen („Projekt“, daher vollständiges Design-Dokument) und Design-Dokument schreiben
   (`docs/superpowers/specs/2026-11-22-archer-tower-design.md`), Builder-Check durchführen.
4. Freigabe des Designs, dann Skill `writing-plans` für den Implementierungsplan (voraussichtlich pro PR ein Plan).
5. Build-Verzeichnis aktualisieren (`cmake --build build`) und Ausgangs-Teststand sichern, bevor Code geändert wird.

## 9. Entscheidungen Runde 2 (2026-11-22, vom Auftraggeber)

- **Architektur: Variante C** (`nobUsual`-basierter Turm, Archer als `nofBuildingWorker`, Muster LookoutTower/Katapult).
  Folgen: Der Turm ist **kein** `nobMilitary`. Damit entfallen aus Abschnitt 1: Katapult-Anfälligkeit (Katapulte zielen nur auf `NobMilitary`),
  direkter Angriff über die Fahne, Fahnenkampf mit 50 % HP, Bogen-Kampfanimation, eigenes Territorium. Der Turm kann nur im eigenen Gebiet gebaut werden.
- **Zerstörung:** wie beim Katapult nur bei Verlust des Territoriums (`GameWorld::destroyPlayerRests` → `destroyNO` → `nobUsual::DestroyBuilding`).
  Bogenschützen verlassen das brennende Gebäude (`nofBuildingWorker::lostWork` → `startWandering`/`wander`, Rückkehr zum Lager). Kein Zusatzcode erwartet.
- **Reichweite: 7 Felder** (Luftlinie, `calcDistance`).
- **Wein:** bis zu **3 Wein** im Gebäude (`WaresNeeded(GoodType::Wine)`, `numSpacesPerWare = 3`). Ein Bogenschütze verbraucht 1 Wein
  und schießt dann **10 Schuss** mit doppeltem Schaden. Danach kann er einen neuen Wein verbrauchen. Wein-Zähler pro Schütze (neuer, zu serialisierender Zustand).
- **Feuerrate:** ohne Wein alle **7 Sekunden**, mit Wein alle **5 Sekunden**. Trefferchance bleibt gleich (Katapult: 70 %). Umrechnung Sekunden → GF (Spielgeschwindigkeit) im Design klären.
- **Grafik:** vorhandenen **LookoutTower-Sprite unverändert** wiederverwenden (Aliasing im `Loader`, siehe Abschnitt 10). Leicht abgewandelte Variante nur, wenn leicht möglich (nicht ohne neue Asset-Dateien).
- **Ziele:** nur Soldaten außerhalb von Gebäuden und außerhalb von `noFighting`, sowie Späher. Keine Zivilisten.
- **Territorium Radius 0 / Bau außerhalb:** Punkt entfällt bei Variante C (Frage 7: „nein“).
- **PR-Zuschnitt:** **ein großer PR** (Frage 10).
- Abschnitte 5 / Fragen 2, 3, 4, 5, 6, 7, 10, 11 sind damit beantwortet.

### Offen
1. **Anzahl Bogenschützen:** 1 oder 4 (siehe Abschnitt 10). Frage vom Auftraggeber gestellt.
2. **Rekrutierung:** Braucht der Archer beim Rekrutieren weiterhin Wein (Spec 1.3) oder nur Träger + Bogen (Wein nur als Munition im Turm)? Empfehlung: nur Träger + Bogen.
3. **Späher-Treffer:** Späher haben keine HP. Stirbt ein Späher bei jedem Treffer (egal ob 1 oder 2 Schaden)?
4. **Soldaten-HP:** Schaden 1 bzw. 2 gegen Soldaten mit 3–7 HP; neuer externer Schadenspfad nötig (siehe 2.6).
5. **Zielpriorität „Schwächstes/Stärkstes“:** nach Rang oder aktueller HP? Bei Spähern?
6. **Savegame-Kompatibilität** (Frage 8) und **KI** (Frage 9): bei Upstream-Beitrag Savegame-Guards nötig, KI mindestens crashfrei.

## 10. Recon Runde 2 (Variante C)

### 10.1 Mehrere Arbeiter in einem `nobUsual`
- `nobUsual` hat genau **einen** `nofBuildingWorker* worker` (`buildings/nobUsual.h:25`). Auch Tempel, Werft, Minen haben genau einen.
  Ein Turm mit **4** Bogenschützen braucht deshalb eine Unterklasse mit Slot-Array (**nicht** „nur 1“ ist die Grenze, aber 4 sind nicht gratis).
- Präzedenz für mehrere Jobs pro Gebäude: `nobHarborBuilding` ruft mehrfach `addJobWanted(Job::Scout, this)` (`nobHarborBuilding.cpp:404,818`) und entfernt über `oneJobNotWanted`/`jobNotWanted(wp, true)`.
- Notwendige Änderungen für 4 Arbeiter (Auszug): `GamePlayer::addBuilding` (`GamePlayer.cpp:474-479`) fordert N-mal an; `DestroyBuilding` (`nobUsual.cpp:93-122`) ruft `lostWork()` für alle;
  `workerLost` braucht Parameter (`nofBuildingWorker::abrogateWorkplace`, `nofBuildingWorker.cpp:68`); `gotWorker` füllt freien Slot und startet Ware-Order-Event nur einmal (`nobUsual.cpp:333-340`);
  `workerArrived` (`:491-494`) erzeugt Produktivitäts-Event nur einmal; Tür: `closeDoor` asserted offen (`noBuilding.cpp:68`), also N-mal `openDoor` oder anderes Zählen; `hasWorker()` nutzen `iwBuilding.cpp:105,188,190`,
  `GameWorldView.cpp:441`, `GameWorld.cpp:1069`, `AIPlayerJH.cpp:294`; Weckrufe `addWare`/`SetProductionEnabled`/`freePlaceAtFlag` an alle; Ware-Wettlauf in `consumeWares` (Assert `numWares != 0`, `:417`):
  `waresAvailable` beim `Waiting1`-Event neu prüfen wie `nofWorkman::handleStateWaiting1` (`nofWorkman.cpp:39-45`).
- Neue Klassen-Verdrahtung nach Vorbild `nobTemple`: `GO_Type.h`, `RTTR_CREATE_GO` (`SerializedGameData.cpp`), `BuildingFactory.cpp:22-31`, `GameWorldView.cpp:435`, `dskGameInterface.cpp:596-601`, `iwBuildings.cpp:152`, `iwBuilding.cpp:232`.
- `JobFactory.cpp:79,92` prüft `goal.getGOT() == GO_Type::NobUsual`; `Job::Hunter` erzeugt dort immer `nofHunter` (`:105`) – `Job::Archer` braucht eigenen Fall.
- Schussplätze auf dem Dach: pro Slot und Nation ein Offset (`EnumArray<std::array<DrawPoint,4>, Nation>`, visuell abstimmen); Katapult nutzt einen globalen Offset `(-7,-19)` (`nofCatapultMan.cpp:51-95`). Slot-Index muss pro Figur bekannt/serialisiert sein.

### 10.2 Wein im Gebäude
- `BldWorkDescription` (`gameTypes/BuildingTypes.h:43-59`): `{Job::Archer, nullopt, WaresNeeded(GoodType::Wine), 3}` analog Katapult (`BuildingConsts.h:51`, `{Job::Helper, nullopt, WaresNeeded(Stones), 4}`).
- Verbrauch einzelner Weine: `consumeWares()` entfernt genau 1 (`nobUsual.cpp:389-433`); Prüfung `waresAvailable()`/`getNumWares(0)`. Der 10-Schuss-Zähler ist **neu** (pro Schütze, serialisiert).
- Transport: Wein ist nicht in `distributionMap` (`SettingsTypes.cpp:7-44`), sondern hat einen festen Client `Temple` (`GamePlayer.cpp:142`). Ein zweiter Client ist ein `push_back(ArcherTower)` dort; **keine** Verteilungs-Slider (sonst wären `SettingsTypes`, `GameCommands.cpp:128-134`, Savegame betroffen).
  Folge: Tempel und Türme konkurrieren um Wein nach Entfernung und Lagerbestand (`calcDistributionPoints`, `nobUsual.cpp:435-469`).
- Anzeige: `iwBuilding.cpp:108-153` zeichnet pro `waresNeeded` eine Zeile mit `numSpacesPerWare` Slots (also 3 Weine).

### 10.3 Grafik-Aliasing auf LookoutTower
- Neue Enum-Werte würden auf `250 + 5*enumCast(type)` zeigen und kollidieren mit Baustellenschild (Index 450+) und Mühlen-Flügeln (`nofMiller.cpp:80,96`, `nobUsual.cpp:151`). Aliasing nötig:
  `Loader.cpp:637-676` (`building_cache`), `getBuildingTex` (`:160-169`), `getNationIcon` (`:152-158`) – je Abbildung `ArcherTower → LookoutTower`.
- Zusätzlich explizite Einträge für `DOOR_CONSTS` (`DoorConsts.h`, Türy `6/5/6/4/5`) und `BUILDING_SIGN_CONSTS` (`{0,0}`) kopieren.
- LookoutTower hat keine Dach-Dekoration in `nobUsual::draw`; die Schützen werden vom Worker (`drawWorking`) auf das Dach gezeichnet.
- Gebäude-Sprites haben keine Spielerfarbe, Figuren schon (`drawForPlayer`).
- Fazit: „unverändert wiederverwenden“ ist leicht (3 Stellen + 2 Konstanten). Eine „leicht abgewandelte“ Grafik bräuchte neue Asset-Dateien im Addon-Ordner und ist für den ersten PR nicht nötig.

### 10.4 Zerstörung bei Gebietsverlust
- `GameWorld::recalcTerritory` (`world/GameWorld.cpp:379-497`) → `destroyPlayerRests` (`:656-692`); Ausnahmen sind nur HQ, Hafen, besetzte `NobMilitary`. Ein `nobUsual`-Turm brennt also ab.
- Arbeiter: `nofBuildingWorker::lostWork` (`nofBuildingWorker.cpp:229-294`) → `startWandering()`/`wander()` → Rückkehr zu einem Lager oder `die()` nach `WANDER_TRYINGS`.

## 11. Entscheidungen Runde 3 (2026-11-22)

- 4 Bogenschützen (Slot-Array in `nobArcherTower`). Wein nur im Turm (Rekrutierung nur Träger + Bogen).
- Späher sterben bei einem Treffer. **Schaden immer maximal 1 Punkt** (auch mit Wein; Wein erhöht nur die Feuerrate für 10 Schuss).
- Zielpriorität nach Rang, Späher = schwächstes Ziel. Savegame-Guards ja. KI erstmal weglassen.
- Das vollständige Design steht in `2026-11-22-archer-tower-design.md` (wartet auf Freigabe; Builder-Check läuft).
