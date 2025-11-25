#include <windows.h>
#include <shellapi.h>
#include <shlobj_core.h>

#include <locale>
#include <string>
#include <format>
#include <filesystem>
#include <system_error>

#include "slacky.h"
#include "options.h"
#include "shortcut.h"
#include "toast.h"
#include "convert.h"

#undef GetMessage

_Use_decl_annotations_
int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrev*/, PWSTR /*pCmdLine*/, int /*nCmdShow*/)
{
	using namespace slacky;

	try
	{
		std::locale::global(std::locale(""));

		Options options;

		auto aumid = options.GetAppUserModelID();
		auto target = options.GetExecutablePath();

		{
			StartMenuShortcut shortcut(VS_SOLUTION);
			shortcut.Create(target.c_str(), aumid.c_str());
		}

		if (::SetCurrentProcessExplicitAppUserModelID(aumid.c_str()) != S_OK)
		{
			throw std::system_error(::GetLastError(), std::system_category(),
				std::format("SetCurrentProcessExplicitAppUserModelID({})", Narrow(aumid)));
		}

		SlackBot bot(options.GetToken());

		auto channel = options.GetChannel();
		auto message = options.GetMessage();

		if (bot.Post(channel, message))
		{
			auto & icon = bot.Icon();
			auto & name = bot.Name();

			Toast toast(aumid.c_str());
			toast.Show(icon, name, message);
			return 0;
		}

		return 1;
	}
	catch (const std::exception & e)
	{
		::MessageBoxA(nullptr, e.what(), VS_SOLUTIONA, MB_ICONERROR);
	}

	return 2;
}
