// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "memory_abstract.h"
//#include "compile_to_RISC_V.h"
// Purpose: keep memory code generation away from general perf model code

enum rs1_type : UINT {
	rs2_0 = 0x0000000,
	rs2_1 = 0x0100000,
	rs2_2 = 0x0200000,
	rs2_3 = 0x0300000,
	rs2_4 = 0x0400000,
	rs2_5 = 0x0500000,
	rs2_6 = 0x0600000,
	rs2_7 = 0x0700000,
	rs2_8 = 0x0800000,
	rs2_9 = 0x0900000,
	rs2_10 = 0x0a00000,
	rs2_11 = 0x0b00000,
	rs2_12 = 0x0c00000,
	rs2_13 = 0x0d00000,
	rs2_14 = 0x0e00000,
	rs2_15 = 0x0f00000,
	rs2_16 = 0x1000000,
	rs2_17 = 0x1100000,
	rs2_18 = 0x1200000,
	rs2_19 = 0x1300000,
	rs2_20 = 0x1400000,
	rs2_21 = 0x1500000,
	rs2_22 = 0x1600000,
	rs2_23 = 0x1700000,
	rs2_24 = 0x1800000,
	rs2_25 = 0x1900000,
	rs2_26 = 0x1a00000,
	rs2_27 = 0x1b00000,
	rs2_28 = 0x1c00000,
	rs2_29 = 0x1d00000,
	rs2_30 = 0x1e00000,
	rs2_31 = 0x1f00000
};
#define rs1_0  0x00000
#define rs1_1  0x08000
#define rs1_2  0x10000
#define rs1_3  0x18000
#define rs1_4  0x20000
#define rs1_5  0x28000
#define rs1_6  0x30000
#define rs1_7  0x38000
#define rs1_8  0x40000
#define rs1_9  0x48000
#define rs1_10  0x50000
#define rs1_11  0x58000
#define rs1_12  0x60000
#define rs1_13  0x68000
#define rs1_14  0x70000
#define rs1_15  0x78000
#define rs1_16  0x80000
#define rs1_17  0x88000
#define rs1_18  0x90000
#define rs1_19  0x98000
#define rs1_20  0xa0000
#define rs1_21  0xa8000
#define rs1_22  0xb0000
#define rs1_23  0xb8000
#define rs1_24  0xc0000
#define rs1_25  0xc8000
#define rs1_26  0xd0000
#define rs1_27  0xd8000
#define rs1_28  0xe0000
#define rs1_29  0xe8000
#define rs1_30  0xf0000
#define rs1_31  0xf8000
/*
enum rs1_type : UINT {
	rs1_0 = 0x00,
	rs1_1 = 0x8000,
	rs1_2 = 0x10000,
	rs1_3 = 0x18000,
	rs1_4 = 0x20000,
	rs1_5 = 0x28000,
	rs1_6 = 0x30000,
	rs1_7 = 0x38000,
	rs1_8 = 0x40000,
	rs1_9 = 0x48000
};
/**/
enum rd_type :UINT {
	rd_0 = 0x000,
	rd_1 = 0x080,
	rd_2 = 0x100,
	rd_3 = 0x180,
	rd_4 = 0x200,
	rd_5 = 0x280,
	rd_6 = 0x300,
	rd_7 = 0x380,
	rd_8 = 0x400,
	rd_9 = 0x480,
	rd_10 = 0x500,
	rd_11 = 0x580,
	rd_12 = 0x600,
	rd_13 = 0x680,
	rd_14 = 0x700,
	rd_15 = 0x780,
	rd_16 = 0x800,
	rd_17 = 0x880,
	rd_18 = 0x900,
	rd_19 = 0x980,
	rd_20 = 0xa00,
	rd_21 = 0xa80,
	rd_22 = 0xb00,
	rd_23 = 0xb80,
	rd_24 = 0xc00,
	rd_25 = 0xc80,
	rd_26 = 0xd00,
	rd_27 = 0xd80,
	rd_28 = 0xe00,
	rd_29 = 0xe80,
	rd_30 = 0xf00,
	rd_31 = 0xf80
};
enum opcode_type : UINT {
	i_load_op = 0x0003,
	store_op = 0x0023,
	ADDi_op = 0x0013,
	slti_op = 0x2013,	// shift left .. integer
	sltiu_op = 0x3013,	// shift left .. unsigned integer
	SLLi_op = 0x1013,	// shift left .. unsigned integer
	SRLi_op = 0x5013,	// shift right .. unsigned integer
	ORi_op = 0x6013,	// shift right .. unsigned integer
	ANDi_op = 0x7013,	// shift right .. unsigned integer

