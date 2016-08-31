#pragma once

#include "types.h"
#include "memory.h"

// Register pair helper class
class Pair
{
	private:
		Byte& high;
		Byte& low;

	public:
		Pair(Byte& high_, Byte& low_)
			: high(high_), low(low_) {}
		void inc();
		void dec();
		void set(Byte_2 value);
		void set(Byte upper, Byte lower);
		Byte_2 get();

		Address address();
};

// Special register helper class
// Special registers are each 1 byte of data starting from memory location 0xFF00 - 0xFFFF
class SpecialRegister
{
	Address addr;

	public:
		SpecialRegister(Memory& memory_, Address addr_)
			: memory(memory_) {};

	private:
		Memory& memory;
};

// Gameboy CPU: 8-bit (Similar to the Z80 processor)
class CPU
{
	public:

		CPU(Memory& memory_)
			: memory(memory_) {
			init(); 
		};

		void debug();
		void execute(int cycles);

	private:

		Memory& memory;

		Byte reg_A; // Accumulator
		Byte reg_B;
		Byte reg_C;
		Byte reg_D;
		Byte reg_E;
		Byte reg_H;
		Byte reg_L;
		Byte reg_F; // Flag Register
		Byte_2 reg_SP; // Stack Pointer
		Byte_2 reg_PC; // Program Counter

		int cycles = 0;
		
		// 0 - Reset by DI instruction; prohibits all interrupts,
		// 1 - Set by EI instruction, The interrupts set by IE registers are enabled
		bool interrupt_master_enable = false;

		const int FLAG_ZERO       = 0b10000000;
		const int FLAG_SUB        = 0b01000000;
		const int FLAG_HALF_CARRY = 0b00100000;
		const int FLAG_CARRY      = 0b00010000;

		const int INTERRUPT_V_BLANKING       = 0b00000001;
		const int INTERRUPT_LCDC             = 0b00000010;
		const int INTERRUPT_TIMER_OVERFLOW   = 0b00000100;
		const int INTERRUPT_SERIAL_IO_DONE   = 0b00001000;
		const int INTERRUPT_P10_P13_LOW      = 0b00010000;

		const Address INT_CALL_V_BLANKING     = 0x0040;
		const Address INT_CALL_LCDC           = 0x0048; // Mode can be selected by STAT register
		const Address INT_CALL_TIMER_OVERFLOW = 0x0050;
		const Address INT_CALL_SERIAL_IO_DONE = 0x0058;
		const Address INT_CALL_P10_P13_LOW    = 0x0060;

		void init();
		void reset();
		void interrupt_signal();
		void process_interrupt();
		void stop();

		void parse_opcode(Opcode code);
		void parse_bit_op(Opcode code);

		void set_flag(int flag, bool value);
		void op(int pc, int cycle);

		/***** CPU INSTRUCTIONS *****/
		
		// 8-bit loads
		void LD(Byte& destination, Byte value);
		void LD(Byte& destination, Address addr);
		void LD(Address addr, Byte value);

		// 16-bit loads
		void LD(Pair reg_pair, Byte upper, Byte lower);
		void LD(Byte_2& reg_pair, Byte upper, Byte lower);
		void LDHL(Byte value);
		void LDNN(Byte low, Byte high);

		void PUSH(Byte high, Byte low);
		void POP(Byte& high, Byte& low);

		void ADD(Byte& target, Byte value);
		void ADD(Byte& target, Address addr);
		void ADC(Byte& target, Byte value);
		void ADC(Byte& target, Address addr);

		void SUB(Byte& target, Byte value);
		void SUB(Byte& target, Address addr);
		void SBC(Byte& target, Byte value);
		void SBC(Byte& target, Address addr);

		void AND(Byte& target, Byte value);
		void AND(Byte& target, Address addr);

		void OR(Byte& target, Byte value);
		void OR(Byte& target, Address addr);

		void XOR(Byte& target, Byte value);
		void XOR(Byte& target, Address addr);

		void CP(Byte& target, Byte value);
		void CP(Byte& target, Address addr);

		void INC(Byte& target);
		void INC(Address addr);

		void DEC(Byte& target);
		void DEC(Address addr);

		// 16-bit arithmetic
		void ADD16(Byte_2 target, Byte_2 value);
		void ADDHL(Pair reg_pair);
		void ADDSP(Byte value);

		void INC(Pair reg_pair);
		void DEC(Pair reg_pair);

		// Rotate shift instructions
		void RL(Byte& target, bool carry, bool zero_flag = false);
		void RL(Address addr, bool carry);
		void RR(Byte& target, bool carry, bool zero_flag = false);
		void RR(Address addr, bool carry);

		void SLA(Byte& target);
		void SLA(Address addr);
		void SRA(Byte& target);
		void SRA(Address addr);
		void SRL(Byte& target);
		void SRL(Address addr);

		void SWAP(Byte& target);
		void SWAP(Address addr);

		// Bit operations
		void BIT(Byte target, int bit);
		void BIT(Address addr, int bit);

		void SET(Byte& target, int bit);
		void SET(Address addr, int bit);

		void RES(Byte& target, int bit);
		void RES(Address addr, int bit);

		// Jump instructions
		void JP(Pair target);
		void JPNZ(Pair target);
		void JPZ(Pair target);
		void JPNC(Pair target);
		void JPC(Pair target);
		void JR(Byte value);

		void JRNZ(Byte value);
		void JRZ(Byte value);
		void JRNC(Byte value);
		void JRC(Byte value);
		void JPHL();

		// Function Instructions
		void CALL(Byte low, Byte high);
		void CALLNZ(Byte low, Byte high);
		void CALLZ(Byte low, Byte high);
		void CALLNC(Byte low, Byte high);
		void CALLC(Byte low, Byte high);

		void RET();
		void RETI(); // UNIMPLEMENTED
		void RETNZ();
		void RETZ();
		void RETNC();
		void RETC();

		// Miscellaneous Instructions
		void RST(Address addr);
		
		void DAA();
		void CPL();
		void NOP();

		void HALT();
		void STOP();

		// GBCPUMan
		void DI();
		void EI();
};