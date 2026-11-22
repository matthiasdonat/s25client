# Bogenschützenturm (Archer Tower) – Design

> **Status: Entwurf, wartet auf Freigabe.** Danach folgt der Implementierungsplan (`writing-plans`).
> Vorarbeit und Recon: `2026-11-22-archer-tower-brainstorming.md` (Abschnitte 2, 9, 10).

- Datum: 2026-11-22
- Branch: `feature/archer-tower` (von `master`, `b0f0ea98b`)
- Größe: **Projekt**, ein großer Pull-Request an Return-To-The-Roots/s25client
- Als „Entscheidung“ markiert = vom Auftraggeber entschieden. Als „Annahme“ markiert = meine Wahl, bitte prüfen.

## 1. Absicht

Ein optionales Lobby-Addon fügt ein mittelgroßes Gebäude hinzu, das mit Bogenschützen feindliche Soldaten und Späher aus der Distanz beschießt.
Es nutzt das bestehende Wein-Addon (Weinwirtschaft) und den Bogen aus der Schlosserei, braucht also keine neue Ware und kein neues Produktionsgebäude.
Ziel ist ein Beitrag zum offiziellen Spiel. Daher gelten die Regeln des Projekts: Tests, warnungsfreier Build, `clang-format`, Savegame-/Replay-Kompatibilität, deterministisches Spiel im Netzwerk.

## 2. Ziele, Nicht-Ziele, Anti-Ziele

**Ziele**
- Addon `AddonArcherTower`, nur zusammen mit Weinwirtschaft wirksam.
- Neues Gebäude `BuildingType::ArcherTower` mit bis zu 4 Bogenschützen (`Job::Archer`).
- Fernkampf mit 7 Feldern Reichweite, einstellbare Zielpriorität, Feuer an/aus.
- Wein im Turm erhöht für 10 Schuss die Feuerrate.
- Bestehende Spielstände und Replays laden weiter.

**Nicht-Ziele**
- Keine KI-Nutzung (die KI baut den Turm nicht; sie darf nur nicht abstürzen, wenn ein Mensch ihn baut).
- Keine eigene Gebäudegrafik (LookoutTower-Sprite wird wiederverwendet), keine Pfeil-Projektile, keine deutschen Übersetzungen im PR (liegen im Submodul `external/languages`).
- Keine Änderung an `nobMilitary`, Kampfsystem (`noFighting`), Katapult oder Beförderung der Soldaten.
- Kein Nahkampf-Verhalten der Bogenschützen, kein Territorium, kein Angriff auf den Turm (er ist kein Militärgebäude).

**Anti-Ziele (wäre ein Misserfolg, auch wenn es technisch läuft)**
- Netzwerk-Desync durch nicht deterministischen Code (Zufall nur über das Spiel-RNG, Änderungen nur in Events/GameCommands).
- Alte Spielstände oder Replays lassen sich nicht mehr laden.
- Abstürze durch Asserts, wenn ein beschossener Soldat gerade zu einem Gebäude gehört.

## 3. Ansätze (Approaches)

**Architektur**
- A. Archer als Soldat in `nobMilitary`: Eroberung, Fahnenkampf und Katapult gratis, aber größte Eingriffstiefe (5 Ränge, 4 Größen, `fight_cache`, Savegame). Verworfen.
- B. Eigene Klasse neben `nobMilitary` (`nobBaseMilitary`): `nobMilitary` bleibt unberührt, aber viele `isMilitary()`-Stellen casten auf `nobMilitary`. Verworfen.
- **C. `nobUsual`-Unterklasse mit `nofBuildingWorker`-Archern (gewählt):** Muster Spähturm/Katapult. Kleinste Eingriffstiefe. Preis: Turm ist nicht angreifbar, nicht von Katapulten beschießbar, hat keine Fahnenkämpfe und kein Territorium.

