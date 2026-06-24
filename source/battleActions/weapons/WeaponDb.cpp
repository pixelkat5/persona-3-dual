#include "WeaponDb.h"
Weapon WeaponDb::shortSword;
Weapon WeaponDb::imitationKatana;
Weapon WeaponDb::practiceBow;

void WeaponDb::Initialize()
{
    shortSword.weaponPower = 38;
    shortSword.hitRate = 95;
    shortSword.weaponType = WeaponType::OneHandedSword;

    imitationKatana.weaponPower = 45;
    imitationKatana.hitRate = 92;
    imitationKatana.weaponType = WeaponType::TwoHandedSword;

    imitationKatana.weaponPower = 30;
    imitationKatana.hitRate = 98;
    imitationKatana.weaponType = WeaponType::Bow;
}
