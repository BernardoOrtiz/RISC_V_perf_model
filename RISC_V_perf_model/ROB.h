// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#pragma once

#include "internal_bus.h"

#define data_TLB_cr_port 0x22
#define code_sTLB_cr_port 0x24
#define data_sTLB_cr_port 0x26

#define prefetch_unit_id 0x00 // keep port architecture (addr and data port) for option on performance register access

enum q_id_type :UINT8 {
	OP_q_id0 = 0x00,
	OP_q_id1 = 0x01,
	OP_q_id2 = 0x02,
	OP_q_id3 = 0x03,
	OP_q_id4 = 0x04,
	OP_q_id5 = 0x05,
	OP_q_id6 = 0x06,
	OP_q_id7 = 0x07,
	fadd_q_id0 = 0x08,
	fadd_q_id1 = 0x09,
	fadd_q_id2 = 0x0a,
	fadd_q_id3 = 0x0b,
	fadd_q_id4 = 0x0c,
	fadd_q_id5 = 0x0d,
	fadd_q_id6 = 0x0e,
	fadd_q_id7 = 0x0f,
	fmul_q_id0 = 0x10,
	fmul_q_id1 = 0x11,
	fmul_q_id2 = 0x12,
	fmul_q_id3 = 0x13,
	//	LUI_AUIPC_q_id = 0x14,
	iMUL_q_id = 0x15,//
	load_q_id = 0x16,
	store_q_id = 0x17,
	store_q_id2 = 0x18,
	fdiv_q_id = 0x19, //
	prefetch_q_id = 0x1b,
	decode_q_id = 0x1c,
	branch_q_id = 0x1d,
	branch_q_id2 = 0x1e,
	sys_q_id = 0x1f
};

enum ROB_state_type : UINT8 {
	ROB_free = 0,
	ROB_allocate_0 = 1,// issued to the allocator
	ROB_allocate_1 = 2,
	ROB_allocate_2 = 3,
	ROB_execute = 4,// issued to an execute unit 
	ROB_inflight = 5,
	ROB_retire_in = 6, // issued to retire unit
	ROB_retire_out = 7, // issued to retire unit
	ROB_branch_miss = 10,// issued to an execute unit 
	//	ROB_retire_out = 7, // issued to retire unit
	//	ROB_retire_out2 = 8, // issued to retire unit
	ROB_fault = 9 // issued to retire unit
};
enum q_select_type : UINT8 {
	OP_q_select0 = 0,
	OP_q_select1 = 1,
	OP_q_select2 = 2,
	OP_q_select3 = 3,
	OP_q_select4 = 4,
	OP_q_select5 = 5,
	OP_q_select6 = 6,
	OP_q_select7 = 7,
	LUI_AUIPC_q_select = 8,
	iMUL_q_select = 9,
	load_q_select = 10,
	store_q_select = 11,
	op_fp_select = 12,
	//	fmul_q_select = 9,
	//	fdiv_q_select = 10,	// handles div, trig, etc. single instruction, blocks on second issue
	none_q_select = 255 // not implementing
};

enum uop_type : UINT8 {
	uop_ADD = 0x00,// all regs pointing to r0 is defined as NOP
	uop_SUB = 0x01,
	uop_SLL = 0x02,
	uop_SLT = 0x03,
	uop_SLTU = 0x04,
	uop_XOR = 0x05,
	uop_OR = 0x06,
	uop_SRA = 0x07,
	uop_SRL = 0x08,
	uop_AND = 0x09,
	uop_MUL = 0x0a,
	uop_MULH = 0x0b,
	uop_MULHSU = 0x0c,
	uop_MULHU = 0x0d,
	uop_DIV = 0x0e,
	uop_DIVU = 0x0f,
	uop_REM = 0x10,
	uop_REMU = 0x11,

	uop_LOAD = 0x12, // load 
	uop_STORE = 0x13, // store 
	uop_F_LOAD = 0x14, // float point load
	uop_F_STORE = 0x15, // floating point store

