#pragma once

#include <string>
#include <string_view>

namespace slacky
{
	//
	// •¶š—ñ‚Ì‘ŠŒİ•ÏŠ·‹@”\F
	//

	std::wstring ConvertFrom(std::u8string_view u8str);
	std::u8string ConvertFrom(std::wstring_view wstr);

	std::string Narrow(std::wstring_view wstr);
}
