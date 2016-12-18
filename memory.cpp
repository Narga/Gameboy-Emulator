#include "memory.h"

Memory::Memory()
{
	WRAM = vector<Byte>(0x2000); // $C000 - $DFFF, 8kB Working RAM
	ERAM = vector<Byte>(0x2000); // $A000 - $BFFF, 8kB switchable RAM bank, size liable to change in future
	ZRAM = vector<Byte>(0x0100); // $FF00 - $FFFF, 256 bytes of RAM
	VRAM = vector<Byte>(0x2000); // $8000 - $9FFF, 8kB Video RAM
	OAM  = vector<Byte>(0x0100); // $FE00 - $FEFF, OAM Sprite RAM, IO RAM

	// Initialize Memory Register objects for easy reference
	P1   = MemoryRegister(&ZRAM[0x00]);
	DIV  = MemoryRegister(&ZRAM[0x04]);
	TIMA = MemoryRegister(&ZRAM[0x05]);
	TMA  = MemoryRegister(&ZRAM[0x06]);
	TAC  = MemoryRegister(&ZRAM[0x07]);
	LCDC = MemoryRegister(&ZRAM[0x40]);
	STAT = MemoryRegister(&ZRAM[0x41]);
	SCY  = MemoryRegister(&ZRAM[0x42]);
	SCX  = MemoryRegister(&ZRAM[0x43]);
	LY   = MemoryRegister(&ZRAM[0x44]);
	LYC  = MemoryRegister(&ZRAM[0x45]);
	DMA  = MemoryRegister(&ZRAM[0x46]);
	BGP  = MemoryRegister(&ZRAM[0x47]);
	OBP0 = MemoryRegister(&ZRAM[0x48]);
	OBP1 = MemoryRegister(&ZRAM[0x49]);
	WY   = MemoryRegister(&ZRAM[0x4A]);
	WX   = MemoryRegister(&ZRAM[0x4B]);
	IF   = MemoryRegister(&ZRAM[0x0F]);
	IE   = MemoryRegister(&ZRAM[0xFF]);

	reset();
}

void Memory::reset()
{
	fill(WRAM.begin(), WRAM.end(), 0);
	fill(ERAM.begin(), ERAM.end(), 0);
	fill(ZRAM.begin(), ZRAM.end(), 0);
	fill(VRAM.begin(), VRAM.end(), 0);
	fill(OAM.begin(), OAM.end(), 0);

	// The following memory locations are set to the following values after gameboy BIOS runs
	P1.set(0x00);
	DIV.set(0x00);
	TIMA.set(0x00);
	TMA.set(0x00);
	TAC.set(0x00);
	LCDC.set(0x83);
	SCY.set(0x00);
	SCX.set(0x00);
	LYC.set(0x00);
	BGP.set(0xFC);
	OBP0.set(0xFF);
	OBP1.set(0xFF);
	WY.set(0x00);
	WX.set(0x00);
	IF.set(0x00);
	IE.set(0x00);

	// Initialize input to HIGH state (unpressed)
	joypad_buttons = 0xF;
	joypad_arrows  = 0xF;
}

void Memory::load_rom(std::string location)
{
	ifstream input(location, ios::binary);
	vector<Byte> buffer((istreambuf_iterator<char>(input)), (istreambuf_iterator<char>()));
	CART_ROM = buffer;

	// print cartrige data
	string title = "";

	for (int i = 0x0134; i <= 0x142; i++)
	{
		Byte character = read(i);
		if (character == 0)
			break;
		else
			title.push_back(character);
	}

	cout << "Title: " << title << endl;
	Byte gb_type = read(0x0143);
	cout << "Gameboy Type: " << ((gb_type == 0x80) ? "GB Color" : "GB") << endl;
	Byte functions = read(0x0146);
	cout << "Use " << ((functions == 0x3) ? "Super " : "") << "Gameboy functions" << endl;

	string cart_types[0x100];
	cart_types[0x0] = "ROM ONLY";
	cart_types[0x1] = "ROM+MBC1";
	cart_types[0x2] = "ROM+MBC1+RAM";
	cart_types[0x3] = "ROM+MBC1+RAM+BATT";
	cart_types[0x5] = "ROM+MBC2";
	cart_types[0x6] = "ROM+MBC2+BATTERY";
	cart_types[0x8] = "ROM+RAM";
	cart_types[0x9] = "ROM+RAM+BATTERY";
	cart_types[0xB] = "ROM+MMM01";
	cart_types[0xC] = "ROM+MMM01+SRAM";
	cart_types[0xD] = "ROM+MMM01+SRAM+BATT";
	cart_types[0xF] = "ROM+MBC3+TIMER+BATT";
	cart_types[0x10] = "ROM+MBC3+TIMER+RAM+BATT";
	cart_types[0x11] = "ROM+MBC3";
	cart_types[0x12] = "ROM+MBC3+RAM";
	cart_types[0x13] = "ROM+MBC3+RAM+BATT";
	cart_types[0x19] = "ROM+MBC5";
	cart_types[0x1A] = "ROM+MBC5+RAM";
	cart_types[0x1B] = "ROM+MBC5+RAM+BATT";
	cart_types[0x1C] = "ROM+MBC5+RUMBLE";
	cart_types[0x1D] = "ROM+MBC5+RUMBLE+SRAM";
	cart_types[0x1E] = "ROM+MBC5+RUMBLE+SRAM+BATT";
	cart_types[0x1F] = "Pocket Camera";
	cart_types[0xFD] = "Bandai TAMA5";
	cart_types[0xFE] = "Hudson HuC-3";
	cart_types[0xFF] = "Hudson HuC-1";

	Byte cart = read(0x0147);
	cout << "Cartridge Type: " << cart_types[cart] << endl;
	Byte rsize = read(0x0148);
	cout << "ROM Size: " << (32 * (pow(2, rsize))) << "kB " << pow(2, rsize + 1) << " banks" << endl;
	int size, banks;
	switch (read(0x149))
	{
		case 1: size = 2; banks = 1;
		case 2: size = 8; banks = 1;
		case 3: size = 32; banks = 4;
		case 4: size = 128; banks = 16;
		default: size = 0; banks = 0;
	}
	cout << "RAM Size: " << size << "kB " << banks << " banks" << endl;
	cout << "Destination Code: " << (read(0x014A) == 1 ? "Non-" : "") << "Japanese" << endl;
}

