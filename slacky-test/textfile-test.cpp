#include "pch.h"

#include <slacky/textfile.h>
#include <slacky/convert.h>

#include <filesystem>
#include <system_error>
#include <span>
#include <Windows.h>
#include <vector>

namespace slacky
{
	struct TextFileTest : public ::testing::Test
	{
		std::filesystem::path file;

		void SetUp() override
		{
			file = std::filesystem::temp_directory_path() / (TestName() + ".txt");
		}

		void TearDown() override
		{
			std::error_code ec;

			if (!std::filesystem::remove(file, ec))
			{
				if (ec)
				{
					std::cerr << "Failed to remove Test file: " << ec.message() << std::endl;
				}
			}
		}

		std::string TestName() const
		{
			const ::testing::TestInfo * info = ::testing::UnitTest::GetInstance()->current_test_info();
			return info->name();
		}

		// 文字列リテラルをテストデータとして書き込む（終端のヌル文字は除く）
		template <class CharT, std::size_t N>
		void SetTestData(const CharT(&arr)[N]) const
		{
			WriteTestData(arr, (N - 1) * sizeof(CharT));
		}

		template <class CharT>
		void SetTestData(const std::basic_string<CharT> & str) const
		{
			WriteTestData(str.data(), str.size() * sizeof(CharT));
		}

		void WriteTestData(const void * data, size_t size) const
		{
			// * テストコードでは書き込み失敗時のハンドルのリークを許容する

			HANDLE hFile = ::CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

			if (hFile == INVALID_HANDLE_VALUE)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "CreateFileW");
			}

			DWORD written = 0;

			if (!::WriteFile(hFile, data, (DWORD) size, &written, nullptr))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WriteFile");
			}

			::CloseHandle(hFile);
		}
	};

	TEST_F(TextFileTest, ReadUtf16LE)
	{
		// UTF-16 LE BOM 付きのテキストファイルから読み取れること
		SetTestData(L"\xFEFFHello, World! こんにちは");

		auto text = TextFile(file).ToString();
		EXPECT_STREQ(text.c_str(), L"Hello, World! こんにちは");
	}

	TEST_F(TextFileTest, ReadUtf16BE)
	{
		// UTF-16 BE BOM 付きのテキストファイルから読み取れること
		SetTestData(ConvertUTF16Endian(L"\xFEFFHello, World! こんにちは"));

		auto text = TextFile(file).ToString();
		EXPECT_STREQ(text.c_str(), L"Hello, World! こんにちは");
	}

	TEST_F(TextFileTest, ReadUtf8WithBOM)
	{
		// UTF-8 BOM 付きのテキストファイルから読み取れること
		auto utf8 = ConvertFrom(L"Hello, World! こんにちは");
		utf8.insert(utf8.begin(), { 0xEF, 0xBB, 0xBF }); // BOM を先頭に追加
		SetTestData(utf8);

		auto text = TextFile(file).ToString();
		EXPECT_STREQ(text.c_str(), L"Hello, World! こんにちは");
	}

	TEST_F(TextFileTest, ReadShiftJIS)
	{
		// Shift_JIS のテキストファイルから読み取れること
		auto sjis = Narrow(L"Hello, World! こんにちは");
		SetTestData(sjis);

		auto text = TextFile(file).ToString();
		EXPECT_STREQ(text.c_str(), L"Hello, World! こんにちは");
	}
}
