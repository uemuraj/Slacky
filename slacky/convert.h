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


	//
	// ワイド文字列とマルチバイト文字列の相互変換：
	//

	std::wstring Widen(std::string_view mbs);
	std::string Narrow(std::wstring_view wcs);


	// UTF-16 LE 文字列と UTF-16 BE 文字列の相互変換：
	// 
	// * バイト順を入れ替えるだけで変換可能
	// * UTF-16LE -> UTF-16BE と UTF-16BE -> UTF-16LE のどちらも
	//
	inline std::wstring ConvertUTF16Endian(std::wstring_view utf16)
	{
		std::wstring buf;
		buf.reserve(utf16.size());

		for (wchar_t ch : utf16)
		{
			buf.push_back(_byteswap_ushort(ch));
		}

		return buf;
	}


	// URL エンコードを行う
	std::u8string UrlEncode(std::u8string_view str);

	// ファイルパスを file:/// 形式の URI に変換する
	std::wstring UrlFrom(const std::filesystem::path & path);
}