	uop_ADDI = 0x16,// ADD immidiate, immidiate = 0 and r1=rd=x_reg0 is NOP 
	//	uop_SUBI = 0x11,
	uop_SLLI = 0x17,
	uop_SLTI = 0x18,
	uop_SLTIU = 0x19,
	uop_XORI = 0x1a,
	uop_ORI = 0x1b,
	uop_SRAI = 0x1c,
	uop_SRLI = 0x1d,
	uop_ANDI = 0x1e,

	uop_shifti_add = 0x1f, // my uop
	uop_or_add = 0x20, // my uop
	uop_or_addi = 0x21, // my uop
	uop_imadd = 0x22, // my uop
	uop_inmadd = 0x23, // my uop

	uop_LR = 0x3d, // locked load 
	uop_SC = 0x3e, // locked store 

	uop_FADD = 0x40,// all regs pointing to r0 is defined as NOP
	uop_FSUB = 0x41,
	uop_FMUL = 0x42,// all regs pointing to r0 is defined as NOP
	uop_FDIV = 0x43,
	uop_FSGN = 0x44,// sign injection
	uop_FMIN = 0x45,
	uop_FMAX = 0x46,
	uop_FSQRT = 0x47,
	uop_FEQ = 0x48,
	uop_FLT = 0x49,
	uop_FLE = 0x4a,
	uop_FCVTi2f = 0x4b,
	uop_FCVTf2i = 0x4c,
	uop_FMVi2f = 0x4d,
	uop_FMVf2i = 0x4e,
	uop_FMADD = 0x4F,
	uop_FMSUB = 0x50,
	uop_FNMADD = 0x51,
	uop_FNMSUB = 0x52,

	uop_BEQ = 0x30,
	uop_BNE = 0x31,
	uop_BLT = 0x32,
	uop_BGE = 0x33,
	uop_BLTU = 0x34,
	uop_BGEU = 0x35,

	uop_LUI = 0x80, // load immediate
	uop_AUIPC = 0x81, // load immediate address/PC relative
	uop_JAL = 0x82,
	uop_JALR = 0x83,
//	uop_JALR2 = 0x84,
	uop_CSRRW = 0x90,
	uop_CSRRS = 0x91,
	uop_CSRRC = 0x92,
	uop_CSRRWI = 0x93,
	uop_CSRRSI = 0x94,
	uop_CSRRCI = 0x95,

	uop_ECALL = 0xa0,
	uop_EBREAK = 0xa1,
	uop_uret = 0xa2,
	uop_SRET = 0xa3,
	uop_hret = 0xa4,
	uop_MRET = 0xa5,
	uop_WFI = 0xa6,
	uop_FENCE = 0xa7,

	uop_HALT = 0xfe,
	uop_NOP = 0xff
};
enum opcode_map : UINT8 {
	LOAD_map = 0x00,
	LOAD_FP_map = 0x01,
	custom0_map = 0x02,
	MISC_MEM_map = 0x03,
	OP_IMM_map = 0x04,
	AUIPC_map = 0x05,
	OP_IMM64_map = 0x06,
//	n48b_map = 0x07,

	STORE_map = 0x08,
	STORE_FP_map = 0x09,
	custom1_map = 0x0a,
	AMO_map = 0x0b,// atomic operations (locked)
	OP_map = 0x0c,
	LUI_map = 0x0d,
	OP_64_map = 0x0e,
//	n64b_map = 0x0f,

	MADD_map = 0x10,
	MSUB_map = 0x11,
	NMSUB_map = 0x12,
	NMADD_map = 0x13,
	OP_FP_map = 0x14,
	reserved0_map = 0x15,
	custom2_map = 0x16,
	rv128_map = 0x16,
//	n48b2_map = 0x17,

