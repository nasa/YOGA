#include <RingAssertions.h>
#include <vector>
#include <map>
#include <memory>
#include <parfait/ToString.h>

class Animal {
  public:
    Animal() { printf("Base Constructor\n"); }
    virtual ~Animal() { printf("Base destructor\n"); }
};

class Dog : public Animal {
  public:
    Dog() { printf("Dog Constructor\n"); }
    virtual ~Dog() override { printf("Dog Destructor\n"); }
};

class Entity {
  public:
    template <typename T, typename... Args>
    decltype(auto) add(std::string key_name, Args&&... args) {
        size_t key = typeid(T).hash_code();
        std::shared_ptr<void> p = std::make_shared<T>(std::forward<Args>(args)...);
        anything.insert({key_name + std::to_string(key), std::move(p)});
        return *this;
    }
    template <typename T, typename... Args>
    decltype(auto) addGet(std::string key_name, Args&&... args) {
        size_t key = typeid(T).hash_code();
        std::shared_ptr<void> p = std::make_shared<T>(std::forward<Args>(args)...);
        anything.insert({key_name + std::to_string(key), std::move(p)});
        return get<T>();
    }
    template <typename T>
    T& get(std::string key_name) {
        size_t key = typeid(T).hash_code();
        std::shared_ptr<void> p = anything.at(key_name + std::to_string(key));
        return *(T*)p.get();
    }
    template <typename T>
    bool has(std::string key_name) {
        size_t key = typeid(T).hash_code();
        return anything.count(key_name + std::to_string(key)) == 1;
    }

  private:
    std::map<std::string, std::shared_ptr<void>> anything;
};

TEST_CASE("Any array") {
    Entity entity;

    entity.add<int>("the answer", 42);
    entity.add<std::vector<std::string>>("purple", std::vector<std::string>{"pika", "chuuuu", "this is for Chip"});

    int& my_int = entity.get<int>("the answer");
    printf("My int %d\n", my_int);
    auto& strings = entity.get<std::vector<std::string>>("purple");
    printf("Strings: %s\n", Parfait::to_string(strings, "\n").c_str());
}
