#include "toast.h"

#include <system_error>

#include <windows.h>
#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>
#include <roapi.h>
#include <winstring.h>
#include <windows.ui.notifications.h>
#include <windows.data.xml.dom.h>

#pragma comment(lib, "runtimeobject.lib")
#pragma comment(lib, "windowsapp.lib")

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;

namespace slacky
{
    // Pimpl definition
    struct Toast::Impl
    {
        Impl()
        {
            if (auto hr = ::RoInitialize(RO_INIT_SINGLETHREADED); FAILED(hr))
            {
                throw std::system_error(hr, std::system_category(), "RoInitialize");
            }
        }

        ~Impl() noexcept
        {
            ::RoUninitialize();
        }

        void Show(const std::filesystem::path & /*icon*/, const std::wstring & title, const std::wstring & message)
        {
            // Obtain the ToastNotificationManager activation factory
            ComPtr<IToastNotificationManagerStatics> toastManager;
            if (auto hr = ::RoGetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), IID_PPV_ARGS(&toastManager)); FAILED(hr))
            {
                throw std::system_error(hr, std::system_category(), "RoGetActivationFactory(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager)");
            }

            // https://learn.microsoft.com/en-us/uwp/api/windows.ui.notifications.toasttemplatetype?view=winrt-26100
            ComPtr<IXmlDocument> xmlDoc;
            if (auto hr = toastManager->GetTemplateContent(ToastTemplateType_ToastText02, &xmlDoc); FAILED(hr) || !xmlDoc)
            {
                throw std::system_error(hr, std::system_category(), "IToastNotificationManagerStatics::GetTemplateContent");
            }

            // Fill in the text elements
            ComPtr<IXmlNodeList> nodeList;
            if (auto hr = xmlDoc->GetElementsByTagName(HStringReference(L"text").Get(), &nodeList); FAILED(hr) || !nodeList)
            {
                throw std::system_error(hr, std::system_category(), "XmlDocument::GetElementsByTagName");
            }

            // Helper to set text at index
            auto setTextAt = [&](unsigned index, const std::wstring & text) -> void
            {
                ComPtr<IXmlNode> textNode;
                if (auto hr = nodeList->Item(index, &textNode); FAILED(hr) || !textNode)
                {
                    throw std::system_error(hr, std::system_category(), "XmlNodeList::Item");
                }

                // Create an IXmlText, then obtain an IXmlNode from it for AppendChild
                ComPtr<IXmlText> newText;
                if (auto hr = xmlDoc->CreateTextNode(HStringReference(text.c_str()).Get(), &newText); FAILED(hr) || !newText)
                {
                    throw std::system_error(hr, std::system_category(), "XmlDocument::CreateTextNode");
                }

                ComPtr<IXmlNode> newNode;
                if (auto hr = newText.As(&newNode); FAILED(hr) || !newNode)
                {
                    throw std::system_error(hr, std::system_category(), "XmlText::As");
                }

                ComPtr<IXmlNode> appended;
                if (auto hr = textNode->AppendChild(newNode.Get(), &appended); FAILED(hr))
                {
                    throw std::system_error(hr, std::system_category(), "XmlNode::AppendChild");
                }
            };

            setTextAt(0, title);
            setTextAt(1, message);

            // Create the ToastNotification from the XML
            ComPtr<IToastNotificationFactory> toastFactory;
            if (auto hr = ::RoGetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), IID_PPV_ARGS(&toastFactory)); FAILED(hr) || !toastFactory)
            {
                throw std::system_error(hr, std::system_category(), "RoGetActivationFactory(RuntimeClass_Windows_UI_Notifications_ToastNotification)");
            }

            ComPtr<IToastNotification> toast;
            if (auto hr = toastFactory->CreateToastNotification(xmlDoc.Get(), &toast); FAILED(hr) || !toast)
            {
                throw std::system_error(hr, std::system_category(), "IToastNotificationFactory::CreateToastNotification");
            }

            // Obtain a notifier and show the toast
            ComPtr<IToastNotifier> notifier;
            if (auto hr = toastManager->CreateToastNotifier(&notifier); FAILED(hr) || !notifier)
            {
                throw std::system_error(hr, std::system_category(), "IToastNotificationManagerStatics::CreateToastNotifier");
            }

            if (auto hr = notifier->Show(toast.Get()); FAILED(hr))
            {
                throw std::system_error(hr, std::system_category(), "IToastNotifier::Show");
            }
        }
    };

    Toast::Toast()
        : impl(std::make_unique<Impl>())
    {
    }

    Toast::~Toast() noexcept = default;

    void Toast::Show(const std::filesystem::path & icon, const std::wstring & title, const std::wstring & message)
    {
        impl->Show(icon, title, message);
    }
}
