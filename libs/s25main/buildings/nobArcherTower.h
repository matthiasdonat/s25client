// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nobUsual.h"
#include "figures/ArcherTargeting.h"
#include "gameData/ArcherTowerConsts.h"
#include <array>

class nofArcher;
class SerializedGameData;

/// Tower with up to MAX_ARCHERS_PER_TOWER archers. All archers are stored in the tower, nobUsual::worker stays unused.
class nobArcherTower : public nobUsual
{
public:
    nobArcherTower(const nobArcherTower&) = delete;
    ~nobArcherTower() override;

    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NobArcherTower; }

    /// Number of archers assigned to this tower (including those still walking to it)
    unsigned getNumArchers() const;
    /// Number of archers that arrived at the tower
    unsigned getNumPresentArchers() const;
    /// Archer in the given slot or nullptr if the slot is empty
    nofArcher* getArcher(unsigned slot) const { return archers_[slot]; }
    /// Slot of the given archer. The archer must belong to this tower
    unsigned getSlotOf(const nofArcher& archer) const;
    /// Which targets the archers prefer
    ArcherTargetPriority getTargetPriority() const { return targetPriority_; }
    void setTargetPriority(const ArcherTargetPriority priority)
    {
        targetPriority_ = priority;
        targetPriorityVirtual_ = priority;
    }
    /// Value shown in the UI: Changed immediately when the player sends a command, before the command is executed
    ArcherTargetPriority getTargetPriorityVirtual() const { return targetPriorityVirtual_; }
    void setTargetPriorityVirtual(const ArcherTargetPriority priority) { targetPriorityVirtual_ = priority; }
    /// True if at least one archer arrived
    bool hasWorker() const override;

    void gotWorker(Job job, noFigure& worker) override;
    void workerArrived() override;
    void workerLost(nofBuildingWorker& lostWorker) override;

protected:
    void DestroyBuilding() override;
    void notifyWorkersAboutWareOrProductionAllowed() override;
    void notifyWorkersAboutProductionStop() override;

private:
    friend class SerializedGameData;
    friend class BuildingFactory;
    nobArcherTower(MapPoint pos, unsigned char player, Nation nation);
    nobArcherTower(SerializedGameData& sgd, unsigned obj_id);

    std::array<nofArcher*, MAX_ARCHERS_PER_TOWER> archers_;
    ArcherTargetPriority targetPriority_ = ArcherTargetPriority::Nearest;
    ArcherTargetPriority targetPriorityVirtual_ = ArcherTargetPriority::Nearest;
};
