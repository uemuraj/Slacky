#include "toast.h"

#include <system_error>

#include <windows.h>
#include <wrl.h>
#include <roapi.h>

#include <winstring.h>
#include <windows.data.xml.dom.h>
#include <windows.ui.notifications.h>

#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "runtimeobject.lib")

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;

namespace slacky
{
	struct RoInitialized
	{
		RoInitialized()
		{
			if (auto hr = ::RoInitialize(RO_INIT_SINGLETHREADED); FAILED(hr))
			{
				throw std::system_error(hr, std::system_category(), "RoInitialize");
			}
		}

		~RoInitialized() noexcept
		{
			::RoUninitialize();
		}
	};

	struct Toast::Impl : RoInitialized
	{
		ComPtr<IToastNotificationManagerStatics> manager;

		HStringReference applicationId;

		Impl(const wchar_t * appId) : applicationId(appId)
		{
			if (auto hr = ::RoGetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), IID_PPV_ARGS(&manager)); hr != S_OK)
			{
				throw std::system_error(hr, std::system_category(), "RoGetActivationFactory(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager)");
			}
		}

		~Impl() noexcept = default;

		void Show(ComPtr<IXmlDocument> content)
		{
			ComPtr<IToastNotificationFactory> factory;

			if (auto hr = ::RoGetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), IID_PPV_ARGS(&factory)); hr != S_OK)
			{
				throw std::system_error(hr, std::system_category(), "RoGetActivationFactory(RuntimeClass_Windows_UI_Notifications_ToastNotification)");
			}

			ComPtr<IToastNotification> toast;

			if (auto hr = factory->CreateToastNotification(content.Get(), &toast); FAILED(hr))
			{
				throw std::system_error(hr, std::system_category(), "IToastNotificationFactory::CreateToastNotification");
			}

			ComPtr<IToastNotifier> notifier;

			if (auto hr = manager->CreateToastNotifierWithId(applicationId.Get(), &notifier); FAILED(hr))
			{
				throw std::system_error(hr, std::system_category(), "IToastNotificationManagerStatics::CreateToastNotifier");
			}

			if (auto hr = notifier->Show(toast.Get()); FAILED(hr))
			{
				throw std::system_error(hr, std::system_category(), "IToastNotifier::Show");
			}
		}

		decltype(auto) GetTemplate(ToastTemplateType type)
		{
			ComPtr<IXmlDocument> result;

			if (auto hr = manager->GetTemplateContent(type, &result); FAILED(hr))
			{
				throw std::system_error(hr, std::system_category(), "IToastNotificationManagerStatics::GetTemplateContent");
			}

			// TODO: 取得した XML のテキスト表記をデバッグ出力する

			return result;
		}

		void SetTextContent(ComPtr<IXmlDocument> xml, unsigned index, const std::wstring & text)
		{
			ComPtr<IXmlNodeList> nodeList;
			if (auto hr = xml->GetElementsByTagName(HStringReference(L"text").Get(), &nodeList); FAILED(hr))
			{
				throw std::system_error(hr, std::system_category(), "XmlDocument::GetElementsByTagName");
			}

			ComPtr<IXmlNode> textNode;
			if (auto hr = nodeList->Item(index, &textNode); FAILED(hr))
			{
				throw std::system_error(hr, std::system_category(), "XmlNodeList::Item");
			}

			ComPtr<IXmlText> newText;
			if (auto hr = xml->CreateTextNode(HStringReference(text.c_str()).Get(), &newText); FAILED(hr))
			{
				throw std::system_error(hr, std::system_category(), "XmlDocument::CreateTextNode");
			}

			ComPtr<IXmlNode> newNode;
			if (auto hr = newText.As(&newNode); FAILED(hr))
			{
				throw std::system_error(hr, std::system_category(), "XmlText::As");
			}

			ComPtr<IXmlNode> appended;
			if (auto hr = textNode->AppendChild(newNode.Get(), &appended); FAILED(hr))
			{
				throw std::system_error(hr, std::system_category(), "XmlNode::AppendChild");
			}
		}

		decltype(auto) GetToastContent(const std::filesystem::path & icon, const std::wstring & title, const std::wstring & message)
		{
			auto xml = GetTemplate(ToastTemplateType_ToastText02);
			SetTextContent(xml, 0, title);
			SetTextContent(xml, 1, message);
			// TODO: テキストを設定したあとの XML のテキスト表記をデバッグ出力する
			return xml;
		}
	};

	Toast::Toast(const wchar_t * appId) : m_impl(std::make_unique<Impl>(appId))
	{}

	Toast::~Toast() noexcept = default;

	void Toast::Show(const std::filesystem::path & icon, const std::wstring & title, const std::wstring & message)
	{
		m_impl->Show(m_impl->GetToastContent(icon, title, message));
	}
}
