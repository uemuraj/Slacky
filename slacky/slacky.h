#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace slacky
{
	class SlackApi;

	// Slack チャネルにメッセージを投稿する：
	//
	// * コンストラクタで Bot User OAuth Token を指定する。
	// * Post メソッドでチャネルとメッセージを指定して投稿する。
	// * 投稿後、Bot の名前とアイコンを取得できる。
	//
	class SlackBot
	{
		std::unique_ptr<SlackApi> m_api;
		std::wstring m_token;
		std::wstring m_name;
		std::wstring m_icon;

	public:
		SlackBot(std::wstring_view token);
		~SlackBot() noexcept;

		bool Post(std::wstring_view channel, std::wstring_view message);

		const std::wstring & Name()
		{
			return m_name;
		}

		const std::wstring & Icon()
		{
			return m_icon;
		}

	private:
		std::wstring DownloadIcon(std::wstring_view url);
	};

	// アプリケーションの AppUserModelID：
	inline constexpr wchar_t kAppUserModelID[] = L"com.uemuraj.Slacky";
}
