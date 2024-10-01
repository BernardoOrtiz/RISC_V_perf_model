// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614


#include <windows.h>
#include <mmsystem.h>
#include <WinBase.h>
#include<string.h>

#include "memory_model.h"
#include "ROB.h"
#include "cache.h"
#include "TLB.h"
#include "front_end.h"

#include "compile_to_RISC_V.h"

struct core_var_type {
	ROB_Type* ROB; // static to hide data for easier MP modeling, will have impact on stack
	Reg_Table_Type* reg_table;
	csr_type csr[0x1000];
	TLB_type TLB[4];// allows for 2: Intel model, 4- AMD model with shadow caches
	sTLB_type sTLB;// allows for 2: Intel model, 4- AMD model with shadow caches
	prefetcher_type* prefetcher;
	decode_type decode_vars;
	Store_type store_var;
	Load_type load_var;

	Q_type* exec_q;
	UINT8 fdiv_delay;
	fp_vars fpu_vars;

	L0_code_cache_type L0_code_cache;
	L0_data_cache_type L0_data_cache;

	L2_cache_type L2_cache;

	bus_w_snoop_signal1 bus2[2], logical_bus;
	addr_bus_signal1 snoop_L2;
	internal_bus_signal1	bus1c;
	addr_bus_signal logical_data_bus_addr;
	data_bus_type logical_data_bus_data;
	UINT8 ROB_stall;
	UINT uPC;
//	UINT64 SRET_addr;
	UINT8  fadd_q_id;
	UINT8  fmac_q_id;
	reg_bus* rd;
	UINT16 exec_rsp[0x20];

	IntUnitType IntUnitVars;
	branch_vars branch;
	UINT8 block_loads, prefetcher_busy, flush_write_posters;
	decode_shifter_struct shifter;
	retire_type retire;
};

void synch_bus(internal_bus_signal* bus, UINT64 clock) {
	for (UINT8 i = 0; i < bus->size; i++) {
		copy_addr_bus_info(&bus->addr.in[i], &bus->addr.out[i], clock);
		copy_data_bus_info(&bus->data.in[i], &bus->data.out[i]);
		bus->addr.out[i].strobe = 0;
		bus->data.out[i].snoop_response = snoop_idle;
		bus->data.out[i].valid = 0;
		bus->data.out[i].cacheable = page_invalid;
	}
}
void synch_bus(bus_w_snoop_signal1* bus, UINT64 clock) {
	copy_addr_bus_info(&bus->addr.in, &bus->addr.out, clock);
	copy_addr_bus_info(&bus->snoop_addr.in, &bus->snoop_addr.out, clock);
	copy_data_bus_info(&bus->data_read.in, &bus->data_read.out);
	copy_data_bus_info(&bus->data_write.in, &bus->data_write.out);
	bus->addr.out.strobe = 0;
	bus->snoop_addr.out.strobe = 0;
	bus->data_read.out.snoop_response = snoop_idle;
	bus->data_write.out.snoop_response = snoop_idle;
	bus->data_read.out.valid = 0;
	bus->data_write.out.valid = 0;
	bus->data_read.out.cacheable = page_invalid;
	bus->data_write.out.cacheable = page_invalid;
}

void RISCV_compiler_ih(memory_space_type* memory_space, const char* asm_file, const char* MASM_file, const char* src_file, INT64 physical_PC, INT64 logical_PC, UINT64* FunctionStack_ct, INT64* user_entry_ptr, hw_pointers* pointers,
	INT64 logical_base, INT64 base, INT flags, compiler_var_type* compiler_var, operator_type* op_table, operator_type* branch_table, var_type_struct* var_type, param_type *param,
	parse_struct2_set* parse, label_type* labels, UINT8 unit_debug) {

	//	parse_transfer2->index = 0;
	parse->a.index = 0;
	UINT line_count = 0;
	if (flags & 1) {// flatten unit
		clear_constants("clear.txt", src_file, var_type, parse, param->satp);
		cull_branches("culled_branches.txt", parse);
		inline_C("inline.txt", "culled_branches.txt", flags, var_type, parse, 6);
		parenthesis_reduction("clean.txt", "inline.txt", op_table, parse, unit_debug);
		math_reduction("math_reduct_h.txt", "clean.txt", parse, unit_debug);
		cull_branches("culled_branches2.txt", parse);
		parenthesis_reduction("clean3_handler.txt", "culled_branches2.txt", op_table, parse, unit_debug);
		compile_to_RISCV_Masm(&parse->a, MASM_file, FunctionStack_ct, "clean3_handler.txt", &parse->b, memory_space, pointers, logical_base, base, flags, compiler_var, op_table, branch_table, param, logical_PC, unit_debug, 1);
	}
	else {
		compile_to_RISCV_Masm(&parse->a, MASM_file, FunctionStack_ct, src_file, &parse->b, memory_space, pointers, logical_base, base, flags, compiler_var, op_table, branch_table, param, logical_PC, unit_debug, 1);
	}
	compile_to_RISCV_asm(memory_space, asm_file, MASM_file, parse, physical_PC, logical_PC, labels, param, 1);
}
void RISCV_compiler(memory_space_type* memory_space, const char* asm_file, const char* MASM_file, const char* src_file, INT64 physical_PC, INT64 logical_PC, UINT64* FunctionStack_ct, INT64* user_entry_ptr, hw_pointers* pointers,
	INT64 logical_base, INT64 base, compiler_var_type* compiler_var, operator_type* op_table, operator_type* branch_table, var_type_struct* var_type, param_type *param,
	parse_struct2_set* parse, label_type* labels, UINT8 unit_debug) {

	//	parse_transfer2->index = 0;
	parse->a.index = 0;
	UINT line_count = 0;
	// need to include h files first
	var_index_base_inner_loop("unroll12b_OS.txt", src_file, var_type, parse, compiler_var);
	compile_to_RISCV_Masm(&parse->a, MASM_file, FunctionStack_ct, "unroll12b_OS.txt", &parse->b, memory_space, pointers, logical_base, base, 0, compiler_var, op_table, branch_table, param, logical_PC, unit_debug, 1);
	compile_to_RISCV_asm(memory_space, asm_file, MASM_file, parse, physical_PC, logical_PC, labels,param, 1);
}
void RISCV_compiler2(memory_space_type* memory_space, const char* asm_file, const char* MASM_file, const char* src_file, INT64 physical_PC, INT64 logical_PC, UINT64* FunctionStack_ct, INT64* user_entry_ptr, hw_pointers* pointers,
	INT64 logical_base, INT64 base, INT flags, compiler_var_type* compiler_var, operator_type* op_table, operator_type* branch_table, var_type_struct* var_type, param_type *param,
	parse_struct2_set* parse, label_type* labels, UINT8 unit_debug) {

	//	parse_transfer2->index = 0;

	store_load_ellimination("store_load.txt", src_file, op_table, var_type);
	SwitchFill("inline2.txt", "store_load.txt", flags, var_type, parse, 0x10);
	parenthesis_reduction("clean2.txt", "inline2.txt", op_table, parse, unit_debug);
	math_reduction("math_reduct.txt", "clean2.txt", parse, 1);
	format("format.txt", "math_reduct.txt", var_type, parse);
	parenthesis_reduction("clean4.txt", "format.txt", op_table, parse, unit_debug);
	split_line("split.txt", "clean4.txt", parse, var_type);
	// 16b array to 128 reg local variable conv - reduce cache accesses
	FunctCall_ConstEllim("inline3.txt", "split.txt", flags, var_type, parse, 0);
	FunctCall_of_FunctCall_Elli("inline4.txt", "inline3.txt", flags, var_type, parse, 0);
	math_reduction("math_reduct5.txt", "inline4.txt", parse, 1);

	nonaligned_load128_preload("ua_load128.txt", "math_reduct5.txt", var_type, parse, compiler_var);

	var_index_base_inner_loop("var_index_base.txt", "math_reduct5.txt", var_type, parse, compiler_var);
	// force inline
	MarkedInline("marked_inline.txt", "var_index_base.txt", flags, var_type, parse, 0x10);
	// aligned prefetches are now possible

	math_reduction("math_reduct2.txt", "marked_inline.txt", parse, 0);
//	array16toReg128("array16toreg128.txt", "math_reduct2.txt", parse, unit_debug);

//	ScalarPtrEllimination("scaler_ptr.txt", "array16toreg128.txt", parse, unit_debug);
	ScalarPtrEllimination("scaler_ptr.txt", "math_reduct2.txt", parse, unit_debug);
	math_reduction("math_reduct3.txt", "scaler_ptr.txt", parse, 0);
	VarDeclarationEllimination4loop("varDeclarationEllimination.txt", "math_reduct3.txt", parse, unit_debug);
	parenthesis_reduction("clean5.txt", "varDeclarationEllimination.txt", op_table, parse, unit_debug);
	math_reduction("math_reduct4.txt", "clean5.txt", parse, 1);
	// elliminate scalar pointers < 0x800 offset
	// pointer consolidation after inlining multi-funct calls
	// add 128b int prefetching for _fp16 var here, I think
//	variable_substitution("var_substitution.txt", "math_reduct4.txt", parse);// NOTE: inserts safety parenthesis

//	parenthesis_reduction("clean6.txt", "var_substitution.txt", op_table, parse, unit_debug);

	split_line("split2.txt", "math_reduct4.txt", parse, var_type);
	//	InlineShortFunct("inline3.txt", "split2.txt", flags, var_type, parse, 0x10);//2978
	//	parenthesis_reduction("clean7.txt", "inline3.txt", op_table, parse, unit_debug);
	parenthesis_reduction("clean7.txt", "split2.txt", op_table, parse, unit_debug);

	declare_multi2single(&parse->b, &parse->a, "declare_m2s.txt", "clean7.txt");

	compile_to_RISCV_Masm(&parse->a, MASM_file, FunctionStack_ct, "declare_m2s.txt", &parse->b, memory_space, pointers, logical_base, base, flags, compiler_var, op_table, branch_table, param, logical_PC, unit_debug, 0);
	compile_to_RISCV_asm(memory_space, asm_file, MASM_file, parse, physical_PC, logical_PC, labels, param, 1);
}
void data_bus_complex(reg_bus* rd, UINT16* exec_rsp, bus_w_snoop_signal1* bus2, UINT* load_pending, UINT8 block_loads, R_type* load_exec, R3_type* store_exec, branch_vars* branch, UINT8 fault_release, csr_type* csr, core_var_type* core_var, TLB_type* TLB, UINT16 retire_num, UINT8 branch_clear, UINT8 *active_IO, UINT8 prefetcher_busy, UINT8 priviledge, UINT64 clock, UINT debug_core, param_type *param, FILE* debug_stream) {
	int debug = 0;

	addr_bus_signal* logical_addr = &core_var->logical_data_bus_addr;

	L0_data_cache_type* L0_data_cache = &core_var->L0_data_cache;
	Store_type* store_var = &core_var->store_var;
	Load_type* load_var = &core_var->load_var;

	data_bus_type* logical_data = &core_var->logical_data_bus_data;

	data_bus_type TLB_response[4];
	data_bus_type p_data_bus[4];
	addr_bus_type p_addr_bus[4];
	TLB_response[0].snoop_response = snoop_idle;
	TLB_response[1].snoop_response = snoop_idle;
	TLB_response[2].snoop_response = snoop_idle;
	TLB_response[3].snoop_response = snoop_idle;
	p_data_bus[0].snoop_response = snoop_idle;
	p_data_bus[1].snoop_response = snoop_idle;
	p_data_bus[2].snoop_response = snoop_idle;
	p_data_bus[3].snoop_response = snoop_idle;
	p_addr_bus[0].strobe = 0;
	p_addr_bus[1].strobe = 0;
	p_addr_bus[2].strobe = 0;
	p_addr_bus[3].strobe = 0;

	for (UINT8 i = 0; i < 4; i++) {
		copy_addr_bus_info(&logical_addr->in[i], &logical_addr->out[i], clock);
		logical_addr->out[i].strobe = 0;
	}
	UINT mhartid = csr[csr_mhartid].value;

	if (mhartid == 0) {
		if (clock >= 0x1a4)
			debug++;
	}
	data_tlb_unit(p_addr_bus, TLB_response, logical_addr->in, bus2, clock, TLB, mhartid, param, debug_stream);
	L0_data_cache_unit(p_data_bus, bus2, logical_addr->in, logical_data, p_addr_bus, TLB_response, csr[csr_mhartid].value, clock, L0_data_cache, &core_var->L2_cache, debug_core, param, debug_stream);// AMD: 3 clock latency, 1 clock arb (guarantee data bus), 1 clock tag, 1 clock data
	store_amo_unit(&rd[store_q_id], &exec_rsp[store_q_id], &exec_rsp[store_q_id2], store_exec, block_loads,
		&logical_addr->out[2], logical_data, &TLB_response[2], &p_data_bus[2], &p_data_bus[3],
		bus2, fault_release, retire_num, branch_clear, active_IO, prefetcher_busy, priviledge,
		clock, csr, store_var, debug_core, param, debug_stream); // read/write - control
	load_unit_iA(&rd[load_q_id], &logical_addr->out[1], &TLB_response[1], &p_data_bus[1], &bus2->data_read.in,
		&exec_rsp[load_q_id], load_exec, block_loads, clock, csr[csr_mhartid].value, load_var, param, debug_stream); // integer
	load_pending[0] = (load_var->buffer[0].status || load_var->buffer[1].status || load_var->buffer[2].status || load_var->buffer[3].status || rd[load_q_id].strobe);
}

