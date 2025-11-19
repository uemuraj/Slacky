#include "pch.h"

#include <slacky/http.h>
#include <slacky/convert.h>
#include <filesystem>

namespace slacky
{
	TEST(Https, Get_SlackStatus_Current)
	{
		Https https(L"slack-status.com");
		Response resp = https.Get(L"/api/v2.0.0/current");

		auto headers = resp.Headers();
		auto content = resp.GetContent();
		auto contentType = resp.ContentType();

		EXPECT_FALSE(headers.empty());
		EXPECT_FALSE(content.empty());
		EXPECT_STREQ(contentType.c_str(), L"application/json");

		if (::IsDebuggerPresent())
		{
			auto json = std::u8string_view((const char8_t *) content.data(), content.size());

			::OutputDebugStringW(L"--- Slack Status Current Response Headers ---\n");
			::OutputDebugStringW(headers.c_str());
			::OutputDebugStringW(L"--- Slack Status Current Response Content ---\n");
			::OutputDebugStringW(ConvertFrom(json).c_str());
			::OutputDebugStringW(L"\n---------------------------------------------\n");
		}
	}

	TEST(Https, DownloadFile_OkPng)
	{
		auto temp = std::filesystem::temp_directory_path();

		if (std::filesystem::exists(temp / L"Ok.png"))
		{
			GTEST_LOG_(WARNING) << "Temp already contains Ok.png; it will be overwritten.\n";
		}

		Https https(L"slack-status.com");
		auto result = https.DownloadFile(L"/img/v2/Ok.png", temp.wstring());

		EXPECT_TRUE(std::filesystem::exists(result));
		EXPECT_GT(std::filesystem::file_size(result), 0u);

		// cleanup
		std::filesystem::remove(result);
	}
}
