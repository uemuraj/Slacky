#pragma once

#include <string>
#include <memory>
#include <filesystem>

namespace slacky
{
	// トースト通知を表示する：
	class Toast
	{
		struct Impl;
		std::unique_ptr<Impl> m_impl;

	public:
		Toast(const wchar_t * appId);
		~Toast() noexcept;

		void Show(const std::filesystem::path & icon, const std::wstring & title, const std::wstring & message);
	};
}
