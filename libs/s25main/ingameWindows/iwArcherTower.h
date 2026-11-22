// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ingameWindows/iwBuilding.h"

class nobUsual;
class nobArcherTower;
class GameWorldView;
class GameCommandFactory;

/// Window of an archer tower: Additionally allows to choose which targets the archers prefer
class iwArcherTower : public iwBuilding
{
public:
    iwArcherTower(GameWorldView& gwv, GameCommandFactory& gcFactory, nobUsual* building);

private:
    nobArcherTower& tower() const;
    /// Highlight the button of the current priority
    void updatePriorityButtons();
    void msgButtonClick(unsigned ctrl_id) override;
};
