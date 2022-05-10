#pragma once

#include <string>

namespace war {

class Tank {
public:
    Tank(const std::string &name);
    Tank(int i, const std::string &name);
    void fire(float x, float y, float z);
    void fire(std::string);

    std::string getName() const { return name; }
    void setName(const std::string& name) {
        this->name = name;
    }

    int id;

private:
    std::string name;
};

} // namespace war