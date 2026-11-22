// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ArcherTargeting.h"
#include "GamePlayer.h"
#include "figures/nofActiveSoldier.h"
#include "figures/nofScout_Free.h"
#include "world/GameWorldBase.h"
#include "gameData/ArcherTowerConsts.h"
#include <tuple>

std::vector<ArcherTarget> findArcherTargets(const GameWorldBase& world, const MapPoint towerPos,
                                            const unsigned char player)
{
    std::vector<ArcherTarget> targets;
    const GamePlayer& owner = world.getPlayer(player);
    for(const MapPoint pt : world.getPointsInRadiusWithCenter(towerPos, ARCHER_TOWER_RANGE))
    {
        if(world.getNode(pt).fow[player].visibility != Visibility::Visible)
            continue;
        for(noBase& obj : world.getFigures(pt))
        {
            auto* figure = dynamic_cast<noFigure*>(&obj);
            if(!figure || !owner.isAttackable(figure->getPlayer()))
                continue;
            const unsigned distance = world.calcDistance(towerPos, pt);
            if(auto* scout = dynamic_cast<nofScout_Free*>(figure))
            {
                if(scout->isValidRangedTarget())
                    targets.push_back({scout, distance, 0u, true, scout->getObjId()});
            } else if(auto* soldier = dynamic_cast<nofActiveSoldier*>(figure))
            {
                if(soldier->isValidRangedTarget())
                    targets.push_back({soldier, distance, soldier->getRank(), false, soldier->getObjId()});
            }
        }
    }
    return targets;
}

const ArcherTarget* chooseArcherTarget(const std::vector<ArcherTarget>& targets, const ArcherTargetPriority priority)
{
    // Lower key = better target
    const auto key = [priority](const ArcherTarget& t) {
        switch(priority)
        {
            case ArcherTargetPriority::Nearest: return std::make_tuple(0u, t.distance, t.objectId);
            case ArcherTargetPriority::Weakest:
                return std::make_tuple(t.isScout ? 0u : 1u + t.rank, t.distance, t.objectId);
            case ArcherTargetPriority::Strongest:
            default:
                // Scouts last, then the highest rank first
                return std::make_tuple(t.isScout ? 1000u : 100u - t.rank, t.distance, t.objectId);
        }
    };
    const ArcherTarget* best = nullptr;
    for(const ArcherTarget& target : targets)
    {
        if(!best || key(target) < key(*best))
            best = &target;
    }
    return best;
}
