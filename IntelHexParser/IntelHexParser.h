#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include <optional>
#include <algorithm>
#include <string>
#include <regex>

struct IntelHex
{
	uint8_t amountOfData = 0x00;
	uint16_t addresStart = 0x0000;
	uint8_t fieldType = 0x00;
	std::vector<uint8_t> dataField;
	uint8_t controlSum = 0x00;
};

std::optional<IntelHex> ParseLineToIntelHex(std::string str);