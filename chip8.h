#pragma once

#include <cstdint>
#include <string>

class Chip8
{
private:
	// 8 bit
	uint8_t memory[4096];       // memory (4KB)
	uint8_t V[16];              // registers (16 8 bit registers)
	uint8_t sp;                 // stack pointer
	uint8_t delay_timer;
	uint8_t sound_timer;

	// 16 bit
	uint16_t I;                 // index register
	uint16_t pc;                // program counter
	uint16_t stack[16];
	uint16_t opcode;            // current opcode

public:
	uint8_t display[64 * 32];   // display (64x32 pixels)
	uint16_t keypad[16];	

	void loadROM(const std::string& filename);
	void emulateCycle();

	Chip8();
	~Chip8();
};
