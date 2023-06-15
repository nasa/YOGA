#include "SelectedField.h"

inf::SelectedField::SelectedField(const std::vector<int>& selection_ids_to_field_ids,
                                  std::shared_ptr<FieldInterface> field)
    : field(field), selectionId_to_fieldId(selection_ids_to_field_ids) {}
int inf::SelectedField::size() const { return selectionId_to_fieldId.size(); }

int inf::SelectedField::blockSize() const { return field->blockSize(); }
void inf::SelectedField::value(int entity_id, void* v) const {
    entity_id = selectionId_to_fieldId.at(entity_id);
    field->value(entity_id, v);
}
