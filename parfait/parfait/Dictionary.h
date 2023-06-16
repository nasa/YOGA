#pragma once
#include <map>
#include <memory>
#include "DictionaryEntry.h"

namespace Parfait {
class ValueEntry : public DictionaryEntry {
  public:
    ValueEntry(const std::string v, TYPE v_type);
    TYPE type() const;
    std::string asString() const;
    int asInt() const;
    bool asBool() const;
    double asDouble() const;

  private:
    TYPE t;
    std::string value;
    void throwIfString() const;
};

class NullEntry : public DictionaryEntry {
  public:
    TYPE type() const;
};

class Dictionary : public DictionaryEntry {
  public:
    static ValueEntry null();
    Dictionary() : object_type(Object), object(std::make_shared<NullEntry>()) {}
    ~Dictionary() = default;
    Dictionary& operator=(const Dictionary& d);
    Dictionary& operator=(const std::string& s);
    Dictionary& operator=(const char* s);
    Dictionary& operator=(int n);
    Dictionary& operator=(double x);
    Dictionary& operator=(std::vector<int> v);
    Dictionary& operator=(const bool& b);
    Dictionary& operator=(std::vector<bool> v);
    Dictionary& operator=(std::vector<double> v);
    Dictionary& operator=(std::vector<std::string> v);
    Dictionary& operator=(const ValueEntry&& obj);
    bool isTrue(Key key) const;
    int size() const;
    int count(Key key) const;
    bool has(Key key) const { return mapped_objects.count(key) == 1; };
    void erase(Key key);
    TYPE type() const;
    const Dictionary& operator[](Key key) const;
    const Dictionary& operator[](int index) const;
    Dictionary& operator[](Key key);
    Dictionary& operator[](int index);
    const Dictionary& at(Key key) const;
    const Dictionary& at(int index) const;
    std::vector<std::string> keys() const;
    std::string asString() const;
    int asInt() const;
    double asDouble() const;
    bool asBool() const;
    std::vector<int> asInts() const;
    std::vector<double> asDoubles() const;
    std::vector<std::string> asStrings() const;
    std::vector<bool> asBools() const;
    std::vector<Dictionary> asObjects() const;

    bool isValueObject() const;
    bool isMappedObject() const;
    bool isArrayObject() const;

    std::string dump() const;

    std::string dump(int indent) const;
    friend std::ostream& operator<<(std::ostream& s, const Dictionary& d) {
        s << d.dump();
        return s;
    }

    Dictionary overrideEntries(const Dictionary& config) const;

    void clear();

  private:
    TYPE object_type;
    std::shared_ptr<DictionaryEntry> object;
    std::map<std::string, Dictionary> mapped_objects;
    std::map<int, Dictionary> indexed_objects;
};

}