	ADDi_op64 = 0x001b,
	slti_op64 = 0x201b,	// shift left .. integer
	sltiu_op64 = 0x301b,	// shift left .. unsigned integer
	SLLi_op64 = 0x101b,	// shift left .. unsigned integer
	SRLi_op64 = 0x501b,	// shift right .. unsigned integer
	ORi_op64 = 0x601b,	// shift right .. unsigned integer
	ANDi_op64 = 0x701b,	// shift right .. unsigned integer

	ADD_op = 0x0033,
	SRL_op = 0x5033,	// shift right .. unsigned integer
	OR_op = 0x6033,
	AND_op = 0x7033,

	BEQ_op = 0x0063,
	BNE_op = 0x1063,

	addi64_op = 0x001b,

	flw_op = 0x2007,// floating point load
	fsw_op = 0x2027,// floating point store
	fld_op = 0x3007,
	fsd_op = 0x3027,
	fadd_op = 0x00000053,
	fmul_op = 0x10000053,
	fdiv_op = 0x18000053,
	cvt_if_op = 0xc0000053,// single precision
	cvt_fi_op = 0xd0000053,

	LUI_op = 0x037,

	ECALL_op = 0x00000073,
	EBREAK_op = 0x00100073,
	uret_op = 0x00200073,
	sret_op = 0x10200073,
	hret_op = 0x20200073,
	mret_op = 0x30200073,

	CSRRW_op = 0x1073,
	CSRRS_op = 0x2073,
	CSRRC_op = 0x3073,
	CSRRWI_op = 0x5073,
	CSRRSI_op = 0x6073,
	CSRRCI_op = 0x7073
	//	cvt_wd_op = 0xc2000053,// double precision
	//	cvt_dw_op = 0xd2000053,
	//	cvt_wq_op = 0xc6000053,// quad precision
	//	cvt_qw_op = 0xd6000053
};
#define sp_op 0x00000000
#define dp_op 0x02000000
#define qp_op 0x06000000
/*
// bit definition
// 0 - present
// 1 read/write (0 = read only)
// 2 U/S user/supervisor (0 = supervisor only)
// 3 PWT write through (1 = no dirty data allowed)
// 4 PCD cache disabled (1 = dont cache)
// 6 D dirty bit- caches maintain dirty data
// 12-31 address translation
const UINT page_table_4K_32b[0x20] = {
	0x0000100d, // code (logical page 0)
	0x0000200d,// sr1 (logical page 1)
	0x0100000d,// sr2 - core 1 start
	0x0180000d,// dst - core 1 start
	0x00000000,0x00000000,0x00000000,0x00000000,//8
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,//16
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,//
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000	//32
};
const UINT page_table_4K_32b_2[0x20] = {
	//	0x0000100d, // code (logical page 0)
	//	0x0000200d,// sr1 (logical page 1)
		0x0100000d,// sr2 - core 1 start
	//	0x0180000d,// dst - core 1 start
		0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,//8
		0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,//16
		0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,//
		0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000	//32
};
/**/

