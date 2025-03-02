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
  File() = default;
  virtual ~File();

  File(const File&) = delete;
  File& operator=(const File&) = delete;

  bool Open(const xchar* fileName, const xchar* mode);
  // allows you to manage your own buffer
  size_t Read(void* buf, size_t size, size_t count);
  void Close();

  // calls Open/Close for you in rb mode
  // and synchronously calls Read until all bytes
  // are read into an internal buffer.
  size_t ReadAllBytes(const xchar* fileName);
  unsigned char* GetBuffer() { return pBuf_; }
  size_t GetFileSize() const { return fileSize_; };

  static size_t GetFileSize(const xchar* fileName);

 private:
  xstring fileName_;
  FILE* pFile_ = nullptr;
  unsigned char* pBuf_ = nullptr;
  size_t fileSize_ = 0;
};

}  // namespace Mana