**Anzahl Bogenschützen**
- 1 Schütze: zu weit weg von der Spezifikation. Verworfen.
- 1 jetzt, 4 später: wäre ein zweiter Umbau. Verworfen (ein großer PR gewünscht).
- **4 Arbeiter-Plätze in `nobArcherTower` (gewählt):** Vorbild ist der Hafen, der mehrere Späher über `addJobWanted` anfordert.

## 4. Entscheidungen des Auftraggebers

- Architektur C, 4 Bogenschützen. Der Turm brennt wie beim Katapult nur bei Gebietsverlust ab; die Schützen laufen dann zu einem Lager.
- Reichweite 7 Felder, Luftlinie (`calcDistance`), ignoriert Flüsse/Mauern.
- Ziele: feindliche Soldaten und Späher, keine Zivilisten. Keine Soldaten im Nahkampf, keine Soldaten in Gebäuden.
- **Schaden immer genau 1 Punkt.** Späher sterben bei einem Treffer. Trefferwahrscheinlichkeit wie beim Katapult (70 %), auch mit Wein.
- Feuerrate: ohne Wein 1 Schuss alle 7 s, mit Wein alle 5 s. Wein hält für 10 Schuss, dann kann der Schütze einen neuen Wein verbrauchen. Maximal 3 Wein im Turm.
- Rekrutierung nur Träger + Bogen (Wein nur im Turm).
- Zielpriorität nach Rang. Späher gelten als schwächstes Ziel.
- Zielpriorität einstellbar: Nächstes | Schwächstes | Stärkstes. Feuerbefehl: Feuer frei / Feuer einstellen.
- Gebäude: Größe mittel (`BQ_HOUSE`), 3 Bretter + 4 Steine, keine Grenzerweiterung.
- Grafik: LookoutTower-Sprite unverändert, Schütze auf dem Dach mit Jäger-Sprites in Spielerfarbe.
- Savegame-Kompatibilität mit Versions-Guards. KI zunächst weglassen.
- Wein-Abhängigkeit: Addon aktiviert Wein automatisch bzw. setzt es voraus.

## 5. Verhalten im Detail

### 5.1 Addon
- Neues `AddonBool` `AddonArcherTower` (Name/Beschreibung per `_()`), Gruppe Economy, Standard aus. ID **`0x01400000`** (Annahme: nächste freie Nummer im Block `0x01...`, nach `BUILDING_RADIUS = 0x01300000`).
- Wirksam nur, wenn Wein und Archer-Addon aktiv sind (Hilfsfunktion im Stil von `AddonHelperFunctions`). Das Gebäude erscheint nur dann im Baumenü, in der Bauordnung und in Statistiken.
- Aktiviert der Spieler das Archer-Addon, wird Wein automatisch mitaktiviert (`iwAddons`, `GlobalGameSettings::loadSettings`/`deserialize`, Lua-Einstellungen). Netzwerk/Replay/Lua dürfen keinen Zustand „Archer an, Wein aus“ erzeugen können; wenn doch, gilt der Turm als nicht verfügbar.

### 5.2 Gebäude und Beruf
- `BuildingType::ArcherTower` und `Job::Archer` werden **am Ende** der Enums angehängt (Soldaten-Jobs bleiben ein zusammenhängender Bereich).
- `nobArcherTower : nobUsual` mit eigenem `GO_Type`. Vier Arbeiter-Plätze, jeder Platz hat einen Slot-Index 0–3.
- Arbeiter werden beim Bau 4-mal angefordert (`addJobWanted`). Fällt ein Schütze aus oder wird ersetzt, wird nur dieser Platz neu angefordert.
- Rekrutierung über den vorhandenen Weg `tryRecruitJob`: Träger + Bogen. Der Beruf bekommt in `JobConsts` den Werkzeug-Eintrag `GoodType::Bow`. Es gibt kein neues Werkzeug.
- Wein: `WaresNeeded(GoodType::Wine)`, 3 Plätze. Wein-Transport über einen zusätzlichen statischen Client in der Verteilung (neben dem Tempel); keine neue Zeile im Verteilungsfenster.
- Sprites: Gebäude-, Schatten-, Skelett-, Tür-Bild und Icon werden im `Loader` auf `LookoutTower` abgebildet. Tür- und Schild-Konstanten werden kopiert. Der Beruf nutzt die Jäger-Laufsprites und das Jäger-Symbol.
- Der Turm erscheint im Baumenü auf dem „Haus“-Reiter.

