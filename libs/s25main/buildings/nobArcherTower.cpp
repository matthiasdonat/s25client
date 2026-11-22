// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nobArcherTower.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "SerializedGameData.h"
#include "figures/nofArcher.h"
#include "helpers/containerUtils.h"
#include "world/GameWorld.h"
#include <algorithm>

nobArcherTower::nobArcherTower(const MapPoint pos, const unsigned char player, const Nation nation)
    : nobUsual(BuildingType::ArcherTower, pos, player, nation), archers_{}
{}

nobArcherTower::nobArcherTower(SerializedGameData& sgd, const unsigned obj_id) : nobUsual(sgd, obj_id)
{
    for(nofArcher*& archer : archers_)
        archer = sgd.popObject<nofArcher>(GO_Type::NofArcher);
}

nobArcherTower::~nobArcherTower() = default;

void nobArcherTower::serialize(SerializedGameData& sgd) const
{
    nobUsual::serialize(sgd);
    for(const nofArcher* archer : archers_)
        sgd.pushObject(archer, true);
}

unsigned nobArcherTower::getNumArchers() const
{
    return static_cast<unsigned>(std::count_if(archers_.begin(), archers_.end(), [](const auto* a) { return a; }));
}

unsigned nobArcherTower::getNumPresentArchers() const
{
    return static_cast<unsigned>(std::count_if(archers_.begin(), archers_.end(), [](const nofArcher* a) {
        return a && a->getState() != nofBuildingWorker::State::FigureWork;
    }));
}

unsigned nobArcherTower::getSlotOf(const nofArcher& archer) const
{
    const auto it = std::find(archers_.begin(), archers_.end(), &archer);
    RTTR_Assert(it != archers_.end());
    return static_cast<unsigned>(it - archers_.begin());
}

bool nobArcherTower::hasWorker() const
{
    return getNumPresentArchers() > 0;
}

void nobArcherTower::gotWorker(Job /*job*/, noFigure& worker)
{
    auto* archer = checkedCast<nofArcher*>(&worker);
    const auto freeSlot = std::find(archers_.begin(), archers_.end(), nullptr);
    RTTR_Assert(freeSlot != archers_.end());
    *freeSlot = archer;

    // Order wares only once
    if(!orderware_ev)
        handleEvent(0);
}

void nobArcherTower::workerArrived()
{
    // The first archer closes the door and starts the productivity calculation
    if(!productivity_ev)
    {
        nobUsual::workerArrived();
        world->makeVisibleAroundPoint(pos, VISUALRANGE_ARCHERTOWER, player);
    }
}

void nobArcherTower::workerLost(nofBuildingWorker& lostWorker)
{
    const auto it = std::find(archers_.begin(), archers_.end(), &lostWorker);
    RTTR_Assert(it != archers_.end());
    *it = nullptr;

    if(getNumPresentArchers() == 0)
    {
        // Open the door till we get a new archer
        if(productivity_ev)
        {
            openDoor();
            getEvMgr().removeEvent(productivity_ev);
            world->recalcVisibilitiesAroundPoint(pos, VISUALRANGE_ARCHERTOWER, player, this);
        }
    }
    if(getNumArchers() == 0)
        getEvMgr().removeEvent(orderware_ev);

    world->getPlayer(player).addJobWanted(Job::Archer, this);
}

void nobArcherTower::DestroyBuilding()
{
    const bool hadVision = productivity_ev != nullptr;
    for(nofArcher*& archer : archers_)
    {
        if(archer)
        {
            archer->lostWork();
            archer = nullptr;
        }
    }
    world->getPlayer(player).jobNotWanted(this, true);

    nobUsual::DestroyBuilding();

    // The tower is still registered at this point, so exclude it from the visibility calculation
    if(hadVision)
        world->recalcVisibilitiesAroundPoint(pos, VISUALRANGE_ARCHERTOWER, player, this);
}

void nobArcherTower::notifyWorkersAboutWareOrProductionAllowed()
{
    for(nofArcher* archer : archers_)
    {
        if(archer)
            archer->gotWareOrProductionAllowed();
    }
}

void nobArcherTower::notifyWorkersAboutProductionStop()
{
    for(nofArcher* archer : archers_)
    {
        if(archer)
            archer->productionStopped();
    }
}
