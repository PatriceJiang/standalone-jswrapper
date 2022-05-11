#include "Tank.h"
#include <iostream>

namespace war {

Tank::Tank(const std::string &name):Weapon(0) {
    this->name = name;
}

Tank::Tank(int i, const std::string &name):Weapon(0) {
    id         = i;
    this->name = name;
}

void Tank::fire(float x, float y, float z) {
    std::cout << "Tank[" << name << "] aim (" << x << "," << y << "," << z << ") and fire!" << std::endl;
}

void Tank::fire(std::string targetName) {
    std::cout << "Tank fire at target " << targetName << std::endl;
}

} // namespace war