### 5.3 Schussablauf (je Bogenschütze, ereignisgesteuert wie `nofCatapultMan`)
1. Wartezustand. Wenn Feuer erlaubt ist, wird in festen Abständen nach einem Ziel gesucht.
2. Zielsuche: Figuren im Radius 7 mit Sichtbarkeit `Visible` für den Besitzer (wie der Katapult; der Turm sorgt selbst für Sicht, siehe 5.3a), die feindlich sind (`GamePlayer::isAttackable`) und eines der folgenden:
   - Späher (`nofScout_Free`)
   - aktiver Soldat (`nofActiveSoldier`), der **nicht** im Nahkampf ist und **nicht** in einem Gebäude (Annahme: auch Angreifer, aggressive Verteidiger und zum Fahnenkampf laufende Verteidiger; Soldaten, die als Passivsoldaten zu einem Gebäude laufen, sind **nicht** Ziel).
3. Zielauswahl nach der Einstellung des Turms. Gleichstände werden deterministisch nach Entfernung, dann nach Objekt-ID entschieden:
   - Nächstes: kleinste Entfernung.
   - Schwächstes: niedrigster Rang, Späher zuerst.
   - Stärkstes: höchster Rang, Späher zuletzt.
4. Der Schütze dreht sich zum Ziel, schießt (Jäger-Schussanimation, Spielerfarbe, Sound), würfelt mit dem Spiel-RNG 70 % Treffer und wendet den Treffer am Ende der Animation an.
5. Bei Treffer: Späher stirbt; Soldat erleidet **1** Schaden (Rüstung geht zuerst verloren, wie bei `takeHit`), bei 0 HP stirbt er sauber (Aufräumen der Verweise auf Heimat-/Angriffsgebäude und den Gegner, kein Assert-Fehler).
6. Pause bis zum nächsten Schuss: **7 s ohne Wein, 5 s mit Wein**, umgerechnet in Spielframes (Referenzgeschwindigkeit 50 ms/GF → 140 bzw. 100 GF, mit den Hilfsfunktionen in `GameConsts.h`).
7. Wein: Hat der Schütze keine Wein-Schüsse mehr übrig und Wein ist im Turm, verbraucht er beim nächsten Schuss einen Wein (Annahme: nur wenn ein Ziel gefunden wurde) und hat dann 10 schnelle Schüsse. Ohne Wein schießt er langsam weiter.
8. Der Zähler der übrigen schnellen Schüsse ist pro Schütze und wird gespeichert.

### 5.3a Sicht (Entscheidung: Variante B)
- Solange mindestens ein Bogenschütze im Turm angekommen ist, deckt der Turm einen Radius von 7 Feldern um sich auf (benannte Konstante `VISUALRANGE_ARCHERTOWER = 7`, gleich der Schussreichweite).
- Vorbild ist der Spähturm: `makeVisibleAroundPoint` beim Eintreffen des ersten Schützen (`workplaceReached`), Neuberechnung (`recalcVisibilitiesAroundPoint`) beim Verlust des letzten Schützen oder der Zerstörung des Turms (`workAborted`/`DestroyBuilding`).
  Außerdem wird der Turm wie `LookoutTower` in der Sichtberechnung in `GameWorld.cpp:1066-1079` berücksichtigt (Gebäude mit `hasWorker()`), damit die Sicht bei jeder Neuberechnung erhalten bleibt.
- Sicht gilt für den Besitzer (und Verbündete über die vorhandene Team-Sicht). Kein Effekt auf andere Spieler.

