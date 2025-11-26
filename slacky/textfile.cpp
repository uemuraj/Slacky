#include "textfile.h"
#include "convert.h"

#include <windows.h>
#include <span>

namespace slacky
{
	// 読み込み専用のファイルハンドルを保持する：
	//
	// * コピーは禁止。
	//
	struct FileHandle
	{
		HANDLE hFile;

		FileHandle(const std::filesystem::path & path) :
			hFile(::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr))
		{
			if (hFile == INVALID_HANDLE_VALUE)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "CreateFileW()");
			}
		}

		~FileHandle() noexcept
		{
			::CloseHandle(hFile);
		}

		FileHandle(const FileHandle &) = delete;
		FileHandle & operator=(const FileHandle &) = delete;
	};

	// 読み込み専用のファイルマッピングを作成する：
	//
	// * FileHandle を継承、コピーは禁止。
	//
	struct FileMappingObject : FileHandle
	{
		HANDLE hFileMappingObject;

		FileMappingObject(const std::filesystem::path & path) :
			FileHandle(path), hFileMappingObject(::CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr))
		{
			if (!hFileMappingObject)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "CreateFileMappingW()");
			}
		}

		~FileMappingObject() noexcept
		{
			::CloseHandle(hFileMappingObject);
		}
	};

	// ファイル全体を読み込み、バイト列としてアクセスする：
	//
	// * 指定されたパスのファイル全体に対する読み込み専用のビューを作成する。
	//
	class ViewOfFile : FileMappingObject
	{
		void * m_data;

	public:
		ViewOfFile(const std::filesystem::path & path) :
			FileMappingObject(path), m_data(::MapViewOfFile(hFileMappingObject, FILE_MAP_READ, 0, 0, 0))
		{
			if (!m_data)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "MapViewOfFile()");
			}
		}

		~ViewOfFile() noexcept
		{
			::UnmapViewOfFile(m_data);
		}

		size_t size() const noexcept
		{
			return ::GetFileSize(hFile, nullptr);
		}

		template<typename T>
		decltype(auto) data() const noexcept
		{
			return reinterpret_cast<const T *>(m_data);
		}
	};

	TextFile::TextFile(const std::filesystem::path & path) : m_file(std::make_unique<ViewOfFile>(path))
	{}

	TextFile::~TextFile() noexcept = default;

	std::wstring TextFile::ToString() const
	{
		const auto size = m_file->size();
		const auto data = m_file->data<wchar_t>();

		if (size > 3)
		{
			// UTF-32 は LE/BE どちもサポートしないため例外とする
			if ((data[0] == 0xFEFF && data[1] == 0x0000) || (data[0] == 0x0000 && data[1] == 0xFFFE))
			{
				throw std::runtime_error("UTF-32 encoding is not supported.");
			}

			// UTF-16 LE BOM
			if (*data == 0xFEFF)
			{
				// BOM の分を除いた部分を複製して返す
				const auto wcs_size = (size - 2) / sizeof(wchar_t);
				const auto wcs_data = (data + 1);
				return std::wstring{ wcs_data, wcs_size };
			}

			// UTF-16 BE BOM
			if (*data == 0xFFFE)
			{
				// BOM の分を除いた部分を変換して返す
				const auto wcs_size = (size - 2) / sizeof(wchar_t);
				const auto wcs_data = (data + 1);
				return ConvertUTF16Endian({ wcs_data, wcs_size });
			}

			// UTF-8 BOM
			if (*data == 0xBBEF)
			{
				const auto data = m_file->data<char8_t>();

				if (data[2] == 0xBF)
				{
					// BOM の分を除いた部分をコード変換して返す
					const auto utf8_size = size - 3;
					const auto utf8_data = data + 3;
					return ConvertFrom({ utf8_data, utf8_size });
				}
			}
		}

		// BOM がない場合、全体をワイド文字列として変換して返す

		return Widen({ m_file->data<char>(), size });
	}
}