void Memory::do_dma_transfer()
{
	Byte_2 address = DMA.get() << 8; // multiply by 100

	for (int i = 0; i < 0xA0; i++)
	{
		write((0xFE00 + i), read(address + i));
	}
}

Byte Memory::get_joypad_state()
{
	Byte request = P1.get();

	switch (request)
	{
		case 0x10:
			//cout << "read buttons" << endl;
			//cout << "A is " << (is_bit_set(joypad_buttons, BIT_0) ? "SET" : "UNSET") << endl;
			return joypad_buttons;
		case 0x20:
			//cout << "read direction" << endl;
			return joypad_arrows;
		default:
			//cout << "SOMETHING ELSE" << endl;
			return 0xE;
	}
}

Byte Memory::read(Address location)
{
	switch (location & 0xF000)
	{
	// ROM0
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
		return CART_ROM[location];

	// ROM1 (no bank switching)
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
		return CART_ROM[location];

	// Graphics VRAM
	case 0x8000:
	case 0x9000:
		return VRAM[location & 0x1FFF];

	// External RAM
	case 0xA000:
	case 0xB000:
		return ERAM[location & 0x1FFF];

	// Working RAM (8kB) and RAM Shadow
	case 0xC000:
	case 0xD000:
	case 0xE000:
		return WRAM[location & 0x1FFF];

	// Remaining Working RAM Shadow, I/O, Zero page RAM
	case 0xF000:
		switch (location & 0x0F00)
		{
			// Remaining Working RAM
			case 0x000: case 0x100: case 0x200: case 0x300:
			case 0x400: case 0x500: case 0x600: case 0x700:
			case 0x800: case 0x900: case 0xA00: case 0xB00:
			case 0xC00: case 0xD00:
				return WRAM[location & 0x1FFF];

			// Sprite OAM
			case 0xE00:
					return OAM[location & 0xFF];

			case 0xF00:
				if (location == 0xFF00)
					return get_joypad_state();
				else
					return ZRAM[location & 0xFF];
		}
	default:
		return 0;
	}
}

void Memory::write(Address location, Byte data)
{
	switch (location & 0xF000)
	{
	// ROM0
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
		// CART_ROM[location] = data; - READ ONLY
		break;

	// ROM1 (no bank switching)
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
		// CART_ROM[location] = data; - READ ONLY
		break;

	// Graphics VRAM
	case 0x8000:
	case 0x9000:
		VRAM[location & 0x1FFF] = data;
		break;

	// External RAM
	case 0xA000:
	case 0xB000:
		ERAM[location & 0x1FFF] = data;
		break;

	// Working RAM (8kB) and RAM Shadow
	case 0xC000:
	case 0xD000:
	case 0xE000:
		WRAM[location & 0x1FFF] = data;
		break;

	// Remaining Working RAM Shadow, I/O, Zero page RAM
	case 0xF000:
		switch (location & 0x0F00)
		{
		// Remaining Working RAM
		case 0x000: case 0x100: case 0x200: case 0x300:
		case 0x400: case 0x500: case 0x600: case 0x700:
		case 0x800: case 0x900: case 0xA00: case 0xB00:
		case 0xC00: case 0xD00:
			WRAM[location & 0x1FFF] = data;
			break;

		// Sprite OAM
		case 0xE00:
			OAM[location & 0xFF] = data;
			break;

		case 0xF00:
			write_zero_page(location, data);
			break;
		}
	}
}

void Memory::write_zero_page(Address location, Byte data)
{
	switch (location)
	{
	// Joypad Register - only bits 4 & 5 can be written to
	case 0xFF00:
		ZRAM[0x00] = (data & 0x30);
		break;
	// Divider Register - Write as zero no matter content
	case 0xFF04:
		ZRAM[0x04] = 0;
		break;
	// TODO: STAT - writing to match flag resets flag but doesn't change mode

	// LY Register - Game cannot write to this register directly 
	case 0xFF44:
		ZRAM[0x44] = 0;
		break;
	// DMA transfer request
	case 0xFF46:
		ZRAM[0x46] = data;
		do_dma_transfer();
		break;
	default:
		ZRAM[location & 0xFF] = data;
		break;
	}
}