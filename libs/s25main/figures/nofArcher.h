// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofBuildingWorker.h"

class nobArcherTower;
class SerializedGameData;

/// Archer working in an archer tower
class nofArcher : public nofBuildingWorker
{
protected:
    /// Called by the base class when the archer walked
    void walkedDerived() override;
    /// Draws the archer while working
    void drawWorking(DrawPoint drawPt) override;
    /// Archers never carry something
    unsigned short getCarryID() const override { return 0; }
    /// Archers only work (shoot) when there is something to shoot at, see nofArcher::tryToWork
    bool areWaresAvailable() const override;

public:
    nofArcher(MapPoint pos, unsigned char player, nobArcherTower* workplace);
    nofArcher(SerializedGameData& sgd, unsigned obj_id);

    GO_Type getGOT() const final { return GO_Type::NofArcher; }

    void handleDerivedEvent(unsigned id) override;
};
