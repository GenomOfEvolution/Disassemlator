#include <iostream>
#include "IntelHexParser.h"

int main(int argc, char* argv[])
{
	ParseLineToIntelHex(":10007000DEBFCDBF0E9440000C9452000C940000E3");

	return EXIT_SUCCESS;
}