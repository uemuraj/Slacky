#pragma once

#include <Windows.h>
#include <winhttp.h>

#include <string>
#include <string_view>

namespace slacky
{
	// UTF-8 文字列とワイド文字列の相互変換を行う。
	std::wstring ConvertFrom(std::u8string_view u8str);
	std::u8string ConvertFrom(std::wstring_view wstr);

}
