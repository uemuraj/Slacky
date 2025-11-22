#pragma once

#include <string>
#include <string_view>
#include <filesystem>

namespace slacky
{
	// スタートメニューにショートカットを作成または削除する：
	//
	// * トースト通知を表示するためにショートカットには AppUserModelID も設定する。
	// * 作成時、ショートカットが既に存在する場合は何もしない。
	// * 削除時、ショートカットが存在しない場合は何もしない。
	//
	class StartMenuShortcut
	{
		std::wstring m_name;

	public:
		StartMenuShortcut(std::wstring_view name);
		~StartMenuShortcut() noexcept;

		void Create(const wchar_t * target, const wchar_t * appId) const;
		void Remove() const;

		std::filesystem::path GetPath() const;
	};
}
