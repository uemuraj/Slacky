#include "pch.h"
#include <slacky/http.h>

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

	TEST(Url, HostAndPath_Basic)
	{
		Url u(L"https://example.com/path/to/resource?query=1#frag");
		EXPECT_STREQ(u.Host().c_str(), L"example.com");
		EXPECT_STREQ(u.Path().c_str(), L"/path/to/resource");
	}

	TEST(Url, HostWithPort)
	{
		Url u(L"http://localhost:8080/");
		EXPECT_STREQ(u.Host().c_str(), L"localhost");
		EXPECT_STREQ(u.Path().c_str(), L"/");
	}

	TEST(Url, CopyConstructor)
	{
		Url original(L"https://copy.example.com/dir/file");
		Url copy(original);
		EXPECT_STREQ(copy.Host().c_str(), L"copy.example.com");
		EXPECT_STREQ(copy.Path().c_str(), L"/dir/file");
	}

	TEST(Url, MoveConstructor)
	{
		Url original(L"https://move.example.com/abc");
		Url moved(std::move(original));
		EXPECT_STREQ(moved.Host().c_str(), L"move.example.com");
		EXPECT_STREQ(moved.Path().c_str(), L"/abc");
	}

	TEST(Url, CopyAssignment)
	{
		Url a(L"https://a.example.com/x");
		Url b(L"https://b.example.com/y");
		a = b;
		EXPECT_STREQ(a.Host().c_str(), L"b.example.com");
		EXPECT_STREQ(a.Path().c_str(), L"/y");
	}

	TEST(Url, MoveAssignment)
	{
		Url a(L"https://a.example.com/x");
		Url b(L"https://b.example.com/y");
		a = std::move(b);
		EXPECT_STREQ(a.Host().c_str(), L"b.example.com");
		EXPECT_STREQ(a.Path().c_str(), L"/y");
	}
}
