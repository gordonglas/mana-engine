#include "pch.h"
#include "utils/Memory.h"
#include <assert.h>

namespace Mana {

bool AlignedMalloc(size_t align, size_t size,
    void** ppAligned, void** ppRaw) {
  uintptr_t mask = ~(uintptr_t)(align - 1);
  void* mem = malloc(size + align - 1);
  if (!mem)
    return false;
  void* ptr = (void*)(((uintptr_t)mem + align - 1) & mask);
  assert((align & (align - 1)) == 0);
  // printf("0x%08" PRIXPTR ", 0x%08" PRIXPTR "\n", (uintptr_t)mem,
  // (uintptr_t)ptr);
  ppRaw = &mem;
  ppAligned = &ptr;
  return true;
}

} // namespace Mana
