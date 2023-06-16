#include "FileStreamer.h"
#include <parfait/StringTools.h>
#include <parfait/FileTools.h>
#include <parfait/OmlParser.h>
#include <Tracer.h>

namespace inf {
class SimpleFileStreamer : public FileStreamer {
  public:
    bool openRead(std::string filename) override {
        fp = fopen(filename.c_str(), "r");
        return fp != nullptr;
    }
    bool openWrite(std::string filename) override {
        fp = fopen(filename.c_str(), "w");
        return fp != nullptr;
    }
    void write(const void* data, size_t item_size, size_t num_items) override {
        fwrite(data, item_size, num_items, fp);
    }
    void read(void* data, size_t item_size, size_t num_items) override {
        fread(data, item_size, num_items, fp);
    }
    void skip(size_t num_bytes) override { fseek(fp, num_bytes, SEEK_CUR); }
    void close() override { fclose(fp); }

  private:
    FILE* fp;
};

#if defined(ENABLE_POSIX_HINTS)
#include <fcntl.h>
#include <unistd.h>

class FadviseFileStreamer : public FileStreamer {
  public:
    FadviseFileStreamer() {
        if (Parfait::FileTools::doesFileExist("settings.inf")) {
            auto settings =
                Parfait::OmlParser::parse(Parfait::FileTools::loadFileToString("settings.inf"));
            if (settings.has("min-mem-threshold-mb")) {
                min_avail_threshold_mb = settings.at("min-mem-threshold-mb").asInt();
            }
            if (settings.isTrue("verbose")) {
                verbose = true;
            }
            printf("Loaded settings: \n%s\n", settings.dump(2).c_str());
        }
    }
    bool openRead(std::string filename) override {
        fp = fopen(filename.c_str(), "r");
        return fp != nullptr;
    }
    bool openWrite(std::string filename) override {
        fp = fopen(filename.c_str(), "w");
        return fp != nullptr;
    }
    void write(const void* data, size_t item_size, size_t num_items) override {
        size_t mem_avail_mb = getAvailMBEstimate();
        if (mem_avail_mb < min_avail_threshold_mb) {
            auto offset = ftell(fp);
            auto fp_int = fileno(fp);
            if (verbose) {
                printf("Detected avail mem %ldMB under threshold %ldMB\n",
                       mem_avail_mb,
                       min_avail_threshold_mb);
            }
            fdatasync(fp_int);
            posix_fadvise(fp_int, 0, offset, POSIX_FADV_DONTNEED);
        }
        fwrite(data, item_size, num_items, fp);
        assumeIJustUsedMemory(item_size * num_items / bytes_in_mb);
    }
    void read(void* data, size_t item_size, size_t num_items) override {
        auto offset = ftell(fp);
        auto fp_int = fileno(fp);
        fread(data, item_size, num_items, fp);
        assumeIJustUsedMemory(item_size * num_items / bytes_in_mb);
        size_t mem_avail_mb = getAvailMBEstimate();
        if (mem_avail_mb < min_avail_threshold_mb) {
            posix_fadvise(fp_int, offset, item_size * num_items, POSIX_FADV_DONTNEED);
        }
    }
    void skip(size_t num_bytes) override { fseek(fp, num_bytes, SEEK_CUR); }
    void close() override {
        size_t mem_avail_mb = getAvailMBEstimate();
        if (mem_avail_mb < min_avail_threshold_mb) {
            fseek(fp, 0, SEEK_END);
            auto length = ftell(fp);
            size_t offset = 0;
            auto fp_int = fileno(fp);
            fdatasync(fp_int);
            posix_fadvise(fp_int, offset, length, POSIX_FADV_DONTNEED);
        }
        fclose(fp);
    }

    size_t getAvailMBEstimate() {
        if (guess_of_remaining_mb < min_avail_threshold_mb) {
            guess_of_remaining_mb = 0.5 * Tracer::availableMemoryMB();
        }
        return guess_of_remaining_mb;
    }

    size_t assumeIJustUsedMemory(size_t mb_used) {
        if (guess_of_remaining_mb < mb_used) {
            guess_of_remaining_mb = 0;
        } else {
            guess_of_remaining_mb -= mb_used;
        }
        return guess_of_remaining_mb;
    }

  private:
    FILE* fp;
    size_t min_avail_threshold_mb = 1024;
    size_t guess_of_remaining_mb = 0;
    size_t bytes_in_mb = 1024 * 1024;
    bool verbose = false;
};

#endif

std::shared_ptr<FileStreamer> inf::FileStreamer::create(std::string implementation_name) {
    implementation_name = Parfait::StringTools::tolower(implementation_name);
#if defined(ENABLE_POSIX_HINTS)
    std::string default_option = "fadvise";
#else
    std::string default_option = "simple";
#endif
    bool verbose = false;
    if (Parfait::FileTools::doesFileExist("settings.inf")) {
        auto settings =
            Parfait::OmlParser::parse(Parfait::FileTools::loadFileToString("settings.inf"));
        if (settings.has("filestreamer-default")) {
            default_option = settings.at("filestreamer-default").asString();
        }
        if (settings.isTrue("verbose")) {
            verbose = true;
        }
        printf("Loaded settings: \n%s\n", settings.dump(2).c_str());
    }
    if (verbose) {
        printf("implementation selected: %s\n", implementation_name.c_str());
        printf("implementation  default: %s\n", default_option.c_str());
    }
    if (implementation_name == "default") {
        implementation_name = default_option;
    }
    if (implementation_name == "simple") {
        return std::make_shared<SimpleFileStreamer>();
    }
#if defined(ENABLE_POSIX_HINTS)
    if (implementation_name == "fadvise") {
        return std::make_shared<FadviseFileStreamer>();
    }
#endif
    PARFAIT_WARNING("Could not find file streamer of type " + implementation_name +
                    "\nFalling back to simple");
    return std::make_shared<SimpleFileStreamer>();
}

}
