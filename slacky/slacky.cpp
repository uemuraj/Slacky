#include "slacky.h"
#include "http.h"
#include "json.h"

#include <optional>
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

	class SlackApi : Https
	{
		SlackApiResponse m_response;

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
			return m_response.bot;
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

				VisitJson(m_response, json);

				if (m_response.ok.has_value())
				{
					return m_response.ok.value();
				}

				// TODO: 例外に適切な情報を含める
				throw std::runtime_error("Missing 'ok' field in response.");
			}

			// TODO: 例外に適切な情報を含める
			throw std::runtime_error("Unexpected response.");
		}
	};

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
				//m_icon = DownloadIcon(bot.icons.image_48);
			}

			return true;
		}

		return false;
	}
}