void init_L2_bank(L2_block_type* bank, UINT8 set_shift, UINT8 tag_shift) {
	bank->tags = (L2_tag_type*)malloc(0x800 * sizeof(L2_tag_type));
	bank->array.line = (UINT64***)malloc(8 * sizeof(UINT64**));
	bank->array.read_write = 0;	bank->set_shift = set_shift;
	bank->tag_shift = tag_shift;
	bank->array.array_busy_snoop_stall = 0;
	for (UINT8 j = 0; j < 8; j++) {
		bank->array.select[j] = 0;
		bank->array.data[j].snoop_response = snoop_idle;
		bank->array.line[j] = (UINT64**)malloc(0x800 * sizeof(UINT64*));
		for (UINT k = 0; k < 0x800; k++) {
			bank->array.line[j][k] = (UINT64*)malloc(0x10 * sizeof(UINT64));
		}
	}
	for (UINT j = 0; j < 0x800; j++) {
		bank->tags[j].way_ptr = 0;
		bank->tags[j].in_use = 0;
		bank->tags[j].state[0] = invalid_line;
		bank->tags[j].state[1] = invalid_line;
		bank->tags[j].state[2] = invalid_line;
		bank->tags[j].state[3] = invalid_line;
		bank->tags[j].state[4] = invalid_line;
		bank->tags[j].state[5] = invalid_line;
		bank->tags[j].state[6] = invalid_line;
		bank->tags[j].state[7] = invalid_line;
	}
}

