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

	static const URL_COMPONENTS kDefaultUrlComponents = URL_COMPONENTS
	{
		.dwStructSize = sizeof(URL_COMPONENTS),
		.dwSchemeLength = DWORD(-1),
		.dwHostNameLength = DWORD(-1),
		.dwUserNameLength = DWORD(-1),
		.dwPasswordLength = DWORD(-1),
		.dwUrlPathLength = DWORD(-1),
		.dwExtraInfoLength = DWORD(-1),
	};

	Url::Url(std::wstring_view url) : URL_COMPONENTS(kDefaultUrlComponents), m_url(url)
	{
		if (!::WinHttpCrackUrl(m_url.data(), (DWORD) m_url.size(), 0, this))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "WinHttpCrackUrl");
		}
	}

	Url::Url(const Url & other) : URL_COMPONENTS(kDefaultUrlComponents), m_url(other.m_url)
	{
		::WinHttpCrackUrl(m_url.data(), (DWORD) m_url.size(), 0, this);
	}

	Url::Url(Url && other) noexcept : URL_COMPONENTS(kDefaultUrlComponents), m_url(std::move(other.m_url))
	{
		::WinHttpCrackUrl(m_url.data(), (DWORD) m_url.size(), 0, this);
	}

	Session::Session() : Handle(::WinHttpOpen(L"A WinHTTP Program Slacky/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0))
	{
		if (!m_handle)
		{
			throw std::system_error(::GetLastError(), std::system_category(), "WinHttpOpen");
		}
	}

	Connection::Connection(Session & session, const wchar_t * host) : Handle(::WinHttpConnect(session, host, INTERNET_DEFAULT_HTTPS_PORT, 0))
	{
		if (m_handle == nullptr)
		{
			throw std::system_error(::GetLastError(), std::system_category(), "WinHttpConnect");
		}
	}

	Request::Request(Connection & connection, const wchar_t * verb, const wchar_t * path)
		: Handle(::WinHttpOpenRequest(connection, verb, path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE))
	{
		if (m_handle == nullptr)
		{
			throw std::system_error(::GetLastError(), std::system_category(), "WinHttpOpenRequest");
		}
	}

	void Request::Send(const wchar_t * headers, void * content, uint32_t size)
	{
		if (!::WinHttpSendRequest(m_handle, headers, -1, content, size, size, 0))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "WinHttpSendRequest");
		}
	}

	Response::Response(Request & request) : Handle(std::exchange(request.m_handle, nullptr))
	{
		if (!::WinHttpReceiveResponse(m_handle, nullptr))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "WinHttpReceiveResponse");
		}
	}

	uint32_t Response::ContentLength()
	{
		DWORD size = sizeof(DWORD);
		DWORD length = 0;

		if (!::WinHttpQueryHeaders(m_handle, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &length, &size, nullptr))
		{
			if (auto error = ::GetLastError(); error != ERROR_WINHTTP_HEADER_NOT_FOUND)
			{
				throw std::system_error(error, std::system_category(), "WinHttpQueryHeaders");
			}
		}

		return length;
	}

	static std::wstring QueryHeaderString(HINTERNET hRequest, DWORD infoLevel)
	{
		DWORD size = 0;

		if (!::WinHttpQueryHeaders(hRequest, infoLevel, nullptr, nullptr, &size, nullptr))
		{
			auto error = GetLastError();

			if (error == ERROR_WINHTTP_HEADER_NOT_FOUND)
			{
				return {};
			}

			if (error != ERROR_INSUFFICIENT_BUFFER)
			{
				throw std::system_error(error, std::system_category(), "WinHttpQueryHeaders");
			}
		}

		std::wstring buff(size / sizeof(wchar_t) - 1, L'\0');

		if (!::WinHttpQueryHeaders(hRequest, infoLevel, nullptr, buff.data(), &size, nullptr))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "WinHttpQueryHeaders");
		}

		return buff;
	}

	std::wstring Response::Headers()
	{
		return QueryHeaderString(m_handle, WINHTTP_QUERY_RAW_HEADERS_CRLF);
	}

	std::wstring Response::ContentType()
	{
		return QueryHeaderString(m_handle, WINHTTP_QUERY_CONTENT_TYPE);
	}

	std::wstring Response::ContentEncoding()
	{
		return QueryHeaderString(m_handle, WINHTTP_QUERY_CONTENT_TRANSFER_ENCODING);
	}

	static DWORD QueryDataAvailable(HINTERNET hRequest)
	{
		DWORD size = 0;

		if (!::WinHttpQueryDataAvailable(hRequest, &size))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "WinHttpQueryDataAvailable");
		}

		return size;
	}

	std::vector<std::byte> Response::GetContent()
	{
		std::vector<std::byte> buff;

		if (auto length = ContentLength(); length == 0)
		{
			buff.reserve(Minimum);
		}
		else if (length <= Maximum)
		{
			buff.reserve(length);
		}
		else
		{
			throw std::logic_error("Content-Length is too large.");
		}

		size_t offset = 0;

		for (auto size = QueryDataAvailable(m_handle), read = 0ul; size > 0; size = QueryDataAvailable(m_handle), offset += read)
		{
			buff.resize(offset + size);

			if (!::WinHttpReadData(m_handle, buff.data() + offset, size, &read))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpReadData");
			}

			if (read < size)
			{
				throw std::underflow_error(__FUNCTION__);
			}
		}

		return buff;
	}

	void Response::Recv(std::function<void(std::byte *, uint32_t)> callback)
	{
		std::vector<std::byte> buff(Minimum);

		for (auto size = QueryDataAvailable(m_handle); size > 0; size = QueryDataAvailable(m_handle))
		{
			DWORD read{};

			if (!::WinHttpReadData(m_handle, buff.data(), Minimum, &read))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpReadData");
			}

			if (read < size)
			{
				throw std::underflow_error(__FUNCTION__);
			}

			callback(buff.data(), read);
		}
	}
}
