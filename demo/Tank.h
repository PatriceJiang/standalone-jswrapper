#pragma once

#include <iostream>

#include <string>
namespace war {

class Weapon {
public:
    Weapon() = default;
    Weapon(int id) : __id(id) {}
    
    virtual ~Weapon() {
        std::cout << "finalize Weapon  __id " << __id << std::endl;
    }
    virtual void fire2(std::string s) {
        std::cout << "fire with weapon!!! @" << s << std::endl;
    }

    int __id;
};

class Tank : public Weapon {
public:
    Tank(const std::string &name);
    Tank(int i, const std::string &name);

    virtual ~Tank() {
        std::cout << "finalize tank " << name << ", id " << id << std::endl;
    }
    void fire(float x, float y, float z);
    void fire(std::string);

    std::string getName() const { return name; }
    void        setName(const std::string &name) {
        this->name = name;
    }

    void load(Weapon *w) {
        std::cout << "load weapon id __ " << w->__id << std::endl;
    }

    void tick() {}
    void tick2(int) {}

    int id{-1};

    size_t badID() { return id; }
    bool   updateBadId(int ) {
  /*      id = nid;
        c*/
        return true;
    }

private:
    std::string name;
};

} // namespace war