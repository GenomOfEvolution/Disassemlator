#include <iostream>
#include <fstream>
#include "IntelHexParser.h"
#include "HexToASM.h"

struct Args
{
	std::string inputFileName;
	std::string outputFileName;
};

std::optional<Args>ParseArgs(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cout << "Wrong parametrs, use Disassemlator.exe <Input file> instead!\n";
		return std::nullopt;
	}

	Args result;
	result.inputFileName = argv[1];
	result.outputFileName = argv[2];

	return result;
}

std::optional<std::vector<std::string>>GetAllIntelHexLines(const Args& input)
{
	const char INTELHEX_ID = ':';

	std::ifstream inputFile;
	inputFile.open(input.inputFileName);
	
	if (!inputFile.is_open())
	{
		std::cout << "Can't open input file!";
		return std::nullopt;
	}

	std::string intelHexLine;
	std::vector<std::string> result;
	while (std::getline(inputFile, intelHexLine, INTELHEX_ID))
	{
		if (intelHexLine.length() > 1)
		{
			result.push_back(INTELHEX_ID + intelHexLine);
		}
	}

	return result;
}

std::vector<IntelHex>ParseAllLines(const std::vector<std::string>& lines)
{
	std::vector<IntelHex> result;
	std::optional<IntelHex> parsedIntelHex;
	for (std::string i : lines)
	{
		parsedIntelHex = ParseLineToIntelHex(i);
		if (!parsedIntelHex)
		{
			return {};
		}
		else
		{
			result.push_back(*parsedIntelHex);
		}
	}
	return result;
}

int main(int argc, char* argv[])
{
	std::optional<Args> args;
	args = ParseArgs(argc, argv);

	if (!args)
	{
		return EXIT_FAILURE;
	}

	Args parsedArgs = *args;

	std::optional<std::vector<std::string>>IntelHexLines = GetAllIntelHexLines(parsedArgs);
	if (!IntelHexLines)
	{
		return EXIT_FAILURE;
	}

	std::vector<IntelHex> parsedIntelHex = ParseAllLines(*IntelHexLines);
	if (parsedIntelHex.empty())
	{
		return EXIT_FAILURE;
	}

	std::optional<std::vector<AVMParametr>>  AllAVMCommands = LoadAVMCommands();
	if (!AllAVMCommands)
	{
		return EXIT_FAILURE;
	}

	ReplaceIntelHexToAVM(parsedArgs.outputFileName, *AllAVMCommands, parsedIntelHex);

	return EXIT_SUCCESS;
}