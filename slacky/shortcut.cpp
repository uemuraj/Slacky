#include "shortcut.h"

#include <windows.h>
#include <comdef.h>
#include <shlobj.h>
#include <propkey.h>
#include <propvarutil.h>

#include <memory>
#include <filesystem>
#include <system_error>

template<typename T>
using com_ptr_t = _com_ptr_t<_com_IIID<T, &__uuidof(T)>>;

namespace slacky
{
	struct PropVariant : PROPVARIANT
	{
		PropVariant(const wchar_t * psz) : PROPVARIANT{}
		{
			if (auto hr = ::InitPropVariantFromString(psz, this); FAILED(hr))
			{
				throw std::system_error(hr, std::system_category(), "InitPropVariantFromString");
			}
		}

		~PropVariant() noexcept
		{
			::PropVariantClear(this);
		}

		PropVariant(const PropVariant &) = delete;
		PropVariant(PropVariant &&) = delete;

		PropVariant & operator=(const PropVariant &) = delete;
		PropVariant & operator=(PropVariant &&) = delete;
	};


	//
	// スタートメニューにショートカットを作成または削除する：
	//

	StartMenuShortcut::StartMenuShortcut(std::wstring_view name) : m_name(name)
	{
		if (auto hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE); FAILED(hr))
		{
			throw std::system_error(hr, std::system_category(), "CoInitializeEx");
		}
	}

	StartMenuShortcut::~StartMenuShortcut() noexcept
	{
		::CoUninitialize();
	}

	void StartMenuShortcut::Create(const wchar_t * target, const wchar_t * appId) const
	{
		// 既に存在する場合は何もしない
		auto path = GetPath();

		if (std::filesystem::exists(path))
		{
			return;
		}

		// ショートカットの基本部分を構成する
		com_ptr_t<IShellLinkW> link;

		if (auto hr = link.CreateInstance(CLSID_ShellLink); FAILED(hr))
		{
			throw std::system_error(hr, std::system_category(), "CoCreateInstance(CLSID_ShellLink)");
		}

		if (auto hr = link->SetPath(target); FAILED(hr))
		{
			throw std::system_error(hr, std::system_category(), "IShellLink::SetPath");
		}

		if (auto hr = link->SetIconLocation(L"%SystemRoot%\\System32\\shell32.dll", 220); FAILED(hr))
		{
			throw std::system_error(hr, std::system_category(), "IShellLink::SetIconLocation");
		}

		// AppUserModelID を設定する
		com_ptr_t<IPropertyStore> props;

		if (auto hr = link->QueryInterface(&props); FAILED(hr))
		{
			throw std::system_error(hr, std::system_category(), "IShellLink::QueryInterface(IPropertyStore)");
		}

		PropVariant pv{ appId };

		if (auto hr = props->SetValue(PKEY_AppUserModel_ID, pv); FAILED(hr))
		{
			throw std::system_error(hr, std::system_category(), "IPropertyStore::SetValue(PKEY_AppUserModel_ID)");
		}

		// 構成したショートカットを保存する
		com_ptr_t<IPersistFile> file;

		if (auto hr = link->QueryInterface(&file); FAILED(hr))
		{
			throw std::system_error(hr, std::system_category(), "IShellLink::QueryInterface(IPersistFile)");
		}

		if (auto hr = file->Save(path.c_str(), true); FAILED(hr))
		{
			throw std::system_error(hr, std::system_category(), "IPersistFile::Save");
		}
	}

	void StartMenuShortcut::Remove() const
	{
		std::error_code ec;

		if (!std::filesystem::remove(GetPath(), ec))
		{
			if (ec)
			{
				throw std::system_error(ec, "filesystem::remove");
			}
		}
	}

	template<typename T, typename D>
	inline auto adopt_unique(T * ptr, D del)
	{
		return std::unique_ptr<T, D>(ptr, del);
	}

	std::filesystem::path StartMenuShortcut::GetPath() const
	{
		wchar_t * path{};

		if (auto hr = ::SHGetKnownFolderPath(FOLDERID_StartMenu, KF_FLAG_DEFAULT, nullptr, &path); FAILED(hr))
		{
			throw std::system_error(hr, std::system_category(), "SHGetKnownFolderPath");
		}

		auto owner = adopt_unique(path, &::CoTaskMemFree);

		return std::filesystem::path{ path } / (m_name + L".lnk");
	}
}
