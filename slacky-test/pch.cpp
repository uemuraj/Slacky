//
// pch.cpp
//

#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include <locale>

struct WindowsEnvironment : public ::testing::Environment
{
	void SetUp() override
	{
		std::locale::global(std::locale(""));
	}

	void TearDown() override
	{
		// nothing to do
	}
};

int main(int argc, char ** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(new WindowsEnvironment);
	return RUN_ALL_TESTS();
}
