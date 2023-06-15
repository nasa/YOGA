
#include "MeasureLoadBalance.h"
#include "MeshConnectivity.h"
#include <parfait/ToString.h>

namespace inf {
namespace LoadBalance {
    int countOwnedVolumeCells(const MeshInterface& mesh) {
        int volume_cell_count = 0;
        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.ownedCell(c)) {
                if (mesh.is3DCellType(mesh.cellType(c))) {
                    volume_cell_count++;
                }
            }
        }

        return volume_cell_count;
    }

    int countCutFaces(const MeshInterface& mesh) {
        auto c2c = CellToCell::build(mesh);

        int cut_face_count = 0;

        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.ownedCell(c)) {
                if (mesh.is3DCellType(mesh.cellType(c))) {
                    for (auto neighbor : c2c[c]) {
                        if (mesh.partitionId() != mesh.cellOwner(neighbor)) cut_face_count++;
                    }
                }
            }
        }
        return cut_face_count;
    }

    void printStatistics(MessagePasser mp, std::string name, double local_count) {
        auto maximum = mp.ParallelMax(local_count);
        auto minimum = mp.ParallelMin(local_count);
        auto total = mp.ParallelSum(local_count);
        auto average = total / double(mp.NumberOfProcesses());
        double load_percent = 100 * average / maximum;

        mp_rootprint("Load Balance Statistics:\n");
        mp_rootprint("Statistics for: %s\n", name.c_str());
        mp_rootprint("Total:           %1.2e\n", total);
        mp_rootprint("Maximum:         %1.2e\n", maximum);
        mp_rootprint("Minimum:         %1.2e\n", minimum);
        mp_rootprint("Average:         %1.2e\n", average);
        mp_rootprint("Load balance     %2.1lf\n", load_percent);
    }

    void printCommunicationStatistics(MessagePasser mp, const MeshInterface& mesh) {
        auto cut_face_count = countCutFaces(mesh);
        printStatistics(mp, "Cut Faces", double(cut_face_count));
    }

    void printUnweightedCellStatistics(MessagePasser mp, const MeshInterface& mesh) {
        auto volume_cell_count = countOwnedVolumeCells(mesh);
        printStatistics(mp, "Volume Cells", double(volume_cell_count));
    }

    void printLoadBalaceStatistics(MessagePasser mp, const MeshInterface& mesh) {
        printUnweightedCellStatistics(mp, mesh);
        printCommunicationStatistics(mp, mesh);
    }
}
}