	BRANCH_map = 0x18,
	JALR_map = 0x19,
	reserved1_map = 0x1a,
	JAL_map = 0x1b,// atomic operations (locked)
	SYSTEM_map = 0x1c,
	reserved2_map = 0x1d,
	custom3_map = 0x1e,
	rv128_2_map = 0x1e,
//	n80b_map = 0x1f
};
enum store_status_type :UINT8 {
	store_inactive = 0,
	store_alloc_addr_valid = 1,
	store_wait_TLB = 2,//in data buffer, alloc issued
	store_wait_TLB_issue = 3,//in data buffer, no alloc issued
	store_TLB_valid = 4,//waiting for branch, write cannot be speculative
	store_TLB_and_retire_valid = 11,//waiting for branch, write cannot be speculative
	store_branch_match = 5,//ready to start write addr cycle
	store_data_merged = 10,//waiting for retire ptr match, write cannot be speculative
	store_w_addr_valid = 6,
	store_w1_addr_valid = 7,
	store_w2_addr_valid = 8,
	store_flush = 9,
	store_retire = 12
};
struct R_type {
	UINT8 strobe, ROB_id;
	INT64 rs1, rs1_h;
	INT64 rs2, rs2_h;
	uop_type uop; // different op encodings
	UINT8 size;
};
struct R3_type {
	UINT8 strobe, ROB_id;
	INT64 rs1, rs1_h;
	INT64 rs2, rs2_h;//rs2 or imm
	INT64 rs3, rs3_h;
	uop_type uop; // different op encodings
	UINT8 size;
};
struct branch_type {
	UINT8 strobe, ROB_id;
	INT64 rs1, rs1_h;
	INT64 rs2, rs2_h;//rs2 or imm
	INT16 offset;
	INT64 addr, addr_h;
	uop_type uop; // different op encodings
};

struct ROB_entry_Type {
	ROB_state_type state;// 0=free, 1=allocated, 2=issued, 3=complete, 0 = retired
	opcode_map map;
	uop_type uop; // different op encodings

	UINT8 branch_num; // used for fetch/load data match, instead of addr

	q_select_type q_select;// 1=float, 0=integer, 2=load/store
	UINT8 q_ptr, q_ptr_valid;
	UINT64 addr, addr_H;// addr is equivalent to PC at retirement, left as addr for snoop purposes	UINT8 reg_type; // 0-int, 1-uint, 2float; 
	//	UINT8 size;// 0 - 1B, 1 - 2B, 2 - 4B, 3 - 8B; int 4-7 Unsigned / float: 1 - 16B, 2 - 32B, 3 - 64B, 4 - 128B
	UINT8 funct3;
	UINT16 funct7;
	INT imm;
	UINT8 rd, rs1, rs2, rs3;// index to reg table
	UINT8  rd_ptr, rs1_ptr, rs2_ptr, rs3_ptr;
	UINT8 rd_retire_ptr;
	UINT8 rs_count;
	control_status_reg_type csr; // indexing CSR is a 12 bit operation
	UINT8 reg_type;//0,1- int, uint; 2- fp; 3- control
	UINT8 bytes;
};

struct ROB_Type {
	UINT16 decode_ptr;// start
	UINT16 allocate_ptr; // place new entries here
	UINT16 retire_ptr_out; // start retiring entries here - in order
	UINT16 retire_ptr_in; // start retiring entries here - in order//stop
	ROB_entry_Type q[0x100];

	UINT8  branch_start, branch_stop;
	UINT8 fence;
	UINT8 fault_halt;
};

struct Q_type {
	UINT start_ptr, curr_ptr, end_ptr;
	UINT ROB_ptr[0x40];	// 256/4 = 64
	UINT8 ROB_ptr_valid[0x40];
	UINT8 state[0x40]; // 0 = inactive, 1=active
	UINT count;
};
struct branch_vars {
	INT ROB_entry;
	UINT8 pred[0x1000], count[0x1000], total[0x1000];
};
struct store_buffer_type {
	store_status_type status;// 0-inactive, 1-valid address, 2-waiting for TLB response
	UINT64 addr;
	UINT8 index; // dissambiguity index for transaction reordering in the bus 
	bus_xaction_type xaction;
	UINT16 xaction_id;
	page_cache_type cacheable;
	UINT64 valid_out_h, valid_out_l;
	UINT64 data_out[0x10];
	UINT8 amo;
	//	UINT8 branch_num;

