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


void Chip8::emulateCycle()
{
	opcode = memory[pc] << 8 | memory[pc + 1]; // fetch opcode

	// opcode masks
	uint8_t x = opcode >> 8 & 0x0F;  // lower 4 bits of the higher byte
	uint8_t y = opcode >> 4 & 0x0F;  // higher 4 bits of the lower byte
	uint8_t kk = opcode & 0xFF;      // lower byte 
	uint8_t nnn = opcode & 0x0FFF;   // lower 12 bits 
	uint8_t n = opcode & 0x0F;       // lower 4 bits

	switch (opcode & 0xF000)
	{ // first 4 bits of the upper byte of the opcode
	case 0x0000:
		switch (n)
		{
		case 0x0000: // Clear the display; CLS
			printf("Clearing display.\n");
			for (int i = 0; i < 64 * 32; ++i) {
				display[i] = 0;
			}
			printf("Display cleared.\n");
			pc += 2;
			break;
		case 0x000E: // Return from subroutine; RET
			printf("Returning from subroutine.\n");
			if (sp == 0) {
				std::cerr << "Error: stack underflow." << std::endl;
				return; // stack underflow
			}
			--sp;
			pc = stack[sp];
			break;
		default:
			// Unknown opcode
			printf("Unknown opcode: %04X\n", opcode);
			exit(3);
		}
		break;
	case 0x1000: // JP addr
		printf("Jumping to address: %03X\n", nnn);
		pc = nnn;
		break;
	case 0x2000: // CALL addr
		printf("Calling subroutine at address: %03X\n", nnn);
		++sp;
		stack[sp] = pc;
		pc = nnn;
		break;
	case 0x3000: // 3xkk; SE Vx, byte;
		printf("Skipping next instruction if V%X == %02X\n", x, kk);
		if (V[x] == kk) {
			pc += 4;
		}
		else {
			pc += 2;
		}
		break;
	case 0x4000: // SNE Vx, byte 
		printf("Skipping next instruction if V%X != %02X\n", x, kk);
		if (V[x] != kk) {
			pc += 4;
		}
		else {
			pc += 2;
		}
		break;
	case 0x5000: // SE Vx, Vy
		printf("Skipping next instruction if V%X == V%X\n", x, y);
		if (V[x] == V[y]) {
			pc += 4;
		}
		else {
			pc += 2; 
		}
		break;
	case 0x6000: // LD Vx, byte
		printf("Loading %02X into V%X\n", kk, x);
		V[x] = kk;
		pc += 2;
		break;
	case 0x7000: // ADD Vx, byte
		printf("Adding %02X to V%X\n", kk, x);
		V[x] += kk;
		pc += 2;
		break;
	case 0x8000:
		switch (n)
		{
		case 0x0000: // 8XY0; LD Vx, Vy 
			printf("Loading V%X into V%X\n", y, x);
			V[x] = V[y];
			pc += 2;
			break;
		case 0x0001: // 8XY1; OR Vx, Vy
			printf("ORing V%X with V%X\n", y, x);
			V[x] |= V[y];
			pc += 2;
			break;
		case 0x0002: // 8XY2; AND Vx, Vy
			printf("ANDing V%X with V%X\n", y, x);
			V[x] &= V[y];
			pc += 2;
			break;
		case 0x0003: // 8XY3; XOR Vx, Vy
			printf("XORing V%X with V%X\n", y, x);
			V[x] ^= V[y];
			pc += 2;
			break;
		case 0x0004: // 8XY4; ADD Vx, Vy
			printf("Adding V%X to V%X\n", y, x);
			uint16_t sum = V[x] + V[y];
			V[0xF] = (sum > 255) ? 1 : 0; // set carry flag if overflow occurs
			printf("Carry flag set to %d\n", V[0xF]);
			V[x] = sum & 0xFF; // store only the lower 8 bits
			pc += 2;
			break;
		case 0x0005: // 8XY5; SUB Vx, Vy
			printf("Subtracting V%X from V%X\n", y, x);
			V[0xF] = (V[x] >= V[y]) ? 1 : 0; // set carry flag if Vx > Vy
			V[x] -= V[y];
			pc += 2;
			break;
		case 0x0006: // 8XY6; SHR Vx {, Vy}
			printf("Shifting V%X right\n", x);
			V[0xF] = V[x] & 0x01;
			V[x] >>= 1; // shift right
			pc += 2;
			break;
		case 0x0007: // 8XY7; SUBN Vx, Vy
			printf("Subtracting V%X from V%X\n", x, y);
			V[0xF] = (V[y] >= V[x]) ? 1 : 0; // set carry flag if Vy > Vx
			V[x] = V[y] - V[x];
			pc += 2;
			break;
		case 0x000E: // 8XYE; SHL Vx {, Vy}
			printf("Shifting V%X left\n", x);
			V[0xF] = (V[x] & 0x80) ? 1 : 0;  // set carry flag if the highest bit is set (1)
			V[x] <<= 1; // shift left
			pc += 2;
			break;
		default:
			// Unknown opcode
			printf("Unknown opcode: %04X\n", opcode);
			exit(3);
		}
		break;
	case 0x9000: // 9XY0; SNE Vx, Vy
		if (V[x] != V[y]) {
			printf("Skipping next instruction because V%X != V%X\n", x, y);
			pc += 4;
		}
		else {
			printf("Not skipping next instruction because V%X == V%X\n", x, y);
			pc += 2;
		}
		break;
	case 0xA000: // ANNN; LD I, addr
		printf("Setting I to address: %03X\n", nnn);
		I = nnn;
		pc += 2; // move to the next instruction
		break;
	case 0xB000: // BNNN; JP V0, addr
		printf("Jumping to address: %03X + V0\n", nnn);
		pc = nnn + V[0];
		break;
	case 0xC000: // CXKK; RND Vx, byte
		printf("Setting V%X to random value ANDed with %02X\n", x, kk);
		V[x] = (rand() % 0xFF) & kk; // generate a random byte and AND with kk
		pc += 2; // move to the next instruction
		break;
	case 0xD000: // DXYN; DRW Vx, Vy, nibble
		printf("Drawing sprite at V%X, V%X with %d bytes\n", x, y, n);

		// é aqui que o filho chora e a mãe não vê

		/*
		*	From Cowgod's documentation:
			Display n-byte sprite starting at memory location I at(Vx, Vy), set VF = collision.
			The interpreter reads n bytes from memory, starting at the address stored in I.
			These bytes are then displayed as sprites on screen at coordinates(Vx, Vy).
			Sprites are XORed onto the existing screen.
			If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0.
			If the sprite is positioned so part of it is outside the coordinates of the display,
			it wraps around to the opposite side of the screen.
		*/

		uint8_t posX = V[x];
		uint8_t posY = V[y];
		uint8_t spriteLine = 0;
		V[0xF] = 0;
		/*
			I don't know if this is working correctly, but I'm going to assume it is for now.
			I still need to understand some details about how the display works.
		*/
		for (int i = 0; i < n; ++i) { // for each vertical line of the sprite
			spriteLine = memory[I + i]; // load current sprite line from memory (1 byte per line ~ 8 bits long always)
			uint8_t currentY = (posY + i) % 32; // Wrap-around if the sprite goes off the bottom edge
			for (int j = 0; j < 8; ++j) { // for each horizontal pixel in the line (from 7=left (MSB) to 0=right (LSB))
				uint8_t currentX = (posX + (7 - j)) % 64;  // Wrap-around if the sprite goes off the right edge
				int displayIndex = currentY * 64 + currentX; // calculate the screen index

				if (spriteLine & (1 << (7 - j))) { // check if the current pixel is set in the sprite line
					if (display[displayIndex] == 1) { // if the pixel is already set
						V[0xF] = 1; // set collision flag
					}

					display[displayIndex] ^= 1; // XOR the pixel (toggle it)
				}
			}
		}
		pc += 2;
		break;
	case 0xE000:
		switch (n)
		{
		case 0x000E:
			if (keypad[V[x]] != 0) { // EX9E; SKP Vx
				printf("Skipping next instruction because key %X is pressed\n", V[x]);
				pc += 4;
			}
			else {
				printf("Not skipping next instruction because key %X is not pressed\n", V[x]);
				pc += 2;
			}
			break;
		case 0x0001:
			if (keypad[V[x]] == 0) { // EXA1; SKNP Vx
				printf("Skipping next instruction because key %X is not pressed\n", V[x]);
				pc += 4;
			}
			else {
				printf("Not skipping next instruction because key %X is pressed\n", V[x]);
				pc += 2;
			}
			break;
		default:
			printf("Unknown opcode: %04X\n", opcode);
			exit(3);
		}
		break;
	case 0xF000:
		switch (kk)
		{
		case 0x0007: // FX07; LD Vx, DT
			printf("Loading delay timer into V%X\n", x);
			V[x] = delay_timer;
			pc += 2;
			break;
		case 0x000A: // FX0A; LD Vx, K
			bool keyPressed = false;
			printf("Waiting for key press to load into V%X\n", x);
			for (int i = 0; i < 16; ++i) { // check if any key is pressed
				if (keypad[i] != 0) {
					V[x] = i; // load the key index into Vx
					keyPressed = true;
					printf("Key %X pressed, loaded into V%X\n", i, x);
					break;
				}
			}

			if (!keyPressed) {
				std::cerr << "Error: no key pressed." << std::endl;
				return; // no key pressed, cannot continue
			}
			pc += 2;
			break;
		case 0x0015: // FX15; LD DT, Vx
			printf("Setting delay timer to V%X\n", x);
			delay_timer = V[x];
			pc += 2;
			break;
		case 0x0018: // FX18; LD ST, Vx
			printf("Setting sound timer to V%X\n", x);
			sound_timer = V[x];
			pc += 2;
			break;
		case 0x001E: // FX1E; ADD I, Vx
			printf("Adding V%X to I\n", x);
			uint16_t newI = I + V[x];
			if (newI > 0xFFF) { // if I overflows
				V[0xF] = 1;  
				I = 0xFFF;
			}
			else {
				I = newI;
			}
			pc += 2;
			break;
		case 0x0029: // FX29; LD F, Vx
			printf("Setting I to the location of sprite for character in V%X\n", x);
			I = V[x] * 5; // each character sprite is 5 bytes long
			pc += 2;
			break;
		case 0x0033: // FX33; LD B, Vx
			printf("Storing BCD of V%X in memory at I\n", x);
			memory[I] = V[x] / 100;
			memory[I + 1] = (V[x] / 10) % 10;
			memory[I + 2] = V[x] % 10;
			pc += 2;
			break;
		case 0x0055: // FX55; LD [I], Vx
			printf("Storing V0 to V%X in memory starting at I\n", x);
			for (int i = 0; i <= x; ++i) {
				memory[I + i] = V[i];
			}
			I += x + 1; // move I to the next free location
			pc += 2;
			break;
		case 0x0065: // FX65; LD Vx, [I]
			printf("Loading memory from I to V0 to V%X\n", x);
			for (int i = 0; i <= x; ++i) {
				V[i] = memory[I + i];
			}
			I += x + 1; // move I to the next free location
			pc += 2;
			break;
		default:
			printf("Unknown opcode: %04X\n", opcode);
			exit(3);
		}
		break;
	default:
		printf("Unknown opcode: %04X\n", opcode);
		exit(3);
	}
}
