#pragma once

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include "utils/StringTypes.h"

namespace Mana {

class File {
 public:
  File() {}
  ~File();

  bool Open(const xchar* fileName, const xchar* mode);
  // allows you to manage your own buffer
  size_t Read(void* buf, size_t size, size_t count);
  void Close();

  // calls Open/Close for you in rb mode
  // and synchronously calls Read until all bytes
  // are read into an internal buffer.
  size_t ReadAllBytes(const xchar* fileName);
  unsigned char* GetBuffer() { return m_pBuf; }

  static size_t GetFileSize(const xchar* fileName);

 private:
  xstring m_fileName;
  FILE* m_pFile = nullptr;
  unsigned char* m_pBuf = nullptr;
};

}  // namespace Mana
