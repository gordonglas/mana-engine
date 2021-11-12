#include "pch.h"
#include "utils/File.h"

namespace Mana
{
	File::~File()
	{
		if (m_pBuf)
		{
			delete[] m_pBuf;
			m_pBuf = nullptr;
		}

		Close();
	}

	bool File::Open(const xchar* fileName, const xchar* mode)
	{
		// TODO: this seems to have issues reading from the file after file is not properly closed or something?
		_wfopen_s(&m_pFile, fileName, mode);
		if (!m_pFile)
		{
			return false;
		}

		m_fileName = fileName;
		return true;
	}

	size_t File::Read(void* buf, size_t size, size_t count)
	{
		if (!m_pFile)
		{
			return 0;
		}

		return fread(buf, size, count, m_pFile);
	}

	size_t File::ReadAllBytes(const xchar* fileName)
	{
		size_t fileSize = File::GetFileSize(fileName);
		if (fileSize == 0)
		{
			return 0;
		}

		m_pBuf = new unsigned char[fileSize];

		if (!Open(fileName, _X("rb")))
		{
			return 0;
		}

		size_t pos = 0;
		size_t bytesRead = 0;
		size_t bytesLeft = fileSize;
		size_t bytesAttempt = 0;
		while (true)
		{
			bytesAttempt = bytesLeft > 65535 ? 65535 : bytesLeft;
			bytesRead = Read(&m_pBuf[pos], 1, bytesAttempt);
			pos += bytesRead;
			bytesLeft -= bytesRead;
			if (bytesLeft == 0 ||
				(bytesRead < bytesAttempt && feof(m_pFile) == 0))
			{
				// end of file
				break;
			}
			else if (bytesLeft > 0 && bytesRead < bytesAttempt)
			{
				// error
				fileSize = 0;
				delete[] m_pBuf;
				m_pBuf = nullptr;
				break;
			}
		}

		Close();

		return fileSize;
	}

	void File::Close()
	{
		if (!m_pFile)
		{
			return;
		}

		fclose(m_pFile);
		m_pFile = nullptr;
	}

	// static
	size_t File::GetFileSize(const xchar* fileName)
	{
		struct _stat buf;

		int result = _wstat(fileName, &buf);
		if (result != 0)
		{
			return 0;
		}
			
		return (size_t)buf.st_size;
	}
}
