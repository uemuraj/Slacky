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

	// URL を保持するクラス：
	//
	// * コンストラクタで渡した文字列の URL を複製して保持する。
	// * URL_COMPONENTS 構造体を継承しており URL の各要素を取得するメソッドを提供する。
	//
	class Url : protected URL_COMPONENTS
	{
		std::wstring m_url;

	public:
		Url(std::wstring_view url);
		Url(const Url & other);
		Url(Url && other) noexcept;
		~Url() noexcept = default;

		Url & operator=(const Url & other);
		Url & operator=(Url && other) noexcept;

		std::wstring Host() const
		{
			return { lpszHostName, dwHostNameLength };
		}

		std::wstring Path() const
		{
			return { lpszUrlPath, dwUrlPathLength };
		}
	};
}
