#pragma once

#include <string>
#include <string_view>
#include <span>

#if defined(GetMessage)
#pragma push_macro("GetMessage")
#define SLACKY_RESTORE_GetMessage
#undef GetMessage
#endif

namespace slacky
{
	// 動作オプションを取得する：
	//
	// * コマンドライン引数を基に動作オプションを取得する。
	// * wWinMain 関数の引数ではなく内部で GetCommandLineW 関数を使用する。
	//
	class Options
	{
		int m_argc;
		wchar_t ** m_argv;

	public:
		Options();
		~Options() noexcept;

		Options(const Options &) = delete;
		Options & operator=(const Options &) = delete;

		std::wstring GetAppUserModelID() const;
		std::wstring GetExecutablePath() const;

		std::wstring GetToken() const;
		std::wstring GetChannel() const;
		std::wstring GetMessage() const;

	private:
		decltype(auto) Params() const
		{
			if (m_argc > 1)
			{
				return std::span(m_argv + 1, static_cast<size_t>(m_argc - 1));
			}

			return std::span(m_argv, 0);
		}
	};
}

#if defined(SLACKY_RESTORE_GetMessage)
#pragma pop_macro("GetMessage")
#undef SLACKY_RESTORE_GetMessage
#endif
