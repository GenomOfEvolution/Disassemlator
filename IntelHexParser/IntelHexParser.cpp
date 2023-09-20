#include "IntelHexParser.h"

bool IsCorrectData(const std::string& intelHex)
{
	uint8_t sum = 0;

	for (size_t i = 0; i < (intelHex.length() - 1) / 2; i++)
	{
		std::string dataByte;
		std::copy(intelHex.begin() + i * 2 + 1, intelHex.begin() + (i + 1) * 2 + 1, std::back_inserter(dataByte));
		sum += std::stoi(dataByte, 0, 16);
	}

	sum = ~sum + 1;

	return sum == 0x00 ? true : false;
}

std::optional<IntelHex> ParseLineToIntelHex(std::string str)
{
	IntelHex result;

	str.erase(std::remove_if(str.begin(), str.end(), ::isspace),
		str.end());

	std::transform(str.begin(), str.end(), str.begin(),
		[](unsigned char c) { return std::tolower(c); });

	std::regex intelHexRegex(":([0-9a-f]{2})([0-9a-f]{4})([0-9a-f]{2})([0-9a-f]*)([0-9a-f]{2}$)");
	std::smatch intelHexMatch;

	if (!std::regex_match(str, intelHexMatch, intelHexRegex))
	{
		std::cout << "Wrong Intel Hex structure!" << std::endl;
		return std::nullopt;
	}

	//:LLAAAATTDD…CC
	std::string LL = intelHexMatch[1].str();	//byte amount
	std::string AAAA = intelHexMatch[2].str();	//start addres
	std::string TT = intelHexMatch[3].str();	//field type
	std::string DD = intelHexMatch[4].str();	//data
	std::string CC = intelHexMatch[5].str();	//control sum

	result.amountOfData = std::stoi(LL, 0, 16);
	result.addresStart = std::stoi(AAAA, 0, 16);
	result.fieldType = std::stoi(TT, 0, 16);
	result.controlSum = std::stoi(CC, 0, 16);

	if (DD.length() / 2 != result.amountOfData)
	{
		std::cout << "Amount of bits isn't equal!" << std::endl;
		return std::nullopt;
	}

	if (!DD.empty())
	{
		result.dataField.resize(result.amountOfData);
		for (size_t i = 0; i < result.amountOfData; i++)
		{
			std::string dataByte;
			std::copy(DD.begin() + i * 2, DD.begin() + (i + 1) * 2, std::back_inserter(dataByte));
			result.dataField[i] = std::stoi(dataByte, 0, 16);
		}
	}

	if (!IsCorrectData(str))
	{
		return std::nullopt;
	}
	
	return result;
}