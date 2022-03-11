#include "Tank.h"
#include <iostream>

namespace war {

Tank::Tank(const std::string &name) {
    this->name = name;
}

void Tank::fire(float x, float y, float z) {
    std::cout << "Tank[" << name << "] aim (" << x << "," << y << "," << z << ") and fire!" << std::endl;
}

} // namespace war