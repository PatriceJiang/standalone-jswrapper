#include "Tank_setpl.h"

#include "Tank.h"
#include "class.h"

bool js_register_war_Tank(se::Object *obj) {
    setpl::class_<war::Tank> tank("Tank2");
    tank.ctor<std::string>()
        .ctor<int, std::string>()
        .attribute("name", &war::Tank::getName, &war::Tank::setName)
        .method("fire", static_cast<void (war::Tank::*)(std::string)>(&war::Tank::fire))
        .method("fire", static_cast<void (war::Tank::*)(float, float, float)>(&war::Tank::fire))
        .field("id", &war::Tank::id)
        .attribute("badId", &war::Tank::badID, &war::Tank::updateBadId)
        .attribute("readOnlyId", &war::Tank::badID, nullptr)
        .install();

    return true;
}

bool register_all_war(se::Object* obj) {
    js_register_war_Tank(obj);
    return true;
}