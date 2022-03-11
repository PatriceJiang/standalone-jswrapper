#pragma once

#include <string>

namespace war {

class Tank {
public:
    Tank(const std::string &name);
    void fire(float x, float y, float z);

private:
    std::string name;
};

} // namespace war