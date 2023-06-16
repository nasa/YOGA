#pragma once
#include <t-infinity/FieldInterface.h>
#include <memory>
#include <string>
#include <vector>

namespace inf {
class SelectedField : public FieldInterface {
  public:
    SelectedField(const std::vector<int>& selection_to_field_ids,
                  std::shared_ptr<FieldInterface> field);

    virtual int size() const override;
    virtual int blockSize() const override;
    virtual void value(int entity_id, void* v) const override;

  private:
    std::shared_ptr<FieldInterface> field;
    std::vector<int> selectionId_to_fieldId;
};

}
