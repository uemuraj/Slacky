#include "pch.h"

#include <slacky/convert.h>

namespace slacky
{
	TEST(ConvertFrom, Utf8ToWstring_Japanese)
	{
		std::u8string u8(u8"‚±‚ñ‚É‚¿‚Í"); // "Hello" in Japanese
		EXPECT_STREQ(ConvertFrom(u8).c_str(), L"‚±‚ñ‚É‚¿‚Í");
	}

	TEST(ConvertFrom, RoundTrip_Japanese)
	{
		std::u8string u8 = ConvertFrom(L"‚±‚ñ‚É‚¿‚Í"); // "Hello" in Japanese
		EXPECT_STREQ(ConvertFrom(u8).c_str(), L"‚±‚ñ‚É‚¿‚Í");
	}

	TEST(ConvertFrom, Empty)
	{
		EXPECT_TRUE(ConvertFrom(u8"").empty());
		EXPECT_TRUE(ConvertFrom(L"").empty());
	}

	TEST(Narrow, Japanese)
	{
		// Use a multibyte (Japanese) string to ensure Narrow actually performs encoding conversion
		auto result = Narrow(L"‚±‚ñ‚É‚¿‚Í");
		EXPECT_FALSE(result.empty());
	}

	TEST(Narrow, Empty)
	{
		EXPECT_TRUE(Narrow(L"").empty());
	}

	TEST(Widen, RoundTrip_Japanese)
	{
		// Create a multibyte string from a wide string then widen it back and compare
		auto mbs = Narrow(L"‚±‚ñ‚É‚¿‚Í");
		auto w = Widen(mbs);
		EXPECT_STREQ(w.c_str(), L"‚±‚ñ‚É‚¿‚Í");
	}

	TEST(Widen, Empty)
	{
		EXPECT_TRUE(Widen("").empty());
	}

	TEST(UrlFrom, Basic)
	{
		auto uri = UrlFrom(__FILEW__);
		EXPECT_FALSE(uri.empty());
		EXPECT_EQ(uri.rfind(L"file:///", 0), 0);
	}

	TEST(UrlFrom, SpaceEncoding)
	{
		auto uri = UrlFrom(L"slacky test url.txt");
		EXPECT_FALSE(uri.empty());
		EXPECT_NE(uri.find(L"%20"), std::wstring::npos);
	}

	TEST(UrlEncode, Empty)
	{
		EXPECT_TRUE(UrlEncode(u8"").empty());
	}

	TEST(UrlEncode, UnreservedRemain)
	{
		// unreserved characters and ~ should remain unencoded
		EXPECT_STREQ(ConvertFrom(UrlEncode(u8"AZaz09-_.~")).c_str(), L"AZaz09-_.~");
	}

	TEST(UrlEncode, SpaceToPlus)
	{
		EXPECT_STREQ(ConvertFrom(UrlEncode(u8" ")).c_str(), L"+");
	}

	TEST(UrlEncode, PercentEncode)
	{
		// A set of characters that should be percent-encoded
		auto encoded = UrlEncode(u8"!*'();:@&=+$,/?#[]");
		EXPECT_STREQ(ConvertFrom(encoded).c_str(), L"%21%2A%27%28%29%3B%3A%40%26%3D%2B%24%2C%2F%3F%23%5B%5D");
	}
}
