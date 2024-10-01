// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#pragma once
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

#include "internal_bus.h"

enum data_type_enum :UINT8 {
	int8_enum = 0x00,
	int16_enum = 0x01,
	int32_enum = 0x02,
	int64_enum = 0x03,
	int128_enum = 0x04,
	uint8_enum = 0x08,
	uint16_enum = 0x09,
	uint32_enum = 0x0a,
	uint64_enum = 0x0b,
	uint128_enum = 0x0c,
	fp16_enum = 0x11,
	fp32_enum = 0x12,
	fp64_enum = 0x13,
	fp128_enum = 0x14,
	void_enum = 0x1f
};
struct hw_pointers {
	INT64 sp, gp, OS_gp, tp[0x100];
};
struct parse_type {
	char line[0x200], word[0x100];
	UINT16 ptr;
};
struct line_struct {
	char line[0x400];
};
struct parse_struct2 {
	line_struct* line;
	char word[0x200];
	UINT8 word_ptr;
	UINT size;
	UINT ptr;
	UINT index;
};
struct parse_struct2_set {
	parse_struct2 a, b;
	UINT select;
};
struct parse_transfer_type {
	parse_type* data;
	UINT count;
};
struct VariableListEntry {
	char name[0x80];
	char alias[0x80];
	data_type_enum type;
	INT64 addr;
	UINT8 atomic;
	UINT8 pointer;
	UINT8 size; // in bytes
	UINT8 sp_offset_valid;
	INT sp_offset;
	UINT8 reg_index, reg_depth, reg_fp;
	UINT8 reg_index_valid;
	UINT8 function;
	UINT8 depth, depth_count;
	VariableListEntry* next, * last;
};
struct FunctionListEntry {
	char name[0x100];
	data_type_enum type;
	INT64 addr;
	UINT16 argc;
	VariableListEntry argument[0x10];

	FunctionListEntry* next, * last;
};
struct compiler_var_type {
	VariableListEntry* global_list_base;
	VariableListEntry* list_base;
	FunctionListEntry* f_list_base;
	INT gp_offset, OS_gp_offset;
	int switch_count;
};
struct operator_type {
	char symbol[0x10];
	char opcode[0x10];
	char rev_opcode[0x10];
	char type;
};

struct loop_variable_type {
	char unit_type[0x100];
	char name[0x100];
	INT64 initial, limit, increment, addr, out_addr;
};
struct loop_control_type {
	loop_variable_type index[0x10];
	char depth_test, depth_out;// , depth_hold;
	UINT index_start;
};
struct vector_type {
	// content
	char name[0x80];
	INT64 size;		// array size
	char** rhs;		// entry, replacement code
	// rhs maintanence
	INT64 index;
	UINT  ptr;
	UINT order;// 4 = most recently accessed, all others add -1; zero is minimum (forced write)
	UINT depth; // invalidate when out of visible range
	// cull vars
	UINT8 active;
	UINT out_addr, in_addr;
};

enum op_type : UINT8 {
	none_op = 0,
	mul_op = 1,
	sr_op = 2,
	sl_op = 3,
	add_op = 4,
	sub_op = 5,
	or_op = 6,
	xor_op = 7,
	and_op = 8,
	and_log = 9,
	or_log = 10,
	eq_log = 11
};
struct label_type {
	char name[0x100];
	INT64 addr;
};
struct FunctionListEntry3 {
	char name[0x100];
	char type[0x10]; // label, UINT8, UINT16,... enumerated
	UINT8 type_index;
	VariableListEntry argument[8];
	char match[8][0x100];
	char match_content[8][0x100];
	UINT8 arg_count;

	FunctionListEntry3* next, * last;
};
struct var_type_struct {
	char name[0x20];
	data_type_enum type;
};
#define var_type_count  0x12

//void copy_ptr_char(parse_struct2* parse_out, parse_struct2* parse_in);
void getWord(parse_struct2* parse);
void getWord(parse_struct2* parse, UINT index, UINT* ptr);

void getVector(parse_struct2* parse);

//void copy_ptr_char(parse_struct2* parse_out, parse_struct2* parse_in);
void copy_ptr_char(parse_struct2* parse_out, char letter);
void strcpy_p2(parse_struct2* parse, char* word);
void strcpy_p2(parse_struct2* parse_out, parse_struct2* parse_in);
void strcat_p2(parse_struct2* parse, char* word);
void strcat_p2(parse_struct2* parse_out, parse_struct2* parse_in);

void copy_ptr_char(char* parse_out, UINT* parse_out_ptr, char* parse_in, UINT* parse_in_ptr);
void copy_ptr_char(char* parse_out, UINT* parse_out_ptr, char letter);
void strcpy_word(char* out, parse_struct2* parse);

//void new_page(memory_space_type *memory_space, UINT64 page, page_state type, UINT init);
UINT64 allocate_data_4K_page(memory_space_type* memory_space, UINT64 logical_address, UINT guxwr, INT64 base, UINT8 RV64_mode, FILE* malloc_file);
UINT64 allocate_data_2M_page(memory_space_type* memory_space, UINT64 logical_address, UINT guxwr, INT64 base, UINT8 RV64_mode, FILE* malloc_file);

