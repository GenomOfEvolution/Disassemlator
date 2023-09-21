#include "HexToASM.h"

std::optional<std::vector<AVMParametr>> LoadAVMCommands()
{
	const std::string AVR_TABLE_NAME = "ASM-AVR-Codes.txt";
	std::ifstream input;
	input.open(AVR_TABLE_NAME);

	if (!input.is_open())
	{
		std::cout << "Can't open codes table!";
		return std::nullopt;
	}

	std::string line;
	std::getline(input, line);

	std::vector<AVMParametr> result;

	const char DELIMETR = ';';
	while (std::getline(input, line))
	{
		line.erase(remove(line.begin(), line.end(), '\"'), line.end());
		line.erase(remove(line.begin(), line.end(), ' '), line.end());
		std::stringstream args(line);
		AVMParametr AVM;
		std::getline(args, AVM.command, DELIMETR);
		std::getline(args, AVM.p1, DELIMETR);
		std::getline(args, AVM.p2, DELIMETR);
		std::getline(args, AVM.nameP1, DELIMETR);
		std::getline(args, AVM.nameP2, DELIMETR);
		std::getline(args, AVM.mask, DELIMETR);

		std::string maskHolder;
		std::getline(args, maskHolder, DELIMETR);
		AVM.maskAND = std::stoul(maskHolder, 0, 2);

		std::getline(args, maskHolder, DELIMETR);
		AVM.maskXOR = std::stoul(maskHolder, 0, 2);

		result.push_back(AVM);
	}

	return result;
}

uint32_t SetMachineWord(const IntelHex& iHex, size_t pos)
{
	return (iHex.dataField[pos + 1] << 8) + (iHex.dataField[pos]);
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

void GetParamFromCommand(uint32_t machineWord, const AVMParametr& command, std::string& firstParam, std::string& secondParam)
{
	std::string mWord;
	if (command.mask.length() == 16)
	{
		mWord = std::format("{:016b}", machineWord);
	}
	else
	{
		mWord = std::format("{:032b}", machineWord);
	}
	
	for (size_t i = 0; i < command.mask.length(); i++)
	{
		if (command.mask[i] == command.nameP1[0])
		{
			firstParam += mWord[i];
		}

		if (command.mask[i] == command.nameP2[0])
		{
			secondParam += mWord[i];
		}
	}
}

std::string ToSigned(size_t size, uint32_t mWord)
{
	std::string newStr = std::format("{:b}", mWord);
	if (newStr.length() < 4)
	{
		newStr = std::string(4 - std::min(size_t(4), newStr.length()), '0') + newStr;
	}
	bool isNegative = (newStr[0] == '1');
	
	if (isNegative)
	{
		for (size_t i = 0; i < newStr.length(); i++)
		{
			if (newStr[i] == '0')
			{
				newStr[i] = '1';
			}
			else
			{
				newStr[i] = '0';
			}
		}
		return ".-" + std::to_string((std::stol(newStr, 0, 2) + 1) << 1);
	}
	else
	{
		return ".+" + std::to_string((std::stol(newStr, 0, 2))  << 1);
	}

	return{};
}

std::string ModifyParametr(std::string param, std::string modifiers, std::string commandName, bool firstParam)
{
	if (!param.empty())
	{
		uint32_t mWord = std::stol(param, 0, 2);

		for (char i : modifiers)
		{
			switch (i)
			{
			case 'a':
				mWord += 16;
				break;
			case 'b':
				mWord += 24 + 2 * mWord;
				break;
			case 'm':
				if (param.length() <= 8)
				{
					return ToSigned(8, mWord);
				}

				if ((param.length() > 8) && (param.length() <= 16))
				{
					return ToSigned(16, mWord);
				}
				
				if ((param.length() > 16) && (param.length() <= 32))
				{
					return ToSigned(32, mWord);
				}
				break;
			case 's':
				mWord = mWord << 1;
				break;
			}
		}

		if (commandName == "ldi" && firstParam)
		{
			return "r" + std::to_string(mWord);
		}

		if (commandName == "out" && !firstParam)
		{
			return "r" + std::to_string(mWord);
		}

		if (commandName == "eor")
		{
			return "r" + std::to_string(mWord);
		}

		if ((commandName == "subi" || commandName == "sbci") && firstParam)
		{
			return "r" + std::to_string(mWord);
		}

		return std::format("0x{:x}", mWord);
	}
	
	return {};
}

std::string PrintFormatedHex(uint8_t byte)
{
	std::string hexed = std::format("{:x}", byte);
	return hexed.length() < 2 ? "0" + hexed + " " : hexed + " ";
}

std::string WriteCommand(const AVMParametr& avmCommand, const IntelHex& i, size_t pos)
{
	uint16_t address = i.addresStart + uint16_t(pos);
	std::string fullAddress = std::format("{:x}", address) + ": ";

	if (avmCommand.mask.length() == 16)
	{
		fullAddress += PrintFormatedHex(i.dataField[pos]) + PrintFormatedHex(i.dataField[pos + 1]);
	}
	else
	{
		fullAddress += PrintFormatedHex(i.dataField[pos]) + PrintFormatedHex(i.dataField[pos + 1]) + PrintFormatedHex(i.dataField[pos + 2]) + PrintFormatedHex(i.dataField[pos + 3]);;
	}

	if (avmCommand.mask.length() == 16)
	{
		fullAddress += "\t" + avmCommand.command + "\t";
	}
	else
	{
		fullAddress += avmCommand.command + "\t";
	}
	

	std::string paramA;
	std::string paramB;

	uint16_t word = SetMachineWord(i, pos);
	uint32_t wordExtended = word;
	if (pos + 2 < i.dataField.size())
	{
		wordExtended = (uint32_t(word) << 16) + uint32_t(SetMachineWord(i, pos + 2));
	}
	
	if (avmCommand.mask.length() == 16)
	{
		GetParamFromCommand(word, avmCommand, paramA, paramB);
	}
	else if (avmCommand.mask.length() == 32)
	{
		GetParamFromCommand(wordExtended, avmCommand, paramA, paramB);
	}

	paramA = ModifyParametr(paramA, avmCommand.p1, avmCommand.command, true);
	paramB = ModifyParametr(paramB, avmCommand.p2, avmCommand.command, false);

	if (paramB.empty())
	{
		return fullAddress + paramA + " ; " + "\n";
	}
	else
	{
		return fullAddress + paramA + ", " + paramB + " ; " + "\n";
	}
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
				std::cout << "No such command\n";
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
				std::cout << "No such command\n";
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