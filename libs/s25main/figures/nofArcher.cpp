// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofArcher.h"
#include "buildings/nobArcherTower.h"

nofArcher::nofArcher(const MapPoint pos, const unsigned char player, nobArcherTower* workplace)
    : nofBuildingWorker(Job::Archer, pos, player, workplace)
{}

nofArcher::nofArcher(SerializedGameData& sgd, const unsigned obj_id) : nofBuildingWorker(sgd, obj_id) {}

void nofArcher::walkedDerived() {}

void nofArcher::drawWorking(DrawPoint /*drawPt*/) {}

void nofArcher::handleDerivedEvent(const unsigned /*id*/) {}

bool nofArcher::areWaresAvailable() const
{
    // No shooting yet
    return false;
}
