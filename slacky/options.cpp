#include "options.h"
#include "convert.h"

#include <windows.h>
#include <shellapi.h>
#undef GetMessage

#include <span>
#include <filesystem>
#include <system_error>

namespace slacky
{
	// 環境変数を展開する：
	//
	// * ExpandEnvironmentStringsW 関数を使用して環境変数を展開する。
	// * 環境変数が定義されていない場合、展開は行われず元の文字列が返される。
	//
	std::wstring Expand(const wchar_t * str)
	{
		if (const auto size = ::ExpandEnvironmentStringsW(str, nullptr, 0); size > 1)
		{
			std::wstring buff(size - 1, L'\0');

			if (::ExpandEnvironmentStringsW(str, buff.data(), size) > 1)
			{
				return buff;
			}
		}

		throw std::system_error(::GetLastError(), std::system_category(), "ExpandEnvironmentStringsW()");
	}


	// 読み込み専用のファイルハンドルを保持する：
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
	// * 指定されたパスのファイル全体に対するビューを作成する。
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

	// テキストファイル全体をそのまま読み込む：
	//
	// * 指定されたパスのテキストファイルをすべて読み込み、その内容を文字列として返す。
	// * BOM の判別と文字コードの変換も行う。
	//
	std::wstring ReadAllText(const std::filesystem::path & path)
	{
		ViewOfFile file(path);
		const auto size = file.size();
		const auto data = file.data<wchar_t>();

		if (size > 3)
		{
			// UTF-16 LE BOM
			if (*data == 0xFFFE)
			{
				// BOM の分を除いた部分を複製して返す
				const auto wcs_size = (size - 2) / sizeof(wchar_t);
				const auto wcs_data = (data + 1);
				return std::wstring{ wcs_data, wcs_size };
			}

			// UTF-16 BE BOM
			if (*data == 0xFEFF)
			{
				// バイト順を入れ替えて詰めたものを返す
				const auto wcs_size = (size - 2) / sizeof(wchar_t);
				const auto wcs_data = (data + 1);

				std::wstring wcs;
				wcs.reserve(wcs_size);

				for (wchar_t ch : std::span{ wcs_data, wcs_size })
				{
					wcs.push_back(_byteswap_ushort(ch));
				}

				return wcs;
			}

			// UTF-8 BOM
			if (*data == 0xBBEF)
			{
				const auto data = file.data<char8_t>();

				if (data[2] == 0xBF)
				{
					// BOM の分を除いた部分を変換して返す
					const auto utf8_size = size - 3;
					const auto utf8_data = data + 3;
					return ConvertFrom(std::u8string_view{ utf8_data, utf8_size });
				}
			}
		}

		return Widen(std::string_view{ file.data<char>(), size });
	}


	//
	// 動作オプションを取得する：
	//

	Options::Options() : m_argc(0), m_argv(::CommandLineToArgvW(::GetCommandLineW(), &m_argc))
	{
		if (!m_argv)
		{
			throw std::system_error(::GetLastError(), std::system_category(), "CommandLineToArgvW()");
		}
	}

	Options::~Options() noexcept
	{
		::LocalFree(m_argv);
	}

	std::wstring Options::GetAppUserModelID() const
	{
		// 環境変数の展開ができなかった場合は例外をスローする

		auto aumid = Expand(L"%COMPUTERNAME%.slacky");

		if (aumid[0] == L'%')
		{
			throw std::runtime_error("Environment variable 'COMPUTERNAME' is not defined.");
		}

		return aumid;
	}

	std::wstring Options::GetExecutablePath() const
	{
		// * コマンドライン引数の最初の要素が実行可能ファイルのパス
		// -> CommandLineToArgvW 関数が成功していれば少なくとも１つ要素は存在するはず

		return std::filesystem::absolute(*m_argv);
	}

	std::wstring Options::GetToken() const
	{
		// コマンドライン引数だけでなく、環境変数 SLACKY_TOKEN からも取得できるようにする

		for (std::wstring_view argv : std::span(m_argv, static_cast<size_t>(m_argc)))
		{
			if (argv.starts_with(L"xoxb-"))
			{
				return std::wstring{ argv };
			}
		}

		auto token = Expand(L"%SLACKY_TOKEN%");

		if (token[0] == L'%')
		{
			throw std::runtime_error("Environment variable 'SLACKY_TOKEN' is not defined.");
		}

		return token;
	}

	std::wstring Options::GetChannel() const
	{
		// コマンドライン引数のうち Token 以外の最初の要素をチャネル ID とする

		for (std::wstring_view argv : std::span(m_argv, static_cast<size_t>(m_argc)))
		{
			if (argv.starts_with(L"xoxb-"))
			{
				continue;
			}

			if (argv.starts_with(L"C"))
			{
				return std::wstring{ argv };
			}

			break;
		}

		throw std::runtime_error("Channel ID is not specified.");
	}

	std::wstring Options::GetMessage() const
	{
		// コマンドライン引数のうち Token でも Channel でもない要素をメッセージとする
		//
		// * @ で始まる場合はメッセージを格納したテキストファイルの名前とみなし、その内容を読み込む
		// * 環境変数の展開も行う

		for (std::wstring_view argv : std::span(m_argv, static_cast<size_t>(m_argc)))
		{
			if (argv.starts_with(L"xoxb-"))
			{
				continue;
			}

			if (argv.starts_with(L"C"))
			{
				continue;
			}

			if (argv.starts_with(L"@"))
			{
				auto path = std::filesystem::absolute(argv.substr(1));
				auto text = ReadAllText(path);

				return Expand(text.c_str());
			}

			return Expand(argv.data());
		}

		throw std::runtime_error("Message is not specified.");
	}
}
