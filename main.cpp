#include "emulator.h"
#include "cpu.h"
#include "display.h"

int main(int argc, char *args[])
{
	Emulator emulator;

	string name = "11-op a,(hl)";

	//emulator.memory.load_rom("tests/" + name + ".gb");
	emulator.memory.load_rom("tetris.gb");

	//emulator.cpu.debug();
	emulator.run();

	return 0;
}