	UINT64 time;
	UINT16 ROB_ptr;
};
struct allocate_unit {
	store_status_type status;
	bus_xaction_type xaction;// allocate versus load (AMO)
	UINT16 xaction_id;
	page_cache_type cacheable;
	INT64 addr;

	UINT64 clock;
	UINT16 ROB_ptr; // used for page faults
};
struct Store_type {
	store_buffer_type buffer[4];
	allocate_unit alloc[4];
	UINT alloc_ptr;
	UINT8 lock;
	UINT8 fault;
	INT64 fault_addr;
	UINT16 fault_ROB_ptr;

	addr_bus_type IO_track;
};
struct load_buffer {
	UINT8 status;// 0 = free, 1 in flight, 2 = victim
	UINT16 xaction_id;

	UINT64 addr;
	UINT8 size;
	UINT8 fp;
	UINT ROB_ptr;

	UINT8 index; // dissambiguity index for transaction reordering in the bus 

	UINT8 data_valid;
	UINT64 data[0x10];
	UINT64 clock;

	char rsp_valid;
};
struct Load_type {
	load_buffer buffer[4];
	UINT buffer_ptr;
	UINT8 tlb_rsp_pending;
	UINT8 fault;
	reg_bus rd_latch;
};
struct IntUnitType {
	UINT8 mul_q_valid[8]; // contains ROB_ptr
	UINT8 mul_q[8]; // contains ROB_ptr
	UINT8 mul_q_ptr;
};
enum branch_response :UINT8 {
	inactive = 0,
	//	reverse = 1,//default: not taken
	taken = 3,//default: taken
	unconditional = 7,
	service_fault = 8,
	stall = 15,
	halt = 31
};
struct decode_block {
	UINT8 valid;
	UINT16 buff[4];
};

struct decode_type {
	UINT8 index;
	UINT8 fault_release;
	UINT8 int_index;

	decode_block block[8];

	UINT64 perf_reg[0x20];
};
struct fp_tracker_type {
	UINT8 mode;
	UINT8 sign;
	UINT16 exp64;
	UINT16 exp32[2];
	UINT8 exp16[4];
	UINT8 valid;
	UINT8 ROB_ptr[4];
	uop_type uop[4];
};
struct fp_vars {
	INT64 stage0[64];
	INT64 stage1[32];
	INT64 stage2[16];
	INT64 stage3[8];
	INT64 stage4[4];
	INT64 stage5[2];
	INT64 stage6;

	fp_tracker_type tracker[7];
};

enum fetch_status_type :UINT8 {
	fetch_invalid = 0,
	fetch_valid = 1,
	fetch_incoming = 2,
	fetch_q_hit = 3,
	fetch_available = 4,
	fetch_fault = 5
};
typedef struct {
	fetch_status_type status;//0-invalid, 1- addr valid,  2- fetched, 4- data valid
	UINT16 xaction_id;
	UINT64 tag;
	UINT64 time;
}prefetch_buffer_entry_type;
typedef struct {
	prefetch_buffer_entry_type entry[4];

	UINT64 PC;
	UINT8 unique; // switched every time we switch demand pointer to not repeat xaction id.

	UINT8 sm; // state machine
	UINT data[4 * 0x20];// need 2 for alignment cache l; algorithm wants 2 valid to decode
	UINT8 data_count;
}prefetch_buffer_type;

typedef struct {
	UINT8 demand_ptr;
	UINT8 aux_ptr;
	prefetch_buffer_type buffer[4];
	prefetch_buffer_type victim;

	UINT8 reset_latch;
	UINT8 flush;
	UINT8 halt;

	UINT64 fault_addr;

	UINT8 idle_flag;
	UINT8 id_in_use[0x100];
}prefetcher_type;
typedef struct {
	UINT8 load_PC;
	UINT64 PC;
	UINT8 priviledge;
	UINT8 next_priviledge;
}retire_type;

