#pragma once
#include "IntelHexParser.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <optional>
#include <algorithm>
#include <string>
#include <format>

struct AVMParametr
{
	std::string command;
	std::string mask;
	uint32_t maskAND = 0x00000000;
	uint32_t maskXOR = 0x00000000;
};

std::optional<std::vector<AVMParametr>> LoadAVMCommands();
bool ReplaceIntelHexToAVM(const std::string& outputFileName, const std::vector<AVMParametr>& allAVMCommands, const std::vector<IntelHex>& iHex);