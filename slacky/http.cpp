#include "http.h"
#include <system_error>

namespace slacky
{
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
}