void fprint_csr(FILE* debug_stream, control_status_reg_type csr);
void print_uop(FILE* debug_stream, ROB_entry_Type entry, Reg_Table_Type* reg_table, UINT clock);

void allocator_iA(Q_type* exec_q, ROB_Type* ROB, UINT8 load_PC, UINT8* fadd_q_id, UINT8* fmac_q_id, Reg_Table_Type* reg_table,
	UINT64 clock, UINT mhartid, UINT debug_core, param_type *param, FILE* debug_stream);
void store_amo_unit(reg_bus* rd, UINT16* exec_rsp, UINT16* exec_rsp2, R3_type* store_exec, UINT8 block_loads, addr_bus_type* logical_addr,
	data_bus_type* logical_data_out, data_bus_type* tlb_response, data_bus_type* physical_data2_in, data_bus_type* physical_data3_in,
	bus_w_snoop_signal1* bus2, UINT8 fault_release, UINT16 retire_num, UINT8 branch_clear, UINT8* active_IO, UINT8 prefetcher_busy, UINT8 priviledge,
	UINT64 clock, csr_type* csr, Store_type* store_var, UINT debug_core, param_type* param, FILE* debug_stream);
void OP_unit(reg_bus* rd, UINT16* exec_rsp, R_type* op_exec, UINT64 clock, UINT8 q_num, UINT mhartid, UINT debug_core, param_type param, FILE* debug_stream);
void load_unit_iA(reg_bus* rd, addr_bus_type* logical_addr, data_bus_type* TLB_response, data_bus_type* physical_data, data_bus_type* bus2_data_in,
	UINT16* exec_rsp, R_type* load_exec, UINT8 block_loads, UINT64 clock,
	UINT mhartid, Load_type* load_var, param_type *param, FILE* debug_stream);
void retire_unit(ROB_Type* ROB, Reg_Table_Type* reg_table, csr_type* csr, retire_type* retire, UINT8 reset, reg_bus* rd_decode, UINT64 clock, UINT mhartid, UINT debug_core, param_type* param, FILE* debug_stream);
void system_map_unit(csr_type* csr, reg_bus* rd, UINT16* exec_rsp, R_type* sys_exec, retire_type* retire, UINT8 reset,
	UINT64 clock, UINT debug_core, param_type param, FILE* debug_stream);
void branch_unit(reg_bus* rd, branch_vars* branch, UINT16* exec_rsp, UINT8* block_loads, branch_type* branch_exec, UINT load_pending, UINT16 retire_index, retire_type* retire, UINT8 stores_pending, UINT64 sbound, UINT64 mbound,
	UINT64 clock, UINT mhartid, UINT debug_core, param_type* param, FILE* debug_stream);

void OP_FP_unit(reg_bus* rd, UINT16* exec_rsp, R_type* fp_exec, UINT64 clock, char q_num, UINT mhartid, UINT debug_core, param_type param, FILE* debug_stream);
void fdiv_unit(Q_type* fdiv_q, ROB_Type* ROB, Reg_Table_Type* Reg, UINT8* delay);
void FMUL64_adders(reg_bus* rd, UINT16* exec_rsp, R3_type* fmul_exec, fp_vars* vars, UINT64 clock, UINT8 q_num, UINT mhartid, UINT debug_core, param_type param, FILE* debug_stream);

void reg_unit(R_type* load_exec, R3_type* store_exec, branch_type* branch_exec, R_type* fp_exec, R3_type* fmul_exec, R_type* OP_exec, R_type* sys_exec, R_type* LUI_AUIPC_exec,
	Reg_Table_Type* reg_table, reg_bus* rd_JALR, reg_bus* rd, UINT16* exec_rsp, ROB_Type* ROB, Q_type* exec_q, UINT8 prefetcher_idle, UINT stores_active, UINT8 block_loads, UINT8 priviledge,
	UINT64 clock, csr_type* csr, store_buffer_type* store_buffer, UINT debug_core, param_type* param, FILE* debug_stream);