int main()
{
 //   std::cout << "Hello World!\n";

	int debug = 0;

	var_type_struct *var_type = (var_type_struct*)malloc(var_type_count * sizeof(var_type_struct));
//	char** var_type = (char**)malloc(var_type_count * sizeof(char*));
//	for (UINT8 i = 0; i < var_type_count; i++) {
//		var_type[i] = (char*)malloc(0x10 * sizeof(char));
//	}
	sprintf_s(var_type[0].name , "void");
	sprintf_s(var_type[1].name, "short");
	sprintf_s(var_type[2].name, "int");
	sprintf_s(var_type[3].name, "INT");
	sprintf_s(var_type[4].name, "INT8");
	sprintf_s(var_type[5].name, "INT16");
	sprintf_s(var_type[6].name, "INT32");
	sprintf_s(var_type[7].name, "INT64");
	sprintf_s(var_type[8].name, "INT128");
	sprintf_s(var_type[9].name, "UINT");
	sprintf_s(var_type[10].name, "UINT8");
	sprintf_s(var_type[11].name, "UINT16");
	sprintf_s(var_type[12].name, "UINT32");
	sprintf_s(var_type[13].name, "UINT64");
	sprintf_s(var_type[14].name, "UINT128");
	sprintf_s(var_type[15].name, "float");
	sprintf_s(var_type[16].name, "double");
	sprintf_s(var_type[17].name, "_fp16");


	var_type[0].type = void_enum;
	var_type[1].type = int16_enum;
	var_type[2].type = int32_enum;
	var_type[3].type = int32_enum;
	var_type[4].type = int8_enum;
	var_type[5].type = int16_enum;
	var_type[6].type = int32_enum;
	var_type[7].type = int64_enum;
	var_type[8].type = int128_enum;
	var_type[9].type = uint32_enum;
	var_type[10].type = uint8_enum;
	var_type[11].type = uint16_enum;
	var_type[12].type = uint32_enum;
	var_type[13].type = uint64_enum;
	var_type[14].type = uint128_enum;
	var_type[15].type = fp32_enum;
	var_type[16].type = fp64_enum;
	var_type[17].type = fp16_enum;

	operator_type* op_table = (operator_type*)malloc(0x10*sizeof(operator_type));
	sprintf_s(op_table[0x00].symbol, "+");
	sprintf_s(op_table[0x01].symbol, "-");
	sprintf_s(op_table[0x02].symbol, "*");
	sprintf_s(op_table[0x03].symbol, "/");
	sprintf_s(op_table[0x04].symbol, "&");
	sprintf_s(op_table[0x05].symbol, "|");
	sprintf_s(op_table[0x06].symbol, "^");
	sprintf_s(op_table[0x07].symbol, "<<");
	sprintf_s(op_table[0x08].symbol, ">>");
	sprintf_s(op_table[0x09].symbol, "|=");
	sprintf_s(op_table[0x0a].symbol, "^=");
	sprintf_s(op_table[0x0b].symbol, "&=");
	sprintf_s(op_table[0x0c].symbol, "+=");
	sprintf_s(op_table[0x0d].symbol, "-=");

	sprintf_s(op_table[0x0e].symbol, "&&");
	sprintf_s(op_table[0x0f].symbol, "||");

	sprintf_s(op_table[0x00].opcode, "add");
	sprintf_s(op_table[0x01].opcode, "sub");
	sprintf_s(op_table[0x02].opcode, "mul");
	sprintf_s(op_table[0x03].opcode, "div");
	sprintf_s(op_table[0x04].opcode, "and");
	sprintf_s(op_table[0x05].opcode, "or");
	sprintf_s(op_table[0x06].opcode, "xor");
	sprintf_s(op_table[0x07].opcode, "sll");
	sprintf_s(op_table[0x08].opcode, "srl");
	sprintf_s(op_table[0x09].opcode, "or");
	sprintf_s(op_table[0x0a].opcode, "xor");
	sprintf_s(op_table[0x0b].opcode, "and");
	sprintf_s(op_table[0x0c].opcode, "add");
	sprintf_s(op_table[0x0d].opcode, "sub");

	sprintf_s(op_table[0x0e].opcode, "and");//logical
	sprintf_s(op_table[0x0f].opcode, "or");// logical

	// will add branching to opcodes, 
	// logical versus arithmetic (boolean vs bitwise response)
	op_table[0x00].type = 0;
	op_table[0x01].type = 0;
	op_table[0x02].type = 0;
	op_table[0x03].type = 0;
	op_table[0x04].type = 0;
	op_table[0x05].type = 0;
	op_table[0x06].type = 0;
	op_table[0x07].type = 0;
	op_table[0x08].type = 0;
	op_table[0x09].type = 0;
	op_table[0x0a].type = 0;
	op_table[0x0b].type = 0;
	op_table[0x0c].type = 0;
	op_table[0x0d].type = 0;

	op_table[0x0e].type = 1;
	op_table[0x0f].type = 1;

	operator_type branch_table[0x10];
	sprintf_s(branch_table[0x00].symbol, "<");
	sprintf_s(branch_table[0x01].symbol, ">");
	sprintf_s(branch_table[0x02].symbol, "<=");
	sprintf_s(branch_table[0x03].symbol, ">=");
	sprintf_s(branch_table[0x04].symbol, "==");
	sprintf_s(branch_table[0x05].symbol, "!=");

	sprintf_s(branch_table[0x00].rev_opcode, "bge");
	sprintf_s(branch_table[0x01].rev_opcode, "ble");
	sprintf_s(branch_table[0x02].rev_opcode, "bgt");
	sprintf_s(branch_table[0x03].rev_opcode, "blt");
	sprintf_s(branch_table[0x04].rev_opcode, "bne");
	sprintf_s(branch_table[0x05].rev_opcode, "beq");

	sprintf_s(branch_table[0x00].opcode, "blt");
	sprintf_s(branch_table[0x01].opcode, "bgt");
	sprintf_s(branch_table[0x02].opcode, "ble");
	sprintf_s(branch_table[0x03].opcode, "bge");
	sprintf_s(branch_table[0x04].opcode, "beq");
	sprintf_s(branch_table[0x05].opcode, "bne");

	memory_space_type* memory_space = (memory_space_type*)malloc(sizeof(memory_space_type));
	memory_space->reset = (page_4K_type*)malloc(1 * sizeof(page_4K_type));
	memory_space->reset->memory = (UINT64*)malloc(0x40000 * sizeof(UINT64));
	for (UINT i = 0; i < 0x20000; i++)memory_space->reset->memory[i] = 0x13;// NOP

	memory_space->clic = (page_4K_type*)malloc(1 * sizeof(page_4K_type));
	memory_space->clic[0].memory = (UINT64*)malloc(0x20000 * sizeof(UINT64));
	for (UINT i = 0; i < 0x20000; i++) 	memory_space->clic[0].memory[i] = 0x13;// NOP

	memory_space->page = (page_4K_type*)malloc(0x900 * sizeof(page_4K_type));// allocating memory by 4K pages	

	for (UINT i = 0; i < 0x900; i++) memory_space->page[i].type = page_free;
	// 0x86000000
	//	Supervisor space (OS)
	// 0x80000000 // below is non-cacheable
	//	I/O space
	// 0x10000000
	//	machine space (UC) - includes BIOS
	// 0x100000000

	INT64 user_entry_ptr = 0x86400000;

	INT64 PC = 0x86400000, SPC = 0x80000000;
	INT64 physical_PC = 0x86400000, physical_SPC = 0x80000000;
	INT64 reset_PC = 0x00001000;

	INT64 CLIC_PC = 0x0000000002000000;//CLIC
	INT64 physical_CLIC_PC = 0x0000000002000000;//CLIC
	INT64 SIH_PC = 0x0000000080200000;//supervisor interrupt handler PC
	INT64 physical_SIH_PC = 0x0000000080200000;//supervisor interrupt handler PC
	UINT64 FunctionStack_ct = 0;

	compiler_var_type compiler_var;
	compiler_var.global_list_base = (VariableListEntry*)malloc(sizeof(VariableListEntry));
	compiler_var.global_list_base->next = compiler_var.global_list_base;
	compiler_var.global_list_base->last = compiler_var.global_list_base;
	compiler_var.gp_offset = 0;
	compiler_var.OS_gp_offset = 0;
	compiler_var.switch_count = 0;

	INT64 page_table_base = 0x80400000;
	param_type param;
	load_params(&param, "params.txt");
	param.satp = 0x9000000000000000 | (page_table_base >> 12);

	hw_pointers pointers;
	pointers.sp = 0x100000000;
	pointers.gp = 0x0f0000000;
	pointers.tp[0] = 0x86400000 - 0x1200000;
	pointers.OS_gp = 0x86400000;



	char malloc_filename[0x10] = "malloc.txt";
//	FILE* malloc_file = fopen(malloc_filename, "w");
	FILE* malloc_file;
	fopen_s(&malloc_file, malloc_filename, "w");

	allocate_data_2M_page(memory_space, 0x80000000, 0x03, page_table_base, (param.satp >> 60), malloc_file);// allocates non-swappable page, OS
	allocate_data_2M_page(memory_space, 0x80200000, 0x03, page_table_base, (param.satp >> 60), malloc_file);// allocates non-swappable page, System Interrupt Handler
	allocate_data_2M_page(memory_space, pointers.OS_gp, 0x0f, page_table_base, (param.satp >> 60), malloc_file);// code space
	allocate_data_2M_page(memory_space, pointers.OS_gp + (1 * 0x200000), 0x0f, page_table_base, (param.satp >> 60), malloc_file);// code space
	for (UINT8 i = 0; i < core_count; i++) {// 8 threads
		allocate_data_2M_page(memory_space, page_table_base + ((1 + i) << 21), 0x03, page_table_base, (param.satp >> 60), malloc_file);// allocates non-swappable page, System Interrupt Handler
		allocate_data_2M_page(memory_space, pointers.OS_gp - ((1 + i) * 0x200000), 0x03, page_table_base, (param.satp >> 60), malloc_file);// used in environement call from user
		allocate_data_2M_page(memory_space, pointers.OS_gp - ((9 + i) * 0x200000), 0x03, page_table_base, (param.satp >> 60), malloc_file);// tp must be non-swappable space - not page misses in the handlers: environment call to user
		allocate_data_2M_page(memory_space, pointers.sp - ((1 + i) * 0x200000), 0x0b, page_table_base, (param.satp >> 60), malloc_file);//SP
	}

	for (UINT8 i = 0; i < 0x8; i++) {// 3 images: 2M pixels (16b fp) + slop
		allocate_data_2M_page(memory_space, pointers.gp - ((1 + i) * 0x200000), 0x0b, page_table_base, (param.satp >> 60), malloc_file);
	}
	for (UINT8 i = 0; i < 0x8; i++)
		allocate_data_2M_page(memory_space, pointers.gp - 0x001000000 - ((1 + i) * 0x200000), 0x0b, page_table_base, (param.satp >> 60), malloc_file);//in1
	for (UINT8 i = 0; i < 0x8; i++)
		allocate_data_2M_page(memory_space, pointers.gp - 0x002000000 - ((1 + i) * 0x200000), 0x0b, page_table_base, (param.satp >> 60), malloc_file);//in2
	for (UINT8 i = 0; i < 0x8; i++)
		allocate_data_2M_page(memory_space, pointers.gp - 0x003000000 - ((1 + i) * 0x200000), 0x0b, page_table_base, (param.satp >> 60), malloc_file);//out

	fclose(malloc_file);

	parse_struct2_set parse;
	parse.a.line = (line_struct*)malloc(0x20000 * sizeof(line_struct));
	parse.b.line = (line_struct*)malloc(0x20000 * sizeof(line_struct));
	parse.select = 0;

	label_type* labels = (label_type*)malloc(0x10000 * sizeof(label_type));

	RISCV_compiler(memory_space, "reset_asm.txt", "reset_Masm.txt", "reset.txt",
		reset_PC, reset_PC, &FunctionStack_ct, &user_entry_ptr, &pointers, 0x86400000, page_table_base, &compiler_var, op_table, branch_table, var_type, &param, &parse, labels, 0);
	RISCV_compiler(memory_space, "OS_h_asm.txt", "OS_h_Masm.txt", "OS64_h.txt",
		physical_SPC, SPC, &FunctionStack_ct, &user_entry_ptr, &pointers, 0x86400000, page_table_base, &compiler_var, op_table, branch_table, var_type, &param, &parse, labels, 0);
	RISCV_compiler(memory_space, "OS_asm.txt", "OS_Masm.txt", "OS.txt",
		physical_SPC, SPC, &FunctionStack_ct, &user_entry_ptr, &pointers, 0x86400000, page_table_base, &compiler_var, op_table, branch_table, var_type, &param, &parse, labels, 0);
	RISCV_compiler_ih(memory_space, "M_handlers_asm.txt", "M_handlers_Masm.txt", "InterruptExceptionHandler.txt",
		physical_CLIC_PC, CLIC_PC, &FunctionStack_ct, &user_entry_ptr, &pointers, 0x86400000, page_table_base, 1, &compiler_var, op_table, branch_table, var_type, &param, &parse, labels, 0);
	RISCV_compiler_ih(memory_space, "S_handlers_asm.txt", "S_handlers_Masm.txt", "InterruptExceptionHandler.txt",
		physical_SIH_PC, SIH_PC, &FunctionStack_ct, &user_entry_ptr, &pointers, 0x86400000, page_table_base, 1, &compiler_var, op_table, branch_table, var_type, &param, &parse, labels, 1);

	RISCV_compiler2(memory_space, "v_decode_asm.txt", "v_decode_Masm.txt", "v_decode.txt",
		physical_PC, PC, &FunctionStack_ct, &user_entry_ptr, &pointers, 0x86400000, page_table_base, 0, &compiler_var, op_table, branch_table, var_type, &param, &parse, labels, 1);
	free(parse.a.line);
	free(parse.b.line);
	free(labels);

	//	UINT core_count = 16;

	core_var_type* core_var;
	core_var = (core_var_type*)malloc(core_count * sizeof(core_var_type));

	for (UINT mhartid = 0; mhartid < core_count; mhartid++) {
		core_var[mhartid].retire.load_PC = 0;
		core_var[mhartid].shifter.response.msg = inactive;
		core_var[mhartid].decode_vars.index = 0;
		for (UINT16 j = 0; j < 0x1000; j++) {
			core_var[mhartid].branch.pred[j] = 1;
			core_var[mhartid].branch.count[j] = 0;
			core_var[mhartid].branch.total[j] = 255;
		}

		core_var[mhartid].ROB_stall = 0;
		core_var[mhartid].ROB = (ROB_Type*)malloc(sizeof(ROB_Type));// longer initialization, but will only have size pointer impacting stack, as opposed to entire structure
		core_var[mhartid].ROB->fence = 0;
		core_var[mhartid].ROB->branch_start = core_var[mhartid].ROB->branch_stop = 0;
		core_var[mhartid].ROB->decode_ptr = core_var[mhartid].ROB->allocate_ptr = 0;
		core_var[mhartid].ROB->retire_ptr_in = core_var[mhartid].ROB->retire_ptr_out = 255;
		core_var[mhartid].ROB->fault_halt = 0;

		core_var[mhartid].IntUnitVars.mul_q_ptr = 0;
		for (UINT8 i = 0; i < 8; i++) core_var[mhartid].IntUnitVars.mul_q_valid[i] = 0;

		core_var[mhartid].reg_table = (Reg_Table_Type*)malloc(sizeof(Reg_Table_Type));
		sprintf_s(core_var[mhartid].reg_table->x_reg[0].name, "zero");
		sprintf_s(core_var[mhartid].reg_table->x_reg[1].name, "ra");
		sprintf_s(core_var[mhartid].reg_table->x_reg[2].name, "sp");
		sprintf_s(core_var[mhartid].reg_table->x_reg[3].name, "gp");
		sprintf_s(core_var[mhartid].reg_table->x_reg[4].name, "tp");
		sprintf_s(core_var[mhartid].reg_table->x_reg[5].name, "t0");
		sprintf_s(core_var[mhartid].reg_table->x_reg[6].name, "s3");
		sprintf_s(core_var[mhartid].reg_table->x_reg[7].name, "s4");
		sprintf_s(core_var[mhartid].reg_table->x_reg[8].name, "s0/fp");
		sprintf_s(core_var[mhartid].reg_table->x_reg[9].name, "s1");
		sprintf_s(core_var[mhartid].reg_table->x_reg[10].name, "a0");
		sprintf_s(core_var[mhartid].reg_table->x_reg[11].name, "a1");
		sprintf_s(core_var[mhartid].reg_table->x_reg[12].name, "a2");
		sprintf_s(core_var[mhartid].reg_table->x_reg[13].name, "a3");
		sprintf_s(core_var[mhartid].reg_table->x_reg[14].name, "s2");
		sprintf_s(core_var[mhartid].reg_table->x_reg[15].name, "t1");
		sprintf_s(core_var[mhartid].reg_table->x_reg[16].name, "s5");
		sprintf_s(core_var[mhartid].reg_table->x_reg[17].name, "s6");
		sprintf_s(core_var[mhartid].reg_table->x_reg[18].name, "s7");
		sprintf_s(core_var[mhartid].reg_table->x_reg[19].name, "s8");
		sprintf_s(core_var[mhartid].reg_table->x_reg[20].name, "s9");
		sprintf_s(core_var[mhartid].reg_table->x_reg[21].name, "s10");
		sprintf_s(core_var[mhartid].reg_table->x_reg[22].name, "s11");
		sprintf_s(core_var[mhartid].reg_table->x_reg[23].name, "s12");
		sprintf_s(core_var[mhartid].reg_table->x_reg[24].name, "s13");
		sprintf_s(core_var[mhartid].reg_table->x_reg[25].name, "s14");
		sprintf_s(core_var[mhartid].reg_table->x_reg[26].name, "s15");
		sprintf_s(core_var[mhartid].reg_table->x_reg[27].name, "s16");
		sprintf_s(core_var[mhartid].reg_table->x_reg[28].name, "s17");
		sprintf_s(core_var[mhartid].reg_table->x_reg[29].name, "s18");
		sprintf_s(core_var[mhartid].reg_table->x_reg[30].name, "s19");
		sprintf_s(core_var[mhartid].reg_table->x_reg[31].name, "s20");

		for (UINT16 j = 0; j < 0x1000; j++) core_var[mhartid].csr[j].value = 0;
		// program counter
		core_var[mhartid].csr[csr_mhartid].value = mhartid;//protection bits; enter 0x80000001 to enable paging
		core_var[mhartid].csr[csr_misa].value = param.misa;
		core_var[mhartid].csr[csr_mbound].value = 0x80000000;
		core_var[mhartid].csr[csr_sbound].value = 0x86400000;
		core_var[mhartid].csr[csr_iobase].value = 0x10000000;

		for (UINT16 j = 0; j < 2; j++) {
			core_var[mhartid].TLB[j].L2_set = 0;
			for (UINT16 k = 0; k < 0x80; k++) {
				core_var[mhartid].TLB[j].L2[k].way_ptr = 0;
				for (UINT8 way = 0; way < 8; way++) {
					core_var[mhartid].TLB[j].L2[k].way[way].directory = 0;
					core_var[mhartid].TLB[j].L2[k].way[way].directory_h = 0;
					core_var[mhartid].TLB[j].L2[k].way[way].p_addr = 0;
					core_var[mhartid].TLB[j].L2[k].way[way].p_addr_h = 0;
					core_var[mhartid].TLB[j].L2[k].way[way].type = 0;
				}
			}

			core_var[mhartid].TLB[j].lock = 0;
		}
		// code TLB initialization
		core_var[mhartid].TLB[0].way_ptr1 = 0;
		for (UINT i = 0; i < 0x40; i++)		core_var[mhartid].TLB[0].L1[i].directory = 0;
		core_var[mhartid].TLB[0].delay.snoop_response = snoop_idle;
		core_var[mhartid].TLB[0].walk.reg = 0;
		// code snoop filter (shadow TLB)
		core_var[mhartid].sTLB.walk.lock = 0;
		core_var[mhartid].sTLB.walk.lockout = 0;
		core_var[mhartid].sTLB.walk.est_lockout = 0;

		for (UINT8 i = 0; i < 4; i++) core_var[mhartid].sTLB.cL1.sm[i] = 0;// 4 ports
		core_var[mhartid].sTLB.cL1.way_count = 512; // 512 entry
		core_var[mhartid].sTLB.cL1.way_ptr = 0;
		core_var[mhartid].sTLB.cL1.way = (TLB_cache_way_type*)malloc(core_var[mhartid].sTLB.cL1.way_count * sizeof(TLB_cache_way_type));
		for (UINT i = 0; i < core_var[mhartid].sTLB.cL1.way_count; i++) 			core_var[mhartid].sTLB.cL1.way[i].directory = 0;

		for (UINT8 i = 0; i < 4; i++) core_var[mhartid].sTLB.dL1.sm[i] = 0;// 4 ports
		core_var[mhartid].sTLB.dL1.way_count = 512; // 512 entry
		core_var[mhartid].sTLB.dL1.way_ptr = 0;
		core_var[mhartid].sTLB.dL1.way = (TLB_cache_way_type*)malloc(core_var[mhartid].sTLB.dL1.way_count * sizeof(TLB_cache_way_type));
		for (UINT i = 0; i < core_var[mhartid].sTLB.dL1.way_count; i++) 			core_var[mhartid].sTLB.dL1.way[i].directory = 0;

		core_var[mhartid].sTLB.delay.snoop_response = snoop_idle;
		core_var[mhartid].sTLB.walk.reg_access_out = 0;
		core_var[mhartid].sTLB.walk.reg_access_in = 0;
		core_var[mhartid].sTLB.walk.lockout = 0;
		core_var[mhartid].sTLB.walk.est_lockout = 0;

		for (UINT8 i = 0; i < 4; i++)	core_var[mhartid].sTLB.ext_snoop_track[i].strobe = 0;
		for (UINT8 i = 0; i < 4; i++)	core_var[mhartid].sTLB.snoop_latch[i].strobe = 0;
		core_var[mhartid].sTLB.ext_snoop_ptr = 0;
		core_var[mhartid].sTLB.snoop_latch_ptr = 0;

		// data TLB initialization - using ZEN3 numbers
		core_var[mhartid].TLB[1].way_ptr1 = 0;
		for (UINT i = 0; i < 0x40; i++) 	core_var[mhartid].TLB[1].L1[i].directory = 0;
		core_var[mhartid].TLB[1].delay.snoop_response = snoop_idle;
		core_var[mhartid].TLB[1].walk.reg = 0;
		// data TLB initialization - shadow
		core_var[mhartid].prefetcher = (prefetcher_type*)malloc(sizeof(prefetcher_type));
//		core_var[mhartid].prefetcher->shift_buf_valid = 0;
		core_var[mhartid].shifter.valid = 0;
		for (UINT16 i = 0; i < 0x100; i++) core_var[mhartid].prefetcher->id_in_use[i] = 0;

		core_var[mhartid].decode_vars.int_index = 0;
		core_var[mhartid].shifter.response.fault_in_service = 0;
		for (UINT8 i = 0; i < 8; i++) {
			core_var[mhartid].decode_vars.block[i].valid = 0;
		}
		for (UINT8 i = 0; i < 0x20; i++) {
			core_var[mhartid].decode_vars.perf_reg[i] = 0;
		}

		core_var[mhartid].store_var.alloc_ptr = 0;
		core_var[mhartid].store_var.fault = 0;
		core_var[mhartid].store_var.lock = 0;
		core_var[mhartid].store_var.IO_track.strobe = 0;
		for (UINT8 i = 0; i < 4; i++) {
			core_var[mhartid].store_var.buffer[i].status = store_inactive;
			core_var[mhartid].store_var.buffer[i].valid_out_l = 0;
			core_var[mhartid].store_var.buffer[i].valid_out_h = 0;
			core_var[mhartid].store_var.buffer[i].index = 0;

			core_var[mhartid].store_var.alloc[i].status = store_inactive;
			core_var[mhartid].store_var.alloc[i].clock = 0;
		}

		core_var[mhartid].load_var.buffer_ptr = 0;
		core_var[mhartid].load_var.tlb_rsp_pending = 0;
		core_var[mhartid].load_var.fault = 0;
		for (UINT8 i = 0; i < 4; i++) {
			core_var[mhartid].load_var.buffer[i].status = 0;
			core_var[mhartid].load_var.buffer[i].data_valid = 0;
			core_var[mhartid].load_var.buffer[i].index = 0;
			core_var[mhartid].load_var.buffer[i].ROB_ptr = 0;

			//				core_var[mhartid].load_var.buffer[i].fault = 0;
			//				core_var[mhartid].load_var.buffer[i].fault_addr = 0;
		}
		core_var[mhartid].load_var.rd_latch.strobe = 0;

		core_var[mhartid].exec_q = (Q_type*)malloc(0x20 * sizeof(Q_type));
		for (UINT j = 0; j < 0x20; j++) {
			core_var[mhartid].exec_q[j].end_ptr = core_var[mhartid].exec_q[j].curr_ptr = core_var[mhartid].exec_q[j].start_ptr = 0;
			core_var[mhartid].exec_q[j].count = 0x40;
			for (UINT8 i = 0; i < core_var[mhartid].exec_q[j].count; i++) {
				core_var[mhartid].exec_q[j].ROB_ptr[i] = 0;
				core_var[mhartid].exec_q[j].ROB_ptr_valid[i] = 0;
				core_var[mhartid].exec_q[j].state[i] = 0;
			}
		}
		core_var[mhartid].exec_q[branch_q_id].count = 4;

		core_var[mhartid].fdiv_delay = 0;

		core_var[mhartid].rd = (reg_bus*)malloc(0x20 * sizeof(reg_bus));
		for (UINT i = 0; i < 0x20; i++) {
			core_var[mhartid].rd[i].strobe = 0;
			core_var[mhartid].exec_rsp[i] = 0;
		}

		core_var[mhartid].L0_data_cache.entry = (cache_8way_type*)malloc(0x20 * sizeof(cache_8way_type));//4K per way, 32KB cache
		for (UINT i = 0; i < 0x20; i++) {
			core_var[mhartid].L0_data_cache.entry[i].way_ptr = 0;
			core_var[mhartid].L0_data_cache.entry[i].dirty = 0;
			core_var[mhartid].L0_data_cache.entry[i].in_use = 0;

			core_var[mhartid].L0_data_cache.entry[i].way[0].state = invalid_line;
			core_var[mhartid].L0_data_cache.entry[i].way[1].state = invalid_line;
			core_var[mhartid].L0_data_cache.entry[i].way[2].state = invalid_line;
			core_var[mhartid].L0_data_cache.entry[i].way[3].state = invalid_line;
			core_var[mhartid].L0_data_cache.entry[i].way[4].state = invalid_line;
			core_var[mhartid].L0_data_cache.entry[i].way[5].state = invalid_line;
			core_var[mhartid].L0_data_cache.entry[i].way[6].state = invalid_line;
			core_var[mhartid].L0_data_cache.entry[i].way[7].state = invalid_line;
			core_var[mhartid].L0_data_cache.entry[i].way[8].state = invalid_line;
		}
		for (UINT i = 0; i < 8; i++)core_var[mhartid].L0_data_cache.id_in_use[i] = 0;

		core_var[mhartid].L0_data_cache.l_addr_latch[0].strobe = 0;
		core_var[mhartid].L0_data_cache.l_addr_latch[1].strobe = 0;
		core_var[mhartid].L0_data_cache.l_addr_latch[2].strobe = 0;
		core_var[mhartid].L0_data_cache.l_addr_latch[3].strobe = 0;
		core_var[mhartid].L0_data_cache.l_data_latch[0].snoop_response = snoop_idle;
		core_var[mhartid].L0_data_cache.l_data_latch[1].snoop_response = snoop_idle;
		core_var[mhartid].L0_data_cache.l_data_latch[2].snoop_response = snoop_idle;
		core_var[mhartid].L0_data_cache.l_data_latch[3].snoop_response = snoop_idle;

		core_var[mhartid].L0_data_cache.delayed_bus2write[0].snoop_response = snoop_idle;
		core_var[mhartid].L0_data_cache.delayed_bus2write[1].snoop_response = snoop_idle;
		core_var[mhartid].L0_data_cache.ctrl_addr.strobe = 0;
		core_var[mhartid].L0_data_cache.store_data_latch_start = 0;
		core_var[mhartid].L0_data_cache.store_data_latch_stop = 0;
		core_var[mhartid].L0_data_cache.delayed_bus2write_start = 0;
		core_var[mhartid].L0_data_cache.delayed_bus2write_stop = 0;

		for (UINT i = 0; i < 4; i++) core_var[mhartid].L0_data_cache.store_data_latch[i].snoop_response = snoop_idle;
		for (UINT i = 0; i < 4; i++) core_var[mhartid].L0_data_cache.read_list[i].status = link_free;
		for (UINT i = 0; i < 4; i++) core_var[mhartid].L0_data_cache.alloc_list[i].status = link_free;
		for (UINT i = 0; i < 4; i++) core_var[mhartid].L0_data_cache.write_list[i].status = link_free;

		for (UINT i = 0; i < 4; i++) core_var[mhartid].L0_data_cache.write_data_fifo[i].snoop_response = snoop_idle;

		core_var[mhartid].L0_code_cache.entry = (cache_8way_type*)malloc(0x20 * sizeof(cache_8way_type));//4K per way, 32KB cache
		core_var[mhartid].L0_code_cache.reg = 1;
		for (UINT i = 0; i < 0x20; i++) {
			core_var[mhartid].L0_code_cache.entry[i].way_ptr = 0;

			core_var[mhartid].L0_code_cache.entry[i].way[0].state = invalid_line;
			core_var[mhartid].L0_code_cache.entry[i].way[1].state = invalid_line;
			core_var[mhartid].L0_code_cache.entry[i].way[2].state = invalid_line;
			core_var[mhartid].L0_code_cache.entry[i].way[3].state = invalid_line;
			core_var[mhartid].L0_code_cache.entry[i].way[4].state = invalid_line;
			core_var[mhartid].L0_code_cache.entry[i].way[5].state = invalid_line;
			core_var[mhartid].L0_code_cache.entry[i].way[6].state = invalid_line;
			core_var[mhartid].L0_code_cache.entry[i].way[7].state = invalid_line;
			core_var[mhartid].L0_code_cache.entry[i].way[8].state = invalid_line;
		}
		core_var[mhartid].L0_code_cache.l_addr_latch.strobe = 0;


		core_var[mhartid].logical_data_bus_data.snoop_response = snoop_idle;

		// error: need completely seperate arrays
		// array of content, pointer to next free entry
		// active queue, pointers to content
		// hold queue, pointers to content
		// *** similar structure to reorder buffer and execute queues is needed
		//

//			core_var[mhartid].L2_cache.external_snoop = ExternalSnoop_WaitCodeData;
		core_var[mhartid].L2_cache.data_r_start = core_var[mhartid].L2_cache.data_r_stop = 0;

		core_var[mhartid].L2_cache.write_count = 8;
		for (UINT j = 0; j < 8; j++) {
			core_var[mhartid].L2_cache.addr_write_list[j].status = link_free;
			core_var[mhartid].L2_cache.addr_write_list[j].data.snoop_response = snoop_idle;
			core_var[mhartid].L2_cache.snoop_write_list[j].status = link_free;
			core_var[mhartid].L2_cache.snoop_write_list[j].external_snoop = ExternalSnoop_WaitCodeData;
		}

		core_var[mhartid].L2_cache.flush = 0;
		core_var[mhartid].L2_cache.enabled = 1;
		for (UINT16 j = 0; j < 0x0040; j++)	core_var[mhartid].L2_cache.addr_fetch_list[j].status = link_free;
		for (UINT16 j = 0; j < 0x0040; j++)	core_var[mhartid].L2_cache.addr_load_list[j].status = link_free;
		for (UINT16 j = 0; j < 0x0040; j++)	core_var[mhartid].L2_cache.addr_alloc_list[j].status = link_free;

		// set = 11b
		init_L2_bank(&core_var[mhartid].L2_cache.bank, 7, 7+11);

		core_var[mhartid].bus1c.addr.in.strobe = 0;
		core_var[mhartid].bus1c.data.in.snoop_response = snoop_idle;
		core_var[mhartid].bus1c.addr.out.strobe = 0;
		core_var[mhartid].bus1c.data.out.snoop_response = snoop_idle;

		core_var[mhartid].bus2[0].addr.in.strobe = core_var[mhartid].bus2[1].addr.in.strobe = 0;
		core_var[mhartid].bus2[0].addr.out.strobe = core_var[mhartid].bus2[1].addr.out.strobe = 0;
		core_var[mhartid].bus2[0].addr.in.xaction = core_var[mhartid].bus2[1].addr.in.xaction = bus_idle;
		core_var[mhartid].bus2[0].addr.out.xaction = core_var[mhartid].bus2[1].addr.out.xaction = bus_idle;
		core_var[mhartid].bus2[0].snoop_addr.in.strobe = core_var[mhartid].bus2[1].snoop_addr.in.strobe = 0;
		core_var[mhartid].bus2[0].snoop_addr.out.strobe = core_var[mhartid].bus2[1].snoop_addr.out.strobe = 0;
		core_var[mhartid].bus2[0].snoop_addr.in.xaction = core_var[mhartid].bus2[1].snoop_addr.in.xaction = bus_idle;
		core_var[mhartid].bus2[0].snoop_addr.out.xaction = core_var[mhartid].bus2[1].snoop_addr.out.xaction = bus_idle;
		core_var[mhartid].bus2[0].data_write.in.snoop_response = core_var[mhartid].bus2[1].data_write.in.snoop_response = snoop_idle;
		core_var[mhartid].bus2[0].data_write.out.snoop_response = core_var[mhartid].bus2[1].data_write.out.snoop_response = snoop_idle;

		core_var[mhartid].logical_data_bus_addr.in = (addr_bus_type*)malloc(4 * sizeof(addr_bus_type));
		core_var[mhartid].logical_data_bus_addr.out = (addr_bus_type*)malloc(4 * sizeof(addr_bus_type));

		core_var[mhartid].logical_bus.addr.in.strobe = core_var[mhartid].logical_bus.addr.out.strobe = 0;
		core_var[mhartid].logical_bus.addr.in.xaction = core_var[mhartid].logical_bus.addr.out.xaction = bus_idle;
		core_var[mhartid].logical_bus.data_read.in.snoop_response = core_var[mhartid].logical_bus.data_read.out.snoop_response = snoop_idle;
		for (UINT8 i = 0; i < 4; i++) {
			core_var[mhartid].logical_data_bus_addr.in[i].strobe = 0;
			core_var[mhartid].logical_data_bus_addr.in[i].xaction = bus_idle;
			core_var[mhartid].logical_data_bus_addr.out[i].strobe = 0;
			core_var[mhartid].logical_data_bus_addr.out[i].xaction = bus_idle;
		}
	}

	bus_w_snoop_signal1  bus3[core_count];
	for (UINT i = 0; i < core_count; i++) {
		bus3[i].addr.in.strobe = bus3[i].addr.out.strobe = 0;
		bus3[i].addr.in.xaction = bus3[i].addr.out.xaction = bus_idle;

		bus3[i].data_write.in.snoop_response = snoop_idle;
		bus3[i].data_write.out.snoop_response = snoop_idle;
	}

	addr_bus_signal1 mbus_addr;
	data_bus_signal1 mbus_data_read, mbus_data_write;
	mbus_addr.in.strobe = mbus_addr.out.strobe = 0;
	mbus_addr.in.xaction = mbus_addr.out.xaction = bus_idle;
	mbus_addr.out.strobe = mbus_addr.out.strobe = 0;
	mbus_addr.out.xaction = mbus_addr.out.xaction = bus_idle;

	mbus_data_write.in.snoop_response = snoop_idle;
	mbus_data_write.out.snoop_response = snoop_idle;

	// ERROR
	// need to fix definition to make fdiv 1 entry deep. allow changes in q depth per q type


	// Mem clock = 333 MHz, IO = 1333 MHz (double edge), according to spec; run all at B clock
	// B clock = 1333 MHz
	// core clock = 2666 MHz
	banked_cache_type L3_cache;		// issue: L3 is banks of L2 accessed in parallel (L2 is serial) L2 banks == L3 banks	
//	for (UINT8 i = 0; i < core_count; i++) {
//	}

	for (UINT mhartid = 0; mhartid < core_count; mhartid++) {
		L3_cache.bank[mhartid].in_use = 0;
		switch (core_count) {
		case 4:
			init_L2_bank(&L3_cache.bank[mhartid], 7, (7 + 11 + 2));
			break;
		case 8:
			init_L2_bank(&L3_cache.bank[mhartid], 7, (7 + 11 + 3));
			break;
		case 16:
			init_L2_bank(&L3_cache.bank[mhartid], 7, (7 + 11 + 4));
			break;
		default:
			debug++;
			break;
		}

		for (UINT i = 0; i < 8; i++) {
			L3_cache.data_write_fifo[mhartid][i].snoop_response = snoop_idle;
		}

		L3_cache.snoop_active[mhartid] = 0;
		L3_cache.bus3_tracker[mhartid].start = 0;
		L3_cache.bus3_tracker[mhartid].stop = 0;
		L3_cache.bus3_tracker[mhartid].list = (cache_addr_linked_list_type*)malloc(0x200 * sizeof(cache_addr_linked_list_type));// is this too large, too small ???
		L3_cache.bus3_tracker[mhartid].round_count = (0x200 - 1);
		for (UINT i = 0; i < 0x200; i++) {
			L3_cache.bus3_tracker[mhartid].list[i].status = link_free;
			L3_cache.bus3_tracker[mhartid].list[i].data.snoop_response = snoop_idle;
		}
		for (UINT i = 0; i < 8; i++)	L3_cache.data_read_fifo[mhartid][i].snoop_response = snoop_idle;
	}
	for (UINT i = 0; i < 4 * core_count; i++)	L3_cache.mem_addr_out[i].strobe = 0;
	L3_cache.data_read_start_ptr = L3_cache.data_read_end_ptr = 0;
	L3_cache.data_write_start_ptr = L3_cache.data_write_end_ptr = 0;
	L3_cache.active_core = 0;

	// intialization
	// need to autogenerate cache - enter only cache size, cache line size, way count
	DDR_control_type* DDR_ctrl;
	DDR_ctrl = (DDR_control_type*)malloc(sizeof(DDR_control_type));

	DDR_ctrl->bank_ptr = 0;
	DDR_ctrl->CAS_latency = 13;
	DDR_ctrl->CAS_W_latency = 7;// write latency
	for (UINT8 i = 0; i < 4; i++) {
		DDR_ctrl->bank[i].RA = -1;
		DDR_ctrl->bank[i].CA = -1;
		DDR_ctrl->bank[i].row_valid = 0;
		DDR_ctrl->bank[i].delay = 0;
	}

	DDR_ctrl->data_valid_count = 0;
	DDR_ctrl->data_bus_ptr = 0;
	for (UINT8 i = 0; i < 0x20; i++) 			DDR_ctrl->data_bus[i].valid = 0;

	DDR_ctrl->list_start_ptr = DDR_ctrl->list_stop_ptr = 0;
	DDR_ctrl->list_count = 0x80;
	DDR_ctrl->wc_ptr = 0;
	DDR_ctrl->victim_ptr = 0;
	for (UINT8 i = 0; i < DDR_ctrl->list_count; i++) {
		DDR_ctrl->list[i].status = ddr_idle;
		DDR_ctrl->list[i].clock = 0;
	}

	UINT64 Bclock;//bus clock, memory clock, doing around ddr4 26666, 1.3 GHz IO bus/L3 bus. 128b wide rather than standard 64b wide; set B clock = IO clock for startup ease
	UINT64 clock = 0;

	FILE* debug_stream;
	fopen_s(&debug_stream, "RISC_V_events.txt", "w");

	UINT8 exit_flag = 0;
	UINT8 reset = 0, ext_timer, ext_timer_int = 0;
	UINT ecall_out[8] = { 0,0,0,0,0,0,0,0 }, ecall_in;
	for (; clock < param.stop_time && !exit_flag; clock++) { // VHDL limit was 32b. will need to take to 64b.

		if (clock == 0) { // reset / external timer interrupt unit
			reset = 1;
		}
		else if (clock == 0x10) {
			reset = 0;
		}
		if (reset == 1 || ext_timer_int == 1) {
			ext_timer = 0;
			ext_timer_int = 0;
		}
		else {
			ext_timer++;
			if (ext_timer > 0x1000000) // ~16ms
				ext_timer_int = 1;
		}
		if (clock >= 0x0178)
			debug++;

		if ((clock & 1) == 0) { // bus runs at half core clock, need to latch and issue bus request on L0/TLB miss
			Bclock = clock >> 1; // DRAM clock = B clock
			for (UINT i = 0; i < core_count; i++)	synch_bus(&bus3[i], clock);
			copy_addr_bus_info(&mbus_addr.in, &mbus_addr.out, clock);
			mbus_addr.out.strobe = 0;
			copy_data_bus_info(&mbus_data_read.in, &mbus_data_read.out);
			copy_data_bus_info(&mbus_data_write.in, &mbus_data_write.out);
			mbus_data_read.out.snoop_response = snoop_idle;
			mbus_data_write.out.snoop_response = snoop_idle;
			memory_controller(&mbus_addr.in, &mbus_data_read.out, &mbus_data_write.in, reset, Bclock, DDR_ctrl, memory_space, &param, debug_stream);
		}
		L3_cache_unit(&mbus_addr.out, bus3, &mbus_data_write.out, &mbus_data_read.in, reset, clock, &L3_cache, &exit_flag, &param, debug_stream);//multi-core access + snoops (aka L3); need to write as banks of 2MB caches
		// snoop_addr - 1 for all
		// snoop data (response)
		ecall_in = ecall_out[0] | ecall_out[1] | ecall_out[2] | ecall_out[3] | ecall_out[4] | ecall_out[5] | ecall_out[6] | ecall_out[7];
		for (UINT mhartid = 0; mhartid < core_count && !exit_flag; mhartid++) {
			{
				int debug = 0;

				ROB_Type* ROB = core_var[mhartid].ROB; // static to hide data for easier MP modeling, will have impact on stack
				Reg_Table_Type* reg_table = core_var[mhartid].reg_table;
				csr_type* csr = core_var[mhartid].csr;

				TLB_type* TLB = core_var[mhartid].TLB;
				sTLB_type* sTLB = &core_var[mhartid].sTLB;
				prefetcher_type* prefetcher = core_var[mhartid].prefetcher;
				decode_type* decode_vars = &core_var[mhartid].decode_vars;
				Q_type* exec_q = core_var[mhartid].exec_q;

				UINT8* fdiv_delay = &core_var[mhartid].fdiv_delay;
				fp_vars* fpu_vars = &core_var[mhartid].fpu_vars;

				L0_code_cache_type* L0_code_cache = &core_var[mhartid].L0_code_cache;

				L2_cache_type* L2_cache = &core_var[mhartid].L2_cache;

				bus_w_snoop_signal1* bus2 = core_var[mhartid].bus2;
				addr_bus_signal1* snoop_L2 = &core_var[mhartid].snoop_L2;

				internal_bus_signal1* physical_load_bus = &core_var[mhartid].bus1c;
				bus_w_snoop_signal1* logical_code_bus = &core_var[mhartid].logical_bus;
				retire_type retire_in;
				UINT8* ROB_stall = &core_var[mhartid].ROB_stall;
				UINT* uPC = &core_var[mhartid].uPC;
//				UINT64* SRET_addr = &core_var[mhartid].SRET_addr;
				UINT8* fadd_q_id = &core_var[mhartid].fadd_q_id;
				UINT8* fmac_q_id = &core_var[mhartid].fmac_q_id;

				reg_bus* rd = core_var[mhartid].rd;
				UINT16* exec_rsp = core_var[mhartid].exec_rsp;

				IntUnitType* IntUnitVars = &core_var[mhartid].IntUnitVars;
				branch_vars* branch = &core_var[mhartid].branch;
				UINT8* block_loads = &core_var[mhartid].block_loads;
				UINT8* prefetcher_busy = &core_var[mhartid].prefetcher_busy;
				UINT8* flush_write_posters = &core_var[mhartid].flush_write_posters;
				decode_shifter_struct*shifter = &core_var[mhartid].shifter;
				retire_type* retire = &core_var[mhartid].retire;

				synch_bus(logical_code_bus, clock);
				synch_bus(&bus2[0], clock);
				synch_bus(&bus2[1], clock);

				physical_load_bus->addr.in.addr = physical_load_bus->addr.out.addr;
				physical_load_bus->addr.in.xaction = physical_load_bus->addr.out.xaction;
				physical_load_bus->addr.in.xaction_id = physical_load_bus->addr.out.xaction_id;
				physical_load_bus->addr.in.strobe = physical_load_bus->addr.out.strobe;
				physical_load_bus->addr.in.cacheable = physical_load_bus->addr.out.cacheable;
				physical_load_bus->addr.out.strobe = 0;
				for (UINT i = 0; i < 0x10; i++)	physical_load_bus->data.in.data[i] = physical_load_bus->data.out.data[i];
				physical_load_bus->data.in.xaction_id = physical_load_bus->data.out.xaction_id;
				physical_load_bus->data.in.snoop_response = physical_load_bus->data.out.snoop_response;
				physical_load_bus->data.in.cacheable = physical_load_bus->data.out.cacheable;
				physical_load_bus->data.out.snoop_response = snoop_idle;
				// synch snoop_L2
				snoop_L2->in.addr = snoop_L2->out.addr;
				snoop_L2->in.cacheable = snoop_L2->out.cacheable;
				snoop_L2->in.xaction = snoop_L2->out.xaction;
				snoop_L2->in.xaction_id = snoop_L2->out.xaction_id;
				snoop_L2->in.strobe = snoop_L2->out.strobe;
				snoop_L2->out.strobe = 0;
				ROB->retire_ptr_in = ROB->retire_ptr_out;
				if (ecall_in)
					csr[csr_mcause].value |= ecall_in << 8;
				for (UINT i = 0; i < 0x100; i++) {
					switch (ROB->q[i].state) {
					case ROB_allocate_0:
						ROB->q[i].state = ROB_allocate_1;
						break;
					case ROB_allocate_1:
						ROB->q[i].state = ROB_allocate_2;
						break;
					case ROB_retire_out:
						ROB->q[i].state = ROB_retire_in;
						break;
					default:
						break;
					}
				}
				for (UINT i = 0; i < 0x20; i++) {
					for (UINT8 j = 0; j < reg_rename_size; j++) {
						reg_table->x_reg[i].valid[j] = (reg_table->x_reg[i].valid[j] == reg_valid_out) ? reg_valid_in : reg_table->x_reg[i].valid[j];
						reg_table->f_reg[i].valid[j] = (reg_table->f_reg[i].valid[j] == reg_valid_out) ? reg_valid_in : reg_table->f_reg[i].valid[j];
						reg_table->f_reg[i].valid[j] = (reg_table->f_reg[i].valid[j] == reg_valid_out2) ? reg_valid_out : reg_table->f_reg[i].valid[j];
					}
				}
				if (mhartid == 3) {
					if (clock >= 0x2ea7) {
						debug++;
						if (reg_table->x_reg[2].data[12] != 0x00ffbff958)
							debug++;
					}
					if (clock >= 0x005f)
						debug++;
				}
				if (clock == param.start_time) {
					for (UINT8 i = 0; i < core_count; i++) {
						core_var[i].csr[csr_cycle].value = 0;
						core_var[i].csr[csr_scycle].value = 0;
						core_var[i].csr[csr_hcycle].value = 0;
						core_var[i].csr[csr_mcycle].value = 0;
						core_var[i].csr[csr_instret].value = 0;
						core_var[i].csr[csr_sinstret].value = 0;
						core_var[i].csr[csr_hinstret].value = 0;
						core_var[i].csr[csr_minstret].value = 0;
						core_var[i].csr[csr_load_issued].value = 0;
						core_var[i].csr[csr_alloc_issued].value = 0;
						core_var[i].csr[csr_store_issued].value = 0;
						core_var[i].csr[csr_sload_issued].value = 0;
						core_var[i].csr[csr_salloc_issued].value = 0;
						core_var[i].csr[csr_sstore_issued].value = 0;
						core_var[i].csr[csr_hload_issued].value = 0;
						core_var[i].csr[csr_halloc_issued].value = 0;
						core_var[i].csr[csr_hstore_issued].value = 0;
						core_var[i].csr[csr_mload_issued].value = 0;
						core_var[i].csr[csr_malloc_issued].value = 0;
						core_var[i].csr[csr_mstore_issued].value = 0;
						core_var[i].csr[csr_hpmcounter3].value = 0;
						core_var[i].csr[csr_hpmcounter4].value = 0;
						core_var[i].csr[csr_hpmcounter5].value = 0;
						core_var[i].csr[csr_hpmcounter8].value = 0;
						core_var[i].csr[csr_hpmcounter9].value = 0;
						core_var[i].csr[csr_hpmcounter10].value = 0;
						core_var[i].csr[csr_hpmcounter11].value = 0;
						core_var[i].csr[csr_shpmcounter3].value = 0;
						core_var[i].csr[csr_shpmcounter4].value = 0;
						core_var[i].csr[csr_shpmcounter5].value = 0;
						core_var[i].csr[csr_shpmcounter8].value = 0;
						core_var[i].csr[csr_shpmcounter9].value = 0;
						core_var[i].csr[csr_shpmcounter10].value = 0;
						core_var[i].csr[csr_shpmcounter11].value = 0;
						core_var[i].csr[csr_hhpmcounter4].value = 0;
						core_var[i].csr[csr_hhpmcounter5].value = 0;
						core_var[i].csr[csr_hhpmcounter8].value = 0;
						core_var[i].csr[csr_hhpmcounter9].value = 0;
						core_var[i].csr[csr_hhpmcounter10].value = 0;
						core_var[i].csr[csr_hhpmcounter11].value = 0;
						core_var[i].csr[csr_mhpmcounter4].value = 0;
						core_var[i].csr[csr_mhpmcounter5].value = 0;
						core_var[i].csr[csr_mhpmcounter8].value = 0;
						core_var[i].csr[csr_mhpmcounter9].value = 0;
						core_var[i].csr[csr_mhpmcounter10].value = 0;
						core_var[i].csr[csr_mhpmcounter11].value = 0;

						for (UINT j=0;j<0x20;j++)
							core_var[i].decode_vars.perf_reg[j] = 0;
					}
				}
				if (mhartid == 0) {
					if (clock >= 0x01fc)
						debug++;
				}
				// L2 complex
				UINT debug_core = clock >= param.start_time &&
					(((param.core & 1) && mhartid == 0) || ((param.core & 2) && mhartid == 1) || ((param.core & 4) && mhartid == 2) || ((param.core & 8) && mhartid == 3) ||
						((param.core & 0x10) && mhartid == 4) || ((param.core & 0x20) && mhartid == 5) || ((param.core & 0x40) && mhartid == 6) || ((param.core & 0x80) && mhartid == 7) ||
						((param.core & 0x100) && mhartid == 8) || ((param.core & 0x200) && mhartid == 9) || ((param.core & 0x400) && mhartid == 10) || ((param.core & 0x800) && mhartid == 11) ||
						((param.core & 0x1000) && mhartid == 12) || ((param.core & 0x2000) && mhartid == 13) || ((param.core & 0x4000) && mhartid == 14) || ((param.core & 0x8000) && mhartid == 15));

				shadow_tlb_unit(bus2, &snoop_L2->out, &bus3[mhartid], clock, sTLB, mhartid, debug_core, &param, debug_stream);
				L2_2MB_cache(bus2, &bus3[mhartid], &snoop_L2->in, mhartid, clock, L2_cache, 0, debug_core, &param, debug_stream);// single core access + snoops // replay loop target

				// core complex
				// reg_unit
				UINT16 exec_strobe[0x20];
				R_type OP_exec[8], fp_exec[8], LUI_AUIPC_exec, load_exec, sys_exec;
				R3_type fmul_exec[4], store_exec;
				branch_type branch_exec;
				reg_bus rs1[0x20], rs2[0x20];
				if (mhartid == 0x00) {
					if (clock >= 0x1e96)// 
						debug++;
				}

				reg_bus rd_JALR;
				rd_JALR.strobe = 0;

				UINT stores_pending = core_var[mhartid].store_var.buffer[0].status || core_var[mhartid].store_var.buffer[1].status || core_var[mhartid].store_var.buffer[2].status || core_var[mhartid].store_var.buffer[3].status;
				reg_unit(&load_exec, &store_exec, &branch_exec, fp_exec, fmul_exec, OP_exec, &sys_exec, &LUI_AUIPC_exec, reg_table, &rd_JALR, rd, exec_rsp, ROB, exec_q, prefetcher->idle_flag,
					stores_pending, block_loads[0], retire->priviledge,
					clock, csr, core_var[mhartid].store_var.buffer, debug_core, &param, debug_stream);
				//	
				UINT load_pending = 0;
				UINT8 active_IO = 0;
				data_bus_complex(rd, exec_rsp, &bus2[1], &load_pending, block_loads[0], &load_exec, &store_exec, branch, decode_vars->fault_release, csr,
					&core_var[mhartid], &TLB[1], ROB->retire_ptr_in, rd[branch_q_id2].strobe || flush_write_posters[0], &active_IO, prefetcher_busy[0], retire->priviledge, clock,
					debug_core, &param, debug_stream);
				// bus 2 is being used for csr access - will change to bus0[3]
				//	snoops are done to shadow tlb's need to incorporate - will need different interface

				retire_in.load_PC = retire->load_PC;
				retire_in.PC = retire->PC;
				retire_in.priviledge = retire->priviledge;
				retire->load_PC = 0; // clock out

//				iMUL_unit(&exec_q[iMUL_q_id], ROB, IntUnitVars, clock, reg_table, mhartid, param, debug_stream);
				for (UINT8 i = 0; i < param.decode_width; i++) {
					OP_unit(&rd[OP_q_id0 + i], &exec_rsp[OP_q_id0 + i], &OP_exec[i], clock, i, mhartid, debug_core, param, debug_stream);
				}

				for (UINT8 i = 0; i < param.decode_width; i++) {
					OP_FP_unit(&rd[fadd_q_id0 + i], &exec_rsp[fadd_q_id0 + i], &fp_exec[i], clock, i, mhartid, debug_core, param, debug_stream);
				}
				for (UINT8 i = 0; i < param.fmac; i++) {
					FMUL64_adders(&rd[fmul_q_id0 + i], &exec_rsp[fmul_q_id0 + i], &fmul_exec[i], fpu_vars, clock, i, mhartid, debug_core, param, debug_stream);
				}

				stores_pending = core_var[mhartid].store_var.buffer[0].status || core_var[mhartid].store_var.buffer[1].status || core_var[mhartid].store_var.buffer[2].status || core_var[mhartid].store_var.buffer[3].status;
				system_map_unit(csr, rd, &exec_rsp[sys_q_id], &sys_exec, &retire_in, reset, clock, debug_core, param, debug_stream);

				if (retire_in.load_PC == 0) {
					retire_unit(ROB, reg_table, csr, retire, reset, &rd[decode_q_id],clock, mhartid, debug_core, &param, debug_stream);
				}
				if (retire_in.load_PC == 0 || retire_in.load_PC == 3) {
					branch_unit(&rd[branch_q_id], branch, &exec_rsp[branch_q_id], block_loads, &branch_exec, load_pending, ROB->retire_ptr_in, retire, stores_pending, csr[csr_sbound].value, csr[csr_mbound].value,
						clock, mhartid, debug_core, &param, debug_stream);
				}
				if (rd[branch_q_id2].strobe == 0) {
					allocator_iA(exec_q, ROB, retire_in.load_PC, fadd_q_id, fmac_q_id, reg_table, clock, mhartid, debug_core, &param, debug_stream);
				}
				shifter->response.msg = inactive;

				if ((rd[branch_q_id2].strobe == 0) && ((shifter->valid &&  ROB->fault_halt == 0 && (retire_in.load_PC == 0 || retire_in.load_PC == 7)) || (csr[csr_mcause].value != 0 && (((csr[csr_mstatus].value & 0x08) == 0x08) || ((csr[csr_mstatus].value & 0x02) == 0x02))))) {
					decode32_RISC_V(ROB, exec_q, &rd[decode_q_id],flush_write_posters, &ecall_out[mhartid], ecall_in, shifter, prefetcher->idle_flag, decode_vars, branch, IntUnitVars, stores_pending, load_pending, retire,
						clock, csr, uPC, core_var[mhartid].store_var.buffer, mhartid, debug_core, &param, debug_stream);
				}
				ROB_stall[0] = 0;
				if (((ROB->decode_ptr + 1) & 0xff) == ROB->retire_ptr_in || ((ROB->decode_ptr + 2) & 0xff) == ROB->retire_ptr_in || ((ROB->decode_ptr + 3) & 0xff) == ROB->retire_ptr_in || ((ROB->decode_ptr + 4) & 0xff) == ROB->retire_ptr_in)
					ROB_stall[0] = 1;
				// parallel cache and TLB access - ISSUE: code order dependent;
				data_bus_type cTLB_response;
				cTLB_response.snoop_response = snoop_idle;
				code_tlb_unit(&cTLB_response, &physical_load_bus->addr.out, &logical_code_bus->addr.in, &bus2[0], clock, &TLB[0], mhartid, &param, debug_stream);// code = 0; data = 1
				L0_code_cache_unit(&physical_load_bus->data.out, &bus2[0], &logical_code_bus->addr.in, &physical_load_bus->addr.out, &cTLB_response, L0_code_cache, clock, debug_core, mhartid, &param, debug_stream);
				char interrupt = ((csr[csr_mcause].value != 0) && (((csr[csr_mstatus].value & 0x08) == 0x08) || ((csr[csr_mstatus].value & 0x02) == 0x02)) && shifter->response.fault_in_service == 0);
				prefetch_unit(shifter, &rd[prefetch_q_id],  & logical_code_bus->addr.out, &cTLB_response, &physical_load_bus->data.out, &bus2[0].data_read.in, &retire_in, &rd[branch_q_id2], ROB_stall[0], ROB->branch_start == ROB->branch_stop && retire->load_PC == 0, mhartid, interrupt, prefetcher, reset, &rd_JALR, prefetcher_busy, active_IO,
					clock, debug_core, &param, debug_stream);// code bus used in parallel TLB code access, disconnected for now
			}
		}
	}
	fprintf(debug_stream, "\n\n\n");
	for (UINT8 i = 0; i < core_count; i++) {
		fprintf(debug_stream, "CPU(%d)  : instr: ret %#010x, dec %#010x, clocks: %#08x, branches: ret %#08x, dec %#08x, loads: issued %#08x, ret %#08x, dec %#08x, stores: alloc %#08x, issued %#08x, ret %#08x, dec %#08x, TLB loads code %#05x, data %#05x   \n",
			i, core_var[i].csr[csr_instret].value, core_var[i].csr[csr_hpmcounter3].value, core_var[i].csr[csr_cycle].value, // instr
			core_var[i].csr[csr_hpmcounter4].value, core_var[i].decode_vars.perf_reg[branch_count], // branch
			core_var[i].csr[csr_load_issued].value, core_var[i].csr[csr_hpmcounter8].value, core_var[i].decode_vars.perf_reg[load_count], // loads
			core_var[i].csr[csr_alloc_issued].value, core_var[i].csr[csr_store_issued].value, core_var[i].csr[csr_hpmcounter10].value, core_var[i].csr[csr_hpmcounter11].value, // store
			core_var[i].TLB[0].way_ptr1, core_var[i].TLB[1].way_ptr1);
		fprintf(debug_stream, "CPU(%d) S: instr: ret %#010x, dec %#010x, clocks: %#08x, branches: ret %#08x, dec %#08x, loads: issued %#08x, ret %#08x, dec %#08x, stores: alloc %#08x, issued %#08x, ret %#08x, dec %#08x, TLB loads code %#05x, data %#05x   \n",
			i, core_var[i].csr[csr_sinstret].value, core_var[i].csr[csr_shpmcounter3].value, core_var[i].csr[csr_scycle].value,
			core_var[i].csr[csr_shpmcounter4].value, core_var[i].decode_vars.perf_reg[sbranch_count],
			core_var[i].csr[csr_sload_issued].value, core_var[i].csr[csr_shpmcounter8].value, core_var[i].decode_vars.perf_reg[sload_count],
			core_var[i].csr[csr_salloc_issued].value, core_var[i].csr[csr_sstore_issued].value, core_var[i].csr[csr_shpmcounter10].value, core_var[i].csr[csr_shpmcounter11].value,
			core_var[i].TLB[0].way_ptr1, core_var[i].TLB[1].way_ptr1);
		fprintf(debug_stream, "CPU(%d) H: instr: ret %#010x, dec %#010x, clocks: %#08x, branches: ret %#08x, dec %#08x, loads: issued %#08x, ret %#08x, dec %#08x, stores: alloc %#08x, issued %#08x, ret %#08x, dec %#08x, TLB loads code %#05x, data %#05x   \n",
			i, core_var[i].csr[csr_hinstret].value, core_var[i].csr[csr_hhpmcounter3].value, core_var[i].csr[csr_hcycle].value,
			core_var[i].csr[csr_hhpmcounter4].value, core_var[i].decode_vars.perf_reg[hbranch_count],
			core_var[i].csr[csr_hload_issued].value, core_var[i].csr[csr_hhpmcounter8].value, core_var[i].decode_vars.perf_reg[hload_count],
			core_var[i].csr[csr_halloc_issued].value, core_var[i].csr[csr_hstore_issued].value, core_var[i].csr[csr_hhpmcounter10].value, core_var[i].csr[csr_hhpmcounter11].value,
			core_var[i].TLB[0].way_ptr1, core_var[i].TLB[1].way_ptr1);
		fprintf(debug_stream, "CPU(%d) M: instr: ret %#010x, dec %#010x, clocks: %#08x, branches: ret %#08x, dec %#08x, loads: issued %#08x, ret %#08x, dec %#08x, stores: alloc %#08x, issued %#08x, ret %#08x, dec %#08x, TLB loads code %#05x, data %#05x   \n",
			i, core_var[i].csr[csr_minstret].value, core_var[i].csr[csr_mhpmcounter3].value, core_var[i].csr[csr_mcycle].value,
			core_var[i].csr[csr_mhpmcounter4].value, core_var[i].decode_vars.perf_reg[mbranch_count],
			core_var[i].csr[csr_mload_issued].value, core_var[i].csr[csr_mhpmcounter8].value, core_var[i].decode_vars.perf_reg[mload_count],
			core_var[i].csr[csr_malloc_issued].value, core_var[i].csr[csr_mstore_issued].value, core_var[i].csr[csr_mhpmcounter10].value, core_var[i].csr[csr_mhpmcounter11].value,
			core_var[i].TLB[0].way_ptr1, core_var[i].TLB[1].way_ptr1);
		fprintf(debug_stream, "CPU(%d) U: instr: ret %#010x, dec %#010x, clocks: %#08x, branches: ret %#08x, dec %#08x, loads: issued %#08x, ret %#08x, dec %#08x, stores: alloc %#08x, issued %#08x, ret %#08x, dec %#08x, TLB loads code %#05x, data %#05x   \n",
			i, core_var[i].csr[csr_instret].value - core_var[i].csr[csr_sinstret].value - core_var[i].csr[csr_hinstret].value - core_var[i].csr[csr_minstret].value,
			core_var[i].csr[csr_hpmcounter3].value - core_var[i].csr[csr_shpmcounter3].value - core_var[i].csr[csr_hhpmcounter3].value - core_var[i].csr[csr_mhpmcounter3].value,
			core_var[i].csr[csr_cycle].value - core_var[i].csr[csr_scycle].value - core_var[i].csr[csr_hcycle].value - core_var[i].csr[csr_mcycle].value,
			core_var[i].csr[csr_hpmcounter4].value - core_var[i].csr[csr_shpmcounter4].value - core_var[i].csr[csr_hhpmcounter4].value - core_var[i].csr[csr_mhpmcounter4].value,
			core_var[i].decode_vars.perf_reg[branch_count] - core_var[i].decode_vars.perf_reg[sbranch_count] - core_var[i].decode_vars.perf_reg[hbranch_count] - core_var[i].decode_vars.perf_reg[mbranch_count],
			core_var[i].csr[csr_load_issued].value - core_var[i].csr[csr_sload_issued].value - core_var[i].csr[csr_hload_issued].value - core_var[i].csr[csr_mload_issued].value,
			core_var[i].csr[csr_hpmcounter8].value - core_var[i].csr[csr_shpmcounter8].value - core_var[i].csr[csr_hhpmcounter8].value - core_var[i].csr[csr_mhpmcounter8].value,
			core_var[i].decode_vars.perf_reg[load_count] - core_var[i].decode_vars.perf_reg[sload_count] - core_var[i].decode_vars.perf_reg[hload_count] - core_var[i].decode_vars.perf_reg[mload_count],
			core_var[i].csr[csr_alloc_issued].value - core_var[i].csr[csr_salloc_issued].value - core_var[i].csr[csr_halloc_issued].value - core_var[i].csr[csr_malloc_issued].value,
			core_var[i].csr[csr_store_issued].value - core_var[i].csr[csr_sstore_issued].value - core_var[i].csr[csr_hstore_issued].value - core_var[i].csr[csr_mstore_issued].value,
			core_var[i].csr[csr_hpmcounter10].value - core_var[i].csr[csr_shpmcounter10].value - core_var[i].csr[csr_hhpmcounter10].value - core_var[i].csr[csr_mhpmcounter10].value,
			core_var[i].csr[csr_hpmcounter11].value - core_var[i].csr[csr_shpmcounter11].value - core_var[i].csr[csr_hhpmcounter11].value - core_var[i].csr[csr_mhpmcounter11].value,
			core_var[i].TLB[0].way_ptr1, core_var[i].TLB[1].way_ptr1);
		fprintf(debug_stream, "\n");
		fprintf(debug_stream, "CPU(%d): instr/clock %f, S %f, H %f, M %f, U %f; CPI < 4 - not enough parallelism for 8 wide decoder\n",
			i,
			(core_var[i].csr[csr_cycle].value == 0) ? 0 : (float)core_var[i].csr[csr_instret].value / (float)core_var[i].csr[csr_cycle].value,
			(core_var[i].csr[csr_scycle].value == 0) ? 0 : (float)core_var[i].csr[csr_sinstret].value / (float)core_var[i].csr[csr_scycle].value,
			(core_var[i].csr[csr_hcycle].value == 0) ? 0 : (float)core_var[i].csr[csr_hinstret].value / (float)core_var[i].csr[csr_hcycle].value,
			(core_var[i].csr[csr_mcycle].value == 0) ? 0 : (float)core_var[i].csr[csr_minstret].value / (float)core_var[i].csr[csr_mcycle].value,
			((core_var[i].csr[csr_cycle].value - core_var[i].csr[csr_scycle].value - core_var[i].csr[csr_hcycle].value - core_var[i].csr[csr_mcycle].value) == 0) ? 0 :
			(float)(core_var[i].csr[csr_instret].value - core_var[i].csr[csr_sinstret].value - core_var[i].csr[csr_hinstret].value - core_var[i].csr[csr_minstret].value) /
			(float)(core_var[i].csr[csr_cycle].value - core_var[i].csr[csr_scycle].value - core_var[i].csr[csr_hcycle].value - core_var[i].csr[csr_mcycle].value));
		fprintf(debug_stream, "CPU(%d): instr/branch %#x, S %#x, H %#x, M %#x, U %#x; < 8 branches exceed decoder width, < 0x80 exceeds ROB capacity\n",
			i,
			(core_var[i].csr[csr_hpmcounter4].value == 0) ? 0 : core_var[i].csr[csr_instret].value / core_var[i].csr[csr_hpmcounter4].value,
			(core_var[i].csr[csr_shpmcounter4].value == 0) ? 0 : core_var[i].csr[csr_sinstret].value / core_var[i].csr[csr_shpmcounter4].value,
			(core_var[i].csr[csr_hhpmcounter4].value == 0) ? 0 : core_var[i].csr[csr_hinstret].value / core_var[i].csr[csr_hhpmcounter4].value,
			(core_var[i].csr[csr_mhpmcounter4].value == 0) ? 0 : core_var[i].csr[csr_minstret].value / core_var[i].csr[csr_mhpmcounter4].value,
			((core_var[i].csr[csr_hpmcounter4].value - core_var[i].csr[csr_shpmcounter4].value - core_var[i].csr[csr_hhpmcounter4].value - core_var[i].csr[csr_mhpmcounter4].value) == 0) ? 0 :
			(core_var[i].csr[csr_instret].value - core_var[i].csr[csr_sinstret].value - core_var[i].csr[csr_hinstret].value - core_var[i].csr[csr_minstret].value) /
			(core_var[i].csr[csr_hpmcounter4].value - core_var[i].csr[csr_shpmcounter4].value - core_var[i].csr[csr_hhpmcounter4].value - core_var[i].csr[csr_mhpmcounter4].value));
		fprintf(debug_stream, "CPU(%d): instr/load %#x, S %#x, H %#x, M %#x, U %#x; < 8 loads executed exceed reorder capability\n",
			i,
			(core_var[i].csr[csr_hpmcounter8].value == 0) ? 0 : core_var[i].csr[csr_instret].value / core_var[i].csr[csr_hpmcounter8].value,
			(core_var[i].csr[csr_shpmcounter8].value == 0) ? 0 : core_var[i].csr[csr_sinstret].value / core_var[i].csr[csr_shpmcounter8].value,
			(core_var[i].csr[csr_hhpmcounter8].value == 0) ? 0 : core_var[i].csr[csr_hinstret].value / core_var[i].csr[csr_hhpmcounter8].value,
			(core_var[i].csr[csr_mhpmcounter8].value == 0) ? 0 : core_var[i].csr[csr_minstret].value / core_var[i].csr[csr_mhpmcounter8].value,
			((core_var[i].csr[csr_hpmcounter8].value - core_var[i].csr[csr_shpmcounter8].value - core_var[i].csr[csr_hhpmcounter8].value - core_var[i].csr[csr_mhpmcounter8].value) == 0) ? 0 :
			(core_var[i].csr[csr_instret].value - core_var[i].csr[csr_sinstret].value - core_var[i].csr[csr_hinstret].value - core_var[i].csr[csr_minstret].value) /
			(core_var[i].csr[csr_hpmcounter8].value - core_var[i].csr[csr_shpmcounter8].value - core_var[i].csr[csr_hhpmcounter8].value - core_var[i].csr[csr_mhpmcounter8].value));
		fprintf(debug_stream, "CPU(%d): instr/(alloc + store) %#x, S %#x, H %#x, M %#x, U %#x; < 8 (alloc + store) issued exceed reorder capability\n",
			i,
			(core_var[i].csr[csr_alloc_issued].value + core_var[i].csr[csr_store_issued].value == 0) ? 0 : core_var[i].csr[csr_instret].value / (core_var[i].csr[csr_alloc_issued].value + core_var[i].csr[csr_store_issued].value),
			(core_var[i].csr[csr_salloc_issued].value + core_var[i].csr[csr_sstore_issued].value == 0) ? 0 : core_var[i].csr[csr_instret].value / (core_var[i].csr[csr_salloc_issued].value + core_var[i].csr[csr_sstore_issued].value),
			(core_var[i].csr[csr_halloc_issued].value + core_var[i].csr[csr_hstore_issued].value == 0) ? 0 : core_var[i].csr[csr_instret].value / (core_var[i].csr[csr_halloc_issued].value + core_var[i].csr[csr_hstore_issued].value),
			(core_var[i].csr[csr_malloc_issued].value + core_var[i].csr[csr_mstore_issued].value == 0) ? 0 : core_var[i].csr[csr_instret].value / (core_var[i].csr[csr_malloc_issued].value + core_var[i].csr[csr_mstore_issued].value),
			((core_var[i].csr[csr_alloc_issued].value + core_var[i].csr[csr_store_issued].value -
				core_var[i].csr[csr_salloc_issued].value - core_var[i].csr[csr_sstore_issued].value -
				core_var[i].csr[csr_halloc_issued].value - core_var[i].csr[csr_hstore_issued].value -
				core_var[i].csr[csr_malloc_issued].value - core_var[i].csr[csr_mstore_issued].value) == 0) ? 0 :
			(core_var[i].csr[csr_instret].value - core_var[i].csr[csr_sinstret].value - core_var[i].csr[csr_hinstret].value - core_var[i].csr[csr_minstret].value) /
			(core_var[i].csr[csr_alloc_issued].value + core_var[i].csr[csr_store_issued].value -
				core_var[i].csr[csr_salloc_issued].value - core_var[i].csr[csr_sstore_issued].value -
				core_var[i].csr[csr_halloc_issued].value - core_var[i].csr[csr_hstore_issued].value -
				core_var[i].csr[csr_malloc_issued].value - core_var[i].csr[csr_mstore_issued].value));
		fprintf(debug_stream, "\n");
	}
	fclose(debug_stream);
	exit(0);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
