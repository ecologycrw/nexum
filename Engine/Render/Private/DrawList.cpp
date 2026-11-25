#include "Nexum/Render/DrawList.hpp"

#include <string>

namespace nexum::render {

core::Result<void> DrawList::Add(Blit blit) {
    if (m_count == kMaxBlits) {
        return core::MakeError("the draw list is full (" + std::to_string(kMaxBlits) + " copies)");
    }
    m_blits[m_count] = blit;
    ++m_count;
    return {};
}

} // namespace nexum::render
