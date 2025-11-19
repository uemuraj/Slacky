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
}
