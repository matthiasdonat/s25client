// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwArcherTower.h"
#include "Loader.h"
#include "buildings/nobArcherTower.h"
#include "controls/ctrlTextButton.h"
#include "enum_cast.hpp"
#include "factories/GameCommandFactory.h"
#include "helpers/EnumRange.h"
#include "ogl/FontStyle.h"
#include "world/GameWorldView.h"

namespace {
/// Control id of the first priority button, the others follow in the order of ArcherTargetPriority
constexpr unsigned ID_FirstPriorityButton = 20;

const char* getPriorityText(const ArcherTargetPriority priority)
{
    switch(priority)
    {
        case ArcherTargetPriority::Nearest: return gettext_noop("Nearest");
        case ArcherTargetPriority::Weakest: return gettext_noop("Weakest");
        case ArcherTargetPriority::Strongest: return gettext_noop("Strongest");
    }
    return "";
}
} // namespace

iwArcherTower::iwArcherTower(GameWorldView& gwv, GameCommandFactory& gcFactory, nobUsual* const building)
    : iwBuilding(gwv, gcFactory, building, Extent(226, 223))
{
    getCtrl<Window>(1)->setPos(DrawPoint(117, 160));
    constexpr Extent btSize(62, 22);
    for(const auto priority : helpers::enumRange<ArcherTargetPriority>())
    {
        addTextButton(ID_FirstPriorityButton + rttr::enumCast(priority), DrawPoint(10 + 68 * rttr::enumCast(priority), 120),
                      btSize, TextureColor::Grey, _(getPriorityText(priority)), SmallFont,
                      _("Preferred target of the archers"));
    }
    updatePriorityButtons();
}

nobArcherTower& iwArcherTower::tower() const
{
    return static_cast<nobArcherTower&>(*building);
}

void iwArcherTower::updatePriorityButtons()
{
    for(const auto priority : helpers::enumRange<ArcherTargetPriority>())
    {
        auto* button = getCtrl<ctrlTextButton>(ID_FirstPriorityButton + rttr::enumCast(priority));
        button->setChecked(tower().getTargetPriorityVirtual() == priority);
    }
}

void iwArcherTower::msgButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id >= ID_FirstPriorityButton && ctrl_id < ID_FirstPriorityButton + helpers::NumEnumValues_v<ArcherTargetPriority>)
    {
        const auto priority = static_cast<ArcherTargetPriority>(ctrl_id - ID_FirstPriorityButton);
        if(gcFactory.SetArcherTargetPriority(building->getPos(), priority))
        {
            tower().setTargetPriorityVirtual(priority);
            updatePriorityButtons();
        }
    } else
        iwBuilding::msgButtonClick(ctrl_id);
}
