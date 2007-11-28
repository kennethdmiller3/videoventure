// Hash.cpp : Defines the entry point for the console application.
//

#include "..\Source\Hash.h"
#include <stdio.h>

int main(int argc, const char* argv[])
{
	for (int i = 1; i < argc; i++)
	{
		printf("0x%08x /* \"%s\" */\n", Hash(argv[i]), argv[i]);
	}
	return 0;
}

