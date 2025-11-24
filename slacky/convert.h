#pragma once

#include <string>
#include <string_view>
#include <filesystem>

namespace slacky
{
	//
	// ワイド文字列と UTF-8 文字列の相互変換：
	//

	std::wstring ConvertFrom(std::u8string_view u8str);
	std::u8string ConvertFrom(std::wstring_view wstr);

	// ワイド文字列をマルチバイト文字列に変換する
	std::string Narrow(std::wstring_view wstr);

	// ファイルパスを file:/// 形式の URI に変換する
	std::wstring UrlFrom(const std::filesystem::path & path);
}