### 5.4 Bedienung
- „Feuer einstellen / Feuer frei“ nutzt den vorhandenen Produktions-Stopp (`SetProductionEnabled`, Button im Gebäudefenster).
- Zielpriorität: neuer GameCommand `SetArcherTargetPriority(MapPoint, Wert)` (am Ende von `GCType`), Eigentümer-Prüfung im `execute`, Zustand pro Turm gespeichert.
- Eigenes Fenster `iwArcherTower` (abgeleitet von `iwBuilding`): drei Auswahl-Buttons für die Zielpriorität, Weinanzeige, Anzeige der besetzten Plätze.

### 5.5 Zerstörung
- Wie ein beliebiges `nobUsual` bei Gebietsverlust (`destroyPlayerRests` → `destroy`). Die Schützen werden über `lostWork` frei und laufen zu einem Lager oder sterben, wenn kein Weg existiert.
- Nicht angreifbar, nicht von Katapulten beschießbar, keine Fahnen- oder Nahkämpfe.

### 5.6 Speichern, Netzwerk, Lua
- `currentGameDataVersion` 15 → 16 mit Changelog; Versions-Guards für alle aufzählungsgroßen, ohne Längenangabe gespeicherten Arrays (Bauordnung, Inventar-Berufe, Gebäude-Register, Warenhaus-Einstellungen), nach dem Vorbild von Wine/Leather.
- Neue `GO_Type`-Werte am Ende, Registrierung in `SerializedGameData`. Alte Spielstände laden und enthalten dann null Bogenschützen/Türme.
- GameCommand `SetArcherTargetPriority` am Ende von `GCType`.
- Lua: `BLD_ARCHERTOWER`, `JOB_ARCHER` in `LuaInterfaceGame`, Doku `doc/lua/constants.md`, Test `testLuaConstants`.
- KI: keine Planung des Turms (`buildingsWanted` = 0). Die KI darf bei Existenz des Turms oder des Berufs nicht abstürzen (z. B. bei Werkzeugquoten, Bau-Listen, Tabellen mit Aufzählungs-Größe).

## 6. Einschränkungen (Constraints)

- Alle `EnumArray`-Tabellen haben keine Längenprüfung, `BUILDING_NAMES`/`BUILDING_HELP_STRINGS` dürfen keine `nullptr` enthalten, `getBuildingRadius` hat keinen `default` (Compile-Fehler bei `-Werror`).
- `militaryBldTypes` bleibt bei 4 (`ArcherTower` ist kein Militärgebäude).
- `iwBuildingProductivities::allIcons` ist ein `std::array` fester Größe; `iwBuildOrder` ergänzt versteckte Gebäude am Ende.
- Keine Magic Constants: Reichweite, Trefferchance, Schusszahl, Wein-Plätze, Feuerpausen als benannte Konstanten.
- Fortschritt im Netzwerk nur über Events und GameCommands (`RANDOM_*`-Makros des Spiel-RNG).
- Sicherstellen, dass kein Zustand des Turms von der Spielgeschwindigkeit abhängt (nur GF-Zeiten).

## 7. Was dem Bauenden überlassen bleibt

- Dach-Offsets der vier Schützen je Nation (visuell abstimmen).
- Exakte Aufräum-Reihenfolge, wenn ein Soldat durch den Fernkampf stirbt (Vorbild `lostFighting`/`informTargetsAboutCancelling`, `noFigure::die`).
- Zielsuche-Intervall und Details des Event-Zustandsautomaten (neue `State`-Werte in `nofBuildingWorker`).
- Tabellenwerte für Hilfetexte, Smoke/Schilder (aus LookoutTower kopieren).
- Aufteilung des Plans in Aufgaben und Reihenfolge der Commits innerhalb des einen PR.
- Wie der Addon-Zwang technisch normalisiert wird (so wenig neuer Mechanismus wie möglich).

## 8. Tests (Pflicht für Upstream)

