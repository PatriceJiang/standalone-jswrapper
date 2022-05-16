#pragma once

#include <iostream>
#include <vector>
#include <string>

namespace se {
class Object;
}

namespace war {

class Weapon {
public:
    Weapon() = default;
    Weapon(int id) : seqId(id) {}
    
    virtual ~Weapon() {
        std::cout << "finalize Weapon  seqId " << seqId << std::endl;
    }
    virtual void fire2(std::string s) {
        std::cout << "Weapon::["<< seqId <<"] fire with weapon!!! @" << s << std::endl;
    }

    std::vector<int> numbers() {
        return {1, 2, 3, 4};
    }

    int getSeqId() { return seqId; }
    void setSeqId(int r) { seqId = r; }

    int seqId;
};

class Tank : public Weapon {
public:
    Tank(const std::string &name);
    Tank(int i, const std::string &name);
    Tank(se::Object *jsthis) {
    }

    virtual ~Tank() {
        std::cout << "finalize tank " << name << ", id " << seqId << std::endl;
    }
    void fire(float x, float y, float z);
    void fire(std::string);

    std::string getName() const { return name; }
    void        setName(const std::string &name) {
        this->name = name;
    }

    void load(Weapon *w) {
        std::cout << "load weapon seqId " << w->seqId << std::endl;
    }

    void tick() {}
    void tick2(int) {}

    size_t badID() { return seqId; }
    bool   updateBadId(int ) {
  /*      id = nid;
        c*/
        return true;
    }

private:
    std::string name;
};

} // namespace war