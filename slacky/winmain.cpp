#include <windows.h>
#include <shellapi.h>

#include <locale>
#include <string>
#include <system_error>

#include "slacky.h"

namespace
{
	// コマンドラインにアクセスする：
	//
	// * wWinMain 関数の引数ではなく GetCommandLineW 関数を使用する。
	// * 実行ファイル名を含む完全なコマンドライン文字列にアクセスできる。
	//
	class CommandLine
	{
		int m_argc;
		wchar_t ** m_argv;

	public:
		CommandLine() : m_argc(0), m_argv(::CommandLineToArgvW(::GetCommandLineW(), &m_argc))
		{
			if (!m_argv)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "CommandLineToArgvW()");
			}
		}

		~CommandLine()
		{
			::LocalFree(m_argv);
		}

		CommandLine(const CommandLine &) = delete;
		CommandLine & operator=(const CommandLine &) = delete;

		size_t size() const noexcept
		{
			return m_argc;
		}

		const wchar_t * operator[](size_t index) const noexcept
		{
			return m_argv[index];
		}
	};


	// 環境変数を展開する
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
}


_Use_decl_annotations_
int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrev*/, PWSTR /*pCmdLine*/, int /*nCmdShow*/)
{
	using slacky::SlackBot;

	try
	{
		std::locale::global(std::locale(""));

		CommandLine cmdLine;

		if (cmdLine.size() > 3)
		{
			SlackBot bot(cmdLine[1]);

			if (bot.Post(cmdLine[2], Expand(cmdLine[3])))
			{
				// TODO: Toaster notification with name and icon !!
				auto name = bot.Name();
				auto icon = bot.Icon();
				return 0;
			}
		}

		return 1;
	}
	catch (const std::exception & e)
	{
		::MessageBoxA(nullptr, e.what(), VS_TARGETNAME, MB_ICONERROR);
	}

	return 2;
}
