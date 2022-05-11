#include "Tank_setpl.h"

#include "Tank.h"
#include "class.h"

std::string weapon_power(const war::Weapon* w) {
    return "godgodogd";
}

bool js_register_war_Tank(se::Object *obj) {
    setpl::class_<war::Weapon> weapon("Weapon");

    weapon.namespaceObject(obj)
        .ctor<int>()
        .function("fire2", &war::Weapon::fire2)
        .function("power", &weapon_power)
       .install();

    setpl::class_<war::Tank> tank("Tank2");
    tank.namespaceObject(obj)
        .inherits(weapon.prototype())
        .ctor<std::string>()
        .ctor<int, std::string>()
        .property("name", &war::Tank::getName, &war::Tank::setName)
        .function("fire", static_cast<void (war::Tank::*)(std::string)>(&war::Tank::fire))
        .function("fire", static_cast<void (war::Tank::*)(float, float, float)>(&war::Tank::fire))
        .field("id", &war::Tank::id)
        .property("badId", &war::Tank::badID, &war::Tank::updateBadId)
        .property("readOnlyId", &war::Tank::badID, nullptr)
        .property("writeOnlyId", nullptr, &war::Tank::updateBadId)
        .install();

    return true;
}

bool register_all_war(se::Object *obj) {
    js_register_war_Tank(obj);
    return true;
}