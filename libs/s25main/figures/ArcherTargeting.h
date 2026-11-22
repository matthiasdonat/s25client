// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/MapCoordinates.h"
#include <cstdint>
#include <vector>

class GameWorldBase;
class noFigure;

/// How the archers of a tower choose between several targets
enum class ArcherTargetPriority : uint8_t
{
    Nearest,
    Weakest,
    Strongest
};
constexpr auto maxEnumValue(ArcherTargetPriority)
{
    return ArcherTargetPriority::Strongest;
}

/// A figure an archer could shoot at
struct ArcherTarget
{
    noFigure* figure;
    /// Distance to the tower
    unsigned distance;
    /// Rank of a soldier (0 for scouts)
    unsigned rank;
    bool isScout;
    unsigned objectId;
};

/// Find all enemy soldiers and scouts the archers of a tower at the given position may shoot at
std::vector<ArcherTarget> findArcherTargets(const GameWorldBase& world, MapPoint towerPos, unsigned char player);

/// Choose the target according to the priority or nullptr if there are none.
/// Nearest: smallest distance. Weakest: lowest rank, scouts first. Strongest: highest rank, scouts last.
/// Ties are decided by distance and then by object id so the choice is deterministic
const ArcherTarget* chooseArcherTarget(const std::vector<ArcherTarget>& targets, ArcherTargetPriority priority);
