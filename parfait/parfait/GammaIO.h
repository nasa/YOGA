#pragma once
#include <stdio.h>
#include <map>

namespace Parfait {

class GammaSolIO {
  public:
    enum Keywords { Begin = 0, Dimensionality = 3, End = 54, NodalSolution = 62, ReferenceStrings = 999 };

  protected:
    FILE* f;

    enum Types { Scalar = 1 };
    class Record {
      public:
        long start = 0;
        Keywords key = Begin;
        long length = 0;

        long next() { return start + length; }

      protected:
        long headerSize() { return 12; }
    };

    class BeginRecord : public Record {
      public:
        BeginRecord() {
            start = 0;
            key = Begin;
            length = 8;
        }
    };

    class EndRecord : public Record {
      public:
        EndRecord(long pos) {
            start = pos;
            key = End;
            length = -pos;
        }
    };

    class DimensonalityRecord : public Record {
      public:
        DimensonalityRecord() {
            start = 8;
            key = Dimensionality;
            length = headerSize() + 4;
        }
        int dim = 3;
    };

    class ReferenceStringsRecord : public Record {
      public:
        ReferenceStringsRecord(long pos, const std::vector<std::string>& names) : names(names) {
            start = pos;
            key = ReferenceStrings;
            length = headerSize() + 4;
            for (auto name : names) length += 4 + name.size();
        }
        std::vector<std::string> strings() { return names; }

      private:
        std::vector<std::string> names;
    };

    class NodeFieldRecord : public Record {
      public:
        NodeFieldRecord(std::string name, long pos, int values_per_node, long nnodes)
            : name(name), values_per_node(values_per_node) {
            start = pos;
            length = metaDataSize() + values_per_node * nnodes * 8;
            key = NodalSolution;
        }
        long metaDataSize() { return headerSize() + 4 + 4 * values_per_node; }
        int values_per_node;
        std::string name;
    };

    struct NodeField {
        std::string name;
        long node_count;
        int entry_length;
    };

    std::vector<std::shared_ptr<Record>> records;
    std::vector<NodeField> node_fields;

    void seek(Keywords key) {
        long offset = 0;
        for (auto& r : records) {
            if (r->key == key) break;
            offset = r->next();
        }
        fseek(f, offset, SEEK_SET);
        printf("seek %li (key %i)\n", offset, key);
    }

    std::shared_ptr<NodeFieldRecord> getFieldRecord(const std::string& name) const {
        for (auto& r : records) {
            if (r->key == NodalSolution) {
                auto ptr = std::static_pointer_cast<NodeFieldRecord>(r);
                printf("looking for %s:  %s\n", name.c_str(), ptr->name.c_str());
                if (ptr->name == name) return ptr;
            }
        }
        throw std::logic_error("Could not find node field: " + name);
        return nullptr;
    }
};

class SolWriter : public GammaSolIO {
  public:
    SolWriter() {}

    void close() {
        writeEndRecord();
        fclose(f);
    }

    void registerNodeField(const std::string name, long node_count, int entry_length) {
        NodeField field;
        field.name = name;
        field.node_count = node_count;
        field.entry_length = entry_length;
        node_fields.push_back(field);
    }

    void writeNodeField(std::string name, long starting_global_id, const std::vector<double>& v) {
        auto field = getFieldRecord(name);
        // write header
        fseek(f, field->start, SEEK_SET);
        printf("here: %li, next record: %li\n", ftell(f), field->next());
        write(NodalSolution);
        write(field->next());
        write(field->values_per_node);
        // write payload
        for (int i = 0; i < field->values_per_node; i++) write(int(Scalar));

        fseek(f, ftell(f) + starting_global_id * 8 * field->values_per_node, SEEK_SET);
        fwrite(v.data(), sizeof(double), v.size(), f);
    }

    void open(const std::string& filename) {
        f = fopen(filename.c_str(), "w");
        records = buildRecordOffsets();
        writeFileHeader();
        writeDimensionalityRecord();
        writeReferenceStringsRecord();
    }

    std::vector<std::shared_ptr<Record>> buildRecordOffsets() const {
        std::vector<std::shared_ptr<Record>> v;
        v.push_back(std::make_shared<BeginRecord>());
        v.push_back(std::make_shared<DimensonalityRecord>());
        if (not node_fields.empty()) {
            std::vector<std::string> field_names;
            for (auto field : node_fields) field_names.push_back(field.name);
            long pos = v.back()->next();
            v.push_back(std::make_shared<ReferenceStringsRecord>(v.back()->next(), field_names));
            for (auto& field : node_fields) {
                pos = v.back()->next();
                v.push_back(std::make_shared<NodeFieldRecord>(field.name, pos, field.entry_length, field.node_count));
            }
        }

        v.push_back(std::make_shared<EndRecord>(v.back()->next()));

        for (auto r : v) {
            printf("Record: type: %i start %li length %li next %li\n", r->key, r->start, r->length, r->next());
        }

        return v;
    }

    void writeEndRecord() {
        seek(End);
        write(End);
        write(long(0));
    }

    void writeDimensionalityRecord() {
        seek(Dimensionality);
        long next_record_pos = ftell(f) + recordSize(Dimensionality);
        write(int(Dimensionality));
        write(next_record_pos);
        write(three_d);
        throwIfFilePtrInWrongPos(ftell(f), next_record_pos);
    }

    void writeReferenceStringsRecord() {
        seek(ReferenceStrings);
        long next_record_pos = ftell(f) + calcReferenceStringRecordSize();
        write(int(ReferenceStrings));
        write(next_record_pos);
        write(int(node_fields.size()));
        for (auto& field : node_fields) {
            write(int(field.name.size()));
            fwrite(field.name.c_str(), sizeof(char), field.name.size(), f);
        }
        throwIfFilePtrInWrongPos(ftell(f), next_record_pos);
    }

