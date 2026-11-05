#pragma once
#include <string>
#include <string_view>
namespace GameLogic::RedeemCodes { bool Send(std::wstring_view code, std::wstring& error); }
