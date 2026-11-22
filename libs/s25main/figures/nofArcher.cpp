// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofArcher.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "buildings/nobArcherTower.h"
#include "enum_cast.hpp"
#include "figures/ArcherTargeting.h"
#include "figures/nofActiveSoldier.h"
#include "figures/nofScout_Free.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "gameData/ArcherTowerConsts.h"
#include <algorithm>
#include <cstdlib>

namespace {
/// Direction to face when shooting from one point to another (the world wraps around)
Direction getShootingDirection(const GameWorldBase& world, const MapPoint from, const MapPoint to)
{
    int distX = to.x - from.x;
    if(distX > world.getWidth() / 2)
        distX -= world.getWidth();
    else if(distX < -static_cast<int>(world.getWidth() / 2))
        distX += world.getWidth();
    int distY = to.y - from.y;
    if(distY > world.getHeight() / 2)
        distY -= world.getHeight();
    else if(distY < -static_cast<int>(world.getHeight() / 2))
        distY += world.getHeight();

    const bool targetIsRight = distX >= 0;
    // Nearly horizontal: Use east/west, otherwise one of the 4 quadrants
    if(std::abs(distY) <= std::abs(distX) / 5)
        return targetIsRight ? Direction::East : Direction::West;
    if(distY > 0)
        return targetIsRight ? Direction::SouthEast : Direction::SouthWest;
    return targetIsRight ? Direction::NorthEast : Direction::NorthWest;
}
} // namespace

nofArcher::nofArcher(const MapPoint pos, const unsigned char player, nobArcherTower* workplace)
    : nofBuildingWorker(Job::Archer, pos, player, workplace), boostedShotsLeft_(0), shotBoosted_(false), targetId_(0),
      shootingDir_(Direction::SouthEast)
{}

nofArcher::nofArcher(SerializedGameData& sgd, const unsigned obj_id)
    : nofBuildingWorker(sgd, obj_id), boostedShotsLeft_(sgd.popUnsignedChar()), shotBoosted_(sgd.popBool()),
      targetId_(sgd.popUnsignedInt()), shootingDir_(sgd.pop<Direction>())
{}

void nofArcher::serialize(SerializedGameData& sgd) const
{
    nofBuildingWorker::serialize(sgd);
    sgd.pushUnsignedChar(boostedShotsLeft_);
    sgd.pushBool(shotBoosted_);
    sgd.pushUnsignedInt(targetId_);
    sgd.pushEnum<uint8_t>(shootingDir_);
}

const nobArcherTower& nofArcher::getTower() const
{
    return static_cast<const nobArcherTower&>(*workplace);
}

void nofArcher::walkedDerived() {}

void nofArcher::tryToWork()
{
    if(workplace->isProductionDisabled())
    {
        state = State::WaitingForWaresOrProductionStopped;
        workplace->startNotWorking();
    } else
        waitFor(ARCHER_SEARCH_INTERVAL_GFS);
}

void nofArcher::waitFor(const unsigned numGFs)
{
    if(workplace->isProductionDisabled())
    {
        state = State::WaitingForWaresOrProductionStopped;
        workplace->startNotWorking();
        return;
    }
    state = State::Waiting1;
    current_ev = getEvMgr().addEvent(this, numGFs, 1);
    workplace->stopNotWorking();
}

void nofArcher::handleDerivedEvent(const unsigned /*id*/)
{
    // The event is done (and deleted afterwards), so forget it
    current_ev = nullptr;
    switch(state)
    {
        default: RTTR_Assert(false); break;
        case State::Waiting1: searchTarget(); break;
        case State::ArcherShooting: finishShot(); break;
    }
}

void nofArcher::searchTarget()
{
    const nobArcherTower& tower = getTower();
    const auto targets = findArcherTargets(*world, tower.getPos(), player);
    const ArcherTarget* target = chooseArcherTarget(targets, tower.getTargetPriority());
    if(!target)
    {
        waitFor(ARCHER_SEARCH_INTERVAL_GFS);
        return;
    }

    // Use up a wine when the last boost is used up
    if(boostedShotsLeft_ == 0 && workplace->waresAvailable())
    {
        workplace->consumeWares();
        boostedShotsLeft_ = ARCHER_BOOST_SHOTS;
    }
    shotBoosted_ = boostedShotsLeft_ > 0;
    if(shotBoosted_)
        --boostedShotsLeft_;

    targetId_ = target->objectId;
    shootingDir_ = getShootingDirection(*world, pos, target->figure->getPos());
    state = State::ArcherShooting;
    current_ev = getEvMgr().addEvent(this, ARCHER_SHOT_ANIMATION_GFS, 1);
}

void nofArcher::finishShot()
{
    // Re-resolve the target: It might have died, entered a building or started a fight in the meantime
    const auto targets = findArcherTargets(*world, getTower().getPos(), player);
    const auto itTarget = std::find_if(targets.begin(), targets.end(),
                                       [id = targetId_](const ArcherTarget& t) { return t.objectId == id; });
    targetId_ = 0;
    if(itTarget != targets.end() && RANDOM_RAND(100) < static_cast<int>(ARCHER_HIT_CHANCE_PERCENT))
    {
        if(auto* scout = dynamic_cast<nofScout_Free*>(itTarget->figure))
            scout->killByRangedHit();
        else if(auto* soldier = dynamic_cast<nofActiveSoldier*>(itTarget->figure))
            soldier->takeRangedHit();
    }
    waitFor((shotBoosted_ ? ARCHER_BOOSTED_SHOT_INTERVAL_GFS : ARCHER_SHOT_INTERVAL_GFS) - ARCHER_SHOT_ANIMATION_GFS);
}

void nofArcher::drawOtherStates(DrawPoint drawPt)
{
    // Waiting on the roof
    if(state == State::Waiting1 || state == State::WaitingForWaresOrProductionStopped)
    {
        const GamePlayer& owner = world->getPlayer(player);
        const auto& offset = ARCHER_ROOF_OFFSETS[getTower().getSlotOf(*this)];
        LOADER.getBobSprite(owner.nation, Job::Hunter, shootingDir_, 0)
          .drawForPlayer(drawPt + DrawPoint(offset[0], offset[1]), owner.color);
    }
}

void nofArcher::drawWorking(DrawPoint drawPt)
{
    if(state != State::ArcherShooting)
        return;
    const GamePlayer& owner = world->getPlayer(player);
    const auto& offset = ARCHER_ROOF_OFFSETS[getTower().getSlotOf(*this)];
    drawPt += DrawPoint(offset[0], offset[1]);
    if(shootingDir_ == Direction::East)
    {
        // The animation in this direction differs from the others
        const unsigned short id = GAMECLIENT.interpolate(13, current_ev);
        LOADER.getPlayerImage("rom_bobs", 219 + id)->drawForPlayer(drawPt, owner.color);
        if(id == 12)
            world->getSoundMgr().playNOSound(74, *this, 0);
    } else
    {
        const unsigned short id = GAMECLIENT.interpolate(8, current_ev);
        LOADER.getPlayerImage("rom_bobs", 1686 + rttr::enumCast(shootingDir_ + 2u) * 8 + id)
          ->drawForPlayer(drawPt, owner.color);
        if(id == 7)
            world->getSoundMgr().playNOSound(74, *this, 0);
    }
}