const UINT page_code2[0x60] = {
	// input base address
	(1 << 20) | rs1_0 | rd_7 | ADDi_op,				// reg1 = 1
	(2 << 20) | rs1_0 | rd_30 | ADDi_op,			// reg30 = 2
	 rs1_30 | rd_30 | cvt_if_op | sp_op,			// f_reg30 = 2.0
	 rs1_7 | rd_31 | cvt_if_op | sp_op,				// f_reg31 = 1.0
	 rs2_30 | rs1_31 | rd_31 | fdiv_op | sp_op,		// f_reg31 = 0.5
	 (24 << 20) | rs1_7 | rd_8 | SLLi_op,			// shift left 24 to reg3 => reg7 = 0x1000000
	 (25 << 20) | rs1_7 | rd_6 | SLLi_op,			// shift left 25 to reg3 => reg6 = 0x2000000
	 (12 << 20) | rs1_7 | rd_11 | SLLi_op,			// shift left 24 to reg3 => reg7 = 0x4000
	 // load input value to be transformed first
	 (0 << 20) | rs1_8 | rd_9 | ADDi_op,			// add 0 offset for 1st load
	 (0x00 << 20) | rs1_9 | 0x2000 | rd_0 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	
	 (0x04 << 20) | rs1_9 | 0x2000 | rd_1 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	
	 (0x08 << 20) | rs1_9 | 0x2000 | rd_2 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	
	 (0x0c << 20) | rs1_9 | 0x2000 | rd_3 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	
	 (0x10 << 20) | rs1_9 | 0x2000 | rd_4 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	
	 (0x14 << 20) | rs1_9 | 0x2000 | rd_5 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	
	 (0x18 << 20) | rs1_9 | 0x2000 | rd_6 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	
	 (0x1c << 20) | rs1_9 | 0x2000 | rd_7 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	

	 (0 << 20) | rs1_6 | rd_10 | ADDi_op,			// add 0 offset for 2nd load
	 (0x00 << 20) | rs1_10 | 0x2000 | rd_8 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	
	 (0x04 << 20) | rs1_10 | 0x2000 | rd_9 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	
	 (0x08 << 20) | rs1_10 | 0x2000 | rd_10 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	
	 (0x0c << 20) | rs1_10 | 0x2000 | rd_11 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	
	 (0x10 << 20) | rs1_10 | 0x2000 | rd_12 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	
	 (0x14 << 20) | rs1_10 | 0x2000 | rd_13 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	
	 (0x18 << 20) | rs1_10 | 0x2000 | rd_14 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	
	 (0x1c << 20) | rs1_10 | 0x2000 | rd_15 | flw_op, // float load f_reg3 ; f_reg3 = mem[imm+x_reg3] ; reg3 table pointer,  imm = element in table  0x2000 == width = single precision (32b)	

	 rs2_8 | rs1_0 | rd_16 | fadd_op | sp_op,
	 rs2_9 | rs1_1 | rd_17 | fadd_op | sp_op,
	 rs2_10 | rs1_2 | rd_18 | fadd_op | sp_op,
	 rs2_11 | rs1_3 | rd_19 | fadd_op | sp_op,
	 rs2_12 | rs1_4 | rd_20 | fadd_op | sp_op,
	 rs2_13 | rs1_5 | rd_21 | fadd_op | sp_op,
	 rs2_14 | rs1_6 | rd_22 | fadd_op | sp_op,
	 rs2_15 | rs1_7 | rd_23 | fadd_op | sp_op,

	 rs2_31 | rs1_16 | rd_16 | fmul_op | sp_op,
	 rs2_31 | rs1_17 | rd_17 | fmul_op | sp_op,
	 rs2_31 | rs1_18 | rd_18 | fmul_op | sp_op,
	 rs2_31 | rs1_19 | rd_19 | fmul_op | sp_op,
	 rs2_31 | rs1_20 | rd_20 | fmul_op | sp_op,
	 rs2_31 | rs1_21 | rd_21 | fmul_op | sp_op,
	 rs2_31 | rs1_22 | rd_22 | fmul_op | sp_op,
	 rs2_31 | rs1_23 | rd_23 | fmul_op | sp_op,

	 (0x00 << 20) | rs1_11 | rd_16 | fsw_op,
	 (0x04 << 20) | rs1_11 | rd_17 | fsw_op,
	 (0x08 << 20) | rs1_11 | rd_18 | fsw_op,
	 (0x0c << 20) | rs1_11 | rd_19 | fsw_op,
	 (0x10 << 20) | rs1_11 | rd_20 | fsw_op,
	 (0x14 << 20) | rs1_11 | rd_21 | fsw_op,
	 (0x18 << 20) | rs1_11 | rd_22 | fsw_op,
	 (0x1c << 20) | rs1_11 | rd_23 | fsw_op,
	 // load table  and start MACS
	 // output entry[0]
	 (1 << 20) | rs1_0 | rd_1 | ADDi_op,			// add immediate (1) to reg0 (0) => reg3 = 1
	 (12 << 20) | rs1_1 | rd_1 | SLLi_op,		// shift left 12 to reg2 => reg2 = 0x1000
	 (0 << 20) | rs1_1 | rd_2 | ADDi_op,			// add 0 offset for 1st load
	 (0 << 20) | rs1_2 | 0x2000 | rd_0 | flw_op, // float load f_reg2 ; f_reg2 = mem[imm+x_reg2] ; reg2 table pointer, imm = element in table  0x2000 == width = single precision (32b)
	 rs2_0 | rs1_8 | rd_16 | fmul_op,				// f_reg1 = f_reg2 * f_reg3
	 (4 << 20) | rs1_2 | 0x2000 | rd_1 | flw_op, // float load f_reg2 ; f_reg2 = mem[imm+x_reg2] ; reg2 table pointer, imm = element in table  0x2000 == width = single precision (32b)
	 rs2_1 | rs1_8 | rd_17 | fmul_op,				// f_reg1 = f_reg2 * f_reg3
	 (8 << 20) | rs1_2 | 0x2000 | rd_2 | flw_op, // float load f_reg2 ; f_reg2 = mem[imm+x_reg2] ; reg2 table pointer, imm = element in table  0x2000 == width = single precision (32b)
	 rs2_2 | rs1_8 | rd_18 | fmul_op,				// f_reg1 = f_reg2 * f_reg3
	 (12 << 20) | rs1_2 | 0x2000 | rd_3 | flw_op, // float load f_reg2 ; f_reg2 = mem[imm+x_reg2] ; reg2 table pointer, imm = element in table  0x2000 == width = single precision (32b)
	 rs2_3 | rs1_8 | rd_19 | fmul_op,				// f_reg1 = f_reg2 * f_reg3
	 (16 << 20) | rs1_2 | 0x2000 | rd_4 | flw_op, // float load f_reg2 ; f_reg2 = mem[imm+x_reg2] ; reg2 table pointer, imm = element in table  0x2000 == width = single precision (32b)
	 rs2_4 | rs1_8 | rd_20 | fmul_op,				// f_reg1 = f_reg2 * f_reg3
	 (20 << 20) | rs1_2 | 0x2000 | rd_5 | flw_op, // float load f_reg2 ; f_reg2 = mem[imm+x_reg2] ; reg2 table pointer, imm = element in table  0x2000 == width = single precision (32b)
	 rs2_5 | rs1_8 | rd_21 | fmul_op,				// f_reg1 = f_reg2 * f_reg3
	 (24 << 20) | rs1_2 | 0x2000 | rd_6 | flw_op, // float load f_reg2 ; f_reg2 = mem[imm+x_reg2] ; reg2 table pointer, imm = element in table  0x2000 == width = single precision (32b)
	 rs2_6 | rs1_8 | rd_22 | fmul_op,				// f_reg1 = f_reg2 * f_reg3
	 (28 << 20) | rs1_2 | 0x2000 | rd_7 | flw_op, // float load f_reg2 ; f_reg2 = mem[imm+x_reg2] ; reg2 table pointer, imm = element in table  0x2000 == width = single precision (32b)
	 rs2_7 | rs1_8 | rd_23 | fmul_op,				// f_reg1 = f_reg2 * f_reg3
	 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,//8
	 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,//8
	 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,//8
	 0x00000000,0x00000000,0x00000000
};
// change all data to 32b. RISC_V is 16 bit aligned normally. Force to 32b aligned for now. 
// today, engeering code is 32fp, could use 16b fp for graphics
// graphics moving to Vectored data
// int used for loops, control of vectored data. Blow up to 64b aligned???
void memory_abstract(UINT64* data, UINT64 address, UINT8 write_enable, memory_space_type* memory_space, UINT16 xaction_id, UINT64 Bclock) {
	int debug = 0;
	if (Bclock == 0x0bc7)
		debug++;

	if (write_enable) {
		if (address >= (UINT64)0x86400000 && address < (UINT64)0x86500000) {
			debug++; // overwritting "main" program
		}
		// CLINT - 0x02000000 Core Logic Interrupt space
		if (address < 0x80000000) { // UC space
			if (address >= 0x02000000 && address < 0x04000000) {// CLINT
				UINT index = (address - 0x02000000) >> 21;
				for (UINT i = 0; i < 0x10; i++)   memory_space->clic[index].memory[((address & 0x00000f80) >> 3) | i] = data[i];
			}
		}
		if (address == 0x00000000853ffff0)
			debug++;
	}
	else {
		if (address < 0x80000000) { // UC space
			if (address >= 0x00001000 && address < 0x00002000) { // reset
				UINT index = (address - 0x00001000) >> 21;
				for (UINT i = 0; i < 0x10; i++)  data[i] = memory_space->reset[index].memory[((address & 0x00000f80) >> 3) | i];
			}
			else if (address >= 0x02000000 && address < 0x04000000) {// CLINT// CLINT - 0x02000000
				UINT index = (address - 0x02000000) >> 21;
				for (UINT i = 0; i < 0x10; i++)  data[i] = memory_space->clic[index].memory[((address & 0x00000f80) >> 3) | i];
			}
		}
		else {
			if (address == 0x80002000)
				debug++;
			int tag = (address - 0x80000000) >> 21;
			for (UINT i = 0; i < 0x10; i++)  data[i] = memory_space->page[tag].memory[((address & 0x001fff80) >> 3) | i];
		}
	}
}