UINT8 find_variable(VariableListEntry* VariableList, parse_struct2* parse, var_type_struct* var_type, UINT depth);

UINT8 get_integer(INT64* number, char* input);
UINT8 getOperatorB(UINT8* op_match, parse_struct2* parse, operator_type* op_table);
UINT8 getBranchB(UINT8* op_match, parse_struct2* parse, operator_type* op_table);

void inline_C(const char* dst_name, const char* src_name, INT flags, var_type_struct* var_type, parse_struct2_set* parse, int pass_count);
void SwitchFill(const char* dst_name, const char* src_name, INT flags, var_type_struct* var_type, parse_struct2_set* parse, int pass_count);
void FunctCall_ConstEllim(const char* dst_name, const char* src_name, INT flags, var_type_struct* var_type, parse_struct2_set* parse, int pass_count);
void FunctCall_of_FunctCall_Elli(const char* dst_name, const char* src_name, INT flags, var_type_struct* var_type, parse_struct2_set* parse, int pass_count);

//void InlineShortFunct(char* dst_name, char* src_name, INT flags, char** var_type, parse_struct2_set* parse, int pass_count);
void MarkedInline(const char* dst_name, const char* src_name, INT flags, var_type_struct* var_type, parse_struct2_set* parse, int pass_count);
void declare_multi2single(parse_struct2* parse_out, parse_struct2* parse_in, const char* dst_name, const char* src_name);
//void loop2D_merge(parse_transfer_type* parse_transfer, char* dst_name, char* src_name, char** var_type);
void parenthesis_reduction(const char* dst_name, const char* src_name, operator_type* op_table, parse_struct2_set* parse, UINT debug_unit);
void bracket_reduction(const char* dst_name, parse_struct2_set* parse, operator_type* op_table);
void store_load_ellimination(const char* dst_name, const char* src_name, operator_type* op_table, var_type_struct* var_type);

void split_line(const char* dst_name, const char* src_name, parse_struct2_set* parse, var_type_struct* var_type);
void array16toReg128(const char* dst_name, const char* src_name, parse_struct2_set* parse, UINT8 unit_debug);

void sprint_addr_p2(parse_struct2* parse_out, INT64 addr, param_type* param);
void print_parse_out(FILE* Masm, parse_struct2* parse_out);
void compile_to_RISCV_Masm(parse_struct2* parse_out, const char* dst_name, UINT64* FunctionStack_ct, const char* src_name, parse_struct2* parse_input, memory_space_type* memory_space,
	hw_pointers* pointers, INT64 logical_base, INT64 base, INT flags, compiler_var_type* compiler_var, operator_type* op_table, operator_type* branch_table, param_type* param, INT64 logical_PC_init, UINT8 unit_debug, UINT8 read_src);
void compile_to_RISCV_asm(memory_space_type* memory_space, const  char* dst_name, const char* src_name, parse_struct2_set* parse, INT64 physical_PC, INT64 logical_PC, label_type* labels, param_type* param, UINT8 print_asm);

void cull_branches(const char* dst_name, parse_struct2_set* parse);
void clear_constants(const char* dst_name, const char* src_name, var_type_struct* var_type, parse_struct2_set* parse, UINT64 satp);

void math_reduction(const char* dst_name, const char* src_name, parse_struct2_set* parse, UINT8 unit_debug);

void copy_blanks(parse_struct2* parse_out, parse_struct2* parse_in);
void decode_for(loop_control_type* l_control, parse_struct2* parse_in, UINT loop_start2);
UINT8 VariableTypeMatch(char* index, char* word, var_type_struct* var_type);
void var_index_base_inner_loop(const char* dst_name, const char* src_name, var_type_struct* var_type, parse_struct2_set* parse, compiler_var_type* compiler_var);
void nonaligned_load128_preload(const char* dst_name, const char* src_name, var_type_struct* var_type, parse_struct2_set* parse, compiler_var_type* compiler_var);

void ScalarPtrEllimination(const char* dst_name, const char* src_name, parse_struct2_set* parse, UINT8 unit_debug);
void skip_blanks(parse_struct2* parse_out, parse_struct2* parse_in);
void copy_char(parse_struct2* parse_out, parse_struct2* parse_in);
void copy2end(parse_struct2* parse_out, parse_struct2* parse_in);

void VarDeclarationEllimination4loop(const char* dst_name, const char* src_name, parse_struct2_set* parse, UINT8 unit_debug);

void format(const char* dst_name, const char* src_name, var_type_struct* var_type, parse_struct2_set* parse);
void variable_substitution(const char* dst_name, const char* src_name, parse_struct2_set* parse);

//data_type_enum getUnitType(UINT8 select);
UINT8 type_decode(data_type_enum* type, char* word);

UINT8 parenthesis_simple_cleanup(parse_struct2* parse_out, parse_struct2* parse_in);
UINT8 parenthesis_depth_based(parse_struct2* parse_out, parse_struct2* parse_in);// + (a + b) + => + a + b +
op_type decode_op(char* parse_in, UINT* ptr);
char* op2word(op_type op);

UINT8 get_VariableListEntryB(VariableListEntry** current, char* word, VariableListEntry* list_base);

//UINT8 find_function2(FunctionListEntry3* entry, parse_struct2* parse, char** var_type);