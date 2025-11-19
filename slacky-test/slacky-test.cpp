#include "pch.h"

#include <slacky/slacky.h>
#include <windows.h>
#include <filesystem>

namespace slacky
{
	std::wstring GetEnvWide(const wchar_t * name)
	{
		if (auto size = ::GetEnvironmentVariableW(name, nullptr, 0); size > 1)
		{
			std::wstring value(size - 1, L'\0');

			if (::GetEnvironmentVariableW(name, value.data(), size) > 0)
			{
				return value;
			}
		}

		return {};
	}

	class SlackBotTest : public ::testing::Test
	{
	protected:
		std::wstring m_token;
		std::wstring m_channel;

		void SetUp() override
		{
			m_token = GetEnvWide(L"SLACK_BOT_TOKEN");
			m_channel = GetEnvWide(L"SLACK_CHANNEL");

			ASSERT_FALSE(m_token.empty()) << "Integration test requires SLACK_BOT_TOKEN environment variable (wide string on Windows).";
			ASSERT_FALSE(m_channel.empty()) << "Integration test requires SLACK_CHANNEL environment variable (wide string on Windows).";
		}
	};

	TEST_F(SlackBotTest, PostMessage_Integration)
	{
		SlackBot bot(m_token);

		bool result = bot.Post(m_channel, L"Slacky integration test: hello from Google Test");

		ASSERT_TRUE(result);
		EXPECT_FALSE(bot.Name().empty());
		EXPECT_FALSE(bot.Icon().empty());

		std::filesystem::path icon = bot.Icon();
		EXPECT_TRUE(std::filesystem::exists(icon));
		EXPECT_STREQ(icon.extension().c_str(), L".png");
	}
}
