// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for the archer tower (requires the wine economy)
 */
class AddonArcherTower : public AddonBool
{
public:
    AddonArcherTower()
        : AddonBool(AddonId::ARCHER_TOWER, AddonGroup::Economy, _("Enable archer tower"),
                    _("Allows to build the archer tower, which shoots enemy soldiers and scouts. Requires the wine "
                      "economy, which is enabled automatically."))
    {}
};
