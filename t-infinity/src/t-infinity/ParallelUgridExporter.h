#pragma once
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/MeshInterface.h>
#include <parfait/UgridWriter.h>
#include <parfait/StringTools.h>

namespace inf {
class ParallelUgridExporter {
  public:
    static void write(std::string name,
                      const MeshInterface& mesh,
                      const MessagePasser& mp,
                      bool write_mapbc_file = true);

  private:
    static long globalCellCount(MeshInterface::CellType type,
                                const MeshInterface& mesh,
                                const MessagePasser& mp);

    static void streamCellsToFile(std::string filename,
                                  MeshInterface::CellType type,
                                  const MeshInterface& mesh,
                                  bool swap_bytes,
                                  const MessagePasser& mp);

    static void streamCellTagsToFile(std::string filename,
                                     MeshInterface::CellType type,
                                     const MeshInterface& mesh,
                                     bool swap_bytes,
                                     const MessagePasser& mp);

    static std::vector<long> extractGlobalNodeIds(int rank,
                                                  const MeshInterface& mesh,
                                                  long start,
                                                  long end);

    static std::vector<Parfait::Point<double>> extractPoints(int rank,
                                                             const MeshInterface& mesh,
                                                             long start,
                                                             long end);

    static std::vector<Parfait::Point<double>> shuttleNodes(
        const MessagePasser& mp, int target_rank, const MeshInterface& mesh, long start, long end);

  protected:
    static bool hasEndiannessInFilename(const std::string& name) {
        return hasBigEndianExtension(name) or hasLittleEndianExtension(name);
    }

    static bool hasBigEndianExtension(const std::string& s) {
        return s.find(".b8.ugrid") != s.npos;
    }

    static bool hasLittleEndianExtension(const std::string& s) {
        return s.find(".lb8.ugrid") != s.npos;
    }

    static std::string setUgridExtension(const std::string s) {
        if (hasEndiannessInFilename(s)) return s;
        auto root = Parfait::StringTools::stripExtension(s, "ugrid");
        return root + ".lb8.ugrid";
    }
};

}
