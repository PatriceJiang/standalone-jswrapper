#include "Tank_setpl.h"

#include <sstream>

#include "Tank.h"
#include "class.h"


// extension functions for 
namespace {
std::string weapon_power(const war::Weapon *w) {
    return "weapon_power";
}

auto *weapon_add_id(war::Weapon *w, int arg) {
    w->seqId += arg;
    return w;
}

std::string weapon_toString(war::Weapon *w) {
    std::stringstream ss;
    ss << "Object[Weapon: id " << w->seqId << "]" << std::endl;
    return ss.str();
}

war::Tank *construct_tank(int id, std::string name, int ) {
    return new war::Tank(id * 10000, name + " ants");
}

void Tank_Sleep() {
    std::cout << "All tank is sleeping ... " << std::endl;
}
void Tank_Sleep2(int n) {
    std::cout << "All tank is sleeping ... " << n << std::endl;
}

int Tank_random() {
    return rand();
}

void Tank_seed(int seed) {
    srand(seed);
}
std::string Tank_nick(war::Tank *n) {
    static int        id = 333;
    std::stringstream ss;
    ss << " nickname from " << __FUNCTION__ << " " << (++id);
    return ss.str();
}
} // namespace


bool js_register_war_Tank(se::Object *obj) {
    sebind::class_<war::Weapon> weapon("Weapon");

    weapon.ctor(+[](float r) { return new war::Weapon(); })
        .ctor<int>()
        .function("fire2", &war::Weapon::fire2)
        .function("power", &weapon_power)
        .function("addid", &weapon_add_id)
        .function("toString", &weapon_toString)
        .withRawClass([](se::Class *kls) {
            // do something with se::Class* directly!
            std::cout << "class name ---> " << kls->getName() << std::endl;
        })
        .gcCallback(sebind::optional_lambda([](war::Weapon *w) {
            std::cout << " dying ... " << w->seqId << std::endl;
        }))
        .install(obj);

    sebind::class_<war::Tank> tank("Tank2");
    tank.ctor(&construct_tank)
        .ctor<std::string>()
        .ctor<int, std::string>()
        .inherits(weapon.prototype())
        .field("id", &war::Tank::seqId)
        .property("name", &war::Tank::getName, &war::Tank::setName)
        .property("nick", &Tank_nick, &war::Tank::setName)
        .property("badId", &war::Tank::badID, &war::Tank::updateBadId)
        .property("readOnlyId", &war::Tank::badID, nullptr)
        //.property("readOnlyId2", nullptr, nullptr) // should failed
        .property("writeOnlyId", nullptr, &war::Tank::updateBadId)
        .function("fire", static_cast<void (war::Tank::*)(std::string)>(&war::Tank::fire))
        .function("fire", static_cast<void (war::Tank::*)(float, float, float)>(&war::Tank::fire))
        .function("fire3", &war::Weapon::fire2)
        .property("seqId", &war::Weapon::getSeqId, &war::Tank::updateBadId)
        .function("load", &war::Tank::load)
        .function("tick", &war::Tank::tick)
        .function("tick2", &war::Tank::tick2)
        .gcCallback([](auto *w) {
            std::cout << " before gc tank " << w->getName() << std::endl;
        })
        .staticFunction("sleep", &Tank_Sleep)
        .staticFunction("sleep", &Tank_Sleep2)
        .staticProperty("rand", &Tank_random, &Tank_seed)
        .install(obj);

    return true;
}

bool register_all_war(se::Object *obj) {
    js_register_war_Tank(obj);
    return true;
}