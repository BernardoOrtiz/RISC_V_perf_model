// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#pragma once
#include "ROB.h"

enum perf_count :UINT8 {
	load_count = 0x00,
	sload_count = 0x01,
	hload_count = 0x02,
	mload_count = 0x03,
	store_count = 0x04,
	sstore_count = 0x05,
	hstore_count = 0x06,
	mstore_count = 0x07,
	int_count = 0x08,
	sint_count = 0x09,
	hint_count = 0x0a,
	mint_count = 0x0b,
	float_count = 0x0c,
	sfloat_count = 0x0d,
	hfloat_count = 0x0e,
	mfloat_count = 0x0f,
	atomic_count = 0x10,
	satomic_count = 0x11,
	hatomic_count = 0x12,
	matomic_count = 0x13,
	branch_count = 0x10,
	sbranch_count = 0x11,
	hbranch_count = 0x12,
	mbranch_count = 0x13

};
struct shifter_response {
	branch_response msg;
	INT64 addr;
	UINT8 fault_in_service;
};
struct decode_shifter_struct {
	UINT8 valid, index;
	UINT16 buffer[16];
	UINT64 tag[16];

	shifter_response response;
};
UINT8 fence_check(ROB_entry_Type* ROB_ptr, UINT8* flush_write_posters, ROB_Type* ROB, csr_type* csr, UINT8 prefetcher_idle, UINT stop_prefetches, UINT stores_active, UINT64 clock, store_buffer_type* store_buffer, UINT mhartid, UINT debug_unit, param_type* param, FILE* debug_stream);
void fprint_decode_header(FILE* debug_stream, ROB_Type* ROB, ROB_entry_Type* ROB_ptr, UINT8 mhartid, param_type* param);
void print_uop_decode(FILE* debug_stream, ROB_entry_Type entry, UINT clock);

UINT8 decode_32b2(ROB_Type* ROB, UINT8* flush_write_posters, INT buffer, UINT64 tag, shifter_response* response, UINT8 prefetcher_idle, UINT stores_pending, UINT load_pending,
	UINT64 clock, branch_vars* branch, store_buffer_type* store_buffer, UINT* uPC);

void prefetch_unit(decode_shifter_struct* shifter, reg_bus* rd, addr_bus_type* logical_addr, data_bus_type* logical_data, data_bus_type* physical_data, data_bus_type* bus2_data_in, retire_type *retire, reg_bus* rd_branch,
	UINT8 ROB_stall, UINT8 on_branch, UINT mhartid, char interrupt, prefetcher_type* prefetcher, UINT8 reset, reg_bus* rd_JALR, UINT8* prefetcher_busy, UINT8 active_IO, UINT64 clock, UINT debug_core, param_type *param, FILE* debug_stream);
UINT8 decode_32b(ROB_Type* ROB, UINT64* perf_reg, UINT8* flush_write_posters, UINT8 priviledge, decode_shifter_struct* shifter, decode_type* decode_vars, UINT8 prefetcher_idle, UINT stores_pending, UINT load_pending,
	UINT64 clock, branch_vars* branch, store_buffer_type* store_buffer, UINT* uPC, UINT mhartid, UINT debug_core, param_type* param, FILE* debug_stream);
UINT8 decode_64b(ROB_Type* ROB, UINT8* flush_write_posters, decode_shifter_struct* shifter, decode_type* decode_vars, csr_type* csr, UINT8 prefetcher_idle, UINT stores_pending, UINT load_pending, UINT8 priviledge,
	UINT64 clock, branch_vars* branch, store_buffer_type* store_buffer, Reg_Table_Type* reg_table, UINT* uPC, UINT mhartid, UINT debug_core, param_type* param, FILE* debug_stream);
void decode32_RISC_V(ROB_Type* ROB, Q_type* exec_q, reg_bus* rd, UINT8* flush_write_posters, UINT* ecall_out, UINT ecall_in, decode_shifter_struct* shifter, UINT8 prefetcher_idle, decode_type* decode_vars, branch_vars* branch, IntUnitType* IntUnitVars, UINT stores_pending, UINT load_pending, retire_type* retire,
	UINT64 clock, csr_type* csr, UINT* uPC, store_buffer_type* store_buffer, UINT mhartid, UINT debug_core, param_type* param, FILE* debug_stream);
