#include "pch.h"
#include "utils/File.h"

namespace Mana {

File::~File() {
  if (pBuf_) {
    delete[] pBuf_;
    pBuf_ = nullptr;
  }

  Close();
}

bool File::Open(const xchar* fileName, const xchar* mode) {
  // TODO: this seems to have issues reading from the file after file is not
  // properly closed or something?
  _wfopen_s(&pFile_, fileName, mode);
  if (!pFile_) {
    return false;
  }

  fileName_ = fileName;
  return true;
}

size_t File::Read(void* buf, size_t size, size_t count) {
  if (!pFile_) {
    return 0;
  }

  return fread(buf, size, count, pFile_);
}

size_t File::ReadAllBytes(const xchar* fileName) {
  size_t fileSize = File::GetFileSize(fileName);
  if (fileSize == 0) {
    return 0;
  }

  pBuf_ = new unsigned char[fileSize];
  if (!pBuf_) {
    return 0;
  }

  if (!Open(fileName, _X("rb"))) {
    return 0;
  }

  size_t pos = 0;
  size_t bytesRead = 0;
  size_t bytesLeft = fileSize;
  size_t bytesAttempt = 0;
  while (true) {
    bytesAttempt = bytesLeft > 65535 ? 65535 : bytesLeft;
    bytesRead = Read(&pBuf_[pos], 1, bytesAttempt);
    pos += bytesRead;
    bytesLeft -= bytesRead;
    if (bytesLeft == 0 || (bytesRead < bytesAttempt && feof(pFile_) == 0)) {
      // end of file
      break;
    } else if (bytesLeft > 0 && bytesRead < bytesAttempt) {
      // error
      fileSize = 0;
      delete[] pBuf_;
      pBuf_ = nullptr;
      break;
    }
  }

  Close();

  fileSize_ = fileSize;
  return fileSize;
}

void File::Close() {
  if (!pFile_) {
    return;
  }

  fclose(pFile_);
  pFile_ = nullptr;
}

// static
size_t File::GetFileSize(const xchar* fileName) {
  struct _stat buf;

  int result = _wstat(fileName, &buf);
  if (result != 0) {
    return 0;
  }

  return static_cast<size_t>(buf.st_size);
}

}  // namespace Mana