    long calcReferenceStringRecordSize() const {
        int header_size = 12;
        long n = header_size + 4;
        for (auto& field : node_fields) n += 4 + field.name.size();
        return n;
    }

    void writeFileHeader() {
        seek(Begin);
        write(endian_checker);
        int version = 4;
        write(version);
    }

  private:
    int endian_checker = 1;
    int three_d = 3;
    const std::string filename;

    void throwIfFilePtrInWrongPos(long a, long b) {
        if (a != b)
            throw std::logic_error("Record size mismatch, expected pos: " + std::to_string(a) +
                                   " actual: " + std::to_string(b));
    }

    long recordSize(NodeField field) const {
        int record_header = 12;
        return record_header + 4 + 4 * field.entry_length + 8 * field.node_count * field.entry_length;
    }

    long recordSize(Keywords key) const {
        int record_header = 12;
        switch (key) {
            case Begin:
                return 8;
            case Dimensionality:
                return record_header + 4;
            case End:
            case NodalSolution:
            case ReferenceStrings:
            default:
                throw std::logic_error("Invalid record type");
        }
        return 0;
    }

    template <typename T>
    void write(T n) {
        int status = fwrite(&n, sizeof(T), 1, f);
        if (status != 1) throw;
    }
};

class SolReader : GammaSolIO {
  public:
    SolReader(const std::string filename) {
        f = open(filename);

        records = buildRecordMap();
        printf("Setting field record names\n");
        setFieldRecordNames();
    }
    int version() { return version_number; }
    int dimensionality() {
        for (auto& r : records)
            if (r->key == Dimensionality) return std::static_pointer_cast<DimensonalityRecord>(r)->dim;
        throw std::logic_error("Could not find dim record");
        return 0;
    }
    bool has(Keywords key) {
        for (auto& r : records)
            if (r->key == key) return true;
        return false;
    }
    std::vector<std::string> nodeFieldNames() {
        std::vector<std::string> names;
        for (auto& r : records) {
            if (r->key == ReferenceStrings) {
                auto sr = std::static_pointer_cast<ReferenceStringsRecord>(r);
                return sr->strings();
            }
        }
        return names;
    }

    std::vector<double> readNodeField(const std::string name, long gid_start, long gid_end) {
        auto field = getFieldRecord(name);
        long pos = field->start;
        pos += field->metaDataSize();
        fseek(f, pos + gid_start * 8, SEEK_SET);
        std::vector<double> v((gid_end - gid_start + 1) * field->values_per_node);
        fread(v.data(), sizeof(double), v.size(), f);
        return v;
    }

    void close() { fclose(f); }

  private:
    int version_number;

    struct Header {
        Keywords key;
        long next_record_start;
    };

    Header readRecordHeader() {
        Header h;
        read(h.key);
        read(h.next_record_start);
        return h;
    }

    FILE* open(const std::string filename) {
        auto fptr = fopen(filename.c_str(), "r");
        if (NULL == fptr) throw std::logic_error("Failed to open " + filename);
        return fptr;
    }

    void checkEndianness() {
        int n;
        read(n);
        if (n != 1) throw std::logic_error("Only native byte order supported");
    }

    std::vector<std::shared_ptr<Record>> buildRecordMap() {
        std::vector<std::shared_ptr<Record>> v;
        checkEndianness();
        read(version_number);
        v.push_back(std::make_shared<BeginRecord>());
        while (true) {
            long here = ftell(f);
            auto h = readRecordHeader();
            fseek(f, here, SEEK_SET);
            v.push_back(readRecord(h.key));
            fseek(f, h.next_record_start, SEEK_SET);
            if (h.next_record_start == 0) break;
        }
        return v;
    }

    void setFieldRecordNames() {
        printf("There are %i field names\n", nodeFieldNames().size());
        for (auto name : nodeFieldNames()) {
            auto ptr = getFieldRecord("not set");
            ptr->name = name;
            printf("Setting field name: %s\n", ptr->name.c_str());
        }
    }

    std::shared_ptr<Record> readRecord(Keywords key) {
        auto here = ftell(f);
        switch (key) {
            case ReferenceStrings:
                printf("Reading reference strings\n");
                return readReferenceStrings();
            case End:
                printf("Reading end\n");
                return std::make_shared<EndRecord>(here);
            case NodalSolution:
                printf("Reading node field\n");
                return readNodeFieldRecord();
            case Dimensionality:
                printf("Reading dimensionality record\n");
                return std::make_shared<DimensonalityRecord>();
        }
        throw std::logic_error("Unrecognized Gamma keyword: " + std::to_string(key));
    }

    std::shared_ptr<Record> readReferenceStrings() {
        long here = ftell(f);
        readRecordHeader();
        std::vector<std::string> names;
        int nfields;
        read(nfields);
        for (int i = 0; i < nfields; i++) {
            int length;
            read(length);
            std::string name;
            name.resize(length);
            fread(&name[0], 1, length, f);
            names.push_back(name);
            printf("Reading reference string: %s\n", name.c_str());
        }
        return std::make_shared<ReferenceStringsRecord>(here, names);
    }

    std::shared_ptr<Record> readNodeFieldRecord() {
        auto here = ftell(f);
        auto h = readRecordHeader();
        int values_per_node;
        read(values_per_node);
        int type;
        for (int i = 0; i < values_per_node; i++) read(type);
        long nnodes = (h.next_record_start - ftell(f)) / values_per_node / 8;
        return std::make_shared<NodeFieldRecord>("not set", here, values_per_node, nnodes);
    }

    template <typename T>
    void read(T& t) {
        auto status = fread(&t, sizeof(T), 1, f);
        if (status != 1) throw;
    }
};

}
