#include "convert.h"

#include <Windows.h>
#include <shlwapi.h>
#include <system_error>

namespace slacky
{
	//
	// ワイド文字列と UTF-8 文字列の相互変換：
	//

	std::wstring ConvertFrom(std::u8string_view utf8)
	{
		if (!utf8.empty())
		{
			const auto utf8_size = (int) utf8.size();
			const auto wstr_size = ::MultiByteToWideChar(CP_UTF8, 0, (const char *) utf8.data(), utf8_size, nullptr, 0);

			if (wstr_size <= 0)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "MultiByteToWideChar");
			}

			std::wstring wstr(wstr_size, L'\0');

			if (::MultiByteToWideChar(CP_UTF8, 0, (const char *) utf8.data(), utf8_size, wstr.data(), wstr_size) == 0)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "MultiByteToWideChar");
			}

			return wstr;
		}

		return {};
	}

	std::u8string ConvertFrom(std::wstring_view wstr)
	{
		if (!wstr.empty())
		{
			const auto wstr_size = (int) wstr.size();
			const auto utf8_size = ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstr_size, nullptr, 0, nullptr, nullptr);

			if (utf8_size <= 0)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WideCharToMultiByte");
			}

			std::u8string utf8(utf8_size, '\0');

			if (::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstr_size, (char *) utf8.data(), utf8_size, nullptr, nullptr) == 0)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WideCharToMultiByte");
			}

			return utf8;
		}

		return {};
	}

	std::wstring Widen(std::string_view mbs)
	{
		if (!mbs.empty())
		{
			const auto mbs_size = (int) mbs.size();
			const auto wcs_size = ::MultiByteToWideChar(CP_ACP, 0, mbs.data(), mbs_size, nullptr, 0);

			if (wcs_size <= 0)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "MultiByteToWideChar");
			}

			std::wstring wcs(wcs_size, L'\0');

			if (::MultiByteToWideChar(CP_ACP, 0, mbs.data(), mbs_size, wcs.data(), wcs_size) == 0)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "MultiByteToWideChar");
			}

			return wcs;
		}

		return {};
	}

	std::string Narrow(std::wstring_view wcs)
	{
		if (!wcs.empty())
		{
			const auto wcs_size = (int) wcs.size();
			const auto mbs_size = ::WideCharToMultiByte(CP_ACP, 0, wcs.data(), wcs_size, nullptr, 0, nullptr, nullptr);

			if (mbs_size <= 0)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WideCharToMultiByte");
			}

			std::string mbs(mbs_size, '\0');

			if (::WideCharToMultiByte(CP_ACP, 0, wcs.data(), wcs_size, mbs.data(), mbs_size, nullptr, nullptr) == 0)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WideCharToMultiByte");
			}

			return mbs;
		}

		return {};
	}

	std::u8string UrlEncode(std::u8string_view str)
	{
		if (!str.empty())
		{
			std::u8string encoded;
			encoded.reserve(str.size() * 3); // 最悪ケースでも余裕あり

			// * RFC 3986 の "unreserved" + よく使う ~ はそのまま
			// * スペースは + に変換
			// * それ以外は %HH の形式でエンコード

			static const char8_t hex[] = u8"0123456789ABCDEF";

			for (char8_t c : str)
			{
				if ((c >= 'A' && c <= 'Z') ||
					(c >= 'a' && c <= 'z') ||
					(c >= '0' && c <= '9') ||
					c == '-' || c == '_' || c == '.' || c == '~')
				{
					encoded.push_back(c);
				}
				else if (c == ' ')
				{
					encoded.push_back(u8'+');
				}
				else
				{
					encoded.push_back(u8'%');
					encoded.push_back(hex[c >> 4]);
					encoded.push_back(hex[c & 0x0F]);
				}
			}

			return encoded;
		}

		return {};
	}

	std::wstring UrlFrom(const std::filesystem::path & path)
	{
		if (!path.empty())
		{
			auto abs = std::filesystem::absolute(path).wstring();

			std::wstring url(abs.size() + _countof(L"file:///"), L'\0');

			DWORD cch = (DWORD) url.size();

			if (auto hr = ::UrlCreateFromPathW(abs.c_str(), url.data(), &cch, 0); FAILED(hr))
			{
				url.resize((size_t) cch - 1);

				if (auto hr = ::UrlCreateFromPathW(abs.c_str(), url.data(), &cch, 0); FAILED(hr))
				{
					throw std::system_error(hr, std::system_category(), "UrlCreateFromPathW");
				}
			}

			return url;
		}

		throw std::invalid_argument("path is empty.");
	}
}
