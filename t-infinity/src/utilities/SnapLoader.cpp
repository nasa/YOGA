#include <t-infinity/FieldLoader.h>
#include <t-infinity/Snap.h>

namespace inf  {
class SnapLoader : public FieldLoader {
  public:
    void load(const std::string& filename, MPI_Comm comm, const inf::MeshInterface& mesh) override {
        snap = std::make_shared<Snap>(comm);
        snap->addMeshTopologies(mesh);
        snap->load(filename);
    }
    std::vector<std::string> availableFields() const override {
        throwInNotInitialized();
        return snap->availableFields();
    }
    std::shared_ptr<FieldInterface> retrieve(const std::string& field_name) const override {
        throwInNotInitialized();
        return snap->retrieve(field_name);
    }
    ~SnapLoader() override = default;
    std::shared_ptr<Snap> snap;

  private:
    void throwInNotInitialized() const {
        if (not snap) PARFAIT_THROW("No fields loaded.  Did you forget to call FieldLoader::load?");
    }
};
}
extern "C" {
std::shared_ptr<inf::FieldLoader> createFieldLoader() { return std::make_shared<inf::SnapLoader>(); }
}
