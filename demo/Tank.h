#pragma once

#include <iostream>

#include <string>
namespace war {

class Weapon {
public:
    Weapon(int) {}
    virtual void fire2(std::string s) {
        std::cout << "fire with weapon!!! @" << s << std::endl;
    }
};

class Tank : public Weapon {
public:
    Tank(const std::string &name);
    Tank(int i, const std::string &name);
    void fire(float x, float y, float z);
    void fire(std::string);

    std::string getName() const { return name; }
    void        setName(const std::string &name) {
        this->name = name;
    }

    int id{-1};

    size_t badID() { return id; }
    bool   updateBadId(uint8_t nid) {
        id = nid;
        return true;
    }

private:
    std::string name;
};

} // namespace war