- Addon: Registrierung, Serialisierung der Einstellungen, Wein-Abhängigkeit (Archer an → Wein an), Gebäude nur sichtbar/baubar, wenn beide aktiv sind.
- Tabellen: Kosten, Größe, Name nicht null, `getBuildingRadius` für `ArcherTower`, Lua-Konstanten.
- Rekrutierung: Archer entsteht aus Träger + Bogen, kein Archer ohne Bogen.
- Turm: 4 Arbeiter werden angefordert, Ersatz bei Ausfall, Zerstörung bei Gebietsverlust (Schützen verlassen das Gebäude), Wein-Lieferung bis 3.
- Sicht: Radius 7 ab erstem Schützen, Rückbau bei Verlust des letzten Schützen bzw. Zerstörung.
- Schuss: Zielwahl pro Priorität (Nächstes/Schwächstes/Stärkstes, Späher), keine Ziele im Nahkampf/im Gebäude/Zivilisten, Reichweite 7, Feuerrate 140/100 GF, Wein verbraucht 1 pro 10 Schuss, 70 % Trefferchance über feste RNG-Folge.
- Schaden: Soldat verliert 1 HP, Rüstung zuerst, Tod räumt Verweise sauber auf (kein Assert), Späher stirbt bei einem Treffer.
- GameCommand `SetArcherTargetPriority`: Ausführung, Besitzer-Prüfung, Serialisierung.
- Savegame: Laden eines Spielstands der Version 15, Speichern/Laden mit Turm und Schützen (inkl. Wein-Zähler), Replay.
- Bestehende Suiten (`Test_integration`, `Test_simple`, `Test_UI`, `Test_lua`) bleiben grün.

## 9. Offene Annahmen zur Prüfung

1. Addon-ID `0x01400000`.
2. Welche Soldaten Ziel sind (alle aktiven, nicht kämpfenden Soldaten außerhalb von Gebäuden; keine Passivsoldaten auf dem Weg zum Gebäude).
3. Wein wird erst beim Schuss mit Ziel verbraucht, nicht beim bloßen Warten.
4. „Feuer einstellen“ = vorhandener Produktions-Stopp.
5. Kein sichtbares Projektil; der Treffer wirkt am Ende der Schussanimation.
6. Bei Priorität nach Rang wird bei Gleichstand nach Entfernung, dann Objekt-ID entschieden.

## 10a. Entscheidung zur Sicht

Variante B (Auftraggeber): Turm deckt mit mindestens einem Schützen 7 Felder auf; siehe 5.3a. Tests: Sicht entsteht beim Eintreffen des ersten Schützen und verschwindet bei Zerstörung/Verlust des letzten; Ziel im Radius 7 wird ohne weitere Einheiten beschossen.

## 10. Ergebnisse des Builder-Checks (meine Entscheidungen, bitte prüfen)

1. **Wein-Konkurrenz Tempel/Turm:** Beide konkurrieren über die vorhandene Bewertung in `findClientForWare` (`calcDistributionPoints`); kein Schutz des Tempels, keine Slider.
   Für alte Spielstände (Version < 16) wird der Turm beim Laden als zusätzlicher Wein-Client ergänzt, falls das Archer-Addon aktiv ist.
2. **Vier Arbeiter:** Slot 0 bleibt `nobUsual::worker`, die Slots 1–3 liegen in `nobArcherTower`. `nobUsual` wird nur an den nötigen Stellen virtuell bzw. minimal angepasst (Anforderung, `workerLost`, `DestroyBuilding`, Tür, `hasWorker`); die Speicherung der Zusatzslots geschieht in `nobArcherTower::serialize`, damit das Format der anderen `nobUsual`-Gebäude unverändert bleibt.
   Der Turm schießt, sobald **ein** Schütze da ist. „Feuer einstellen“ gilt für alle vier.
3. **Ungültiges Ziel am Ende der Animation:** Der Treffer wird nur angewendet, wenn das Ziel noch gültig ist (lebt, nicht im Nahkampf, nicht im Gebäude). Sonst verfällt der Schuss. Der Wurf (70 %) erfolgt beim Anwenden. Mehrfach-Beschuss desselben Ziels (Overkill) ist erlaubt, es gibt keine Zielreservierung.
