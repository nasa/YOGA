#pragma once
#include <vector>
#include <stdexcept>

namespace SliceAcrossSections {
struct Slice {
    int section;
    long start;
    long end;
};

inline Slice getRangeInSection(long section_start, long section_end, long start, long end) {
    if (section_start >= end or start >= section_end)
        return Slice{0, -1, -1};
    else if (section_start >= start and section_end >= end) {
        long slice_start = 0;
        long slice_end = end - section_start;
        return Slice{0, slice_start, slice_end};
    } else if (section_start >= start and section_end < end) {
        long slice_start = 0;
        long slice_end = section_end - section_start;
        return Slice{0, slice_start, slice_end};
    } else if (section_start < start and section_end < end) {
        long slice_start = start - section_start;
        long slice_end = section_end - section_start;
        return Slice{0, slice_start, slice_end};
    } else if (section_start < start and section_end >= end) {
        long slice_start = start - section_start;
        long slice_end = end - section_start;
        return Slice{0, slice_start, slice_end};
    } else {
        throw std::logic_error("whoops");
    }
}

inline std::vector<Slice> calcSlicesForRange(const std::vector<long>& count_per_section, long start, long end) {
    std::vector<Slice> slices;
    long section_offset = 0;
    for (size_t section = 0; section < count_per_section.size(); section++) {
        long section_start = section_offset;
        long section_end = section_offset + count_per_section[section];
        auto slice = getRangeInSection(section_start, section_end, start, end);
        if (slice.start != -1) {
            slice.section = section;
            slices.push_back(slice);
        }
        section_offset = section_end;
    }
    return slices;
}
}