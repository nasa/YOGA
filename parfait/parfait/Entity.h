#pragma once

#include <map>
#include <memory>
#include <parfait/Throw.h>

namespace Parfait {

class Entity {
  public:
    struct Name {
        std::string m_name;
        explicit Name(std::string n) : m_name(n) {}
        operator std::string() const { return m_name; }
    };
    template <typename T>
    decltype(auto) add(const T& t) {
        size_t key = typeid(T).hash_code();
        if (anything.count(key) == 1) {
            std::string m =
                "Entity " + name() + " already has component of type <" + std::string(typeid(T).name()) + ">";
            PARFAIT_THROW(m);
        }
        std::shared_ptr<void> p = std::make_shared<T>(t);
        anything.insert({key, std::move(p)});
        return *this;
    }
    template <typename T, typename... Args>
    decltype(auto) add(Args&&... args) {
        size_t key = typeid(T).hash_code();
        if (anything.count(key) == 1) {
            std::string m =
                "Entity " + name() + " already has component of type <" + std::string(typeid(T).name()) + ">";
            PARFAIT_THROW(m);
        }
        std::shared_ptr<void> p = std::make_shared<T>(std::forward<Args>(args)...);
        anything.insert({key, std::move(p)});
        return *this;
    }
    Entity(std::string n = "Unnamed Entity") { add<Name>(n); }
    template <typename T, typename... Args>
    decltype(auto) addGet(Args&&... args) {
        add<T>(std::forward<Args>(args)...);
        return get<T>();
    }
    template <typename T>
    bool has() {
        size_t key = typeid(T).hash_code();
        return anything.count(key) == 1;
    }
    template <typename T>
    T& get() {
        size_t key = typeid(T).hash_code();
        if (anything.count(key) == 0) {
            std::string m =
                "Entity " + name() + " does not have component of type <" + std::string(typeid(T).name()) + ">";
            PARFAIT_THROW(m);
        }
        std::shared_ptr<void> p = anything.at(key);
        return *(T*)p.get();
    }
    template <typename T>
    const T& get() const {
        size_t key = typeid(T).hash_code();
        if (anything.count(key) == 0) {
            std::string m =
                "Entity " + name() + " does not have component of type <" + std::string(typeid(T).name()) + ">";
            PARFAIT_THROW(m);
        }
        std::shared_ptr<void> p = anything.at(key);
        return *(T*)p.get();
    }
    std::string name() const { return get<Name>(); }

    template <typename T>
    void remove() {
        size_t key = typeid(T).hash_code();
        if (has<T>()) anything.erase(key);
    }

  private:
    std::map<size_t, std::shared_ptr<void>> anything;
};
}
