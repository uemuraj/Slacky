#pragma once

#include <memory>
#include <string>
#include <filesystem>

namespace slacky
{
	class ViewOfFile;

	// テキストファイル全体をそのまま読み込む：
	//
	// * 指定されたパスのテキストファイルをすべて読み込み、その内容を文字列として取得できる。
	// * BOM の判別と文字コードの変換も行う。
	//
	class TextFile
	{
		std::unique_ptr<ViewOfFile> m_file;

	public:
		TextFile(const std::filesystem::path & path);
		~TextFile() noexcept;

		std::wstring ToString() const;
	};
}
