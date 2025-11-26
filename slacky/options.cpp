#include "options.h"
#include "textfile.h"

#include <windows.h>
#include <shellapi.h>
#undef GetMessage

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

		for (std::wstring_view argv : Params())
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

		for (std::wstring_view argv : Params())
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

		for (std::wstring_view argv : Params())
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
				auto text = TextFile(path).ToString();
				return Expand(text.c_str());
			}

			return Expand(argv.data());
		}

		throw std::runtime_error("Message is not specified.");
	}
}
