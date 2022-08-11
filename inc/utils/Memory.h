#pragma once

#include "ManaGlobals.h"

namespace Mana {

// From:
// https://stackoverflow.com/questions/227897/how-to-allocate-aligned-memory-only-using-the-standard-library
// Note that we could probably use C11's "aligned_alloc" function instead,
// but this was a nice learning exersize.
// |align| must be a power of 2.
bool AlignedMalloc(size_t align, size_t size,
    void** ppAligned, void** ppRaw);

} // namespace Mana
