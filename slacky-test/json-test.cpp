#include "pch.h"
#include <slacky/json.h>

#include <algorithm>
#include <optional>
#include <vector>

namespace slacky
{
	TEST(JsonBasic, ParseQuotedString)
	{
		Json json(LR"("hello")");
		EXPECT_EQ(json.GetString(), std::wstring(L"hello"));
	}

	TEST(JsonBasic, ParseNumberUnquoted)
	{
		Json json(L"12345");
		EXPECT_EQ(json.GetString(), std::wstring(L"12345"));
	}

	TEST(JsonBasic, ParseBoolAndNull)
	{
		EXPECT_TRUE(Json(L"true").GetBool());
		EXPECT_FALSE(Json(L"false").GetBool());
		EXPECT_TRUE(Json(L"null").IsNull());
	}

	TEST(JsonEscape, UnicodeEscape)
	{
		EXPECT_EQ(Json(LR"("\u0041\u0042\u0043")").GetString(), std::wstring(L"ABC"));
	}

	TEST(JsonVisit, CollectValues)
	{
		struct Collector
		{
			std::vector<std::wstring> items;

			void operator()(std::wstring && value)
			{
				items.push_back(value);
			}
		};

		Json json(LR"({"k1":10,"k2":"20","k3":"30"})");
		Collector collector;
		VisitJson(collector, json);

		EXPECT_EQ(collector.items.size(), 3u);
		EXPECT_EQ(collector.items[0], L"10");
		EXPECT_EQ(collector.items[1], L"20");
		EXPECT_EQ(collector.items[2], L"30");
	}

	TEST(JsonVisit, CollectKeysAndValues)
	{
		struct Collector
		{
			std::map<std::wstring, std::wstring> items;

			void operator()(std::wstring && key, Json && value)
			{
				items[key] = value.GetString();
			}
		};

		Json json(LR"({"k1":10,"k2":"20","k3":"30"})");
		Collector collector;
		VisitJson(collector, json);

		EXPECT_EQ(collector.items.size(), 3u);
		EXPECT_EQ(collector.items[L"k1"], L"10");
		EXPECT_EQ(collector.items[L"k2"], L"20");
		EXPECT_EQ(collector.items[L"k3"], L"30");
	}

	TEST(JsonVisit, SlackResponseType1)
	{
		struct ResponseType1
		{
			struct Metadata
			{
				struct Warnings : std::vector<std::wstring>
				{
					void operator()(std::wstring && value)
					{
						push_back(value);
					}
				};

				Warnings warnings;

				void operator()(std::wstring && key, Json && value)
				{
					if (key == L"warnings")
					{
						VisitJson(warnings, value);
					}
				}
			};

			std::optional<bool> ok;
			std::wstring error;
			std::wstring warning;
			Metadata metadata;

			void operator()(std::wstring && key, Json && value)
			{
				if (key == L"ok")
				{
					ok = value.GetBool();
					return;
				}

				if (key == L"error")
				{
					error = value.GetString();
					return;
				}

				if (key == L"warning")
				{
					warning = value.GetString();
					return;
				}

				if (key == L"response_metadata")
				{
					VisitJson(metadata, value);
					return;
				}
			}
		};

		Json json(LR"({"ok":false,"error":"not_in_channel","warning":"superfluous_charset","response_metadata":{"warnings":["superfluous_charset"]}})");

		ResponseType1 response;
		VisitJson(response, json);

		EXPECT_TRUE(response.ok.has_value());
		EXPECT_FALSE(response.ok.value());
		EXPECT_EQ(response.error, L"not_in_channel");
		EXPECT_EQ(response.warning, L"superfluous_charset");
		EXPECT_EQ(response.metadata.warnings.size(), 1u);
		EXPECT_EQ(response.metadata.warnings[0], L"superfluous_charset");
	}

	TEST(JsonVisit, SlackResponseType2)
	{
		struct ResponseType2
		{
			struct Message
			{
				std::wstring type;
				std::wstring text;

				struct BotProfile
				{
					std::wstring name;

					struct Icons
					{
						std::wstring image36;
						std::wstring image48;
						std::wstring image72;

						void operator()(std::wstring && key, Json && value)
						{
							if (key == L"image_36")
							{
								image36 = value.GetString();
								return;
							}

							if (key == L"image_48")
							{
								image48 = value.GetString();
								return;
							}

							if (key == L"image_72")
							{
								image72 = value.GetString();
								return;
							}
						}
					};

					BotProfile::Icons icons;

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
				};

				BotProfile botProfile;

				void operator()(std::wstring && key, Json && value)
				{
					if (key == L"type")
					{
						type = value.GetString();
						return;
					}

					if (key == L"text")
					{
						text = value.GetString();
						return;
					}

					if (key == L"bot_profile")
					{
						VisitJson(botProfile, value);
						return;
					}
				}
			};

			std::optional<bool> ok;

			Message message;

			void operator()(std::wstring && key, Json && value)
			{
				if (key == L"ok")
				{
					ok = value.GetBool();
					return;
				}

				if (key == L"message")
				{
					VisitJson(message, value);
					return;
				}
			}
		};

		Json json(LR"(
{
  "ok":true,
  "message":
  {
    "type":"message",
    "text":"\u65b0\u3057\u3044\u30c1\u30e3\u30f3\u30cd\u30eb\u306f\u3069\u3093\u306a\u611f\u3058\uff1f",
    "bot_profile":
    {
      "name":"hoge",
      "icons":
      {
        "image_36":"https:\/\/a.slack-edge.com\/hoge\/img\/plugins\/app\/bot_36.png",
        "image_48":"https:\/\/a.slack-edge.com\/hoge\/img\/plugins\/app\/bot_48.png",
        "image_72":"https:\/\/a.slack-edge.com\/hoge\/img\/plugins\/app\/service_72.png"}
      }
    }
  }
}
)");
		ResponseType2 response;
		VisitJson(response, json);

		EXPECT_TRUE(response.ok.has_value());
		EXPECT_TRUE(response.ok.value());
		EXPECT_EQ(response.message.type, L"message");
		EXPECT_EQ(response.message.text, L"新しいチャンネルはどんな感じ？");
		EXPECT_EQ(response.message.botProfile.name, L"hoge");
		EXPECT_EQ(response.message.botProfile.icons.image36, L"https://a.slack-edge.com/hoge/img/plugins/app/bot_36.png");
		EXPECT_EQ(response.message.botProfile.icons.image48, L"https://a.slack-edge.com/hoge/img/plugins/app/bot_48.png");
		EXPECT_EQ(response.message.botProfile.icons.image72, L"https://a.slack-edge.com/hoge/img/plugins/app/service_72.png");
	}
}
