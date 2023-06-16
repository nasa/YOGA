
// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space
// Administration. No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
//
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.
#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>
#include <parfait/SyncPattern.h>
#include <parfait/SyncField.h>

std::map<long, int> buildGlobalToLocal(const std::vector<long>& local_to_global) {
    std::map<long, int> global_to_local;
    for (size_t i = 0; i < local_to_global.size(); i++) {
        auto global = local_to_global[i];
        global_to_local[global] = i;
    }
    return global_to_local;
}

class ExampleField {
  public:
    double getEntry(long global_id) { return data[global_to_local.at(global_id)]; }

    void setEntry(long global_id, double d) { data[global_to_local.at(global_id)] = d; }

    std::map<long, int> global_to_local;
    std::vector<double> data;
};

TEST_CASE("Field Exists Tests") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;

    std::vector<long> global_ids;
    if (mp.Rank() == 0) {
        global_ids = std::vector<long>{0, 1, 2, 3, 4, 5, 6, 7};
    } else if (mp.Rank() == 1) {
        global_ids = std::vector<long>{4, 5, 6, 7, 0, 1, 2, 3};
    }
    auto sync_pattern = Parfait::SyncPattern(mp,
                                             std::vector<long>(global_ids.begin(), global_ids.begin() + 4),
                                             std::vector<long>(global_ids.begin() + 4, global_ids.end()));

    SECTION("Use example field") {
        ExampleField f;
        f.data.resize(8);
        for (int i = 0; i < 4; i++) {
            f.data[i] = 5;
        }
        for (int i = 4; i < 8; i++) f.data[i] = -99;
        f.global_to_local = buildGlobalToLocal(global_ids);

        Parfait::syncField<double>(mp, f, sync_pattern);

        REQUIRE(f.data[5] == 5);
    }

    SECTION("Use getter and setter") {
        std::vector<double> data;
        data.resize(8);
        for (int i = 0; i < 4; i++) {
            data[i] = 5;
        }
        for (int i = 4; i < 8; i++) data[i] = -99;
        auto global_to_local = buildGlobalToLocal(global_ids);

        auto getElementRef = [&](long id) -> double& { return data.at(global_to_local.at(id)); };
        auto packer = [](MessagePasser::Message& msg, double x) { msg.pack(x); };
        auto unpacker = [](MessagePasser::Message& msg, double& x) { msg.unpack(x); };

        auto syncer = Parfait::SyncerFactory::build(mp, getElementRef, packer, unpacker, sync_pattern);
        // Syncer<decltype(getElementRef),decltype(packer), decltype(unpacker)>
        // syncer(mp,getElementRef,packer,unpacker,sync_pattern);
        syncer.sync();
        REQUIRE(data[5] == 5);
    }
}

template <typename T>
class StridedVectorWrapper {
  public:
    StridedVectorWrapper(int block_size, std::vector<T>& q, const std::map<long, int>& global_to_local)
        : block_size(block_size), Q(q), g2l(global_to_local) {}
    T getEntry(long global, int i) const { return Q[block_size * g2l.at(global) + i]; }
    void setEntry(long global, int i, const T& value) { Q[block_size * g2l.at(global) + i] = value; }
    int stride() const { return block_size; }
    int block_size;
    std::vector<T>& Q;
    const std::map<long, int>& g2l;
};

TEST_CASE("Sync data with stride") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;

    std::vector<long> global_ids;
    int num_owned_entries = 4;
    if (mp.Rank() == 0) {
        global_ids = std::vector<long>{0, 1, 2, 3, 4, 5, 6, 7};
    } else if (mp.Rank() == 1) {
        global_ids = std::vector<long>{4, 5, 6, 7, 0, 1, 2, 3};
    }
    auto sync_pattern =
        Parfait::SyncPattern(mp,
                             std::vector<long>(global_ids.begin(), global_ids.begin() + num_owned_entries),
                             std::vector<long>(global_ids.begin() + num_owned_entries, global_ids.end()));

    long total_resident_entries = global_ids.size();
    int block_size = 3;
    std::vector<double> data(total_resident_entries * block_size, 0.0);
    for (int r = 0; r < num_owned_entries; r++) {
        data[r * block_size + 0] = double(global_ids[r]);
        data[r * block_size + 1] = double(10 * global_ids[r]);
        data[r * block_size + 2] = double(100 * global_ids[r]);
    }
    for (int r = num_owned_entries; r < total_resident_entries; r++) {
        REQUIRE(data[r * block_size + 0] == 0.0);
        REQUIRE(data[r * block_size + 1] == 0.0);
        REQUIRE(data[r * block_size + 2] == 0.0);
    }

    std::map<long, int> global_to_local;
    for (int local = 0; local < int(global_ids.size()); local++) {
        long global = global_ids[local];
        global_to_local[global] = local;
    }

    auto checkResult = [&]() {
        for (int r = num_owned_entries; r < total_resident_entries; r++) {
            REQUIRE(data[r * block_size + 0] == double(global_ids[r]));
            REQUIRE(data[r * block_size + 1] == double(10 * global_ids[r]));
            REQUIRE(data[r * block_size + 2] == double(100 * global_ids[r]));
        }
    };

    SECTION("Strided Vector") {
        Parfait::syncStridedVector(mp, data, global_to_local, sync_pattern, block_size);
        checkResult();
    }
    SECTION("Strided Pointer") {
        Parfait::syncStridedPointer(mp, data.data(), global_to_local, sync_pattern, block_size);
        checkResult();
    }
    SECTION("Arbitrary strided data 2") {
        auto wrapper = StridedVectorWrapper<double>(block_size, data, global_to_local);
        Parfait::syncStridedField<double>(mp, wrapper, sync_pattern);
        checkResult();
    }
}

