#include <windows.h>
#include <shellapi.h>
#include <shlobj_core.h>

#include <locale>
#include <string>
#include <format>
#include <filesystem>
#include <system_error>

#include "slacky.h"
#include "shortcut.h"
#include "toast.h"
#include "convert.h"

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


	// アプリケーションの AppUserModelID を構成する
	std::wstring GetAppUserModelID()
	{
		auto aumid = Expand(L"%COMPUTERNAME%.slacky");

		if (aumid[0] == L'%')
		{
			throw std::runtime_error("Environment variable 'COMPUTERNAME' is not defined.");
		}

		return aumid;
	}
}


_Use_decl_annotations_
int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrev*/, PWSTR /*pCmdLine*/, int /*nCmdShow*/)
{
	using namespace slacky;

	try
	{
		std::locale::global(std::locale(""));

		auto aumid = GetAppUserModelID();

		CommandLine cmdLine;

		if (cmdLine.size() > 0)
		{
			auto target = std::filesystem::absolute(cmdLine[0]);
			StartMenuShortcut shortcut(VS_SOLUTION);
			shortcut.Create(target.c_str(), aumid.c_str());
		}

		if (::SetCurrentProcessExplicitAppUserModelID(aumid.c_str()) != S_OK)
		{
			throw std::system_error(::GetLastError(), std::system_category(),
				std::format("SetCurrentProcessExplicitAppUserModelID({})", Narrow(aumid)));
		}

		if (cmdLine.size() > 3)
		{
			SlackBot bot(cmdLine[1]);

			auto channel = cmdLine[2];
			auto message = Expand(cmdLine[3]);

			if (bot.Post(channel, message))
			{
				auto & icon = bot.Icon();
				auto & name = bot.Name();

				Toast toast(aumid.c_str());
				toast.Show(icon, name, message);
				return 0;
			}
		}

		return 1;
	}
	catch (const std::exception & e)
	{
		::MessageBoxA(nullptr, e.what(), VS_SOLUTIONA, MB_ICONERROR);
	}

	return 2;
}
