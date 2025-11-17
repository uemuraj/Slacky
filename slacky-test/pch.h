//
// pch.h
//

#pragma once

#include "gtest/gtest.h"
#include <ostream>
#include <string>
#include <string_view>

void PrintTo(std::wstring_view str, std::ostream & os);
