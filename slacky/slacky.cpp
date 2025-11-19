#include "slacky.h"
#include "json.h"
#include "http.h"
#include "convert.h"

#include <optional>
#include <filesystem>
#include <system_error>

namespace slacky
{
	// Slack API のレスポンスを受け取るための構造体：
	//
	// * VisitJson テンプレートと組み合わせて使用することで必要な値を抽出できる。
	//
	struct SlackApiResponse
	{
		std::optional<bool> ok;

		struct Bot
		{
			std::wstring name;

			struct Icons
			{
				std::wstring image_36;
				std::wstring image_48;
				std::wstring image_76;

				void operator()(std::wstring && key, Json && value)
				{
					if (key == L"image_36")
					{
						image_36 = value.GetString();
						return;
					}

					if (key == L"image_48")
					{
						image_48 = value.GetString();
						return;
					}

					if (key == L"image_76")
					{
						image_76 = value.GetString();
						return;
					}
				}

			} icons;

			void operator()(std::wstring && key, Json && value)
			{
				if (key == L"name")
				{
					name = value.GetString();
					return;
				}

				if (key == L"icons")
				{
					VisitJson(icons, value);
					return;
				}
			}

		} bot;

		void operator()(std::wstring && key, Json && value)
		{
			if (key == L"ok")
			{
				ok = value.GetBool();
				return;
			}

			if (key.starts_with(L"bot"))
			{
				VisitJson(bot, value);
				return;
			}
		}
	};


	// Slack API へのアクセス：
	//
	// * コンストラクタで Bot User OAuth Token を指定する。
	// * ホスト名は "slack.com" に固定されており、Get/Post メソッドでパスを指定して API を呼び出す。
	// * 直近のレスポンスを SlackApiResponse 構造体にパースして保持する。
	//
	class SlackApi : Https
	{
		std::unique_ptr<SlackApiResponse> m_response;

	public:
		SlackApi(std::wstring_view token) : Https(L"slack.com")
		{
			SetBearerToken(token);
			AddContentType(L"application/x-www-form-urlencoded");
		}

		bool Get(const wchar_t * path)
		{
			auto response = Https::Get(path);
			return ParseResponse(response);
		}

		bool Post(const wchar_t * path, std::wstring_view form)
		{
			auto content = ConvertFrom(form);
			auto response = Https::Post(path, content.data(), (uint32_t) content.size());
			return ParseResponse(response);
		}

		SlackApiResponse::Bot & Bot()
		{
			if (m_response)
			{
				return m_response->bot;
			}

			throw std::logic_error("Bot() called before any response: call Get/Post first.");
		}

	private:
		bool ParseResponse(Response & response)
		{
			auto contentType = response.ContentType();
			auto contentData = response.GetContent();

			if (contentType.starts_with(L"application/json"))
			{
				auto data = std::u8string_view((const char8_t *) contentData.data(), contentData.size());
				auto text = ConvertFrom(data);
				auto json = Json(text);

				if (::IsDebuggerPresent())
				{
					::OutputDebugStringW(L"=== Response ===\n");
					::OutputDebugStringW(response.Headers().c_str());
					::OutputDebugStringW(text.c_str());
					::OutputDebugStringW(L"\n================\n");
				}

				m_response = std::make_unique<SlackApiResponse>();

				VisitJson(*m_response, json);

				if (m_response->ok.has_value())
				{
					return m_response->ok.value();
				}

				throw std::runtime_error("Response missing 'ok' field.");
			}

			throw std::runtime_error(std::format("Unexpected content type.\n`{}`", Narrow(contentType)));
		}
	};


	// URL を解析してホスト名とパスを返す
	std::pair<std::wstring, std::wstring> UrlParser(std::wstring_view url)
	{
		URL_COMPONENTS uc{ .dwStructSize = sizeof(URL_COMPONENTS), .dwHostNameLength = DWORD(-1), .dwUrlPathLength = DWORD(-1), };

		if (!::WinHttpCrackUrl(url.data(), (DWORD) url.size(), 0, &uc))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "WinHttpCrackUrl");
		}

		if (!uc.lpszHostName || !uc.lpszUrlPath)
		{
			throw std::runtime_error(std::format("UrlParser: Failed to parse URL.\n`{}`", Narrow(url)));
		}

		return { {uc.lpszHostName, uc.dwHostNameLength}, {uc.lpszUrlPath, uc.dwUrlPathLength} };
	}


	//
	// SlackBot クラスの実装：
	//

	SlackBot::SlackBot(std::wstring_view token) : m_api(std::make_unique<SlackApi>(token)), m_token(token)
	{}

	SlackBot::~SlackBot() noexcept
	{}

	bool SlackBot::Post(std::wstring_view channel, std::wstring_view message)
	{
		if (m_api->Post(L"/api/chat.postMessage", std::format(L"channel={}&text={}", channel, message)))
		{
			if (m_name.empty() || m_icon.empty())
			{
				auto & bot = m_api->Bot();
				m_name = bot.name;
				m_icon = DownloadIcon(bot.icons.image_48);
			}

			return true;
		}

		return false;
	}

	std::wstring SlackBot::DownloadIcon(std::wstring_view url)
	{
		auto [host, path] = UrlParser(url);

		Https https(host.c_str());
		https.SetBearerToken(m_token);
		return https.DownloadFile(path.c_str(), std::filesystem::temp_directory_path().native());
	}
}
