#pragma once

#include <string>
#include <string_view>

namespace slacky
{
	//
	// 文字列の相互変換機能：
	//

	std::wstring ConvertFrom(std::u8string_view u8str);
	std::u8string ConvertFrom(std::wstring_view wstr);

	// TODO: string_view と wstring_view の相互変換機能も足す
}
