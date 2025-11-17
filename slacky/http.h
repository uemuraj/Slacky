#pragma once

#include <Windows.h>
#include <winhttp.h>

#include <string>
#include <string_view>
#include <cstddef> 
#include <vector>
#include <format>
#include <functional>

namespace slacky
{
	//
	// UTF-8 文字列とワイド文字列の相互変換
	//

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

		Url & operator=(const Url & other) = delete;
		Url & operator=(Url && other) = delete;

		std::wstring Host() const
		{
			return { lpszHostName, dwHostNameLength };
		}

		std::wstring Path() const
		{
			return { lpszUrlPath, dwUrlPathLength };
		}
	};


	//
	// 以下は WinHTTP を利用するためのラッパークラス群：
	// 
	// * https かつ INTERNET_DEFAULT_HTTPS_PORT に機能を限定。接続先の指定はホスト名のみ。
	//

	struct Handle
	{
		HINTERNET m_handle;

		Handle(HINTERNET handle) noexcept : m_handle(handle)
		{}

		~Handle() noexcept
		{
			if (m_handle)
			{
				::WinHttpCloseHandle(m_handle);
			}
		}

		operator HINTERNET() const noexcept
		{
			return m_handle;
		}

		Handle(const Handle &) = delete;
		Handle(Handle && other) = delete;

		Handle & operator=(const Handle &) = delete;
		Handle & operator=(Handle && other) = delete;
	};

	struct Session : Handle
	{
		Session();
		~Session() noexcept = default;
	};

	struct Connection : Handle
	{
		Connection(Session & session, const wchar_t * host);
		~Connection() noexcept = default;
	};

	struct Request : Handle
	{
		Request(Connection & connection, const wchar_t * verb, const wchar_t * path);
		~Request() noexcept = default;

		void Send(const wchar_t * headers, void * content, uint32_t size);
	};

	struct Response : Handle
	{
		Response(Request & request);
		~Response() noexcept = default;

		std::wstring Headers();
		std::wstring ContentType();
		std::wstring ContentEncoding();

		const uint32_t Minimum = (uint32_t) 16 * 1024;
		const uint32_t Maximum = (uint32_t) 16 * 1024 * 1024;

		uint32_t ContentLength();

		std::vector<std::byte> GetContent();

		void Recv(std::function<void(std::byte *, uint32_t)> callback);
	};


	// HTTPS 通信の簡易な実装：
	//
	// * コンストラクタでホスト名を指定、Get/Post メソッドで URL パスを指定する。
	// * リクエストヘッダの設定メソッドは最低限の実装。
	//
	class Https
	{
		Session m_session;
		Connection m_connection;
		std::wstring m_headers;

	public:
		Https(const wchar_t * host) : m_connection(m_session, host)
		{}
		~Https() noexcept = default;

		void SetBearerToken(std::wstring_view token)
		{
			m_headers += std::format(L"Authorization: Bearer {}\r\n", token);
		}

		void AddContentType(std::wstring_view type)
		{
			m_headers += std::format(L"Content-Type: {}\r\n", type);
		}

		Response Get(const wchar_t * path)
		{
			Request request(m_connection, L"GET", path);
			request.Send(m_headers.c_str(), nullptr, 0);
			return Response(request);
		}

		Response Post(const wchar_t * path, void * content, uint32_t size)
		{
			Request request(m_connection, L"POST", path);
			request.Send(m_headers.c_str(), content, size);
			return Response(request);
		}
	};
}
