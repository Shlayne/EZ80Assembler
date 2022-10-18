#include "EZ80Assembler.h"
#include "Debug.h"

int main(int argc, char** argv)
{
	ez80::AssemblerInfo info;
	info.inputFilepath = L"C:\\Workspace\\Programming\\Dev\\C++\\EZ80Assembler\\EZ80Assembler\\test\\test.asm";
	info.outputFilepath = L"C:\\Workspace\\Programming\\Dev\\C++\\EZ80Assembler\\EZ80Assembler\\test\\TEST.8xp";
	if (auto result = ez80::Assemble(info))
	{
		stdcout(result.error << '\n');
		__debugbreak();
	}

	return 0;
}
