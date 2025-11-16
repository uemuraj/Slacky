#include "json.h"

#include <stack>
#include <stdexcept>

namespace slacky
{
	// ワイド文字１つを16進数の数値 (0..15) に変換する。16進数文字でない場合は例外を投げる。
	static std::uint16_t FromHexChar(wchar_t ch)
	{
		static constexpr wchar_t hexChars[] = L"0123456789ABCDEF";

		if (auto pos = wcschr(hexChars, towupper(ch)))
		{
			return (std::uint16_t) (pos - hexChars);
		}

		throw new std::invalid_argument("Invalid hex character.");
	}

	// エスケープ文字の妥当性を検証し、対応する文字を返す。
	static wchar_t ValidateEscapeChar(wchar_t ch)
	{
		switch (ch)
		{
		case L'"':
		case L'\\':
		case L'/':
			return ch;
		case L'b':
			return L'\b';
		case L'f':
			return L'\f';
		case L'n':
			return L'\n';
		case L'r':
			return L'\r';
		case L't':
			return L'\t';
		case L'u':
			return L'\0';
		default:
			throw new std::invalid_argument("Invalid escape character.");
		}
	}

	// エスケープシーケンスを展開した文字列を返す。エスケープシーケンスが不正な場合は例外を投げる。
	static std::wstring UnescapeString(const wchar_t * ptr, const wchar_t * end)
	{
		std::wstring buff;

		buff.reserve(end - ptr);

		while (ptr < end)
		{
			auto ch = *ptr++;

			if (ch == L'\\')
			{
				ch = ValidateEscapeChar(*ptr++);

				if (ch == L'\0')
				{
					if ((end - ptr) < 4)
					{
						throw new std::invalid_argument("Invalid escape sequence.");
					}

					ch |= FromHexChar(*ptr++);
					ch <<= 4;
					ch |= FromHexChar(*ptr++);
					ch <<= 4;
					ch |= FromHexChar(*ptr++);
					ch <<= 4;
					ch |= FromHexChar(*ptr++);
				}
			}

			buff.push_back(ch);
		}

		return buff;
	}


	// JSON 解析コンテキスト
	struct JsonContext : std::enable_shared_from_this<JsonContext>
	{
		const wchar_t * txt;
		const wchar_t * end;
		bool next;
		std::stack<Json::State> current;

		JsonContext(std::wstring_view json) : txt(json.data()), end(json.data() + json.size()), next(false)
		{}

		Json::Value Parse(size_t nested)
		{
			if (SkipWhiteSpace())
			{
				switch (*txt++)
				{
				case L'{':
					current.push(Json::State::Object), next = true;
					return Json::State::Object;

				case L'[':
					current.push(Json::State::Array), next = true;
					return Json::State::Array;

				case L'}':
				case L']':
					current.pop(), next = false;
					return (current.size() > nested) ? Json::State::Next : Json::State::End;

				case L',':
					return ParseNext(nested);

				case L'"':
					if (std::exchange(next, false))
					{
						--txt;
						return ParseNext(nested);
					}
					return QuotedString();

				default:
					--txt;
					if (std::exchange(next, false))
					{
						return ParseNext(nested);
					}
					return UnquotedString();
				}
			}

			return Json::State::End;
		}

		Json::Value ParseNext(size_t nested)
		{
			if (SkipWhiteSpace())
			{
				if (*txt == L'}' || *txt == L']')
				{
					++txt;
					current.pop();
					return (current.size() > nested) ? Json::State::Next : Json::State::End;
				}

				if (current.empty() || current.top() == Json::State::Array)
				{
					return Parse(nested);
				}
				else
				{
					return ParseKeyValue();
				}
			}

			return Json::State::End;
		}

		bool SkipWhiteSpace()
		{
			while (txt < end)
			{
				if (iswgraph(*txt))
				{
					return true;
				}

				++txt;
			}

			return false;
		}

		bool SkipWhiteSpaceTo(wchar_t ch)
		{
			while (txt < end)
			{
				if (*txt == ch)
				{
					return true;
				}

				if (iswgraph(*txt))
				{
					return false;
				}

				++txt;
			}

			return false;
		}

		std::pair<std::wstring, Json> ParseKeyValue()
		{
			auto key = ParseString();

			if (SkipWhiteSpaceTo(L':'))
			{
				++txt;
				return { key, Json(shared_from_this()) };
			}

			throw new std::invalid_argument("Key value pair expected.");
		}

		std::wstring ParseString()
		{
			if (*txt == L'"')
			{
				++txt;
				return QuotedString();
			}

			return UnquotedString();
		}

		std::wstring QuotedString()
		{
			bool escape = false;
			auto begin = txt;

			while (txt < end)
			{
				if (*txt == L'"')
				{
					if (!escape)
					{
						return { begin, txt++ };
					}

					return UnescapeString(begin, txt++);
				}

				if (*txt++ == L'\\')
				{
					ValidateEscapeChar(*txt++);
					escape = true;
				}
			}

			throw new std::invalid_argument("Unterminated string.");
		}

		std::wstring UnquotedString()
		{
			bool escape = false;
			auto begin = txt;

			while (txt < end)
			{
				if (wcschr(LR"(,[]{})", *txt) || !iswgraph(*txt))
				{
					if (!escape)
					{
						return { begin, txt };
					}

					return UnescapeString(begin, txt);
				}

				if (*txt++ == L'\\')
				{
					ValidateEscapeChar(*txt++);
					escape = true;
				}
			}

			return { begin, end };
		}
	};


	Json::Json(std::shared_ptr<JsonContext> context) : m_nested(context->current.size()), m_context(context)
	{}

	Json::Json(std::wstring_view text) : m_nested(0), m_context(std::make_shared<JsonContext>(text))
	{}

	Json::~Json() noexcept
	{}

	Json::Value Json::Parse()
	{
		return m_context->Parse(m_nested);
	}
}
