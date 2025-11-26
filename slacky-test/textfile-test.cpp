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

		void SetTestData(std::span<const std::byte> data) const
		{
			// * テストコードでは書き込み失敗時のハンドルのリークを許容する

			HANDLE hFile = ::CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

			if (hFile == INVALID_HANDLE_VALUE)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "CreateFileW");
			}

			auto bytes = static_cast<DWORD>(data.size());

			if (!::WriteFile(hFile, data.data(), bytes, &bytes, nullptr))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WriteFile");
			}

			::CloseHandle(hFile);
		}
	};

	TEST_F(TextFileTest, ReadUtf16LE)
	{
		// UTF-16 LE (detected branch in TextFile implementation) を読み取れること
			// Craft bytes so that first wchar_t value equals 0xFFFE (bytes 0xFE,0xFF),
			// followed by little-endian UTF-16 characters 'A' and 'B' (0x41,0x00, 0x42,0x00).
		auto data = std::vector<std::byte>{ std::byte{0xFE}, std::byte{0xFF}, std::byte{0x41}, std::byte{0x00}, std::byte{0x42}, std::byte{0x00} };
		SetTestData(data);

		TextFile tf(file);
		EXPECT_EQ(tf.ToString(), L"AB");
	}

	// UTF-16 BE (detected branch in TextFile implementation) を読み取れること
	TEST_F(TextFileTest, ReadUtf16BE)
	{
		// Craft bytes so that first wchar_t value equals 0xFEFF (bytes 0xFF,0xFE),
		// followed by big-endian UTF-16 characters for 'A' and 'B' (0x00,0x41, 0x00,0x42).
		auto data = std::vector<std::byte>{ std::byte{0xFF}, std::byte{0xFE}, std::byte{0x00}, std::byte{0x41}, std::byte{0x00}, std::byte{0x42} };
		SetTestData(data);

		TextFile tf(file);
		EXPECT_EQ(tf.ToString(), L"AB");
	}

	// UTF-8 with BOM を読み取れること
	TEST_F(TextFileTest, ReadUtf8WithBOM)
	{
		auto data = std::vector<std::byte>{ std::byte{0xEF}, std::byte{0xBB}, std::byte{0xBF}, std::byte{'A'}, std::byte{'B'} };
		SetTestData(data);

		TextFile tf(file);
		EXPECT_EQ(tf.ToString(), L"AB");
	}

	// ASCII (no BOM) を読み取れること
	TEST_F(TextFileTest, ReadAsciiNoBOM)
	{
		const std::string s = "Hello";
		auto bytes = std::vector<std::byte>(s.size());
		for (size_t i = 0; i < s.size(); ++i)
		{
			bytes[i] = static_cast<std::byte>(s[i]);
		}

		SetTestData(bytes);

		TextFile tf(file);
		EXPECT_EQ(tf.ToString(), L"Hello");
	}
}
