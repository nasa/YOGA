#pragma once

class SerialGmshExporter {
  public:
    static void writeFile(const std::string& filename, const inf::MeshInterface& mesh) {
        FILE* f = fopen(filename.c_str(), "w");
        fprintf(f, "$MeshFormat\n");
        fprintf(f, "2.2 1 8\n");
        int one = 1;
        fwrite(&one, sizeof(int), 1, f);
        fprintf(f, "\n");
        fprintf(f, "$EndMeshFormat\n");
        fprintf(f, "$Nodes\n");
        fprintf(f, "%i\n", mesh.nodeCount());
        printf("Writing %i nodes\n", mesh.nodeCount());
        for (int i = 0; i < mesh.nodeCount(); i++) {
            auto p = mesh.node(i);
            fwrite(&i, sizeof(int), 1, f);
            fwrite(p.data(), sizeof(double), 3, f);
        }
        fprintf(f, "\n");
        fprintf(f, "$EndNodes\n");
        fprintf(f, "$Elements\n");
        fprintf(f, "%i\n", mesh.cellCount());
        std::vector<int> cell;
        printf("Writing %i cells\n", mesh.cellCount());
        for (int i = 0; i < mesh.cellCount(); i++) {
            int gmsh_type = GmshReader::convertTinfType(mesh.cellType(i));
            int ncell_in_section = 1;
            int ntags = 2;
            fwrite(&gmsh_type, sizeof(int), 1, f);
            fwrite(&ncell_in_section, sizeof(int), 1, f);
            fwrite(&ntags, sizeof(int), 1, f);
            int n = mesh.cellLength(i);
            cell.resize(n);
            mesh.cell(i, cell.data());
            auto fortranIndexing = [](int x) { return ++x; };
            std::transform(cell.begin(), cell.end(), cell.begin(), fortranIndexing);
            fwrite(&one, sizeof(int), 1, f);
            int tag = mesh.cellTag(i);
            fwrite(&tag, sizeof(int), 1, f);
            fwrite(&tag, sizeof(int), 1, f);
            fwrite(cell.data(), sizeof(int), cell.size(), f);
        }
        fprintf(f, "\n");
        fprintf(f, "$EndElements\n");

        fclose(f);
    }
};
