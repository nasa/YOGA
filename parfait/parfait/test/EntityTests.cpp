#include <RingAssertions.h>
#include <parfait/Entity.h>

class Dictionary {
  public:
    struct TypeErasedObject {
        size_t type_hash_code;
        std::shared_ptr<void> v;
    };
    typedef std::shared_ptr<std::map<std::string, TypeErasedObject>> Database;
    Dictionary() { anything = std::make_shared<std::map<std::string, TypeErasedObject>>(); }
    Dictionary(const Dictionary&) = delete;
    template <typename T>
    decltype(auto) add(std::string key, const T& t) {
        if (anything->count(key) == 1) {
            PARFAIT_THROW("Trying to add key that already exists: " + key);
        }
        std::shared_ptr<void> p = std::make_shared<T>(t);
        anything->insert({key, {typeid(T).hash_code(), std::move(p)}});
        return *this;
    }

    template <typename T>
    T& get(std::string key) {
        if (anything->count(key) == 0) {
            PARFAIT_THROW("wrong");
        }
        auto p = anything->at(key);
        if (typeid(T).hash_code() != p.type_hash_code) {
            PARFAIT_THROW("Error casting to type mismatch");
        }
        return *(T*)p.v.get();
    }

    struct InternalReturnType {
        Database parent;
        std::string key;
        size_t type_hash_code;
        std::shared_ptr<void> v;

        template <typename T>
        InternalReturnType& operator=(const T& t) {
            if (v == nullptr) {
                (*parent)[key] = {typeid(T).hash_code(), std::make_shared<T>(t)};
                v = parent->at(key).v;
                type_hash_code = typeid(T).hash_code();
                return *this;
            } else {
                // store the hash of T in the database and check it here.
                *((T*)v.get()) = t;
                return *this;
            }
        }

        template <typename T>
        bool operator==(const T& t) const {
            if (typeid(T).hash_code() != type_hash_code) {
                PARFAIT_THROW("Error casting to type mismatch");
            }
            return *(T*)v.get() == t;
        }

        template <typename T>
        operator T&() {
            if (typeid(T).hash_code() != type_hash_code) {
                PARFAIT_THROW("Error casting to type mismatch");
            }
            return *(T*)v.get();
        }

        template <typename T>
        operator T() const {
            if (typeid(T).hash_code() != type_hash_code) {
                PARFAIT_THROW("Error casting to type mismatch");
            }
            return *(T*)v.get();
        }
    };

    InternalReturnType operator[](std::string key) {
        if (anything->count(key) == 0) {
            size_t unset_hash_code = 0;
            return InternalReturnType{anything, key, unset_hash_code, nullptr};
        }
        auto p = (*anything)[key];
        return InternalReturnType{anything, key, p.type_hash_code, p.v};
    }

    InternalReturnType at(std::string key) {
        if (anything->count(key) == 0) {
            PARFAIT_THROW("No key found " + key);
        }
        auto p = anything->at(key);
        return InternalReturnType{anything, key, p.type_hash_code, p.v};
    }

  private:
    Database anything;
};

void my_func(int& i) { i = 4; }

void my_func(double& i) { i = 88.77; }

TEST_CASE("can't assign int from string") {
    Dictionary d;
    d["my string"] = std::string("dog");
    auto call = [&]() {
        int& i = d["my string"];
        (void)i;
    };  // just wrapping in a lambda to pass to REQUIRE_THROWS
    REQUIRE_THROWS(call());
}

TEST_CASE("Copying dictionary is not allowed") {
    Dictionary d1;
    d1["my int"] = 1;
    // Dictionary d2 = d1; // This fails to compile
}

TEST_CASE("Dictionary version ") {
    Dictionary d;
    d.add("my int", 1);
    REQUIRE(d["my int"] == 1);
    d["my int"] = 2;
    d["my int"] = 39;
    REQUIRE(39 == static_cast<int&>(d["my int"]));

    int& i = d["my int"];
    i = 3;
    REQUIRE(3 == i);
    REQUIRE(3 == static_cast<int&>(d["my int"]));
    my_func(static_cast<int&>(d["my int"]));
    REQUIRE(4 == static_cast<int&>(d["my int"]));

    d["my double"] = 138.9;
    auto& my_double = d.get<double>("my double");
    REQUIRE(138.9 == my_double);
    REQUIRE(138.9 == static_cast<double&>(d["my double"]));
}

TEST_CASE("Can make a bag of Components ") {
    Parfait::Entity e;
    e.add(long(42));
    REQUIRE(e.get<long>() == 42);

    auto& s = e.addGet<std::string>("I can't get no");
    s = "satisfaction";
    REQUIRE("satisfaction" == e.get<std::string>());

    e.remove<int>();
    REQUIRE_FALSE(e.has<int>());

    REQUIRE_THROWS(e.add<std::string>("Can't add type if it already has one"));

    // can't get type if it doesn't have one
    // also notice floats and doubles are explicitly different types
    REQUIRE_THROWS(e.get<double>());
}