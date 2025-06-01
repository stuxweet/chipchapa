#include "chip8.h"
#include <iostream>

Chip8::Chip8()
{
	// initialize registers and memory
	for (int i = 0; i < 4096; ++i) {
		memory[i] = 0;
	}

	for (int i = 0; i < 16; ++i) {
		V[i] = 0;
		stack[i] = 0;
		keypad[i] = 0;
	}

	for (int i = 0; i < 64 * 32; ++i) {
		display[i] = 0;
	}

	I = 0;
	sp = 0;
	pc = 0x200;

	opcode = 0;

	delay_timer = 0;
	sound_timer = 0;

	// load fontset into memory
	uint8_t fontset[80] = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, //0
		0x20, 0x60, 0x20, 0x20, 0x70, //1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
		0x90, 0x90, 0xF0, 0x10, 0x10, //4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
		0xF0, 0x10, 0x20, 0x40, 0x40, //7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
		0xF0, 0x90, 0xF0, 0x90, 0x90, //A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
		0xF0, 0x80, 0x80, 0x80, 0xF0, //C
		0xE0, 0x90, 0x90, 0x90, 0xE0, //D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
		0xF0, 0x80, 0xF0, 0x80, 0x80  //F
	};

	for (int i = 0; i < 80; ++i) {
		memory[i] = fontset[i];
	}
}

Chip8::~Chip8()
{
}

void Chip8::loadROM(const std::string& filename)
{
	printf("Loading ROM: %s\n", filename.c_str());
	FILE* rom = fopen(filename.c_str(), "rb");

	if (!rom) {
		std::cerr << "Error: could not open the ROM file: " << filename << std::endl;
		return;
	}

	fseek(rom, 0, SEEK_END); // move to the end of the file
	long size = ftell(rom);  // get the size
	rewind(rom);             // rewind to the beginning

	if (size > 4096 - 512) { // 4096 is the total memory size, 512 is the reserved space for the interpreter
		std::cerr << "ROM size exceeds memory limit." << std::endl;
		fclose(rom);
		return;
	}

	// load ROM into memory, starting address = 0x200 in Chip-8
	// there's no need to allocate memory dynamically since we have a fixed-size array
	size_t read = fread(&memory[0x200], 1, size, rom);
	if (read != size) {
		std::cerr << "Error reading the ROM file." << std::endl;
	}

	printf("ROM loaded successfully, size: %ld bytes.\n", size);
	fclose(rom);
}

void Chip8::emulateCycle() {
	opcode = memory[pc] << 8 | memory[pc + 1]; // fetch opcode

	switch (opcode & 0xF000) {
		case 0x0000:
			// Handle 0x00E0 and 0x00EE
			switch (opcode) {
				case 0x00E0: // 00E0 - Clear the display / CLS
					printf("Clearing display.\n");
					for (int i = 0; i < 64 * 32; ++i) {
						display[i] = 0;
					}
					pc += 2;
					break;
				case 0x00EE: // 00EE - Return from subroutine / RET
					printf("Returning from subroutine.\n");
					if (sp == 0) {
						std::cerr << "Error: stack underflow." << std::endl;
						return; // stack underflow
					}
					sp--;
					pc = stack[sp];
					break;
			}
		case 0x1000: // 1NNN - Jump to address NNN / JP addr
			printf("Jumping to address: %03X\n", opcode & 0x0FFF);
			pc = opcode & 0x0FFF;
		case 0x2000: // CALL addr
			break;
		case 0x3000: // SE Vx, byte
			break;
		case 0x4000: // SNE Vx, byte
			break;
		case 0x5000: // SE Vx, Vy
			break;
		case 0x6000: // LD Vx, byte
			break;
		case 0x7000: // ADD Vx, byte
			break;
		case 0x8000: // LD Vx, Vy
			break;
		// ... handle other opcodes...
		default:
			// Unknown opcode
	}
}
