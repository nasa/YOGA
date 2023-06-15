#include <t-infinity/InternalFaceRemover.h>
#include <SerialMeshImporter.h>
#include <GmshReader.h>
#include <SerialGmshExporter.h>

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Need input and output grid filenames\n");
        return 0;
    }
    std::string input_filename = argv[1];
    std::string output_filename = argv[2];

    MessagePasser::Init();
    MessagePasser mp(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 1) {
        printf("This is a serial tool, exiting\n");
        return 0;
    }

    printf("Importing %s (serial)\n", input_filename.c_str());
    GmshReader reader(input_filename);
    auto original_mesh = SerialMeshImporter::import(reader);
    printf("Checking for internal faces\n");
    auto internal_faces = inf::InternalFaceRemover::identifyInternalFaces(*original_mesh);
    printf("Removing %li internal faces\n", internal_faces.size());
    auto output_mesh = inf::InternalFaceRemover::removeCells(mp.getCommunicator(), original_mesh, internal_faces);
    printf("Writing file: %s\n", output_filename.c_str());
    SerialGmshExporter::writeFile(output_filename, *output_mesh);
    printf("Done!\n");
    return 0;
}