class NonTrivialPokemon {
  public:
    struct Properties {
        bool resistant;
        float percentage_dmg_amplification;
        std::string sound;
    };
    enum Element { Fire, Water, Air, Earth, Avatar };

    NonTrivialPokemon(const std::string name) : name(name) {}

    std::map<Element, Properties> affinity;
    std::string name;
};

NonTrivialPokemon unknownGhostPokemon() {
    NonTrivialPokemon ghost("unknown");
    ghost.affinity = {};
    return ghost;
}

TEST_CASE("It's time to get down to business and sync pokemon") {
    MessagePasser mp(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() == 2) {
        std::vector<NonTrivialPokemon> my_pokemon;

        std::vector<long> have, need;
        std::vector<int> need_from;
        if (0 == mp.Rank()) {
            NonTrivialPokemon pikachu("pikachu");
            NonTrivialPokemon charmander("charmander");
            pikachu.affinity[NonTrivialPokemon::Element::Water] = {false, 3.5, "zzzzt"};
            charmander.affinity[NonTrivialPokemon::Element::Water] = {false, 4.2, "tssssss"};
            have = {99, 67};
            need = {23};
            need_from = {1};
            my_pokemon = {pikachu, charmander, unknownGhostPokemon()};
        } else if (1 == mp.Rank()) {
            NonTrivialPokemon squirtle("squirtle");
            squirtle.affinity[NonTrivialPokemon::Element::Water] = {true, 0.2, "bloop"};
            have = {23};
            need = {99, 67};
            need_from = {0, 0};
            my_pokemon = {unknownGhostPokemon(), unknownGhostPokemon(), squirtle};
        }

        auto sync_pattern = Parfait::SyncPattern(mp, have, need, need_from);

        std::vector<long> global_pokemon_ids{99, 67, 23};
        std::map<long, int> global_to_local;
        for (size_t i = 0; i < global_pokemon_ids.size(); i++) {
            global_to_local[global_pokemon_ids[i]] = i;
        }

        auto packer = [](MessagePasser::Message& msg, const NonTrivialPokemon& pokemon) {
            msg.pack(pokemon.name);
            msg.pack(int(pokemon.affinity.size()));
            for (auto& elem : pokemon.affinity) {
                msg.pack(elem.first);
                msg.pack(elem.second.percentage_dmg_amplification);
                msg.pack(elem.second.resistant);
                msg.pack(elem.second.sound);
            }
        };
        auto unpacker = [](MessagePasser::Message& msg, NonTrivialPokemon& pokemon) {
            msg.unpack(pokemon.name);
            int n = -1;
            msg.unpack(n);
            for (int i = 0; i < n; i++) {
                NonTrivialPokemon::Properties properties;
                NonTrivialPokemon::Element element = NonTrivialPokemon::Earth;  // initialize to something...
                msg.unpack(element);
                msg.unpack(properties.percentage_dmg_amplification);
                msg.unpack(properties.resistant);
                msg.unpack(properties.sound);
                pokemon.affinity[element] = properties;
            }
        };

        auto getElementRef = [&](long id) -> NonTrivialPokemon& { return my_pokemon.at(global_to_local.at(id)); };

        auto syncer = Parfait::SyncerFactory::build(mp, getElementRef, packer, unpacker, sync_pattern);
        syncer.sync();

        auto& pokemon = my_pokemon[0];
        REQUIRE("pikachu" == pokemon.name);
        REQUIRE(1 == pokemon.affinity.count(NonTrivialPokemon::Water));
        auto& affinity = pokemon.affinity[NonTrivialPokemon::Water];
        REQUIRE(false == affinity.resistant);
        REQUIRE(3.5 == Approx(affinity.percentage_dmg_amplification));
        REQUIRE("zzzzt" == affinity.sound);

        pokemon = my_pokemon[1];
        REQUIRE("charmander" == pokemon.name);
        REQUIRE(1 == pokemon.affinity.count(NonTrivialPokemon::Water));
        affinity = pokemon.affinity[NonTrivialPokemon::Water];
        REQUIRE(false == affinity.resistant);
        REQUIRE(4.2 == Approx(affinity.percentage_dmg_amplification));
        REQUIRE("tssssss" == affinity.sound);

        pokemon = my_pokemon[2];
        REQUIRE("squirtle" == pokemon.name);
        REQUIRE(1 == pokemon.affinity.count(NonTrivialPokemon::Water));
        affinity = pokemon.affinity[NonTrivialPokemon::Water];
        REQUIRE(true == affinity.resistant);
        REQUIRE(0.2 == Approx(affinity.percentage_dmg_amplification));
        REQUIRE("bloop" == affinity.sound);
    }
}
