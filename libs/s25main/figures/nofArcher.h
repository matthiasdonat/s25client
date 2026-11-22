// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofBuildingWorker.h"
#include "gameTypes/Direction.h"

class nobArcherTower;
class SerializedGameData;

/// Archer working in an archer tower: Looks for enemy soldiers and scouts in range and shoots at them
class nofArcher : public nofBuildingWorker
{
    /// Number of shots left that use the (faster) wine boost
    uint8_t boostedShotsLeft_;
    /// Is the current shot a boosted one (determines the time until the next shot)
    bool shotBoosted_;
    /// Object id of the current target (0 if none)
    unsigned targetId_;
    /// Direction the archer faces
    Direction shootingDir_;

    /// Tower this archer belongs to
    const nobArcherTower& getTower() const;

    /// Looks for a target and starts shooting or keeps waiting
    void searchTarget();
    /// Shooting animation is done: Apply the hit if the target is still valid and wait for the next shot
    void finishShot();
    /// Wait for the next search/shot (or stop if production is disabled)
    void waitFor(unsigned numGFs);

protected:
    /// Called by the base class when the archer walked
    void walkedDerived() override;
    /// Draws the archer while shooting
    void drawWorking(DrawPoint drawPt) override;
    /// Draws the waiting archer
    void drawOtherStates(DrawPoint drawPt) override;
    /// Archers never carry something
    unsigned short getCarryID() const override { return 0; }
    /// Archers start waiting for the next target unless production is stopped
    void tryToWork() override;

public:
    nofArcher(MapPoint pos, unsigned char player, nobArcherTower* workplace);
    nofArcher(SerializedGameData& sgd, unsigned obj_id);

    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NofArcher; }

    void handleDerivedEvent(unsigned id) override;

    /// Number of remaining shots with the wine boost
    unsigned getBoostedShotsLeft() const { return boostedShotsLeft_; }
};
