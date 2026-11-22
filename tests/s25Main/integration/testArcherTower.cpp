// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AddonHelperFunctions.h"
#include "GlobalGameSettings.h"
#include "addons/const_addons.h"
#include <s25util/Serializer.h>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ArcherTowerSuite)

BOOST_AUTO_TEST_CASE(AddonRegistered)
{
    GlobalGameSettings ggs;
    BOOST_TEST_REQUIRE(ggs.getAddon(AddonId::ARCHER_TOWER) != nullptr);
    BOOST_TEST(ggs.getSelection(AddonId::ARCHER_TOWER) == 0u);
    BOOST_TEST(!isArcherTowerActive(ggs));
}

BOOST_AUTO_TEST_CASE(EnablingArcherEnablesWine)
{
    GlobalGameSettings ggs;
    ggs.setSelection(AddonId::ARCHER_TOWER, 1);
    BOOST_TEST(ggs.isEnabled(AddonId::WINE));
    BOOST_TEST(isArcherTowerActive(ggs));
}

BOOST_AUTO_TEST_CASE(DisablingWineKeepsArcherInactive)
{
    GlobalGameSettings ggs;
    ggs.setSelection(AddonId::ARCHER_TOWER, 1);
    ggs.setSelection(AddonId::WINE, 0);
    BOOST_TEST(!isArcherTowerActive(ggs));
}

BOOST_AUTO_TEST_CASE(SettingsSerializationNormalises)
{
    GlobalGameSettings ggs;
    ggs.setSelection(AddonId::ARCHER_TOWER, 1);
    ggs.setSelection(AddonId::WINE, 0);
    Serializer ser;
    ggs.serialize(ser);

    GlobalGameSettings loaded;
    loaded.deserialize(ser);
    BOOST_TEST(loaded.isEnabled(AddonId::ARCHER_TOWER));
    BOOST_TEST(loaded.isEnabled(AddonId::WINE));
    BOOST_TEST(isArcherTowerActive(loaded));
}

BOOST_AUTO_TEST_SUITE_END()
