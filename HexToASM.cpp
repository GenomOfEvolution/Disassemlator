#include "HexToASM.h"

std::optional<std::vector<AVMParametr>> LoadAVMCommands()
{
	const std::string AVR_TABLE_NAME = "ASM-AVR-Codes.csv";
	std::ifstream input;
	input.open(AVR_TABLE_NAME);

	if (!input.is_open())
	{
		std::cout << "Can't open csv table!";
		return std::nullopt;
	}

	std::string line;
	std::getline(input, line);

	std::vector<AVMParametr> result;

	while (std::getline(input, line))
	{
		line.erase(remove(line.begin(), line.end(), '\"'), line.end());
		line.erase(remove(line.begin(), line.end(), ' '), line.end());
		std::stringstream args(line);
		AVMParametr AVM;
		std::getline(args, AVM.command, ',');
		std::getline(args, AVM.mask, ',');

		std::string maskHolder;
		std::getline(args, maskHolder, ',');
		AVM.maskAND = std::stoul(maskHolder, 0, 2);

		std::getline(args, maskHolder, ',');
		AVM.maskXOR = std::stoul(maskHolder, 0, 2);

		result.push_back(AVM);
	}

	return result;
}

uint32_t SetMachineWord(const IntelHex& iHex, size_t pos)
{
	return (iHex.dataField[pos + 1] << 8) + (iHex.dataField[pos]);
}

uint32_t MaskToBin(std::string mask)
{
	mask.erase(std::remove_if(mask.begin(), mask.end(), ::isspace),
		mask.end());

	for (size_t i = 0; i < mask.length(); i++)
	{
		if (mask[i] != '0' && mask[i] != '1')
		{
			mask[i] = '0';
		}
	}

	return std::stol(mask, 0, 2);
}

std::optional<AVMParametr> ChooseCommand(uint16_t word, uint32_t wordEx, const std::vector<AVMParametr>& commands, bool& wasExtCommand)
{
	for (size_t pos = 0; pos < commands.size(); pos++)
	{
		if (((word & uint16_t(commands[pos].maskAND)) == commands[pos].maskXOR) && (commands[pos].mask.length() == 16))
		{
			wasExtCommand = false;
			return commands[pos];
		}

		if (((wordEx & commands[pos].maskAND) == commands[pos].maskXOR) && (commands[pos].mask.length() == 32))
		{
			wasExtCommand = true;
			return commands[pos];
		}
	}
	return std::nullopt;
}

void GetParamFromCommand(uint32_t machineWord, const std::string& mask, std::string& paramA, std::string& paramB)
{

	std::string mWord;
	if (mask.length() == 16)
	{
		mWord = std::format("{:016b}", machineWord);
	}
	else
	{
		mWord = std::format("{:032b}", machineWord);
	}
	
	for (size_t i = 0; i < mask.length(); i++)
	{
		if (mask[i] == 'A')
		{
			paramA += mWord[i];
		}

		if (mask[i] == 'B')
		{
			paramB += mWord[i];
		}
	}
}

void UpgradeCommand(const AVMParametr& avmCommand, uint32_t& parA, uint32_t parB)
{
	if (avmCommand.command == "RJMP" || avmCommand.command == "JMP" || avmCommand.command == "CALL")
	{
		parA = parA << 1;
	}

	//TODO: add LDI and other commands if needed
}

std::string WriteCommand(const AVMParametr& avmCommand, const IntelHex& i, size_t pos)
{
	uint16_t address = i.addresStart + uint16_t(pos);
	std::string fullAddress = std::format("{:x}", address) + ": ";

	if (avmCommand.mask.length() == 16)
	{
		fullAddress += std::format("{:x} {:x} ", i.dataField[pos], i.dataField[pos + 1]);
	}
	else
	{
		fullAddress += std::format("{:x} {:x} {:x} {:x} ", i.dataField[pos], i.dataField[pos + 1], i.dataField[pos + 2], i.dataField[pos + 3]);
	}

	fullAddress += avmCommand.command + " ";

	std::string paramA;
	std::string paramB;

	uint16_t word = SetMachineWord(i, pos);
	uint32_t wordExtended = word;
	if (pos + 2 < i.dataField.size())
	{
		wordExtended = (uint32_t(word) << 16) + uint32_t(SetMachineWord(i, pos + 2));
	}
		
	GetParamFromCommand(wordExtended, avmCommand.mask, paramA, paramB);
	
	uint32_t parA = std::stol(paramA, 0, 2);

	//TODO: remove to another function
	if (paramB.empty())
	{
		UpgradeCommand(avmCommand, parA, parA);
		fullAddress += std::format("0x{:x}", parA);
	}
	else
	{
		uint32_t parB = std::stol(paramB, 0, 2);
		UpgradeCommand(avmCommand, parA, parB);
		fullAddress += std::format("0x{:x}, 0x{:x}", parA, parB);
	}

	return fullAddress + "\n";
}

std::string IHexToAVM(const IntelHex& i, const std::vector<AVMParametr>& allAVMCommands)
{
	std::string result;
	for (size_t j = 0; j < i.dataField.size(); j += 2)
	{
		uint16_t word = SetMachineWord(i, j);
		bool wasExtended = false;
		std::optional <AVMParametr> command;
		if (j + 2 < i.dataField.size())
		{
			uint32_t wordExtended = (uint32_t(word) << 16) + uint32_t(SetMachineWord(i, j + 2));
			command = ChooseCommand(word, wordExtended, allAVMCommands, wasExtended);

			if (!command)
			{
				return {};
			}
			else
			{
				result += WriteCommand(*command, i, j);
			}

			if (wasExtended)
			{
				j += 2;
			}
		}
		else
		{
			command = ChooseCommand(word, word, allAVMCommands, wasExtended);
			if (!command)
			{
				return {};
			}
			else
			{
				result += WriteCommand(*command, i, j);
			}
		}
	}
	return result;
}

bool ReplaceIntelHexToAVM(const std::string& outputFileName, const std::vector<AVMParametr>& allAVMCommands, const std::vector<IntelHex>& iHex)
{
	std::ofstream output;
	output.open(outputFileName);

	if (!output.is_open())
	{
		std::cout << "Can't open output file!";
		return false;
	}

	for (IntelHex i : iHex)
	{
		std::string command = IHexToAVM(i, allAVMCommands);
		std::cout << command << "\n";
		output << command << "\n";
	}

	return true;
}