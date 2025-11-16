#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace slacky
{
	struct JsonContext;

	// Pull 型の JSON 解析器：
	//
	// * 解析対象の JSON テキストを所有しないことに注意。内部的に JsonContext も、そのように動作する。
	// * Parse メソッドを繰り返し呼び出すことで JSON テキストを逐次的に解析できる。
	// * 解析結果は文字列のみ。数値や真偽値、null なども文字列として取得される。
	// * VisitJson テンプレートと組み合わせて使用することで、Visitor パターンで JSON テキストをすべて処理できる。
	//
	class Json
	{
		const size_t m_nested;
		std::shared_ptr<JsonContext> m_context;

	public:
		Json(std::shared_ptr<JsonContext> context);
		Json(std::wstring_view text);
		~Json() noexcept;

		enum State { Object, Array, Next, End };

		using Value = std::variant<State, std::pair<std::wstring, Json>, Json, std::wstring>;

		Value Parse();

		std::wstring GetString()
		{
			return std::get<std::wstring>(Parse());
		}

		bool GetBool()
		{
			return std::get<std::wstring>(Parse()) == L"true";
		}

		bool IsNull()
		{
			return std::get<std::wstring>(Parse()) == L"null";
		}
	};


	// VisitJson 内部で使用するアダプタ的なクラステンプレート：
	//
	// * ユーザー定義 T 型のオブジェクトをラップする。ユーザーは興味がある値に対してのみ operator() を定義すればよい。
	//
	template <typename T>
	class JsonVisitor
	{
		T & m_visitor;

	public:
		JsonVisitor(T & visitor) : m_visitor(visitor)
		{}

		bool operator()(std::pair<std::wstring, Json> && keyValue)
		{
			if constexpr (std::is_invocable_v<T, std::wstring &&, Json &&>)
			{
				m_visitor(std::move(std::get<0>(keyValue)), std::move(std::get<1>(keyValue)));
			}

			return true;
		}

		bool operator()(Json && value)
		{
			if constexpr (std::is_invocable_v<T, Json &&>)
			{
				m_visitor(std::move(value));
			}

			return true;
		}

		bool operator()(std::wstring && value)
		{
			if constexpr (std::is_invocable_v<T, std::wstring &&>)
			{
				m_visitor(std::move(value));
			}

			return true;
		}

		bool operator()(Json::State state)
		{
			return (state != Json::State::End); // 解析を終了するタイミングは End のみ
		}
	};


	// JSON テキストを Visitor パターンで処理するためのユーティリティ
	template <typename T>
	inline void VisitJson(T & visitor, Json & json)
	{
		while (std::visit(JsonVisitor<T>(visitor), json.Parse())) /**/;
	}
}
