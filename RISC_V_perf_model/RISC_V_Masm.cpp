// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>
//#include <string.h>


struct reg_table_entry {
	char name[0x10];
	UINT8 in_use, temp, saved, saved_depth;
};
struct reg_table_struct {
	reg_table_entry entry_x[0x20], entry_fp[0x20];
	UINT8 fp_ptr, temp, saved, arg, depth, error, count_x, count_fp;
	UINT8 s_count, t_count;
};
struct lc_4loop_struct {
	char detected; // 1: for loop, 2: while loop
	char index_reg, limit_reg, limit_reg_valid;
	char index_name[0x100];
	int increment;
	int limit;
	int branch_match;
};
struct loop_control {
	int depth;
	lc_4loop_struct for_loop[0x10];
	int count[0x10];
	char line[0x10][0x100];
	char line_valid[0x10];
	int end[0x10];
	char index;
	INT64 switch_base_addr;

	char fence[0x10];
};
struct IO_list_type {
	var_type_struct name[0x10];
	INT addr[0x10];
	UINT8 ptr;
};
struct fp_data_struct {
	UINT16 hp; // half precision: 5b exp; 12b mantissa
	UINT32 sp;
	UINT64 dp;
	//	UINT128 qp;// quad precision (128b): 15exp; 112b mantissa
};
void print_parse_out(FILE* Masm, parse_struct2* parse_out) {
	for (UINT i = 0; i < parse_out->index; i++)
		fprintf(Masm, "%s", parse_out->line[i].line);
	fclose(Masm);
}

void sprintf_string(parse_struct2* parse_out, INT64* PC, char*message, param_type* param) {
	parse_out->ptr = 0;
	sprint_addr_p2(parse_out, PC[0], param);
	strcat_p2(parse_out, message);
	parse_out->index++;
	PC[0] += 4;
}
void sprint_label(parse_struct2* parse_out, INT64 PC, char*header, char*name, param_type* param) {
//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_%s \n", PC, name);
	parse_out->ptr = 0;
	sprint_addr_p2(parse_out,PC ,param);
	strcat_p2(parse_out, (char*)" // label: ");
	strcat_p2(parse_out, header);
	strcat_p2(parse_out, name);
	strcat_p2(parse_out, (char*)" \n");
	parse_out->index++;
}
void sprint_label(parse_struct2* parse_out, INT64 PC, char* header, loop_control* l_control, param_type* param) {
	// sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label%d_%d\n", logical_PC[0], l_control->depth, l_control->count[l_control->depth]++);
	parse_out->ptr = 0;
	sprint_addr_p2(parse_out, PC, param);
	strcat_p2(parse_out, (char*)" // label: ");
	strcat_p2(parse_out, header); 
	char word[0x10];
	sprintf_s(word, "%d_%d\n", l_control->depth, l_control->count[l_control->depth]++);
	strcat_p2(parse_out, word);
	parse_out->index++;
}

void sprint_csr_imm(parse_struct2* parse_out, INT64 *PC, char* op, char* rd, UINT8 imm, char* csr_name, param_type* param) {
//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrci %s, 0x00, %s\n", PC[0], reg_table->entry_x[target->reg_index].name, csr_list[csr_index]); PC[0] += 4;//current->name,
	parse_out->ptr = 0;
	sprint_addr_p2(parse_out, PC[0], param);
	strcat_p2(parse_out, (char*)" ");
	strcat_p2(parse_out, op);
	strcat_p2(parse_out, (char*)" ");
	strcat_p2(parse_out, rd);
	char word[0x10];
	sprintf_s(word, ", 0x%02x, ", imm);
	strcat_p2(parse_out, word);
	strcat_p2(parse_out, csr_name);
	strcat_p2(parse_out, (char*)" \n");
	parse_out->index++;
	PC[0] += 4;
}
void sprint_op_2src(parse_struct2* parse_out, INT64* PC, char* op, char* rd, char* rs1, char* rs2, param_type* param) {
//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[csr_copy].name, reg_table->entry_x[csr_copy].name); PC[0] += 4;
	parse_out->ptr = 0;
	sprint_addr_p2(parse_out, PC[0], param);
	strcat_p2(parse_out, (char*)" ");
	strcat_p2(parse_out, op);
	strcat_p2(parse_out, (char*)" ");
	strcat_p2(parse_out, rd);
	strcat_p2(parse_out, (char*)", ");
	strcat_p2(parse_out, rs1);
	strcat_p2(parse_out, (char*)", ");
	strcat_p2(parse_out, rs2);
	strcat_p2(parse_out, (char*)" \n");
	parse_out->index++;
	PC[0] += 4;
}
void sprint_op_2src(parse_struct2* parse_out, INT64* PC, char* op, char* rd, char* rs1, INT16 imm, param_type* param) {
	//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[csr_copy].name, reg_table->entry_x[csr_copy].name); PC[0] += 4;
	parse_out->ptr = 0;
	sprint_addr_p2(parse_out, PC[0], param);
	strcat_p2(parse_out, (char*)" ");
	strcat_p2(parse_out, op);
	strcat_p2(parse_out, (char*)" ");
	strcat_p2(parse_out, rd);
	strcat_p2(parse_out, (char*)", ");
	strcat_p2(parse_out, rs1);
	char word[0x10];
	sprintf_s(word, ", 0x%02x\n", imm);
	strcat_p2(parse_out, word);
	parse_out->index++;
	PC[0] += 4;
}
void sprint_op_2src(parse_struct2* parse_out, INT64* PC, char* op, char* rd, char* rs1, char* rs2, char* comment, param_type* param) {
	//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[csr_copy].name, reg_table->entry_x[csr_copy].name); PC[0] += 4;
	parse_out->ptr = 0;
	sprint_addr_p2(parse_out, PC[0], param);
	strcat_p2(parse_out, (char*)" ");
	strcat_p2(parse_out, op);
	strcat_p2(parse_out, (char*)" ");
	strcat_p2(parse_out, rd);
	strcat_p2(parse_out, (char*)", ");
	strcat_p2(parse_out, rs1);
	strcat_p2(parse_out, (char*)", ");
	strcat_p2(parse_out, rs2);
	strcat_p2(parse_out, (char*)" // ");
	strcat_p2(parse_out, comment);
	strcat_p2(parse_out, (char*)" \n");
	parse_out->index++;
	PC[0] += 4;
}

void sprint_op_2src(parse_struct2* parse_out, INT64* PC, char* op, char* rd, char* rs1, INT16 imm, char* comment, param_type* param) {
	//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[csr_copy].name, reg_table->entry_x[csr_copy].name); PC[0] += 4;
	parse_out->ptr = 0;
	sprint_addr_p2(parse_out, PC[0], param);
	strcat_p2(parse_out, (char*)" ");
	strcat_p2(parse_out, op);
	strcat_p2(parse_out, (char*)" ");
	strcat_p2(parse_out, rd);
	strcat_p2(parse_out, (char*)", ");
	strcat_p2(parse_out, rs1);
	char word[0x10];
	sprintf_s(word, ", 0x%02x // ", imm);
	strcat_p2(parse_out, word);
	strcat_p2(parse_out, comment);
	strcat_p2(parse_out, (char*)"\n");
	parse_out->index++;
	PC[0] += 4;
}
void sprint_op_imm(parse_struct2* parse_out, INT64* PC, char* op, char* rd, INT imm, char* comment, param_type* param) {
	//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[csr_copy].name, reg_table->entry_x[csr_copy].name); PC[0] += 4;
	parse_out->ptr = 0;
	sprint_addr_p2(parse_out, PC[0], param);
	strcat_p2(parse_out, (char*)" ");
	strcat_p2(parse_out, op);
	strcat_p2(parse_out, (char*)" ");
	strcat_p2(parse_out, rd);
	char word[0x10];
	sprintf_s(word, ", 0x%05x // ", imm);
	strcat_p2(parse_out, word);
	strcat_p2(parse_out, comment);
	strcat_p2(parse_out, (char*)"\n");
	parse_out->index++;
	PC[0] += 4;
}
void sprint_load(parse_struct2* parse_out, INT64* PC, char* op, char* rd, INT16 imm,  char* rs1, char* comment, param_type* param) {
	//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[csr_copy].name, reg_table->entry_x[csr_copy].name); PC[0] += 4;
	parse_out->ptr = 0;
	sprint_addr_p2(parse_out, PC[0], param);
	strcat_p2(parse_out, (char*)" ");
	strcat_p2(parse_out, op);
	strcat_p2(parse_out, (char*)"   ");
	strcat_p2(parse_out, rd);
	char word[0x10];
	if (imm>=0)
		sprintf_s(word, ",  0x%03x(", imm);
	else
		sprintf_s(word, ", -0x%03x(", -imm);
	strcat_p2(parse_out, word);
	strcat_p2(parse_out, rs1);
	strcat_p2(parse_out, (char*)") // ");
	strcat_p2(parse_out, comment);
	strcat_p2(parse_out, (char*)"\n");
	parse_out->index++;
	PC[0] += 4;
}

UINT8 alloc_temp_UABI(reg_table_struct* reg_table, UINT depth, parse_struct2* parse_out, parse_struct2* parse_in, FILE* Masm) {
	int debug = 0;
	UINT8 start = reg_table->temp;
	int count = 0;
	if (reg_table->temp >= 32)
		reg_table->temp = 5;
	while (reg_table->entry_x[reg_table->temp].in_use && count < 32) {
		reg_table->temp++;
		if (reg_table->temp >= 32)
			reg_table->temp = 5;
		count++;
	}
	if (reg_table->entry_x[reg_table->temp].in_use == 1) {
		print_parse_out(Masm, parse_out);
		exit(1);
	}
	reg_table->entry_x[reg_table->temp].in_use = 1;
	reg_table->entry_x[reg_table->temp].temp = 1;
	UINT temp = reg_table->temp;
	reg_table->temp++;
	if (reg_table->temp == 32)
		reg_table->temp = 5;

	reg_table->t_count = (reg_table->entry_x[5].in_use == 1) ? 1 : 0;
	if (reg_table->entry_x[15].in_use == 1)
		reg_table->t_count++;
	reg_table->s_count = (reg_table->entry_x[14].in_use == 1) ? 1 : 0;
	for (UINT8 i = 6; i < 10; i++)
		if (reg_table->entry_x[i].in_use == 1)
			reg_table->s_count++;
	for (UINT16 i = 16; i < 32; i++)
		if (reg_table->entry_x[i].in_use == 1)
			reg_table->s_count++;
	return temp;
}
UINT8 alloc_float(reg_table_struct* reg_table, UINT depth, parse_struct2* parse_out, parse_struct2* parse_in, FILE* Masm) {
	int debug = 0;
	reg_table->fp_ptr &= 0x1f;
	UINT8 start = reg_table->fp_ptr;
	int count = 0;
	while (reg_table->entry_fp[reg_table->fp_ptr].in_use && count < 32) {
		reg_table->fp_ptr++;
		reg_table->fp_ptr &= 0x1f;
		count++;
	}
	if (reg_table->entry_fp[reg_table->fp_ptr].in_use == 1) {
		print_parse_out(Masm, parse_out);
		exit(1);
	}
	reg_table->entry_fp[reg_table->fp_ptr].in_use = 1;
	reg_table->entry_fp[reg_table->fp_ptr].saved_depth = depth;
	UINT fp_ptr = reg_table->fp_ptr;
	reg_table->fp_ptr++;
	reg_table->fp_ptr &= 0x1f;

	reg_table->count_fp = 0;
	for (UINT8 i = 0; i < 0x20; i++)
		if (reg_table->entry_fp[i].in_use)reg_table->count_fp++;
	return fp_ptr;
}
UINT8 alloc_saved_EABI(reg_table_struct* reg_table, UINT depth, parse_struct2* parse_out, parse_struct2* parse_in, FILE* Masm) {
	int debug = 0;
	UINT8 start = reg_table->saved;
	int count = 0;
	if (reg_table->saved >= 31)
		reg_table->saved = 6;
	else if (reg_table->saved == 0x0f)
		reg_table->saved = 0x10;
	while (reg_table->entry_x[reg_table->saved].in_use && count < 28) {
		reg_table->saved++;
		if (reg_table->saved >= 31)
			reg_table->saved = 6;
		else if (reg_table->saved == 0x0f)
			reg_table->saved = 0x10;
		count++;
	}
	if (reg_table->entry_x[reg_table->saved].in_use == 1) {
		debug++;
		print_parse_out(Masm, parse_out);
		exit(1);
	}
	reg_table->entry_x[reg_table->saved].in_use = 1;
	reg_table->entry_x[reg_table->saved].temp = 0;
	reg_table->entry_x[reg_table->saved].saved = 1;
	reg_table->entry_x[reg_table->saved].saved_depth = depth;
	UINT temp = reg_table->saved;
	reg_table->saved++;
	if (reg_table->saved == 28)
		reg_table->saved = 8;

	reg_table->t_count = (reg_table->entry_x[5].in_use == 1) ? 1 : 0;
	if (reg_table->entry_x[15].in_use == 1)
		reg_table->t_count++;
	reg_table->s_count = (reg_table->entry_x[14].in_use == 1) ? 1 : 0;
	for (UINT8 i = 6; i < 10; i++)
		if (reg_table->entry_x[i].in_use == 1)
			reg_table->s_count++;
	for (UINT16 i = 16; i < 32; i++)
		if (reg_table->entry_x[i].in_use == 1)
			reg_table->s_count++;
	return temp;
}
UINT8 alloc_jalr_UABI(reg_table_struct* reg_table, UINT depth, parse_struct2* parse_out, FILE* Masm) {
	int debug = 0;
	UINT8 start = reg_table->temp;
	int count = 0;
	if (reg_table->temp >= 32 || reg_table->temp == 5)
		reg_table->temp = 6;
	while (reg_table->entry_x[reg_table->temp].in_use && count < 32) {
		reg_table->temp++;
		if (reg_table->temp >= 32)
			reg_table->temp = 6;
		count++;
	}
	if (reg_table->entry_x[reg_table->temp].in_use == 1) {
		debug++;
		print_parse_out(Masm, parse_out);
		exit(1);
	}
	reg_table->entry_x[reg_table->temp].in_use = 1;
	reg_table->entry_x[reg_table->temp].temp = 1;
	UINT temp = reg_table->temp;
	reg_table->temp++;
	if (reg_table->temp == 32)
		reg_table->temp = 6;
	return temp;
}
void addVariableEntry(VariableListEntry* entry, data_type_enum type, char* name, reg_table_struct* reg_table, loop_control* l_control, parse_struct2* parse_out, parse_struct2* parse_in, FILE* Masm) {
	if (type == fp16_enum) {
		entry->reg_index = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
	}
	else {
		entry->reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
	}
	entry->reg_index_valid = 1;
	sprintf_s(entry->name,"%s", name);
	entry->type = type;
}

void load_64bB(parse_struct2* parse_out, INT64* PC, UINT8 top, INT64 target_addr, char* target_name, reg_table_struct* reg_table, UINT depth, parse_struct2* parse_in, param_type* param, FILE* Masm) {
	int debug = 0;
	if (target_addr > (-0x800) && target_addr < 0) {
//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x // %s\n", PC[0], reg_table->entry_x[top].name, target_addr, target_name); PC[0] += 4;
		sprint_op_2src(parse_out, PC, (char*)"addi", reg_table->entry_x[top].name, reg_table->entry_x[0].name, target_addr, param);
	}
	else {
		UINT index0 = (target_addr & 0x0fff);
		UINT index1 = (((target_addr) >> 12) & 0x0fffff);

		UINT index2 = (((target_addr) >> 32) & 0x0fff);
		UINT index3 = (((target_addr) >> 44) & 0x0fffff); // only valid to 64b
		char word[0x80];

		if (index2 != 0 || index3 != 0) {
			UINT8 upper = alloc_temp_UABI(reg_table, depth, parse_out, parse_in, Masm);
			UINT8 lower = alloc_temp_UABI(reg_table, depth, parse_out, parse_in, Masm);
			if (index3 == 0) {
				if (index2 != 0) {
	//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, %#05x\n", PC[0], reg_table->entry_x[upper].name, index2); PC[0] += 4;
	//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 0x20\n", PC[0], reg_table->entry_x[top].name, reg_table->entry_x[upper].name); PC[0] += 4;
					sprint_op_2src(parse_out, PC, (char*)"addi", reg_table->entry_x[upper].name, reg_table->entry_x[0].name, index2, param);
					sprint_op_2src(parse_out, PC, (char*)"slli", reg_table->entry_x[top].name, reg_table->entry_x[upper].name, 0x20, param);
				}
			}
			else {
//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s, 0x%05x // start 64b load %s\n", PC[0], reg_table->entry_x[upper].name, index3, target_name); PC[0] += 4;
				sprintf_s(word, "start 64b load %s", target_name);
				sprint_op_imm(parse_out, PC, (char*)"lui", reg_table->entry_x[upper].name, index3, word, param);
				if (index2 != 0) {
					if (index2 < 0x800) {
				//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ori %s, %s, 0x%03x\n", PC[0], reg_table->entry_x[upper].name, reg_table->entry_x[upper].name, index2); PC[0] += 4;
						sprint_op_2src(parse_out, PC, (char*)"ori", reg_table->entry_x[upper].name, reg_table->entry_x[upper].name, index2, param);
					}
					else {
//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 0x%03x\n", PC[0], reg_table->entry_x[upper].name, reg_table->entry_x[upper].name, index2); PC[0] += 4;
						sprint_op_2src(parse_out, PC, (char*)"andi", reg_table->entry_x[upper].name, reg_table->entry_x[upper].name, index2, param);
					}
				}
//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 0x20\n", PC[0], reg_table->entry_x[top].name, reg_table->entry_x[upper].name); PC[0] += 4;
				sprint_op_2src(parse_out, PC, (char*)"slli", reg_table->entry_x[top].name, reg_table->entry_x[upper].name, 0x20, param);
			}
			if (index3 != 0 || index2 != 0) {
//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,0x%05x\n", PC[0], reg_table->entry_x[lower].name, index1); PC[0] += 4;
				sprintf_s(word, "");
				sprint_op_imm(parse_out, PC, (char*)"lui", reg_table->entry_x[lower].name, index1, word, param);
				if (index0 != 0) {
					if (index0 < 0x800) {
//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ori %s, %s, 0x%03x\n", PC[0], reg_table->entry_x[lower].name, reg_table->entry_x[lower].name, index0); PC[0] += 4;
						sprint_op_2src(parse_out, PC, (char*)"ori", reg_table->entry_x[lower].name, reg_table->entry_x[lower].name, index0, param);
					}
					else {
	//					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 0x%03x\n", PC[0], reg_table->entry_x[lower].name, reg_table->entry_x[lower].name, index0); PC[0] += 4;
						sprint_op_2src(parse_out, PC, (char*)"andi", reg_table->entry_x[lower].name, reg_table->entry_x[lower].name, index0, param);
					}
				}
	//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s	// completed load of 0x%016I64x as 64b immediate load \"%s\"\n", PC[0], reg_table->entry_x[top].name, reg_table->entry_x[top].name, reg_table->entry_x[lower].name, (INT64)target_addr, target_name); PC[0] += 4;
				sprintf_s(word, "completed load of 0x%016I64x as 64b immediate load \"%s\"", (INT64)target_addr, target_name);
				sprint_op_2src(parse_out, PC, (char*)"add ", reg_table->entry_x[top].name, reg_table->entry_x[top].name, reg_table->entry_x[lower].name, word, param);
			}
			else {
				if (index0 != 0) {
			//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s, 0x%05x\n", PC[0], reg_table->entry_x[lower].name, index1); PC[0] += 4;
					sprintf_s(word, "");
					sprint_op_imm(parse_out, PC, (char*)"lui", reg_table->entry_x[lower].name, index1, word, param);
					if (index0 < 0x800) {
//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ori %s, %s, 0x%03x	// completed load of 0x%016I64x as 64b immediate load \"%s\"\n", PC[0], reg_table->entry_x[top].name, reg_table->entry_x[lower].name, index0, (INT64)target_addr, target_name); PC[0] += 4;
						sprintf_s(word, "completed load of 0x%016I64x as 64b immediate load \"%s\"", (INT64)target_addr, target_name); 
						sprint_op_2src(parse_out, PC, (char*)"ori", reg_table->entry_x[top].name, reg_table->entry_x[lower].name, index0, word, param);
					}
					else {
//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 0x%03x	// completed load of 0x%016I64x as 64b immediate load \"%s\"\n", PC[0], reg_table->entry_x[top].name, reg_table->entry_x[lower].name, index0, (INT64)target_addr, target_name); PC[0] += 4;
						sprintf_s(word, "completed load of 0x%016I64x as 64b immediate load \"%s\"", (INT64)target_addr, target_name); 
						sprint_op_2src(parse_out, PC, (char*)"andi", reg_table->entry_x[top].name, reg_table->entry_x[lower].name, index0, word, param);
					}
				}
				else {
		//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s, 0x%05x	// completed load of 0x%016I64x as 64b immediate load \"%s\"\n", PC[0], reg_table->entry_x[top].name, index1, (INT64)target_addr, target_name); PC[0] += 4;
					sprintf_s(word, "completed load of 0x%016I64x as 64b immediate load \"%s\"", (INT64)target_addr, target_name);
					sprint_op_imm(parse_out, PC, (char*)"lui", reg_table->entry_x[top].name, index1, word, param);
				}
			}
			reg_table->entry_x[upper].in_use = 0;
			reg_table->entry_x[lower].in_use = 0;
		}
		else if (target_addr >= 0x800) {
			if (index0 == 0) {
//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // 32b load complete, lower bits are zero\n", PC[0], reg_table->entry_x[top].name, index1); PC[0] += 4;
				sprintf_s(word, "32b load complete, lower bits are zero");
				sprint_op_imm(parse_out, PC, (char*)"lui", reg_table->entry_x[top].name, index1, word, param);
			}
			else if (index1 == 0) {
				if (index0 < 0x800) {
		//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ori %s, zero, %#05x	// completed load of 0x%08x as 32b immediate load \"%s\"\n", PC[0], reg_table->entry_x[top].name, index0, target_addr, target_name); PC[0] += 4;
					sprintf_s(word, "completed 32b immediate load of 0x%08x to \"%s\"", (INT64)target_addr, target_name); 
					sprint_op_2src(parse_out, PC, (char*)"ori", reg_table->entry_x[top].name, reg_table->entry_x[0].name, index0, word, param);
				}
				else {
		//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  0x0000000 // prep to load # <0x1000 && >=0x800 \n", PC[0], reg_table->entry_x[top].name, index1); PC[0] += 4;
					sprintf_s(word, "prep to load # <0x1000 && >=0x800");
					sprint_op_imm(parse_out, PC, (char*)"lui", reg_table->entry_x[top].name, index1, word, param);
		//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, %#05x	// completed load of 0x%08x as 32b immediate load \"%s\"\n", PC[0], reg_table->entry_x[top].name, reg_table->entry_x[top].name, index0, target_addr, target_name); PC[0] += 4;
					sprintf_s(word, "completed 32b immediate load of 0x%08x to \"%s\"", (INT64)target_addr, target_name); 
					sprint_op_2src(parse_out, PC, (char*)"andi", reg_table->entry_x[top].name, reg_table->entry_x[top].name, index0, word, param);
				}
			}
			else {
		//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // 32b load complete, lower bits are zero", PC[0], reg_table->entry_x[top].name, index1); PC[0] += 4;
				sprintf_s(word, "32b load complete, lower bits are zero");
				sprint_op_imm(parse_out, PC, (char*)"lui", reg_table->entry_x[top].name, index1, word, param);
				if (index0 < 0x800) {
	//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ori %s, %s, %#05x	// completed load of 0x%08x as 32b immediate load \"%s\"\n", PC[0], reg_table->entry_x[top].name, reg_table->entry_x[top].name, index0, target_addr, target_name); PC[0] += 4;
					sprintf_s(word, "completed 32b immediate load of 0x%08x to \"%s\"\n", (INT64)target_addr, target_name); 
					sprint_op_2src(parse_out, PC, (char*)"ori", reg_table->entry_x[top].name, reg_table->entry_x[top].name, index0, word, param);
				}
				else {
//					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, %#05x	// completed load of 0x%08x as 32b immediate load \"%s\"\n", PC[0], reg_table->entry_x[top].name, reg_table->entry_x[top].name, index0, target_addr, target_name); PC[0] += 4;
					sprintf_s(word, "completed 32b immediate load of 0x%08x to \"%s\"", (INT64)target_addr, target_name);
					sprint_op_2src(parse_out, PC, (char*)"andi", reg_table->entry_x[top].name, reg_table->entry_x[top].name, index0, word, param);
				}
			}
		}
		else {
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, %#05x	// completed load of 0x%08x as 12b immediate load \"%s\"\n", PC[0], reg_table->entry_x[top].name, index0, target_addr, target_name); PC[0] += 4;
			sprintf_s(word, "completed 32b immediate load of 0x%08x to \"%s\"", (INT64)target_addr, target_name); 
			sprint_op_2src(parse_out, PC, (char*)"addi", reg_table->entry_x[top].name, reg_table->entry_x[0].name, index0, word, param);
		}
	}
}

void getWord(parse_struct2* parse) {
	int debug = 0;
	UINT16 length = strlen(parse->line[parse->index].line);
	UINT16 i;
	if (length >= 0x200)
		debug++;
	parse->word[0] = '\0';
	parse->word_ptr = 0;
	if (parse->ptr < length) {
		i = parse->ptr;
		if (parse->line[parse->index].line[i] == '-' && (parse->line[parse->index].line[i + 1] == '0' || parse->line[parse->index].line[i + 1] == '1' || parse->line[parse->index].line[i + 1] == '2' || parse->line[parse->index].line[i + 1] == '3' || parse->line[parse->index].line[i + 1] == '4' ||
			parse->line[parse->index].line[i + 1] == '5' || parse->line[parse->index].line[i + 1] == '6' || parse->line[parse->index].line[i + 1] == '7' || parse->line[parse->index].line[i + 1] == '8' || parse->line[parse->index].line[i + 1] == '9')) {
			parse->word[parse->word_ptr++] = parse->line[parse->index].line[i];
			parse->word[parse->word_ptr] = '\0';
			if (parse->line[parse->index].line[i] == '-') {
				i++;
				while (parse->line[parse->index].line[i] == ' ' || parse->line[parse->index].line[i] == '\t')i++;
			}
			else {
				i++;
			}
		}
		for (; i < length && debug == 0; i++) {
			if (parse->line[parse->index].line[i] != ' ' && parse->line[parse->index].line[i] != '\t' && parse->line[parse->index].line[i] != '\0' && parse->line[parse->index].line[i] != '\n' && parse->line[parse->index].line[i] != ',' && parse->line[parse->index].line[i] != ';' && parse->line[parse->index].line[i] != ':' && parse->line[parse->index].line[i] != '=' && parse->line[parse->index].line[i] != '?' && parse->line[parse->index].line[i] != '&' &&
				parse->line[parse->index].line[i] != '|' && parse->line[parse->index].line[i] != '^' && parse->line[parse->index].line[i] != '+' && parse->line[parse->index].line[i] != '-' && parse->line[parse->index].line[i] != '*' && parse->line[parse->index].line[i] != '/' && parse->line[parse->index].line[i] != '<' && parse->line[parse->index].line[i] != '>' &&
				parse->line[parse->index].line[i] != '(' && parse->line[parse->index].line[i] != ')' && parse->line[parse->index].line[i] != '{' && parse->line[parse->index].line[i] != '}' && parse->line[parse->index].line[i] != '[' && parse->line[parse->index].line[i] != ']' && parse->line[parse->index].line[i] != '\"') {
				parse->word[parse->word_ptr++] = parse->line[parse->index].line[i];
				parse->word[parse->word_ptr] = '\0';
			}
			else {
				debug++;
			}
		}
		parse->ptr += parse->word_ptr;
	}
//	else {
//		parse->word[0] = '\0';
//	}
}
void getWord(parse_struct2* parse, UINT index, UINT* ptr) {
	int debug = 0;
	UINT16 length = strlen(parse->line[index].line);
	UINT16 i;

	for (i = ptr[0]; i < length && debug == 0; i++) {
		if (parse->line[index].line[i] != ' ' && parse->line[index].line[i] != '\t' && parse->line[index].line[i] != '\0' && parse->line[index].line[i] != '\n' && parse->line[index].line[i] != ',' && parse->line[index].line[i] != ';' && parse->line[index].line[i] != ':' && parse->line[index].line[i] != '=' && parse->line[index].line[i] != '?' && parse->line[index].line[i] != '&' &&
			parse->line[index].line[i] != '|' && parse->line[index].line[i] != '^' && parse->line[index].line[i] != '+' && parse->line[index].line[i] != '-' && parse->line[index].line[i] != '*' && parse->line[index].line[i] != '/' && parse->line[index].line[i] != '<' && parse->line[index].line[i] != '>' &&
			parse->line[index].line[i] != '(' && parse->line[index].line[i] != ')' && parse->line[index].line[i] != '{' && parse->line[index].line[i] != '}' && parse->line[index].line[i] != '[' && parse->line[index].line[i] != ']' && parse->line[index].line[i] != '\"') {
			parse->word[i - ptr[0]] = parse->line[index].line[i];
		}
		else {
			debug++;
		}
	}
	if (ptr[0] < length)
		parse->word[i - ptr[0] - 1] = '\0';
	else
		parse->word[0] = '\0';
	ptr[0] += strlen(parse->word);
}
void getWordB(parse_struct2* parse) {
	int debug = 0;
	UINT16 length = strlen(parse->line[parse->index].line);
	UINT16 i;
	while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\"' || parse->line[parse->index].line[parse->ptr] == '\t')parse->ptr++;

	for (i = parse->ptr; i < length && debug == 0; i++) {
		if (parse->line[parse->index].line[i] != ' ' && parse->line[parse->index].line[i] != '\t' && parse->line[parse->index].line[i] != '\0' && parse->line[parse->index].line[i] != '\n' && parse->line[parse->index].line[i] != ',' && parse->line[parse->index].line[i] != ';' && parse->line[parse->index].line[i] != ':' && parse->line[parse->index].line[i] != '=' && parse->line[parse->index].line[i] != '?' && parse->line[parse->index].line[i] != '&' &&
			parse->line[parse->index].line[i] != '|' && parse->line[parse->index].line[i] != '^' && parse->line[parse->index].line[i] != '+' && parse->line[parse->index].line[i] != '-' && parse->line[parse->index].line[i] != '*' && parse->line[parse->index].line[i] != '/' && parse->line[parse->index].line[i] != '<' && parse->line[parse->index].line[i] != '>' &&
			parse->line[parse->index].line[i] != '(' && parse->line[parse->index].line[i] != ')' && parse->line[parse->index].line[i] != '{' && parse->line[parse->index].line[i] != '}' && parse->line[parse->index].line[i] != '[' && parse->line[parse->index].line[i] != ']' && parse->line[parse->index].line[i] != '\"') {
			parse->word[i - parse->ptr] = parse->line[parse->index].line[i];
		}
		else {
			debug++;
		}
	}
	parse->word[i - parse->ptr - 1] = '\0';
	parse->ptr += strlen(parse->word);
	while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\t')parse->ptr++;
}

void getWordSign(parse_struct2* parse) {
	int debug = 0;
	//	char result[0x100];
	UINT16 length = strlen(parse->line[parse->index].line);
	UINT16 i;
	while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\"' || parse->line[parse->index].line[parse->ptr] == '\t')parse->ptr++;
	char sign_index = 0;
	if (parse->line[parse->index].line[parse->ptr] == '-') {
		parse->word[0] = '-';
		sign_index = 1;
		parse->ptr++;
	}
	while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\"' || parse->line[parse->index].line[parse->ptr] == '\t')parse->ptr++;
	for (i = parse->ptr; i < length && debug == 0; i++) {
		if (parse->line[parse->index].line[i] != ' ' && parse->line[parse->index].line[i] != '\t' && parse->line[parse->index].line[i] != '\0' && parse->line[parse->index].line[i] != '\n' && parse->line[parse->index].line[i] != ',' && parse->line[parse->index].line[i] != ';' && parse->line[parse->index].line[i] != ':' && parse->line[parse->index].line[i] != '=' && parse->line[parse->index].line[i] != '?' && parse->line[parse->index].line[i] != '&' &&
			parse->line[parse->index].line[i] != '|' && parse->line[parse->index].line[i] != '^' && parse->line[parse->index].line[i] != '+' && parse->line[parse->index].line[i] != '-' && parse->line[parse->index].line[i] != '*' && parse->line[parse->index].line[i] != '/' && parse->line[parse->index].line[i] != '<' && parse->line[parse->index].line[i] != '>' &&
			parse->line[parse->index].line[i] != '(' && parse->line[parse->index].line[i] != ')' && parse->line[parse->index].line[i] != '{' && parse->line[parse->index].line[i] != '}' && parse->line[parse->index].line[i] != '[' && parse->line[parse->index].line[i] != ']' && parse->line[parse->index].line[i] != '\"') {
			parse->word[i - parse->ptr + sign_index] = parse->line[parse->index].line[i];
		}
		else {
			debug++;
		}
	}
	parse->word[i - parse->ptr + sign_index - 1] = '\0';
	parse->ptr += strlen(parse->word);
	while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\t')parse->ptr++;
}

void push_all(parse_struct2* parse_out, INT64* PC, param_type* param, reg_table_struct* reg_table) {
	int debug = 0;
	switch (param->mxl) {
	case 1:
		for (UINT i = 5; i < 0x20; i++) {
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sw %s, -0x%03x(sp)		//\n",	PC[0], reg_table->entry_x[i].name, ((i - 5) + 1) * 4); PC[0] += 4;
			sprint_load(parse_out, PC, (char*)"sw  ", reg_table->entry_x[i].name, (INT16)-(((i - 5) + 1) * 4), (char*)"sp", (char*)"push all int", param);
		}
		for (UINT i = 0; i < 0x20; i++) {
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sw %s, -0x%03x(sp)		//\n",	PC[0], reg_table->entry_fp[i].name, ((i - 5) + 1+0x20) * 4); PC[0] += 4;
			sprint_load(parse_out, PC, (char*)"fsw ", reg_table->entry_fp[i].name, (INT16)-(((i - 5) + 1 + 0x20) * 4), (char*)"sp", (char*)"push all fp", param);
		}
//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi sp, sp, -0x%03x		//\n",	PC[0], 0x40 * 4); PC[0] += 4;
		sprint_op_2src(parse_out, PC, (char*)"addi", (char*)"sp", (char*)"sp", (INT16)-(0x40 * 4), param);
		break;
	case 2:
		for (UINT i = 5; i < 0x20; i++) {
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, -0x%03x(sp)		//restore a0-a3\n",	PC[0], reg_table->entry_x[i].name, ((i - 5) + 1) * 8); PC[0] += 4;
			sprint_load(parse_out, PC, (char*)"sd  ", reg_table->entry_x[i].name, (INT16)-(((i - 5) + 1) * 8), (char*)"sp", (char*)"push all int", param);
		}
		for (UINT i = 0; i < 0x20; i++) {
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, -0x%03x(sp)		//\n",	PC[0], reg_table->entry_fp[i].name, ((i - 5) + 1 + 0x20) * 8); PC[0] += 4;
			sprint_load(parse_out, PC, (char*)"fsd ", reg_table->entry_fp[i].name, (INT16)-(((i - 5) + 1 + 0x20) * 8), (char*)"sp", (char*)"push all fp", param);
		}
//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi sp, sp, -0x%03x		//\n",	PC[0], 0x40 * 8); PC[0] += 4;
		sprint_op_2src(parse_out, PC, (char*)"addi", (char*)"sp", (char*)"sp", (INT16)-(0x40 * 8), param);
		break;
	case 3:
		for (UINT i = 5; i < 0x20; i++) {
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sq %s, -0x%03x(sp)		//restore a0-a3\n",		PC[0], reg_table->entry_x[i].name, ((i - 5) + 1) * 16); PC[0] += 4;
			sprint_load(parse_out, PC, (char*)"sq  ", reg_table->entry_x[i].name, (INT16)-(((i - 5) + 1) * 16), (char*)"sp", (char*)"push all int", param);
		}
		for (UINT i = 0; i < 0x20; i++) {
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sq %s, -0x%03x(sp)		//\n",		PC[0], reg_table->entry_fp[i].name, ((i - 5) + 1 + 0x20) * 16); PC[0] += 4;
			sprint_load(parse_out, PC, (char*)"fsq ", reg_table->entry_fp[i].name, (INT16)-(((i - 5) + 1 + 0x20) * 16), (char*)"sp", (char*)"push all fp", param);
		}
//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi sp, sp, -0x%03x		//\n",	PC[0], 0x40 * 16); PC[0] += 4;
		sprint_op_2src(parse_out, PC, (char*)"addi", (char*)"sp", (char*)"sp", (INT16)-(0x40 * 16), param);
		break;
	default:
		debug++;
		break;
	}
}
void pop_all(parse_struct2* parse_out, INT64* PC, param_type* param, reg_table_struct* reg_table) {
	int debug = 0;
	switch (param->mxl) {
	case 1:
//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi sp, sp, 0x%03x		//\n",PC[0], 0x40 * 4); PC[0] += 4;
		sprint_op_2src(parse_out, PC, (char*)"addi", (char*)"sp", (char*)"sp", (INT16)(0x40 * 4), param);
		for (INT i = 0x1f; i >=0; i--) {
	//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lw %s, -0x%03x(sp)		//\n",PC[0], reg_table->entry_fp[i].name, ((i - 5) + 1 + 0x20) * 4); PC[0] += 4;
			sprint_load(parse_out, PC, (char*)"flw ", reg_table->entry_fp[i].name, (INT16)-(((i - 5) + 1 + 0x20) * 4),(char*)"sp", (char*)"pop all fp", param);
		}
		for (INT i = 0x1f; i >= 5; i--) {
	//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lw %s, -0x%03x(sp)		//\n",	PC[0], reg_table->entry_x[i].name, ((i - 5) + 1) * 4); PC[0] += 4;
			sprint_load(parse_out, PC, (char*)"lw  ", reg_table->entry_x[i].name, (INT16)-(((i - 5) + 1) * 4), (char*)"sp", (char*)"pop all int", param);
		}
		break;
	case 2:
//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi sp, sp, 0x%03x		//\n",PC[0], 0x40 * 8); PC[0] += 4;
		sprint_op_2src(parse_out, PC, (char*)"addi", (char*)"sp", (char*)"sp", (INT16)(0x40 * 8), param);
		for (INT i = 0x1f; i >= 0; i--) {
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -0x%03x(sp)		//\n",	PC[0], reg_table->entry_fp[i].name, ((i - 5) + 1 + 0x20) * 8); PC[0] += 4;
			sprint_load(parse_out, PC, (char*)"fld ", reg_table->entry_fp[i].name, (INT16)-(((i - 5) + 1 + 0x20) * 8), (char*)"sp", (char*)"pop all fp", param);
		}
		for (INT i = 0x1f; i >= 5; i--) {
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -0x%03x(sp)		//\n",	PC[0], reg_table->entry_x[i].name, ((i - 5) + 1) * 8); PC[0] += 4;
			sprint_load(parse_out, PC, (char*)"ld  ", reg_table->entry_x[i].name, (INT16)-(((i - 5) + 1) * 8), (char*)"sp", (char*)"pop all int", param);
		}
		break;
	case 3:
//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi sp, sp, 0x%03x		//\n",PC[0], 0x40 * 16); PC[0] += 4;
		sprint_op_2src(parse_out, PC, (char*)"addi", (char*)"sp", (char*)"sp", (INT16)(0x40 * 16), param);
		for (INT i = 0x1f; i >= 0; i--) {
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lq %s, -0x%03x(sp)		//\n",	PC[0], reg_table->entry_fp[i].name, ((i - 5) + 1 + 0x20) * 16); PC[0] += 4;
			sprint_load(parse_out, PC, (char*)"flq ", reg_table->entry_fp[i].name, (INT16)-(((i - 5) + 1 + 0x20) * 16), (char*)"sp", (char*)"pop all fp", param);
		}
		for (INT i = 0x1f; i >= 5; i--) {
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lq %s, -0x%03x(sp)		//\n",	PC[0], reg_table->entry_x[i].name, ((i - 5) + 1) * 16); PC[0] += 4;
			sprint_load(parse_out, PC, (char*)"lq  ", reg_table->entry_x[i].name, (INT16)-(((i - 5) + 1) * 16), (char*)"sp", (char*)"pop all int", param);
		}
		break;
	default:
		debug++;
		break;
	}
}

void parse_subset(parse_struct2* parse_out, VariableListEntry* target, INT8* subset_depth, parse_struct2* parse_in, compiler_var_type* compiler_var, IO_list_type* IO_list, operator_type* op_table, operator_type* branch_table,
	var_type_struct* csr_list, UINT8 csr_list_count, INT64* PC, reg_table_struct* reg_table, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug);
void parse_index_a(parse_struct2* parse_out, VariableListEntry* target, UINT8 address_only, INT64* PC, VariableListEntry* current, parse_struct2* parse, compiler_var_type* compiler_var, IO_list_type* IO_list,
	operator_type* op_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, reg_table_struct* reg_table, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug);
void parse_index_b(parse_struct2* parse_out, VariableListEntry* target, INT64* number, UINT8* number_valid, INT64* PC, VariableListEntry* current, parse_struct2* parse, compiler_var_type* compiler_var,
	IO_list_type* IO_list, operator_type* op_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, reg_table_struct* reg_table, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug);
void ParseFunctionCode(parse_struct2* parse_out, parse_struct2* parse_in, loop_control* l_control, INT64* logical_PC, var_type_struct* control_type, var_type_struct* modifier_type, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, operator_type* op_table,
	reg_table_struct* reg_table, memory_space_type* memory_space, hw_pointers* pointers, INT64 base, compiler_var_type* compiler_var, short* sp_offset, FunctionListEntry* current_function, IO_list_type* IO_list, param_type* param, FILE* Masm, UINT8 unit_debug);
void parse_rhs_continued(parse_struct2* parse_out, VariableListEntry* target, VariableListEntry* s2, UINT part2, UINT8* num_valid, INT64* number, INT64* PC, parse_struct2* parse_in, compiler_var_type* compiler_var, IO_list_type* IO_list,
	operator_type* op_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, reg_table_struct* reg_table, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug);
void cast_variable(parse_struct2* parse_out, parse_struct2* parse_in, VariableListEntry* test, VariableListEntry* target, loop_control* l_control, INT64* logical_PC, IO_list_type* IO_list,
	reg_table_struct* reg_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, operator_type* op_table, compiler_var_type* compiler_var, param_type* param, FILE* Masm, UINT8 unit_debug);
void parse_rh_of_eqB(parse_struct2* parse_out, VariableListEntry* target, UINT part2, UINT8* num_valid, INT64* number, INT64* PC, parse_struct2* parse_in, compiler_var_type* compiler_var, IO_list_type* IO_list,
	operator_type* op_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, reg_table_struct* reg_table, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug);
void check_if_cast(parse_struct2* parse_out, parse_struct2* parse_in, VariableListEntry* target, INT8* subset_depth, loop_control* l_control, INT64* logical_PC, IO_list_type* IO_list, reg_table_struct* reg_table, 
	operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, operator_type* op_table, compiler_var_type* compiler_var, param_type* param, FILE* Masm, UINT8 unit_debug);
void clean_temp_reg(reg_table_struct* reg_table) {
	for (UINT i = 5; i < 0x20; i++) {
		if (reg_table->entry_x[i].temp == 1) {
			reg_table->entry_x[i].temp = 0;
			reg_table->entry_x[i].in_use = 0;
		}
	}
	reg_table->count_x = 5;
	for (UINT i = 5; i < 0x20; i++) {
		if (reg_table->entry_x[i].in_use)
			reg_table->count_x++;
	}
	reg_table->count_fp = 0;
	for (UINT i = 0; i < 0x20; i++) {
		if (reg_table->entry_fp[i].in_use)
			reg_table->count_fp++;
	}
}

UINT8 get_float(fp_data_struct* fp_data, char* input) {
	int debug = 0;
	UINT8 result = 0;
	UINT8 i = 0;
	UINT whole = 0;
	UINT fraction = 0;
	UINT fraction_base = 0;
	UINT sign = 0;
	UINT exit_loop = 0;
	if (input[0] == '-') {
		sign = 1;
		i = 1;
	}
	for (; i < strlen(input) && !exit_loop; i++) {
		switch (input[i]) {
		case '0':
			fraction *= 10;
			fraction_base *= 10;
			break;
		case '1':
			fraction *= 10;
			fraction_base *= 10;
			fraction += 1;
			break;
		case '2':
			fraction *= 10;
			fraction_base *= 10;
			fraction += 2;
			break;
		case '3':
			fraction *= 10;
			fraction_base *= 10;
			fraction += 3;
			break;
		case '4':
			fraction *= 10;
			fraction_base *= 10;
			fraction += 4;
			break;
		case '5':
			fraction *= 10;
			fraction_base *= 10;
			fraction += 5;
			break;
		case '6':
			fraction *= 10;
			fraction_base *= 10;
			fraction += 6;
			break;
		case '7':
			fraction *= 10;
			fraction_base *= 10;
			fraction += 7;
			break;
		case '8':
			fraction *= 10;
			fraction_base *= 10;
			fraction += 8;
			break;
		case '9':
			fraction *= 10;
			fraction_base *= 10;
			fraction += 9;
			break;
		case '.':
			whole = fraction;
			fraction = 0;
			fraction_base = 1;
			result = 1;
			break;
		case 'e':
			debug++;
			break;
		default:
			exit_loop = 1;// not a number, likely format error
			break;
		}
	}
	if (!exit_loop && fraction_base > 0) {
		UINT8 fp_exp = 0x0f;
		double fraction_number = (double)fraction / (double)fraction_base;
		if (sign) {
			fp_data->hp = 0x8000;
			fp_data->sp = 0x80000000;
			fp_data->dp = 0x8000000000000000;
		}
		else {
			fp_data->hp = 0;
			fp_data->sp = 0;
			fp_data->dp = 0;
		}
		INT fraction_int = 0;
		if (whole > 0) {
			int count = 0;
			while (count < 13) {
				fraction_number *= 2.0;
				fraction_int << 1;
				if (fraction_number >= 1.0) {
					fraction_int++;
					fraction_number -= 1.0;
				}
				count++;
			}
			while (whole > 1 && fp_exp < 0x1f) {
				fraction_int >>= 1;
				fraction_int |= ((whole & 1) << 11);
				whole >> 1;
				fp_exp++;
			}
			//			fp_data->hp |= (fp_exp << 12);
			//			fp_data->hp |= (fraction_int & 0x0fff);
		}
		else {
			int hp_set = 0;
			int count = 0;
			fp_data->hp = 0;
			int exp_set = 0;
			while (count <= 10 && fp_exp > 0) {// bit 11 is masked out
				fraction_number *= 2.0;
				fraction_int << 1;
				count++;
				if (fraction_number >= 1.0) {
					fraction_int++;
					fraction_number -= 1.0;
					exp_set = 1;
				}
				else {
					if (exp_set == 0) {
						fp_exp--;
						count = 0;
					}
				}
			}
		}
		fp_data->hp |= (fp_exp << 12);
		fp_data->hp |= (fraction_int & 0x03ff);
	}
	else {
		result = 0;
	}
	return result;
}
UINT8 get_integer(INT64* number, char* input) {
	int debug = 0;
	UINT8 result = 1;
	number[0] = 0;
	UINT8 hex = 0;
	UINT8 i = 0;
	UINT sign = (input[0] == '-') ? 1 : 0;
	if (input[0] == '-') {
		i++;
	}
	if (strlen(input) > 2) {
		if (input[i] == '0' && input[i + 1] == 'x') {
			hex = 1;
			for (i += 2; i < strlen(input) && result; i++) {
//				if (input[i] != '_') {
					number[0] <<= 4;
					switch (input[i]) {
					case '_':// format spacing for legibility, ignore
						break;
					case '0':
						number[0] += 0;
						break;
					case '1':
						number[0] += 1;
						break;
					case '2':
						number[0] += 2;
						break;
					case '3':
						number[0] += 3;
						break;
					case '4':
						number[0] += 4;
						break;
					case '5':
						number[0] += 5;
						break;
					case '6':
						number[0] += 6;
						break;
					case '7':
						number[0] += 7;
						break;
					case '8':
						number[0] += 8;
						break;
					case '9':
						number[0] += 9;
						break;
					case 'a':
					case 'A':
						number[0] += 10;
						break;
					case 'b':
					case 'B':
						number[0] += 11;
						break;
					case 'c':
					case 'C':
						number[0] += 12;
						break;
					case 'd':
					case 'D':
						number[0] += 13;
						break;
					case 'e':
					case 'E':
						number[0] += 14;
						break;
					case 'f':
					case 'F':
						number[0] += 15;
						break;
					default:
						debug++;
						result = 0;
						break;
					}
//				}
			}
		}
	}
	if (!hex && input[0]!='\0') {
		for (; i < strlen(input) && result; i++) {
			number[0] *= 10;
			switch (input[i]) {
			case '0':
				number[0] += 0;
				break;
			case '1':
				number[0] += 1;
				break;
			case '2':
				number[0] += 2;
				break;
			case '3':
				number[0] += 3;
				break;
			case '4':
				number[0] += 4;
				break;
			case '5':
				number[0] += 5;
				break;
			case '6':
				number[0] += 6;
				break;
			case '7':
				number[0] += 7;
				break;
			case '8':
				number[0] += 8;
				break;
			case '9':
				number[0] += 9;
				break;
			default:
				debug++;
				result = 0;
				break;
			}
		}
	}
	else if (!hex) {
		result = 0;
	}
	if (sign == 1)
		number[0] = -number[0];
	return result;
}
UINT8 get_integer(INT64* number, parse_struct2* parse_in) {
	int debug = 0;
	UINT8 result = 1;
	number[0] = 0;
	UINT8 hex = 0;
	UINT8 i = 0;
	UINT sign = (parse_in->word[0] == '-') ? 1 : 0;

	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '-') {
		parse_in->ptr++;
		sign = 1;
	}
	if (parse_in->word[0] == '-') {
		i++;
	}
	if (strlen(parse_in->word) > 2) {
		if (parse_in->word[i] == '0' && parse_in->word[i + 1] == 'x') {
			hex = 1;
			for (i += 2; i < strlen(parse_in->word) && result; i++) {
				if (parse_in->word[i] != '_') {
					number[0] <<= 4;
					switch (parse_in->word[i]) {
					case '0':
						number[0] += 0;
						break;
					case '1':
						number[0] += 1;
						break;
					case '2':
						number[0] += 2;
						break;
					case '3':
						number[0] += 3;
						break;
					case '4':
						number[0] += 4;
						break;
					case '5':
						number[0] += 5;
						break;
					case '6':
						number[0] += 6;
						break;
					case '7':
						number[0] += 7;
						break;
					case '8':
						number[0] += 8;
						break;
					case '9':
						number[0] += 9;
						break;
					case 'a':
					case 'A':
						number[0] += 10;
						break;
					case 'b':
					case 'B':
						number[0] += 11;
						break;
					case 'c':
					case 'C':
						number[0] += 12;
						break;
					case 'd':
					case 'D':
						number[0] += 13;
						break;
					case 'e':
					case 'E':
						number[0] += 14;
						break;
					case 'f':
					case 'F':
						number[0] += 15;
						break;
					default:
						debug++;
						result = 0;
						break;
					}
				}
			}
		}
	}
	if (!hex && parse_in->word[0]!='\0') {
		for (; i < strlen(parse_in->word) && result; i++) {
			number[0] *= 10;
			switch (parse_in->word[i]) {
			case '0':
				number[0] += 0;
				break;
			case '1':
				number[0] += 1;
				break;
			case '2':
				number[0] += 2;
				break;
			case '3':
				number[0] += 3;
				break;
			case '4':
				number[0] += 4;
				break;
			case '5':
				number[0] += 5;
				break;
			case '6':
				number[0] += 6;
				break;
			case '7':
				number[0] += 7;
				break;
			case '8':
				number[0] += 8;
				break;
			case '9':
				number[0] += 9;
				break;
			default:
				debug++;
				result = 0;
				break;
			}
		}
	}
	else if (!hex) {
		result = 0;
	}
	if (sign == 1)
		number[0] = -number[0];
	return result;
}
UINT8 is_numberB(char* input) {
	switch (input[0]) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return 1;
		break;
	default:
		return 0;
		break;
	}
}
UINT8 get_reg_indexB(UINT8* index, char* word, reg_table_struct* reg_table) {
	UINT match = 0;
	for (UINT i = 0; i < 0x20; i++) {
		if (strcmp(word, reg_table->entry_x[i].name) == 0) {
			match = 1;
			index[0] = i;
		}
	}
	return match;
}

UINT8 get_12b_numberB(short* number, char* input) {
	int debug = 0;
	UINT8 result = 1;
	//	int number;
	number[0] = 0;
	UINT8 hex = 0;
	if (strlen(input) > 2) {
		if (input[0] == '0' && input[1] == 'x') {
			hex = 1;
			for (UINT8 i = 2; i < strlen(input) && result; i++) {
				number[0] <<= 4;
				switch (input[i]) {
				case '0':
					number[0] += 0;
					break;
				case '1':
					number[0] += 1;
					break;
				case '2':
					number[0] += 2;
					break;
				case '3':
					number[0] += 3;
					break;
				case '4':
					number[0] += 4;
					break;
				case '5':
					number[0] += 5;
					break;
				case '6':
					number[0] += 6;
					break;
				case '7':
					number[0] += 7;
					break;
				case '8':
					number[0] += 8;
					break;
				case '9':
					number[0] += 9;
					break;
				case 'a':
					number[0] += 10;
					break;
				case 'b':
					number[0] += 11;
					break;
				case 'c':
					number[0] += 12;
					break;
				case 'd':
					number[0] += 13;
					break;
				case 'e':
					number[0] += 14;
					break;
				case 'f':
					number[0] += 15;
					break;
				default:
					debug++;
					result = 0;
					break;
				}
			}// loop
			if (number[0] & 0x0800)number[0] |= 0xf000;
		}
	}
	if (!hex) {
		for (UINT8 i = 0; i < strlen(input) && result; i++) {
			number[0] *= 10;
			switch (input[i]) {
			case '0':
				number[0] += 0;
				break;
			case '1':
				number[0] += 1;
				break;
			case '2':
				number[0] += 2;
				break;
			case '3':
				number[0] += 3;
				break;
			case '4':
				number[0] += 4;
				break;
			case '5':
				number[0] += 5;
				break;
			case '6':
				number[0] += 6;
				break;
			case '7':
				number[0] += 7;
				break;
			case '8':
				number[0] += 8;
				break;
			case '9':
				number[0] += 9;
				break;
			default:
				debug++;
				result = 0;
				break;
			}
		}
	}
	return result;
}

UINT8 match_listB(UINT8* index, char* word, var_type_struct* list, UINT8 list_count, INT64 *PC) {
	UINT8 result = 0;
	for (index[0] = 0; index[0] < list_count && !result; index[0]++) {
		if (strcmp(word, list[index[0]].name) == 0) {
			result = 1;
		}
	}
	if (result == 0) {
		char word2[0x80];
		sprintf_s(word2, "%c%s", (PC[0] < 0x80000000) ? 'm' : 's', word);
		for (index[0] = 0; index[0] < list_count && !result; index[0]++) {
			if (strcmp(word2, list[index[0]].name) == 0) {
				result = 1;
			}
		}
	}
	if (result) {
		index[0]--;
	}
	return result;
}
UINT8 type_decode(data_type_enum* type, char* word) {
	UINT8 hit = 0;
	if ((strcmp(word, "int8") == 0) || (strcmp(word, "INT8") == 0) || (strcmp(word, "char") == 0) || (strcmp(word, "CHAR") == 0)) {
		type[0] = int8_enum;
		hit = 1;
	}
	else if ((strcmp(word, "int16") == 0) || (strcmp(word, "INT16") == 0) || (strcmp(word, "short") == 0) || (strcmp(word, "SHORT") == 0)) {
		type[0] = int16_enum;
		hit = 1;
	}
	else if ((strcmp(word, "int32") == 0) || (strcmp(word, "INT32") == 0) || (strcmp(word, "int") == 0) || (strcmp(word, "INT") == 0)) {
		type[0] = int32_enum;
		hit = 1;
	}
	else if ((strcmp(word, "int64") == 0) || (strcmp(word, "INT64") == 0) || (strcmp(word, "long") == 0) || (strcmp(word, "LONG") == 0)) {
		type[0] = int64_enum;
		hit = 1;
	}
	else if ((strcmp(word, "int128") == 0) || (strcmp(word, "INT128") == 0) || (strcmp(word, "long_long") == 0)) {
		type[0] = int128_enum;
		hit = 1;
	}
	else if ((strcmp(word, "uint8") == 0) || (strcmp(word, "UINT8") == 0)) {
		type[0] = uint8_enum;
		hit = 1;
	}
	else if ((strcmp(word, "uint16") == 0) || (strcmp(word, "UINT16") == 0)) {
		type[0] = uint16_enum;
		hit = 1;
	}
	else if ((strcmp(word, "uint32") == 0) || (strcmp(word, "UINT32") == 0) || (strcmp(word, "uint") == 0) || (strcmp(word, "UINT") == 0)) {
		type[0] = uint32_enum;
		hit = 1;
	}
	else if ((strcmp(word, "uint64") == 0) || (strcmp(word, "UINT64") == 0)) {
		type[0] = uint64_enum;
		hit = 1;
	}
	else if ((strcmp(word, "uint128") == 0) || (strcmp(word, "UINT128") == 0)) {
		type[0] = uint128_enum;
		hit = 1;
	}
	else if ((strcmp(word, "_fp16") == 0) || (strcmp(word, "half") == 0)) {
		type[0] = fp16_enum;
		hit = 1;
	}
	else if ((strcmp(word, "_fp32") == 0) || (strcmp(word, "float") == 0)) {
		type[0] = fp32_enum;
		hit = 1;
	}
	else if ((strcmp(word, "_fp64") == 0) || (strcmp(word, "double") == 0)) {
		type[0] = fp64_enum;
		hit = 1;
	}
	else if ((strcmp(word, "_fp128") == 0) || (strcmp(word, "double_double") == 0)) {
		type[0] = fp128_enum;
		hit = 1;
	}
	else if ((strcmp(word, "void") == 0) || (strcmp(word, "VOID") == 0)) {
		type[0] = void_enum;
		hit = 1;
	}
	return hit;
}

UINT8 RISC_V_csr_decode(control_status_reg_type* csr_reg, char* word) {
	UINT8 hit = 0;
	switch (word[0]) {
	case 'h':
		if (strcmp(word, "hepc") == 0) {
			csr_reg[0] = csr_hepc;
			hit = 1;
		}
		break;
	case 'i':
		if (strcmp(word, "iobase") == 0) {
			csr_reg[0] = csr_iobase;
			hit = 1;
		}
		break;
	case 'm':
		if (strcmp(word, "mhartid") == 0) {
			csr_reg[0] = csr_mhartid;
			hit = 1;
		}
		else if (strcmp(word, "mtval") == 0) {
			csr_reg[0] = csr_mtval;
			hit = 1;
		}
		else if (strcmp(word, "mbound") == 0) {
			csr_reg[0] = csr_mbound;
			hit = 1;
		}
		else if (strcmp(word, "mcause") == 0) {
			csr_reg[0] = csr_mcause;
			hit = 1;
		}
		else if (strcmp(word, "medeleg") == 0) {
			csr_reg[0] = csr_medeleg;
			hit = 1;
		}
		else if (strcmp(word, "mepc") == 0) {
			csr_reg[0] = csr_mepc;
			hit = 1;
		}
		else if (strcmp(word, "mideleg") == 0) {
			csr_reg[0] = csr_mideleg;
			hit = 1;
		}
//		else if (strcmp(word, "msp") == 0) {
//			csr_reg[0] = csr_msp;
//			hit = 1;
//		}
		else if (strcmp(word, "mstatus") == 0) {
			csr_reg[0] = csr_mstatus;
			hit = 1;
		}
		else if (strcmp(word, "mtvec") == 0) {
			csr_reg[0] = csr_mtvec;
			hit = 1;
		}
		break;
	case 's':
		if (strcmp(word, "satp") == 0) {
			csr_reg[0] = csr_satp;
			hit = 1;
		}
		else if (strcmp(word, "stval") == 0) {
			csr_reg[0] = csr_stval;
			hit = 1;
		}
		else if (strcmp(word, "sbound") == 0) {
			csr_reg[0] = csr_sbound;
			hit = 1;
		}
		else if (strcmp(word, "scause") == 0) {
			csr_reg[0] = csr_scause;
			hit = 1;
		}
		else if (strcmp(word, "sscratch") == 0) {
			csr_reg[0] = csr_sscratch;
			hit = 1;
		}
		else if (strcmp(word, "sepc") == 0) {
			csr_reg[0] = csr_sepc;
			hit = 1;
		}
//		else if (strcmp(word, "ssp") == 0) {
//			csr_reg[0] = csr_ssp;
//			hit = 1;
//		}
		else if (strcmp(word, "sstatus") == 0) {
			csr_reg[0] = csr_sstatus;
			hit = 1;
		}
		else if (strcmp(word, "stvec") == 0) {
			csr_reg[0] = csr_stvec;
			hit = 1;
		}
		break;
	case 'u':
		if (strcmp(word, "uepc") == 0) {
			csr_reg[0] = csr_uepc;
			hit = 1;
		}
		break;
	default:
		break;
	}
	return hit;
}
UINT8 get_VariableTypeNameB(VariableListEntry* list_base, parse_struct2* parse, reg_table_struct* reg_table, loop_control* l_control, parse_struct2* parse_out, FILE* Masm) {
	UINT8 result = 0;
	int debug = 0;
	data_type_enum type;
	if (type_decode(&type, parse->word)) {
		VariableListEntry* current = (VariableListEntry*)malloc(sizeof(VariableListEntry));

		while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\t') parse->ptr++;
		if (parse->line[parse->index].line[parse->ptr] == '*') {
			current->pointer = 1;
			current->size = 0;
			parse->ptr++;
		}
		else {
			current->pointer = 0;
		}
		current->atomic = 0;
		current->function = 0;
		current->reg_index_valid = 0;
		current->depth = l_control->depth;
		current->depth_count = l_control->count[l_control->depth];
		current->addr = 0;
		current->sp_offset_valid = 0;

		while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\t') parse->ptr++;
		getWordB(parse);
		sprintf_s(current->name, "%s",parse->word);
		current->type = type;
		while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\t') parse->ptr++;
		if (parse->line[parse->index].line[parse->ptr] == '(' && parse->line[parse->index].line[parse->ptr + 1] == ')') {
			current->function = 1;
			parse->ptr += 2;
		}

		// add to variable list
		current->last = list_base->last;
		current->next = list_base;
		list_base->last->next = current;
		list_base->last = current;
		result = 1;
	}
	return result;
}
UINT8 get_VariableTypeName2B(VariableListEntry* list_base, parse_struct2* parse, UINT8 depth, data_type_enum data_type) {
	UINT8 result = 0;
	VariableListEntry* current = (VariableListEntry*)malloc(sizeof(VariableListEntry));

	if (parse->line[parse->index].line[parse->ptr] == '*') {
		current->pointer = 1;
		current->size = 0;
		parse->ptr++;
	}
	else {
		current->pointer = 0;
	}
	current->atomic = 0;
	current->function = 0;
	current->depth = depth;
	current->addr = 0;

	getWord(parse);
	sprintf_s(current->name, "%s", parse->word);
	current->type = data_type;
	if (parse->line[parse->index].line[parse->ptr] == '(' && parse->line[parse->index].line[parse->ptr + 1] == ')') {
		current->function = 1;
		parse->ptr += 2;
	}

	// add to variable list
	current->last = list_base->last;
	current->next = list_base;
	list_base->last->next = current;
	list_base->last = current;

	result = 1;
	return result;
}

UINT8 parse_FunctionB(FunctionListEntry* f_list_base, parse_struct2* parse, loop_control* l_control, reg_table_struct* reg_table) {
	UINT8 result = 0;
	int debug = 0;
	data_type_enum type;
	if (type_decode(&type, parse->word)) {
		UINT saved_ptr = parse->ptr;
		char saved_word[0x100];
		sprintf_s(saved_word, "%s", parse->word);
		getWordB(parse);
		if (parse->line[parse->index].line[parse->ptr] == '(') {
			FunctionListEntry* current = (FunctionListEntry*)malloc(sizeof(FunctionListEntry));
			current->addr = 0;

			sprintf_s(current->name, "%s", parse->word);
			current->type = type;
			// add to variable list
			current->last = f_list_base->last;
			current->next = f_list_base;
			f_list_base->last->next = current;
			f_list_base->last = current;
			if (parse->line[parse->index].line[parse->ptr] == '(')parse->ptr++;
			for (UINT8 i = 0; i < 32; i++) reg_table->entry_x[i].in_use = 0;
			reg_table->entry_x[10].in_use = 1;
			reg_table->entry_x[11].in_use = 1;
			reg_table->entry_x[12].in_use = 1;
			reg_table->entry_x[13].in_use = 1;
			for (UINT8 i = 0; i < 0x10; i++) current->argument[i].name[0] = '\0';
			current->argc = 0;
			for (UINT8 i = 0; i < 0x10 && parse->line[parse->index].line[parse->ptr] != ')'; i++) {
				if (parse->line[parse->index].line[parse->ptr] == ',')parse->ptr++;
				getWordB(parse);
				if (!type_decode(&current->argument[i].type, parse->word))
					debug++;
				current->argument[i].atomic = 0;
				current->argument[i].pointer = 0;
				if (parse->line[parse->index].line[parse->ptr] == '*') {
					parse->ptr++;
					current->argument[i].pointer = 1;
					current->argument[i].size = 0;
				}
				if (parse->line[parse->index].line[parse->ptr] == ' ')parse->ptr++;
				getWordB(parse);
				sprintf_s(current->argument[i].name, "%s", parse->word);
				current->argument[i].reg_index_valid = 0;
				current->argument[i].depth = 1;
				current->argc++;
			}
			result = 1;
		}
		else {
			parse->ptr = saved_ptr;
			sprintf_s(parse->word, "%s", saved_word);
		}
	}
	return result;
}
UINT8 get_VariableTypeB(parse_struct2* parse, VariableListEntry* list_base) {
	UINT8 result = 0;
	data_type_enum type;
	if (type_decode(&type, parse->word)) {
		VariableListEntry* current = (VariableListEntry*)malloc(sizeof(VariableListEntry));

		if (parse->line[parse->index].line[parse->ptr] == '*') {
			current->pointer = 1;
			current->size = 0;
			parse->ptr++;
		}
		else {
			current->pointer = 0;
		}
		current->atomic = 0;
		current->depth = 0;
		current->addr = 0;
		current->type = type;
		while (parse->line[parse->index].line[parse->ptr] == ' ')parse->ptr++;
		result = (parse->line[parse->index].line[parse->ptr] == ')') ? 1 : -1;
		parse->ptr++;
		while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\t')	parse->ptr++;
	}
	return result;
}
UINT8 get_ControlTypeB(UINT* k, char* word, var_type_struct *control_type) {
	UINT8 result = 0;
	for (k[0] = 0; k[0] < 8 & !result; k[0]++) {
		if (strcmp(word, control_type[k[0]].name) == 0) {
			result = 1;
		}
	}
	k[0]--;
	return result;
}
UINT8 get_FunctionListEntryB(FunctionListEntry** current, char* word, FunctionListEntry* list_base) {
	UINT8 result = 0;
	if (word[0]!='\0') {
		for (current[0] = list_base->next; current[0] != list_base & !result; current[0] = current[0]->next) {
			if (strcmp(word, current[0]->name) == 0) {
				result = 1;
			}
		}
		if (result) {
			current[0] = current[0]->last;
		}
	}
	return result;

}

UINT8 get_VariableListEntryB(VariableListEntry** current, char* word, VariableListEntry* list_base) {
	UINT8 result = 0;
	for (current[0] = list_base->next; current[0] != list_base & !result; current[0] = current[0]->next) {
		if (strcmp(word, current[0]->name) == 0) {
			result = 1;
		}
	}
	if (result) {
		current[0] = current[0]->last;
	}
	return result;
}
UINT8 get_ArgumentListEntry(VariableListEntry** current, char* index, char* word, VariableListEntry* argument, char argc) {
	UINT8 result = 0;
	for (index[0] = 0; index[0] < argc && !result; index[0]++) {
		if (strcmp(word, argument[index[0]].name) == 0) {
			current[0] = &argument[index[0]];
			result = 1;
		}
	}
	index[0]--;
	return result;
}
UINT8 getOperatorB(UINT8* op_match, parse_struct2* parse, operator_type* op_table) {
	UINT8 result = 0;
	char word[0x100];
	UINT8 size = 0;

	while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == 0x09) parse->ptr++;// strip off blank spaces and tabs
	UINT16 ptr = parse->ptr;
	while (parse->line[parse->index].line[ptr] == '>' || parse->line[parse->index].line[ptr] == '<' || parse->line[parse->index].line[ptr] == '=' || parse->line[parse->index].line[ptr] == '!' ||
		parse->line[parse->index].line[ptr] == '*' || parse->line[parse->index].line[ptr] == '+' || parse->line[parse->index].line[ptr] == '-' || parse->line[parse->index].line[ptr] == '&' || parse->line[parse->index].line[ptr] == '|' || parse->line[parse->index].line[ptr] == '^') {
		word[size++] = parse->line[parse->index].line[ptr++];
	}
	word[size++] = '\0';
	for (UINT8 match = 0; match < 0x10; match++) {
		if (strcmp(word, op_table[match].symbol) == 0) {
			op_match[0] = match;
			parse->ptr = ptr;
			while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == 0x09) parse->ptr++;// strip off blank spaces and tabs
			match = 0x10;
			result = 1;
		}
	}
	return result;
}
UINT8 getBranchB(UINT8* op_match, parse_struct2* parse, operator_type* op_table) {
	UINT8 result = 0;
	char word[0x100];
	UINT8 size = 0;

	while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == 0x09) parse->ptr++;// strip off blank spaces and tabs
	UINT8 ptr = parse->ptr;
	while (parse->line[parse->index].line[ptr] == '>' || parse->line[parse->index].line[ptr] == '<' || parse->line[parse->index].line[ptr] == '=' || parse->line[parse->index].line[ptr] == '!') {
		word[size++] = parse->line[parse->index].line[ptr++];
	}
	word[size++] = '\0';
	for (UINT8 match = 0; match < 0x10; match++) {
		if (strcmp(word, op_table[match].symbol) == 0) {
			op_match[0] = match;
			parse->ptr = ptr;
			while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == 0x09) parse->ptr++;// strip off blank spaces and tabs
			match = 0x10;
			result = 1;
		}
	}
	return result;
}

void place_IntrExc_headerB(parse_struct2* parse_out, INT64 base) {
	int offset = 0;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_exception_handler					//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_ssi				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_vsi				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_msi				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_uti				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_sti				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_vti				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_mti				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_uei				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_sei				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_vei				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_mei				//\n", base + offset); offset += 4;
	offset = 16 * 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq0				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq1				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq2				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq3				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq4				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq5				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq6				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq7				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq7				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq9				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq10				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq11				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq12				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq13				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq14				//\n", base + offset); offset += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_irq15				//\n", base + offset); offset += 4;
}


void print_op_immediate2(parse_struct2* parse_out, UINT8 op_match, UINT8 target, UINT8 s1, INT64 number, operator_type* op_table, INT64* PC, reg_table_struct* reg_table, UINT depth, parse_struct2* parse_in, param_type* param, FILE* Masm) {
	int debug = 0;
	if ((strcmp(op_table[op_match].opcode, "sll") == 0) || (strcmp(op_table[op_match].opcode, "srl") == 0)) {
		if (number < 128) {
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %si %s, %s, 0x%03x\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target].name, reg_table->entry_x[s1].name, number); PC[0] += 4;
			if (strcmp(op_table[op_match].opcode, "sll") == 0)
				sprint_op_2src(parse_out, PC,(char*)"slli", reg_table->entry_x[target].name, reg_table->entry_x[s1].name, number, param);
			else
				sprint_op_2src(parse_out, PC,(char*)"srli", reg_table->entry_x[target].name, reg_table->entry_x[s1].name, number, param);
		}
		else if (number < 0x800) {
			UINT8 temp_dest = alloc_temp_UABI(reg_table, depth, parse_out, parse_in, Masm);
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x\n", PC[0], reg_table->entry_x[temp_dest].name, number); PC[0] += 4;
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target].name, reg_table->entry_x[s1].name, reg_table->entry_x[temp_dest].name); PC[0] += 4;
			sprint_op_2src(parse_out, PC, (char*)"addi", reg_table->entry_x[temp_dest].name, reg_table->entry_x[0].name, number, param);
			sprint_op_2src(parse_out, PC, op_table[op_match].opcode, reg_table->entry_x[target].name, reg_table->entry_x[s1].name, reg_table->entry_x[temp_dest].name, param);
			reg_table->entry_x[temp_dest].in_use = 0;
		}
		else {
			debug++;// not coded
		}
	}
	else if (strcmp(op_table[op_match].opcode, "mul") == 0) {
		if (number < 0x800) {
			UINT8 temp_dest = alloc_temp_UABI(reg_table, depth, parse_out, parse_in, Masm);
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x\n", PC[0], reg_table->entry_x[temp_dest].name, number); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x mul %s, %s, %s\n", PC[0], reg_table->entry_x[target].name, reg_table->entry_x[s1].name, reg_table->entry_x[temp_dest].name); PC[0] += 4;
			reg_table->entry_x[temp_dest].in_use = 0;
		}
		else {
			debug++;// not coded
		}
	}
	else {
		if (number < 0x800 && number >= -0x800) {
			if (strcmp(op_table[op_match].opcode, "sub ") == 0) {
	//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %subi %s, %s, %d\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target].name, reg_table->entry_x[s1].name, number); PC[0] += 4;
				sprint_op_2src(parse_out, PC, (char*)"addi", reg_table->entry_x[target].name, reg_table->entry_x[s1].name, -number, param);
			}
			else {
//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%03x\n", PC[0], reg_table->entry_x[target].name, reg_table->entry_x[s1].name, -number); PC[0] += 4;
				char word[0x20];
				sprintf_s(word, "%si", op_table[op_match].opcode);
				sprint_op_2src(parse_out, PC, word, reg_table->entry_x[target].name, reg_table->entry_x[s1].name, number, param);
			}
		}
		else {
			UINT8 hold = alloc_temp_UABI(reg_table, depth, parse_out, parse_in, Masm);
			load_64bB(parse_out, PC, hold, number, (char *) "", reg_table, depth, parse_in, param, Masm);
	//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target].name, reg_table->entry_x[s1].name, reg_table->entry_x[hold].name); PC[0] += 4;
			sprint_addr_p2(parse_out, PC[0], param);
			sprint_op_2src(parse_out, PC, op_table[op_match].opcode, reg_table->entry_x[target].name, reg_table->entry_x[s1].name, reg_table->entry_x[hold].name, param);
			reg_table->entry_x[hold].in_use = 0;
		}
	}
}
void memory_load(parse_struct2* parse_out, VariableListEntry* rd, UINT8 rs1, VariableListEntry* src, INT64 offset, INT64* PC, reg_table_struct* reg_table, parse_struct2* parse_in, UINT depth, param_type *param, FILE* Masm) {
	int debug = 0;
	switch (src->type) {
	case int8_enum: { // byte alignement issue
		UINT8 addr = alloc_temp_UABI(reg_table, depth, parse_out, parse_in, Masm);
		if (offset & 1) {// make sure offset is an even number, work with rs1 for alignment
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, -2 // start sign 8b number load from mem: generate alligned address\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[rs1].name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lh %s, 0x%x(%s) // pull value for \"%s\" off the stack\n", PC[0], reg_table->entry_x[rd->reg_index].name, -(offset + 1), reg_table->entry_x[rs1].name, src->name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 1 // check alignement bit\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[rs1].name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // convert addr alignment bit to bytes moved\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srl %s, %s, %s // align final result\n", PC[0], reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[addr].name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 0x0ff // mask out upper bits\n", PC[0], reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[rd->reg_index].name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 56 // prep sign bit\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[rd->reg_index].name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 56 // signed 8b number loaded from memory\n", PC[0], reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[addr].name); PC[0] += 4;
		}
		else {
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, -2 // start sign 8b number load from mem: align s1\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[rs1].name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lh %s, 0x%x(%s) // pull value for \"%s\" off the stack\n", PC[0], reg_table->entry_x[rd->reg_index].name, -(offset), reg_table->entry_x[addr].name, src->name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 1 // set alignment bit from s1\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[rs1].name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // byte alignment\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name); PC[0] += 4;
			if (param->mxl == 3) {//128b
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 112 // \n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sll %s, %s, %s // result byte wise full left shift\n", PC[0], reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[addr].name); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 120 // aligned result\n", PC[0], reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[rd->reg_index].name); PC[0] += 4;
			}
			else if (param->mxl == 2) {//64b
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 48 // \n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sll %s, %s, %s // result byte wise full left shift\n", PC[0], reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[addr].name); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 56 // aligned result\n", PC[0], reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[rd->reg_index].name); PC[0] += 4;
			}
			else if (param->mxl == 1) {//32b
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 16 // \n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sll %s, %s, %s // result byte wise full left shift\n", PC[0], reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[addr].name); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 24 // aligned result\n", PC[0], reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[rd->reg_index].name); PC[0] += 4;
			}
			else {
		//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0 // \n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sll %s, %s, %s // result byte wise full left shift\n", PC[0], reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[addr].name); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 8 // aligned result\n", PC[0], reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[rd->reg_index].name); PC[0] += 4;
			}
		}
		reg_table->entry_x[addr].in_use = 0;
	}
				  break;
	case uint8_enum: {// byte alignement issue
		UINT8 addr = alloc_temp_UABI(reg_table, depth, parse_out, parse_in, Masm);
		if (offset & 1) {// make sure offset is an even number, work with rs1 for alignment
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, -2 // start unsigned 8b number load from mem: generate alligned address\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[rs1].name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lhu %s, 0x%x(%s) // pull value for \"%s\" off the stack\n", PC[0], reg_table->entry_x[rd->reg_index].name, -(offset + 1), reg_table->entry_x[addr].name, src->name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 1 // check alignement bit\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[rs1].name); PC[0] += 4;
		}
		else {
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, -2 // generate alligned address\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[rs1].name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lhu %s, 0x%x(%s) // pull value for \"%s\" off the stack\n", PC[0], reg_table->entry_x[rd->reg_index].name, -(offset), reg_table->entry_x[addr].name, src->name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 1 // check alignement bit\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[rs1].name); PC[0] += 4;
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, 1 // flip alignement bit\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name); PC[0] += 4;
		}
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // convert addr alignment bit to bytes moved\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name); PC[0] += 4;
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srl %s, %s, %s // align final result\n", PC[0], reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[addr].name); PC[0] += 4;
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 0x0ff // unsigned 8b number loaded from memory: out upper bits\n", PC[0], reg_table->entry_x[rd->reg_index].name, reg_table->entry_x[rd->reg_index].name); PC[0] += 4;
		reg_table->entry_x[addr].in_use = 0;
	}
				   break;
	case int16_enum:
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lh %s, 0x%x(%s) // pull base address for \"%s\" off the stack\n", PC[0], reg_table->entry_x[rd->reg_index].name, -(offset + 1) * 2, reg_table->entry_x[rs1].name, src->name); PC[0] += 4;
		break;
	case int32_enum:
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lw %s, 0x%x(%s) // pull base address for \"%s\" off the stack\n", PC[0], reg_table->entry_x[rd->reg_index].name, -(offset + 1) * 4, reg_table->entry_x[rs1].name, src->name); PC[0] += 4;
		break;
	case int64_enum:
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x%x(%s) // pull base address for \"%s\" off the stack\n", PC[0], reg_table->entry_x[rd->reg_index].name, -(offset + 1) * 8, reg_table->entry_x[rs1].name, src->name); PC[0] += 4;
		break;
	case int128_enum:
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lq %s, 0x%x(%s) // pull base address for \"%s\" off the stack\n", PC[0], reg_table->entry_x[rd->reg_index].name, -(offset + 1) * 16, reg_table->entry_x[rs1].name, src->name); PC[0] += 4;
		break;
	case uint16_enum:
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lhu %s, 0x%x(%s) // pull base address for \"%s\" off the stack\n", PC[0], reg_table->entry_x[rd->reg_index].name, -(offset + 1) * 2, reg_table->entry_x[rs1].name, src->name); PC[0] += 4;
		break;
	case uint32_enum:
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lwu %s, 0x%x(%s) // pull base address for \"%s\" off the stack\n", PC[0], reg_table->entry_x[rd->reg_index].name, -(offset + 1) * 4, reg_table->entry_x[rs1].name, src->name); PC[0] += 4;
		break;
	case uint64_enum:
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ldu %s, 0x%x(%s) // pull base address for \"%s\" off the stack\n", PC[0], reg_table->entry_x[rd->reg_index].name, -(offset + 1) * 8, reg_table->entry_x[rs1].name, src->name); PC[0] += 4;
		break;
	case uint128_enum:
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lqu %s, 0x%x(%s) // pull base address for \"%s\" off the stack\n", PC[0], reg_table->entry_x[rd->reg_index].name, -(offset + 1) * 16, reg_table->entry_x[rs1].name, src->name); PC[0] += 4;
		break;
	case fp16_enum:
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x flh %s, 0x%x(%s) // pull base address for \"%s\" off the stack\n", PC[0], reg_table->entry_fp[rd->reg_index].name, -(offset + 1) * 2, reg_table->entry_x[rs1].name, src->name); PC[0] += 4;
		break;
	case fp32_enum:
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x flw %s, 0x%x(%s) // pull base address for \"%s\" off the stack\n", PC[0], reg_table->entry_fp[rd->reg_index].name, -(offset + 1) * 4, reg_table->entry_x[rs1].name, src->name); PC[0] += 4;
		break;
	case fp64_enum:
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fld %s, 0x%x(%s) // pull base address for \"%s\" off the stack\n", PC[0], reg_table->entry_fp[rd->reg_index].name, -(offset + 1) * 8, reg_table->entry_x[rs1].name, src->name); PC[0] += 4;
		break;
	case fp128_enum:
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x flq %s, 0x%x(%s) // pull base address for \"%s\" off the stack\n", PC[0], reg_table->entry_fp[rd->reg_index].name, -(offset + 1) * 16, reg_table->entry_x[rs1].name, src->name); PC[0] += 4;
		break;
	default:
		debug++;
		break;
	}
}

// Issue: target vs function list type missmatch???
UINT8 check_funct_arg_dest(parse_struct2* parse_out, INT64* number, VariableListEntry* target, parse_struct2* parse_in, INT64* PC, reg_table_struct* reg_table, 
	loop_control* l_control, VariableListEntry* current, compiler_var_type* compiler_var, IO_list_type* IO_list, operator_type* op_table, 
	operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, param_type* param, FILE* Masm, UINT8 unit_debug) {

	UINT debug = 0;
	UINT hit = 0;
	if (parse_in->word[0]!='\0') {
		for (UINT i = 0; i < compiler_var->f_list_base->last->argc; i++) {
			if (strcmp(compiler_var->f_list_base->last->argument[i].name, parse_in->word) == 0 && parse_in->word[0] != '\0') {
				hit = 1;
				number[0] = i;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
					if (compiler_var->f_list_base->last->argument[i].reg_index_valid == 0) {
						compiler_var->f_list_base->last->argument[i].reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						memory_load(parse_out, &compiler_var->f_list_base->last->argument[i], 11, &compiler_var->f_list_base->last->argument[i], i, PC, reg_table, parse_in, l_control->depth, param, Masm);
						compiler_var->f_list_base->last->argument[i].reg_index_valid = 1;
						compiler_var->f_list_base->last->argument[i].reg_depth = l_control->depth;
					}
					if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '(') {
						VariableListEntry offset;
						addVariableEntry(&offset, int64_enum, (char*) "offset", reg_table, l_control, parse_out, parse_in, Masm);
						UINT8 number_valid;
						parse_index_b(parse_out, &offset, &number[0], &number_valid, PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						if (number_valid) {
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']' && ((8 * (number[0] + 1)) < 0x800)) {
								memory_load(parse_out, target, compiler_var->f_list_base->last->argument[i].reg_index, target, number[0], PC, reg_table, parse_in, l_control->depth, param, Masm);
							}
							else
								debug++;
						}
						else {
							switch (compiler_var->f_list_base->last->argument[i].type) {
							case int8_enum:
							case uint8_enum:
								break;
							case int64_enum:
							case uint64_enum:
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // index address * 8 (bytes per entry) for variable \"%s\"\n", PC[0], reg_table->entry_x[offset.reg_index].name, reg_table->entry_x[offset.reg_index].name, compiler_var->f_list_base->last->argument[i].name); PC[0] += 4;
								break;
							default:
								debug++;
								break;
							}
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // base address plus offset for  \"%s\"\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[compiler_var->f_list_base->last->argument[i].reg_index].name, reg_table->entry_x[offset.reg_index].name, compiler_var->f_list_base->last->argument[i].name); PC[0] += 4;
						}
						reg_table->entry_x[offset.reg_index].in_use = 0;
						offset.reg_index_valid = 0;
					}
					else {

						parse_in->ptr++;
						getWord(parse_in);
						if (get_integer(&number[0], parse_in->word)) {
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']' && ((8 * (number[0] + 1)) < 0x800)) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x%x(%s) // pull array element for \"%s\"\n", PC[0], reg_table->entry_x[target->reg_index].name, -((number[0] + 1) * 8), reg_table->entry_x[compiler_var->f_list_base->last->argument[i].reg_index].name, compiler_var->f_list_base->last->argument[i].name); PC[0] += 4;
								memory_load(parse_out, target, compiler_var->f_list_base->last->argument[i].reg_index, target, number[0], PC, reg_table, parse_in, l_control->depth, param, Masm);
							}
							else
								debug++;
						}
					}
				}
				else {
					debug++;
				}
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
					parse_in->ptr++;
					i = 8;
				}
				else {
					debug++;
				}
			}
		}
	}
	return hit;
}
UINT8 check_funct_arg(parse_struct2* parse_out, VariableListEntry* target, UINT address_only, parse_struct2* parse_in, INT64* PC, reg_table_struct* reg_table, loop_control* l_control, 
	VariableListEntry* current, compiler_var_type* compiler_var, IO_list_type* IO_list, operator_type* op_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, 
	param_type* param, FILE* Masm, UINT8 unit_debug) {

	UINT debug = 0;
	UINT hit = 0;
	for (UINT i = 0; i < compiler_var->f_list_base->last->argc; i++) {
		if (strcmp(compiler_var->f_list_base->last->argument[i].name, parse_in->word) == 0) {
			hit = 1;
//			sprintf_s(target->name,"%s", compiler_var->f_list_base->last->argument[i].name);
//			target = &compiler_var->f_list_base->last->argument[i];

			if (target->type == void_enum)
				if (compiler_var->f_list_base->last->argument[i].type == fp16_enum)
					target->reg_fp = 1;
				else
					target->reg_fp = 0;
			if (compiler_var->f_list_base->last->argument[i].pointer) {
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[')
					parse_in->ptr++;
				else
					debug++;
				getWord(parse_in);
				INT64 number;
				if (get_integer(&number, parse_in)) {
					VariableListEntry addr;
					addVariableEntry(&addr, int64_enum, compiler_var->f_list_base->last->argument[i].name, reg_table, l_control, parse_out, parse_in, Masm);
					memory_load(parse_out, &addr, 11, &addr, i, PC, reg_table, parse_in, l_control->depth,param, Masm);
					memory_load(parse_out, target, addr.reg_index, target, number, PC, reg_table, parse_in, l_control->depth, param,Masm);
					reg_table->entry_x[addr.reg_index].in_use = 0;
					addr.reg_index_valid = 0;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
						parse_in->ptr++;
					}
					else {
						parse_index_a(parse_out, target, address_only, PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
					}
				}
				else {
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
						memory_load(parse_out, target, 11, target, i, PC, reg_table, parse_in, l_control->depth,param, Masm);
						parse_in->ptr++;
					}
					else if (compiler_var->f_list_base->last->argument[i].reg_index_valid == 0) {
						VariableListEntry addr;
						addVariableEntry(&addr, int64_enum, compiler_var->f_list_base->last->argument[i].name, reg_table, l_control, parse_out, parse_in, Masm);

						memory_load(parse_out, &addr, 11, &addr, i, PC, reg_table, parse_in, l_control->depth, param, Masm);//addr = -i(a1)
						parse_index_a(parse_out, target, address_only, PC, &addr, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
				//		if (address_only) {
							reg_table->entry_x[addr.reg_index].in_use = 0;
							addr.reg_index_valid = 0;
				//		}
					}
					else {
						parse_index_a(parse_out, target, address_only, PC, &compiler_var->f_list_base->last->argument[i], parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
					}
				}
			}
			else {
				if (address_only == 0) {
					VariableListEntry header;
					header.type = int64_enum;
					sprintf_s(header.name, "%s", parse_in->word);
					if (target->reg_index_valid != 1) {
						switch (target->type) {
						case uint16_enum:
							target->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							break;
						default:
							debug++;
							break;
						}
						target->reg_index_valid = 1;
					}
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -0x%03x(a1) // pull value of \"%s\" off the arg stack\n", 
						PC[0], reg_table->entry_x[target->reg_index].name, (i+1)<<3, parse_in->word); PC[0] += 4;
				}
			}
		}
	}
	return hit;
}
UINT8 find_loop_reg_index(loop_control* l_control, char* word) {
	UINT8 hit = 0;
	l_control->index = -1;
	for (UINT8 i = l_control->depth; i > 0 && !hit; i--) {
		if (l_control->for_loop[i].detected && strcmp(l_control->for_loop[i].index_name, word) == 0) {
			hit = 1;
			l_control->index = i;
		}
	}
	return hit;
}
void parse_subset_setup(parse_struct2* parse_out, VariableListEntry* target, UINT8 s1, VariableListEntry* s2, INT8* subset_depth, parse_struct2* parse_in, UINT8 op_match, 
	compiler_var_type* compiler_var, IO_list_type* IO_list, operator_type* op_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, INT64* PC,
	reg_table_struct* reg_table, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug) {

	int debug = 0;
	if (target->type == fp16_enum) {
		UINT ptr_hold = parse_in->ptr;
		parse_in->ptr++;
		getWord(parse_in);
		VariableListEntry* test = (VariableListEntry*)malloc(sizeof(VariableListEntry));
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && type_decode(&test->type, parse_in->word)) {
			parse_in->ptr++;
			getWord(parse_in);
			char index;
			if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
				if (test->reg_index_valid == 0) {
					test->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, PC, test->reg_index, test->addr, test->name, reg_table, l_control->depth, parse_in, param, Masm);// error, dropped data
					test->reg_index_valid = 1;
				}
				cast_variable(parse_out, parse_in, test, s2, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param,Masm, unit_debug);
			}
			else if (get_ArgumentListEntry(&test, &index, parse_in->word, compiler_var->f_list_base->last->argument, compiler_var->f_list_base->last->argc)) {
				cast_variable(parse_out, parse_in, test, s2, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param,Masm, unit_debug);
			}
			else {
				debug++;
			}
		}
		else {
			parse_in->ptr = ptr_hold;
			parse_subset(parse_out, s2, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param,Masm, unit_debug);
		}
	}
	else {
		parse_subset(parse_out, s2, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control,param, Masm, unit_debug);
	}
}
void parse_s2B(parse_struct2* parse_out, VariableListEntry* target, UINT8 s1, INT8* subset_depth, parse_struct2* parse_in, compiler_var_type* compiler_var, IO_list_type* IO_list, operator_type* op_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, INT64* PC, reg_table_struct* reg_table, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug) {
	int debug = 0;
	UINT8 branch_match;
	UINT8 op_match;
	if (getOperatorB(&op_match, parse_in, op_table)) {// need to add conditinal statement check here, response is true, false (1,0)
		getWord(parse_in);
		INT64 number;
		fp_data_struct fp_data;
		VariableListEntry* current = compiler_var->list_base->last;
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
			UINT8 ptr_hold = parse_in->ptr;
			parse_in->ptr++;
			char word[0x80];
			sprintf_s(word, "%s", parse_in->word);
			getWord(parse_in);
			if (strcmp(parse_in->word, "_fp16") == 0) {
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
					parse_in->ptr++;
				else
					debug++;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				getWord(parse_in);
				VariableListEntry* s2 = compiler_var->list_base;
				if (get_VariableListEntryB(&s2, parse_in->word, compiler_var->list_base)) {
					cast_variable(parse_out, parse_in, s2, current, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
					if (target->type == fp16_enum) {
			//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[current->reg_index].name); PC[0] += 4;
						sprintf(word, "f%s", op_table[op_match].opcode);
						sprint_op_2src(parse_out, PC, word, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[current->reg_index].name, param);
					}
					else {
			//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1].name, reg_table->entry_x[current->reg_index].name); PC[0] += 4;
						sprint_op_2src(parse_out, PC, op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1].name, reg_table->entry_x[current->reg_index].name, param);
					}
				}
				else {
					debug++;
				}
			}
			else {
				parse_in->ptr = ptr_hold;
				sprintf_s(parse_in->word,"%s", word);
				VariableListEntry s2;
				addVariableEntry(&s2, target->type, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
				parse_subset_setup(parse_out, target, s1, &s2, subset_depth, parse_in, op_match, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
				if (target->type == fp16_enum) {
				//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[s2.reg_index].name); PC[0] += 4;
					sprintf(word, "f%s", op_table[op_match].opcode);
					sprint_op_2src(parse_out, PC, word, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[s2.reg_index].name, param);
					reg_table->entry_fp[s2.reg_index].in_use = 0;
				}
				else {
	//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1].name, reg_table->entry_x[s2.reg_index].name); PC[0] += 4;
					sprint_op_2src(parse_out, PC, op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1].name, reg_table->entry_x[s2.reg_index].name, param);
					reg_table->entry_x[s2.reg_index].in_use = 0;
				}
				s2.reg_index_valid = 0;
			}
		}
		else if (find_loop_reg_index(l_control, parse_in->word)) {
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // rd = rs1 %s %s\n", PC[0],
				op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1].name, reg_table->entry_x[l_control->for_loop[l_control->index].index_reg].name,
				op_table[op_match].symbol, l_control->for_loop[l_control->index].index_name); PC[0] += 4;
		}
		else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
			if (current->pointer && parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
				VariableListEntry hold_temp;
				addVariableEntry(&hold_temp, target->type, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
				parse_index_a(parse_out, &hold_temp, 0, PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
				if (target->type == fp16_enum) {
					if (target->reg_index == s1) {
						UINT8 hold = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[hold].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[hold_temp.reg_index].name); PC[0] += 4;
						reg_table->entry_fp[target->reg_index].in_use = 0;
						target->reg_index = hold;
					}
					else {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[hold_temp.reg_index].name); PC[0] += 4;
					}
				}
				else {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1].name, reg_table->entry_x[hold_temp.reg_index].name); PC[0] += 4;
				}
				if (target->type == fp16_enum)
					reg_table->entry_fp[hold_temp.reg_index].in_use = 0;
				else
					reg_table->entry_x[hold_temp.reg_index].in_use = 0;
				hold_temp.reg_index_valid = 0;
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '<' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '<') {
				parse_in->ptr += 2;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				getWord(parse_in);
				if (get_integer(&number, parse_in->word)) {
					UINT8 hold_temp = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					if (current->reg_index_valid != 1) {
						current->reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, current->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
					}
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, 0x%x\n", PC[0], reg_table->entry_x[hold_temp].name, reg_table->entry_x[current->reg_index].name, number); PC[0] += 4;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1].name, reg_table->entry_x[hold_temp].name); PC[0] += 4;
					if (current->reg_index_valid != 1) {
						reg_table->entry_x[current->reg_index].in_use = 0;
						current->reg_index_valid = 0;
					}
					reg_table->entry_x[hold_temp].in_use = 0;
				}
				else {
					debug++; // not coded
				}
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' || parse_in->line[parse_in->index].line[parse_in->ptr] == '-') {
					parse_s2B(parse_out, target, target->reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
				}
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '>' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '>') {
				parse_in->ptr += 2;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				getWord(parse_in);
				if (get_integer(&number, parse_in->word)) {
					UINT8 hold_temp = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					if (current->reg_index_valid != 1) {
						current->reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, current->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
					}
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, 0x%x\n", PC[0], reg_table->entry_x[hold_temp].name, reg_table->entry_x[current->reg_index].name, number); PC[0] += 4;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1].name, reg_table->entry_x[hold_temp].name); PC[0] += 4;
					if (current->reg_index_valid != 1) {
						reg_table->entry_x[current->reg_index].in_use = 0;
						current->reg_index_valid = 0;
					}
					reg_table->entry_x[hold_temp].in_use = 0;
				}
				else {
					debug++; // not coded
				}
			}
			else {
				if (current->reg_index_valid != 1) {
					current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					current->reg_index_valid = 1;
					current->reg_depth = l_control->depth;
					load_64bB(parse_out, PC, current->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
				}
				if (target->type == fp16_enum) {
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*' && (op_match == 0 || op_match == 1)) {
						parse_in->ptr++;
						VariableListEntry s2;
						addVariableEntry(&s2, target->type, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
						check_if_cast(parse_out, parse_in, &s2, subset_depth, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
						debug++;
						if (target->reg_index == s1) {
							UINT8 hold_temp = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
							reg_table->entry_fp[target->reg_index].in_use = 0;
							target->reg_index = hold_temp;
						}
						if (op_match == 0) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s, %s, %s\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[s2.reg_index].name, reg_table->entry_fp[s1].name); PC[0] += 4;
						}
						else {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s, %s, %s\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[s2.reg_index].name, reg_table->entry_fp[s1].name); PC[0] += 4;
						}
					}
					else {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[current->reg_index].name); PC[0] += 4;
					}
				}
				else {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1].name, reg_table->entry_x[current->reg_index].name); PC[0] += 4;
				}
			}
		}
		else if (get_integer(&number, parse_in->word)) {
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*' && (op_match == 0 || op_match == 1)) {// need to expand to include fnmadd and fnmsub
				parse_in->ptr++;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
					VariableListEntry s2;
					addVariableEntry(&s2, target->type, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
					parse_subset_setup(parse_out, target, target->reg_index, &s2, subset_depth, parse_in, op_match, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
					UINT8 temp_i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, PC, temp_i, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
					if (target->type == fp16_enum) {
						UINT8 temp_fp = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.x.h %s, %s // int to float \n", PC[0], reg_table->entry_fp[temp_fp].name, reg_table->entry_x[temp_i].name); PC[0] += 4;
						if (op_match == 0) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s, %s, %s// target = target +(num*RightHandSIde) \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[temp_fp].name, reg_table->entry_fp[s2.reg_index].name, reg_table->entry_fp[target->reg_index].name); PC[0] += 4;
						}
						else {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s, %s, %s// target = target -(num*RightHandSIde) \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[temp_fp].name, reg_table->entry_fp[s2.reg_index].name, reg_table->entry_fp[target->reg_index].name); PC[0] += 4;
						}
						reg_table->entry_fp[s2.reg_index].in_use = 0;
						reg_table->entry_fp[temp_fp].in_use = 0;
					}
					else {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x mul %s, %s, %s // target = target -(num*RightHandSIde) \n", PC[0], reg_table->entry_x[s2.reg_index].name, reg_table->entry_x[temp_i].name, reg_table->entry_x[s2.reg_index].name); PC[0] += 4;
						if (op_match == 0) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s // target = target +(num*RightHandSIde) \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s2.reg_index].name); PC[0] += 4;
						}
						else {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // target = target -(num*RightHandSIde) \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s2.reg_index].name); PC[0] += 4;
						}
						reg_table->entry_x[s2.reg_index].in_use = 0;
					}
					reg_table->entry_x[temp_i].in_use = 0;
				}
				else {
					UINT8 num_valid = 0;
					INT64 number2;
					VariableListEntry data;
					addVariableEntry(&data, target->type, (char*) "data", reg_table, l_control, parse_out, parse_in, Masm);

					parse_rh_of_eqB(parse_out, &data, 0, &num_valid, &number2, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
					UINT8 temp_i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, PC, temp_i, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
					if (target->type == fp16_enum) {
						UINT8 temp_fp = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.x.h %s, %s // int to float \n", PC[0], reg_table->entry_fp[temp_fp].name, reg_table->entry_x[temp_i].name); PC[0] += 4;
						if (op_match == 0) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s, %s, %s// target = target +(num*RightHandSIde) \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[temp_fp].name, reg_table->entry_fp[data.reg_index].name, reg_table->entry_fp[target->reg_index].name); PC[0] += 4;
						}
						else {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s, %s, %s// target = target -(num*RightHandSIde) \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[temp_fp].name, reg_table->entry_fp[data.reg_index].name, reg_table->entry_fp[target->reg_index].name); PC[0] += 4;
						}
						reg_table->entry_fp[data.reg_index].in_use = 0;
						reg_table->entry_fp[temp_fp].in_use = 0;
					}
					else {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x mul %s, %s, %s // target = target -(num*RightHandSIde) \n", PC[0], reg_table->entry_x[data.reg_index].name, reg_table->entry_x[temp_i].name, reg_table->entry_x[data.reg_index].name); PC[0] += 4;
						if (op_match == 0) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s // target = target +(num*RightHandSIde) \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[data.reg_index].name); PC[0] += 4;
						}
						else {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // target = target -(num*RightHandSIde) \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[data.reg_index].name); PC[0] += 4;
						}
						reg_table->entry_x[data.reg_index].in_use = 0;
					}
					reg_table->entry_x[temp_i].in_use = 0;
				}
			}
			else {
				print_op_immediate2(parse_out, op_match, target->reg_index, s1, number, op_table, PC, reg_table, l_control->depth, parse_in,param, Masm);
			}
		}
		else if (get_float(&fp_data, parse_in->word)) {
			UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
			UINT index0 = (fp_data.hp & 0x0fff);
			UINT64 carry = (fp_data.hp & 0x800) << 1;
			UINT index1 = (((fp_data.hp + carry) >> 12) & 0x0fffff);
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // start 16b load\n", PC[0], reg_table->entry_x[s2i].name, index1); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %#05x	// completed load of 0x%08x as 16b immediate load\n", PC[0], reg_table->entry_x[s2i].name, reg_table->entry_x[s2i].name, index0); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[s2].name, reg_table->entry_x[s2i].name); PC[0] += 4;

			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' || parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
				subset_depth[0]--;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[s2].name); PC[0] += 4;
			}
			else {
				debug++;
			}
			reg_table->entry_x[s2i].in_use = 0;
			reg_table->entry_fp[s2].in_use = 0;
		}
		else {
			VariableListEntry temp;
			addVariableEntry(&temp, int64_enum, (char*)"", reg_table, l_control, parse_out, parse_in, Masm);
			if (check_funct_arg(parse_out, &temp, 0, parse_in, PC, reg_table, l_control, current, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, param, Masm, unit_debug)) {
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[temp.reg_index].name); PC[0] += 4;
			}
			else {
				debug++;
			}
			reg_table->entry_x[temp.reg_index].in_use = 0;
			temp.reg_index_valid = 0;
		}
	}
	else if (getBranchB(&branch_match, parse_in, branch_table)) {// need to add conditinal statement check here, response is true, false (1,0)
		getWord(parse_in);
		INT64 num;
		VariableListEntry s2;
		addVariableEntry(&s2, int64_enum, (char*)"", reg_table, l_control, parse_out, parse_in, Masm);
		if (get_integer(&num, parse_in->word)) {
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, %#05x\n", PC[0], reg_table->entry_x[s2.reg_index].name, num);
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, %#05x\n", PC[0], reg_table->entry_x[s2.reg_index].name, num); PC[0] += 4;
		}
		else {
			debug++;
			exit(0);
		}
		UINT8 dst = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
		UINT8 pred = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
		if (strcmp(branch_table[branch_match].symbol, "<") == 0) {
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // conditional statement, true/false response ensues \n", PC[0], reg_table->entry_x[dst].name, reg_table->entry_x[s1].name, reg_table->entry_x[s2.reg_index].name);
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 63  // all 1's if neg \n", PC[0], reg_table->entry_x[pred].name, reg_table->entry_x[dst].name);
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // conditional statement, true/false response ensues \n", PC[0], reg_table->entry_x[dst].name, reg_table->entry_x[s1].name, reg_table->entry_x[s2.reg_index].name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 63  // all 1's if neg \n", PC[0], reg_table->entry_x[pred].name, reg_table->entry_x[dst].name); PC[0] += 4;
		}
		else if (strcmp(branch_table[branch_match].symbol, ">") == 0) {
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // conditional statement, true/false response ensues \n", PC[0], reg_table->entry_x[dst].name, reg_table->entry_x[s2.reg_index].name, reg_table->entry_x[s1].name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 63  // all 1's if neg \n", PC[0], reg_table->entry_x[pred].name, reg_table->entry_x[dst].name); PC[0] += 4;
		}
		else {
			debug++;
		}
		reg_table->entry_x[dst].in_use = 0;
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
			subset_depth[0]--;
			parse_in->ptr++;
		}
		else
			debug++;
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '?') {
			parse_in->ptr++;
			getWord(parse_in);
			if (get_integer(&num, parse_in->word)) {
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ori %s, %s, %#05x	// (?) if true result \n", PC[0], reg_table->entry_x[s1].name, reg_table->entry_x[pred].name, num); PC[0] += 4;
			}
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ':') {
				parse_in->ptr++;
				UINT8 minus_one = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, zero, 1\n", PC[0], reg_table->entry_x[minus_one].name); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xor %s, %s, %s // (?) predicate for if false \n", PC[0], reg_table->entry_x[pred].name, reg_table->entry_x[pred].name, reg_table->entry_x[minus_one].name); PC[0] += 4;
				reg_table->entry_x[minus_one].in_use = 0;
				VariableListEntry* current = compiler_var->list_base->next;
				getWord(parse_in);
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
					check_if_cast(parse_out, parse_in, &s2, subset_depth, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // (?) if false response \n", PC[0], reg_table->entry_x[s2.reg_index].name, reg_table->entry_x[pred].name, reg_table->entry_x[s2.reg_index].name); PC[0] += 4;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // (?) merge true/false response, one should be equal to zero, or error \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1].name, reg_table->entry_x[s2.reg_index].name); PC[0] += 4;
					reg_table->entry_x[pred].in_use = 0;
					reg_table->entry_x[s1].in_use = 0;
					reg_table->entry_x[s2.reg_index].in_use = 0;
					s2.reg_index_valid = 0;
				}
				else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
					if (current->pointer && parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
						VariableListEntry hold_temp;
						addVariableEntry(&hold_temp, int64_enum, (char*)"", reg_table, l_control, parse_out, parse_in, Masm);

						parse_index_a(parse_out, &hold_temp, 0, PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // (?) if false response \n", PC[0], reg_table->entry_x[s2.reg_index].name, reg_table->entry_x[pred].name, reg_table->entry_x[hold_temp.reg_index].name); PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // (?) merge true/false response, one should be equal to zero, or error \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1].name, reg_table->entry_x[s2.reg_index].name); PC[0] += 4;
						reg_table->entry_x[hold_temp.reg_index].in_use = 0;
						reg_table->entry_x[pred].in_use = 0;
						reg_table->entry_x[s1].in_use = 0;
						reg_table->entry_x[s2.reg_index].in_use = 0;
						hold_temp.reg_index_valid = 0;
						s2.reg_index_valid = 0;
					}
					else {
						debug++;
					}
				}
				else {
					debug++;
				}
			}
			else {
				debug++; // syntax error
			}
		}
		else {
			debug++;
		}
		reg_table->entry_x[s2.reg_index].in_use = 0;
	}
	else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {// ignore, no error
		subset_depth[0]--;
	}
	else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
		VariableListEntry s1;
		addVariableEntry(&s1, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
		check_if_cast(parse_out, parse_in, &s1, subset_depth, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
	}
	else {
		debug++;
		exit(0);
	}
}

void parse_index_a(parse_struct2* parse_out, VariableListEntry* target, UINT8 address_only, INT64* PC, VariableListEntry* current, parse_struct2* parse, compiler_var_type* compiler_var, IO_list_type* IO_list,
	operator_type* op_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, reg_table_struct* reg_table, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug) {
	int debug = 0;
	if (!current->pointer) {
		debug++;
	}
	if (parse->line[parse->index].line[parse->ptr] == '[') parse->ptr++;
	while (parse->line[parse->index].line[parse->ptr] != ']') {
		if (parse->line[parse->index].line[parse->ptr] == '(') {
			INT8 subset_depth = 0;
			VariableListEntry addr;
			addVariableEntry(&addr, int64_enum, (char *) "", reg_table, l_control, parse_out, parse, Masm);
			check_if_cast(parse_out, parse, &addr, &subset_depth, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
			while (parse->line[parse->index].line[parse->ptr] != ']') {
				parse_s2B(parse_out, &addr, addr.reg_index, &subset_depth, parse, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control,param, Masm, unit_debug);
			}
			if (subset_depth != 0)
				debug++;
			if (current->atomic == 1) {
	//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lr.d.aq.rl %s, 0(%s)\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[addr.reg_index].name); PC[0] += 4;//current->name,
				sprint_load(parse_out, PC, (char*)"lr.d.aq.rl", reg_table->entry_x[target->reg_index].name, (short) 0,reg_table->entry_x[addr.reg_index].name,(char*)"",param);
			}
			else {
				if (current->reg_index_valid == 0) {
					current->reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
					load_64bB(parse_out, PC, current->reg_index, current->addr, current->name, reg_table, l_control->depth, parse, param, Masm);
					current->reg_index_valid = 1;
				}
				if (address_only) {
					switch (current->type) {
					case int16_enum:
					case uint16_enum:
					case fp16_enum:
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 1 // adjust to 16b (2B per entry)\n", PC[0], reg_table->entry_x[addr.reg_index].name, reg_table->entry_x[addr.reg_index].name); PC[0] += 4;//current->name,
						break;
					case int32_enum:
					case uint32_enum:
					case fp32_enum:
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 2 // adjust to 32b (4B per entry)\n", PC[0], reg_table->entry_x[addr.reg_index].name, reg_table->entry_x[addr.reg_index].name); PC[0] += 4;//current->name,
						break;
					case int64_enum:
					case uint64_enum:
					case fp64_enum:
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // adjust to 64b (8B per entry)\n", PC[0], reg_table->entry_x[addr.reg_index].name, reg_table->entry_x[addr.reg_index].name); PC[0] += 4;//current->name,
						break;
					default:
						debug++;
						break;
					}
					if (target->reg_index_valid == 0) {
						target->reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
	//					target->reg_index_valid = 1;
					}
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // add index to base address (vector)\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[addr.reg_index].name); PC[0] += 4;//current->name,
				}
				else {
					if (target->type == fp16_enum) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 1 // adjust index to fp16 size\n", PC[0], reg_table->entry_x[addr.reg_index].name, reg_table->entry_x[addr.reg_index].name); PC[0] += 4;//current->name,
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // add index to base address (vector)\n", PC[0], reg_table->entry_x[addr.reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[addr.reg_index].name); PC[0] += 4;//current->name,
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fld %s, 0(%s) // load array element\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_x[addr.reg_index].name); PC[0] += 4;//current->name,
					}
					else if (target->type == int8_enum || target->type == uint8_enum) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // add index to base address (vector)\n", PC[0], reg_table->entry_x[addr.reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[addr.reg_index].name); PC[0] += 4;//current->name,
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s) // load array element\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[addr.reg_index].name); PC[0] += 4;//current->name,
					}
					else {
						debug++;
					}
				}
			}
			reg_table->entry_x[addr.reg_index].in_use = 0;
			addr.reg_index_valid = 0;
		}
		else if (parse->line[parse->index].line[parse->ptr] == '-') {
			parse->ptr++;
			getWordB(parse);
			INT64 number;
			if (get_integer(&number, parse->word)) {
				if (parse->line[parse->index].line[parse->ptr] == ']') {
					UINT8 offset;
					if (current->reg_index_valid) {
						offset = current->reg_index;
					}
					else {
						offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
						load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse, param, Masm);
					}
					if (current->atomic) {// should mark as error I think, need to verify
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lr.d.aq.rl %s, %d(%s)\n", PC[0], reg_table->entry_x[target->reg_index].name, -number, reg_table->entry_x[offset].name); PC[0] += 4;//current->name,
					}
					else {
						if (number < 0x800) {
							if (current->type == fp16_enum) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x flh %s, %#x(%s)// load \"%s\", assumes _fp16, stack order \n", PC[0], reg_table->entry_fp[target->reg_index].name, (number - 1) * 2, reg_table->entry_x[offset].name, current->name); PC[0] += 4;
							}
							else {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, %#x(%s)// load \"%s\", assumes entries are 64b, stack order \n", PC[0], reg_table->entry_x[target->reg_index].name, (number - 1) * 8, reg_table->entry_x[offset].name, current->name); PC[0] += 4;
							}
						}
						else {
							UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
							load_64bB(parse_out, PC, offset, number, (char *) "", reg_table, l_control->depth, parse, param, Masm);
							UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s	// array base + offset \n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;//current->name,
							if (target->type == fp16_enum) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x flh %s, 0(%s)	//  load \"%s\",assumes entries are 64b, stack order\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_x[addr].name, current->name); PC[0] += 4;//current->name,
							}
							else {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s)	//  load \"%s\",assumes entries are 64b, stack order\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[addr].name, current->name); PC[0] += 4;//current->name,
							}
							reg_table->entry_x[offset].in_use = 0;
							reg_table->entry_x[addr].in_use = 0;
						}
					}
					if (!current->reg_index_valid)
						reg_table->entry_x[offset].in_use = 0;
				}
				else {
					INT8 subset_depth = 0;
					parse_s2B(parse_out, target, target->reg_index, &subset_depth, parse, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
					if (subset_depth != 0)
						debug++;
				}
			}
			else {
				debug++;
			}
		}
		else {
			getWordB(parse);
			INT64 number;
			UINT8 index;
			UINT8 op_match;
			VariableListEntry* current2 = compiler_var->list_base->next;
			if (find_loop_reg_index(l_control, parse->word)) {
				UINT8 temp = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
				if (parse->line[parse->index].line[parse->ptr] == ']') {
					//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, zero\n", PC[0],reg_table->entry_x[target->reg_index].name, reg_table->entry_x[l_control->for_loop[l_control->index].index_reg].name); PC[0] += 4;
					if (current->type == uint128_enum) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 4\n", PC[0], reg_table->entry_x[temp].name, reg_table->entry_x[l_control->for_loop[l_control->index].index_reg].name); PC[0] += 4;
					}
					else {
						debug++;
					}
				}
				else {
					UINT8 op_match;
					INT64 number;
					if (getOperatorB(&op_match, parse, op_table)) {
						getWordB(parse);
						if (get_integer(&number, parse->word)) {
							if (parse->line[parse->index].line[parse->ptr] == ']' &&
								(strcmp(op_table[op_match].opcode, "sll") == 0) || (strcmp(op_table[op_match].opcode, "srl") == 0)) {
								if (current->type == uint128_enum) {
									UINT s1 = l_control->for_loop[l_control->index].index_reg;
									if (strcmp(op_table[op_match].opcode, "sll") == 0) {
										if (number < (128 - 4)) {
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 0x%03x\n", PC[0], reg_table->entry_x[temp].name, reg_table->entry_x[s1].name, number + 4); PC[0] += 4;
										}
										else if (number < (0x800 - 4)) {
											UINT8 temp_dest = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x\n", PC[0], reg_table->entry_x[temp_dest].name, number + 4); PC[0] += 4;
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sll %s, %s, %s\n", PC[0], reg_table->entry_x[temp].name, reg_table->entry_x[s1].name, reg_table->entry_x[temp_dest].name); PC[0] += 4;
											reg_table->entry_x[temp_dest].in_use = 0;
										}
										else {
											debug++;// not coded
										}
									}
									else {
										if (number < (128 + 4)) {
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 0x%03x\n", PC[0], reg_table->entry_x[temp].name, reg_table->entry_x[s1].name, number - 4); PC[0] += 4;
										}
										else if (number < (0x800 + 4)) {
											UINT8 temp_dest = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x\n", PC[0], reg_table->entry_x[temp_dest].name, number - 4); PC[0] += 4;
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srl %s, %s, %s\n", PC[0], reg_table->entry_x[temp].name, reg_table->entry_x[s1].name, reg_table->entry_x[temp_dest].name); PC[0] += 4;
											reg_table->entry_x[temp_dest].in_use = 0;
										}
										else {
											debug++;// not coded
										}
									}
								}
								else {
									debug++;// not coded yet
								}
							}
							else {
								print_op_immediate2(parse_out, op_match, temp, l_control->for_loop[l_control->index].index_reg, number, op_table, PC, reg_table, l_control->depth, parse,param, Masm);
								if (current->type == uint128_enum) {
									UINT8 temp2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 4\n", PC[0], reg_table->entry_x[temp2].name, reg_table->entry_x[temp].name); PC[0] += 4;
									reg_table->entry_x[temp].in_use = 0;
									temp = temp2;
								}
								else {
									debug++;
								}
							}
						}
						else {
							debug++;
						}
					}
					else {
						debug++;
					}
				}
				UINT8 temp2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s\n", PC[0], reg_table->entry_x[temp2].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[temp].name); PC[0] += 4;
				reg_table->entry_x[temp].in_use = 0;
				temp = temp2;
				if (!address_only) {
					memory_load(parse_out, target, temp, current, 0, PC, reg_table, parse, l_control->depth, param, Masm);
				}
			}
			else if (get_VariableListEntryB(&current2, parse->word, compiler_var->list_base)) {
				UINT8 op_match;
				INT64 number;
				if (getOperatorB(&op_match, parse, op_table)) {
					getWordB(parse);
					if (get_integer(&number, parse->word)) {
						UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
						load_64bB(parse_out, PC, addr, current->addr, current->name, reg_table, l_control->depth, parse, param, Masm);
						if (current2->reg_index_valid) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, reg_table->entry_x[current2->reg_index].name); PC[0] += 4;
						}
						else
							debug++;
						if (parse->line[parse->index].line[parse->ptr] == ']') {
							print_op_immediate2(parse_out, op_match, target->reg_index, addr, -number, op_table, PC, reg_table, l_control->depth, parse,param, Masm);
						}
						else {
							print_op_immediate2(parse_out, op_match, target->reg_index, addr, number, op_table, PC, reg_table, l_control->depth, parse,param, Masm);
						}
						reg_table->entry_x[addr].in_use = 0;
					}
					else {
						debug++;
					}
				}
				else {
					UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
					load_64bB(parse_out, PC, addr, current->addr, current->name, reg_table, l_control->depth, parse, param, Masm);
					if (current2->pointer)
						debug++;
					if (target->type == uint64_enum || target->type == int64_enum) {
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // adjust index to 64b entries\n", PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[current2->reg_index].name); PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[addr].name, reg_table->entry_x[offset].name); PC[0] += 4;
						reg_table->entry_x[offset].in_use = 0;
					}
					else if (target->type == uint8_enum || target->type == int8_enum) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[addr].name, reg_table->entry_x[current2->reg_index].name); PC[0] += 4;
					}
					else {
						debug++;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[addr].name, reg_table->entry_x[current2->reg_index].name); PC[0] += 4;
					}
					reg_table->entry_x[addr].in_use = 0;
					if (parse->line[parse->index].line[parse->ptr] == '+' && parse->line[parse->index].line[parse->ptr + 1] == '+') {
						parse->ptr += 2;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1 // %s++\n", PC[0], reg_table->entry_x[current2->reg_index].name, reg_table->entry_x[current2->reg_index].name, current2->name); PC[0] += 4;
					}
				}
				if (parse->line[parse->index].line[parse->ptr] != ']') {
					if (parse->line[parse->index].line[parse->ptr] == ')') {
						parse->ptr++;
					}
					if (getOperatorB(&op_match, parse, op_table)) {
						getWordB(parse);
						if (get_integer(&number, parse->word)) {
							print_op_immediate2(parse_out, op_match, target->reg_index, target->reg_index, number, op_table, PC, reg_table, l_control->depth, parse, param,Masm);
						}
						else if (get_VariableListEntryB(&current2, parse->word, compiler_var->list_base)) {
							UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
							load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
							reg_table->entry_x[offset].in_use = 0;
						}
						else {
							debug++;
						}
					}
					else {
						debug++;
					}
					if (parse->line[parse->index].line[parse->ptr] == ']') {

						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
						load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name, reg_table->entry_x[target->reg_index].name);
						if (current->atomic) {
//							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lr.d.aq.rl %s, 0(%s)\n", PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[target->reg_index].name); PC[0] += 4;//current->name,
							sprint_load(parse_out, PC, (char*)"lr.d.aq.rl", reg_table->entry_x[offset].name, (short)0, reg_table->entry_x[target->reg_index].name, (char*)"", param);
						}
						else {
						//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s)\n", PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[target->reg_index].name); PC[0] += 4;//current->name,
							sprint_load(parse_out, PC, (char*)"ld", reg_table->entry_x[offset].name, (short)0, reg_table->entry_x[target->reg_index].name, (char*)"", param);
						}
						reg_table->entry_x[target->reg_index].in_use = 0;
						reg_table->entry_x[offset].in_use = 0;
					}
					else {
						debug++;
					}
				}
			}
			else if (get_integer(&number, parse->word)) {
				if (parse->line[parse->index].line[parse->ptr] == ']') {
					UINT8 offset;
					if (current->sp_offset_valid == 1) {
						INT64 index = current->sp_offset_valid;
						if (current->type == fp16_enum) {
							index += 2 * number;
							if (index < 0x800) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x flh %s, %#x(sp)// load \"%s\", assumes _fp16 based data on the stack \n",
									PC[0], reg_table->entry_fp[target->reg_index].name, -(index + 2), current->name); PC[0] += 4;
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
					}
					else {
						if (current->reg_index_valid) {
							offset = current->reg_index;
						}
						else {
							offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
							load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse, param, Masm);
						}
						if (current->atomic) {
				//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lr.d.aq.rl %s, %d(%s)\n", PC[0], reg_table->entry_x[target->reg_index].name, number, reg_table->entry_x[offset].name); PC[0] += 4;//current->name,
							sprint_load(parse_out, PC, (char*)"lr.d.aq.rl", reg_table->entry_x[target->reg_index].name, number, reg_table->entry_x[offset].name, (char*)"", param);
						}
						else {
							if (number < 0x800) {
								switch (current->type) {
								case fp16_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x flh %s, %#x(%s)// load \"%s\", assumes _fp16, stack order \n", PC[0], reg_table->entry_fp[target->reg_index].name, -(number + 1) * 2, reg_table->entry_x[offset].name, current->name); PC[0] += 4;
									break;
								case int64_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, %#x(%s)// load \"%s\", assumes entries are 64b, stack order \n", PC[0], reg_table->entry_x[target->reg_index].name, -(number + 1) * 8, reg_table->entry_x[offset].name, current->name); PC[0] += 4;
									break;
								case uint64_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ldu %s, %#x(%s)// load \"%s\", assumes entries are 64b, stack order \n", PC[0], reg_table->entry_x[target->reg_index].name, -(number + 1) * 8, reg_table->entry_x[offset].name, current->name); PC[0] += 4;
									break;
								case int128_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lq %s, %#x(%s)// load \"%s\", assumes entries are 128b, stack order \n", PC[0], reg_table->entry_x[target->reg_index].name, -(number + 1) * 16, reg_table->entry_x[offset].name, current->name); PC[0] += 4;
									break;
								case uint128_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lqu %s, %#x(%s)// load \"%s\", assumes entries are 128b, stack order \n", PC[0], reg_table->entry_x[target->reg_index].name, -(number + 1) * 16, reg_table->entry_x[offset].name, current->name); PC[0] += 4;
									break;
								default:
									debug++;
									break;
								}
							}
							else {
								UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
								load_64bB(parse_out, PC, offset, number, (char *) "", reg_table, l_control->depth, parse, param, Masm);
								UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s	// array base + offset \n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;//current->name,
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s)	//  load \"%s\",assumes entries are 64b, stack order\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[addr].name, current->name); PC[0] += 4;//current->name,
								reg_table->entry_x[offset].in_use = 0;
								reg_table->entry_x[addr].in_use = 0;
							}
						}
						if (!current->reg_index_valid)
							reg_table->entry_x[offset].in_use = 0;
					}
				}
				else {
					INT8 subset_depth = 0;
					parse_s2B(parse_out, target, target->reg_index, &subset_depth, parse, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
					if (subset_depth != 0)
						debug++;
				}
			}
			else if (get_reg_indexB(&index, parse->word, reg_table)) {
				if (getOperatorB(&op_match, parse, op_table)) {
					getWordB(parse);
					INT64 number;
					VariableListEntry* current2 = compiler_var->list_base->next;
					if (get_integer(&number, parse->word)) {
						print_op_immediate2(parse_out, op_match, target->reg_index, index, number, op_table, PC, reg_table, l_control->depth, parse,param, Masm);
					}
					else if (get_VariableListEntryB(&current2, parse->word, compiler_var->list_base)) {
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
						load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[index].name, reg_table->entry_x[offset].name); PC[0] += 4;
						reg_table->entry_x[offset].in_use = 0;
					}
					else {
						debug++;
					}
				}
			}
			else {
				debug++;
				UINT8 op_match;
				INT64 number;
				if (getOperatorB(&op_match, parse, op_table)) {
					getWordB(parse);
					if (get_integer(&number, parse->word)) {
						if (parse->line[parse->index].line[parse->ptr] == ']') {
							print_op_immediate2(parse_out, op_match, target->reg_index, target->reg_index, number, op_table, PC, reg_table, l_control->depth, parse,param, Masm);
						}
						else {
							print_op_immediate2(parse_out, op_match, target->reg_index, target->reg_index, number, op_table, PC, reg_table, l_control->depth, parse, param,Masm);
						}
					}
					else {
						debug++;
					}
				}
				else {
					debug++;
				}

			}
		}
	}
	if (parse->line[parse->index].line[parse->ptr] == ']') parse->ptr++;
	while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\t') parse->ptr++;
}
void parse_index_b2(parse_struct2* parse_out, UINT8 target, UINT8 s1, parse_struct2* parse, INT64* PC, VariableListEntry* current, compiler_var_type* compiler_var, operator_type* op_table, reg_table_struct* reg_table, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug) {
	int debug = 0;
	UINT8 op_match;
	INT64 number;
	VariableListEntry* current2 = compiler_var->list_base->next;

	UINT pass = 0;
	while (parse->line[parse->index].line[parse->ptr] != ']') {
		if (parse->line[parse->index].line[parse->ptr] == ')') parse->ptr++;
		if (getOperatorB(&op_match, parse, op_table)) {
			getWordB(parse);
			if (get_integer(&number, parse->word)) {
				if (pass == 0)
					print_op_immediate2(parse_out, op_match, target, s1, number, op_table, PC, reg_table, l_control->depth, parse, param,Masm);
				else
					print_op_immediate2(parse_out, op_match, target, target, number, op_table, PC, reg_table, l_control->depth, parse,param, Masm);
			}
			else if (get_VariableListEntryB(&current2, parse->word, compiler_var->list_base)) {
				UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
				load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse, param, Masm);
				if (pass == 0) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target].name, reg_table->entry_x[s1].name, reg_table->entry_x[offset].name); PC[0] += 4;
				}
				else {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target].name, reg_table->entry_x[target].name, reg_table->entry_x[offset].name); PC[0] += 4;
				}
				reg_table->entry_x[offset].in_use = 0;
			}
			else {
				debug++;
			}
		}
		else {
			debug++;
		}
		if (parse->line[parse->index].line[parse->ptr] == ']') {
		}
		else {
			debug++;
		}
		pass++;
	}
}
void parse_index_b(parse_struct2* parse_out, VariableListEntry* target, INT64* number, UINT8* number_valid, INT64* PC, VariableListEntry* current, parse_struct2* parse, compiler_var_type* compiler_var,
	IO_list_type* IO_list, operator_type* op_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, reg_table_struct* reg_table, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug) {
	int debug = 0;
	number_valid[0] = 0;
	fp_data_struct fp_data;
	if (!current->pointer) {
		debug++;
	}
	if (parse->line[parse->index].line[parse->ptr] == '[') parse->ptr++;
	while (parse->line[parse->index].line[parse->ptr] != ']') {
		if (parse->line[parse->index].line[parse->ptr] == '(') {
			INT8 subset_depth = 0;
			check_if_cast(parse_out, parse, target, &subset_depth, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
			while (parse->line[parse->index].line[parse->ptr] != ']') {
				parse_s2B(parse_out, target, target->reg_index, &subset_depth, parse, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
			}
			if (subset_depth != 0)
				debug++;
		}
		else {
			getWordB(parse);
			UINT8 index;
			VariableListEntry* current2 = compiler_var->list_base->next;
			if (find_loop_reg_index(l_control, parse->word)) {
				if (parse->line[parse->index].line[parse->ptr] == ']') {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3		// adjust output for variable size, assume 64b \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[l_control->for_loop[l_control->index].index_reg].name); PC[0] += 4;
				}
				else {
					parse_index_b2(parse_out, target->reg_index, l_control->for_loop[l_control->index].index_reg, parse, PC, current, compiler_var, op_table, reg_table, l_control, param, Masm, unit_debug);
				}
			}
			else if (get_VariableListEntryB(&current2, parse->word, compiler_var->list_base)) {
				INT64 number;
				UINT8 op_match;
				if (current2->reg_index_valid == 0) {
					current2->reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
					load_64bB(parse_out, PC, current2->reg_index, current->addr, current->name, reg_table, l_control->depth, parse, param, Masm);
				}
				if (getOperatorB(&op_match, parse, op_table)) {
					getWordB(parse);
					if (get_integer(&number, parse->word)) {
						if (parse->line[parse->index].line[parse->ptr] == ']') {
							print_op_immediate2(parse_out, op_match, target->reg_index, current2->reg_index, number, op_table, PC, reg_table, l_control->depth, parse, param, Masm);
						}
						else {
							print_op_immediate2(parse_out, op_match, target->reg_index, current2->reg_index, number, op_table, PC, reg_table, l_control->depth, parse,param,  Masm);
						}
					}
					else {
						debug++;
					}
				}
				else {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1		// copy variable to keep from getting corrupted, adjust for stack order\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[current2->reg_index].name); PC[0] += 4;
					if (parse->line[parse->index].line[parse->ptr] == '+' && parse->line[parse->index].line[parse->ptr + 1] == '+') {
						parse->ptr += 2;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1		// %s++ \n", PC[0], reg_table->entry_x[current2->reg_index].name, reg_table->entry_x[current2->reg_index].name, current2->name); PC[0] += 4;
					}
					else if (parse->line[parse->index].line[parse->ptr] == '-' && parse->line[parse->index].line[parse->ptr + 1] == '-') {
						parse->ptr += 2;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, -1		// %s-- \n", PC[0], reg_table->entry_x[current2->reg_index].name, reg_table->entry_x[current2->reg_index].name, current2->name); PC[0] += 4;
					}
				}
				if (current2->reg_index_valid == 0) {
					reg_table->entry_x[current2->reg_index].in_use = 0;
				}
				parse_index_b2(parse_out, target->reg_index, target->reg_index, parse, PC, current, compiler_var, op_table, reg_table, l_control,param, Masm, unit_debug);
			}
			else if (get_integer(number, parse->word)) {
				if (parse->line[parse->index].line[parse->ptr] == ']') {
					number_valid[0] = 1;
				}
				else {
					INT8 subset_depth = 0;

					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x		// load number into reg before further processing \n", PC[0], reg_table->entry_x[target->reg_index].name, number); PC[0] += 4;
					parse_s2B(parse_out, target, target->reg_index, &subset_depth, parse, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
					if (subset_depth != 0)
						debug++;
				}
				if (target->type == fp16_enum)
					debug++;
			}
			else if (get_float(&fp_data, parse->word)) {
				UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
				UINT index0 = (fp_data.hp & 0x0fff);
				UINT64 carry = (fp_data.hp & 0x800) << 1;
				UINT index1 = (((fp_data.hp + carry) >> 12) & 0x0fffff);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // start 16b load\n", PC[0], reg_table->entry_x[s2i].name, index1); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %#05x	// completed load of 0x%08x as 16b immediate load\n", PC[0], reg_table->entry_x[s2i].name, reg_table->entry_x[s2i].name, index0); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_x[s2i].name); PC[0] += 4;
				reg_table->entry_x[s2i].in_use = 0;
				if (target->type != fp16_enum)
					debug++;
			}
			else if (get_reg_indexB(&index, parse->word, reg_table)) {
				UINT8 op_match;
				if (getOperatorB(&op_match, parse, op_table)) {
					getWordB(parse);
					INT64 number;
					VariableListEntry* current2 = compiler_var->list_base->next;
					if (get_integer(&number, parse->word)) {
						print_op_immediate2(parse_out, op_match, target->reg_index, index, number, op_table, PC, reg_table, l_control->depth, parse, param, Masm);
					}
					else if (get_VariableListEntryB(&current2, parse->word, compiler_var->list_base)) {
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse, Masm);
						load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[index].name, reg_table->entry_x[offset].name); PC[0] += 4;
						reg_table->entry_x[offset].in_use;
					}
					else {
						debug++;
					}
				}
			}
			else {
				debug++;
			}
		}
	}
	if (parse->line[parse->index].line[parse->ptr] == ']') parse->ptr++;
	while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\t') parse->ptr++;
}
void parse_rh_of_eqB(parse_struct2* parse_out, VariableListEntry* target, UINT part2, UINT8* num_valid, INT64* number, INT64* PC, parse_struct2* parse_in, compiler_var_type* compiler_var, IO_list_type* IO_list,
	operator_type* op_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, reg_table_struct* reg_table, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug) {
	int debug = 0;

	int bracket_level = 0;

	num_valid[0] = 0;

	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') parse_in->ptr++;
	UINT first_pass = 1;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == 0x09)parse_in->ptr++;
	// check for tyecast first
	UINT8 neg_flag = 0;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '-') {
		parse_in->ptr++;
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '1' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ' ' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == '*') {
			parse_in->ptr += 3;
		}
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == 0x09)parse_in->ptr++;
		neg_flag = 1;
	}
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
		INT8 subset_depth = 0;
		check_if_cast(parse_out, parse_in, target, &subset_depth, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
		if (neg_flag) {
			UINT8 t1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s,-1 \n", PC[0], reg_table->entry_x[t1].name, reg_table->entry_x[target->reg_index].name); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1 \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[t1].name); PC[0] += 4;
			reg_table->entry_x[t1].in_use = 0;
		}
		UINT count = 0;
		while (parse_in->line[parse_in->index].line[parse_in->ptr] != ';' && count < 0x100) {
			parse_s2B(parse_out, target, target->reg_index, &subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
			count++;
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
		}
		if (subset_depth != 0)
			debug++;
	}
	else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '&') {
		if (neg_flag) {
			debug++;
		}
		parse_in->ptr++;
		getWordB(parse_in);
		VariableListEntry* current = compiler_var->list_base->next;
		UINT8 index;
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') { // get function address
			debug++;
		}
		else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
			UINT8 op_match;
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
				VariableListEntry offset;
				addVariableEntry(&offset, int64_enum,(char *) "offset", reg_table, l_control, parse_out, parse_in, Masm);
				INT64 number;
				UINT8 number_valid;
				parse_index_b(parse_out, &offset, &number, &number_valid, PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // index address * 8 (bytes per entry)\n", PC[0], reg_table->entry_x[offset.reg_index].name, reg_table->entry_x[offset.reg_index].name); PC[0] += 4;
				UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				if (current->reg_index_valid) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // base address plus offset \n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset.reg_index].name); PC[0] += 4;
				}
				else {
					load_64bB(parse_out, PC, addr, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // base address plus offset \n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, reg_table->entry_x[offset.reg_index].name); PC[0] += 4;				
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s) \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[addr].name); PC[0] += 4;
				}
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s) \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[addr].name); PC[0] += 4;
				reg_table->entry_x[offset.reg_index].in_use = 0;
				offset.reg_index_valid = 0;
				reg_table->entry_x[addr].in_use = 0;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
					parse_in->ptr++;
				}
				else {
					debug++;
				}
			}
			else if (getOperatorB(&op_match, parse_in, op_table)) {
				debug++;
			}
			else {
				debug++;
			}
		}
		else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->global_list_base)) {
			UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			load_64bB(parse_out, PC, addr, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
				parse_in->ptr++;
				getWordB(parse_in);
				if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
					UINT8 loop = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 \n", PC[0], reg_table->entry_x[loop].name, reg_table->entry_x[offset].name); PC[0] += 4;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %s \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[addr].name, reg_table->entry_x[loop].name); PC[0] += 4;
					reg_table->entry_x[offset].in_use = 0;
					reg_table->entry_x[loop].in_use = 0;
				}
				else {
					debug++;
				}
			}
			else {
				debug++;
			}
			reg_table->entry_x[addr].in_use = 0;
		}
		else if (get_reg_indexB(&index, parse_in->word, reg_table)) {
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
				parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
					VariableListEntry right;
					addVariableEntry(&right, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
					INT8 subset_depth = 0;
					parse_subset(parse_out, &right, &subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
					if (subset_depth != 0)
						debug++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] != ']') {
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
							parse_in->ptr++;
							getWordB(parse_in);
							INT64 number;
							if (get_integer(&number, parse_in->word)) {//need to change to signed value
								UINT8 s1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								UINT8 s2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								load_64bB(parse_out, PC, s1, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1 \n", PC[0], reg_table->entry_x[s1].name, reg_table->entry_x[s1].name); PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s \n", PC[0], reg_table->entry_x[s2].name, reg_table->entry_x[right.reg_index].name, reg_table->entry_x[s1].name); PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 \n", PC[0], reg_table->entry_x[s2].name, reg_table->entry_x[s2].name); PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s \n", PC[0], reg_table->entry_x[s2].name, reg_table->entry_x[index].name, reg_table->entry_x[s2].name); PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s,0(%s) \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s2].name); PC[0] += 4;
								reg_table->entry_x[s2].in_use = 0;
								reg_table->entry_x[s1].in_use = 0;
							}
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
								parse_in->ptr++;
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
					}
					else {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s,0 \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[right.reg_index].name); PC[0] += 4;
					}
					reg_table->entry_x[right.reg_index].in_use = 0;
					right.reg_index_valid = 1;
				}
				else {
					debug++;
				}
			}
			else {
				debug++;
			}
		}
		else if (check_funct_arg(parse_out, target, 1, parse_in, PC, reg_table, l_control, current, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, param, Masm, unit_debug)) {
			// purposely left blank
		}
		else {// get variable address
			debug++;
		}
	}
	else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '~') {
		parse_in->ptr++;
		getWord(parse_in);
		if (get_integer(number, parse_in->word)) {
			if ((number[0] < 0x800) && (number[0] > (-0x800))) {
				UINT8 target_i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x\n", PC[0], reg_table->entry_x[target_i].name, number[0]); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1 // ~%s\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target_i].name, parse_in->word); PC[0] += 4;
				reg_table->entry_x[target_i].in_use = 0;
			}
			else {
				UINT8 target_i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				load_64bB(parse_out, PC, target_i, number[0], (char*) "", reg_table, l_control->depth, parse_in, param, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1 // ~%s\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target_i].name, parse_in->word); PC[0] += 4;
				reg_table->entry_x[target_i].in_use = 0;
			}
		}
		else {
			debug++;
		}
	}
	else {
		if (parse_in->index >= 6805 && unit_debug)
			debug++;
		getWord(parse_in);//rs1
		VariableListEntry* current = compiler_var->list_base->last;
		UINT8 csr_index = 0;
		UINT8 index;
		UINT8 reg_index = 0;
		fp_data_struct fp_data;
		if (find_loop_reg_index(l_control, parse_in->word)) {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
				parse_in->ptr++;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '1') {
					parse_in->ptr++;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1 // %s+1\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[l_control->for_loop[l_control->index].index_reg].name, l_control->for_loop[l_control->index].index_name); PC[0] += 4;
				}
				else {
					debug++;
				}

			}
			else {
				debug++;
			}
			parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
		}
		else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {

			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
				if (neg_flag) {
					VariableListEntry target2;
					addVariableEntry(&target2, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
					parse_index_a(parse_out, &target2, 0, PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
					if (current->type == fp16_enum) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fsub.h %s, zero, %s \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_x[target2.reg_index].name); PC[0] += 4;
					}
					else {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, zero, %s \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target2.reg_index].name); PC[0] += 4;
					}
					reg_table->entry_x[target2.reg_index].in_use = 0;
					target2.reg_index_valid = 0;
				}
				else {
					parse_index_a(parse_out, target, 0, PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
				}
				parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
			}
			else {
				if (current->reg_index_valid) {
					if (neg_flag) {
						if (current->type == fp16_enum) {
							UINT8 s1 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, zero \n", PC[0], reg_table->entry_fp[s1].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fsub.h %s, %s, %s \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[current->reg_index].name); PC[0] += 4;
							reg_table->entry_fp[s1].in_use = 0;
						}
						else {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, zero, %s \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[current->reg_index].name); PC[0] += 4;
						}
						parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param,Masm, unit_debug);
					}
					else {
						if (target->type == void_enum) {
							target->reg_index_valid = 1;
							target->reg_index = current->reg_index;
							if (current->type == fp16_enum || current->type == fp32_enum || current->type == fp64_enum || current->type == fp128_enum) {
								target->reg_fp = 1;
							}
							else {
								target->reg_fp = 0;
							}
							parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
						}
						else if (current->type == void_enum) {
							if (target->reg_index_valid)
								if (target->type == fp16_enum) {
									if (current->reg_fp) {
										reg_table->entry_fp[target->reg_index].in_use = 0;
										target->reg_index = current->reg_index;
										reg_table->entry_fp[target->reg_index].in_use = 1;
									}
									else {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.h.x %s, %s \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_fp[current->reg_index].name); PC[0] += 4;
									}
								}
								else {
									if (current->reg_fp) {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_fp[current->reg_index].name); PC[0] += 4;
									}
									else {
										reg_table->entry_x[target->reg_index].in_use = 0;
										target->reg_index = current->reg_index;
										reg_table->entry_x[target->reg_index].in_use = 1;
									}
								}
							parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
						}
						else if (current->type == fp16_enum) {
							VariableListEntry* current2 = compiler_var->list_base->next;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							switch (parse_in->line[parse_in->index].line[parse_in->ptr]) {
							case '+':
								parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
									parse_in->ptr++;
									getWord(parse_in);
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
									if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
										if (current2->reg_index_valid) {
											if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
												parse_in->ptr++;
												while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
												getWord(parse_in);
												VariableListEntry* current3 = compiler_var->list_base->next;
												if (get_float(&fp_data, parse_in->word)) {
													UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
													UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
													UINT index0 = (fp_data.hp & 0x0fff);
													UINT64 carry = (fp_data.hp & 0x800) << 1;
													UINT index1 = (((fp_data.hp + carry) >> 12) & 0x0fffff);
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // start 16b load\n", PC[0], reg_table->entry_x[s2i].name, index1); PC[0] += 4;
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %#05x	// completed load of 0x%08x as 16b immediate load\n", PC[0], reg_table->entry_x[s2i].name, reg_table->entry_x[s2i].name, index0); PC[0] += 4;
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[s2].name, reg_table->entry_x[s2i].name); PC[0] += 4;
													reg_table->entry_x[s2i].in_use = 0;

													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s, %s, %s\n", PC[0], reg_table->entry_fp[target->reg_index].name,
														reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name, reg_table->entry_fp[s2].name); PC[0] += 4;
													reg_table->entry_fp[s2].in_use = 0;
												}
												else if (get_VariableListEntryB(&current3, parse_in->word, compiler_var->list_base)) {
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s, %s, %s\n", PC[0], reg_table->entry_fp[target->reg_index].name,
														reg_table->entry_fp[current2->reg_index].name, reg_table->entry_fp[current3->reg_index].name, reg_table->entry_fp[current->reg_index].name); PC[0] += 4;
												}
												else {
													debug++;
												}
											}
											else {
												debug++;
											}
										}
										else {
											debug++;
										}
									}
									else {
										debug++;
									}
								}
								else {
									getWord(parse_in);
									INT64 num;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
										parse_in->ptr++;
										if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
										}
										else {
											debug++;
										}
										getWord(parse_in);
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
											VariableListEntry* current3 = compiler_var->list_base->next;
											if (get_VariableListEntryB(&current3, parse_in->word, compiler_var->list_base)) {
												sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s,%s,%s\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current2->reg_index].name, reg_table->entry_fp[current3->reg_index].name, reg_table->entry_fp[current->reg_index].name); PC[0] += 4;
											}
											else {
												debug++;
											}
										}
										else {
											debug++;
										}
									}
									else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
										if (current2->reg_index_valid) {
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fadd.h %s, %s,%s\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name); PC[0] += 4;
										}
										else {
											debug++;
										}
									}
									else if (get_float(&fp_data, parse_in->word)) {
										UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
										UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
										UINT index0 = (fp_data.hp & 0x0fff);
										UINT64 carry = (fp_data.hp & 0x800) << 1;
										UINT index1 = (((fp_data.hp + carry) >> 12) & 0x0fffff);
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // start 16b load\n", PC[0], reg_table->entry_x[s2i].name, index1); PC[0] += 4;
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %#05x	// completed load of 0x%08x as 16b immediate load\n", PC[0], reg_table->entry_x[s2i].name, reg_table->entry_x[s2i].name, index0); PC[0] += 4;
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[s2].name, reg_table->entry_x[s2i].name); PC[0] += 4;
										reg_table->entry_x[s2i].in_use = 0;
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
											parse_in->ptr++;
											while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
											getWord(parse_in);
											if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
												if (current2->reg_index_valid) {
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s,%s\n", PC[0], reg_table->entry_fp[target->reg_index].name,
														reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name, reg_table->entry_fp[s2].name); PC[0] += 4;
												}
												else {
													debug++;
												}
											}
											else {
												debug++;
											}
										}
										else {
											debug++;
										}
										reg_table->entry_fp[s2].in_use = 0;
									}
									else if (get_integer(&num, parse_in)) {
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
											compiler_var->list_base->last->addr = current->addr + num;
											UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
											load_64bB(parse_out, PC, s2i, num, (char *) "pointer offset", reg_table, l_control->depth, parse_in, param, Masm);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s,%s\n", PC[0], reg_table->entry_x[target->reg_index].name,
												reg_table->entry_x[current->reg_index].name, reg_table->entry_x[s2i].name); PC[0] += 4;
											reg_table->entry_x[s2i].in_use = 0;
										}
										else {
											debug++;
										}
									}
									else {
										debug++;
									}
								}
								break;
							case '-':
								parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
									parse_in->ptr++;
									getWord(parse_in);
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
									if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
										if (current2->reg_index_valid) {
											if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
												parse_in->ptr++;
												while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
												getWord(parse_in);
												VariableListEntry* current3 = compiler_var->list_base->next;
												if (get_float(&fp_data, parse_in->word)) {
													UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
													UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
													UINT index0 = (fp_data.hp & 0x0fff);
													UINT64 carry = (fp_data.hp & 0x800) << 1;
													UINT index1 = (((fp_data.hp + carry) >> 12) & 0x0fffff);
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // start 16b load\n", PC[0], reg_table->entry_x[s2i].name, index1); PC[0] += 4;
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %#05x	// completed load of 0x%08x as 16b immediate load\n", PC[0], reg_table->entry_x[s2i].name, reg_table->entry_x[s2i].name, index0); PC[0] += 4;
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[s2].name, reg_table->entry_x[s2i].name); PC[0] += 4;
													reg_table->entry_x[s2i].in_use = 0;

													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s, %s, %s\n", PC[0], reg_table->entry_fp[target->reg_index].name,
														reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name, reg_table->entry_fp[s2].name); PC[0] += 4;
													reg_table->entry_fp[s2].in_use = 0;
												}
												else if (get_VariableListEntryB(&current3, parse_in->word, compiler_var->list_base)) {
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s, %s, %s\n", PC[0], reg_table->entry_fp[target->reg_index].name,
														reg_table->entry_fp[current2->reg_index].name, reg_table->entry_fp[current3->reg_index].name, reg_table->entry_fp[current->reg_index].name); PC[0] += 4;
												}
												else {
													debug++;
												}
											}
											else {
												debug++;
											}
										}
										else {
											debug++;
										}
									}
									else {
										debug++;
									}
								}
								else {
									getWord(parse_in);
									INT64 num;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
										parse_in->ptr++;
										if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
										}
										else {
											debug++;
										}
										getWord(parse_in);
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
											VariableListEntry* current3 = compiler_var->list_base->next;
											if (get_VariableListEntryB(&current3, parse_in->word, compiler_var->list_base)) {
												sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s,%s,%s\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current2->reg_index].name, reg_table->entry_fp[current3->reg_index].name, reg_table->entry_fp[current->reg_index].name); PC[0] += 4;
											}
											else {
												debug++;
											}
										}
										else {
											debug++;
										}
									}
									else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
										if (current2->reg_index_valid) {
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fsub.h %s, %s,%s\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name); PC[0] += 4;
										}
										else {
											debug++;
										}
									}
									else if (get_float(&fp_data, parse_in->word)) {
										UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
										UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
										UINT index0 = (fp_data.hp & 0x0fff);
										UINT64 carry = (fp_data.hp & 0x800) << 1;
										UINT index1 = (((fp_data.hp + carry) >> 12) & 0x0fffff);
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // start 16b load\n", PC[0], reg_table->entry_x[s2i].name, index1); PC[0] += 4;
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %#05x	// completed load of 0x%08x as 16b immediate load\n", PC[0], reg_table->entry_x[s2i].name, reg_table->entry_x[s2i].name, index0); PC[0] += 4;
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[s2].name, reg_table->entry_x[s2i].name); PC[0] += 4;
										reg_table->entry_x[s2i].in_use = 0;
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
											parse_in->ptr++;
											while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
											getWord(parse_in);
											if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
												if (current2->reg_index_valid) {
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s,%s\n", PC[0], reg_table->entry_fp[target->reg_index].name,
														reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name, reg_table->entry_fp[s2].name); PC[0] += 4;
												}
												else {
													debug++;
												}
											}
											else {
												debug++;
											}
										}
										else {
											debug++;
										}
										reg_table->entry_fp[s2].in_use = 0;
									}
									else if (get_integer(&num, parse_in)) {
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
											compiler_var->list_base->last->addr = current->addr + num;
											UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
											load_64bB(parse_out, PC, s2i, num, (char*)"pointer offset", reg_table, l_control->depth, parse_in, param, Masm);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s,%s\n", PC[0], reg_table->entry_x[target->reg_index].name,
												reg_table->entry_x[current->reg_index].name, reg_table->entry_x[s2i].name); PC[0] += 4;
											reg_table->entry_x[s2i].in_use = 0;
										}
										else {
											debug++;
										}
									}
									else {
										debug++;
									}
								}
								break;
							default:
								UINT8 s1 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, zero \n", PC[0], reg_table->entry_fp[s1].name); PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fadd.h %s, %s,%s\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[s1].name); PC[0] += 4;
								reg_table->entry_fp[s1].in_use = 0;
								break;
							}
							parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param,  Masm, unit_debug);
						}
						else {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, zero\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[current->reg_index].name); PC[0] += 4;
								parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
							}
							else {
								parse_rhs_continued(parse_out, target, current, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
							}
						}

					}
				}
				else {
					if (neg_flag) {
						debug++;
					}
					load_64bB(parse_out, PC, target->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
					parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param,  Masm, unit_debug);
				}
			}
		}
		else if (check_funct_arg(parse_out, target, 0, parse_in, PC, reg_table, l_control, current, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, param, Masm, unit_debug)) {
			debug++; // NOTE: must check function arg before global variables
			if (parse_in->line[parse_in->index].line[parse_in->ptr] != ';' && parse_in->line[parse_in->index].line[parse_in->ptr - 1] == ';')
				parse_in->ptr--;
			parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
		}
		else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->global_list_base)) {
			if (neg_flag) {
				debug++;
			}

			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
				parse_in->ptr++;
				UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				load_64bB(parse_out, PC, addr, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
				getWordB(parse_in);
				INT64 number1;
				VariableListEntry* current2;
				if (get_integer(&number1, parse_in->word)) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x%03x(%s)\n", PC[0], reg_table->entry_x[target->reg_index].name, -8 * (number1 + 1), reg_table->entry_x[addr].name); PC[0] += 4;
				}
				else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
					UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3	// %s is an array of 64b entir=es\n", PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[current2->reg_index].name, current->name); PC[0] += 4;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, reg_table->entry_x[offset].name); PC[0] += 4;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -8(%s)\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[addr].name); PC[0] += 4;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '+') {
						parse_in->ptr += 2;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1\n", PC[0], reg_table->entry_x[current2->reg_index].name, reg_table->entry_x[current2->reg_index].name); PC[0] += 4;
					}
					reg_table->entry_x[offset].in_use = 0;
				}
				else {
					debug++;
				}
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
					parse_in->ptr++;
				}
				else {
					debug++;
				}
			}
			else {
				UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s,0 \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
				reg_table->entry_x[offset].in_use = 0;
			}
			parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
		}
		else if (match_listB(&csr_index, parse_in->word, csr_list, csr_list_count, PC)) {
			if (neg_flag) {
				debug++;
			}
			sprint_csr_imm(parse_out, PC,(char*)"csrrci", reg_table->entry_x[target->reg_index].name, 0, csr_list[csr_index].name,param);
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrci %s, 0x00, %s\n", PC[0], reg_table->entry_x[target->reg_index].name, csr_list[csr_index]); PC[0] += 4;//current->name,
			if (parse_in->line[parse_in->index].line[parse_in->ptr] != ';') {
				UINT8 op_match;
				if (getOperatorB(&op_match, parse_in, op_table)) {
					VariableListEntry right;
					right.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					right.reg_index_valid = 1;
					sprintf_s(right.name,"%s", target->name);
					right.type = target->type;
					parse_rh_of_eqB(parse_out, &right, 1, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
					num_valid[0] = 0;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s \n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[right.reg_index].name); PC[0] += 4;
					reg_table->entry_x[right.reg_index].in_use = 0;
				}
			}
			parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param,  Masm, unit_debug);
		}
		else if (get_integer(number, parse_in->word)) {//need to change to signed value
			if (neg_flag) {
				debug++;
			}
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
			if (number[0] == 1 && parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
				parse_in->ptr++;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\(') {
					INT8 subset_depth = 0;
					check_if_cast(parse_out, parse_in, target, &subset_depth, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
					if (neg_flag) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, zero, %s\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name); PC[0] += 4;
					}
				}
				else {
					debug++;
				}
			}
			else {
				if ((number[0] < 0x800) && (number[0] > (-0x800))) {
					if (parse_in->line[parse_in->index].line[parse_in->ptr] != ':' && parse_in->line[parse_in->index].line[parse_in->ptr] != ';' && parse_in->line[parse_in->index].line[parse_in->ptr] != ',') {
						if (target->type == fp16_enum) {
							UINT8 target_i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							if (neg_flag) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x\n", PC[0], reg_table->entry_x[target_i].name, -number[0]); PC[0] += 4;
							}
							else {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x\n", PC[0], reg_table->entry_x[target_i].name, number[0]); PC[0] += 4;
							}
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.x.h %s, %s\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_x[target_i].name); PC[0] += 4;
							reg_table->entry_x[target_i].in_use = 0;
						}
						else {
							if (neg_flag) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x\n", PC[0], reg_table->entry_x[target->reg_index].name, -number[0]); PC[0] += 4;
							}
							else {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x\n", PC[0], reg_table->entry_x[target->reg_index].name, number[0]); PC[0] += 4;
							}
							INT8 subset_depth = 0;
							parse_s2B(parse_out, target, target->reg_index, &subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
							if (subset_depth != 0)
								debug++;
						}
					}
					else {
						num_valid[0] = 1;
						if (neg_flag) {
	//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x\n", PC[0], reg_table->entry_x[target->reg_index].name, -number[0]); PC[0] += 4;
							sprint_op_2src(parse_out, PC, (char*)"addi", reg_table->entry_x[target->reg_index].name, reg_table->entry_x[0].name, -number[0], param);
						}
						else {
//							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x\n", PC[0], reg_table->entry_x[target->reg_index].name, number[0]); PC[0] += 4;
							sprint_op_2src(parse_out, PC, (char*)"addi", reg_table->entry_x[target->reg_index].name, reg_table->entry_x[0].name, number[0], param);
						}
					}
				}
				else {
					load_64bB(parse_out, PC, target->reg_index, number[0], (char*)"", reg_table, l_control->depth, parse_in, param, Masm);
					if (neg_flag) {
					//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, zero, %s\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name); PC[0] += 4;
						sprint_op_2src(parse_out, PC, (char*)"sub ", reg_table->entry_x[target->reg_index].name, reg_table->entry_x[0].name,  reg_table->entry_x[target->reg_index].name, param);
					}
					if (parse_in->line[parse_in->index].line[parse_in->ptr] != ':' && parse_in->line[parse_in->index].line[parse_in->ptr] != ';' && parse_in->line[parse_in->index].line[parse_in->ptr] != ',') {
						INT8 subset_depth = 0;
						parse_s2B(parse_out, target, target->reg_index, &subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
						if (!(subset_depth == 0 || subset_depth == -1))
							debug++;
					}
				}
			}
			parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param,  Masm, unit_debug);
		}
		else if (get_float(&fp_data, parse_in->word)) {
			if (strcmp(parse_in->word, "0.0") == 0) {
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, zero	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[target->reg_index].name); PC[0] += 4;
			}
			else {
				UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				UINT index0 = (fp_data.hp & 0x0fff);
				UINT64 carry = (fp_data.hp & 0x800) << 1;
				UINT index1 = (((fp_data.hp + carry) >> 12) & 0x0fffff);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // start 16b load\n", PC[0], reg_table->entry_x[s2i].name, index1); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %#05x	// completed load of \"%s\" as 16b immediate load\n", PC[0], reg_table->entry_x[s2i].name, reg_table->entry_x[s2i].name, index0, parse_in->word ); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_x[s2i].name); PC[0] += 4;
				reg_table->entry_x[s2i].in_use = 0;
				if (target->type != fp16_enum)
					debug++; // syntax error
			}
			parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
		}
		else if (get_reg_indexB(&index, parse_in->word, reg_table)) {
			if (neg_flag) {
				debug++;
			}
			sprintf_s(current->name,"%s", reg_table->entry_x[index].name);
			current->pointer = 0;
			current->atomic = 0;
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
				VariableListEntry s1;
				addVariableEntry(&s1, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
				INT64 number2;
				UINT8 number_valid2 = 0;
				parse_index_b(parse_out, &s1, &number2, &number_valid2, PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);

				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // 64b entries \n", PC[0], reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s1.reg_index].name); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // add index offset to array base \n", PC[0], reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[index].name, reg_table->entry_x[s1.reg_index].name); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s) // load array element, assumes 64b address \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1.reg_index].name); PC[0] += 4;
			
				reg_table->entry_x[s1.reg_index].in_use = 0;
				s1.reg_index_valid = 0;
			}
			else {
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[index].name); PC[0] += 4;
			}
			parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
		}
		else if (match_listB(&reg_index, parse_in->word, IO_list->name, IO_list->ptr, PC)) {
			if (neg_flag) {
				debug++;
			}
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
				UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				load_64bB(parse_out, PC, addr, IO_list->addr[reg_index], IO_list->name[reg_index].name, reg_table, l_control->depth, parse_in, param, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lr.w.aq.rl %s, 0(%s) \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[addr].name); PC[0] += 4;
				reg_table->entry_x[addr].in_use = 0;
			}
			else {
				debug++;
			}
			parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
		}
		else if (strcmp(parse_in->word, "max") == 0) {// error, need to do key words first, then variables
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
				parse_in->ptr++;
			}
			else {
				debug++;
			}
			getWordB(parse_in);
			if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
				if (current->reg_index_valid == 0) {
					current->reg_index = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
				}
			}
			else {
				debug++;
			}
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',') {
				parse_in->ptr++;
			}
			else {
				debug++;
			}
			getWordB(parse_in);
			VariableListEntry* current2 = compiler_var->list_base->next;
			if (get_integer(number, parse_in->word)) {
				UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
				load_64bB(parse_out, PC, s2i, number[0], (char*)"", reg_table, l_control->depth, parse_in, param, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[s2].name, reg_table->entry_x[s2i].name); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmax.h %s, %s, %s \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[s2].name); PC[0] += 4;
				reg_table->entry_x[s2i].in_use = 0;
				reg_table->entry_fp[s2].in_use = 0;
			}
			else if (get_float(&fp_data, parse_in->word)) {
				UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
				UINT index0 = (fp_data.hp & 0x0fff);
				UINT64 carry = (fp_data.hp & 0x800) << 1;
				UINT index1 = (((fp_data.hp + carry) >> 12) & 0x0fffff);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // start 16b load\n", PC[0], reg_table->entry_x[s2i].name, index1); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %#05x	// completed load of 0x%08x as 16b immediate load\n", PC[0], reg_table->entry_x[s2i].name, reg_table->entry_x[s2i].name, index0); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[s2].name, reg_table->entry_x[s2i].name); PC[0] += 4;
				if (current->reg_index == target->reg_index) {
					UINT8 new_target = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmax.h %s, %s, %s	// \n", PC[0], reg_table->entry_fp[new_target].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[s2].name); PC[0] += 4;
					reg_table->entry_fp[target->reg_index].in_use = 0;
					target->reg_index = new_target;
				}
				else {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmax.h %s, %s, %s	// \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[s2].name); PC[0] += 4;
				}
				reg_table->entry_fp[s2].in_use = 0;
				reg_table->entry_x[s2i].in_use = 0;
				if (target->type != fp16_enum)
					debug++; // syntax error
			}
			else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
				if (current2->reg_index_valid == 0) {
					current2->reg_index = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
				}
				if (target->reg_index != current->reg_index) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmax.h %s, %s, %s	// \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name); PC[0] += 4;
				}
				else {
					UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmax.h %s, %s, %s	// \n", PC[0], reg_table->entry_fp[s2].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name); PC[0] += 4;
					reg_table->entry_fp[target->reg_index].in_use = 0;
					target->reg_index = s2;
				}
			}
			else {
				debug++;
			}
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
				parse_in->ptr++;
				//				subset_depth[0]--;
			}
			else {
				debug++;
			}
			parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
		}
		else if (strcmp(parse_in->word, "min") == 0) {// error, need to do key words first, then variables
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
				parse_in->ptr++;
			}
			else {
				debug++;
			}
			getWordB(parse_in);
			if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
				if (current->reg_index_valid == 0) {
					current->reg_index = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
				}
			}
			else {
				debug++;
			}
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',') {
				parse_in->ptr++;
			}
			else {
				debug++;
			}
			getWordB(parse_in);
			VariableListEntry* current2 = compiler_var->list_base->next;
			if (get_integer(number, parse_in->word)) {
				UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
				load_64bB(parse_out, PC, s2i, number[0], (char*)"", reg_table, l_control->depth, parse_in, param, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[s2].name, reg_table->entry_x[s2i].name); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmin.h %s, %s, %s \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[s2].name); PC[0] += 4;
				reg_table->entry_x[s2i].in_use = 0;
				reg_table->entry_fp[s2].in_use = 0;
			}
			else if (get_float(&fp_data, parse_in->word)) {
				UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
				UINT index0 = (fp_data.hp & 0x0fff);
				UINT64 carry = (fp_data.hp & 0x800) << 1;
				UINT index1 = (((fp_data.hp + carry) >> 12) & 0x0fffff);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // start 16b load\n", PC[0], reg_table->entry_x[s2i].name, index1); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %#05x	// completed load of 0x%08x as 16b immediate load\n", PC[0], reg_table->entry_x[s2i].name, reg_table->entry_x[s2i].name, index0); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[s2].name, reg_table->entry_x[s2i].name); PC[0] += 4;
				if (current->reg_index == target->reg_index) {
					UINT8 new_target = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmin.h %s, %s, %s	// \n", PC[0], reg_table->entry_fp[new_target].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[s2].name); PC[0] += 4;
					reg_table->entry_fp[target->reg_index].in_use = 0;
					target->reg_index = new_target;
				}
				else {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmin.h %s, %s, %s	// \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[s2].name); PC[0] += 4;
				}
				reg_table->entry_fp[s2].in_use = 0;
				reg_table->entry_x[s2i].in_use = 0;
				if (target->type != fp16_enum)
					debug++; // syntax error
			}
			else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
				if (current2->reg_index_valid == 0) {
					current2->reg_index = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
				}
				//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmin.h %s, %s, %s	// \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name); PC[0] += 4;
				if (target->reg_index != current->reg_index) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmin.h %s, %s, %s	// \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name); PC[0] += 4;
				}
				else {
					UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmin.h %s, %s, %s	// \n", PC[0], reg_table->entry_fp[s2].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name); PC[0] += 4;
					reg_table->entry_fp[target->reg_index].in_use = 0;
					target->reg_index = s2;
				}
			}
			else {
				debug++;
			}
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
				parse_in->ptr++;
			}
			else {
				debug++;
			}
			parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
		}
		else if (strcmp(parse_in->word, "round") == 0) {
			UINT ptr_hold = parse_in->ptr;
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
				parse_in->ptr++;
			}
			else {
				debug++;
			}
			getWordB(parse_in);
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
				parse_in->ptr++;
				VariableListEntry* current2 = compiler_var->list_base->next;
				if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
					UINT8 temp2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.h.x %s, %s \n", PC[0], reg_table->entry_x[temp2].name, reg_table->entry_fp[current2->reg_index].name); PC[0] += 4;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.x.h %s, %s \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_x[temp2].name); PC[0] += 4;
					reg_table->entry_x[temp2].in_use = 0;
				}
				else {
					debug++;
				}
			}
			else {
				parse_in->ptr = ptr_hold;
				VariableListEntry target2;
				addVariableEntry(&target2, fp16_enum, (char*)"round() entry", reg_table, l_control, parse_out, parse_in, Masm);
				INT8 subset_depth = 0;
				parse_subset(parse_out, &target2, &subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control,param, Masm, unit_debug);
				if (subset_depth != 0)
					debug++;
				UINT8 temp2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.h.x %s, %s // start round()\n", PC[0], reg_table->entry_x[temp2].name, reg_table->entry_fp[target2.reg_index].name); PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.x.h %s, %s // complete round()\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_x[temp2].name); PC[0] += 4;
				reg_table->entry_fp[target2.reg_index].in_use = 0;
				target2.reg_index_valid = 0;
				reg_table->entry_x[temp2].in_use = 0;
			}
			num_valid = 0;
			parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control,param, Masm, unit_debug);
		}
		else {
			if (neg_flag) {
				debug++;
			}
			debug++;
			parse_rhs_continued(parse_out, target, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param,Masm, unit_debug);
		}
		//		parse_rhs_continued(parse_out, target, part2, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
	}
}

void parse_rhs_continued(parse_struct2* parse_out, VariableListEntry* target, VariableListEntry* s2, UINT part2, UINT8* num_valid, INT64* number, INT64* PC, parse_struct2* parse_in, compiler_var_type* compiler_var, IO_list_type* IO_list,
	operator_type* op_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, reg_table_struct* reg_table, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug) {
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != ';' && parse_in->line[parse_in->index].line[parse_in->ptr] != ':' && parse_in->line[parse_in->index].line[parse_in->ptr] != ')' && !part2) {
		UINT8 op_match;
		int debug = 0;
		if (getOperatorB(&op_match, parse_in, op_table)) {
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
				debug++;
				UINT8 hold = parse_in->ptr;
				parse_in->ptr++;
				getWord(parse_in);
				data_type_enum type;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && type_decode(&type, parse_in->word)) {
					parse_in->ptr++;
					getWord(parse_in);
					VariableListEntry* test;
					char index;
					if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
						cast_variable(parse_out, parse_in, test, target, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var,param, Masm, unit_debug);
					}
					else if (get_ArgumentListEntry(&test, &index, parse_in->word, compiler_var->f_list_base->last->argument, compiler_var->f_list_base->last->argc)) {
						cast_variable(parse_out, parse_in, test, target, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var,param, Masm, unit_debug);
					}
					else {
						debug++;
					}
				}
				else {
					parse_in->ptr = hold;
					// check for cast prior to going after subset
					VariableListEntry rs2;
					addVariableEntry(&rs2, target->type, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
					INT8 subset_depth = 0;
					parse_subset(parse_out, &rs2, &subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control,param, Masm, unit_debug);
					if (subset_depth != 0)
						debug++;
					switch (target->type) {
					case fp16_enum:
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s \n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s2->reg_index].name, reg_table->entry_fp[rs2.reg_index].name); PC[0] += 4;
						reg_table->entry_fp[rs2.reg_index].in_use = 0;
						rs2.reg_index_valid = 0;
						break;
					case int64_enum:
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s \n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s2->reg_index].name, reg_table->entry_x[rs2.reg_index].name); PC[0] += 4;
						reg_table->entry_x[rs2.reg_index].in_use = 0;
						rs2.reg_index_valid = 0;
						break;
					default:
						debug++;
						break;
					}
				}
			}
			else {
				if (target->type == fp16_enum) {
					UINT ptr_hold = parse_in->ptr;
					getWord(parse_in);
					fp_data_struct fp_data;
					VariableListEntry* test1, * test2;
					if (get_integer(number, parse_in->word) && (op_match == 0 || op_match == 1)) {//need to change to signed value
						parse_in->ptr = parse_in->ptr;
						UINT8 op_match2;
						UINT8 target_i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						UINT8 rs1 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x\n", PC[0], reg_table->entry_x[target_i].name, number[0]); PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.x.h %s, %s\n", PC[0], reg_table->entry_fp[rs1].name, reg_table->entry_x[target_i].name); PC[0] += 4;
						reg_table->entry_x[target_i].in_use = 0;
						if (getOperatorB(&op_match2, parse_in, op_table)) {
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(' && op_match2 == 2) {
								VariableListEntry rs2;
								addVariableEntry(&rs2, target->type, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
								INT8 subset_depth = 0;
								parse_subset(parse_out, &rs2, &subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control,param, Masm, unit_debug);
								if (subset_depth != 0)
									debug++;
								if (op_match == 0) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s, %s, %s\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[rs1].name, reg_table->entry_fp[rs2.reg_index].name, reg_table->entry_fp[target->reg_index].name); PC[0] += 4;
								}
								else {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s, %s, %s\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[rs1].name, reg_table->entry_fp[rs2.reg_index].name, reg_table->entry_fp[target->reg_index].name); PC[0] += 4;
								}
								reg_table->entry_fp[rs2.reg_index].in_use = 0;
							}
							else {
								getWord(parse_in);
								VariableListEntry* current;
								if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
									if (op_match == 0) {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s, %s, %s\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[rs1].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[target->reg_index].name); PC[0] += 4;
									}
									else {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s, %s, %s\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[rs1].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[target->reg_index].name); PC[0] += 4;
									}
								}
								else {
									debug++;
								}
							}
						}
						else {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%sh %s, %s, %s\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[rs1].name); PC[0] += 4;
							l_control->fence[l_control->depth] = 1;
						}
						reg_table->entry_fp[rs1].in_use = 0;
					}
					else if (get_float(&fp_data, parse_in->word) && parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
						parse_in->ptr++;
						getWord(parse_in);
						char index2;
						if (get_ArgumentListEntry(&test2, &index2, parse_in->word, compiler_var->f_list_base->last->argument, compiler_var->f_list_base->last->argc)) {
							memory_load(parse_out, test2, 11, test2, index2, PC, reg_table, parse_in, l_control->depth,param,  Masm);
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
								VariableListEntry offset;
								addVariableEntry(&offset, int64_enum, (char *) "offset", reg_table, l_control, parse_out, parse_in, Masm);
								UINT8 number_valid;
								parse_index_b(parse_out, &offset, &number[0], &number_valid, PC, test2, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param,Masm, unit_debug);
								if (number_valid) {
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']' && ((8 * (number[0] + 1)) < 0x800)) {
										memory_load(parse_out, target, test2->reg_index, target, number[0], PC, reg_table, parse_in, l_control->depth, param, Masm);
									}
									else
										debug++;
								}
								else {
									switch (test2->type) {
									case int8_enum:
									case uint8_enum:
										break;
									case int16_enum:
									case fp16_enum:
									case uint16_enum:
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 1 // index address * 8 (bytes per entry) for variable \"%s\"\n",
											PC[0], reg_table->entry_x[offset.reg_index].name, reg_table->entry_x[offset.reg_index].name, test2->name); PC[0] += 4;
										break;
									case int64_enum:
									case uint64_enum:
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // index address * 8 (bytes per entry) for variable \"%s\"\n",
											PC[0], reg_table->entry_x[offset.reg_index].name, reg_table->entry_x[offset.reg_index].name, test2->name); PC[0] += 4;
										break;
									default:
										debug++;
										break;
									}
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // base address plus offset for  \"%s\"\n",
										PC[0], reg_table->entry_x[offset.reg_index].name, reg_table->entry_x[test2->reg_index].name, reg_table->entry_x[offset.reg_index].name, test2->name); PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x flh %s, 0xff8(%s) // base address plus offset for  \"%s\"\n",
										PC[0], reg_table->entry_x[test2->reg_index].name, reg_table->entry_x[offset.reg_index].name, test2->name); PC[0] += 4;
								}
								reg_table->entry_x[offset.reg_index].in_use = 0;
							}
							else {
								debug++;
							}
						}
						else if (get_VariableListEntryB(&test2, parse_in->word, compiler_var->list_base)) {
							debug++;
						}
						else {
							debug++;
						}
						UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						UINT8 s2f = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
						UINT index0 = (fp_data.hp & 0x0fff);
						UINT64 carry = (fp_data.hp & 0x800) << 1;
						UINT index1 = (((fp_data.hp + carry) >> 12) & 0x0fffff);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // start 16b load\n", PC[0], reg_table->entry_x[s2i].name, index1); PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %#05x	// completed load of 0x%08x as 16b immediate load\n", PC[0], reg_table->entry_x[s2i].name, reg_table->entry_x[s2i].name, index0); PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[s2f].name, reg_table->entry_x[s2i].name); PC[0] += 4;
						reg_table->entry_x[s2i].in_use = 0;
						if (op_match == 0) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s, %s, %s	// \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s2f].name, reg_table->entry_fp[test2->reg_index].name, reg_table->entry_fp[s2->reg_index].name); PC[0] += 4;
						}
						else if (op_match == 1) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s, %s, %s	// \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s2f].name, reg_table->entry_fp[test2->reg_index].name, reg_table->entry_fp[s2->reg_index].name); PC[0] += 4;
						}
						else {
							debug++;
						}
						reg_table->entry_fp[s2f].in_use = 0;
						reg_table->entry_fp[test2->reg_index].in_use = 0;
						test2->reg_index_valid = 0;
					}
					else if (get_VariableListEntryB(&test1, parse_in->word, compiler_var->list_base) && parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
						parse_in->ptr++;
						getWord(parse_in);
						if (get_VariableListEntryB(&test2, parse_in->word, compiler_var->list_base)) {
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
								VariableListEntry offset;
								addVariableEntry(&offset, int64_enum, (char *) "offset", reg_table, l_control, parse_out, parse_in, Masm);
								UINT8 number_valid;
								parse_index_b(parse_out, &offset, &number[0], &number_valid, PC, test2, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
								UINT8 s2f = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
								if (number_valid && (number[0] + 1) * 2 < 0x800) {
									if (test2->reg_index_valid) {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x flh %s, 0x%03x(%s) \n", PC[0], reg_table->entry_fp[s2f].name, -((number[0] + 1) * 2), reg_table->entry_fp[test2->reg_index].name); PC[0] += 4;
									}
									else {
										//										test2->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
										//										test2->reg_index_valid = 1;
										if (test2->sp_offset < 0x800) {
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x flh %s, 0x%03x(sp) \n", PC[0], reg_table->entry_fp[s2f].name, test2->sp_offset - ((number[0] + 1) * 2), reg_table->entry_fp[test2->reg_index].name); PC[0] += 4;
										}
										else {
											debug++;
										}
									}
								}
								else {
									debug++;
								}
								if (op_match == 0) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s, %s, %s \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[test1->reg_index].name, reg_table->entry_fp[s2f].name, reg_table->entry_fp[s2->reg_index].name); PC[0] += 4;
								}
								else if (op_match == 1) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s, %s, %s \n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[test1->reg_index].name, reg_table->entry_fp[s2f].name, reg_table->entry_fp[s2->reg_index].name); PC[0] += 4;
								}
								else {
									debug++;
								}
								reg_table->entry_fp[offset.reg_index].in_use = 0;
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
					}
					else {
						VariableListEntry right;
						right.reg_index = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
						right.reg_index_valid = 1;
						sprintf_s(right.name, "");
						right.type = target->type;
						parse_in->ptr = ptr_hold;
						parse_rh_of_eqB(parse_out, &right, 1, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						num_valid[0] = 0;
						if (target->reg_index == s2->reg_index) {
							UINT8 temp_fp = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s \n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[temp_fp].name, reg_table->entry_fp[s2->reg_index].name, reg_table->entry_fp[right.reg_index].name); PC[0] += 4;
							reg_table->entry_fp[target->reg_index].in_use = 0;
							target->reg_index = temp_fp;
						}
						else {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s \n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s2->reg_index].name, reg_table->entry_fp[right.reg_index].name); PC[0] += 4;
						}
						reg_table->entry_fp[right.reg_index].in_use = 0;
					}
				}
				else {
					VariableListEntry right;
					right.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					right.reg_index_valid = 1;
					sprintf_s(right.name, "");
					right.type = target->type;
					parse_rh_of_eqB(parse_out, &right, 1, num_valid, number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
					num_valid[0] = 0;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s \n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s2->reg_index].name, reg_table->entry_x[right.reg_index].name); PC[0] += 4;
					reg_table->entry_x[right.reg_index].in_use = 0;
				}
			}
		}
	}
}

void cast_variable(parse_struct2* parse_out, parse_struct2* parse_in, VariableListEntry* test, VariableListEntry* target, loop_control* l_control, INT64* logical_PC, IO_list_type* IO_list, reg_table_struct* reg_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, operator_type* op_table, compiler_var_type* compiler_var, param_type* param, FILE* Masm, UINT8 unit_debug) {
	int debug = 0;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
		if (test->type == uint8_enum) {
		}
		else {
			debug++;
		}
		if (target->type != fp16_enum)
			debug++;
		UINT8 number_valid = 0;
		INT64 number;
		VariableListEntry offset;
		addVariableEntry(&offset, int64_enum, (char *) "offset", reg_table, l_control, parse_out, parse_in, Masm);

		parse_index_b(parse_out, &offset, &number, &number_valid, logical_PC, test, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
		VariableListEntry temp_i;
		addVariableEntry(&temp_i, uint8_enum, test->name, reg_table, l_control, parse_out, parse_in, Masm);
		if (number_valid) {
			if (number < 0x800 && number >= -0x800) {
				if (test->reg_index_valid != 1)
					debug++;
				memory_load(parse_out, &temp_i, test->reg_index, &temp_i, number, logical_PC, reg_table, parse_in, l_control->depth, param, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.x.h %s, %s \n", logical_PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_x[temp_i.reg_index].name); logical_PC[0] += 4;
			}
			else {
				if (test->reg_index_valid == 0)
					debug++;
				load_64bB(parse_out, logical_PC, offset.reg_index, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %s \n", logical_PC[0], reg_table->entry_x[offset.reg_index].name, reg_table->entry_x[test->reg_index].name, reg_table->entry_x[offset.reg_index].name); logical_PC[0] += 4;
				memory_load(parse_out, &temp_i, offset.reg_index, &temp_i, 0, logical_PC, reg_table, parse_in, l_control->depth, param, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.x.h %s, %s \n", logical_PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_x[temp_i.reg_index].name); logical_PC[0] += 4;
			}
		}
		else {
			if (test->reg_index_valid == 0)
				debug++;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %s \n", logical_PC[0], reg_table->entry_x[offset.reg_index].name, reg_table->entry_x[test->reg_index].name, reg_table->entry_x[offset.reg_index].name); logical_PC[0] += 4;
			memory_load(parse_out, &temp_i, offset.reg_index, &temp_i, 0, logical_PC, reg_table, parse_in, l_control->depth, param, Masm);
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.x.h %s, %s // target is %s\n", logical_PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_x[temp_i.reg_index].name, target->name); logical_PC[0] += 4;
		}
		reg_table->entry_x[temp_i.reg_index].in_use = 0;
		reg_table->entry_x[offset.reg_index].in_use = 0;
		temp_i.reg_index_valid = 0;
		offset.reg_index_valid = 0;
	}
	else if (test->type == void_enum) {
		if (test->reg_fp == 0) {
			if (target->type == fp16_enum) {
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s // target is %s\n", logical_PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_x[test->reg_index].name, target->name); logical_PC[0] += 4;
			}
			else
				debug++;
		}
		else { // need to add zero fp
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fadd.h %s, %s,%s // target is %s\n", logical_PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[test->reg_index].name, reg_table->entry_fp[test->reg_index].name, target->name); logical_PC[0] += 4;
		}
	}
	else if (test->type == uint16_enum) {
		if (target->type == fp16_enum) {
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.x.h %s, %s // int to float\n", logical_PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_x[test->reg_index].name, target->name); logical_PC[0] += 4;
		}
		else if (target->type == int64_enum) {
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, zero // \n", logical_PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[test->reg_index].name, target->name); logical_PC[0] += 4;
		}
		else {
			debug++;
		}
	}
	else {
		debug++;
	}

}
void parse_subset(parse_struct2* parse_out, VariableListEntry* target, INT8* subset_depth, parse_struct2* parse_in, compiler_var_type* compiler_var, IO_list_type* IO_list, operator_type* op_table, operator_type* branch_table,
	var_type_struct* csr_list, UINT8 csr_list_count, INT64* PC, reg_table_struct* reg_table, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug) {
	int debug = 0;
	int cast_input = 0;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
		subset_depth[0]++;
		parse_in->ptr++;
	}
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
		subset_depth[0]++;
		UINT hold = parse_in->ptr;
		parse_in->ptr++;
		getWord(parse_in);

		if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
			subset_depth[0]--;
			parse_in->ptr++;
			data_type_enum type;
			if (type_decode(&type, parse_in->word)) {
				getWord(parse_in);
				VariableListEntry* test;
				char index;
				if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
					cast_variable(parse_out, parse_in, test, target, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
				}
				else if (get_ArgumentListEntry(&test, &index, parse_in->word, compiler_var->f_list_base->last->argument, compiler_var->f_list_base->last->argc)) {
					cast_variable(parse_out, parse_in, test, target, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
				}
				else {
					debug++;
				}
			}
			else {
				debug++;
			}
		}
		else {
			parse_in->ptr = hold;
			parse_subset(parse_out, target, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
		}
		UINT subset_depth_test = subset_depth[0] - 1;
		while (parse_in->line[parse_in->index].line[parse_in->ptr] != ';' && parse_in->line[parse_in->index].line[parse_in->ptr] != ')' && parse_in->line[parse_in->index].line[parse_in->ptr] != ']' && subset_depth[0] > subset_depth_test)
			parse_s2B(parse_out, target, target->reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
	}
	else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '-') {
		parse_in->ptr++;
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ')	parse_in->ptr++;
		getWord(parse_in);
		INT64 number0;
		VariableListEntry* current = compiler_var->list_base->next;
		if (get_integer(&number0, parse_in->word)) {
			number0 *= -1;
			UINT8 op_match;
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
				subset_depth++;
				UINT8 index0 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				load_64bB(parse_out, PC, index0, number0, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);// error, dropped data
				VariableListEntry s1;
				addVariableEntry(&s1, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
				parse_subset(parse_out, &s1, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
				if (parse_in->line[parse_in->index].line[parse_in->ptr] != ';')
					parse_s2B(parse_out, target, s1.reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
				reg_table->entry_x[s1.reg_index].in_use = 0;
				reg_table->entry_x[index0].in_use = 0;
			}
			else if (getOperatorB(&op_match, parse_in, op_table)) {
				UINT8 index0 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				load_64bB(parse_out, PC, index0, number0, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
				getWord(parse_in);
				INT64 number;
				VariableListEntry* current2 = compiler_var->list_base->next;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
					subset_depth++;
					VariableListEntry s1;
					addVariableEntry(&s1, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
					parse_subset(parse_out, &s1, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
					if (parse_in->line[parse_in->index].line[parse_in->ptr] != ';')
						parse_s2B(parse_out, target, s1.reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[index0].name); PC[0] += 4;
				}
				else if (get_integer(&number, parse_in->word)) {
					if (index0 > 0x1f)
						debug++;
					print_op_immediate2(parse_out, op_match, target->reg_index, index0, number, op_table, PC, reg_table, l_control->depth, parse_in,param,  Masm);
				}
				else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
					UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, PC, offset, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name, reg_table->entry_x[index0].name); PC[0] += 4;
					reg_table->entry_x[offset].in_use = 0;
				}
				else {
					debug++;// not coded
				}
				reg_table->entry_x[index0].in_use = 0;
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
				subset_depth--;
				load_64bB(parse_out, PC, target->reg_index, number0, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
			}
			else {
				debug++;// syntax
			}
		}
		else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
			UINT8 op_match, branch_match;
			UINT8 neg_current = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != ')' && parse_in->line[parse_in->index].line[parse_in->ptr] != ';') {
				if (getOperatorB(&op_match, parse_in, op_table)) {
					getWordB(parse_in);
					INT64 number;
					VariableListEntry* current2 = compiler_var->list_base->next;
					//					FunctionListEntry* function_match = compiler_var->f_list_base->next;
					if (get_integer(&number, parse_in)) {
						if (current->pointer) {
							UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, PC, addr, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
							print_op_immediate2(parse_out, op_match, target->reg_index, addr, number, op_table, PC, reg_table, l_control->depth, parse_in, param, Masm);
							reg_table->entry_x[addr].in_use = 0;
						}
						else {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, zero, %s\n", PC[0], reg_table->entry_x[neg_current].name, reg_table->entry_x[current->reg_index].name); PC[0] += 4;
							print_op_immediate2(parse_out, op_match, target->reg_index, neg_current, number, op_table, PC, reg_table, l_control->depth, parse_in,param,  Masm);
						}
					}
					else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, zero, %s\n", PC[0], reg_table->entry_x[neg_current].name, reg_table->entry_x[current->reg_index].name); PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[neg_current].name, reg_table->entry_x[offset].name); PC[0] += 4;
						reg_table->entry_x[offset].in_use = 0;
					}
					else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->global_list_base)) {// global_list_base
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, zero, %s\n", PC[0], reg_table->entry_x[neg_current].name, reg_table->entry_x[current->reg_index].name); PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[neg_current].name, reg_table->entry_x[offset].name); PC[0] += 4;
						reg_table->entry_x[offset].in_use = 0;
					}
					else {
						UINT hit = 0;
						for (UINT i = 0; i < compiler_var->f_list_base->last->argc && !hit; i++) {
							if (strcmp(parse_in->word, compiler_var->f_list_base->last->argument[i].name) == 0) {
								hit = 1;
								UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, zero, %s\n", PC[0], reg_table->entry_x[neg_current].name, reg_table->entry_x[current->reg_index].name); PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s,0x%03x(a1) // pull value of \"%s\" of the stack\n", 
									PC[0], reg_table->entry_x[offset].name, -(int) ((i + 1) << 3), compiler_var->f_list_base->last->argument[i].name); PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[neg_current].name, reg_table->entry_x[offset].name); PC[0] += 4;
								reg_table->entry_x[offset].in_use = 0;
							}
						}
						if (!hit) { // error, dropping current register value - must have an invalid value 
							UINT8 num_valid;
							VariableListEntry target2;
							target2.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							target2.reg_index_valid = 1;
							sprintf_s(target2.name, "");
							target2.type = int64_enum;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x nop // ERROR: dropping neg current reg index, must yield invalid result\n", PC[0]); PC[0] += 4;
							parse_rh_of_eqB(parse_out, &target2, 1, &num_valid, &number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target2.reg_index].name); PC[0] += 4;
							reg_table->entry_x[target2.reg_index].in_use = 0;
						}
					}
				}
				else if (getBranchB(&branch_match, parse_in, branch_table)) {
					getWord(parse_in);
					INT64 number;
					VariableListEntry* current2 = compiler_var->list_base->next;
					if (get_integer(&number, parse_in->word)) {
						UINT n2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
						if (number == 0) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, zero // predication comparison\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[offset].name); PC[0] += 4;
						}
						else {
							load_64bB(parse_out, PC, n2, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // predication comparison\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[offset].name, reg_table->entry_x[n2].name); PC[0] += 4;
						}
						reg_table->entry_x[offset].in_use = 0;
						UINT s2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						VariableListEntry s1;
						s1.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						s1.reg_index_valid = 1;
						sprintf_s(s1.name, "");
						s1.type = int64_enum;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '?') {
							subset_depth[0]--;
							parse_in->ptr += 2;
							UINT8 num_valid = 0;
							parse_rh_of_eqB(parse_out, &s1, 1, &num_valid, &number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
							num_valid = 0;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ':') {
								parse_in->ptr++;
								parse_rh_of_eqB(parse_out, &s1, 1, &num_valid, &number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
								num_valid = 0;
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 64 // pos = 0, neg = -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
						switch (branch_match) {
						case 0:// <
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition true\n", PC[0], reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition false\n", PC[0], reg_table->entry_x[s2].name, reg_table->entry_x[s2].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // predicated result\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s2].name); PC[0] += 4;
							break;
						case 1:// >
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition false\n", PC[0], reg_table->entry_x[s2].name, reg_table->entry_x[s2].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition true\n", PC[0], reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // predicated result\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s2].name); PC[0] += 4;
							break;
						default:
							debug++;
							break;
						}
						reg_table->entry_x[n2].in_use = 0;
						reg_table->entry_x[s1.reg_index].in_use = 0;
						s1.reg_index_valid = 0;
						reg_table->entry_x[s2].in_use = 0;
					}
					else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
						UINT8 offset2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset2, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name, reg_table->entry_x[offset2].name); PC[0] += 4;
						reg_table->entry_x[offset].in_use = 0;
						reg_table->entry_x[offset2].in_use = 0;
					}
					else {
						debug++;
					}

				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
					int ptr_hold = parse_in->ptr;
					parse_in->ptr++;
					getWordB(parse_in);
					INT64 number;
					if (get_integer(&number, parse_in->word)) {
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
							UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
							if (current->atomic) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lr.d.aq.rl %s, 0x%03x(%s)\n", PC[0], reg_table->entry_x[target->reg_index].name, number, reg_table->entry_x[offset].name); PC[0] += 4;//current->name,
							}
							else {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x%03x(%s)\n", PC[0], reg_table->entry_x[target->reg_index].name, -8 * (number + 1), reg_table->entry_x[offset].name); PC[0] += 4;//current->name,
							}
							parse_in->ptr++;
							reg_table->entry_x[offset].in_use = 0;
						}
						else {
							parse_s2B(parse_out, target, target->reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
						}
					}
					else {
						parse_in->ptr = ptr_hold;
						parse_index_a(parse_out, target, 0, PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
					}

					if (parse_in->line[parse_in->index].line[parse_in->ptr] != ')') {
						parse_s2B(parse_out, target, target->reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
					}
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
					subset_depth[0]--;
					UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, zero		// \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
					parse_in->ptr++;
					reg_table->entry_x[offset].in_use = 0;
				}
				else {
					debug++;// syntax
				}
			}
			reg_table->entry_x[neg_current].in_use = 0;
		}
		else {
			VariableListEntry target2;
			addVariableEntry(&target2, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
			if (check_funct_arg(parse_out, &target2, 0, parse_in, PC, reg_table, l_control, current, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, param, Masm, unit_debug)) {
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, zero,%s		// \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target2.reg_index].name); PC[0] += 4;
				UINT8 op_match, branch_match;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] != ')' && parse_in->line[parse_in->index].line[parse_in->ptr] != ';') {
					if (getOperatorB(&op_match, parse_in, op_table)) {
						getWordB(parse_in);
						INT64 number;
						VariableListEntry* current2 = compiler_var->list_base->next;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
							parse_subset(parse_out, &target2, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s	// accumulate result\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target2.reg_index].name); PC[0] += 4;
							reg_table->entry_x[target2.reg_index].in_use = 0;
						}
						else if (get_integer(&number, parse_in)) {
							if (current->pointer) {
								UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								load_64bB(parse_out, PC, addr, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
								print_op_immediate2(parse_out, op_match, target->reg_index, addr, number, op_table, PC, reg_table, l_control->depth, parse_in, param, Masm);
								reg_table->entry_x[addr].in_use = 0;
							}
							else {
								print_op_immediate2(parse_out, op_match, target->reg_index, target->reg_index, number, op_table, PC, reg_table, l_control->depth, parse_in, param, Masm);
							}
						}
						else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
							UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, PC, offset, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
							reg_table->entry_x[offset].in_use = 0;
						}
						else if (check_funct_arg(parse_out, &target2, 0, parse_in, PC, reg_table, l_control, current, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, param, Masm, unit_debug)) {
							// must check function arguments before global variables
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target2.reg_index].name); PC[0] += 4;
						}
						else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->global_list_base)) {// global_list_base
							UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, PC, offset, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
							reg_table->entry_x[offset].in_use = 0;
						}
						else {
							UINT hit = 0;
							for (UINT i = 0; i < compiler_var->f_list_base->last->argc && !hit; i++) {
								if (strcmp(parse_in->word, compiler_var->f_list_base->last->argument[i].name) == 0) {
									hit = 1;
									UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s,0x%03x(a1) // pull value of \"%s\" of the stack\n", 
										PC[0], reg_table->entry_x[offset].name, -(int)((i + 1) << 3), compiler_var->f_list_base->last->argument[i].name); PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
									reg_table->entry_x[offset].in_use = 0;
								}
							}
							if (!hit) { // error, dropping current register value - must have an invalid value 
								UINT8 num_valid;
								//								UINT8 target2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								VariableListEntry target2;
								target2.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								target2.reg_index_valid = 1;
								sprintf_s(target2.name, "");
								target2.type = int64_enum;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x nop // ERROR: dropping neg current reg index, must yield invalid result\n", PC[0]); PC[0] += 4;
								parse_rh_of_eqB(parse_out, &target2, 1, &num_valid, &number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target2.reg_index].name); PC[0] += 4;
								reg_table->entry_x[target2.reg_index].in_use = 0;
								target2.reg_index_valid = 0;
							}
						}
					}
					else if (getBranchB(&branch_match, parse_in, branch_table)) {
						getWord(parse_in);
						INT64 number;
						VariableListEntry* current2 = compiler_var->list_base->next;
						if (get_integer(&number, parse_in->word)) {
							UINT n2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, PC, offset, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
							if (number == 0) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, zero // predication comparison\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[offset].name); PC[0] += 4;
							}
							else {
								load_64bB(parse_out, PC, n2, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // predication comparison\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[offset].name, reg_table->entry_x[n2].name); PC[0] += 4;
							}
							reg_table->entry_x[offset].in_use = 0;
							UINT s2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							VariableListEntry s1;
							s1.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							s1.reg_index_valid = 1;
							sprintf_s(s1.name, "");
							s1.type = int64_enum;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '?') {
								subset_depth[0]--;
								parse_in->ptr += 2;
								UINT8 num_valid = 0;
								parse_rh_of_eqB(parse_out, &s1, 1, &num_valid, &number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
								num_valid = 0;
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == ':') {
									parse_in->ptr++;
									parse_rh_of_eqB(parse_out, &s1, 1, &num_valid, &number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
									num_valid = 0;
								}
								else {
									debug++;
								}
							}
							else {
								debug++;
							}
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 64 // pos = 0, neg = -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
							switch (branch_match) {
							case 0:// <
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition true\n", PC[0], reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[n2].name); PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition false\n", PC[0], reg_table->entry_x[s2].name, reg_table->entry_x[s2].name, reg_table->entry_x[n2].name); PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // predicated result\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s2].name); PC[0] += 4;
								break;
							case 1:// >
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition false\n", PC[0], reg_table->entry_x[s2].name, reg_table->entry_x[s2].name, reg_table->entry_x[n2].name); PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition true\n", PC[0], reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[n2].name); PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // predicated result\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s2].name); PC[0] += 4;
								break;
							default:
								debug++;
								break;
							}
							reg_table->entry_x[n2].in_use = 0;
							reg_table->entry_x[s1.reg_index].in_use = 0;
							reg_table->entry_x[s2].in_use = 0;
							s1.reg_index_valid = 0;
						}
						else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
							UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
							UINT8 offset2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, PC, offset2, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name, reg_table->entry_x[offset2].name); PC[0] += 4;
							reg_table->entry_x[offset].in_use = 0;
							reg_table->entry_x[offset2].in_use = 0;
						}
						else {
							debug++;
						}

					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
						int ptr_hold = parse_in->ptr;
						parse_in->ptr++;
						getWordB(parse_in);
						INT64 number;
						if (get_integer(&number, parse_in->word)) {
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
								UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
								if (current->atomic) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lr.d.aq.rl %s, 0x%03x(%s)\n", PC[0], reg_table->entry_x[target->reg_index].name, number, reg_table->entry_x[offset].name); PC[0] += 4;//current->name,
								}
								else {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x%03x(%s)\n", PC[0], reg_table->entry_x[target->reg_index].name, -8 * (number + 1), reg_table->entry_x[offset].name); PC[0] += 4;//current->name,
								}
								parse_in->ptr++;
								reg_table->entry_x[offset].in_use = 0;
							}
							else {
								parse_s2B(parse_out, target, target->reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
							}
						}
						else {
							parse_in->ptr = ptr_hold;
							parse_index_a(parse_out, target, 0, PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						}

						if (parse_in->line[parse_in->index].line[parse_in->ptr] != ')') {
							parse_s2B(parse_out, target, target->reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
						}
					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
						subset_depth[0]--;
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, zero		// \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
						parse_in->ptr++;
						reg_table->entry_x[offset].in_use = 0;
					}
					else {
						debug++;// syntax
					}
				}
			}
			else {
				debug++;
			}
			reg_table->entry_x[target2.reg_index].in_use = 0;
		}
	}
	else {
		getWord(parse_in);
		INT64 number0;
		fp_data_struct fp_data;
		UINT8 index;
		VariableListEntry* current = compiler_var->list_base->next;
		UINT8 csr_index;
		UINT8 reg_index = 0;
		if (parse_in->index == 101)
			debug++;
		data_type_enum type;
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && type_decode(&type, parse_in->word)) {
			subset_depth[0]--;
			cast_input = 1;
			parse_in->ptr++;
			getWord(parse_in);
			VariableListEntry* test;
			char index;
			if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
				cast_variable(parse_out, parse_in, test, target, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
			}
			else if (get_ArgumentListEntry(&test, &index, parse_in->word, compiler_var->f_list_base->last->argument, compiler_var->f_list_base->last->argc)) {
				cast_variable(parse_out, parse_in, test, target, l_control, PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
			}
			else {
				debug++;
			}
		}
		else if (match_listB(&reg_index, parse_in->word, IO_list->name, IO_list->ptr, PC)) {
			debug++;
			UINT8 regA = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			load_64bB(parse_out, PC, addr, IO_list->addr[reg_index], IO_list->name[reg_index].name, reg_table, l_control->depth, parse_in, param, Masm);
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lr.w.aq.rl %s, 0(%s) \n", PC[0], reg_table->entry_x[regA].name, reg_table->entry_x[addr].name); PC[0] += 4;
			reg_table->entry_x[addr].in_use = 0;

			UINT8 op_match;
			if (getOperatorB(&op_match, parse_in, op_table)) {
				getWordB(parse_in);
				INT64 number;
				if (get_integer(&number, parse_in->word)) {
					print_op_immediate2(parse_out, op_match, target->reg_index, regA, number, op_table, PC, reg_table, l_control->depth, parse_in, param, Masm);
				}
				else {
					debug++;
				}
			}
			else {
				debug++;
			}
			reg_table->entry_x[regA].in_use = 0;
		}
		else if (match_listB(&csr_index, parse_in->word, csr_list, csr_list_count, PC)) {
			UINT8 csr_copy = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrci %s, 0x00, %s\n", PC[0], reg_table->entry_x[csr_copy].name, csr_list[csr_index]); PC[0] += 4;
			sprint_csr_imm(parse_out, PC, (char*)"csrrci", reg_table->entry_x[csr_copy].name, 0, csr_list[csr_index].name, param);
			UINT8 op_match;
			if (getOperatorB(&op_match, parse_in, op_table)) {
				getWordB(parse_in);
				INT64 number;
				VariableListEntry* current2 = compiler_var->list_base->next;
				if (get_integer(&number, parse_in->word)) {
					if (csr_copy > 0x1f)
						debug++;
					print_op_immediate2(parse_out, op_match, target->reg_index, csr_copy, number, op_table, PC, reg_table, l_control->depth, parse_in,param, Masm);
				}
				else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
//					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[csr_copy].name, reg_table->entry_x[csr_copy].name); PC[0] += 4;
//					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[csr_copy].name, reg_table->entry_x[csr_copy].name); PC[0] += 4;
					sprint_op_2src(parse_out, PC, op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[csr_copy].name, reg_table->entry_x[csr_copy].name, param);
					sprint_op_2src(parse_out, PC, op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[csr_copy].name, reg_table->entry_x[csr_copy].name, param);
				}
				else {
					debug++;
				}
			}
			reg_table->entry_x[csr_copy].in_use = 0;
		}
		else if (find_loop_reg_index(l_control, parse_in->word)) {
			UINT8 op_match, branch_match;

			if (getOperatorB(&op_match, parse_in, op_table)) {
				getWord(parse_in);
				INT64 number;
				char index;
				char loop_index = l_control->index;
				VariableListEntry* current2 = compiler_var->list_base->next;
				if (get_integer(&number, parse_in)) {
					print_op_immediate2(parse_out, op_match, target->reg_index, l_control->for_loop[l_control->index].index_reg, number, op_table, PC, reg_table, l_control->depth, parse_in,param, Masm);
				}
				else if (find_loop_reg_index(l_control, parse_in->word)) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // %s %s %s \n", PC[0],
						op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[l_control->for_loop[loop_index].index_reg].name, reg_table->entry_x[l_control->for_loop[l_control->index].index_reg].name,
						l_control->for_loop[loop_index].index_name, op_table[op_match].symbol, l_control->for_loop[l_control->index].index_name); PC[0] += 4;
				}
				else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
					UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, PC, offset, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
					reg_table->entry_x[offset].in_use = 0;
				}
				else if (get_ArgumentListEntry(&current2, &index, parse_in->word, compiler_var->f_list_base->last->argument, compiler_var->f_list_base->last->argc)) {
					if (current2->pointer) {
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[')
							parse_in->ptr++;
						else
							debug++;
						getWord(parse_in);
						if (get_integer(&number, parse_in)) {
							VariableListEntry addr;
							addVariableEntry(&addr, int64_enum, compiler_var->f_list_base->last->argument[index].name, reg_table, l_control, parse_out, parse_in, Masm);
							VariableListEntry temp;
							addVariableEntry(&temp, compiler_var->f_list_base->last->argument[index].type, compiler_var->f_list_base->last->argument[index].name, reg_table, l_control, parse_out, parse_in, Masm);
							memory_load(parse_out, &addr, 11, &addr, index, PC, reg_table, parse_in, l_control->depth, param, Masm);
							memory_load(parse_out, &temp, addr.reg_index, &temp, number, PC, reg_table, parse_in, l_control->depth, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // %s %s %s[%d] \n", PC[0],
								op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[l_control->for_loop[loop_index].index_reg].name, reg_table->entry_x[temp.reg_index].name, l_control->for_loop[loop_index].index_name, op_table[op_match].symbol, compiler_var->f_list_base->last->argument[index].name, number); PC[0] += 4;
							reg_table->entry_x[addr.reg_index].in_use = 0;
							reg_table->entry_x[temp.reg_index].in_use = 0;
							addr.reg_index_valid = 0;
							temp.reg_index_valid = 0;
						}
						else {
							debug++;
						}
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']')
							parse_in->ptr++;
						else
							debug++;
					}
					else {
						debug++;
						/*
						VariableListEntry temp;
						addVariableEntry(&temp, int64_enum, strcat(compiler_var->f_list_base->last->argument[index].name, " value(base64)"), reg_table, l_control, parse_out, parse_in, Masm);
						memory_load(parse_out, &temp, 11, &temp, index, PC, reg_table, parse_in, l_control->depth, param, Masm);
						// NOTE: if fp, need to move value to fp reg
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[l_control->for_loop[loop_index].index_reg].name, reg_table->entry_x[temp.reg_index].name); PC[0] += 4;
						reg_table->entry_x[temp.reg_index].in_use = 0;
						temp.reg_index_valid = 0;
						/**/
					}
				}
				else {
					debug++;
				}
			}
			else if (getBranchB(&branch_match, parse_in, branch_table)) {
				getWordB(parse_in);
				INT64 number;
				VariableListEntry* current2 = compiler_var->list_base->next;
				if (get_integer(&number, parse_in->word)) {
					UINT n2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, PC, offset, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
					if (number == 0) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, zero // predication comparison\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[offset].name); PC[0] += 4;
					}
					else {
						load_64bB(parse_out, PC, n2, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // predication comparison\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[offset].name, reg_table->entry_x[n2].name); PC[0] += 4;
					}
					reg_table->entry_x[offset].in_use = 0;
					//					UINT s1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					UINT s2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					VariableListEntry s1;
					s1.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					s1.reg_index_valid = 1;
					sprintf_s(s1.name, "");
					s1.type = int64_enum;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '?') {
						subset_depth[0]--;
						parse_in->ptr += 2;
						UINT8 num_valid = 0;
						parse_rh_of_eqB(parse_out, &s1, 1, &num_valid, &number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						num_valid = 0;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ':') {
							parse_in->ptr++;
							parse_rh_of_eqB(parse_out, &s1, 1, &num_valid, &number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
							num_valid = 0;
						}
						else {
							debug++;
						}
					}
					else {
						debug++;
					}
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 64 // pos = 0, neg = -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
					switch (branch_match) {
					case 0:// <
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition true\n", PC[0], reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[n2].name); PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition false\n", PC[0], reg_table->entry_x[s2].name, reg_table->entry_x[s2].name, reg_table->entry_x[n2].name); PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // predicated result\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s2].name); PC[0] += 4;
						break;
					case 1:// >
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition false\n", PC[0], reg_table->entry_x[s2].name, reg_table->entry_x[s2].name, reg_table->entry_x[n2].name); PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition true\n", PC[0], reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[n2].name); PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // predicated result\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s2].name); PC[0] += 4;
						break;
					default:
						debug++;
						break;
					}
					reg_table->entry_x[n2].in_use = 0;
					reg_table->entry_x[s1.reg_index].in_use = 0;
					reg_table->entry_x[s2].in_use = 0;
					s1.reg_index_valid = 0;
				}
				else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
					UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
					UINT8 offset2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, PC, offset2, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name, reg_table->entry_x[offset2].name); PC[0] += 4;
					reg_table->entry_x[offset].in_use = 0;
					reg_table->entry_x[offset2].in_use = 0;
				}
				else {
					debug++;
				}

			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
				int ptr_hold = parse_in->ptr;
				parse_in->ptr++;
				getWord(parse_in);
				INT64 number;
				if (get_integer(&number, parse_in->word)) {
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
						if (current->atomic) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lr.d.aq.rl %s, 0x%03x(%s)\n", PC[0], reg_table->entry_x[target->reg_index].name, number, reg_table->entry_x[offset].name); PC[0] += 4;//current->name,
						}
						else {
							memory_load(parse_out, target, offset, target, number, PC, reg_table, parse_in, l_control->depth, param, Masm);
						}
						parse_in->ptr++;
						reg_table->entry_x[offset].in_use = 0;
					}
					else {
						parse_s2B(parse_out, target, target->reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
					}
				}
				else {
					parse_in->ptr = ptr_hold;
					parse_index_a(parse_out, target, 0, PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
				}

				if (parse_in->line[parse_in->index].line[parse_in->ptr] != ')') {
					parse_s2B(parse_out, target, target->reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
				}
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
				subset_depth[0]--;
				UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, zero		// \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
				parse_in->ptr++;
				reg_table->entry_x[offset].in_use = 0;
			}
			else {
				debug++;// syntax
			}
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != ')' && parse_in->line[parse_in->index].line[parse_in->ptr] != ';') {
				if (getOperatorB(&op_match, parse_in, op_table)) {
					getWord(parse_in);
					INT64 number;
					char index;
					VariableListEntry* current2 = compiler_var->list_base->next;
					if (get_integer(&number, parse_in)) {
						print_op_immediate2(parse_out, op_match, target->reg_index, l_control->for_loop[l_control->index].index_reg, number, op_table, PC, reg_table, l_control->depth, parse_in,param, Masm);
					}
					else if (find_loop_reg_index(l_control, parse_in->word)) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // acc %s= %s \n", PC[0],
							op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[l_control->for_loop[l_control->index].index_reg].name,
							op_table[op_match].symbol, l_control->for_loop[l_control->index].index_name); PC[0] += 4;
					}
					else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
						reg_table->entry_x[offset].in_use = 0;
					}
					else if (get_ArgumentListEntry(&current2, &index, parse_in->word, compiler_var->f_list_base->last->argument, compiler_var->f_list_base->last->argc)) {
						if (current2->pointer) {
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[')
								parse_in->ptr++;
							else
								debug++;
							getWord(parse_in);
							if (get_integer(&number, parse_in)) {
								VariableListEntry addr;
								addVariableEntry(&addr, int64_enum, compiler_var->f_list_base->last->argument[index].name, reg_table, l_control, parse_out, parse_in, Masm);
								VariableListEntry temp;
								addVariableEntry(&temp, compiler_var->f_list_base->last->argument[index].type, compiler_var->f_list_base->last->argument[index].name, reg_table, l_control, parse_out, parse_in, Masm);
								memory_load(parse_out, &addr, 11, &addr, index, PC, reg_table, parse_in, l_control->depth, param, Masm);
								memory_load(parse_out, &temp, addr.reg_index, &temp, number, PC, reg_table, parse_in, l_control->depth,param,  Masm);
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // acc %s= %s[%d] \n", PC[0],
									op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[temp.reg_index].name,
									op_table[op_match].symbol, compiler_var->f_list_base->last->argument[index].name, number); PC[0] += 4;
								reg_table->entry_x[addr.reg_index].in_use = 0;
								reg_table->entry_x[temp.reg_index].in_use = 0;
							}
							else {
								debug++;
							}
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']')
								parse_in->ptr++;
							else
								debug++;
						}
						else {
							debug++;
							/*
							VariableListEntry temp;
							addVariableEntry(&temp, int64_enum, strcat(compiler_var->f_list_base->last->argument[index].name, " value(base64)"), reg_table, l_control, parse_out, parse_in, Masm);
							memory_load(parse_out, &temp, 11, &temp, index, PC, reg_table, parse_in, l_control->depth, param, Masm);
							// NOTE: if fp, need to move value to fp reg
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // acc %s= %s\n", PC[0],
								op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[temp.reg_index].name,
								op_table[op_match].symbol, strcat(compiler_var->f_list_base->last->argument[index].name, " value(base64)")); PC[0] += 4;
							reg_table->entry_x[temp.reg_index].in_use = 0;
							/**/
						}
					}
					else {
						debug++;
					}
				}
				else if (getBranchB(&branch_match, parse_in, branch_table)) {
					getWordB(parse_in);
					INT64 number;
					VariableListEntry* current2 = compiler_var->list_base->next;
					if (get_integer(&number, parse_in->word)) {
						UINT n2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
						if (number == 0) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, zero // predication comparison\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[offset].name); PC[0] += 4;
						}
						else {
							load_64bB(parse_out, PC, n2, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // predication comparison\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[offset].name, reg_table->entry_x[n2].name); PC[0] += 4;
						}
						reg_table->entry_x[offset].in_use = 0;
						//					UINT s1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						UINT s2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						VariableListEntry s1;
						s1.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						s1.reg_index_valid = 1;
						sprintf_s(s1.name, "");
						s1.type = int64_enum;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '?') {
							subset_depth[0]--;
							parse_in->ptr += 2;
							UINT8 num_valid = 0;
							parse_rh_of_eqB(parse_out, &s1, 1, &num_valid, &number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
							num_valid = 0;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ':') {
								parse_in->ptr++;
								parse_rh_of_eqB(parse_out, &s1, 1, &num_valid, &number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
								num_valid = 0;
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 64 // pos = 0, neg = -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
						switch (branch_match) {
						case 0:// <
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition true\n", PC[0], reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition false\n", PC[0], reg_table->entry_x[s2].name, reg_table->entry_x[s2].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // predicated result\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s2].name); PC[0] += 4;
							break;
						case 1:// >
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition false\n", PC[0], reg_table->entry_x[s2].name, reg_table->entry_x[s2].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition true\n", PC[0], reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // predicated result\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s2].name); PC[0] += 4;
							break;
						default:
							debug++;
							break;
						}
						reg_table->entry_x[n2].in_use = 0;
						reg_table->entry_x[s1.reg_index].in_use = 0;
						reg_table->entry_x[s2].in_use = 0;
						s1.reg_index_valid = 0;
					}
					else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
						UINT8 offset2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset2, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name, reg_table->entry_x[offset2].name); PC[0] += 4;
						reg_table->entry_x[offset].in_use = 0;
						reg_table->entry_x[offset2].in_use = 0;
					}
					else {
						debug++;
					}

				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
					int ptr_hold = parse_in->ptr;
					parse_in->ptr++;
					getWord(parse_in);
					INT64 number;
					if (get_integer(&number, parse_in->word)) {
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
							UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
							if (current->atomic) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lr.d.aq.rl %s, 0x%03x(%s)\n", PC[0], reg_table->entry_x[target->reg_index].name, number, reg_table->entry_x[offset].name); PC[0] += 4;//current->name,
							}
							else {
								memory_load(parse_out, target, offset, target, number, PC, reg_table, parse_in, l_control->depth, param, Masm);
							}
							parse_in->ptr++;
							reg_table->entry_x[offset].in_use = 0;
						}
						else {
							parse_s2B(parse_out, target, target->reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
						}
					}
					else {
						parse_in->ptr = ptr_hold;
						parse_index_a(parse_out, target, 0, PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
					}

					if (parse_in->line[parse_in->index].line[parse_in->ptr] != ')') {
						parse_s2B(parse_out, target, target->reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
					}
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
					subset_depth[0]--;
					UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, zero		// \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
					parse_in->ptr++;
					reg_table->entry_x[offset].in_use = 0;
				}
				else {
					debug++;// syntax
				}
			}
		}
		else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
			UINT8 op_match, branch_match;
			UINT8 s1 = current->reg_index;
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != ')' && parse_in->line[parse_in->index].line[parse_in->ptr] != ';') {
				if (getOperatorB(&op_match, parse_in, op_table)) {
					UINT8 neg_flag = 0;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '-') {
						neg_flag = 1;
						parse_in->ptr++;
					}
					getWordB(parse_in);
					INT64 number;
					VariableListEntry* current2 = compiler_var->list_base->next;
					if (parse_in->word[0] == '\0') {
						if (neg_flag)
							debug++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
							VariableListEntry target2;
							addVariableEntry(&target2, target->type, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
							parse_subset(parse_out, &target2, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
							if (target->type == fp16_enum) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s	// accumulate result\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[target2.reg_index].name); PC[0] += 4;
								reg_table->entry_fp[target2.reg_index].in_use = 0;
							}
							else {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s	// accumulate result\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target2.reg_index].name); PC[0] += 4;
								reg_table->entry_x[target2.reg_index].in_use = 0;
							}
						}
						else {
							debug++;
						}
					}
					else if (get_integer(&number, parse_in)) {
						if (current->pointer) {
							UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, PC, addr, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
							if (neg_flag) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, zero, %s	// negative number handling (hack)\n", PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name); PC[0] += 4;
							}
							print_op_immediate2(parse_out, op_match, target->reg_index, addr, number, op_table, PC, reg_table, l_control->depth, parse_in, param, Masm);
							reg_table->entry_x[addr].in_use = 0;
						}
						else {
							if (neg_flag) {
								print_op_immediate2(parse_out, op_match, target->reg_index, s1, -number, op_table, PC, reg_table, l_control->depth, parse_in,param,  Masm);
							}
							else {
								if (target->type == fp16_enum) {
									UINT8 target2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									UINT8 target2f = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%03x	// negative number handling (hack)\n", PC[0], reg_table->entry_x[target2].name, number); PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.h.x %s, %s	// negative number handling (hack)\n", PC[0], reg_table->entry_fp[target2f].name, reg_table->entry_x[target2].name); PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[s1].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[target2f].name); PC[0] += 4;
									reg_table->entry_x[target2].in_use = 0;
									reg_table->entry_fp[target2f].in_use = 0;
								}
								else {
									print_op_immediate2(parse_out, op_match, target->reg_index, s1, number, op_table, PC, reg_table, l_control->depth, parse_in,param,  Masm);
								}
							}
						}
					}
					else if (get_float(&fp_data, parse_in->word)) {
						if (current->pointer) {
							debug++;
						}
						else {
							UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
							UINT index0 = (fp_data.hp & 0x0fff);
							UINT64 carry = (fp_data.hp & 0x800) << 1;
							UINT index1 = (((fp_data.hp + carry) >> 12) & 0x0fffff);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // start 16b load\n", PC[0], reg_table->entry_x[s2i].name, index1); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %#05x	// completed load of 0x%08x as 16b immediate load\n", PC[0], reg_table->entry_x[s2i].name, reg_table->entry_x[s2i].name, index0); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[s2].name, reg_table->entry_x[s2i].name); PC[0] += 4;
							reg_table->entry_x[s2i].in_use = 0;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[target->reg_index].name); PC[0] += 4;
							reg_table->entry_fp[s2].in_use = 0;
						}
					}
					else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
						if (neg_flag)
							debug++;
						if (current2->reg_index_valid) {
							if (current2->pointer) {
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[')
									parse_in->ptr++;
								else
									debug++;
								getWordSign(parse_in);
								if (get_integer(&number, parse_in)) {
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']')
										parse_in->ptr++;
									else
										debug++;
									if (current2->type == fp16_enum) {
										UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x flh %s,0x%03x(%s) // pull value of \"%s\" of the stack\n", 
											PC[0], reg_table->entry_fp[s2].name, -((number + 1) << 1), reg_table->entry_x[current2->reg_index].name, current2->name); PC[0] += 4;
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[s2].name); PC[0] += 4;
										reg_table->entry_fp[s2].in_use = 0;
									}
									else {
										if (target->type == fp16_enum) {
											UINT8 target_i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s,0x%03x(%s) // pull value of \"%s\" of the stack\n", 
												PC[0], reg_table->entry_x[target_i].name, -((number + 1) << 3), reg_table->entry_x[current2->reg_index].name, current2->name); PC[0] += 4;
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.x.h %s, %s\n", PC[0], reg_table->entry_fp[target->reg_index].name, reg_table->entry_x[target_i].name); PC[0] += 4;
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[target->reg_index].name); PC[0] += 4;
										}
										else {
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s,0x%03x(%s) // pull value of \"%s\" of the stack\n", 
												PC[0], reg_table->entry_x[target->reg_index].name, -((number + 1) << 3), reg_table->entry_x[current2->reg_index].name, current2->name); PC[0] += 4;
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[target->reg_index].name); PC[0] += 4;
										}
									}
									s1 = target->reg_index;
								}
								else {
									debug++;
								}
							}
							else {
								if (target->type == fp16_enum) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name); PC[0] += 4;
								}
								else {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current2->reg_index].name); PC[0] += 4;
								}
							}
						}
						else {
							UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, PC, offset, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
							reg_table->entry_x[offset].in_use = 0;
						}
					}
					else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->global_list_base)) {// global_list_base
						if (neg_flag)
							debug++;
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
						reg_table->entry_x[offset].in_use = 0;
					}
					else {
						if (neg_flag)
							debug++;
						UINT hit = 0;
						for (UINT i = 0; i < compiler_var->f_list_base->last->argc && !hit; i++) {
							if (strcmp(parse_in->word, compiler_var->f_list_base->last->argument[i].name) == 0) {
								hit = 1;
								UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s,0x%03x(a1) // pull value of \"%s\" of the stack\n", 
									PC[0], reg_table->entry_x[offset].name, -(int)((i + 1) << 3), compiler_var->f_list_base->last->argument[i].name); PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
								reg_table->entry_x[offset].in_use = 0;
							}
						}
						if (!hit) {
							UINT8 num_valid;
							UINT8 target2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x nop // ERROR: dropping neg current reg index, must yield invalid result\n", PC[0]); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target2].name); PC[0] += 4;
							reg_table->entry_x[target2].in_use = 0;
						}
					}
				}
				else if (getBranchB(&branch_match, parse_in, branch_table)) {
					getWord(parse_in);
					INT64 number;
					VariableListEntry* current2 = compiler_var->list_base->next;
					if (get_integer(&number, parse_in->word)) {
						UINT n2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
						if (number == 0) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, zero // predication comparison\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[offset].name); PC[0] += 4;
						}
						else {
							load_64bB(parse_out, PC, n2, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // predication comparison\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[offset].name, reg_table->entry_x[n2].name); PC[0] += 4;
						}
						reg_table->entry_x[offset].in_use = 0;
						UINT s2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						VariableListEntry s1;
						s1.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						s1.reg_index_valid = 1;
						sprintf_s(s1.name, "");
						s1.type = int64_enum;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '?') {
							subset_depth[0]--;
							parse_in->ptr += 2;
							UINT8 num_valid = 0;
							parse_rh_of_eqB(parse_out, &s1, 1, &num_valid, &number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
							num_valid = 0;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ':') {
								parse_in->ptr++;
								parse_rh_of_eqB(parse_out, &s1, 1, &num_valid, &number, PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
								num_valid = 0;
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 64 // pos = 0, neg = -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
						switch (branch_match) {
						case 0:// <
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition true\n", PC[0], reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition false\n", PC[0], reg_table->entry_x[s2].name, reg_table->entry_x[s2].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // predicated result\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s2].name); PC[0] += 4;
							break;
						case 1:// >
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition false\n", PC[0], reg_table->entry_x[s2].name, reg_table->entry_x[s2].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1\n", PC[0], reg_table->entry_x[n2].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s // ? condition true\n", PC[0], reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[n2].name); PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // predicated result\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[s2].name); PC[0] += 4;
							break;
						default:
							debug++;
							break;
						}
						reg_table->entry_x[n2].in_use = 0;
						reg_table->entry_x[s1.reg_index].in_use = 0;
						reg_table->entry_x[s2].in_use = 0;
						s1.reg_index_valid = 0;
					}
					else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
						UINT8 offset2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset2, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name, reg_table->entry_x[offset2].name); PC[0] += 4;
						reg_table->entry_x[offset].in_use = 0;
						reg_table->entry_x[offset2].in_use = 0;
					}
					else {
						debug++;
					}

				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
					int ptr_hold = parse_in->ptr;
					parse_in->ptr++;
					getWordB(parse_in);
					INT64 number;
					if (get_integer(&number, parse_in->word)) {
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
							if (number < 0x800) {
								if (target->type == fp16_enum) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x flh %s, 0x%03x(%s)// load value for \"%s\"\n", PC[0], reg_table->entry_fp[target->reg_index].name, -2 * (number + 1), reg_table->entry_x[current->reg_index].name, current->name); PC[0] += 4;//current->name,
								}
								else {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x%03x(%s)// load value for \"%s\"\n", PC[0], reg_table->entry_x[target->reg_index].name, -8 * (number + 1), reg_table->entry_x[current->reg_index].name, current->name); PC[0] += 4;//current->name,
								}
							}
							else {// warning, this appears to have an error, current addr not valid??
								UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
								if (current->atomic) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lr.d.aq.rl %s, 0x%03x(%s)\n", PC[0], reg_table->entry_x[target->reg_index].name, number, reg_table->entry_x[offset].name); PC[0] += 4;//current->name,
								}
								else {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x%03x(%s)\n", PC[0], reg_table->entry_x[target->reg_index].name, -8 * (number + 1), reg_table->entry_x[offset].name); PC[0] += 4;//current->name,
								}
								reg_table->entry_x[offset].in_use = 0;
							}
							parse_in->ptr++;
						}
						else {
							parse_s2B(parse_out, target, target->reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
						}
					}
					else {
						parse_in->ptr = ptr_hold;
						parse_index_a(parse_out, target, 0, PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
					}

					if (parse_in->line[parse_in->index].line[parse_in->ptr] != ')') {
						parse_s2B(parse_out, target, target->reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
					}
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
					subset_depth[0]--;
					UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, zero		// \n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name); PC[0] += 4;
					parse_in->ptr++;
					reg_table->entry_x[offset].in_use = 0;
				}
				else {
					debug++;// syntax
				}
			}
		}
		else if (get_integer(&number0, parse_in->word)) {
			UINT8 op_match;
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
				UINT8 index0 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				load_64bB(parse_out, PC, index0, number0, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);// error, dropped data
				VariableListEntry s1;
				addVariableEntry(&s1, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
				parse_subset(parse_out, &s1, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
				if (parse_in->line[parse_in->index].line[parse_in->ptr] != ';')
					parse_s2B(parse_out, target, s1.reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
				reg_table->entry_x[s1.reg_index].in_use = 0;
				reg_table->entry_x[index0].in_use = 0;
			}
			else if (getOperatorB(&op_match, parse_in, op_table)) {
				UINT8 index0 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				load_64bB(parse_out, PC, index0, number0, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
				getWordB(parse_in);
				INT64 number;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
					VariableListEntry s1;
					addVariableEntry(&s1, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
					parse_subset(parse_out, &s1, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
					if (parse_in->line[parse_in->index].line[parse_in->ptr] != ';')
						parse_s2B(parse_out, target, s1.reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
//					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[index0].name); PC[0] += 4;
					sprint_op_2src(parse_out, PC, op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[s1.reg_index].name, reg_table->entry_x[index0].name, param);
				}
				else if (get_integer(&number, parse_in->word)) {
					if (index0 > 0x1f)
						debug++;
					print_op_immediate2(parse_out, op_match, target->reg_index, index0, number, op_table, PC, reg_table, l_control->depth, parse_in,param,  Masm);
				}
				else {
					debug++;// not coded
				}
				reg_table->entry_x[index0].in_use = 0;
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
				subset_depth[0]--;

				load_64bB(parse_out, PC, target->reg_index, number0, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
			}
			else {
				debug++;// syntax
			}
		}
		else if (get_float(&fp_data, parse_in->word)) {
			UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			UINT8 s2i2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
			UINT index0 = (fp_data.hp & 0x0fff);
			UINT64 carry = (fp_data.hp & 0x800) << 1;
			UINT index1 = (((fp_data.hp + carry) >> 12) & 0x0fffff);
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // start 16b load\n", PC[0], reg_table->entry_x[s2i2].name, index1); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %#05x	// completed load of 0x%08x as 16b immediate load\n", PC[0], reg_table->entry_x[s2i].name, reg_table->entry_x[s2i2].name, index0); PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", PC[0], reg_table->entry_fp[s2].name, reg_table->entry_x[s2i].name); PC[0] += 4;
			reg_table->entry_x[s2i2].in_use = 0;
			reg_table->entry_x[s2i].in_use = 0;
			UINT8 op_match;
			if (getOperatorB(&op_match, parse_in, op_table)) {
				getWordB(parse_in);
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
					VariableListEntry s1;
					addVariableEntry(&s1, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
					parse_subset(parse_out, &s1, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s2].name, reg_table->entry_fp[s1.reg_index].name); PC[0] += 4;
					reg_table->entry_fp[s1.reg_index].in_use = 0;
					s1.reg_index_valid = 0;
				}
				else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
					if (current->pointer) {
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
							parse_in->ptr++;
							getWordB(parse_in);
							if (get_integer(&number0, parse_in->word)) {

							}
							else {
								debug++;
							}
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
								parse_in->ptr++;
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
						UINT8 s1 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x flh %s, %d(%s)\n", PC[0], reg_table->entry_fp[s1].name, -((1 + number0) * 2), reg_table->entry_fp[current->reg_index].name); PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h, %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s2].name, reg_table->entry_fp[s1].name); PC[0] += 4;
						reg_table->entry_fp[s1].in_use = 0;
					}
					else {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h, %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s2].name, reg_table->entry_fp[current->reg_index].name); PC[0] += 4;
					}
				}
				else {
					debug++;
				}
			}
			else {
				debug++;
			}
			reg_table->entry_fp[s2].in_use = 0;
		}
		else if (get_reg_indexB(&index, parse_in->word, reg_table)) {
			UINT8 op_match;
			if (getOperatorB(&op_match, parse_in, op_table)) {
				getWordB(parse_in);
				INT64 number;
				VariableListEntry* current2 = compiler_var->list_base->next;
				if (get_integer(&number, parse_in->word)) {
					print_op_immediate2(parse_out, op_match, target->reg_index, index, number, op_table, PC, reg_table, l_control->depth, parse_in,param, Masm);
				}
				else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
					UINT8 offset2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, PC, offset2, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[index].name, reg_table->entry_x[offset2].name); PC[0] += 4;
					reg_table->entry_x[offset2].in_use = 0;
				}
				else {
					debug++;
				}
			}
		}
		else {
			if (check_funct_arg(parse_out, target, 0, parse_in, PC, reg_table, l_control, current, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, param, Masm, unit_debug)) {
				if (current->pointer == 1) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s) // pull variable value out of array\n", PC[0], reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name); PC[0] += 4;
				}
				while (parse_in->line[parse_in->index].line[parse_in->ptr] != ')' && parse_in->line[parse_in->index].line[parse_in->ptr] != ';') {
					subset_depth[0]--;

					if (parse_in->index == 140)
						debug++;
					UINT8 op_match;
					if (getOperatorB(&op_match, parse_in, op_table)) {
						getWordB(parse_in);
						INT64 number;
						VariableListEntry* current2 = compiler_var->list_base->next;
						if (parse_in->word[0]!='\0') {
							VariableListEntry target2;
							addVariableEntry(&target2, int64_enum, parse_in->word, reg_table, l_control, parse_out, parse_in, Masm);
							if (get_integer(&number, parse_in->word)) {
								print_op_immediate2(parse_out, op_match, target->reg_index, target->reg_index, number, op_table, PC, reg_table, l_control->depth, parse_in,param, Masm);
							}
							else if (find_loop_reg_index(l_control, parse_in->word)) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s	// %s loop index \"%s\" into result\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name,
									reg_table->entry_x[target->reg_index].name, reg_table->entry_x[l_control->for_loop[l_control->index].index_reg].name, op_table[op_match].opcode, parse_in->word); PC[0] += 4;
							}
							else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
								if (current2->pointer) {
									VariableListEntry offset;
									addVariableEntry(&offset, int64_enum, (char *) "offset", reg_table, l_control, parse_out, parse_in, Masm);
									UINT8 number_valid = 0;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
										//										parse_in->ptr++ ;
										parse_index_b(parse_out, &offset, &number, &number_valid, PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
									}
									else {
										debug++;
									}
									if (current2->reg_index_valid == 0) {
										current2->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
										load_64bB(parse_out, PC, current2->reg_index, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
										current2->reg_index_valid = 1;
										current2->reg_depth = l_control->depth;
									}
									if (current2->type == fp16_enum) {
										UINT8 s2 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
										if (number_valid) {
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x flh %s, %d(%s)\n", PC[0], reg_table->entry_fp[s2].name, -(1 + number) * 2, reg_table->entry_x[current2->reg_index].name); PC[0] += 4;
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[s2].name); PC[0] += 4;
										}
										else {
											debug++;
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[current2->reg_index].name); PC[0] += 4;
										}
									}
									else {
										debug++;
									}
									reg_table->entry_x[offset.reg_index].in_use = 0;
								}
								else {
									if (current2->reg_index_valid == 0) {
										current2->reg_index = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
										load_64bB(parse_out, PC, current2->reg_index, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
										current2->reg_index_valid = 1;
										current2->reg_depth = l_control->depth;
									}
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[current2->reg_index].name); PC[0] += 4;
								}
							}
							else if (check_funct_arg(parse_out, &target2, 0, parse_in, PC, reg_table, l_control, current, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, param, Masm, unit_debug)) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s	\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name,
									reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target2.reg_index].name, op_table[op_match].opcode, parse_in->word); PC[0] += 4;
							}
							else {// int or float, need to check target_type
								VariableListEntry target2;
								addVariableEntry(&target2, target->type, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
								if (target->type == fp16_enum) {
									if (check_funct_arg(parse_out, &target2, 0, parse_in, PC, reg_table, l_control, current, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, param, Masm, unit_debug)) {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[target->reg_index].name, reg_table->entry_fp[target2.reg_index].name); PC[0] += 4;
									}
									else {
										debug++;
									}
									reg_table->entry_fp[target2.reg_index].in_use = 0;
								}
								else {
									if (check_funct_arg(parse_out, &target2, 0, parse_in, PC, reg_table, l_control, current, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count,param, Masm, unit_debug)) {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target2.reg_index].name); PC[0] += 4;
									}
									else {
										debug++;
									}
									reg_table->entry_x[target2.reg_index].in_use = 0;
								}
							}
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
							VariableListEntry target2;
							addVariableEntry(&target2, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
							parse_subset(parse_out, &target2, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[target2.reg_index].name); PC[0] += 4;
							reg_table->entry_x[target2.reg_index].in_use = 0;
						}
						else {
							debug++;
						}
					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
						parse_subset(parse_out, target, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
					}
					else {
						debug++;
					}
				}
			}
			else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->global_list_base)) {// global_list_base
				UINT8 gv = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				if (current->reg_index_valid = 0) {
					current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, PC, current->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
					current->reg_index_valid = 1;
					current->reg_depth = l_control->depth;
				}
	//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s,0(%s) // load global variable into register \n", PC[0], reg_table->entry_x[gv].name, reg_table->entry_x[current->reg_index].name); PC[0] += 4;
				sprint_load(parse_out, PC, (char*)"ld", reg_table->entry_x[gv].name, (UINT16)0, reg_table->entry_x[current->reg_index].name, (char*)"load global variable into register", param);
				UINT8 op_match;
				if (getOperatorB(&op_match, parse_in, op_table)) {
					getWordB(parse_in);
					INT64 number;
					VariableListEntry* current2 = compiler_var->list_base->next;
					if (get_integer(&number, parse_in->word)) {
						if (gv > 0x1f)
							debug++;
						print_op_immediate2(parse_out, op_match, target->reg_index, gv, number, op_table, PC, reg_table, l_control->depth, parse_in,param, Masm);
					}
					else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
						UINT8 offset2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, PC, offset2, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", PC[0], op_table[op_match].opcode, reg_table->entry_x[target->reg_index].name, reg_table->entry_x[offset].name, reg_table->entry_x[offset2].name); PC[0] += 4;
						reg_table->entry_x[offset].in_use = 0;
						reg_table->entry_x[offset2].in_use = 0;
					}
					else {
						debug++;
					}
					parse_s2B(parse_out, target, target->reg_index, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, PC, reg_table, l_control, param, Masm, unit_debug);
				}
				else {
					debug++;
				}
			}
			else {
				debug++;
			}
		}
	}
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
		subset_depth[0]--;
		parse_in->ptr++;
	}
	else if (parse_in->line[parse_in->index].line[parse_in->ptr] != ';' && !cast_input) {
		debug++;
		exit(0);
	}
}// parse_subset
void check_if_cast(parse_struct2* parse_out, parse_struct2* parse_in, VariableListEntry* target, INT8* subset_depth, loop_control* l_control, INT64* logical_PC, IO_list_type* IO_list, reg_table_struct* reg_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, operator_type* op_table, compiler_var_type* compiler_var, param_type* param, FILE* Masm, UINT8 unit_debug) {
	int debug = 0;
	UINT hold = parse_in->ptr;
	parse_in->ptr++;
	getWord(parse_in);

	if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
		parse_in->ptr++;
		data_type_enum type;
		if (type_decode(&type, parse_in->word)) {
			getWord(parse_in);
			VariableListEntry* test;
			char index;
			if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
				cast_variable(parse_out, parse_in, test, target, l_control, logical_PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
			}
			else if (get_ArgumentListEntry(&test, &index, parse_in->word, compiler_var->f_list_base->last->argument, compiler_var->f_list_base->last->argc)) {
				cast_variable(parse_out, parse_in, test, target, l_control, logical_PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
			}
			else {
				debug++;
			}
		}
		else {
			debug++;
		}
	}
	else {
		parse_in->ptr = hold;
		parse_subset(parse_out, target, subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, logical_PC, reg_table, l_control, param, Masm, unit_debug);
	}
}
void get_modifierB(UINT8* modifier_index, parse_struct2* parse, var_type_struct*modifier_type) {
	for (UINT k = 0; k < 2; k++) {
		if (strcmp(parse->word, modifier_type[k].name) == 0) {
			modifier_index[0] = k;
			getWordB(parse);
			k = 0x10;
		}
	}
}
/**/
void for_inner_loopB(parse_struct2* parse_out, parse_struct2* parse_in, parse_struct2* parse_in2, UINT init_number, UINT number, INT64* logical_PC, VariableListEntry* test, var_type_struct* control_type, var_type_struct*modifier_type,
	reg_table_struct* reg_table, memory_space_type* memory_space, hw_pointers* pointers, INT64 base, compiler_var_type* compiler_var, UINT branch_match, operator_type* branch_table, operator_type* op_table, 
	var_type_struct* csr_list, UINT8 csr_list_count, loop_control* l_control, short* sp_offset, FunctionListEntry* current_function, IO_list_type* IO_list, param_type* param, FILE* Masm, UINT8 unit_debug) {
	
	UINT debug = 0;
	VariableListEntry* current;
	if ((parse_in2->index - parse_in->index < 3)) {
		parse_struct2 parse_in3;
		parse_in3.line = parse_in->line;
		parse_in3.ptr = 0;
		for (parse_in3.index = parse_in->index + 1; parse_in3.index < parse_in2->index; parse_in3.index++) {
			sprintf_s(parse_out->line[parse_out->index++].line, "//\t%d\t%s", parse_in3.index, parse_in->line[parse_in3.index].line);
			//			ptr3 = 0;
			UINT ptr3 = 0;
			while (parse_in->line[parse_in3.index].line[ptr3] == ' ' || parse_in->line[parse_in3.index].line[ptr3] == '\t') ptr3++;
			getWord(parse_in, parse_in3.index, &ptr3);
			if (get_VariableListEntryB(&current, parse_in->word, compiler_var->global_list_base)) {
				if (current->pointer) {
					if (parse_in->line[parse_in3.index].line[ptr3] != '[')
						debug++;
					ptr3++;
					getWord(parse_in, parse_in3.index, &ptr3);
					if (parse_in->line[parse_in3.index].line[ptr3] == ']') {
						ptr3++;
						VariableListEntry* index;
						if (get_VariableListEntryB(&index, parse_in->word, compiler_var->list_base)) {
							while (parse_in->line[parse_in3.index].line[ptr3] == ' ' || parse_in->line[parse_in3.index].line[ptr3] == '\t') ptr3++;
							if (parse_in->line[parse_in3.index].line[ptr3] == '=') {
								ptr3++;
								getWord(parse_in, parse_in3.index, &ptr3);
								INT64 number_b;
								if (get_integer(&number_b, parse_in->word)) {
									UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									load_64bB(parse_out, logical_PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
									if (number_b == 0) {
										for (UINT i = init_number; i < number; i++) {
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd zero, %#x(%s)\n", logical_PC[0], i * 8, reg_table->entry_x[offset].name); logical_PC[0] += 4;
											l_control->fence[l_control->depth] = 1;
										}
									}
									else {
										UINT8 t2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
										load_64bB(parse_out, logical_PC, t2, number_b, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
										for (UINT i = init_number; i < number; i++) {
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, %#x(%s)\n", logical_PC[0], reg_table->entry_x[t2].name, i * 8, reg_table->entry_x[offset].name); logical_PC[0] += 4;
											l_control->fence[l_control->depth] = 1;
										}
										reg_table->entry_x[t2].in_use = 0;
									}
									reg_table->entry_x[offset].in_use = 0;
								}
								else {
									debug++;
								}
							}
						}
						else {
							debug++;
						}
					}
					else {
						VariableListEntry index;
						addVariableEntry(&index, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
						parse_index_a(parse_out, &index, 0, logical_PC, current, &parse_in3, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						reg_table->entry_x[index.reg_index].in_use = 0;
					}
				}
				else {
					debug++;
				}
			}
			else {
				{

				}
			}
		}
	}
	else {
		parse_struct2 parse_in3;
		parse_in3.line = parse_in->line;
		parse_in3.ptr = 0;
		for (parse_in3.index = parse_in->index + 1; parse_in3.index < parse_in2->index && l_control->depth > 0; parse_in3.index++) {
			clean_temp_reg(reg_table);
			sprintf_s(parse_out->line[parse_out->index++].line, "// %d %s", parse_in3.index, parse_in->line[parse_in3.index].line);
			UINT ptr3 = 0;
			getWord(parse_in, parse_in3.index, &ptr3);
			ParseFunctionCode(parse_out, &parse_in3, l_control, logical_PC, control_type, modifier_type, branch_table, csr_list, csr_list_count, op_table,
				reg_table, memory_space, pointers, base, compiler_var, sp_offset, current_function, IO_list, param, Masm, unit_debug);
		}
		debug++;
	}
}
void process_if_variable3(parse_struct2* parse_out, parse_struct2* parse_in, INT64* logical_PC, UINT8 temp1, reg_table_struct* reg_table, compiler_var_type* compiler_var, IO_list_type* IO_list, 
	operator_type* branch_table, operator_type* op_table, var_type_struct* csr_list, UINT8 csr_list_count, int control_depth, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug) {
	UINT debug = 0;
	UINT8 op_match;
	UINT8 branch_match;
	if (getOperatorB(&op_match, parse_in, op_table)) {
		INT64 number;
		getWord(parse_in);
		if (get_integer(&number, parse_in->word)) {
			UINT8 t1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			UINT8 t2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			load_64bB(parse_out, logical_PC, t2, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // accumulate the result\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[t1].name, reg_table->entry_x[temp1].name, reg_table->entry_x[t2].name); logical_PC[0] += 4;
			reg_table->entry_x[t2].in_use = 0;
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '{') {// need to preserve saved valuesNT8, 
				//				subset_depth[0]--;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x beq %s, zero, label%d_%d \n", logical_PC[0], reg_table->entry_x[t1].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
			}
			else {
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {

					parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
					if (getBranchB(&branch_match, parse_in, branch_table)) {
						getWord(parse_in);
						UINT8 t2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						if (get_integer(&number, parse_in->word)) {
							load_64bB(parse_out, logical_PC, t2, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
						}
						else {
							debug++;
						}
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
							parse_in->ptr++;
						else
							debug++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
							switch (branch_match) {
							case 4: // equal
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x bne %s, %s, label%d_%d // accumulate the result\n", logical_PC[0], reg_table->entry_x[t1].name, reg_table->entry_x[t2].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
								break;
							default:
								debug++;
								break;
							}
						}
						else {
							UINT8 r1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							UINT8 r2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							switch (branch_match) {
							case 1: // >
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // b-a (a>b)\n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[t2].name, reg_table->entry_x[t1].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srai %s, %s, 63 // true = -1, false = 0 \n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[r1].name); logical_PC[0] += 4;
								break;
							case 4: // equal
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // a-b\n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[t1].name, reg_table->entry_x[t2].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // b-a\n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[t2].name, reg_table->entry_x[t1].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // \n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[r2].name, reg_table->entry_x[r1].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srai %s ,%s, 63 // b==a -> 0; b!=a ->-1 \n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[r1].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1 // true = -1, false = 0 \n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[r1].name); logical_PC[0] += 4;
								break;
							default:
								debug++;
								break;
							}
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
							if (getOperatorB(&op_match, parse_in, op_table)) {
								getWord(parse_in);
								if (get_integer(&number, parse_in->word)) {
									UINT8 t3 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									UINT8 t4 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									load_64bB(parse_out, logical_PC, t4, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // accumulate the result\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[t3].name, reg_table->entry_x[temp1].name, reg_table->entry_x[t4].name); logical_PC[0] += 4;
									reg_table->entry_x[t4].in_use = 0;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '{') {// need to preserve saved valuesNT8, 
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x beq %s, zero, // label%d_%d \n", logical_PC[0], reg_table->entry_x[t3].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
									}
									else {
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
											parse_in->ptr++;
											while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
											getWord(parse_in);
											UINT8 t2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
											if (get_integer(&number, parse_in->word)) {
												load_64bB(parse_out, logical_PC, t2, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
											}
											else {
												debug++;
											}
											if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
												parse_in->ptr++;
											else
												debug++;
											if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
												switch (branch_match) {
												case 4: // equal
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x bne %s, %s, label%d_%d // accumulate the result\n", logical_PC[0], reg_table->entry_x[t1].name, reg_table->entry_x[t2].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
													break;
												default:
													debug++;
													break;
												}
											}
											else {
												debug++;
											}
										}
									}
								}
								else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
									parse_in->ptr++;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
										parse_in->ptr++;
										getWord(parse_in);
										VariableListEntry* current2 = compiler_var->list_base->next;
										if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
											UINT8 op_match2;
											if (getOperatorB(&op_match2, parse_in, op_table)) {
												getWord(parse_in);
												if (get_integer(&number, parse_in->word)) {
													UINT8 t3 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
													UINT8 t4 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
													UINT8 r3 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
													load_64bB(parse_out, logical_PC, t4, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
													if (current2->reg_index_valid) {
														sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // accumulate the result\n", logical_PC[0], op_table[op_match2].opcode, reg_table->entry_x[t3].name, reg_table->entry_x[current2->reg_index].name, reg_table->entry_x[t4].name); logical_PC[0] += 4;
													}
													else {
														debug++;
													}
													if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')parse_in->ptr++;
													while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
													if (getBranchB(&branch_match, parse_in, branch_table)) {
														getWord(parse_in);
														load_64bB(parse_out, logical_PC, t4, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
														switch (branch_match) {
														case 4: // ==
															sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // a-b (a==b)\n", logical_PC[0], reg_table->entry_x[r3].name, reg_table->entry_x[t3].name, reg_table->entry_x[t4].name); logical_PC[0] += 4;
															sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // b-a\n", logical_PC[0], reg_table->entry_x[r2].name, reg_table->entry_x[t4].name, reg_table->entry_x[t3].name); logical_PC[0] += 4;
															sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // \n", logical_PC[0], reg_table->entry_x[r3].name, reg_table->entry_x[r2].name, reg_table->entry_x[r3].name); logical_PC[0] += 4;
															sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 63 // b==a -> 0; b!=a ->-1 \n", logical_PC[0], reg_table->entry_x[r3].name, reg_table->entry_x[r3].name); logical_PC[0] += 4;
															sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1 // true = -1, false = 0 \n", logical_PC[0], reg_table->entry_x[r3].name, reg_table->entry_x[r3].name); logical_PC[0] += 4;
															break;
														default:
															debug++;
															break;
														}
													}
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // accumulate the result\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[r1].name, reg_table->entry_x[r3].name, reg_table->entry_x[r1].name); logical_PC[0] += 4;
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, -1 // load compare value\n", logical_PC[0], reg_table->entry_x[r3].name); logical_PC[0] += 4;
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x bne %s, %s, label%d_%d // \n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[r3].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
													reg_table->entry_x[t4].in_use = 0;
													if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
														parse_in->ptr++;
													else
														debug++;
													if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
														parse_in->ptr++;
													else
														debug++;
												}
											}
											else {
												debug++;
											}
										}
									}
									else {
										debug++;
									}
								}
								else {
									debug++;
								}
							}
							else {
								debug++;
							}
							reg_table->entry_x[r1].in_use = 0;
							reg_table->entry_x[r2].in_use = 0;
						}
						reg_table->entry_x[t2].in_use = 0;
					}
					else {
						debug++;
					}
				}
				else {
					debug++;
				}
			}
			reg_table->entry_x[t1].in_use = 0;
		}
		else {
			debug++;
		}
	}
	else if (getBranchB(&branch_match, parse_in, branch_table)) {
		getWord(parse_in);
		INT64 num = 0;
		if (get_integer(&num, parse_in->word)) {
			if (num == 0) {
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, zero, label_near%d_%d \n",
					logical_PC[0], branch_table[branch_match].opcode, reg_table->entry_x[temp1].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;

				UINT8 s1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label%d_%d \n", logical_PC[0], control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x nop 0x%02x // make space if >20b jump\n", logical_PC[0], s1); logical_PC[0] += 4;
				reg_table->entry_x[s1].in_use = 0;

				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_near%d_%d \n", logical_PC[0], control_depth, l_control->count[control_depth]);
			}
			else {
				UINT8 c2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, %#05x\n", logical_PC[0], reg_table->entry_x[c2].name, num); logical_PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, label%d_%d \n", logical_PC[0], branch_table[branch_match].opcode, reg_table->entry_x[temp1].name, reg_table->entry_x[c2].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
				reg_table->entry_x[c2].in_use = 0;
			}
		}
	}
	else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x beq %s, zero, label%d_%d \n", logical_PC[0], reg_table->entry_x[temp1].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
	}
	else {
		debug++;//syntax
	}
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')parse_in->ptr++;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
		l_control->depth++;
		sprintf_s(parse_out->line[parse_out->index++].line, "// current depth = %d.%d // plus 1 \n", l_control->depth, l_control->count[l_control->depth]);
	}
}

void process_if_variable2(parse_struct2* parse_out, parse_struct2* parse_in, INT64* logical_PC, VariableListEntry* test, reg_table_struct* reg_table, compiler_var_type* compiler_var, IO_list_type* IO_list, operator_type* branch_table, operator_type* op_table, 
	var_type_struct* csr_list, UINT8 csr_list_count, int control_depth, loop_control* l_control, param_type* param, FILE* Masm, UINT8 unit_debug) {
	UINT debug = 0;
	UINT8 op_match;
	UINT8 branch_match;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
		VariableListEntry index;
		addVariableEntry(&index, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
		parse_index_a(parse_out, &index, 0, logical_PC, test, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -8(%s) // load %s\n", logical_PC[0], reg_table->entry_x[test->reg_index].name, reg_table->entry_x[index.reg_index].name, test->name); logical_PC[0] += 4;
		reg_table->entry_x[index.reg_index].in_use = 0;

		if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x beq %s, zero, label%d_%d \n", logical_PC[0], reg_table->entry_x[test->reg_index].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
			parse_in->ptr += 2;
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
			getWord(parse_in);
			INT64 number;
			UINT8 csr_index;
			if (get_integer(&number, parse_in->word)) {
				if (number == 0) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x bne %s, zero, label%d_%d \n", logical_PC[0], reg_table->entry_x[test->reg_index].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
				}
				else {
					UINT8 t2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, logical_PC, t2, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x bne %s, %s, label%d_%d \n", logical_PC[0], reg_table->entry_x[test->reg_index].name, reg_table->entry_x[t2].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
					reg_table->entry_x[t2].in_use = 0;
				}
			}
			else if (match_listB(&csr_index, parse_in->word, csr_list, csr_list_count, logical_PC)) {
				UINT8 t2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrc %s, zero, %s\n", logical_PC[0], reg_table->entry_x[t2].name, csr_list[csr_index]); logical_PC[0] += 4;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ' ' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == '1') {
					parse_in->ptr += 3;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1 \n", logical_PC[0], reg_table->entry_x[t2].name, reg_table->entry_x[t2].name); logical_PC[0] += 4;
				}
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x bne %s, %s, label%d_%d \n", logical_PC[0], reg_table->entry_x[test->reg_index].name, reg_table->entry_x[t2].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
				reg_table->entry_x[t2].in_use = 0;
			}
			else {
				debug++;
			}
		}
		else {
			debug++;
		}
		//	reg_table->entry_x[index].in_use = 0;
	}
	else if (getOperatorB(&op_match, parse_in, op_table)) {
		INT64 number;
		getWord(parse_in);
		if (get_integer(&number, parse_in->word)) {
			UINT8 t1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			if (number < 0x800 && number > -0x800) {
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %si %s, %s, 0x%02x // accumulate the result\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[t1].name, reg_table->entry_x[test->reg_index].name, number); logical_PC[0] += 4;
			}
			else {
				UINT8 t2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				load_64bB(parse_out, logical_PC, t2, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // accumulate the result\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[t1].name, reg_table->entry_x[test->reg_index].name, reg_table->entry_x[t2].name); logical_PC[0] += 4;
				reg_table->entry_x[t2].in_use = 0;
			}
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '{') {// need to preserve saved valuesNT8, 
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x beq %s, zero, label%d_%d \n", logical_PC[0], reg_table->entry_x[t1].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
			}
			else {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
					parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
					if (getBranchB(&branch_match, parse_in, branch_table)) {
						getWord(parse_in);
						UINT8 t2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						if (get_integer(&number, parse_in->word)) {
							load_64bB(parse_out, logical_PC, t2, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
						}
						else {
							debug++;
						}
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
							parse_in->ptr++;
						else
							debug++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
							switch (branch_match) {
							case 4: // equal
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x bne %s, %s, label%d_%d // accumulate the result\n", logical_PC[0], reg_table->entry_x[t1].name, reg_table->entry_x[t2].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
								break;
							default:
								debug++;
								break;
							}
						}
						else {
							UINT8 r1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							UINT8 r2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							switch (branch_match) {
							case 1: // >
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // b-a (a>b)\n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[t2].name, reg_table->entry_x[t1].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srai %s, %s, 63 // true = -1, false = 0 \n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[r1].name); logical_PC[0] += 4;
								break;
							case 4: // equal
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // a-b\n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[t1].name, reg_table->entry_x[t2].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // b-a\n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[t2].name, reg_table->entry_x[t1].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // \n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[r2].name, reg_table->entry_x[r1].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srai %s ,%s, 63 // b==a -> 0; b!=a ->-1 \n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[r1].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1 // true = -1, false = 0 \n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[r1].name); logical_PC[0] += 4;
								break;
							default:
								debug++;
								break;
							}
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
							if (getOperatorB(&op_match, parse_in, op_table)) {
								getWord(parse_in);
								if (get_integer(&number, parse_in->word)) {
									UINT8 t3 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									UINT8 t4 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									load_64bB(parse_out, logical_PC, t4, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
									UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									load_64bB(parse_out, logical_PC, offset, test->addr, test->name, reg_table, l_control->depth, parse_in, param, Masm);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // accumulate the result\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[t3].name, reg_table->entry_x[offset].name, reg_table->entry_x[t4].name); logical_PC[0] += 4;
									reg_table->entry_x[offset].in_use = 0;
									reg_table->entry_x[t4].in_use = 0;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '{') {// need to preserve saved valuesNT8, 
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x beq %s, zero, label%d_%d \n", logical_PC[0], reg_table->entry_x[t3].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
									}
									else {
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
											parse_in->ptr++;
											while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
											getWord(parse_in);
											UINT8 t2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
											if (get_integer(&number, parse_in->word)) {
												load_64bB(parse_out, logical_PC, t2, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
											}
											else {
												debug++;
											}
											if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
												parse_in->ptr++;
											else
												debug++;
											if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
												switch (branch_match) {
												case 4: // equal
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x bne %s, %s, label%d_%d // accumulate the result\n", logical_PC[0], reg_table->entry_x[t1].name, reg_table->entry_x[t2].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
													break;
												default:
													debug++;
													break;
												}
											}
											else {
												debug++;
											}
										}
									}
								}
								else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
									parse_in->ptr++;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
										parse_in->ptr++;
										getWord(parse_in);
										VariableListEntry* current2 = compiler_var->list_base->next;
										if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
											UINT8 op_match2;
											if (getOperatorB(&op_match2, parse_in, op_table)) {
												getWord(parse_in);
												if (get_integer(&number, parse_in->word)) {
													UINT8 t3 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
													UINT8 t4 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
													UINT8 r3 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
													load_64bB(parse_out, logical_PC, t4, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
													if (current2->reg_index_valid) {
														sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // accumulate the result\n", logical_PC[0], op_table[op_match2].opcode, reg_table->entry_x[t3].name, reg_table->entry_x[current2->reg_index].name, reg_table->entry_x[t4].name); logical_PC[0] += 4;
													}
													else if (current2->pointer == 0) {
														current2->reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
													}
													else {
														debug++;
													}
													if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')parse_in->ptr++;
													while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
													if (getBranchB(&branch_match, parse_in, branch_table)) {
														getWord(parse_in);
														load_64bB(parse_out, logical_PC, t4, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
														switch (branch_match) {
														case 4: // ==
															sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // a-b (a==b)\n", logical_PC[0], reg_table->entry_x[r3].name, reg_table->entry_x[t3].name, reg_table->entry_x[t4].name); logical_PC[0] += 4;
															sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // b-a\n", logical_PC[0], reg_table->entry_x[r2].name, reg_table->entry_x[t4].name, reg_table->entry_x[t3].name); logical_PC[0] += 4;
															sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // \n", logical_PC[0], reg_table->entry_x[r3].name, reg_table->entry_x[r2].name, reg_table->entry_x[r3].name); logical_PC[0] += 4;
															sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 63 // b==a -> 0; b!=a ->-1 \n", logical_PC[0], reg_table->entry_x[r3].name, reg_table->entry_x[r3].name); logical_PC[0] += 4;
															sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1 // true = -1, false = 0 \n", logical_PC[0], reg_table->entry_x[r3].name, reg_table->entry_x[r3].name); logical_PC[0] += 4;
															break;
														default:
															debug++;
															break;
														}
													}
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // accumulate the result\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[r1].name, reg_table->entry_x[r3].name, reg_table->entry_x[r1].name); logical_PC[0] += 4;
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, -1 // load compare value\n", logical_PC[0], reg_table->entry_x[r3].name); logical_PC[0] += 4;
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x bne %s, %s, label%d_%d // \n", logical_PC[0], reg_table->entry_x[r1].name, reg_table->entry_x[r3].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
													reg_table->entry_x[t3].in_use = 0;
													reg_table->entry_x[t4].in_use = 0;
													reg_table->entry_x[r3].in_use = 0;
													reg_table->entry_x[current2->reg_index].in_use = 0;
													if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
														parse_in->ptr++;
													else
														debug++;
													if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
														parse_in->ptr++;
													else
														debug++;
												}
											}
											else {
												debug++;
											}
										}
									}
									else {
										debug++;
									}
								}
								else {
									debug++;
								}
							}
							else {
								debug++;
							}
							reg_table->entry_x[r1].in_use = 0;
							reg_table->entry_x[r2].in_use = 0;
						}
						reg_table->entry_x[t2].in_use = 0;
					}
					else {
						debug++;
					}
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
					parse_in->ptr += 2;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
					getWord(parse_in);
					if (get_integer(&number, parse_in->word)) {
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
							UINT8 t3 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, t3, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x bne %s, %s, label%d_%d \n", logical_PC[0], reg_table->entry_x[t1].name, reg_table->entry_x[t3].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
							reg_table->entry_x[t3].in_use = 0;
							parse_in->ptr++;
						}
						else {
							debug++;
						}
					}
				}
				else {
					debug++;
				}
			}
			reg_table->entry_x[t1].in_use = 0;
		}
		else {
			debug++;
		}
	}
	else if (getBranchB(&branch_match, parse_in, branch_table)) {
		getWord(parse_in);
		INT64 num = 0;
		if (get_integer(&num, parse_in->word)) {
			if (num == 0) {
				//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, zero, label%d_%d \n", logical_PC[0], branch_table[branch_match].opcode, reg_table->entry_x[test->reg_index].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, zero, label_near%d_%d \n", logical_PC[0], branch_table[branch_match].rev_opcode, reg_table->entry_x[test->reg_index].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;

				UINT8 s1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label%d_%d \n", logical_PC[0], control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x nop 0x%02x // make space if >20b jump\n", logical_PC[0], s1); logical_PC[0] += 4;
				reg_table->entry_x[s1].in_use = 0;

				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_near%d_%d \n", logical_PC[0], control_depth, l_control->count[control_depth]);
			}
			else {
				UINT8 c2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, %#05x\n", logical_PC[0], reg_table->entry_x[c2].name, num); logical_PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, label%d_%d \n", logical_PC[0], branch_table[branch_match].opcode, reg_table->entry_x[test->reg_index].name, reg_table->entry_x[c2].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
				reg_table->entry_x[c2].in_use = 0;
			}
		}
		else {
			debug++;
		}
	}
	else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
		UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
		load_64bB(parse_out, logical_PC, offset, test->addr, test->name, reg_table, l_control->depth, parse_in, param, Masm);
		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x beq %s, zero, label%d_%d \n", logical_PC[0], reg_table->entry_x[offset].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
		reg_table->entry_x[offset].in_use = 0;
	}
	else {
		debug++;//syntax
	}
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')parse_in->ptr++;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
		l_control->depth++;
		sprintf_s(parse_out->line[parse_out->index++].line, "// current depth = %d.%d // plus 1 \n", l_control->depth, l_control->count[l_control->depth]);
	}
}
void reg_maintanence(reg_table_struct* reg_table, compiler_var_type* compiler_var, loop_control* l_control, parse_struct2* parse_out, INT64* logical_PC, parse_struct2* parse_in, param_type* param) {
	int debug = 0;
	for (UINT8 i = 5; i < 0x20; i++) {
		if (reg_table->entry_x[i].temp == 1) {
			reg_table->entry_x[i].temp = 0;
			reg_table->entry_x[i].in_use = 0;
		}
		else if (reg_table->entry_x[i].saved == 1 && reg_table->entry_x[i].saved_depth > l_control->depth) {
			reg_table->entry_x[i].saved = 0;
			reg_table->entry_x[i].saved_depth = 0;
			reg_table->entry_x[i].in_use = 0;
		}
	}
	for (UINT8 i = 0; i < 0x20; i++) {
		if (reg_table->entry_fp[i].saved_depth > l_control->depth) {
			reg_table->entry_fp[i].saved = 0;
			reg_table->entry_fp[i].saved_depth = 0;
			reg_table->entry_fp[i].in_use = 0;
		}
	}
	for (VariableListEntry* current = compiler_var->list_base->next; current != compiler_var->list_base; current = current->next) {
		if (current->depth > l_control->depth && current->pointer) {
			current->reg_index_valid = 0;
			if (current->pointer == 1 && current->size > 0) {
				UINT size = current->size;
				if (current->type == fp16_enum)
					size *= 2;
				else
					debug++;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi sp, sp, 0x%03x	// array \"%s\" adjust\n", logical_PC[0], size, current->name); logical_PC[0] += 4;
				current->size = 0;
				current->depth = 0;
			}
		}
		if (current->depth > l_control->depth) {
			current->last->next = current->next;
			current->next->last = current->last;
			VariableListEntry* goner = current;
			current = current->last;
			free(goner);
		}
	}
	for (VariableListEntry* current = compiler_var->global_list_base->next; current != compiler_var->global_list_base; current = current->next) {
		if (compiler_var->global_list_base->reg_depth > l_control->depth) {
			compiler_var->global_list_base->reg_index_valid = 0;
		}
		if (compiler_var->global_list_base->depth > l_control->depth) {
			compiler_var->global_list_base->reg_index_valid = 0;
			compiler_var->global_list_base->depth = 0;
		}
	}
	for (UINT i = 0; i < compiler_var->f_list_base->last->argc; i++) {
		if (compiler_var->f_list_base->last->argument[i].reg_index_valid == 1) {
			if (compiler_var->f_list_base->last->argument[i].reg_depth > l_control->depth) {
				compiler_var->f_list_base->last->argument[i].reg_index_valid = 0;
			}
			if (reg_table->entry_x[compiler_var->f_list_base->last->argument[i].reg_index].in_use == 0) {
				compiler_var->f_list_base->last->argument[i].reg_index = 0;
				compiler_var->f_list_base->last->argument[i].reg_index_valid = 0;
			}
		}
	}
	if (l_control->fence[l_control->depth] == 1) {
		l_control->fence[l_control->depth] = 0;
	}
	if (l_control->depth == 0) {
	//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fence 0 // order all memory operations 00\n", logical_PC[0]); logical_PC[0] += 4;
		sprintf_string(parse_out, logical_PC,(char*) " fence 0 // order all memory operations 00\n", param);
	}
}
void parse_complex_condition(UINT8 condition, int* depth, parse_struct2* parse_out, parse_struct2* parse_in, reg_table_struct* reg_table, INT64* logical_PC, loop_control* l_control, compiler_var_type* compiler_var, param_type* param, FILE* Masm) {
	UINT debug = 0;
	UINT8 leave = 0;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0' && !leave) {
		switch (parse_in->line[parse_in->index].line[parse_in->ptr]) {
		case ' ':
		case '\t':
			break;
		case '(':
			depth[0]++;
			parse_in->ptr++;
			parse_complex_condition(condition, &l_control->depth, parse_out, parse_in, reg_table, logical_PC, l_control, compiler_var, param, Masm);
			parse_in->ptr--;
			break;
		case ')':
			depth[0]--;
			leave = 1;
			break;
		case '{':
			l_control->depth++;
			break;
		case '}':
			l_control->depth--;
			reg_maintanence(reg_table, compiler_var, l_control, parse_out, logical_PC, parse_in,param);
			break;
		case '&':
			if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '&') {
				parse_in->ptr += 2;
				UINT8 c2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				parse_complex_condition(c2, &l_control->depth, parse_out, parse_in, reg_table, logical_PC, l_control, compiler_var, param, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s\n", logical_PC[0], reg_table->entry_x[condition].name, reg_table->entry_x[condition].name, reg_table->entry_x[c2].name); logical_PC[0] += 4;
				parse_in->ptr--;
			}
			else {
				INT64 num = 0;
				parse_in->ptr++;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				getWord(parse_in);
				if (get_integer(&num, parse_in->word)) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 0x%03x\n", logical_PC[0], reg_table->entry_x[condition].name, reg_table->entry_x[condition].name, num); logical_PC[0] += 4;
				}
				else {
					debug++;
				}
				parse_in->ptr--;
			}
			break;
		case '>':
			if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '>') {
				debug++;
			}
			else {
				INT64 num = 0;
				parse_in->ptr++;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				getWord(parse_in);
				if (get_integer(&num, parse_in->word)) {
					if (num == 0) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sltu %s, zero, %s	// > 0 \n", logical_PC[0], reg_table->entry_x[condition].name, reg_table->entry_x[condition].name); logical_PC[0] += 4;
					}
					else {
						UINT8 t3 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, %#05x\n", logical_PC[0], reg_table->entry_x[t3].name, num); logical_PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sltu %s, %s, %s	// > num\n", logical_PC[0], reg_table->entry_x[condition].name, reg_table->entry_x[t3].name, reg_table->entry_x[condition].name); logical_PC[0] += 4;
					}
				}
				else {
					debug++;
				}
				parse_in->ptr--;
			}
			break;
		case '=':
			if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
				INT64 num = 0;
				parse_in->ptr += 2;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				getWord(parse_in);
				if (get_integer(&num, parse_in->word)) {
					if (num == 0) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sltu %s, zero, %s	// == 0\n", logical_PC[0], reg_table->entry_x[condition].name, reg_table->entry_x[condition].name); logical_PC[0] += 4;
					}
					else {
						UINT8 t3 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s,zero, %#05x\n", logical_PC[0], reg_table->entry_x[t3].name, num); logical_PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sltu %s, %s, %s	// == num\n", logical_PC[0], reg_table->entry_x[condition].name, reg_table->entry_x[t3].name, reg_table->entry_x[condition].name); logical_PC[0] += 4;
					}
				}
				else {
					debug++;
				}
				parse_in->ptr--;
			}
			else {
				debug++;
			}
			break;
		default:
			getWord(parse_in);
			VariableListEntry* test;
			if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '&') {
					parse_in->ptr++;
				}
				else {
					debug++;
				}
				getWord(parse_in);
				INT64 num = 0;
				if (get_integer(&num, parse_in->word)) {
					UINT8 t3 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, logical_PC, offset, test->addr, test->name, reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, %#05x\n", logical_PC[0], reg_table->entry_x[t3].name, reg_table->entry_x[offset].name, num); logical_PC[0] += 4;
					reg_table->entry_x[offset].in_use = 0;
				}
				else {
					debug++;
				}
				parse_in->ptr--;
			}
			else if (parse_in->word[0]=='\0') {
				// skip charcter
			}
			else {
				debug++;
			}
			break;
		}
		parse_in->ptr++;
	}
}
void inner_loop_prep(parse_struct2* parse_in, parse_struct2* parse_in2, INT64* logical_PC, VariableListEntry* current, loop_control* l_control, reg_table_struct* reg_table, compiler_var_type* compiler_var, parse_struct2* parse_out, param_type* param, FILE* Masm) {
	UINT8 stop = 0;
	loop_control l_control2;

	l_control2.depth = l_control->depth;
	l_control2.count[l_control2.depth] = l_control->count[l_control->depth];
	for (; parse_in2->index < parse_in->index && !stop; parse_in2->index++) {
		parse_in2->ptr = 0;
		while (parse_in->line[parse_in2->index].line[parse_in2->ptr] == ' ' || parse_in->line[parse_in2->index].line[parse_in2->ptr] == '\t') parse_in2->ptr++;
		getWord(parse_in2);
		if (get_VariableListEntryB(&current, parse_in->word, compiler_var->global_list_base)) {
			UINT8 addr = alloc_temp_UABI(reg_table, l_control2.depth, parse_out, parse_in, Masm);
			load_64bB(parse_out, logical_PC, addr, current->addr, current->name, reg_table, l_control2.depth, parse_in, param, Masm);
			reg_table->entry_x[addr].in_use = 0;
		}
		while (!(parse_in->line[parse_in2->index].line[parse_in2->ptr] == '\0' || parse_in->line[parse_in2->index].line[parse_in2->ptr] == '{' ||
			parse_in->line[parse_in2->index].line[parse_in2->ptr] == '}' || parse_in->line[parse_in2->index].line[parse_in2->ptr] == '/')) parse_in2->ptr++;
		if (parse_in->line[parse_in2->index].line[parse_in2->ptr] == '/' && parse_in->line[parse_in2->index].line[parse_in2->ptr + 1] == '/') {
			while (!(parse_in->line[parse_in2->index].line[parse_in2->ptr] == '\0')) parse_in2->ptr++;
		}
		if (parse_in->line[parse_in2->index].line[parse_in2->ptr] == '{')
			l_control2.depth++;
		if (parse_in->line[parse_in2->index].line[parse_in2->ptr] == '}') {
			if (l_control2.depth == l_control->depth)
				stop = 1;
			else
				l_control2.depth--;
		}
	}
	parse_in2->index--;
}
void variableDeclarationBody(parse_struct2* parse_out, parse_struct2* parse_in, loop_control* l_control, INT64* logical_PC, UINT8 modifier_index, IO_list_type* IO_list, reg_table_struct* reg_table, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, operator_type* op_table, compiler_var_type* compiler_var, hw_pointers* pointers, param_type* param, FILE* Masm, UINT8 unit_debug) {
	int debug = 0;
	VariableListEntry* current = compiler_var->list_base->last;
	current->atomic = (modifier_index == 1) ? 1 : 0;
	current->sp_offset_valid = 0;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
		current->pointer = 1;
		parse_in->ptr++;
		getWord(parse_in);
		INT64 num = 0;
		if (get_integer(&num, parse_in->word)) {
			current->size = num;
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ';') {
				switch (current->type) {
				case int8_enum:
				case uint8_enum:
					pointers->sp = pointers->sp - (1 * num);  //?? how do we apply this ??
					current->sp_offset = num * 1;
					current->sp_offset_valid = 1;
					break;
				case int16_enum:
				case uint16_enum:
				case fp16_enum:
					pointers->sp = pointers->sp - (2 * num);  //?? how do we apply this ??
					current->sp_offset = num * 2;
					current->sp_offset_valid = 1;
					break;
				case int64_enum:
				case uint64_enum:
				case fp64_enum:
					pointers->sp = pointers->sp - (8 * num);  //?? how do we apply this ??
					current->sp_offset = num * 8;
					current->sp_offset_valid = 1;
					break;
				default:
					debug++;
					break;
				}
				sprintf_s(parse_out->line[parse_out->index++].line, "// %s sp offset = 0x%016I64x, in stack order (sp stack)\n", current->name, current->sp_offset);
				current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				current->reg_index_valid = 1;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, sp, 0x000x // \"%s\"array base address \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, current->name); logical_PC[0] += 4;
				if (current->sp_offset < 0)
					debug++;
				else if (current->sp_offset < 0x800) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi sp, sp, 0x%03x // update stack pointer after pushing array declaration onto stack \n", logical_PC[0], -current->sp_offset); logical_PC[0] += 4;
				}
				else {
					UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, logical_PC, offset, current->sp_offset, (char *) "offset", reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub sp, sp, %s // update stack pointer after pushing array declaration onto stack \n", logical_PC[0], reg_table->entry_x[offset].name); logical_PC[0] += 4;
					reg_table->entry_x[offset].in_use = 0;
				}
				parse_in->ptr++;
				for (VariableListEntry* current2 = compiler_var->list_base->next; current2 != compiler_var->list_base && current2 != current; current2 = current2->next) {
					if (current2->pointer) {
						current2->sp_offset += current->sp_offset;
					}
				}
			}
			else {
				debug++;
			}
		}
		else {
			debug++;
		}
	}
	else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {// initial value
		int hold = parse_in->ptr;
		parse_in->ptr++;
		FunctionListEntry* function_match = compiler_var->f_list_base->next;
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
			parse_in->ptr++;
			getWord(parse_in);
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
				parse_in->ptr++;
				data_type_enum type;
				if (type_decode(&type, parse_in->word)) {
					getWord(parse_in);
					VariableListEntry* test;
					char index;
					if (current->reg_index_valid != 1) {
						if (current->pointer) {
							current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						}
						else {
							if (current->type == fp16_enum)
								current->reg_index = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
							else
								current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						}
						current->reg_index_valid = 1;
						current->reg_depth = l_control->depth;
					}

					if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
						cast_variable(parse_out, parse_in, test, current, l_control, logical_PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
					}
					else if (get_ArgumentListEntry(&test, &index, parse_in->word, compiler_var->f_list_base->last->argument, compiler_var->f_list_base->last->argc)) {
						if (test->reg_index_valid == 0) {
							test->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							test->reg_index_valid = 1;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x%03x(a1) // %s\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, -((index + 1) << 3), test->name); logical_PC[0] += 4;
						}
						cast_variable(parse_out, parse_in, test, current, l_control, logical_PC, IO_list, reg_table, branch_table, csr_list, csr_list_count, op_table, compiler_var, param, Masm, unit_debug);
					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
						INT8 subset_depth = 0;
						parse_subset(parse_out, test, &subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, logical_PC, reg_table, l_control, param, Masm, unit_debug);
					}
					else {
						debug++;
					}
				}
				else {
					debug++;
				}
			}
			else if (get_VariableTypeB(parse_in, compiler_var->list_base)) {
				getWord(parse_in);
				if (strcmp(parse_in->word, "malloc") == 0) {
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == '(' || parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
					getWord(parse_in);
					INT64 num = 0;
					UINT64 addr = 0;
					if (get_integer(&num, parse_in->word)) {
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
							parse_in->ptr++;
							getWord(parse_in);
							INT64 number;
							get_integer(&number, parse_in->word);
							num *= number;
						}
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: %s \n", pointers->gp + compiler_var->gp_offset - 8 * num, current->name);
						compiler_var->gp_offset -= (8 * num);
						if (8 * num < 0x1000) {
							debug++;
						}
						else {
							addr = pointers->gp + compiler_var->gp_offset;// ERROR: need gp offset for compiler usage
							current->addr = addr;
							UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add gp, zero, %s // malloc; set gp to array base address \n", logical_PC[0], reg_table->entry_x[offset].name); logical_PC[0] += 4;
							reg_table->entry_x[offset].in_use = 0;
						}
						debug++;
					}
				}
				else {
					parse_in->ptr = hold;
				}
			}
			else {
				parse_in->ptr = hold;
			}
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '&') {
			parse_in->ptr++;
			getWord(parse_in);
			UINT8 op_match;
			VariableListEntry* test;
			if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
				if (getOperatorB(&op_match, parse_in, op_table)) {
					getWord(parse_in);
					INT64 number;
					if (get_integer(&number, parse_in->word)) {
						if (test->pointer == 0)
							debug++;
						if (test->reg_index_valid != 1)
							debug++;
						if (current->pointer == 1) {
							if (current->reg_index_valid == 1)
								debug++;
							current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							current->reg_index_valid = 1;
						}
						UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, logical_PC, offset, number, (char*)"pointer offset", reg_table, l_control->depth, parse_in, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[test->reg_index].name, reg_table->entry_x[offset].name); logical_PC[0] += 4;
					}
					else {
						debug++;
					}
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
					VariableListEntry* base;
					current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					current->reg_index_valid = 1;
					current->reg_depth = l_control->depth;
					if (get_VariableListEntryB(&base, parse_in->word, compiler_var->list_base)) {
						if (base->sp_offset < 0x800) {
							parse_in->ptr++;
							getWord(parse_in);
							INT64 number;
							if (get_integer(&number, parse_in->word)) {
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
									parse_in->ptr++;
									if (base->type == fp16_enum) {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, sp, 0x%03x // %s address\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, base->sp_offset - number * 2, current->name); logical_PC[0] += 4;
									}
									else {
										debug++;
									}
								}
								else {
									debug++;
								}
							}
							else {
								debug++;
							}
						}
						else {
							if (base->reg_index_valid == 0) {
								base->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								load_64bB(parse_out, logical_PC, base->reg_index, base->sp_offset, base->name, reg_table, l_control->depth, parse_in, param, Masm);

								UINT8 number_valid = 0;
								INT64 number;
								VariableListEntry offset;
								addVariableEntry(&offset, int64_enum, (char *) "offset", reg_table, l_control, parse_out, parse_in, Masm);
								parse_index_b(parse_out, &offset, &number, &number_valid, logical_PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
								if (number_valid && number == 0) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, sp // %s address\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[base->reg_index].name, current->name); logical_PC[0] += 4;
								}
								else {
									if (base->type == fp16_enum) {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 1 // 16 bit entry size\n", logical_PC[0], reg_table->entry_x[offset.reg_index].name, reg_table->entry_x[offset.reg_index].name, number); logical_PC[0] += 4;
									}
									else {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // assume 64 bit entry size\n", logical_PC[0], reg_table->entry_x[offset.reg_index].name, reg_table->entry_x[offset.reg_index].name, number); logical_PC[0] += 4;
									}
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // total sp offset for variable\n", logical_PC[0], reg_table->entry_x[offset.reg_index].name, reg_table->entry_x[base->reg_index].name, reg_table->entry_x[offset.reg_index].name); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, sp // %s address\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset.reg_index].name, current->name); logical_PC[0] += 4;
								}
								reg_table->entry_x[base->reg_index].in_use = 0;
								reg_table->entry_x[offset.reg_index].in_use = 0;
							}
							else {
								UINT8 number_valid = 0;
								INT64 number;
								VariableListEntry offset;
								addVariableEntry(&offset, int64_enum, (char *) "offset", reg_table, l_control, parse_out, parse_in, Masm);
								parse_index_b(parse_out, &offset, &number, &number_valid, logical_PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
								if (number_valid && number == 0) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, zero // %s address = %s address + 0 \n", logical_PC[0],
										reg_table->entry_x[current->reg_index].name, reg_table->entry_x[base->reg_index].name, current->name, base->name); logical_PC[0] += 4;
								}
								else {
									if (base->type == fp16_enum) {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 1 // 16 bit entry size\n", logical_PC[0], reg_table->entry_x[offset.reg_index].name, reg_table->entry_x[offset.reg_index].name, number); logical_PC[0] += 4;
									}
									else {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // assume 64 bit entry size\n", logical_PC[0], reg_table->entry_x[offset.reg_index].name, reg_table->entry_x[offset.reg_index].name, number); logical_PC[0] += 4;
									}
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s // %s address = %s address + offset \n", logical_PC[0],
										reg_table->entry_x[current->reg_index].name, reg_table->entry_x[base->reg_index].name, reg_table->entry_x[offset.reg_index].name, current->name, base->name); logical_PC[0] += 4;
								}
								reg_table->entry_x[offset.reg_index].in_use = 0;
							}
						}
					}
					else if (get_VariableListEntryB(&base, parse_in->word, compiler_var->global_list_base)) {
						UINT8 base_offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, logical_PC, base_offset, base->addr, base->name, reg_table, l_control->depth, parse_in, param, Masm);
						UINT8 number_valid = 0;
						INT64 number;
						VariableListEntry offset;
						addVariableEntry(&offset, int64_enum, (char *) "offset", reg_table, l_control, parse_out, parse_in, Masm);
						parse_index_b(parse_out, &offset, &number, &number_valid, logical_PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						if (number_valid) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%x \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[base_offset].name, -number); logical_PC[0] += 4;
						}
						else {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[base_offset].name, reg_table->entry_x[offset.reg_index].name, number); logical_PC[0] += 4;
						}
						reg_table->entry_x[offset.reg_index].in_use = 0;
						reg_table->entry_x[base_offset].in_use = 0;
					}
					else if (check_funct_arg(parse_out, current, 1, parse_in, logical_PC, reg_table, l_control, current, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, param, Masm, unit_debug)) {
						// purposely left blank
					}
					else {
						debug++;
					}
				}
				else {
					debug++;
				}
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ')' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == ';') {

				int count = 0;
				UINT8 func_ct = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				UINT8 temp = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				UINT8 src = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -8(gp) \n", logical_PC[0], reg_table->entry_x[func_ct].name); logical_PC[0] += 4;
				int ref_length = strlen(parse_in->word);
				int offset = 0;
				UINT8 test_src = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				UINT8 test_ref = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, -1 \n", logical_PC[0], reg_table->entry_x[temp].name); logical_PC[0] += 4;
				if (ref_length < 8) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_function_stack \n", logical_PC[0]);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1 \n", logical_PC[0], reg_table->entry_x[temp].name, reg_table->entry_x[temp].name); count++; logical_PC[0] += 4;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x beq %s, %s, label_no_match \n", logical_PC[0], reg_table->entry_x[func_ct].name, reg_table->entry_x[temp].name); logical_PC[0] += 4;
					offset += 0x40;
					UINT index = 0;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -%d(gp) // load name\n", logical_PC[0], reg_table->entry_x[src].name, offset); logical_PC[0] += 4;

					while (index <= ref_length) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 0xff // isolate letter\n", logical_PC[0], reg_table->entry_x[test_src].name, reg_table->entry_x[src].name); logical_PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 8 // shift to next ASCII letter\n", logical_PC[0], reg_table->entry_x[src].name, reg_table->entry_x[src].name); logical_PC[0] += 4;

						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ori %s, zero, %#x // load match letter\n", logical_PC[0], reg_table->entry_x[test_ref].name, parse_in->word[index++]); logical_PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x bne %s, %s, label_function_stack // go to next word if not a match\n", logical_PC[0], reg_table->entry_x[test_ref].name, reg_table->entry_x[test_src].name); logical_PC[0] += 4;
					}

					UINT8 offset2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, logical_PC, offset2, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -%d(gp) \n", logical_PC[0], reg_table->entry_x[offset2].name, offset + 8); logical_PC[0] += 4;
					reg_table->entry_x[offset2].in_use = 0;

				}
				else {
					debug++;
				}
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_no_match \n", logical_PC[0]);
				reg_table->entry_x[func_ct].in_use = 0;
				reg_table->entry_x[src].in_use = 0;
				reg_table->entry_x[temp].in_use = 0;
				reg_table->entry_x[test_src].in_use = 0;
				reg_table->entry_x[test_ref].in_use = 0;
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
				VariableListEntry* base;
				current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				current->reg_index_valid = 1;
				current->reg_depth = l_control->depth;
				if (get_VariableListEntryB(&base, parse_in->word, compiler_var->list_base)) {
					if (base->reg_index_valid == 0) {
						base->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, logical_PC, base->reg_index, base->addr, base->name, reg_table, l_control->depth, parse_in, param, Masm);
						base->reg_index_valid = 1;
						base->reg_depth = l_control->depth;
					}
					UINT8 number_valid = 0;
					INT64 number;
					VariableListEntry offset;
					addVariableEntry(&offset, int64_enum, (char *) "offset", reg_table, l_control, parse_out, parse_in, Masm);
					parse_index_b(parse_out, &offset, &number, &number_valid, logical_PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
					if (number_valid) {
						if (number < 0x800 && number >= -0x800) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%x \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[base->reg_index].name, number); logical_PC[0] += 4;
						}
						else {
							UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, offset, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %s \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[base->reg_index].name, reg_table->entry_x[offset].name); logical_PC[0] += 4;
							reg_table->entry_x[offset].in_use = 0;
						}
					}
					else {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // assume 64 bit entry size\n", logical_PC[0], reg_table->entry_x[offset.reg_index].name, reg_table->entry_x[offset.reg_index].name, number); logical_PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[base->reg_index].name, reg_table->entry_x[offset.reg_index].name, number); logical_PC[0] += 4;
					}
					reg_table->entry_x[offset.reg_index].in_use = 0;
				}
				else if (get_VariableListEntryB(&base, parse_in->word, compiler_var->global_list_base)) {
					UINT8 base_offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, logical_PC, base_offset, base->addr, base->name, reg_table, l_control->depth, parse_in, param, Masm);
					UINT8 number_valid = 0;
					INT64 number;
					VariableListEntry offset;
					addVariableEntry(&offset, int64_enum, (char *) "offset", reg_table, l_control, parse_out, parse_in, Masm);
					parse_index_b(parse_out, &offset, &number, &number_valid, logical_PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
					if (number_valid) {
						switch (base->type) {
						case int8_enum:
						case uint8_enum:
							break;
						case int16_enum:
						case uint16_enum:
						case fp16_enum:
							number *= 2;
							//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 1 // adjust index to 16b \n", logical_PC[0], reg_table->entry_x[base_offset].name, reg_table->entry_x[base_offset].name); logical_PC[0] += 4;
							break;
						case int32_enum:
						case uint32_enum:
						case fp32_enum:
							number *= 4;
							//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 2 // adjust index to 32b \n", logical_PC[0], reg_table->entry_x[base_offset].name, reg_table->entry_x[base_offset].name); logical_PC[0] += 4;
							break;
						case int64_enum:
						case uint64_enum:
						case fp64_enum:
							number *= 8;
							//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // adjust index to 64b \n", logical_PC[0], reg_table->entry_x[base_offset].name, reg_table->entry_x[base_offset].name); logical_PC[0] += 4;
							break;
						case int128_enum:
						case uint128_enum:
						case fp128_enum:
							number *= 16;
							//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 4 // adjust index to 128b \n", logical_PC[0], reg_table->entry_x[base_offset].name, reg_table->entry_x[base_offset].name); logical_PC[0] += 4;
							break;
						default:
							debug++;
							break;
						}
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%x \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[base_offset].name, -number); logical_PC[0] += 4;
					}
					else {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[base_offset].name, reg_table->entry_x[offset.reg_index].name, number); logical_PC[0] += 4;
					}
					reg_table->entry_x[offset.reg_index].in_use = 0;
					reg_table->entry_x[base_offset].in_use = 0;
				}
				else if (check_funct_arg(parse_out, current, 1, parse_in, logical_PC, reg_table, l_control, current, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, param, Masm, unit_debug)) {
					// purposely left blank
				}
				else {
					debug++;
				}
			}
			else if (getOperatorB(&op_match, parse_in, op_table)) {
				getWord(parse_in);
				VariableListEntry* test;
				UINT8 op_match2;
				if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
					if (getOperatorB(&op_match2, parse_in, op_table)) {
						getWord(parse_in);
						INT64 number;
						if (get_integer(&number, parse_in->word)) {
							UINT8 func_ct = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -8(gp) \n", logical_PC[0], reg_table->entry_x[func_ct].name); logical_PC[0] += 4;
							UINT8 t1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							UINT8 t2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, t1, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);

							UINT8 test_offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, test_offset, test->addr, test->name, reg_table, l_control->depth, parse_in, param, Masm);
							UINT8 current_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, current_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, gp		// copy array pointer to argument register, offset + gp (skip stack)\n", logical_PC[0], op_table[op_match2].opcode, reg_table->entry_x[t2].name, reg_table->entry_x[test_offset].name, reg_table->entry_x[t1].name); logical_PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, gp		// copy array pointer to argument register, offset + gp (skip stack)\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[current_index].name, reg_table->entry_x[func_ct].name, reg_table->entry_x[t2].name); logical_PC[0] += 4;
							reg_table->entry_x[func_ct].in_use = 0;
							reg_table->entry_x[t1].in_use = 0;
							reg_table->entry_x[t2].in_use = 0;
							reg_table->entry_x[test_offset].in_use = 0;
							reg_table->entry_x[current_index].in_use = 0;
						}
						else {
							debug++;
						}
					}
					else {
						debug++;
					}
				}
				else {
					debug++;
				}
			}
			else {
				debug++;
			}
		}
		else {
			getWord(parse_in);
			VariableListEntry* global_var;
			UINT8 index;
			INT64 number;
			if (get_integer(&number, parse_in->word)) {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
					current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					current->reg_index_valid = 1;
					current->reg_depth = l_control->depth;
					load_64bB(parse_out, logical_PC, current->reg_index, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
				}
				else {
					debug++;
				}
			}
			else if (get_FunctionListEntryB(&function_match, parse_in->word, compiler_var->f_list_base)) {
				int offset1 = 0;
				int offset2 = -0xc8 - offset1;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') parse_in->ptr++;
				UINT limit = (function_match->type == void_enum) ? 0 : 1;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi sp, sp, -0x20		// align SP for push, function call\n", logical_PC[0], offset1);  logical_PC[0] += 4;// SP alignment
				for (INT8 i = limit; i < 4; i++) {//function_match->arg_count
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',') parse_in->ptr++;
					UINT8 csr_index = 0;
					getWord(parse_in);
					VariableListEntry* test;
					if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) { // need to handle simple variables differently from array variables (pointers)

						UINT8 test_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, logical_PC, test_index, test->addr, test->name, reg_table, l_control->depth, parse_in, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add a%d, %s, gp		// copy array pointer to argument register, offset + gp (skip stack)\n", logical_PC[0], i, reg_table->entry_x[test_index].name); logical_PC[0] += 4;
						reg_table->entry_x[test_index].in_use = 0;
					}
					else if (get_integer(&number, parse_in->word)) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi a%d, zero, 0x%x		// copy number to argument register (skip stack)\n", logical_PC[0], i, number); logical_PC[0] += 4;
					}
					else if (match_listB(&csr_index, parse_in->word, csr_list, csr_list_count, logical_PC)) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrc a%d, zero, %s\n", logical_PC[0], i, csr_list[csr_index]); logical_PC[0] += 4;
					}
					else {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ERROR: not coded in Masm compiler yet\n", logical_PC[0]);
						debug++;
					}
				}
				for (UINT i = 0; i < 12; i++) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd s%d, %#06x(sp)		// store %d\n", logical_PC[0], i, offset1 -= 8, i);  logical_PC[0] += 4;// store 
					l_control->fence[l_control->depth] = 1;
				}
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi sp, sp, 0x%x		// set SP for next function usage\n", logical_PC[0], offset1);  logical_PC[0] += 4;// SP alignment
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x jal ra, label_%s // uses return address\n", logical_PC[0], function_match->name); logical_PC[0] += 4; offset2 += 8;
				// pop pointers; will be using jal(r) 0 for return
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi sp, sp, 0x%x		// copy return value to new variable\n", logical_PC[0], -offset1);  logical_PC[0] += 4;// SP alignment
				offset1 = 0;
				for (UINT i = 0; i < 12; i++) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld s%d, %#06x(sp)				// restore s%d\n", logical_PC[0], i, offset1 -= 8, i);  logical_PC[0] += 4;// store 
				}
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld sp, %#06x(sp)				// restore sp (stack pointer)\n", logical_PC[0], offset1 - 16);  logical_PC[0] += 4;// store 
				UINT8 current_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				load_64bB(parse_out, logical_PC, current_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, a0, 0		// copy return value to new variable\n", logical_PC[0], reg_table->entry_x[current_index].name);  logical_PC[0] += 4;// SP alignment
				reg_table->entry_x[current_index].in_use = 0;
			}
			else if (get_VariableListEntryB(&global_var, parse_in->word, compiler_var->global_list_base)) {
				if (current->reg_index_valid == 0) {
					current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					current->reg_index_valid = 1;
					current->reg_depth = l_control->depth;
				}
				if (current->pointer)
					load_64bB(parse_out, logical_PC, current->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
					if (!compiler_var->global_list_base->pointer)
						debug++;
					UINT8 hold = parse_in->ptr;
					parse_in->ptr++;
					getWord(parse_in);
					INT64 num;
					if (get_integer(&num, parse_in->word)) {
						if (num * 8 > 0x800) {
							UINT8 t1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							UINT8 t2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, t1, num, (char*)"", reg_table, l_control->depth, parse_in, param, Masm);
							UINT8 global_offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, global_offset, global_var->addr, global_var->name, reg_table, l_control->depth, parse_in, param, Masm);
						//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %s // calculate address\n", logical_PC[0], reg_table->entry_x[t2].name, reg_table->entry_x[t1].name, reg_table->entry_x[global_offset].name); logical_PC[0] += 4;
						//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s) // load global table entry\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[global_offset].name); logical_PC[0] += 4;
							sprint_op_2src(parse_out, logical_PC, (char*)"addi", reg_table->entry_x[t2].name, reg_table->entry_x[t1].name, reg_table->entry_x[global_offset].name, (char*)"calculate address", param);
							sprint_load(parse_out, logical_PC, (char*)"ld", reg_table->entry_x[current->reg_index].name, (INT16)0, reg_table->entry_x[global_offset].name, (char*)"load global table entry", param);
							reg_table->entry_x[t1].in_use = 0;
							reg_table->entry_x[t2].in_use = 0;
							reg_table->entry_x[global_offset].in_use = 0;
						}
						else {
							UINT8 t2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, t2, global_var->addr, global_var->name, reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -0x%03x(%s) // load global table entry\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, num * 8, reg_table->entry_x[t2].name); logical_PC[0] += 4;
							reg_table->entry_x[t2].in_use = 0;
						}
					}
					else {
						VariableListEntry addr;
						addVariableEntry(&addr, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
						parse_in->ptr = hold;
						parse_index_a(parse_out, &addr, 0, logical_PC, global_var, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -8(%s) // load global table entry\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[addr.reg_index].name); logical_PC[0] += 4;
						reg_table->entry_x[addr.reg_index].in_use = 0;
						debug++;
					}
				}
				else if (global_var->pointer && parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
					load_64bB(parse_out, logical_PC, current->reg_index, global_var->addr, global_var->name, reg_table, l_control->depth, parse_in, param, Masm); // clear logical error, where is the destination that is being retained
				}
				else {
					UINT8 global_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, logical_PC, global_index, global_var->addr, global_var->name, reg_table, l_control->depth, parse_in, param, Masm);
	//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s) // load global variable\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[global_index].name); logical_PC[0] += 4;
					sprint_load(parse_out, logical_PC, (char*)"ld", reg_table->entry_x[current->reg_index].name, (UINT16)0, reg_table->entry_x[global_index].name, (char*)"load global variable", param);
					reg_table->entry_x[global_index].in_use = 0;
				}
			}
			else if (get_reg_indexB(&index, parse_in->word, reg_table)) {
				if (current->reg_index_valid == 0) {
					current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					current->reg_index_valid = 1;
					current->reg_depth = l_control->depth;
				}
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
					parse_in->ptr++;
					getWord(parse_in);
					INT64 num;
					VariableListEntry* test;
					if (get_integer(&num, parse_in->word)) {
						if (num > 0x800) {
							UINT8 t1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							UINT8 t2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, t1, num, (char*)"", reg_table, l_control->depth, parse_in, param, Masm);
					//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s // calculate address\n", logical_PC[0], reg_table->entry_x[t2].name, reg_table->entry_x[t1].name, reg_table->entry_x[index].name); logical_PC[0] += 4;
					//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s) // load global table entry\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[t2].name); logical_PC[0] += 4;
							sprint_op_2src(parse_out, logical_PC, (char*)"addi", reg_table->entry_x[t2].name, reg_table->entry_x[t1].name, reg_table->entry_x[index].name, (char*)"calculate address", param);
							sprint_load(parse_out, logical_PC, (char*)"ld", reg_table->entry_x[current->reg_index].name, (INT16)0, reg_table->entry_x[t2].name, (char*)"load global table entry", param);
							reg_table->entry_x[t1].in_use = 0;
							reg_table->entry_x[t2].in_use = 0;
						}
						else {
				//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -0x%03x(%s) // load global variable\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, (num+1) * 8, reg_table->entry_x[index].name); logical_PC[0] += 4;
							sprint_load(parse_out, logical_PC, (char*)"ld", reg_table->entry_x[current->reg_index].name, -((num + 1) * 8), reg_table->entry_x[index].name, (char*)"load global variable", param);
						}
					}
					else if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
						//						debug++;
						UINT8 t1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // stack assumed set of 64b address or pointer\n", logical_PC[0], reg_table->entry_x[t1].name, reg_table->entry_x[test->reg_index].name); logical_PC[0] += 4;
				//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // calculate address\n", logical_PC[0], reg_table->entry_x[t1].name, reg_table->entry_x[index].name, reg_table->entry_x[t1].name); logical_PC[0] += 4;
				//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -8(%s) // load global table entry\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[t1].name); logical_PC[0] += 4;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '+') {
							parse_in->ptr += 2;
				//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1 // %s++\n", logical_PC[0], reg_table->entry_x[test->reg_index].name, reg_table->entry_x[test->reg_index].name, test->name); logical_PC[0] += 4;
							char word[0x40];
							sprintf_s(word,"%s++", test->name);
							sprint_op_2src(parse_out, logical_PC, (char*)"addi", reg_table->entry_x[test->reg_index].name, reg_table->entry_x[test->reg_index].name, (UINT16)1, word, param);
							sprint_op_2src(parse_out, logical_PC, (char*)"slli", reg_table->entry_x[t1].name, reg_table->entry_x[test->reg_index].name, (INT16)3, (char*)"stack assumed set of 64b address or pointer", param);
						}
						else {
							debug++;
						}
						sprint_op_2src(parse_out, logical_PC, (char*)"sub ", reg_table->entry_x[t1].name, reg_table->entry_x[index].name, reg_table->entry_x[t1].name, (char*)"calculate address", param);
						sprint_load(parse_out, logical_PC, (char*)"ld", reg_table->entry_x[current->reg_index].name, (INT16)0, reg_table->entry_x[t1].name, (char*)"load global table entry", param);
					}
					else {
						debug++;
					}
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
						parse_in->ptr++;
					}
					else {
						debug++;
					}
				}
				else {
	//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0 // load global variable\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[index].name); logical_PC[0] += 4;
					sprint_op_2src(parse_out, logical_PC, (char*)"addi", reg_table->entry_x[current->reg_index].name, reg_table->entry_x[index].name, (UINT16)0, (char*)"load global variable", param);
				}
				//					reg_table->entry_x[current_index].in_use = 0;
			}
			// need to match reg table, not csr
			else {
				parse_in->ptr = hold;
			}
		}
		if (parse_in->ptr == hold) {
			UINT8 num_valid = 0;
			INT64 number;
			if (parse_in->index == 140 && unit_debug)
				debug++;
			if (current->pointer == 1) {
				current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm); // pointer declaration
				current->reg_index_valid = 1;
				current->reg_depth = l_control->depth;
				parse_rh_of_eqB(parse_out, current, 0, &num_valid, &number, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
			}
			else {
				if (current->type == fp16_enum) {
					current->reg_index = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
				}
				else {
					current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm); // pointer declaration
				}
				current->reg_index_valid = 1;
				current->reg_depth = l_control->depth;
				parse_rh_of_eqB(parse_out, current, 0, &num_valid, &number, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
			}
		}
	}
	else {
		debug++;
	}
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
}
int funct_arg(UINT* target, parse_struct2* parse_out, parse_struct2* parse_in, INT64* logical_PC, loop_control* l_control, FunctionListEntry* function_match,
	compiler_var_type* compiler_var, reg_table_struct* reg_table, var_type_struct* csr_list, UINT8 csr_list_count, param_type* param, FILE* Masm) {
	UINT hit = 0;
	int debug = 0;
	for (UINT8 j = 0; j < compiler_var->f_list_base->last->argc; j++) {
		if (strcmp(compiler_var->f_list_base->last->argument[j].name, parse_in->word) == 0) {
			VariableListEntry* current = &compiler_var->f_list_base->last->argument[j];
			hit = 1;
			if (current->reg_index_valid == 0) {
				current->reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x%x(a1)		//load \"%s\" from argp + offset\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, -((j + 1) << 3), current->name); logical_PC[0] += 4;
			}
			else {
				debug++;
			}
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
				parse_in->ptr++;
				getWord(parse_in);
				UINT hit2 = 0;
				for (UINT8 k = 0; k < compiler_var->f_list_base->last->argc; k++) {
					if (strcmp(compiler_var->f_list_base->last->argument[k].name, parse_in->word) == 0) {
						hit2 = 1;
						if (compiler_var->f_list_base->last->argument[k].reg_index_valid == 0) {
							compiler_var->f_list_base->last->argument[k].reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x%x(a1)		//load \"%s\" from argp + offset\n", logical_PC[0], reg_table->entry_x[compiler_var->f_list_base->last->argument[k].reg_index].name, -((k + 1) << 3), compiler_var->f_list_base->last->argument[k].name); logical_PC[0] += 4;
						}
						else {
							debug++;
						}
						UINT8 temp = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
							parse_in->ptr++;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							INT64 number;
							if (get_integer(&number, parse_in->word)) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%03x		//\n", logical_PC[0], reg_table->entry_x[temp].name, reg_table->entry_x[compiler_var->f_list_base->last->argument[k].reg_index].name, number + 1); logical_PC[0] += 4;
							}
							else {
								debug++;
							}
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1		//\n", logical_PC[0], reg_table->entry_x[temp].name, reg_table->entry_x[compiler_var->f_list_base->last->argument[k].reg_index].name); logical_PC[0] += 4;
						}
						else {
							debug++;
						}
						if (compiler_var->f_list_base->last->argument[k].type != uint8_enum)
							debug++;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s		//\n", logical_PC[0], reg_table->entry_x[temp].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[temp].name); logical_PC[0] += 4;
						memory_load(parse_out, current, temp, current, 0, logical_PC, reg_table, parse_in, l_control->depth, param, Masm);
						reg_table->entry_x[temp].in_use = 0;
						if (compiler_var->f_list_base->last->argument[k].reg_index_valid == 0) {
							reg_table->entry_x[compiler_var->f_list_base->last->argument[k].reg_index].in_use = 0;
						}
					}
				}
				if (!hit2) {
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
						parse_in->ptr++;
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						char word[0x100];
						sprintf_s(word, "%s", parse_in->word);
						getWord(parse_in);
						INT64 number2;
						if (get_integer(&number2, parse_in->word)) {
							INT64 number;
							VariableListEntry* test;
							if (get_integer(&number, word)) {
								memory_load(parse_out, current, current->reg_index, current, number+number2, logical_PC, reg_table, parse_in, l_control->depth, param, Masm);
							}
							else if (get_VariableListEntryB(&test, word, compiler_var->list_base)) {
								UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%03x		//  \n", logical_PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[test->reg_index].name,number2+1); logical_PC[0] += 4;
								switch (current->type) {
								case uint8_enum:
							//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 0		// [%s] 8b address \n", logical_PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[offset].name, test->name); logical_PC[0] += 4;
									break;
								case int16_enum:
								case uint16_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 1		// [%s] 16b address \n", logical_PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[offset].name, test->name); logical_PC[0] += 4;
									break;
								case int32_enum:
								case uint32_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 2		// [%s] 32b address \n", logical_PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[offset].name, test->name); logical_PC[0] += 4;
									break;
								case int64_enum:
								case uint64_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3		// [%s] 64b address \n", logical_PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[offset].name, test->name); logical_PC[0] += 4;
									break;
								case int128_enum:
								case uint128_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3		// [%s] 128b address \n", logical_PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[offset].name, test->name); logical_PC[0] += 4;
									break;
								default:
									debug++;
									break;
								}
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s		// assume stack order (descending order) \n", logical_PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset].name); logical_PC[0] += 4;
								memory_load(parse_out, current, offset, current, 0, logical_PC, reg_table, parse_in, l_control->depth, param, Masm);
							}
							else {
								debug++;
							}
						}
						else {
							INT64 number;
							VariableListEntry* test;
							if (get_integer(&number, word)) {
								memory_load(parse_out, current, current->reg_index, current, number, logical_PC, reg_table, parse_in, l_control->depth, param, Masm);
							}
							else if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
								UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3		// [%s] assume 64b address \n", logical_PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[test->reg_index].name, test->name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s		// assume stack order (descending order) \n", logical_PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset].name); logical_PC[0] += 4;
								memory_load(parse_out, current, offset, current, 0, logical_PC, reg_table, parse_in, l_control->depth, param, Masm);
							}
							else {
								debug++;
							}
						}
					}
					else {
						INT64 number;
						VariableListEntry* test;
						if (get_integer(&number, parse_in->word)) {
							memory_load(parse_out, current, current->reg_index, current, number, logical_PC, reg_table, parse_in, l_control->depth, param, Masm);
						}
						else if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
							UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3		// [%s] assume 64b address \n", logical_PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[test->reg_index].name, test->name); logical_PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s		// assume stack order (descending order) \n", logical_PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset].name); logical_PC[0] += 4;
							//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -0x008(%s)	// load %s[%s]\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset].name, current->name, test->name); logical_PC[0] += 4;
							memory_load(parse_out, current, offset, current, 0, logical_PC, reg_table, parse_in, l_control->depth, param, Masm);
						}
						else {
							debug++;
						}
					}
				}
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
					parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
					getWord(parse_in);
					INT64 number;
					UINT target2;
					if (get_integer(&number, parse_in->word)) {
						if (number < 0x0800) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%03x		//\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name, number); logical_PC[0] += 4;
						}
						else {
							target2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, target2, number, parse_in->word, reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s		//\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[target2].name); logical_PC[0] += 4;
							reg_table->entry_x[target2].in_use = 0;
						}
					}
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				}
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']')
					parse_in->ptr++;
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
					INT64 number;
					UINT target2;
					parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
					getWord(parse_in);
					if (get_integer(&number, parse_in->word)) {
						if (number < 0x0800) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%03x		//\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name, number); logical_PC[0] += 4;
						}
						else {
							target2 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, target2, number, parse_in->word, reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s		//\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[target2].name); logical_PC[0] += 4;
							reg_table->entry_x[target2].in_use = 0;
						}
					}
					else if (funct_arg(&target2, parse_out, parse_in, logical_PC, l_control, function_match, compiler_var, reg_table, csr_list, csr_list_count, param, Masm)) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s		//\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[target2].name); logical_PC[0] += 4;
						reg_table->entry_x[target2].in_use = 0;
					}
					else {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ERROR: not coded in Masm compiler yet\n", logical_PC[0]);
					}
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				}
			}
			target[0] = current->reg_index;
		}
	}
	return hit;
}
int exec_funct_call_math(parse_struct2* parse_out, parse_struct2* parse_in, UINT target, INT64* logical_PC, reg_table_struct* reg_table,
	loop_control* l_control, FunctionListEntry* function_match, compiler_var_type* compiler_var, var_type_struct* csr_list, UINT8 csr_list_count, param_type* param, FILE* Masm) {
	int debug = 0;
	int hit = 1;
	while (hit) {
		INT64 number;
		hit = 0;
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
			hit = 1;
			parse_in->ptr++;
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
			getWord(parse_in);
			if (get_integer(&number, parse_in->word)) {
				if (number < 0x800) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%x		// \n", logical_PC[0], reg_table->entry_x[target].name, reg_table->entry_x[target].name, number); logical_PC[0] += 4;
				}
				else {
					debug++;
				}
			}
			else {
				UINT target2;
				if (funct_arg(&target2, parse_out, parse_in, logical_PC, l_control, function_match, compiler_var, reg_table, csr_list, csr_list_count, param, Masm)) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s		// \n", logical_PC[0], reg_table->entry_x[target].name, reg_table->entry_x[target].name, reg_table->entry_x[target2].name); logical_PC[0] += 4;
					reg_table->entry_x[target2].in_use = 0;
				}
				else {
					debug++;
				}
			}
		}
	}
	return hit;
}
void exec_function_call(parse_struct2* parse_out, parse_struct2* parse_in, INT64* logical_PC, loop_control* l_control, FunctionListEntry* function_match,
	compiler_var_type* compiler_var, reg_table_struct* reg_table, var_type_struct* csr_list, UINT8 csr_list_count, param_type* param, FILE* Masm) {
	int debug = 0;
	UINT8 csr_index = 0;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') parse_in->ptr++;
	// push a0-a3 onto stack, update stack??
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd a0, -8(sp)		//store a0-a3\n", logical_PC[0]); logical_PC[0] += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd a1, -16(sp)		//prep for function call\n", logical_PC[0]); logical_PC[0] += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd a2, -24(sp)		//\n", logical_PC[0]); logical_PC[0] += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd a3, -32(sp)		//\n", logical_PC[0]); logical_PC[0] += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi a2, zero, 0x%03x		//load a2 with argument count (arg_c)\n", logical_PC[0], function_match->argc); logical_PC[0] += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi a3, sp, -32		//load a3 with argument pointer (arg_p)\n", logical_PC[0]); logical_PC[0] += 4;
	UINT stack_offset = (function_match->argc * 8) + 32;
	int temp;
	switch (param->align) {
	case 0:
		break;
	case 16:
		temp = (0x10 - (stack_offset & 0x0f)) & 0x0f;
		debug++;
		stack_offset += temp;
		break;
	default:
		debug++;
		break;
	}
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi sp, sp, 0x%03x		//update stack\n", 
		logical_PC[0], -((INT)stack_offset)); logical_PC[0] += 4;
	l_control->fence[l_control->depth] = 1;
	for (VariableListEntry* current = compiler_var->list_base->next; current != compiler_var->list_base; current = current->next) {
		if (current->pointer) {
			current->sp_offset += (function_match->argc * 8 + 32);
		}
	}

	// set arguments
	for (UINT8 i = 0; i < function_match->argc; i++) {
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
		getWord(parse_in);
		INT64 number;
		VariableListEntry* test;
		if (find_loop_reg_index(l_control, parse_in->word)) {
			VariableListEntry test2;
			test2.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			if (exec_funct_call_math(parse_out, parse_in, test2.reg_index, logical_PC, reg_table, l_control, function_match, compiler_var, csr_list, csr_list_count, param, Masm)) {
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s		//\n", logical_PC[0], reg_table->entry_x[test2.reg_index].name, reg_table->entry_x[test2.reg_index].name, reg_table->entry_x[l_control->for_loop[l_control->index].index_reg].name); logical_PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, 0x%x(a3)		//arg[%d] = \"%s\"\n", logical_PC[0], reg_table->entry_x[test2.reg_index].name, -((i + 1) << 3), i, function_match->argument[i].name); logical_PC[0] += 4;
			}
			else {
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, 0x%x(a3)		//arg[%d] = \"%s\"\n", logical_PC[0], reg_table->entry_x[l_control->for_loop[l_control->index].index_reg].name, -((i + 1) << 3), i, function_match->argument[i].name); logical_PC[0] += 4;
			}
			reg_table->entry_x[test2.reg_index].in_use = 0;
		}
		else if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) { // need to handle simple variables differently from array variables (pointers)
			if (test->pointer) {
				if (test->reg_index_valid == 0) {
					test->reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				}
				exec_funct_call_math(parse_out, parse_in, test->reg_index, logical_PC, reg_table, l_control, function_match, compiler_var, csr_list, csr_list_count, param, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, 0x%x(a3)		//arg[%d] = \"%s'\"\n", logical_PC[0], reg_table->entry_x[test->reg_index].name, -((i + 1) << 3),i, function_match->argument[i].name); logical_PC[0] += 4;
				if (test->reg_index_valid == 0)
					reg_table->entry_x[test->reg_index].in_use = 0;
			}
			else {
				switch (test->type) {
				case uint16_enum:
					if (test->reg_index_valid == 0)
						test->reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					if (test->addr < 0x800) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lh %s, -0x008(sp)		//\n", logical_PC[0], reg_table->entry_x[test->reg_index].name, (test->addr+1)*2); logical_PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, 0x%x(a3)		//arg[%d] = \"%s\"\n", logical_PC[0], reg_table->entry_x[test->reg_index].name, -((i + 1) << 3), i, function_match->argument[i].name); logical_PC[0] += 4;
					}
					else {
						debug++;
					}
					exec_funct_call_math(parse_out, parse_in, test->reg_index, logical_PC, reg_table, l_control, function_match, compiler_var, csr_list, csr_list_count, param, Masm);
					if (test->reg_index_valid == 0)
						reg_table->entry_x[test->reg_index].in_use = 0;
					break;
				default:
					debug++;
					break;
				}
			}
		}
		else if (get_integer(&number, parse_in->word)) {
			UINT8 test_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x		// copy number to argument register (skip stack)\n", logical_PC[0], reg_table->entry_x[test_index].name, number); logical_PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, 0x%x(a3)		//arg[%d] = \"%s\"\n", logical_PC[0], reg_table->entry_x[test_index].name, -((i + 1) << 3), i, function_match->argument[i].name); logical_PC[0] += 4;
			reg_table->entry_x[test_index].in_use = 0;
		}
		else if (match_listB(&csr_index, parse_in->word, csr_list, csr_list_count, logical_PC)) {
			UINT8 test_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrc %s, zero, %s\n", logical_PC[0], reg_table->entry_x[test_index].name, csr_list[csr_index]); logical_PC[0] += 4;
			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, 0x%x(a3)		//arg[%d] = \"%s\"\n", logical_PC[0], reg_table->entry_x[test_index].name, -((i + 1) << 3), i, function_match->argument[i].name); logical_PC[0] += 4;
			reg_table->entry_x[test_index].in_use = 0;
		}
		else {
			UINT hit = 0;
			UINT target;
			if (funct_arg(&target, parse_out, parse_in, logical_PC, l_control, function_match, compiler_var, reg_table, csr_list, csr_list_count, param, Masm)) {
				exec_funct_call_math(parse_out, parse_in, target, logical_PC, reg_table, l_control, function_match, compiler_var, csr_list, csr_list_count, param, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, 0x%x(a3)		//arg[%d] = \"%s\"\n", logical_PC[0], reg_table->entry_x[target].name, -((i + 1) << 3), i, function_match->argument[i].name); logical_PC[0] += 4;
				reg_table->entry_x[target].in_use = 0;
			}
			else {
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ERROR: not coded in Masm compiler yet\n", logical_PC[0]);
			}
		}
		l_control->fence[l_control->depth] = 1;

		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',')
			parse_in->ptr++;
	}
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi a0, a2, 0x000		//move arg_c to a0\n", logical_PC[0]); logical_PC[0] += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi a1, a3, 0x000		//move arg_p to a1\n", logical_PC[0]); logical_PC[0] += 4;

	push_all(parse_out, logical_PC, param,reg_table);
//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fence 0 // order all memory operations 1\n", logical_PC[0]); logical_PC[0] += 4;
	sprintf_string(parse_out, logical_PC, (char*)" fence 0 // order all memory operations 1\n", param);
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x jal a2, 0x%05x // adjust for stack ordering			\n", logical_PC[0], function_match->addr - logical_PC[0]);  logical_PC[0] += 4; // store 
	pop_all(parse_out, logical_PC, param, reg_table);
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi sp, sp, 0x%03x		//update stack\n", logical_PC[0], stack_offset); logical_PC[0] += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld a0, -8(sp)		//restore a0-a3\n", logical_PC[0]); logical_PC[0] += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld a1, -16(sp)		//return from function call\n", logical_PC[0]); logical_PC[0] += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld a2, -24(sp)		//\n", logical_PC[0]); logical_PC[0] += 4;
	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld a3, -32(sp)		//return from function call complete\n", logical_PC[0]); logical_PC[0] += 4;
	for (VariableListEntry* current = compiler_var->list_base->next; current != compiler_var->list_base; current = current->next) {
		if (current->pointer) {
			current->sp_offset -= (function_match->argc * 8 + 32);
		}
	}
}
void ParseFunctionCode(parse_struct2* parse_out, parse_struct2* parse_in, loop_control* l_control, INT64* logical_PC, var_type_struct* control_type, var_type_struct* modifier_type, operator_type* branch_table, var_type_struct* csr_list, UINT8 csr_list_count, operator_type* op_table,
	reg_table_struct* reg_table, memory_space_type* memory_space, hw_pointers* pointers, INT64 base, compiler_var_type* compiler_var, short* sp_offset, FunctionListEntry* current_function, IO_list_type* IO_list, param_type* param, FILE* Masm, UINT8 unit_debug) {
	int debug = 0;

	reg_table->t_count = (reg_table->entry_x[5].in_use == 1) ? 1 : 0;
	if (reg_table->entry_x[15].in_use == 1)
		reg_table->t_count++;
	reg_table->s_count = (reg_table->entry_x[14].in_use == 1) ? 1 : 0;
	for (UINT8 i = 6; i < 10; i++)
		if (reg_table->entry_x[i].in_use == 1)
			reg_table->s_count++;
	for (UINT16 i = 16; i < 32; i++)
		if (reg_table->entry_x[i].in_use == 1)
			reg_table->s_count++;


	if (parse_in->index == 21)//
		debug++;
	if (parse_in->index == 100 && unit_debug)
		debug++;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
		parse_in->ptr += 2;
		while ((parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')parse_in->ptr++;
		getWord(parse_in);
		if (strcmp(parse_in->word, "hint") == 0) {
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ':')
				parse_in->ptr++;
			else
				debug++;
			while ((parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')parse_in->ptr++;
			getWord(parse_in);
			if (strcmp(parse_in->word, "free") == 0) {
				while ((parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')parse_in->ptr++;
				getWord(parse_in);
				VariableListEntry* test;
				if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
					if (test->reg_index_valid)
						reg_table->entry_x[test->reg_index].in_use = 0;
					test->reg_index_valid = 0;
					test->last->next = test->next;
					test->next->last = test->last;
					free(test);
				}
				else {
					debug++;
				}
			}
			else {
				debug++;
			}
		}
		else {
		}
	}
	else {
		UINT8 modifier_index = 0;
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
			parse_in->ptr++;
			l_control->depth++;
			sprintf_s(parse_out->line[parse_out->index++].line, "// current depth = %d.%d // plus 2 \n", l_control->depth, l_control->count[l_control->depth]);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '}') {
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '}' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ' ' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'e' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'l' && parse_in->line[parse_in->index].line[parse_in->ptr + 4] == 's' && parse_in->line[parse_in->index].line[parse_in->ptr + 5] == 'e') {
				parse_in->ptr += 6;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
				getWord(parse_in);
				if (strcmp(parse_in->word, "if") == 0) {
					debug++; // drops through and captures the if as a regular statement
				}
				UINT8 s1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_end%d_%d \n", logical_PC[0], l_control->depth, l_control->end[l_control->depth]); logical_PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x nop 0x%02x // make space if >20b jump\n", logical_PC[0], s1); logical_PC[0] += 4;
				reg_table->entry_x[s1].in_use = 0;
			}
			else if (l_control->depth == 1) {
				l_control->depth--;
				reg_maintanence(reg_table, compiler_var, l_control, parse_out, logical_PC, parse_in,param);
				l_control->depth++;// will be counted down later
		//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x jalr zero, 0(a2)	// return to caller\n", logical_PC[0]); logical_PC[0] += 4;// return to caller
				sprint_load(parse_out, logical_PC, (char*)"jalr", (char*)"zero", (UINT16)0, (char*)"a2", (char*)"return to caller", param);
				for (VariableListEntry* list_index = compiler_var->list_base->next; list_index != compiler_var->list_base; list_index = list_index->next) {
					if (list_index->depth >= l_control->depth) {
						VariableListEntry* list_temp = list_index->last;
						list_index->last->next = list_index->next;
						list_index->next->last = list_index->last;
						free(list_index);
						list_index = list_temp;
					}
				}
			}
			else {
				parse_in->ptr++;
				char line_cache[0x100], line_cache_word[0x100];
				if (l_control->line_valid[l_control->depth] == 1) {
					sprintf_s(line_cache, "%s", l_control->line[l_control->depth]);
					UINT line_cache_ptr = 0;
					while (line_cache[line_cache_ptr] == ' ' || line_cache[line_cache_ptr] == '\t') line_cache_ptr++;
					if (line_cache[line_cache_ptr] == 'f' && line_cache[line_cache_ptr + 1] == 'o' && line_cache[line_cache_ptr + 2] == 'r') {
						while (line_cache[line_cache_ptr] != '{') line_cache_ptr++;
						while (line_cache[line_cache_ptr] != ';') line_cache_ptr--;
						line_cache_ptr++;
						if (line_cache[line_cache_ptr] == ' ')line_cache_ptr++;

						int debug = 0;
						//	char result[0x100];
						UINT8 length = strlen(line_cache);
						UINT8 i;
						while (line_cache[line_cache_ptr] == '=' || line_cache[line_cache_ptr] == ' ' || line_cache[line_cache_ptr] == '\"' || line_cache[line_cache_ptr] == '\t')line_cache_ptr++;

						for (i = line_cache_ptr; i < length && debug == 0; i++) {
							if (line_cache[i] != ' ' && line_cache[i] != '\t' && line_cache[i] != '\0' && line_cache[i] != '\n' && line_cache[i] != ',' && line_cache[i] != ';' && line_cache[i] != ':' && line_cache[i] != '=' && line_cache[i] != '?' && line_cache[i] != '&' &&
								line_cache[i] != '|' && line_cache[i] != '^' && line_cache[i] != '+' && line_cache[i] != '-' && line_cache[i] != '*' && line_cache[i] != '/' && line_cache[i] != '<' && line_cache[i] != '>' &&
								line_cache[i] != '(' && line_cache[i] != ')' && line_cache[i] != '{' && line_cache[i] != '}' && line_cache[i] != '[' && line_cache[i] != ']' && line_cache[i] != '\"') {
								line_cache_word[i - line_cache_ptr] = line_cache[i];
							}
							else {
								debug++;
							}
						}
						line_cache_word[i - line_cache_ptr - 1] = '\0';
						line_cache_ptr += strlen(line_cache_word);
						while (line_cache[line_cache_ptr] == ' ' || line_cache[line_cache_ptr] == '\t')line_cache_ptr++;


						VariableListEntry* test;
						if (get_VariableListEntryB(&test, line_cache_word, compiler_var->list_base)) {
							if (line_cache[line_cache_ptr] == '+' && line_cache[line_cache_ptr + 1] == '+') {
								debug++;
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
						while (line_cache[line_cache_ptr] != ';') line_cache_ptr--;
						line_cache_ptr--;
						while (line_cache[line_cache_ptr] != ';') line_cache_ptr--;
						line_cache_ptr++;
						getWord(parse_in);
						if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
							if (line_cache[line_cache_ptr] == '<') {
								line_cache_ptr++;
								getWord(parse_in);
								INT64 number;
								get_integer(&number, parse_in->word);
								if (number < 0x800) {
									UINT8 s1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									load_64bB(parse_out, logical_PC, offset, test->addr, test->name, reg_table, l_control->depth, parse_in, param, Masm);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, %#05x     // for_loop tail\n", logical_PC[0], reg_table->entry_x[s1].name, number); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x bge %s, %s, label_for_%s // for_loop tail\n", logical_PC[0], reg_table->entry_x[offset].name, reg_table->entry_x[s1].name, test->name); logical_PC[0] += 4;
									reg_table->entry_x[s1].in_use = 0;
									reg_table->entry_x[offset].in_use = 0;
								}
								else {
									debug++;
								}
							}
							else {
								debug++;
							}

						}
						else {
							debug++;
						}
					}
					else if (strcmp(line_cache, "switch") == 0) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_switch_end_%d\n", logical_PC[0], compiler_var->switch_count++);
					}
				}
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_end%d_%d\n", logical_PC[0], l_control->depth, l_control->end[l_control->depth]);
				l_control->end[l_control->depth]++;
			}
			if (l_control->for_loop[l_control->depth].detected == 1) {
				if (l_control->for_loop[l_control->depth].increment >= 0x800)
					debug++;
//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%03x // add loop increment\n", logical_PC[0], reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, l_control->for_loop[l_control->depth].increment); logical_PC[0] += 4;
				sprint_op_2src(parse_out, logical_PC, (char*)"addi", reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, (INT16)l_control->for_loop[l_control->depth].increment, (char*)"add loop increment", param);
				if (l_control->for_loop[l_control->depth].limit_reg_valid == 1) {
	//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, label_for_exit%d_%d // exit for loop\n", logical_PC[0], branch_table[l_control->for_loop[l_control->depth].branch_match].rev_opcode, reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, reg_table->entry_x[l_control->for_loop[l_control->depth].limit_reg].name, l_control->depth, l_control->count[l_control->depth]); logical_PC[0] += 4;
					char word[0x80];
					sprintf_s(word, "label_for_exit%d_%d", l_control->depth, l_control->count[l_control->depth]);
					sprint_op_2src(parse_out, logical_PC, branch_table[l_control->for_loop[l_control->depth].branch_match].rev_opcode,
						reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, 
						reg_table->entry_x[l_control->for_loop[l_control->depth].limit_reg].name,word,(char*)"exit for loop",param);
				}
				else {
					UINT8 limit = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					if (l_control->for_loop[l_control->depth].limit < 0x800) {
	//					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%03x // load count limit\n", logical_PC[0], reg_table->entry_x[limit].name, l_control->for_loop[l_control->depth].limit); logical_PC[0] += 4;
						sprint_op_2src(parse_out, logical_PC, (char*)"addi", reg_table->entry_x[limit].name, (char*)"zero", l_control->for_loop[l_control->depth].limit, (char*)"load count limit", param);
					}
					else {
						load_64bB(parse_out, logical_PC, limit, l_control->for_loop[l_control->depth].limit, l_control->for_loop[l_control->depth].index_name, reg_table, l_control->depth, parse_in, param, Masm);
					}
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, label_for_exit%d_%d // exit for loop\n", logical_PC[0], branch_table[l_control->for_loop[l_control->depth].branch_match].rev_opcode, reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, reg_table->entry_x[limit].name, l_control->depth, l_control->count[l_control->depth]); logical_PC[0] += 4;
					reg_table->entry_x[limit].in_use = 0;
				}

				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_for%d_%d					// for_loop tail (0)\n", logical_PC[0], l_control->depth, l_control->count[l_control->depth]); logical_PC[0] += 4;
				UINT8 s1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x nop 0x%02x // make space if >20b jump\n", logical_PC[0], s1); logical_PC[0] += 4;
				reg_table->entry_x[s1].in_use = 0;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_for_exit%d_%d      // for_loop tail (0)\n", logical_PC[0], l_control->depth, l_control->count[l_control->depth]);
				l_control->for_loop[l_control->depth].detected = 0; // dissable after useage
				l_control->for_loop[l_control->depth].index_reg = -1;
				l_control->for_loop[l_control->depth].increment = 0;
				strcmp(l_control->for_loop[l_control->depth].index_name, "");
			}
			else if (l_control->for_loop[l_control->depth - 1].detected == 2) {
				l_control->for_loop[l_control->depth - 1].detected = 0;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_while%d_%d					// while tail (0)\n", logical_PC[0], l_control->depth - 1, l_control->count[l_control->depth - 1]); logical_PC[0] += 4;
				UINT8 s1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x nop 0x%02x // make space if >20b jump\n", logical_PC[0], s1); logical_PC[0] += 4;
				reg_table->entry_x[s1].in_use = 0;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_for_exit%d_%d      // for_loop tail (0)\n", logical_PC[0], l_control->depth, l_control->count[l_control->depth]);
				l_control->for_loop[l_control->depth].index_reg = -1;
				l_control->for_loop[l_control->depth].increment = 0;
				strcmp(l_control->for_loop[l_control->depth].index_name, "");
			}
//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label%d_%d\n", logical_PC[0], l_control->depth, l_control->count[l_control->depth]++);
			sprint_label(parse_out, logical_PC[0], (char*)"label", l_control, param);

			l_control->depth--;
			sprintf_s(parse_out->line[parse_out->index++].line, "// current depth = %d.%d // minus 1\n", l_control->depth, l_control->count[l_control->depth]);
			// clear variables and registers
			reg_maintanence(reg_table, compiler_var, l_control, parse_out, logical_PC, parse_in,param);
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
				l_control->depth++;
				sprintf_s(parse_out->line[parse_out->index++].line, "// current depth = %d.%d // plus 3 \n", l_control->depth, l_control->count[l_control->depth]);
			}
		}
		//		getWord(parse_in);
		get_modifierB(&modifier_index, parse_in, modifier_type);
		UINT control_index;
		VariableListEntry* current = compiler_var->list_base->next;
		UINT8 csr_index = 0, reg_index = 0;
		UINT8 op_match;
		if (strcmp(parse_in->word, "reg") == 0) {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != ';') {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
				getWord(parse_in);
				if (parse_in->word[0] == 'a') {
					switch (parse_in->word[1]) {
					case '0':
						reg_table->entry_x[10].in_use = 1;
						break;
					case '1':
						reg_table->entry_x[11].in_use = 1;
						break;
					case '2':
						reg_table->entry_x[12].in_use = 1;
						break;
					case '3':
						reg_table->entry_x[13].in_use = 1;
						break;
					case '4':
						reg_table->entry_x[14].in_use = 1;
						break;
					case '5':
						reg_table->entry_x[15].in_use = 1;
						break;
					case '6':
						reg_table->entry_x[16].in_use = 1;
						break;
					case '7':
						reg_table->entry_x[17].in_use = 1;
						break;
					default:
						debug++;
						break;
					}
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',') {
						parse_in->ptr++;
					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
						//						parse_in->ptr++;
					}
					else {
						debug++;
					}
				}
				else {
					debug++;
				}
			}
		}
		else if (strcmp(parse_in->word, "const") == 0) {
			if (get_VariableTypeNameB(compiler_var->list_base, parse_in, reg_table, l_control, parse_out, Masm)) {
				variableDeclarationBody(parse_out, parse_in, l_control, logical_PC, modifier_index, IO_list, reg_table, branch_table, csr_list, csr_list_count,
					op_table, compiler_var, pointers, param, Masm, unit_debug);
				UINT8 num_valid = 0;
				INT64 number2;
				UINT8 target;
				if (current->type == fp16_enum)
					target = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
				else
					target = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				while (parse_in->line[parse_in->index].line[parse_in->ptr] != ';') {
					if (getOperatorB(&op_match, parse_in, op_table)) {
						VariableListEntry s2;
						s2.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						s2.reg_index_valid = 1;
						sprintf_s(s2.name, "");
						s2.type = int64_enum;
						parse_rh_of_eqB(parse_out, &s2, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // accumulate the result\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[target].name, reg_table->entry_x[s2.reg_index].name); logical_PC[0] += 4;
						reg_table->entry_x[s2.reg_index].in_use = 0;
						s2.reg_index_valid = 0;
						num_valid = 0;
					}
					else {
						debug++;
					}
				}
				if (current->type == fp16_enum) {
					reg_table->entry_fp[target].in_use = 0;
				}
				else {
					reg_table->entry_x[target].in_use = 0;
				}
			}
			else {
				debug++;
			}
		}
		else if (get_ControlTypeB(&control_index, parse_in->word, control_type)) {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == '(')parse_in->ptr++;
			getWord(parse_in);
			switch (control_index) {
			case 0:// if
			{
				UINT8 csr_index = 0;
				UINT8 index;
				int control_depth = l_control->depth + 1;
				if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
					process_if_variable2(parse_out, parse_in, logical_PC, current, reg_table, compiler_var, IO_list, branch_table, op_table, csr_list, csr_list_count, control_depth, l_control, param, Masm, unit_debug);
				}
				else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->global_list_base)) {
					process_if_variable2(parse_out, parse_in, logical_PC, current, reg_table, compiler_var, IO_list, branch_table, op_table, csr_list, csr_list_count, control_depth, l_control, param, Masm, unit_debug);
				}
				else if (match_listB(&csr_index, parse_in->word, csr_list, csr_list_count, logical_PC)) {
					UINT8 temp1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrs %s, zero, %s  \n", logical_PC[0], reg_table->entry_x[temp1].name, csr_list[csr_index]); logical_PC[0] += 4;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;

					process_if_variable3(parse_out, parse_in, logical_PC, temp1, reg_table, compiler_var, IO_list, branch_table, op_table, csr_list, csr_list_count, control_depth, l_control, param, Masm, unit_debug);

					reg_table->entry_x[temp1].in_use = 0;
				}
				else if (get_reg_indexB(&index, parse_in->word, reg_table)) {
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
						INT64 num;
						parse_in->ptr++;
						getWord(parse_in);
						if (get_integer(&num, parse_in->word)) {
							UINT8 regA = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
//							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, -%d(%s) \n", logical_PC[0], reg_table->entry_x[regA].name, (num + 1) * 8, reg_table->entry_x[index].name); logical_PC[0] += 4;
							sprint_load(parse_out,logical_PC,(char*)"ld", reg_table->entry_x[regA].name, -(num + 1) * 8, reg_table->entry_x[index].name,(char*) "start \"if\" staement", param);
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
								parse_in->ptr++;
								UINT8 branch_match;
								if (getOperatorB(&branch_match, parse_in, branch_table)) {
									getWord(parse_in);
									//											INT64 num;
									UINT8 regB = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									if (get_integer(&num, parse_in->word)) {
	//									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x \n", logical_PC[0], reg_table->entry_x[regB].name, num); logical_PC[0] += 4;
										sprint_op_2src(parse_out, logical_PC, (char*)"addi", reg_table->entry_x[regB].name, reg_table->entry_x[0].name, num, param);
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, label%d_%d \n", logical_PC[0], branch_table[branch_match].rev_opcode, reg_table->entry_x[regA].name, reg_table->entry_x[regB].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
									}
									else {
										debug++;
									}
									reg_table->entry_x[regB].in_use = 0;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')parse_in->ptr++;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
										l_control->depth++;
										parse_in->ptr++;
										sprintf_s(parse_out->line[parse_out->index++].line, "// current depth = %d.%d // plus 5 \n", l_control->depth, l_control->count[l_control->depth]);
									}
								}
								else {
									debug++;
								}
							}
							else {
								debug++;
							}
							reg_table->entry_x[regA].in_use = 0;
						}
						else {
							debug++;
						}
					}
					else {
						UINT8 temp1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						getWord(parse_in);
						INT64 num;
						if (get_integer(&num, parse_in->word)) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x bne %s, %s, label%d_%d \n", logical_PC[0], reg_table->entry_x[index].name, reg_table->entry_x[temp1].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
						}
						else {
							debug++;
						}
						reg_table->entry_x[temp1].in_use = 0;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')parse_in->ptr++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
							l_control->depth++;
							sprintf_s(parse_out->line[parse_out->index++].line, "// current depth = %d.%d // plus 5 \n", l_control->depth, l_control->count[l_control->depth]);
						}
					}
				}
				else if (strcmp(parse_in->word, "error") == 0) {
					reg_table->entry_x[reg_table->error].in_use = 1; // hack
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
					UINT8 branch_match;
					if (getOperatorB(&branch_match, parse_in, branch_table)) {
						getWord(parse_in);
						UINT8 regB = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						INT64 num;
						if (get_integer(&num, parse_in->word)) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x \n", logical_PC[0], reg_table->entry_x[regB].name, num); logical_PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, label%d_%d \n", logical_PC[0], branch_table[branch_match].rev_opcode, reg_table->entry_x[reg_table->error].name, reg_table->entry_x[regB].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
						}
						else {
							debug++;
						}
						reg_table->entry_x[regB].in_use = 0;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')parse_in->ptr++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
							l_control->depth++;
							parse_in->ptr++;
							sprintf_s(parse_out->line[parse_out->index++].line, "// current depth = %d.%d // plus 5 \n", l_control->depth, l_control->count[l_control->depth]);
						}
					}
					debug++;
				}
				else {
					debug++;
					UINT8 regB = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					UINT8 depth = 0;
					parse_complex_condition(regB, &l_control->depth, parse_out, parse_in, reg_table, logical_PC, l_control, compiler_var, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x beq %s, zero, label%d_%d \n", logical_PC[0], reg_table->entry_x[regB].name, control_depth, l_control->count[control_depth]); logical_PC[0] += 4;
					reg_table->entry_x[regB].in_use = 0;
					sprintf_s(parse_out->line[parse_out->index++].line, "// current depth = %d.%d // plus 6 \n", l_control->depth, l_control->count[l_control->depth]);
				}
			}
			break;
			case 2:// switch
			{// switch statement
				l_control->depth++;
				sprintf_s(parse_out->line[parse_out->index++].line, "// current depth = %d.%d //switch \n", l_control->depth, l_control->count[l_control->depth]);
				l_control->line_valid[l_control->depth] = 1;
				sprintf_s(l_control->line[l_control->depth], "switch");
				UINT8 csr_index = 0;

				VariableListEntry* test;
				char index;
				// 16b table too large, needs to go onto heap, not in-line
				if (match_listB(&csr_index, parse_in->word, csr_list, csr_list_count, logical_PC)) {// need to set base addr, if (m/s)cause - 16b table
					UINT8 temp_addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);

					l_control->switch_base_addr = logical_PC[0] + 11 * 4 + ((logical_PC[0] & 0x04) ? 0 : 4);
					load_64bB(parse_out, logical_PC, temp_addr, l_control->switch_base_addr, (char *) "switch base", reg_table, l_control->depth, parse_in, param, Masm);
					UINT8 temp1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					UINT8 temp2 = alloc_jalr_UABI(reg_table, l_control->depth, parse_out, Masm);
			//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrs %s, zero, %s  \n", logical_PC[0], reg_table->entry_x[temp1].name, csr_list[csr_index]); logical_PC[0] += 4;
					sprint_op_2src(parse_out, logical_PC, (char*)"csrrs", reg_table->entry_x[temp1].name, reg_table->entry_x[0].name, csr_list[csr_index].name, param);
			//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3  \n", logical_PC[0], reg_table->entry_x[temp1].name, reg_table->entry_x[temp1].name); logical_PC[0] += 4;
					sprint_op_2src(parse_out, logical_PC, (char*)"slli", reg_table->entry_x[temp1].name, reg_table->entry_x[temp1].name,(UINT8)3, param);
			//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s  \n", logical_PC[0], reg_table->entry_x[temp_addr].name, reg_table->entry_x[temp_addr].name, reg_table->entry_x[temp1].name); logical_PC[0] += 4;
					sprint_op_2src(parse_out, logical_PC, (char*)"add ", reg_table->entry_x[temp_addr].name, reg_table->entry_x[temp_addr].name, reg_table->entry_x[temp1].name, param);
			//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x000(%s)  \n", logical_PC[0], reg_table->entry_x[temp2].name, reg_table->entry_x[temp_addr].name); logical_PC[0] += 4;
					sprint_load(parse_out, logical_PC, (char*)"ld", reg_table->entry_x[temp2].name, (UINT16)0, reg_table->entry_x[temp_addr].name, (char*)"load switch target addr", param);
			//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x jalr zero, 0x00(%s) \n", logical_PC[0], reg_table->entry_x[temp2].name); logical_PC[0] += 4;
					sprint_load(parse_out, logical_PC, (char*)"jalr", (char*)"zero", (UINT16)0, reg_table->entry_x[temp2].name, (char*)"jump to switch target", param);
					while (logical_PC[0] < l_control->switch_base_addr) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi zero, zero, 0x00 // NOP: look-up table alignement\n", logical_PC[0]); logical_PC[0] += 4;
					}
//					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_switch_start \n", logical_PC[0]);
					sprint_label(parse_out, logical_PC[0], (char*)"label_switch_start ", (char*)"", param);
					for (UINT i = 0; i < 0x10000; i++) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD label_default	// jump to default location\n", logical_PC[0]); logical_PC[0] += 4;
					}
					reg_table->entry_x[temp_addr].in_use = 0;
					reg_table->entry_x[temp1].in_use = 0;
					reg_table->entry_x[temp2].in_use = 0;
				}
				else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
					if (current->pointer) {
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
								parse_in->ptr++;
								getWord(parse_in);

								UINT8 target = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								load_64bB(parse_out, logical_PC, addr, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);

								INT64 num;
								VariableListEntry* current2;
								if (get_integer(&num, parse_in->word)) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x%03x(%s) // load value of switch condition \"%s[%d]\" \n", logical_PC[0], reg_table->entry_x[target].name, -8 * (num + 1), reg_table->entry_x[addr].name, current->name, num); logical_PC[0] += 4;

								}
								else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
									UINT8 index_adj = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // %s  \n", logical_PC[0], reg_table->entry_x[index_adj].name, reg_table->entry_x[current2->reg_index].name, current2->name); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // %s base address + %s \n", logical_PC[0], reg_table->entry_x[target].name, reg_table->entry_x[addr].name, reg_table->entry_x[index_adj].name, current->name, current2->name); logical_PC[0] += 4;
									reg_table->entry_x[index_adj].in_use = 0;

									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x%03x(%s) // load value of switch condition \"%s[%d]\" \n", logical_PC[0], reg_table->entry_x[target].name, -8, reg_table->entry_x[target].name, current->name, num); logical_PC[0] += 4;
									}
									else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
										parse_in->ptr++;
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
										getWord(parse_in);
										if (get_integer(&num, parse_in->word)) {
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x%03x(%s) // load value of switch condition \"%s[%d]\" \n", logical_PC[0], reg_table->entry_x[target].name, -8 * (num + 1), reg_table->entry_x[target].name, current->name, num); logical_PC[0] += 4;
											if (parse_in->line[parse_in->index].line[parse_in->ptr] != ']') {
												debug++;
											}
										}
										else {
											debug++;
										}
									}
									else {
										debug++;
									}
								}
								else {
									debug++;
								}

								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x jal zero, label_skip_table_%d  // jump over look-up table, save head of table\n", logical_PC[0], compiler_var->switch_count); logical_PC[0] += 4;
								if (logical_PC[0] & 4) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x nop 0x00 // data alignment \n", logical_PC[0]); logical_PC[0] += 4;
								}
								UINT64 table_base_addr = logical_PC[0];
								for (UINT i = 0; i < 0x10; i++) {// hack, need to debug
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD label_case_%d_%d  \n", logical_PC[0], i, compiler_var->switch_count); logical_PC[0] += 8;
								}
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD label_default_%d  \n", logical_PC[0], compiler_var->switch_count); logical_PC[0] += 8;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_skip_table_%d  \n", logical_PC[0], compiler_var->switch_count);

								UINT8 test_a = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								UINT8 test_b = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								UINT8 destination = alloc_jalr_UABI(reg_table, l_control->depth, parse_out, Masm);
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, -16  // less than or equal to highest case value\n", logical_PC[0], reg_table->entry_x[test_a].name, reg_table->entry_x[target].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 32  // convert negative number to -1 \n", logical_PC[0], reg_table->entry_x[test_a].name, reg_table->entry_x[test_a].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, -1  // if greater than case count, set 0 to -1\n", logical_PC[0], reg_table->entry_x[test_b].name, reg_table->entry_x[test_a].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s  // if true - value\n", logical_PC[0], reg_table->entry_x[test_a].name, reg_table->entry_x[test_a].name, reg_table->entry_x[target].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 0x10  // if false - set to default value %d\n", logical_PC[0], reg_table->entry_x[test_b].name, reg_table->entry_x[test_b].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s  // merge true and false responses %d\n", logical_PC[0], reg_table->entry_x[destination].name, reg_table->entry_x[test_a].name, reg_table->entry_x[test_b].name); logical_PC[0] += 4;

								load_64bB(parse_out, logical_PC, addr, table_base_addr, (char*) "", reg_table, l_control->depth, parse_in, param, Masm);
					//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // offset : 8 bytes per entry \n", logical_PC[0], reg_table->entry_x[destination].name, reg_table->entry_x[destination].name); logical_PC[0] += 4;
								sprint_op_2src(parse_out, logical_PC, (char*)"slli", reg_table->entry_x[destination].name, reg_table->entry_x[destination].name, (UINT8)3, (char*)"offset : 8 bytes per entry", param);
					//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s // add offset to table_base_addr \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, reg_table->entry_x[destination].name); logical_PC[0] += 4;
								sprint_op_2src(parse_out, logical_PC, (char*)"add ", reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, reg_table->entry_x[destination].name,(char*)"add offset to table_base_addr",param);
		//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s) // load address for final destination switch case \n", logical_PC[0], reg_table->entry_x[destination].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
								sprint_load(parse_out, logical_PC, (char*)"ld", reg_table->entry_x[destination].name, (UINT16)0, reg_table->entry_x[addr].name, (char*)"load address for final destination switch case", param);
								//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x jalr zero, 0(%s)  // jump to case to be executed \n", logical_PC[0], reg_table->entry_x[destination].name); logical_PC[0] += 4;
								sprint_load(parse_out, logical_PC, (char*)"jalr", (char*)"zero", (UINT16)0, reg_table->entry_x[destination].name, (char*)"jump to case to be executed", param);
								reg_table->entry_x[destination].in_use = 0;
								reg_table->entry_x[test_a].in_use = 0;
								reg_table->entry_x[test_b].in_use = 0;

								reg_table->entry_x[addr].in_use = 0;
								reg_table->entry_x[target].in_use = 0;
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
					}
					else {
						debug++;
					}
				}
				else if (get_ArgumentListEntry(&test, &index, parse_in->word, compiler_var->f_list_base->last->argument, compiler_var->f_list_base->last->argc)) {
					if (test->type == fp16_enum) {
						test->reg_index = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
					}
					else {
						test->reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					}
					VariableListEntry test2;
					test2.reg_index = test->reg_index;
					test2.type = int64_enum;
					sprintf_s(test2.name, "%s", test->name);
					memory_load(parse_out, test, 11, &test2, index, logical_PC, reg_table, parse_in, l_control->depth, param, Masm);
					VariableListEntry target;
					addVariableEntry(&target, test->type, test->name, reg_table, l_control, parse_out, parse_in, Masm);
					if (test->pointer) {
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[')parse_in->ptr++;
						else
							debug++;
						getWord(parse_in);
						VariableListEntry* test2;
						char index2;
						if (get_ArgumentListEntry(&test2, &index2, parse_in->word, compiler_var->f_list_base->last->argument, compiler_var->f_list_base->last->argc)) {
							if (test2->type == fp16_enum) {
								test2->reg_index = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
							}
							else {
								test2->reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							}
							memory_load(parse_out, test2, 11, test2, index2, logical_PC, reg_table, parse_in, l_control->depth, param, Masm);
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {// skip

							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
								debug++;
								parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								INT64 num;
								if (get_integer(&num, parse_in->word)) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %d//\" \n", logical_PC[0], reg_table->entry_x[test2->reg_index].name, reg_table->entry_x[test2->reg_index].name, num); logical_PC[0] += 4;
								}
								else {
									debug++;
								}
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
						if (test->type == uint8_enum) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s \n", logical_PC[0], reg_table->entry_x[test->reg_index].name, reg_table->entry_x[test->reg_index].name, reg_table->entry_x[test2->reg_index].name); logical_PC[0] += 4;
							memory_load(parse_out, &target, test->reg_index, &target, 0, logical_PC, reg_table, parse_in, l_control->depth, param, Masm);
							reg_table->entry_x[test->reg_index].in_use = 0;
							reg_table->entry_x[test2->reg_index].in_use = 0;
						}
						else {
							debug++;
						}
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // adjust to 64b vector index\n", logical_PC[0], reg_table->entry_x[target.reg_index].name, reg_table->entry_x[target.reg_index].name); logical_PC[0] += 4;
					}
					else {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // adjust to 64b vector index\n", logical_PC[0], reg_table->entry_x[target.reg_index].name, reg_table->entry_x[test->reg_index].name); logical_PC[0] += 4;
						if (test->type == uint8_enum) {
							reg_table->entry_x[test->reg_index].in_use = 0;
						}
						else {
							reg_table->entry_fp[test->reg_index].in_use = 0;
						}
					}
					INT64 address = logical_PC[0] + ((6 + 2 + 1) * 4);
					if (address & 4) {
						address += 4;
					}
					UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, logical_PC, addr, address, (char*)"", reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s // \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, reg_table->entry_x[target.reg_index].name); logical_PC[0] += 4;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x000(%s) // load switch address(%s) \n", logical_PC[0], reg_table->entry_x[target.reg_index].name, reg_table->entry_x[addr].name, current->name); logical_PC[0] += 4;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x jalr zero, 0(%s)  // jump to case to be executed \n", logical_PC[0], reg_table->entry_x[target.reg_index].name); logical_PC[0] += 4;
					while (logical_PC[0] != address) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi zero, zero, 0 // nop \n", logical_PC[0]); logical_PC[0] += 4;
					}
					l_control->switch_base_addr = logical_PC[0];
					for (UINT i = 0; i < 0x10; i++) {// hack, need to debug
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD label_case_%d_%d  \n", logical_PC[0], i, compiler_var->switch_count); logical_PC[0] += 8;
					}
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD label_default_%d  \n", logical_PC[0], compiler_var->switch_count); logical_PC[0] += 8;

					reg_table->entry_x[addr].in_use = 0;
					reg_table->entry_x[target.reg_index].in_use = 0;
				}
				else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->global_list_base)) {// global_list_base
					debug++;
					if (current->pointer) {
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
							VariableListEntry offset;
							addVariableEntry(&offset, current->type, (char*)"offset", reg_table, l_control, parse_out, parse_in, Masm);
							parse_index_a(parse_out, &offset, 0, logical_PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
							VariableListEntry target;
							addVariableEntry(&target, current->type, current->name, reg_table, l_control, parse_out, parse_in, Masm);
							memory_load(parse_out, &target, offset.reg_index, &target, 0, logical_PC, reg_table, parse_in, l_control->depth, param, Masm);
							reg_table->entry_x[offset.reg_index].in_use = 0;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // adjust index to 64b target (jump address) \n", logical_PC[0], reg_table->entry_x[target.reg_index].name, reg_table->entry_x[target.reg_index].name); logical_PC[0] += 4;
							INT64 address = logical_PC[0] + ((6 + 2 + 1) * 4);
							if (address & 4) {
								address += 4;
							}
							UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, addr, address, (char*)"", reg_table, l_control->depth, parse_in, param, Masm);

							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s // %s base address + %s \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, reg_table->entry_x[target.reg_index].name, current->name, current->name); logical_PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x000(%s) // load switch address(%s) \n", logical_PC[0], reg_table->entry_x[target.reg_index].name, reg_table->entry_x[addr].name, current->name); logical_PC[0] += 4;

							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x jalr zero, 0(%s)  // jump to case to be executed \n", logical_PC[0], reg_table->entry_x[target.reg_index].name); logical_PC[0] += 4;
							if (logical_PC[0] & 4) {
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi zero, zero, 0 // nop \n", logical_PC[0]); logical_PC[0] += 4;
							}
							for (UINT i = 0; i < 0x10; i++) {// hack, need to debug
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD label_case_%d_%d  \n", logical_PC[0], i, compiler_var->switch_count); logical_PC[0] += 8;
							}
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD label_default_%d  \n", logical_PC[0], compiler_var->switch_count); logical_PC[0] += 8;

							reg_table->entry_x[addr].in_use = 0;
							reg_table->entry_x[target.reg_index].in_use = 0;
						}
						else {
							debug++;
						}
					}
					else {
						debug++;// not coded yet
					}
				}
				else {
					debug++;
				}
			}
			break;
			case 3:// case
			{
				INT64 num;
				if (get_integer(&num, parse_in->word)) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD 0x%016I64x // for switch jump target table\n", l_control->switch_base_addr + (num * 8), logical_PC[0]);
				}
				else {
					debug++;
				}
			}
			break;
			case 4:// default
	//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_default // for switch jump target table\n", logical_PC[0]);
				sprint_label(parse_out, logical_PC[0], (char*)"label_default // for switch jump target table", (char*)"", param);
				if (logical_PC[0] - l_control->switch_base_addr >= 0x1000000) // jump exceeds 20b
					debug++;
				break;
			case 5: {
				UINT8 s1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_switch_end_%d \n", logical_PC[0], compiler_var->switch_count); logical_PC[0] += 4;
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x nop 0x%02x // make space if >20b jump\n", logical_PC[0], s1); logical_PC[0] += 4;
				reg_table->entry_x[s1].in_use = 0;
			}
				  break;
			case 1:// for 
			{
				l_control->depth++;
				sprintf_s(parse_out->line[parse_out->index++].line, "// current depth = %d.%d // plus 7 \n", l_control->depth, l_control->count[l_control->depth]);
				// variable initialization
				if (get_VariableTypeNameB(compiler_var->list_base, parse_in, reg_table, l_control, parse_out, Masm)) {
					VariableListEntry* current = compiler_var->list_base->last;
					l_control->for_loop[l_control->depth].index_reg = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					INT64 init_number = 0;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
						parse_in->ptr++;
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						getWord(parse_in);
						if (get_integer(&init_number, parse_in->word)) {	// optimization - pull out array base addresses here
							char word[0x80];
							sprintf_s(word, "initialize variable %s = %s", current->name, parse_in->word); 
							if (init_number < 0x800 && init_number > -0x800) {
						//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%03x // initialize variable %s = %s \n", logical_PC[0], reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, init_number, current->name, parse_in->word); logical_PC[0] += 4;
								sprint_op_2src(parse_out, logical_PC, (char*)"addi", reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, reg_table->entry_x[0].name, init_number,word,param);
							}
							else {
//								UINT8 temp = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
//								load_64bB(parse_out, logical_PC, temp, init_number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
//								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0 // initialize variable %s = %s \n", logical_PC[0], reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, reg_table->entry_x[temp].name, current->name, parse_in->word); logical_PC[0] += 4;
								load_64bB(parse_out, logical_PC, l_control->for_loop[l_control->depth].index_reg, init_number, word, reg_table, l_control->depth, parse_in, param, Masm);
							}
							l_control->for_loop[l_control->depth].detected = 1;
							sprintf_s(l_control->for_loop[l_control->depth].index_name, "%s", current->name);
						}
						else {
							debug++;//coding error
						}
					}
					else {
						debug++;
					}
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ',') {
						parse_in->ptr++;
						if (get_VariableTypeName2B(compiler_var->list_base, parse_in, l_control->depth, current->type)) {
							VariableListEntry* current2 = compiler_var->list_base->last;
							load_64bB(parse_out, logical_PC, current2->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
								UINT8 num_valid = 0;
								INT64 number;
								// ERROR: logical error, where does target go 
								parse_rh_of_eqB(parse_out, current2, 0, &num_valid, &number, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
					}

					sprintf_s(l_control->line[l_control->depth],"%s", parse_in->line[parse_in->index].line);
					l_control->line_valid[l_control->depth] = 1;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
						current->depth = l_control->depth;
						if (parse_in->index == 0x0181)
							debug++;
						parse_in->ptr++;
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						getWord(parse_in);
						VariableListEntry* test;
						if (find_loop_reg_index(l_control, parse_in->word)) {
							UINT8 branch_match;
							if (getBranchB(&branch_match, parse_in, branch_table)) {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								INT64 number_0;
								UINT8 index;
								VariableListEntry* limit_var;
								l_control->for_loop[l_control->depth].limit_reg_valid = 0;
								l_control->for_loop[l_control->depth].branch_match = branch_match;
								if (get_integer(&number_0, parse_in->word)) {	// optimization - pull out array base addresses here
									l_control->for_loop[l_control->depth].limit_reg = alloc_jalr_UABI(reg_table, l_control->depth, parse_out, Masm);
									l_control->for_loop[l_control->depth].limit = number_0;
									if (number_0 < 0x800) {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%03x // load count limit\n", logical_PC[0], reg_table->entry_x[l_control->for_loop[l_control->depth].limit_reg].name, l_control->for_loop[l_control->depth].limit); logical_PC[0] += 4;
									}
									else {
										load_64bB(parse_out, logical_PC, l_control->for_loop[l_control->depth].limit_reg, number_0, (char*)"", reg_table, l_control->depth, parse_in, param, Masm);
									}
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, label_for%d_%d // exit for loop\n", logical_PC[0], branch_table[branch_match].opcode, reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, reg_table->entry_x[l_control->for_loop[l_control->depth].limit_reg].name, l_control->depth, l_control->count[l_control->depth]); logical_PC[0] += 4;

									UINT8 s1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_end%d_%d \n", logical_PC[0], l_control->depth, l_control->end[l_control->depth]); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x nop 0x%02x // make space if >20b jump\n", logical_PC[0], s1); logical_PC[0] += 4;
									reg_table->entry_x[s1].in_use = 0;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_for%d_%d	//for loop return address 0\n", logical_PC[0], l_control->depth, l_control->count[l_control->depth]);
								}
								else if (get_VariableListEntryB(&limit_var, parse_in->word, compiler_var->list_base)) {

									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, label_for%d_%d // exit for loop\n", logical_PC[0], branch_table[branch_match].opcode, reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, reg_table->entry_x[limit_var->reg_index].name, l_control->depth, l_control->count[l_control->depth]); logical_PC[0] += 4;
									l_control->for_loop[l_control->depth].limit_reg = limit_var->reg_index;
									l_control->for_loop[l_control->depth].limit_reg_valid = 1;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_for_exit%d_%d // exit for loop - reg\n", logical_PC[0], l_control->depth, l_control->count[l_control->depth]); logical_PC[0] += 4;

									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_for%d_%d	//for loop return address 0\n", logical_PC[0], l_control->depth, l_control->count[l_control->depth]);

								}
								else if (get_reg_indexB(&index, parse_in->word, reg_table)) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_for%d_%d	//for loop return address 2\n", logical_PC[0], l_control->depth, l_control->count[l_control->depth]);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, label%d_%d\n", logical_PC[0], branch_table[branch_match].rev_opcode, reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, reg_table->entry_x[index].name, l_control->depth, l_control->count[l_control->depth]); logical_PC[0] += 4;
									l_control->for_loop[l_control->depth].limit_reg_valid = 1;
									l_control->for_loop[l_control->depth].limit_reg = 10;// a0
								}
								else {
									debug++;
								}
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
									parse_in->ptr++;
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
									getWord(parse_in);
									if (strcmp(parse_in->word, l_control->for_loop[l_control->depth].index_name) == 0) {
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '+') {
											l_control->for_loop[l_control->depth].increment = 1;
										}
										else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
											parse_in->ptr += 2;
											while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
											getWord(parse_in);
											if (get_integer(&number_0, parse_in->word)) {
												l_control->for_loop[l_control->depth].increment = number_0;
											}
											else {
												debug++;
											}
										}
										else {
											debug++;
										}
									}
								}
								else {
									debug++;
								}
							}
						}
						else if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
							UINT8 branch_match;
							if (getBranchB(&branch_match, parse_in, branch_table)) {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								INT64 number_0;
								UINT8 index;
								VariableListEntry* limit_var;
								l_control->for_loop[l_control->depth].limit_reg_valid = 0;
								l_control->for_loop[l_control->depth].branch_match = branch_match;

								if (get_integer(&number_0, parse_in->word)) {	// optimization - pull out array base addresses here

									INT64 increment = 1;
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
										parse_in->ptr++;
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
										getWord(parse_in);
										if (strcmp(parse_in->word, test->name) == 0) {
											if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
												parse_in->ptr += 2;
												while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
												getWord(parse_in);
												if (get_integer(&increment, parse_in->word)) {
												}
												else {
													debug++;
												}
											}
											else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '+') {
												increment = 1;
											}
											else {
												debug++;
											}
											l_control->for_loop[l_control->depth].increment = increment;
										}
										else {
											debug++;
										}
									}
									else {
										debug++;
									}
									parse_struct2 parse_in2;
									parse_in2.line = parse_in->line;
									parse_in->ptr = 0;
									parse_in2.index = parse_in->index + 1;
									inner_loop_prep(parse_in, &parse_in2, logical_PC, current, l_control, reg_table, compiler_var, parse_out, param, Masm);

									l_control->for_loop[l_control->depth].limit = number_0;

									UINT8 limit = alloc_jalr_UABI(reg_table, l_control->depth, parse_out, Masm);
									if (number_0 < 0x800) {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%03x // load count limit\n", logical_PC[0], reg_table->entry_x[limit].name, l_control->for_loop[l_control->depth].limit); logical_PC[0] += 4;
									}
									else {
										load_64bB(parse_out, logical_PC, limit, number_0, (char*) "", reg_table, l_control->depth, parse_in, param, Masm);
									}
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, label_for%d_%d // exit for loop\n", logical_PC[0], branch_table[branch_match].rev_opcode, reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, reg_table->entry_x[limit].name, l_control->depth, l_control->count[l_control->depth]); logical_PC[0] += 4;

									UINT8 s1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_end%d_%d \n", logical_PC[0], l_control->depth, l_control->end[l_control->depth]); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x nop 0x%02x // make space if >20b jump\n", logical_PC[0], s1); logical_PC[0] += 4;
									reg_table->entry_x[s1].in_use = 0;

									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_for%d_%d	//for loop return address 0\n", logical_PC[0], l_control->depth, l_control->count[l_control->depth]);

									for_inner_loopB(parse_out, parse_in, &parse_in2, init_number, 0x080, logical_PC, test, control_type, modifier_type,
										reg_table, memory_space, pointers, base, compiler_var, branch_match, branch_table, op_table, csr_list, csr_list_count, l_control, sp_offset, current_function, IO_list, param, Masm, unit_debug);
								}
								else if (get_VariableListEntryB(&limit_var, parse_in->word, compiler_var->list_base)) {

									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, label_for%d_%d // exit for loop\n", logical_PC[0], branch_table[branch_match].rev_opcode, reg_table->entry_x[l_control->for_loop[l_control->depth].index_reg].name, reg_table->entry_x[limit_var->reg_index].name, l_control->depth, l_control->count[l_control->depth]); logical_PC[0] += 4;
									l_control->for_loop[l_control->depth].limit_reg = limit_var->reg_index;
									l_control->for_loop[l_control->depth].limit_reg_valid = 1;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x j label_for_exit%d_%d // exit for loop - reg\n", logical_PC[0], l_control->depth, l_control->count[l_control->depth]); logical_PC[0] += 4;

									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_for%d_%d	//for loop return address 0\n", logical_PC[0], l_control->depth, l_control->count[l_control->depth]);

									if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
										parse_in->ptr++;
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
										getWord(parse_in);
										if (strcmp(parse_in->word, l_control->for_loop[l_control->depth].index_name) == 0) {
											if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '+')
												l_control->for_loop[l_control->depth].increment = 1;
											else
												debug++;
										}
										else {
											debug++;
										}
									}
									else {
										debug++;
									}

									parse_struct2 parse_in2;
									parse_in2.line = parse_in->line;
									parse_in->ptr = 0;
									parse_in2.index = parse_in->index + 1;
									inner_loop_prep(parse_in, &parse_in2, logical_PC, current, l_control, reg_table, compiler_var, parse_out, param, Masm);

									for_inner_loopB(parse_out, parse_in, &parse_in2, init_number, 0x080, logical_PC, test, control_type, modifier_type,
										reg_table, memory_space, pointers, base, compiler_var, branch_match, branch_table, op_table, csr_list, csr_list_count, l_control, sp_offset, current_function, IO_list, param, Masm, unit_debug);
								}
								else if (get_reg_indexB(&index, parse_in->word, reg_table)) {
									UINT8 current_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									load_64bB(parse_out, logical_PC, current_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_for%d_%d	//for loop return address 2\n", logical_PC[0], l_control->depth, l_control->count[l_control->depth]);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, label%d_%d\n", logical_PC[0], branch_table[branch_match].opcode, reg_table->entry_x[current_index].name, reg_table->entry_x[index].name, l_control->depth, l_control->count[l_control->depth]); logical_PC[0] += 4;
									reg_table->entry_x[current_index].in_use = 0;
								}
								else {
									debug++;
								}
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
									parse_in->ptr++;
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
									getWord(parse_in);
									if (strcmp(parse_in->word, l_control->for_loop[l_control->depth].index_name) == 0) {
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '+') {
											l_control->for_loop[l_control->depth].increment = 1;
										}
									}
								}
								else {
									debug++;
								}
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
					}
					else {
						debug++;
					}
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\n' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\0') {
					debug++;					// end of line; no error
				}
				else {
					debug++;
					VariableListEntry* test;
					if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
					}
				}
			}
			break;
			case 6://return
			{
				VariableListEntry* test;
				if (get_VariableListEntryB(&test, parse_in->word, compiler_var->list_base)) {
					UINT8 test_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, logical_PC, test_index, test->addr, test->name, reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi a0, %s, 0		// load return response (a0)\n", logical_PC[0], reg_table->entry_x[test_index].name); logical_PC[0] += 4;// parameter list, needs work
					reg_table->entry_x[test_index].in_use = 0;
				}
				else {
					debug++;
				}
			}
			break;
			case 7:// while
			{
				UINT8 reg_index = 0;
				UINT8 branch_match;
				UINT8 regA = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_while%d_%d // return address for while loop\n", logical_PC[0], l_control->depth, l_control->count[l_control->depth]);
				if (match_listB(&reg_index, parse_in->word, IO_list->name, IO_list->ptr, logical_PC)) {
					UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, logical_PC, addr, IO_list->addr[reg_index], IO_list->name[reg_index].name, reg_table, l_control->depth, parse_in, param, Masm);
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lr.w.aq.rl %s, 0(%s) \n", logical_PC[0], reg_table->entry_x[regA].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
					UINT8 regB = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					if (getOperatorB(&branch_match, parse_in, branch_table)) {
						UINT8 regB = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						getWord(parse_in);
						INT64 num;
						if (get_integer(&num, parse_in->word)) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%x \n", logical_PC[0], reg_table->entry_x[regB].name, num); logical_PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, label_while%d_%d \n", logical_PC[0], branch_table[branch_match].rev_opcode, reg_table->entry_x[regA].name, reg_table->entry_x[regB].name, l_control->depth, l_control->count[l_control->depth]); logical_PC[0] += 4;
						}
						else {
							debug++;
						}
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')parse_in->ptr++;
					}
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '}') {
						parse_in->ptr++;
						l_control->count[l_control->depth]++;
						l_control->depth--;
						sprintf_s(parse_out->line[parse_out->index++].line, "// current depth = %d.%d // plus 5 \n", l_control->depth, l_control->count[l_control->depth]);
						reg_maintanence(reg_table, compiler_var, l_control, parse_out, logical_PC, parse_in,param);
					}
				}
				else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->global_list_base)) {
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
						parse_in->ptr++;
						getWord(parse_in);
						VariableListEntry* current2;
						if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
							UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							UINT8 temp = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							if (current->reg_index_valid == 0) {
								current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								load_64bB(parse_out, logical_PC, current->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
								current->reg_index_valid = 1;
							}
							if (current2->reg_index_valid == 0) {
								current2->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								load_64bB(parse_out, logical_PC, current2->reg_index, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
								current2->reg_index_valid = 1;
							}
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1 \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[current2->reg_index].name); logical_PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
							//							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s) \n", logical_PC[0], reg_table->entry_x[regA].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s) \n", logical_PC[0], reg_table->entry_x[temp].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;

							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, zero, %s \n", logical_PC[0], reg_table->entry_x[regA].name, reg_table->entry_x[temp].name); logical_PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // negative number if not 0\n", logical_PC[0], reg_table->entry_x[regA].name, reg_table->entry_x[regA].name, reg_table->entry_x[temp].name); logical_PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x srli %s, %s, 31 // set to -1 if not 0  \n", logical_PC[0], reg_table->entry_x[regA].name, reg_table->entry_x[regA].name); logical_PC[0] += 4;

							reg_table->entry_x[temp].in_use = 0;
							reg_table->entry_x[addr].in_use = 0;
							if (current->pointer) {
								reg_table->entry_x[current->reg_index].in_use = 0;
								current->reg_index_valid = 0;
							}
							if (current2->pointer) {
								reg_table->entry_x[current2->reg_index].in_use = 0;
								current2->reg_index_valid = 0;
							}
						}
						else {
							debug++;
						}
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
							parse_in->ptr++;
						}
						else {
							debug++;
						}
					}
					else {
						debug++;
					}
				}
				else {
					debug++;
				}
				while (getOperatorB(&op_match, parse_in, op_table)) {
					UINT8 not_flag = 0;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '!') {
						not_flag = 1;
						parse_in->ptr++;
					}
					getWord(parse_in);
					VariableListEntry* current2;
					if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
						UINT8 regB = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, zero, %s \n", logical_PC[0], reg_table->entry_x[regB].name, reg_table->entry_x[current2->reg_index].name); logical_PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x or %s, %s, %s // negative number if not 0 \n", logical_PC[0], reg_table->entry_x[regB].name, reg_table->entry_x[regB].name, reg_table->entry_x[current2->reg_index].name); logical_PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, -31 // set to -1 if not 0  \n", logical_PC[0], reg_table->entry_x[regB].name, reg_table->entry_x[regB].name); logical_PC[0] += 4;
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 1 // set to 1 if not zero  \n", logical_PC[0], reg_table->entry_x[regB].name, reg_table->entry_x[regB].name); logical_PC[0] += 4;
						if (not_flag) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s, 1 // set to 1 if zero, else 0  \n", logical_PC[0], reg_table->entry_x[regB].name, reg_table->entry_x[regB].name); logical_PC[0] += 4;
						}
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // accumulate result \n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[regA].name, reg_table->entry_x[regA].name, reg_table->entry_x[regB].name); logical_PC[0] += 4;
						reg_table->entry_x[regB].in_use = 0;
					}
					else {
						debug++;
					}
				}
				//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x beq %s, zero, label%d_%d // loop exit condition \n", logical_PC[0],  reg_table->entry_x[regA].name, l_control->depth, l_control->count[l_control->depth]); logical_PC[0] += 4;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')parse_in->ptr++;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '}') {
					parse_in->ptr += 2;
					sprintf_s(parse_out->line[parse_out->index++].line, "// current depth = %d.%d // plus 5 \n", l_control->depth, l_control->count[l_control->depth]);
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
					l_control->for_loop[l_control->depth].detected = 2;
					l_control->depth++;
					sprintf_s(parse_out->line[parse_out->index++].line, "// current depth = %d.%d // plus 1 \n", l_control->depth, l_control->count[l_control->depth]);
				}
				else {
					debug++;
				}
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x beq %s, zero, label%d_%d // loop exit condition \n", logical_PC[0], reg_table->entry_x[regA].name, l_control->depth, l_control->count[l_control->depth]); logical_PC[0] += 4;
				reg_table->entry_x[regA].in_use = 0;
			}
			break;
			default:
				debug++;
				break;
			}
		}
		else if (match_listB(&csr_index, parse_in->word, csr_list, csr_list_count, logical_PC)) {
			UINT8 num_valid = 0;
			INT64 number2;
			if (getOperatorB(&op_match, parse_in, op_table)) {
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '~') {// clear
					parse_in->ptr++;
					getWord(parse_in);
					INT64 number;
					if (get_integer(&number, parse_in->word)) {

						UINT8 top = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, logical_PC, top, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);

						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrc %s, %s, %s\n", logical_PC[0], reg_table->entry_x[0].name, reg_table->entry_x[top].name, csr_list[csr_index]); logical_PC[0] += 4;
						reg_table->entry_x[top].in_use = 0;
					}
					else {// not coded 
						debug++;
					}
				}
				else {//set
					getWord(parse_in);
					INT64 number;
					if (get_integer(&number, parse_in->word)) {
						if (number < 32) {
	//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrsi %s, 0x%02x, %s\n", logical_PC[0], reg_table->entry_x[0].name, number, csr_list[csr_index]); logical_PC[0] += 4;
							sprint_csr_imm(parse_out, logical_PC, (char*)"csrrsi", reg_table->entry_x[0].name, number, csr_list[csr_index].name, param);
						}
						else {
							UINT8 top = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, top, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);

			//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrs %s, %s, %s\n", logical_PC[0], reg_table->entry_x[0].name, reg_table->entry_x[top].name, csr_list[csr_index]); logical_PC[0] += 4;
							sprint_op_2src(parse_out, logical_PC, (char*)"csrrs", reg_table->entry_x[0].name, reg_table->entry_x[top].name, csr_list[csr_index].name, param);
							reg_table->entry_x[top].in_use = 0;
						}
					}
					else {// not coded
						//					UINT8 top = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						VariableListEntry top;
						top.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						top.reg_index_valid = 1;
						sprintf_s(top.name, "");
						top.type = int64_enum;
						parse_rh_of_eqB(parse_out, &top, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrs %s, %s, %s // set bits (equivalent to OR instruction on csr reg) \n", logical_PC[0], reg_table->entry_x[0].name, reg_table->entry_x[top.reg_index].name, csr_list[csr_index]); logical_PC[0] += 4;
						reg_table->entry_x[top.reg_index].in_use = 0;
						top.reg_index_valid = 0;
					}
				}
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
				parse_in->ptr++;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ')parse_in->ptr++;
				getWord(parse_in);
				INT64 number;
				UINT8 csr_index2 = 0;
				VariableListEntry* current = compiler_var->list_base->next;
				if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '&') {
						parse_in->ptr++;
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						UINT8 xor_flag = 0;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '~') {
							xor_flag = 1;
							parse_in->ptr++;
						}
						char word[0x80];
						strcpy_word(word, parse_in);
						getWord(parse_in);
						if (get_integer(&number, parse_in->word)) {
							if (number < 0x800) {
								UINT8 reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								if (xor_flag) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, zero, 0x%03x\n", logical_PC[0], reg_table->entry_x[reg_index].name,  number); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s,-1\n", logical_PC[0], reg_table->entry_x[reg_index].name, reg_table->entry_x[reg_index].name, number); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s\n", logical_PC[0], reg_table->entry_x[reg_index].name, reg_table->entry_x[reg_index].name, csr_list[current->reg_index]); logical_PC[0] += 4;
								}
								else {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 0x%03x\n", logical_PC[0], reg_table->entry_x[reg_index].name,  csr_list[current->reg_index], number); logical_PC[0] += 4;
								}
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrw %s, 0x000, %s\n", logical_PC[0], reg_table->entry_x[reg_index].name, csr_list[csr_index]); logical_PC[0] += 4;
							}
							else {
								UINT8 reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								load_64bB(parse_out, logical_PC, reg_index, number,(char*) " ", reg_table, l_control->depth, parse_in, param, Masm);
								if (xor_flag) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x xori %s, %s,-1\n", logical_PC[0], reg_table->entry_x[reg_index].name, reg_table->entry_x[reg_index].name); logical_PC[0] += 4;
								}
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x and %s, %s, %s\n", logical_PC[0], reg_table->entry_x[reg_index].name, reg_table->entry_x[reg_index].name, reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrw zero, %s, %s\n", logical_PC[0], reg_table->entry_x[reg_index].name, csr_list[csr_index]); logical_PC[0] += 4;
							}
						}
						else {
							debug++;
						}
					}
					else {
						if (current->reg_index_valid == 0)
							debug++;
				//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrw %s, 0x000, %s\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, csr_list[csr_index]); logical_PC[0] += 4;
						sprint_csr_imm(parse_out, logical_PC, (char*)"csrrwi", reg_table->entry_x[current->reg_index].name, 0, csr_list[csr_index].name, param);
					}
				}
				else if (get_reg_indexB(&reg_index, parse_in->word, reg_table)) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrw zero, %s, %s\n", logical_PC[0], reg_table->entry_x[reg_index].name, csr_list[csr_index].name); logical_PC[0] += 4;
				}
				else if (get_integer(&number, parse_in->word)) {
					if (number == 0) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrw zero, zero, %s\n", logical_PC[0], csr_list[csr_index].name); logical_PC[0] += 4;
					}
					else {
						UINT8 top = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);

						load_64bB(parse_out, logical_PC, top, number, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
			//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrw zero, %s, %s\n", logical_PC[0], reg_table->entry_x[top].name, csr_list[csr_index].name); logical_PC[0] += 4;
						sprint_op_2src(parse_out, logical_PC, (char*)"csrrw", reg_table->entry_x[0].name, reg_table->entry_x[top].name, csr_list[csr_index].name, param);
						reg_table->entry_x[top].in_use = 0;
					}
				}
				else if ((parse_in->line[parse_in->index].line[parse_in->ptr] == '(')) {
					VariableListEntry right;
					right.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					right.reg_index_valid = 1;
					sprintf_s(right.name, "");
					right.type = int64_enum;
					parse_rh_of_eqB(parse_out, &right, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
	//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrwi %s, 0x00, %s \n", logical_PC[0], reg_table->entry_x[right.reg_index].name, csr_list[csr_index]); logical_PC[0] += 4;
					sprint_csr_imm(parse_out, logical_PC, (char*)"csrrwi", reg_table->entry_x[right.reg_index].name, 0, csr_list[csr_index].name, param);
					reg_table->entry_x[right.reg_index].in_use = 0;
				}
				else if (match_listB(&csr_index2, parse_in->word, csr_list, csr_list_count, logical_PC)) {
					UINT8 csr_copy = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrci %s, 0x00, %s \n", logical_PC[0], reg_table->entry_x[csr_copy].name, csr_list[csr_index2]); logical_PC[0] += 4;
					sprint_csr_imm(parse_out, logical_PC, (char*)"csrrci", reg_table->entry_x[csr_copy].name, 0, csr_list[csr_index2].name, param);
					if (getOperatorB(&op_match, parse_in, op_table)) {
						VariableListEntry right;
						right.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						right.reg_index_valid = 1;
						sprintf_s(right.name, "");
						right.type = int64_enum;
						parse_rh_of_eqB(parse_out, &right, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s \n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[csr_copy].name, reg_table->entry_x[csr_copy].name, reg_table->entry_x[right.reg_index].name); logical_PC[0] += 4;
						reg_table->entry_x[right.reg_index].in_use = 0;
					}
					else {
						debug++;
					}
//					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrwi %s, 0x00, %s \n", logical_PC[0], reg_table->entry_x[csr_copy].name, csr_list[csr_index]); logical_PC[0] += 4;
					sprint_csr_imm(parse_out, logical_PC, (char*)"csrrwi", reg_table->entry_x[csr_copy].name, 0, csr_list[csr_index].name, param);
					reg_table->entry_x[csr_copy].in_use = 0;
				}
				else {
					debug++;
				}
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] != ';') {
					VariableListEntry right;
					right.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					right.reg_index_valid = 1;
					sprintf_s(right.name, "");
					right.type = int64_enum;
					if (getOperatorB(&op_match, parse_in, op_table)) {
						parse_rh_of_eqB(parse_out, &right, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s \n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[reg_index].name, reg_table->entry_x[reg_index].name, reg_table->entry_x[right.reg_index].name); logical_PC[0] += 4;
					}
					else {
						parse_rh_of_eqB(parse_out, &right, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrw %s, %s, %s \n", logical_PC[0], reg_table->entry_x[0].name, reg_table->entry_x[right.reg_index].name, csr_list[csr_index]); logical_PC[0] += 4;
						debug++;
					}
					reg_table->entry_x[right.reg_index].in_use = 0;
				}
			}
			else {
				debug++; //syntax
			}
		}//get_reg_index
		else if (get_reg_indexB(&reg_index, parse_in->word, reg_table)) {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
				parse_in->ptr++;
				getWord(parse_in);
				INT64 number;
				if (get_integer(&number, parse_in->word)) {
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
						parse_in->ptr++;
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
							parse_in->ptr++;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							UINT8 ptr_hold = parse_in->ptr;
							getWord(parse_in);
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
								UINT8 reg_index2;
								INT64 number2;
								if (get_reg_indexB(&reg_index2, parse_in->word, reg_table)) {
									switch (param->mxl) {
									case 1:
								//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sw %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[reg_index2].name, -((number + 1) * 4), reg_table->entry_x[reg_index].name); logical_PC[0] += 4;
										sprint_load(parse_out, logical_PC, (char*)"sw", reg_table->entry_x[reg_index2].name, -((number + 1) * 4), reg_table->entry_x[reg_index].name, (char*)"", param);
										break;
									case 2:
								//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[reg_index2].name, -((number + 1) * 8), reg_table->entry_x[reg_index].name); logical_PC[0] += 4;
										sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[reg_index2].name, -((number + 1) * 8), reg_table->entry_x[reg_index].name, (char*)"", param);
										break;
									case 3:
								//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sq %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[reg_index2].name, -((number + 1) * 16), reg_table->entry_x[reg_index].name); logical_PC[0] += 4;
										sprint_load(parse_out, logical_PC, (char*)"sq", reg_table->entry_x[reg_index2].name, -((number + 1) * 16), reg_table->entry_x[reg_index].name, (char*)"", param);
										break;
									default:
								//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sh %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[reg_index2].name, -((number + 1) * 2), reg_table->entry_x[reg_index].name); logical_PC[0] += 4;
										sprint_load(parse_out, logical_PC, (char*)"sh", reg_table->entry_x[reg_index2].name, -((number + 1) * 2), reg_table->entry_x[reg_index].name, (char*)"", param);
										break;
									}
								}
								else if (get_integer(&number2, parse_in->word)) {
									if (number2 == 0) {
										switch (param->mxl) {
										case 1:
	//										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sw zero, %d(%s) \n", logical_PC[0],  -((number + 1) * 4), reg_table->entry_x[reg_index].name); logical_PC[0] += 4;
											sprint_load(parse_out, logical_PC, (char*)"sw", reg_table->entry_x[0].name, -((number + 1) * 4), reg_table->entry_x[reg_index].name, (char*)"", param);
											break;
										case 2:
	//										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd zero, %d(%s) \n", logical_PC[0], -((number + 1) * 8), reg_table->entry_x[reg_index].name); logical_PC[0] += 4;
											sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[0].name, -((number + 1) * 8), reg_table->entry_x[reg_index].name, (char*)"", param);
											break;
										case 3:
	//										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sq zero, %d(%s) \n", logical_PC[0],  -((number + 1) * 16), reg_table->entry_x[reg_index].name); logical_PC[0] += 4;
											sprint_load(parse_out, logical_PC, (char*)"sq", reg_table->entry_x[0].name, -((number + 1) * 16), reg_table->entry_x[reg_index].name,(char*)"", param);
											break;
										default:
//											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sh zero, %d(%s) \n", logical_PC[0],  -((number + 1) * 2), reg_table->entry_x[reg_index].name); logical_PC[0] += 4;
											sprint_load(parse_out, logical_PC, (char*)"sh", reg_table->entry_x[0].name, -((number + 1) * 2), reg_table->entry_x[reg_index].name, (char*)"", param);
											break;
										}
									}
									else {
										debug++;
									}
								}
								else {
									debug++;
								}
							}
							else {
								parse_in->ptr = ptr_hold;
								VariableListEntry data;
								data.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								data.reg_index_valid = 1;
								sprintf_s(data.name, "data");
								data.type = int64_enum;
								UINT8 num_valid;
								INT64 number2;
								parse_rh_of_eqB(parse_out, &data, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
								if (number < 0x80) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[data.reg_index].name, -((number + 1) * 8), reg_table->entry_x[reg_index].name); logical_PC[0] += 4;
									l_control->fence[l_control->depth] = 1;
								}
								else {
									debug++;
								}
							}
						}
					}
					else {
						debug++;
					}
				}
				else if (find_loop_reg_index(l_control, parse_in->word)) {
					UINT l_control_index = l_control->index;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
						parse_in->ptr++;
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
							parse_in->ptr++;
							VariableListEntry data;
							data.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							data.reg_index_valid = 1;
							sprintf_s(data.name, "data");
							data.type = int64_enum;
							UINT8 num_valid;
							INT64 number2;
							parse_rh_of_eqB(parse_out, &data, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
							UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // %s<<3 \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[l_control->for_loop[l_control_index].index_reg].name, l_control->for_loop[l_control_index].index_name); logical_PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[reg_index].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, -8(%s) \n", logical_PC[0], reg_table->entry_x[data.reg_index].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
							l_control->fence[l_control->depth] = 1;
							reg_table->entry_x[addr].in_use = 0;
							reg_table->entry_x[data.reg_index].in_use = 0;
						}
					}
					else {
						debug++;
					}
				}
				else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '+') {
						parse_in->ptr += 2;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
							parse_in->ptr++;
						}
						else {
							debug++;
						}
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
							parse_in->ptr++;
						}
						else {
							debug++;
						}
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						getWord(parse_in);
						UINT8 reg_index2;
						if (get_reg_indexB(&reg_index2, parse_in->word, reg_table)) {
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
								parse_in->ptr++;
								UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1 \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
								if (param->mxl == 3) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 4 // adjust for 128b entries\n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[reg_index].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sq %s 0x00(%s)\n", logical_PC[0], reg_table->entry_x[reg_index2].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
								}
								else if (param->mxl == 2) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // adjust for 64b entries\n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[reg_index].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s 0x00(%s)\n", logical_PC[0], reg_table->entry_x[reg_index2].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
								}
								else if (param->mxl == 1) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 2 // adjust for 32b entries\n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[reg_index].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sw %s 0x00(%s)\n", logical_PC[0], reg_table->entry_x[reg_index2].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
								}
								else  {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 1 // adjust for 16b entries\n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[reg_index].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sh %s 0x00(%s)\n", logical_PC[0], reg_table->entry_x[reg_index2].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
								}
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
					}
					else {
						debug++;
					}
				}
				else {
					debug++;
				}
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
				parse_in->ptr++;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				UINT8 ptr_hold = parse_in->ptr;

				getWord(parse_in);
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
					parse_in->ptr++;
					UINT8 reg_index2;
					if (get_reg_indexB(&reg_index2, parse_in->word, reg_table)) {
						getWord(parse_in);
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']' && parse_in->line[parse_in->index].line[parse_in->ptr+1] == ';') {
							INT64 number;
							if (get_integer(&number, parse_in->word)) {
								switch (param->mxl) {
								case 1:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lw %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[reg_index].name, -4 * (number + 1), reg_table->entry_x[reg_index2].name); logical_PC[0] += 4;
									break;
								case 2:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[reg_index].name, -8 * (number + 1), reg_table->entry_x[reg_index2].name); logical_PC[0] += 4;
									break;
								case 3:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lq %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[reg_index].name, -16 * (number + 1), reg_table->entry_x[reg_index2].name); logical_PC[0] += 4;
									break;
								default:
									debug++;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lh %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[reg_index].name, -2 * (number + 1), reg_table->entry_x[reg_index2].name); logical_PC[0] += 4;
									break;
								}
							}
						}
						else {
							debug++;
						}
					}
					else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->global_list_base)) {
						getWord(parse_in);
						UINT incr_flag = 0;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '+') {
							parse_in->ptr += 2;
							incr_flag = 1;
						}
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ';') {
							if (current->reg_index_valid == 0) {
								switch (current->type) {
								case uint64_enum:
									current->reg_index_valid = 1;
									current->reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									load_64bB(parse_out, logical_PC, current->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
									break;
								default:
									debug++;
									break;
								}
							}

							INT64 number;
							VariableListEntry *current2;
							if (get_integer(&number, parse_in->word)) {
								switch (current->type) {
								case uint32_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lw %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[reg_index].name, -4 * (number + 1), reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
									break;
								case uint64_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[reg_index].name, -8 * (number + 1), reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
									break;
								case uint128_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lq %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[reg_index].name, -16 * (number + 1), reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
									break;
								default:
									debug++;
		//							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lh %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[reg_index].name, -2 * (number + 1), reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
									break;
								}
							}
							else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
								UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								switch (current->type) {
								case uint32_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lw %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[reg_index].name, -4 * (number + 1), reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
									break;
								case uint64_enum:
									if (incr_flag) {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1 \n", logical_PC[0], reg_table->entry_x[current2->reg_index].name, reg_table->entry_x[current2->reg_index].name); logical_PC[0] += 4;
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[current2->reg_index].name); logical_PC[0] += 4;
									}
									else {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1 \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[current2->reg_index].name); logical_PC[0] += 4;
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
									}
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // fullows stack order\n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x000(%s) \n", logical_PC[0], reg_table->entry_x[reg_index].name, reg_table->entry_x[addr].name); logical_PC[0] += 4;
									break;
								case uint128_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lq %s, %d(%s) \n", logical_PC[0], reg_table->entry_x[reg_index].name, -16 * (number + 1), reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
									break;
								default:
									debug++;
									break;
								}
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
					}
					else {
						debug++;
					}
				}
				else {
					parse_in->ptr = ptr_hold;
					UINT8 num_valid = 0;
					INT64 number2;
					VariableListEntry right;
					right.reg_index = reg_index;
					right.reg_index_valid = 1;
					sprintf_s(right.name, "register load");
					right.type = int64_enum;
					parse_rh_of_eqB(parse_out, &right, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
				}
			}
			else if (getOperatorB(&op_match, parse_in, op_table)) {
				VariableListEntry right;
				right.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				right.reg_index_valid = 1;
				sprintf_s(right.name, "");
				right.type = int64_enum;
				UINT8 num_valid = 0;
				INT64 number2;
				parse_rh_of_eqB(parse_out, &right, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s \n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[reg_index].name, reg_table->entry_x[reg_index].name, reg_table->entry_x[right.reg_index].name); logical_PC[0] += 4;
				reg_table->entry_x[right.reg_index].in_use = 0;
			}
			else {
				debug++;
			}
		}
		else if (get_VariableTypeNameB(compiler_var->list_base, parse_in, reg_table, l_control, parse_out, Masm)) {
			variableDeclarationBody(parse_out, parse_in, l_control, logical_PC, modifier_index, IO_list, reg_table, branch_table, csr_list, csr_list_count,
				op_table, compiler_var, pointers, param, Masm, unit_debug);
			current = compiler_var->list_base->last;
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != ';') {
				INT8 subset_depth = 0;
				parse_s2B(parse_out, current, current->reg_index, &subset_depth, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, logical_PC, reg_table, l_control, param, Masm, unit_debug);
				if (subset_depth != 0)
					debug++;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
			}
		}
		else {
			FunctionListEntry* function_match = compiler_var->f_list_base->next;
			UINT8 pointer_variable = 0;
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
				pointer_variable = 1;
				parse_in->ptr++;
				getWord(parse_in);
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
			}
			if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
					VariableListEntry offset;
					addVariableEntry(&offset, int64_enum, (char *) "offset", reg_table, l_control, parse_out, parse_in, Masm);
					UINT8 number_valid = 0;
					INT64 number;
					parse_index_b(parse_out, &offset, &number, &number_valid, logical_PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
					UINT8 num_valid = 0;
					INT64 number2;
					if (current->reg_index_valid == 0) {
						current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						current->reg_index_valid = 1;
						current->reg_depth = l_control->depth;
						if (current->sp_offset < 0x800) {
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%03x // addr = sp + sp offset \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[2].name, current->sp_offset); logical_PC[0] += 4;
						}
						else {
							debug++;
						}
					}
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
						char hold_ptr = parse_in->ptr;
						parse_in->ptr++;
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '0' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ';') {
							if (current->type == uint8_enum) {
								if (number_valid) {
				//					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sb zero, 0x%03x(%s) // accumulate the result\n", logical_PC[0], -(number + 1), reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
									sprint_load(parse_out, logical_PC, (char*)"sb", reg_table->entry_x[0].name, -(number + 1), reg_table->entry_x[current->reg_index].name, (char*)"accumulate the result", param);
								}
								else {
									debug++;
								}
							}
							else if (current->type == uint64_enum) {
								if (number_valid) {
//									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd zero, 0x%03x(%s) // accumulate the result\n", logical_PC[0], -8*(number + 1), reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
									sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[0].name, -8*(number + 1), reg_table->entry_x[current->reg_index].name, (char*)"accumulate the result", param);
								}
								else {
									debug++;
								}
							}
							else {
								debug++;
							}
						}
						else {
							parse_in->ptr = hold_ptr;
							VariableListEntry target;
							sprintf_s(target.name, "%s", current->name);
							target.type = current->type;
							if (target.type == fp16_enum) {
								target.reg_index = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
							}
							else if (target.type == uint8_enum || target.type == uint64_enum) {
								target.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							}
							else {
								debug++;
							}
							target.reg_index_valid = 1;
							parse_rh_of_eqB(parse_out, &target, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
							while (parse_in->line[parse_in->index].line[parse_in->ptr] != ';') {

								if (getOperatorB(&op_match, parse_in, op_table)) {
									VariableListEntry s2;
									s2.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									s2.reg_index_valid = 1;
									sprintf_s(s2.name, "");
									s2.type = int64_enum;
									parse_rh_of_eqB(parse_out, &s2, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s // accumulate the result\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[target.reg_index].name, reg_table->entry_x[target.reg_index].name, reg_table->entry_x[s2.reg_index].name); logical_PC[0] += 4;
									reg_table->entry_x[s2.reg_index].in_use = 0;
									s2.reg_index_valid = 0;
									num_valid = 0;
								}
							}
							if (number_valid) {// do not use commented out code for this
								if (number < 0x800) {
									if (num_valid && number2 == 0) {
										parse_out->index -= 1; logical_PC[0] -= 4;// reverse previous register load
							//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd zero, %#x(%s)  // store result, assume stack order, 64b b\n", logical_PC[0], -(number + 1) * 8, reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
										sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[0].name, -(number + 1) * 8, reg_table->entry_x[current->reg_index].name, (char*)" store result, assume stack order, 64b b", param);
									}
									else {
										if (target.type == fp16_enum) {
						//					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fsh %s, %#x(%s)  // store result, assume stack order, _fp16 a\n", logical_PC[0], reg_table->entry_fp[target.reg_index].name, -(number + 1) * 2, reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
											sprint_load(parse_out, logical_PC, (char*)"fsh", reg_table->entry_fp[target.reg_index].name, -(number + 1) * 2, reg_table->entry_x[current->reg_index].name, (char*)" store result, assume stack order, _fp16 a", param);
										}
										else {
		//									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, %#x(%s)  // store result, assume stack order, 64b a\n", logical_PC[0], reg_table->entry_x[target.reg_index].name, -(number + 1) * 8, reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
											sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[target.reg_index].name, -(number + 1) * 8, reg_table->entry_x[current->reg_index].name, (char*)" store result, assume stack order, 64b a", param);
										}
									}
								}
								else {
									load_64bB(parse_out, logical_PC, offset.reg_index, number, (char*)"", reg_table, l_control->depth, parse_in, param, Masm);
									UINT8 address = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s  // generate result address location a\n", logical_PC[0], reg_table->entry_x[address].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset.reg_index].name); logical_PC[0] += 4;
									sprint_op_2src(parse_out, logical_PC, (char*)"sub ", reg_table->entry_x[address].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset.reg_index].name, (char*)" generate result address location a", param);
						//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, 0(%s)  // store result b\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[address].name); logical_PC[0] += 4;
									sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[target.reg_index].name,(INT16)0, reg_table->entry_x[address].name, (char*)" store result b", param);
									reg_table->entry_x[address].in_use = 0;
								}
							}
							else {
								UINT8 address = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s  // generate result address location b\n", logical_PC[0], reg_table->entry_x[address].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset.reg_index].name); logical_PC[0] += 4;
								sprint_op_2src(parse_out, logical_PC, (char*)"sub ", reg_table->entry_x[address].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[offset.reg_index].name, (char*)" generate result address location b", param);
					//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, -8(%s)  // store result a\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[address].name); logical_PC[0] += 4;
								sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[target.reg_index].name, (INT16)0, reg_table->entry_x[address].name, (char*)" store result a", param);
								reg_table->entry_x[address].in_use = 0;
							}
							l_control->fence[l_control->depth] = 1;
							if (target.type == fp16_enum) {
								reg_table->entry_fp[target.reg_index].in_use = 0;
							}
							else if (target.type == uint64_enum) {
								reg_table->entry_x[target.reg_index].in_use = 0;
							}
							else {
								debug++;
							}
						}
					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
						if (getOperatorB(&op_match, parse_in, op_table)) {
							parse_rh_of_eqB(parse_out, current, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
							// obvious error, where does saved reg come from
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[reg_table->saved].name); logical_PC[0] += 4;
						}
						else {
							debug++;// syntax
						}
					}
					else {
						debug++;// syntax error??
					}
					if (current->reg_index_valid == 0) {
						reg_table->entry_x[current->reg_index].in_use = 0;
					}
					reg_table->entry_x[offset.reg_index].in_use = 0;
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
					if (pointer_variable) {
						UINT8 current_index = alloc_jalr_UABI(reg_table, l_control->depth, parse_out, Masm);
						load_64bB(parse_out, logical_PC, current_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x jalr zero, 0(%s) \n", logical_PC[0], reg_table->entry_x[current_index].name); logical_PC[0] += 4;
						reg_table->entry_x[current_index].in_use = 0;
					}
					else {
						debug++;
					}
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
					UINT8 num_valid = 0;
					INT64 number2;
					if (current->reg_index_valid == 0) {
						if (current->type == fp16_enum) {
							current->reg_index = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
						}
						else {
							current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							reg_table->entry_x[current->reg_index].saved_depth = current->depth;
						}
						current->reg_index_valid = 1;
						current->reg_depth = l_control->depth;
					}
					parse_rh_of_eqB(parse_out, current, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
					if (getOperatorB(&op_match, parse_in, op_table)) {
						UINT8 num_valid = 0;
						INT64 number, number2;
						if (current->pointer) {
							VariableListEntry* current2;
							getWord(parse_in);
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
							if (get_integer(&number, parse_in->word)) {
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
									if (number < 0x800 && number > -0x800) {
										if (strcmp(op_table[op_match].symbol, "+=") == 0) {
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%03x//  \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name, number); logical_PC[0] += 4;
										}
										else if (strcmp(op_table[op_match].symbol, "-=") == 0) {
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%03x//  \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name, -number); logical_PC[0] += 4;
										}
										else {
											debug++;
										}
									}
									else {
										if (strcmp(op_table[op_match].symbol, "+=") == 0) {
											UINT8 s2 = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
											load_64bB(parse_out, logical_PC, s2, number, (char*)"loop increment", reg_table, l_control->depth, parse_in, param, Masm);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s// \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[s2].name); logical_PC[0] += 4;
											reg_table->entry_x[s2].in_use = 0;
										}
										else if (strcmp(op_table[op_match].symbol, "-=") == 0) {
											UINT8 s2 = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
											load_64bB(parse_out, logical_PC, s2, number, (char*)"loop increment", reg_table, l_control->depth, parse_in, param, Masm);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s// \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[s2].name); logical_PC[0] += 4;
											reg_table->entry_x[s2].in_use = 0;
										}
										else {
											debug++;
										}
									}
								}
								else {
									debug++;
								}
							}
							else if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
								if (strcmp(op_table[op_match].symbol, "+=") == 0) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s //  \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current2->reg_index].name); logical_PC[0] += 4;
								}
								else if (strcmp(op_table[op_match].symbol, "-=") == 0) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s //  \n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current2->reg_index].name); logical_PC[0] += 4;
								}
								else {
									debug++;
								}
							}
							else {
								debug++;
							}
						}
						else if (current->type == fp16_enum) {
							UINT ptr_hold = parse_in->ptr;
							UINT8 s1 = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
							VariableListEntry s2;
							s2.reg_index = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
							s2.reg_index_valid = 1;
							sprintf_s(s2.name, "%s", current->name);
							s2.type = current->type;
							fp_data_struct fp_data;
							// need to check for 3 input fp 
							getWord(parse_in);
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
							if (get_integer(&number, parse_in->word)) {
								UINT8 s1i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								load_64bB(parse_out, logical_PC, s1i, number, parse_in->word, reg_table, l_control->depth, parse_in, param, Masm);
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fcvt.x.h %s, %s // int to float \n", logical_PC[0], reg_table->entry_fp[s1].name, reg_table->entry_x[s1i].name); logical_PC[0] += 4;
								reg_table->entry_x[s1i].in_use = 0;

								if (strcmp(op_table[op_match].symbol, "+=") == 0 && parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
									parse_in->ptr++;
									parse_rh_of_eqB(parse_out, &s2, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
									if (current->reg_index_valid != 1) {
										current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
										load_64bB(parse_out, logical_PC, current->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
										current->reg_index_valid = 1;
										current->reg_depth = l_control->depth;
									}
									UINT8 dst = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s, %s, %s  // %s = %s + %d * s2\n",
										logical_PC[0], reg_table->entry_fp[dst].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[s2.reg_index].name, reg_table->entry_fp[current->reg_index].name,
										current->name, current->name, number); logical_PC[0] += 4;
									reg_table->entry_fp[current->reg_index].in_use = 0;
									current->reg_index = dst; // releive reg renaming pressure
								}
								else if (strcmp(op_table[op_match].symbol, "-=") == 0 && parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
									parse_in->ptr++;
									parse_rh_of_eqB(parse_out, &s2, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
									if (current->reg_index_valid != 1) {
										current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
										load_64bB(parse_out, logical_PC, current->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
										current->reg_index_valid = 1;
										current->reg_depth = l_control->depth;
									}
									UINT8 dst = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s, %s, %s  // %s = %s - %d * s2\n",
										logical_PC[0], reg_table->entry_fp[dst].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[s2.reg_index].name, reg_table->entry_fp[current->reg_index].name,
										current->name, current->name, number); logical_PC[0] += 4;
									reg_table->entry_fp[current->reg_index].in_use = 0;
									current->reg_index = dst; // releive reg renaming pressure
								}
								else {
									debug++;
								}
							}
							else if (get_float(&fp_data, parse_in->word)) {
								UINT8 s2i = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								UINT index0 = (fp_data.hp & 0x0fff);
								//								UINT64 carry = (fp_data.hp & 0x800) << 1;
								//								UINT index1 = (((fp_data.hp + carry) >> 12) & 0x0fffff);
								UINT index1 = (((fp_data.hp) >> 12) & 0x0fffff);
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x lui %s,  %#07x // start 16b load\n", logical_PC[0], reg_table->entry_x[s2i].name, index1); logical_PC[0] += 4;
								if (index0 < 0x800) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ori %s, %s, %#05x	// completed load of 0x%08x as 16b immediate load\n", logical_PC[0], reg_table->entry_x[s2i].name, reg_table->entry_x[s2i].name, index0, fp_data.hp); logical_PC[0] += 4;
								}
								else {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, %#05x	// completed load of 0x%08x as 16b immediate load\n", logical_PC[0], reg_table->entry_x[s2i].name, reg_table->entry_x[s2i].name, index0, fp_data.hp); logical_PC[0] += 4;
								}
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmv.x.h %s, %s	// move 16b int reg to fp reg, do not convert\n", logical_PC[0], reg_table->entry_fp[s1].name, reg_table->entry_x[s2i].name); logical_PC[0] += 4;
								reg_table->entry_x[s2i].in_use = 0;

								if (strcmp(op_table[op_match].symbol, "+=") == 0 && parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
									parse_in->ptr++; UINT hold_ptr = parse_in->ptr;
									getWord(parse_in);
									VariableListEntry* current2;
									if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
										UINT8 dst = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s, %s, %s  // %s = %s - %f * s2\n",
											logical_PC[0], reg_table->entry_fp[dst].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[current2->reg_index].name, reg_table->entry_fp[current->reg_index].name,
											current->name, current->name, fp_data.hp); logical_PC[0] += 4;
										reg_table->entry_fp[current->reg_index].in_use = 0;
										current->reg_index = dst; // releive reg renaming pressure
									}
									else {
										parse_in->ptr = hold_ptr;
										parse_rh_of_eqB(parse_out, &s2, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
										if (current->reg_index_valid != 1) {
											current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
											load_64bB(parse_out, logical_PC, current->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
											current->reg_index_valid = 1;
											current->reg_depth = l_control->depth;
										}
										UINT8 dst = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s, %s, %s  // %s = %s + %f * s2\n",
											logical_PC[0], reg_table->entry_fp[dst].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[s2.reg_index].name, reg_table->entry_fp[current->reg_index].name,
											current->name, current->name, fp_data.hp); logical_PC[0] += 4;
										reg_table->entry_fp[current->reg_index].in_use = 0;
										current->reg_index = dst; // releive reg renaming pressure
									}
								}
								else if (strcmp(op_table[op_match].symbol, "-=") == 0 && parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
									parse_in->ptr++;
									UINT hold_ptr = parse_in->ptr;
									getWord(parse_in);
									VariableListEntry* current2;
									if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
										UINT8 dst = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s, %s, %s  // %s = %s - %f * s2\n",
											logical_PC[0], reg_table->entry_fp[dst].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[current2->reg_index].name, reg_table->entry_fp[current->reg_index].name,
											current->name, current->name, fp_data.hp); logical_PC[0] += 4;
										reg_table->entry_fp[current->reg_index].in_use = 0;
										current->reg_index = dst; // releive reg renaming pressure
									}
									else {
										parse_in->ptr = hold_ptr;
										parse_rh_of_eqB(parse_out, &s2, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
										if (current->reg_index_valid != 1) {
											current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
											load_64bB(parse_out, logical_PC, current->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
											current->reg_index_valid = 1;
											current->reg_depth = l_control->depth;
										}
										UINT8 dst = alloc_float(reg_table, l_control->depth, parse_out, parse_in, Masm);
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s, %s, %s  // %s = %s - %f * s2\n",
											logical_PC[0], reg_table->entry_fp[dst].name, reg_table->entry_fp[s1].name, reg_table->entry_fp[s2.reg_index].name, reg_table->entry_fp[current->reg_index].name,
											current->name, current->name, fp_data.hp); logical_PC[0] += 4;
										reg_table->entry_fp[current->reg_index].in_use = 0;
										current->reg_index = dst; // releive reg renaming pressure
									}
								}
								else {
									debug++;
								}
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
								VariableListEntry* current2;
								if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name); logical_PC[0] += 4;
								}
								else {
									debug++;
								}
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
								parse_in->ptr++;
								VariableListEntry* current2, * current3;
								if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
									//									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name); logical_PC[0] += 4;
								}
								else {
									debug++;
								}
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
								getWord(parse_in);
								if (get_VariableListEntryB(&current3, parse_in->word, compiler_var->list_base)) {
									if (strcmp(op_table[op_match].symbol, "+=") == 0) {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fmadd.h %s, %s, %s\n", logical_PC[0], reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name, reg_table->entry_fp[current3->reg_index].name, reg_table->entry_fp[current->reg_index].name); logical_PC[0] += 4;
									}
									else if (strcmp(op_table[op_match].symbol, "-=") == 0) {
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fnmsub.h %s, %s, %s\n", logical_PC[0], reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current2->reg_index].name, reg_table->entry_fp[current3->reg_index].name, reg_table->entry_fp[current->reg_index].name); logical_PC[0] += 4;
									}
									else {
										debug++;
									}
								}
								else {
									debug++;
								}
							}
							else {
								parse_in->ptr = ptr_hold;
								parse_rh_of_eqB(parse_out, &s2, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
								if (current->reg_index_valid != 1) {
									current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
									load_64bB(parse_out, logical_PC, current->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
									current->reg_index_valid = 1;
									current->reg_depth = l_control->depth;
								}
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x f%s.h %s, %s, %s\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[current->reg_index].name, reg_table->entry_fp[s2.reg_index].name); logical_PC[0] += 4;
							}
							reg_table->entry_fp[s1].in_use = 0;
							reg_table->entry_fp[s2.reg_index].in_use = 0;
							s2.reg_index_valid = 0;
						}
						else {
							VariableListEntry s2;
							s2.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							s2.reg_index_valid = 1;
							sprintf_s(s2.name, "%s", current->name);
							s2.type = current->type;
							parse_rh_of_eqB(parse_out, &s2, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
							if (current->reg_index_valid != 1) {
								current->reg_index = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								load_64bB(parse_out, logical_PC, current->reg_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
								current->reg_index_valid = 1;
								current->reg_depth = l_control->depth;
							}
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x %s %s, %s, %s\n", logical_PC[0], op_table[op_match].opcode, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name, reg_table->entry_x[s2.reg_index].name); logical_PC[0] += 4;
							reg_table->entry_x[s2.reg_index].in_use = 0;
							s2.reg_index_valid = 0;
						}
					}
					else {
						debug++;// syntax
					}
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '+') {
					parse_in->ptr += 2;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1\n", logical_PC[0], reg_table->entry_x[current->reg_index].name, reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] != ';')
						debug++;
				}
				else {
					debug++;// syntax error??
				}
			}
			else if (get_FunctionListEntryB(&function_match, parse_in->word, compiler_var->f_list_base)) {
				exec_function_call(parse_out, parse_in, logical_PC, l_control, function_match, compiler_var, reg_table, csr_list, csr_list_count, param, Masm);
			}
			else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->global_list_base)) {
				UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				load_64bB(parse_out, logical_PC, addr, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
				UINT8 num_valid = 0;
				INT64 number2;
				char skip = 0;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
					UINT hold = parse_in->ptr;
					parse_in->ptr++;
					getWord(parse_in);
					UINT incr_flag = 0;

					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '+') {
						parse_in->ptr += 2;
						incr_flag = 1;
					}
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
						parse_in->ptr++;
						VariableListEntry* current2;
						INT64 number;
						if (get_VariableListEntryB(&current2, parse_in->word, compiler_var->list_base)) {
							if (current2->reg_index_valid == 0) {
								current2->reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								load_64bB(parse_out, logical_PC, current2->reg_index, current2->addr, current2->name, reg_table, l_control->depth, parse_in, param, Masm);
							}
							UINT8 t1 = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							if (current->type == uint64_enum || current->type == int64_enum) {
								if (incr_flag) {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %d // %s++ (adjust for stack ordering not necessary)	\n", logical_PC[0], reg_table->entry_x[current2->reg_index].name, reg_table->entry_x[current2->reg_index].name, incr_flag, current2->name);  logical_PC[0] += 4; // store 
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // 64b addresses  on stack, adjust index			\n", logical_PC[0], reg_table->entry_x[t1].name, reg_table->entry_x[current2->reg_index].name);  logical_PC[0] += 4; // store 
								}
								else {
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 1 // adjust for stack ordering			\n", logical_PC[0], reg_table->entry_x[t1].name, reg_table->entry_x[current2->reg_index].name);  logical_PC[0] += 4; // store 	
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // 64b addresses  on stack, adjust index			\n", logical_PC[0], reg_table->entry_x[t1].name, reg_table->entry_x[t1].name);  logical_PC[0] += 4; // store 
								}
							}
							else
								debug++;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s 			\n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, reg_table->entry_x[t1].name);  logical_PC[0] += 4; // store 
	//						if (incr_flag) {
	//							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, %d // %s++	\n", logical_PC[0], reg_table->entry_x[current2->reg_index].name, reg_table->entry_x[current2->reg_index].name, incr_flag, current2->name);  logical_PC[0] += 4; // store 
	//						}
						}
						else if (get_integer(&number, parse_in->word)) {
							if (number < 0x800 && number >= 0) {
								skip = 1;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ')parse_in->ptr++;
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
									UINT8 ptr_hold = parse_in->ptr;
									parse_in->ptr++;
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ')parse_in->ptr++;
									getWord(parse_in);
									if (get_integer(&number2, parse_in->word) && parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
										//						if (get_integer(&number2, parse_in->word)) {
										if (number2 == 0) {
											switch (current->type) {
											case int8_enum:
											case uint8_enum:
												sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sb zero, 0x%03x(%s)				// write to global variable %s\n", logical_PC[0], -(number + 1), reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
												break;
											case int16_enum:
											case uint16_enum:
											case fp16_enum:
												sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sh zero, 0x%03x(%s)				// write to global variable %s\n", logical_PC[0], -2 * (number + 1), reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
												break;
											case int32_enum:
											case uint32_enum:
											case fp32_enum:
												sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sw zero, 0x%03x(%s)				// write to global variable %s\n", logical_PC[0], -4 * (number + 1), reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
												break;
											case int64_enum:
											case uint64_enum:
											case fp64_enum:
												sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd zero, 0x%03x(%s)				// write to global variable %s\n", logical_PC[0], -8 * (number + 1), reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
												break;
											default:
												debug++;
												break;
											}
										}
										else {
											//								debug++;
											UINT8 top = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
											load_64bB(parse_out, logical_PC, top, number2, parse_in->word, reg_table, l_control->depth, parse_in, param, Masm);
											switch (current->type) {
											case int8_enum:
											case uint8_enum:
												sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sb %s, 0x%03x(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top].name, -(number + 1), reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
												break;
											case int16_enum:
											case uint16_enum:
											case fp16_enum:
												sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sh %s, 0x%03x(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top].name, -2 * (number + 1), reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
												break;
											case int32_enum:
											case uint32_enum:
											case fp32_enum:
												sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sw %s, 0x%03x(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top].name, -4 * (number + 1), reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
												break;
											case int64_enum:
											case uint64_enum:
											case fp64_enum:
												sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, 0x%03x(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top].name, -8 * (number + 1), reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
												break;
											default:
												debug++;
												break;
											}
											reg_table->entry_x[top].in_use = 0;
										}
									}
									else {
										parse_in->ptr = ptr_hold;

										VariableListEntry top;
										top.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
										top.reg_index_valid = 1;
										sprintf_s(top.name, "");
										top.type = int64_enum;
										parse_rh_of_eqB(parse_out, &top, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
										switch (current->type) {
										case int8_enum:
										case uint8_enum:
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sb %s, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top.reg_index].name, reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
											break;
										case int16_enum:
										case uint16_enum:
										case fp16_enum:
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sh %s, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top.reg_index].name, reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
											break;
										case int32_enum:
										case uint32_enum:
										case fp32_enum:
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sw %s, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top.reg_index].name, reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
											break;
										case int64_enum:
										case uint64_enum:
										case fp64_enum:
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top.reg_index].name, reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
											break;
										default:
											debug++;
											break;
										}
										l_control->fence[l_control->depth] = 1;
										reg_table->entry_x[top.reg_index].in_use = 0;
										top.reg_index_valid = 0;
									}
								}
								else {
									debug++;
								}
							}
							else {
								if ((current->addr & 0x0fff) == 0) {
									if (number < 0x800 && number > 0) {
										switch (current->type) {
										case int8_enum:
										case uint8_enum:
											number = -(number + 1);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ori %s, %s, 0x%x 	// %s is 8b \n",
												logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
											break;
										case int16_enum:
										case uint16_enum:
										case fp16_enum:
											number = -((number + 1) * 2);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ori %s, %s, 0x%x 	// %s is 16b \n",
												logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
											break;
										case int32_enum:
										case uint32_enum:
										case fp32_enum:
											number = -((number + 1) * 4);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ori %s, %s, 0x%x 	// %s is 32b	\n",
												logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
											break;
										case int64_enum:
										case uint64_enum:
										case fp64_enum:
											number = -((number + 1) * 8);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ori %s, %s, 0x%x 	// %s is 64b	\n",
												logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
											break;
										case int128_enum:
										case uint128_enum:
										case fp128_enum:
											number = -((number + 1) * 16);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ori %s, %s, 0x%x 	// %s is 128b	\n",
												logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
											break;
										default:
											debug++;
											break;
										}
									}
									else {
										switch (current->type) {
										case int8_enum:
										case uint8_enum:
											number = -(number + 1);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 0x%x 	// %s is 8b \n",
												logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
											break;
										case int16_enum:
										case uint16_enum:
										case fp16_enum:
											number = -((number + 1) * 2);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 0x%x 	// %s is 16b \n",
												logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
											break;
										case int32_enum:
										case uint32_enum:
										case fp32_enum:
											number = -((number + 1) * 4);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 0x%x 	// %s is 32b	\n",
												logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
											break;
										case int64_enum:
										case uint64_enum:
										case fp64_enum:
											number = -((number + 1) * 8);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 0x%x 	// %s is 64b	\n",
												logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
											break;
										case int128_enum:
										case uint128_enum:
										case fp128_enum:
											number = -((number + 1) * 16);
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x andi %s, %s, 0x%x 	// %s is 128b	\n",
												logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
											break;
										default:
											debug++;
											break;
										}
									}
								}
								else {
									switch (current->type) {
									case int8_enum:
									case uint8_enum:
										number = -(number + 1);
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%x 	// %s is 8b \n",
											logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
										break;
									case int16_enum:
									case uint16_enum:
									case fp16_enum:
										number = -((number + 1) * 2);
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%x 	// %s is 16b \n",
											logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
										break;
									case int32_enum:
									case uint32_enum:
									case fp32_enum:
										number = -((number + 1) * 4);
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%x 	// %s is 32b	\n",
											logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
										break;
									case int64_enum:
									case uint64_enum:
									case fp64_enum:
										number = -((number + 1) * 8);
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%x 	// %s is 64b	\n",
											logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
										break;
									case int128_enum:
									case uint128_enum:
									case fp128_enum:
										number = -((number + 1) * 16);
										sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0x%x 	// %s is 128b	\n",
											logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, number, current->name);  logical_PC[0] += 4; // store 
										break;
									default:
										debug++;
										break;
									}
								}
							}
						}
						else {
							debug++;
						}
					}
					else {
						parse_in->ptr = hold;
						VariableListEntry index;
						addVariableEntry(&index, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);
						parse_index_a(parse_out, &index, 0, logical_PC, current, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3				// \n", logical_PC[0], reg_table->entry_x[index.reg_index].name, reg_table->entry_x[index.reg_index].name);  logical_PC[0] += 4; // store 
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x add %s, %s, %s				// entry number\n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr].name, reg_table->entry_x[index.reg_index].name);  logical_PC[0] += 4; // store 
						reg_table->entry_x[index.reg_index].in_use = 0;

						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') parse_in->ptr++;

						VariableListEntry top;
						addVariableEntry(&top, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);

						parse_rh_of_eqB(parse_out, &top, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, 0(%s)				// \n", logical_PC[0], reg_table->entry_x[top.reg_index].name, reg_table->entry_x[addr].name);  logical_PC[0] += 4; // store 
						l_control->fence[l_control->depth] = 1;
						reg_table->entry_x[top.reg_index].in_use = 0;
						top.reg_index_valid = 0;
					}
				}
				if (!skip) {
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ')parse_in->ptr++;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
						UINT8 ptr_hold = parse_in->ptr;
						parse_in->ptr++;
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ')parse_in->ptr++;
						getWord(parse_in);
						if (get_integer(&number2, parse_in->word) && parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
							//						if (get_integer(&number2, parse_in->word)) {
							if (number2 == 0) {
								switch (current->type) {
								case int8_enum:
								case uint8_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sb zero, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
									break;
								case int16_enum:
								case uint16_enum:
								case fp16_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sh zero, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
									break;
								case int32_enum:
								case uint32_enum:
								case fp32_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sw zero, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
									break;
								case int64_enum:
								case uint64_enum:
								case fp64_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd zero, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
									break;
								default:
									debug++;
									break;
								}
							}
							else {
								//								debug++;
								UINT8 top = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								load_64bB(parse_out, logical_PC, top, number2, parse_in->word, reg_table, l_control->depth, parse_in, param, Masm);
								switch (current->type) {
								case int8_enum:
								case uint8_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sb %s, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top].name, reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
									break;
								case int16_enum:
								case uint16_enum:
								case fp16_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sh %s, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top].name, reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
									break;
								case int32_enum:
								case uint32_enum:
								case fp32_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sw %s, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top].name, reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
									break;
								case int64_enum:
								case uint64_enum:
								case fp64_enum:
									sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top].name, reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
									break;
								default:
									debug++;
									break;
								}
								reg_table->entry_x[top].in_use = 0;
							}
							//						}
							//						else {
							//							debug++;
							//						}
						}
						else {
							parse_in->ptr = ptr_hold;

							VariableListEntry top;
							top.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							top.reg_index_valid = 1;
							sprintf_s(top.name, "");
							top.type = int64_enum;
							parse_rh_of_eqB(parse_out, &top, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
							switch (current->type) {
							case int8_enum:
							case uint8_enum:
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sb %s, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top.reg_index].name, reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
								break;
							case int16_enum:
							case uint16_enum:
							case fp16_enum:
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sh %s, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top.reg_index].name, reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
								break;
							case int32_enum:
							case uint32_enum:
							case fp32_enum:
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sw %s, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top.reg_index].name, reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
								break;
							case int64_enum:
							case uint64_enum:
							case fp64_enum:
								sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, 0(%s)				// write to global variable %s\n", logical_PC[0], reg_table->entry_x[top.reg_index].name, reg_table->entry_x[addr].name, current->name);  logical_PC[0] += 4; // store 
								break;
							default:
								debug++;
								break;
							}
							l_control->fence[l_control->depth] = 1;
							reg_table->entry_x[top.reg_index].in_use = 0;
							top.reg_index_valid = 0;
						}
					}
					else {
						debug++;
					}
				}
				reg_table->entry_x[addr].in_use = 0;
			}
			else if (match_listB(&reg_index, parse_in->word, IO_list->name, IO_list->ptr, logical_PC)) {
				if (reg_table->error == 0)
					reg_table->error = alloc_saved_EABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '0' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == ']') {
					parse_in->ptr += 3;
				}
				else {
					debug++;
				}
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
					parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
					getWord(parse_in);
					UINT8 addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					load_64bB(parse_out, logical_PC, addr, IO_list->addr[reg_index], IO_list->name[reg_index].name, reg_table, l_control->depth, parse_in, param, Masm);
					INT64 num;
					if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
						if (current->pointer) {
							UINT8 current_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, current_index, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
	//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sc.d.aq.rl %s, %s, %s //  \n", logical_PC[0], reg_table->entry_x[reg_table->error].name, reg_table->entry_x[addr].name, reg_table->entry_x[current_index].name); logical_PC[0] += 4;
							sprint_op_2src(parse_out, logical_PC, (char*)"sc.d.aq.rl", reg_table->entry_x[reg_table->error].name, reg_table->entry_x[addr].name, reg_table->entry_x[current_index].name, param);
							reg_table->entry_x[current_index].in_use = 0;
						}
						else {
	//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sc.d.aq.rl %s, %s, %s //  \n", logical_PC[0], reg_table->entry_x[reg_table->error].name, reg_table->entry_x[addr].name, reg_table->entry_x[current->reg_index].name); logical_PC[0] += 4;
							sprint_op_2src(parse_out, logical_PC, (char*)"sc.d.aq.rl", reg_table->entry_x[reg_table->error].name, reg_table->entry_x[addr].name, reg_table->entry_x[current->reg_index].name, param);
						}
					}
					else if (match_listB(&csr_index, parse_in->word, csr_list, csr_list_count, logical_PC)) {
						UINT8 csr_copy = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrci %s, 0x00, %s \n", logical_PC[0], reg_table->entry_x[csr_copy].name, csr_list[csr_index]); logical_PC[0] += 4;
						sprint_csr_imm(parse_out, logical_PC, (char*)"csrrci", reg_table->entry_x[csr_copy].name, 0, csr_list[csr_index].name, param);
//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sc.d.aq.rl %s, %s, %s //  \n", logical_PC[0], reg_table->entry_x[reg_table->error].name, reg_table->entry_x[addr].name, reg_table->entry_x[csr_copy].name); logical_PC[0] += 4;
						sprint_op_2src(parse_out, logical_PC, (char*)"sc.d.aq.rl", reg_table->entry_x[reg_table->error].name, reg_table->entry_x[addr].name, reg_table->entry_x[csr_copy].name, param);
						reg_table->entry_x[csr_copy].in_use = 0;
					}
					else if (get_integer(&num, parse_in->word)) {
						UINT8 data = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, logical_PC, data, num, (char*)"", reg_table, l_control->depth, parse_in, param, Masm);
		//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sc.d.aq.rl %s, %s, %s //  \n", logical_PC[0], reg_table->entry_x[reg_table->error].name, reg_table->entry_x[addr].name, reg_table->entry_x[data].name); logical_PC[0] += 4;
						sprint_op_2src(parse_out, logical_PC, (char*)"sc.d.aq.rl", reg_table->entry_x[reg_table->error].name, reg_table->entry_x[addr].name, reg_table->entry_x[data].name, param);
						reg_table->entry_x[data].in_use = 0;
					}
					else {
						//					UINT8 data = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						VariableListEntry data;
						data.reg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						data.reg_index_valid = 1;
						sprintf_s(data.name, "");
						data.type = int64_enum;
						UINT8 arg_index = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						for (UINT8 i = 0; i < 4; i++) {//current_function->arg_count
							if (strcmp(current_function->argument[i].name, parse_in->word) == 0) {
								load_64bB(parse_out, logical_PC, arg_index, current_function->argument[i].addr, current_function->argument[i].name, reg_table, l_control->depth, parse_in, param, Masm);
	//							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, %s, 0 //  \n", logical_PC[0], reg_table->entry_x[data.reg_index].name, reg_table->entry_x[arg_index].name); logical_PC[0] += 4;
								sprint_op_2src(parse_out, logical_PC, (char*)"addi", reg_table->entry_x[data.reg_index].name, reg_table->entry_x[arg_index].name, (UINT16) 0, param);
							}
						}
						reg_table->entry_x[arg_index].in_use = 0;
						// sc rd, s1(addr), s2(data)
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
							UINT8 num_valid = 0;
							INT64 number2;
							parse_rh_of_eqB(parse_out, &data, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
						}
	//					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sc.d.aq.rl %s, %s, %s //  \n", logical_PC[0], reg_table->entry_x[reg_table->error].name, reg_table->entry_x[addr].name, reg_table->entry_x[data.reg_index].name); logical_PC[0] += 4;
						sprint_op_2src(parse_out, logical_PC, (char*)"sc.d.aq.rl", reg_table->entry_x[reg_table->error].name, reg_table->entry_x[addr].name, reg_table->entry_x[data.reg_index].name, param);
						reg_table->entry_x[data.reg_index].in_use = 0;
						data.reg_index_valid = 0;
					}
					reg_table->entry_x[addr].in_use = 0;
				}
			}
			else if (strcmp(parse_in->word, "goto") == 0) {
				getWord(parse_in);
				VariableListEntry* global_var;
				if (get_VariableListEntryB(&global_var, parse_in->word, compiler_var->global_list_base)) {
					if (global_var->pointer == 0)
						debug++;
					UINT8 addr_ptr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
					UINT8 addr = alloc_jalr_UABI(reg_table, l_control->depth, parse_out, Masm);
					load_64bB(parse_out, logical_PC, addr_ptr, global_var->addr, global_var->name, reg_table, l_control->depth, parse_in, param, Masm);
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[')
						parse_in->ptr++;
					else
						debug++;
					getWord(parse_in);
					INT64 num;
					if (get_integer(&num, parse_in->word)) {
						if (num < (INT64) 0x100) {
				//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0x%03x(%s) // goto ..., load indexed value \n", logical_PC[0], reg_table->entry_x[addr].name,-((num+1)<<3), reg_table->entry_x[addr_ptr].name); logical_PC[0] += 4;
							sprint_load(parse_out, logical_PC, (char*)"ld", reg_table->entry_x[addr].name, -((num + 1) << 3), reg_table->entry_x[addr_ptr].name,(char*)"goto ..., load indexed value", param);
						}
						else {
							UINT8 addr_offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
							load_64bB(parse_out, logical_PC, addr_offset, num+1, (char*)"", reg_table, l_control->depth, parse_in, param, Masm);
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x slli %s, %s, 3 // goto ..., adjust 64b \n", logical_PC[0], reg_table->entry_x[addr_offset].name, reg_table->entry_x[addr_offset].name); logical_PC[0] += 4;
							sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sub %s, %s, %s // goto ..., subtract index, data on stack\n", logical_PC[0], reg_table->entry_x[addr_offset].name, reg_table->entry_x[addr_ptr].name, reg_table->entry_x[addr_offset].name); logical_PC[0] += 4;
					//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ld %s, 0(%s) // goto ..., load indexed value \n", logical_PC[0], reg_table->entry_x[addr].name, reg_table->entry_x[addr_offset].name); logical_PC[0] += 4;
							sprint_load(parse_out, logical_PC, (char*)"ld", reg_table->entry_x[addr].name, (UINT16)0, reg_table->entry_x[addr_offset].name, (char*)"goto ..., load indexed value", param);
							reg_table->entry_x[addr_offset].in_use = 0;
						}
					}
					else {
						debug++;
					}
				//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x jalr zero, 0(%s) // goto ..., jump to saved location \n", logical_PC[0], reg_table->entry_x[addr].name); logical_PC[0] += 4;
					sprint_load(parse_out, logical_PC, (char*)"jalr", (char*)"zero", (UINT16)0, reg_table->entry_x[addr].name, (char*)" goto ..., jump to saved location", param);
					reg_table->entry_x[addr_ptr].in_use = 0;
					reg_table->entry_x[addr].in_use = 0;
				}
				else if (get_VariableListEntryB(&global_var, parse_in->word, compiler_var->list_base)) {
					if (global_var->pointer == 1)
						debug++;
					if (global_var->reg_index_valid == 0)
						load_64bB(parse_out, logical_PC, global_var->reg_index, global_var->addr, global_var->name, reg_table, l_control->depth, parse_in, param, Masm);
				//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x jalr zero, 0(%s) // goto ... \n", logical_PC[0], reg_table->entry_x[global_var->reg_index].name); logical_PC[0] += 4;
					sprint_load(parse_out, logical_PC, (char*)"jalr", (char*)"zero", (UINT16)0, reg_table->entry_x[global_var->reg_index].name, (char*)" goto ...", param);
				}
			}
			else if (strcmp(parse_in->word, "push") == 0) {
				getWord(parse_in);
				sp_offset -= 8;
		//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, %d(sp) // push to stack \n", logical_PC[0], parse_in->word, sp_offset); logical_PC[0] += 4;
				sprint_load(parse_out, logical_PC, (char*)"sd", parse_in->word, (UINT16) sp_offset, (char*)"sp", (char*) "push to stack", param);
				l_control->fence[l_control->depth] = 1;
			}
			else if (strcmp(parse_in->word, "ecall") == 0) {
		//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ecall // generate interrupt \n", logical_PC[0]); logical_PC[0] += 4;
				sprintf_string(parse_out, logical_PC, (char*)" ecall // generate interrupt \n", param);
			}
			else if (strcmp(parse_in->word, "wfi") == 0) {
		//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x wfi // wait for interrupt \n", logical_PC[0]); logical_PC[0] += 4;
				sprintf_string(parse_out, logical_PC, (char*)" wfi // wait for interrupt \n", param);
			}
			else if (strcmp(parse_in->word, "ret") == 0) {
				if (logical_PC[0] < 0x80000000) {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x mret // return from interrupt/exception \n", logical_PC[0]); logical_PC[0] += 4;
				}
				else {
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sret // return from interrupt/exception \n", logical_PC[0]); logical_PC[0] += 4;
				}
			}
			else if (strcmp(parse_in->word, "OS_entry") == 0) {
				UINT8 addr = alloc_jalr_UABI(reg_table, l_control->depth, parse_out, Masm);
				int PC = logical_PC[0];
				load_64bB(parse_out, logical_PC, addr, 0x80000000 & 0xfffff000, (char*)"", reg_table, l_control->depth, parse_in, param, Masm);// needs to be parameterized for different platforms??? maybe
			//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x jalr zero, 0x%03x(%s) // OS entry \n", logical_PC[0], 0x80000000 & 0x00000fff, reg_table->entry_x[addr].name); logical_PC[0] += 4;
				sprint_load(parse_out, logical_PC, (char*)"jalr", (char*)"zero", (UINT16)0, reg_table->entry_x[addr].name, (char*)" OS entry", param);
				reg_table->entry_x[addr].in_use = 0;
			}
			else if (strcmp(parse_in->word, "parallel_invoke") == 0) {
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
					// need to push registers onto sp in same order as SRET pulls them off

					UINT thread_count = 0;
					parse_in->ptr++;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\n' || (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/')) {
						parse_in->index++;
						UINT8 valid_data_flag = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);

				//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 1 // valid data\n", logical_PC[0], reg_table->entry_x[valid_data_flag].name); logical_PC[0] += 4;
						sprint_op_2src(parse_out, logical_PC, (char*)"addi", reg_table->entry_x[valid_data_flag].name, reg_table->entry_x[0].name, (INT8)1, (char*)"valid data", param);
				//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, -0x008(tp) // valid data, assumes sv48 (64b addr) \n", logical_PC[0], reg_table->entry_x[valid_data_flag].name); logical_PC[0] += 4;
						sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[valid_data_flag].name, (UINT16)-8, reg_table->entry_x[4].name, (char*)" OS entry", param);
						l_control->fence[l_control->depth] = 1;
						reg_table->entry_x[valid_data_flag].in_use = 0;

						sprintf_s(parse_out->line[parse_out->index++].line, "//\t%d\t%s", parse_in->index, parse_in->line[parse_in->index].line);
						parse_in->ptr = 0;
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						UINT8 funct_addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						INT16 tp_offset = 0x28;
						UINT thread_count = 0;
						while (!(parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ';')) {
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '&' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == ']') {
								parse_in->ptr += 3;
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
									parse_in->ptr++;
								}
								else {
									debug++; // format error
								}
								thread_count++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								UINT8 argc = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
								if (get_FunctionListEntryB(&function_match, parse_in->word, compiler_var->f_list_base)) {
									load_64bB(parse_out, logical_PC, funct_addr, function_match->addr, function_match->name, reg_table, l_control->depth, parse_in, param, Masm);//return address when all complete
							//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, -0x%03x(tp) // store function address pointer\n", logical_PC[0], reg_table->entry_x[funct_addr].name, tp_offset); logical_PC[0] += 4; tp_offset += 8;
									sprint_load(parse_out, logical_PC, (char*)"sd",reg_table->entry_x[funct_addr].name, -tp_offset, reg_table->entry_x[4].name, (char*)"store function address pointer", param); tp_offset += 8;
									//  need to generate a function to count up threads complete before restarting main thread
							//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%03x // argc = argument count\n", logical_PC[0], reg_table->entry_x[argc].name, function_match->argc); logical_PC[0] += 4;//function_match->arg_count
									sprint_op_2src(parse_out, logical_PC, (char*)"addi", reg_table->entry_x[argc].name, reg_table->entry_x[0].name, function_match->argc, (char*)"argc = argument count", param);
							//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, -0x%03x(tp) // argc to stack\n", logical_PC[0], reg_table->entry_x[argc].name, tp_offset); logical_PC[0] += 4; tp_offset += 8;
									sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[argc].name, -tp_offset, reg_table->entry_x[4].name, (char*)"argc to stack", param); tp_offset += 8;
									l_control->fence[l_control->depth] = 1;

									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
										parse_in->ptr++;
									}
									else {
										debug++; // format error
									}
									for (UINT8 i = 0; i < function_match->argc; i++) {//function_match->arg_count
										INT64 num;
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
										getWord(parse_in);
										char word[0x80];
										sprintf_s(word, "push arg(%d) = %s", i, parse_in->word);
										if (get_VariableListEntryB(&current, parse_in->word, compiler_var->list_base)) {
											UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
											load_64bB(parse_out, logical_PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
									//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, -0x%03x(tp) // push arg(%d) = %s \n", logical_PC[0], reg_table->entry_x[offset].name, tp_offset, i, parse_in->word); logical_PC[0] += 4; tp_offset += 8;
											sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[offset].name, -tp_offset, reg_table->entry_x[4].name, word, param); tp_offset += 8;
											reg_table->entry_x[offset].in_use = 0;
										}
										else if (get_VariableListEntryB(&current, parse_in->word, compiler_var->global_list_base)) {
											UINT8 offset = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
											load_64bB(parse_out, logical_PC, offset, current->addr, current->name, reg_table, l_control->depth, parse_in, param, Masm);
							//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, -0x%03x(tp) // push arg(%d) = %s \n", logical_PC[0], reg_table->entry_x[offset].name, tp_offset, i, parse_in->word); logical_PC[0] += 4; tp_offset += 8;
											sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[offset].name, -tp_offset, reg_table->entry_x[4].name, word, param); tp_offset += 8;
											reg_table->entry_x[offset].in_use = 0;
										}
										else if (get_integer(&num, parse_in->word)) {
											if (num == 0) {
								//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd zero, -0x%03x(tp) // push arg(%d) = %s \n", logical_PC[0], tp_offset, i, parse_in->word); logical_PC[0] += 4; tp_offset += 8;
												sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[0].name, -tp_offset, reg_table->entry_x[4].name, word, param); tp_offset += 8;
											}
											else {
												UINT8 temp = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
												load_64bB(parse_out, logical_PC, temp, num, (char *) "", reg_table, l_control->depth, parse_in, param, Masm);
								//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, -0x%03x(tp) // push arg(%d) = %s \n", logical_PC[0], reg_table->entry_x[temp].name, tp_offset, i, parse_in->word); logical_PC[0] += 4; tp_offset += 8;
												sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[temp].name, -tp_offset, reg_table->entry_x[4].name, word, param); tp_offset += 8;
												reg_table->entry_x[temp].in_use = 0;
											}
										}
										else {
											debug++;
										}
										l_control->fence[l_control->depth] = 1;

										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',') {
											parse_in->ptr++;
										}
									}
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
										parse_in->ptr++;
									}
									else {
										debug++; // format error
									}
								}
								else {
									debug++;
								}
							}
							parse_in->index++;
							sprintf_s(parse_out->line[parse_out->index++].line, "//\t%d\t%s", parse_in->index, parse_in->line[parse_in->index].line);
							parse_in->ptr = 0;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						}
						//						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, %#05x // update thread count\n", logical_PC[0], reg_table->entry_x[thread_count_reg].name, reg_table->entry_x[thread_count_reg].name, thread_count); logical_PC[0] += 4;
						UINT8 CPUID = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
	//					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x csrrci %s, 0, mhartid // a load CPUID\n", logical_PC[0], reg_table->entry_x[CPUID].name); logical_PC[0] += 4;
						sprint_csr_imm(parse_out, logical_PC, (char*)"csrrci", reg_table->entry_x[CPUID].name, 0,(char*) "mhartid", param);
				//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, -0x010(tp) // tp[1] Let OS know this core has data to run\n", logical_PC[0], reg_table->entry_x[CPUID].name); logical_PC[0] += 4;
						sprint_load(parse_out, logical_PC,(char*)"sd", reg_table->entry_x[CPUID].name, (UINT16)-0x010,(char*)"tp",(char*)"tp[1] Let OS know this core has data to run",param);
						reg_table->entry_x[CPUID].in_use = 0;

						UINT8 thread_count_reg = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
			//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x addi %s, zero, 0x%03x // update thread count\n", logical_PC[0], reg_table->entry_x[thread_count_reg].name, thread_count); logical_PC[0] += 4;
						sprint_op_2src(parse_out, logical_PC, (char*)"addi", reg_table->entry_x[thread_count_reg].name,(char*)"zero", thread_count, (char*)"update thread count", param);
			//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, -0x018(tp) // tp[2] push thread count\n", logical_PC[0], reg_table->entry_x[thread_count_reg].name); logical_PC[0] += 4;
						sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[thread_count_reg].name, (UINT16)-0x018, (char*)"tp", (char*)"tp[2] push thread count", param);
						reg_table->entry_x[thread_count_reg].in_use = 0;

						UINT8 return_addr = alloc_temp_UABI(reg_table, l_control->depth, parse_out, parse_in, Masm);
						load_64bB(parse_out, logical_PC, return_addr, logical_PC[0] + 4,(char *) "PC + 4", reg_table, l_control->depth, parse_in, param, Masm);//return address when all complete
				//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, -0x020(tp) // tp[3] push return address\n", logical_PC[0], reg_table->entry_x[return_addr].name); logical_PC[0] += 4;
						sprint_load(parse_out, logical_PC, (char*)"sd", reg_table->entry_x[return_addr].name, (UINT16)-0x020, (char*)"tp", (char*)"tp[3] push return address", param);
						reg_table->entry_x[return_addr].in_use = 0;
					}
					else {
						debug++;
					}
				//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x fence 0 // order all memory operations 3\n", logical_PC[0]); logical_PC[0] += 4;
					sprintf_string(parse_out, logical_PC, (char*)" fence 0 // order all memory operations 3\n", param);
					l_control->fence[l_control->depth] = 0;
					sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x ecall // generate software interrupt\n", logical_PC[0]); logical_PC[0] += 4;

					sprint_label(parse_out, logical_PC[0], (char*)"label_parallel_invoke", (char*)"", param);
				}
				else {
					debug++;
				}

			}
			else {
				VariableListEntry return_addr;
				addVariableEntry(&return_addr, int64_enum, (char *) "return address", reg_table, l_control, parse_out, parse_in, Masm);
				INT64 number3;
				if (check_funct_arg_dest(parse_out, &number3, &return_addr, parse_in, logical_PC, reg_table, l_control, current, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, param, Masm, unit_debug)) {
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
						parse_in->ptr++;
					}
					else {
						debug++;
					}
					UINT8 num_valid = 0;
					INT64 number2;
					VariableListEntry data;
					addVariableEntry(&data, int64_enum, (char *) "", reg_table, l_control, parse_out, parse_in, Masm);

					parse_rh_of_eqB(parse_out, &data, 0, &num_valid, &number2, logical_PC, parse_in, compiler_var, IO_list, op_table, branch_table, csr_list, csr_list_count, reg_table, l_control, param, Masm, unit_debug);
					if (number3 < 0x800 && number3 > 0) {
						sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x sd %s, %03x(a1) \n", logical_PC[0], reg_table->entry_x[data.reg_index].name, -((number3 + 1) << 3)); logical_PC[0] += 4;
					}
					else
						debug++;
					l_control->fence[l_control->depth] = 1;
					reg_table->entry_x[data.reg_index].in_use = 0;
					data.reg_index_valid = 0;
				}
				else {
					debug++;
				}
				reg_table->entry_x[return_addr.reg_index].in_use = 0;
			}
		}
	}
	//	free(line_pointer.line);
}
void compile_to_RISCV_Masm(parse_struct2* parse_out, const char* dst_name, UINT64* FunctionStack_ct, const char* src_name, parse_struct2* parse_in, memory_space_type* memory_space,
	hw_pointers* pointers, INT64 logical_base, INT64 base, INT flags, compiler_var_type* compiler_var, operator_type* op_table, operator_type* branch_table, param_type* param, INT64 logical_PC_init, UINT8 unit_debug, UINT8 read_src) {
	int debug = 0;

	INT64 logical_PC = logical_PC_init;
	UINT8 RV64_mode = param->satp >> 60;

	compiler_var->list_base = (VariableListEntry*)malloc(sizeof(VariableListEntry));
	compiler_var->list_base->next = compiler_var->list_base;
	compiler_var->list_base->last = compiler_var->list_base;

	compiler_var->f_list_base = (FunctionListEntry*)malloc(sizeof(FunctionListEntry));
	compiler_var->f_list_base->next = compiler_var->f_list_base;
	compiler_var->f_list_base->last = compiler_var->f_list_base;

	var_type_struct csr_list[22];
	UINT8 csr_list_count = 22;
	sprintf_s(csr_list[0].name, "satp");
	sprintf_s(csr_list[1].name, "uepc");
	sprintf_s(csr_list[2].name, "sepc");
	sprintf_s(csr_list[3].name, "vepc");
	sprintf_s(csr_list[4].name, "mepc");
	sprintf_s(csr_list[5].name, "mtvec");
	sprintf_s(csr_list[6].name, "mtval");
	sprintf_s(csr_list[7].name, "stvec");
	sprintf_s(csr_list[8].name, "stval");
	sprintf_s(csr_list[9].name, "scause");
	sprintf_s(csr_list[10].name, "mcause");
	sprintf_s(csr_list[11].name, "mhartid");
	sprintf_s(csr_list[12].name, "mideleg");
	sprintf_s(csr_list[13].name, "medeleg");
	sprintf_s(csr_list[14].name, "sstatus");
	sprintf_s(csr_list[15].name, "mstatus");
	sprintf_s(csr_list[16].name, "iobase");
	sprintf_s(csr_list[17].name, "mbound");
	sprintf_s(csr_list[18].name, "sbound");
	sprintf_s(csr_list[19].name, "sscratch");
	sprintf_s(csr_list[20].name, "mscratch");
	sprintf_s(csr_list[21].name, "_sp");

	var_type_struct modifier_type[2];
	
	sprintf_s(modifier_type[0].name, "none");
	sprintf_s(modifier_type[1].name, "atomic");

	var_type_struct control_type[8];
	sprintf_s(control_type[0].name, "if");
	sprintf_s(control_type[1].name, "for");
	sprintf_s(control_type[2].name, "switch");
	sprintf_s(control_type[3].name, "case");
	sprintf_s(control_type[4].name, "default");
	sprintf_s(control_type[5].name, "break");
	sprintf_s(control_type[6].name, "return");
	sprintf_s(control_type[7].name, "while");

	reg_table_struct* reg_table = (reg_table_struct*)malloc(sizeof(reg_table_struct));
	reg_table->temp = 6;
	reg_table->saved = 8;
	reg_table->arg = 10;
	/*
	// UABI
	reg_table->entry_x[0x00].name = "zero";
	reg_table->entry_x[0x01].name = "ra";
	reg_table->entry_x[0x02].name = "sp";
	reg_table->entry_x[0x03].name = "gp";
	reg_table->entry_x[0x04].name = "tp";
	reg_table->entry_x[0x05].name = "t0";
	reg_table->entry_x[0x06].name = "t1";
	reg_table->entry_x[0x07].name = "t2";
	reg_table->entry_x[0x08].name = "s0";
	reg_table->entry_x[0x09].name = "s1";
	reg_table->entry_x[0x0a].name = "a0";
	reg_table->entry_x[0x0b].name = "a1";
	reg_table->entry_x[0x0c].name = "a2";
	reg_table->entry_x[0x0d].name = "a3";
	reg_table->entry_x[0x0e].name = "a4";
	reg_table->entry_x[0x0f].name = "a5";
	reg_table->entry_x[0x10].name = "a6";
	reg_table->entry_x[0x11].name = "a7";
	reg_table->entry_x[0x12].name = "s2";
	reg_table->entry_x[0x13].name = "s3";
	reg_table->entry_x[0x14].name = "s4";
	reg_table->entry_x[0x15].name = "s5";
	reg_table->entry_x[0x16].name = "s6";
	reg_table->entry_x[0x17].name = "s7";
	reg_table->entry_x[0x18].name = "s8";
	reg_table->entry_x[0x19].name = "s9";
	reg_table->entry_x[0x1a].name = "s10";
	reg_table->entry_x[0x1b].name = "s11";
	reg_table->entry_x[0x1c].name = "t3";
	reg_table->entry_x[0x1d].name = "t4";
	reg_table->entry_x[0x1e].name = "t5";
	reg_table->entry_x[0x1f].name = "t6";
	/**/
	// EABI
	sprintf_s(reg_table->entry_x[0x00].name , "zero");
	sprintf_s(reg_table->entry_x[0x01].name , "ra");
	sprintf_s(reg_table->entry_x[0x02].name , "sp");
	sprintf_s(reg_table->entry_x[0x03].name , "gp");
	sprintf_s(reg_table->entry_x[0x04].name , "tp");
	sprintf_s(reg_table->entry_x[0x05].name , "t00");
	sprintf_s(reg_table->entry_x[0x06].name , "s03");
	sprintf_s(reg_table->entry_x[0x07].name , "s04");
	sprintf_s(reg_table->entry_x[0x08].name , "s00");
	sprintf_s(reg_table->entry_x[0x09].name , "s01");
	sprintf_s(reg_table->entry_x[0x0a].name , "a0");
	sprintf_s(reg_table->entry_x[0x0b].name , "a1");
	sprintf_s(reg_table->entry_x[0x0c].name , "a2");
	sprintf_s(reg_table->entry_x[0x0d].name , "a3");
	sprintf_s(reg_table->entry_x[0x0e].name , "s02");
	sprintf_s(reg_table->entry_x[0x0f].name , "t01");
	sprintf_s(reg_table->entry_x[0x10].name , "s05");
	sprintf_s(reg_table->entry_x[0x11].name , "s06");
	sprintf_s(reg_table->entry_x[0x12].name , "s07");
	sprintf_s(reg_table->entry_x[0x13].name , "s08");
	sprintf_s(reg_table->entry_x[0x14].name , "s09");
	sprintf_s(reg_table->entry_x[0x15].name , "s10");
	sprintf_s(reg_table->entry_x[0x16].name , "s11");
	sprintf_s(reg_table->entry_x[0x17].name , "s12");
	sprintf_s(reg_table->entry_x[0x18].name , "s13");
	sprintf_s(reg_table->entry_x[0x19].name , "s14");
	sprintf_s(reg_table->entry_x[0x1a].name , "s15");
	sprintf_s(reg_table->entry_x[0x1b].name , "s16");
	sprintf_s(reg_table->entry_x[0x1c].name , "s17");
	sprintf_s(reg_table->entry_x[0x1d].name , "s18");
	sprintf_s(reg_table->entry_x[0x1e].name , "s19");
	sprintf_s(reg_table->entry_x[0x1f].name , "s20");

	sprintf_s(reg_table->entry_fp[0x00].name , "f00");
	sprintf_s(reg_table->entry_fp[0x01].name , "f01");
	sprintf_s(reg_table->entry_fp[0x02].name , "f02");
	sprintf_s(reg_table->entry_fp[0x03].name , "f03");
	sprintf_s(reg_table->entry_fp[0x04].name , "f04");
	sprintf_s(reg_table->entry_fp[0x05].name , "f05");
	sprintf_s(reg_table->entry_fp[0x06].name , "f06");
	sprintf_s(reg_table->entry_fp[0x07].name , "f07");
	sprintf_s(reg_table->entry_fp[0x08].name , "f08");
	sprintf_s(reg_table->entry_fp[0x09].name , "f09");
	sprintf_s(reg_table->entry_fp[0x0a].name , "f10");
	sprintf_s(reg_table->entry_fp[0x0b].name , "f11");
	sprintf_s(reg_table->entry_fp[0x0c].name , "f12");
	sprintf_s(reg_table->entry_fp[0x0d].name , "f23");
	sprintf_s(reg_table->entry_fp[0x0e].name , "f14");
	sprintf_s(reg_table->entry_fp[0x0f].name , "f15");
	sprintf_s(reg_table->entry_fp[0x10].name , "f16");
	sprintf_s(reg_table->entry_fp[0x11].name , "f17");
	sprintf_s(reg_table->entry_fp[0x12].name , "f18");
	sprintf_s(reg_table->entry_fp[0x13].name , "f19");
	sprintf_s(reg_table->entry_fp[0x14].name , "f20");
	sprintf_s(reg_table->entry_fp[0x15].name , "f21");
	sprintf_s(reg_table->entry_fp[0x16].name , "f22");
	sprintf_s(reg_table->entry_fp[0x17].name , "f23");
	sprintf_s(reg_table->entry_fp[0x18].name , "f24");
	sprintf_s(reg_table->entry_fp[0x19].name , "f25");
	sprintf_s(reg_table->entry_fp[0x1a].name , "f26");
	sprintf_s(reg_table->entry_fp[0x1b].name , "f27");
	sprintf_s(reg_table->entry_fp[0x1c].name , "f28");
	sprintf_s(reg_table->entry_fp[0x1d].name , "f29");
	sprintf_s(reg_table->entry_fp[0x1e].name , "f30");
	sprintf_s(reg_table->entry_fp[0x1f].name , "f31");

	for (UINT8 i = 0; i < 32; i++) reg_table->entry_x[i].in_use = 0;
	for (UINT8 i = 0; i < 32; i++) reg_table->entry_fp[i].in_use = 0;
	reg_table->entry_x[0].in_use = 1; // zero reg is always in use
	reg_table->entry_x[0].saved = 1; // zero reg is always in use
	IO_list_type IO_list;
//	for (UINT i = 0; i < 0x10; i++) IO_list.name[i] = (char*)malloc(0x100 * sizeof(char));
	IO_list.ptr = 0;

	//CompileType level = generalCode;

	loop_control l_control;
	l_control.depth = 0;
	for (UINT i = 0; i < 0x10; i++) l_control.end[i] = 0;
	for (UINT i = 0; i < 0x10; i++) {
		l_control.count[i] = 0;
		sprintf_s(l_control.line[i], "");
		l_control.line_valid[i] = 0;
		l_control.for_loop[i].detected = 0;
	}

	parse_out->index = 0;

	short sp_offset = 0;
	if (flags & 1) {
		place_IntrExc_headerB(parse_out, logical_PC);
		logical_PC += 0x1000; // keep in same 1MB block
	}
	if (read_src) {
		FILE* src_file;
		fopen_s(&src_file, src_name, "r");
		parse_in->index = 0;
		while (fgets(parse_in->line[parse_in->index++].line, 0x200, src_file) != NULL) {}
		parse_in->index--;
		fclose(src_file);
	}
	reg_table->error = 0;
//	FILE* Masm = fopen(dst_name, "w");
	FILE* Masm;
	fopen_s(&Masm, dst_name, "w");
	FunctionListEntry* current_function = compiler_var->f_list_base->last;
	UINT line_count = parse_in->index;
	for (parse_in->index = 0; parse_in->index < line_count; parse_in->index++) {
		parse_in->ptr = 0;

		clean_temp_reg(reg_table);

		for (VariableListEntry* current = compiler_var->global_list_base->next; current != compiler_var->global_list_base; current = current->next) {
			current->reg_index_valid = 0;
		}

		if (parse_in->index >= 24 && unit_debug)
			debug++;
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
		getWord(parse_in);
		sprintf_s(parse_out->line[parse_out->index].line, "//\t%d\t%s", parse_in->index, parse_in->line[parse_in->index].line);
		if (parse_in->line[parse_in->index].line[strlen(parse_in->line[parse_in->index].line) - 1] != '\n') {
			printf(parse_out->line[parse_out->index].line, "\n");
		}
		parse_out->index++;
		switch (l_control.depth) {
		case 0: {
			reg_table->entry_x[reg_table->error].in_use = 0;
			reg_table->error = 0;
			VariableListEntry* current = compiler_var->global_list_base->next;

			while (compiler_var->list_base->next != compiler_var->list_base) {
				compiler_var->list_base->next->next->last = compiler_var->list_base;
				compiler_var->list_base->next = compiler_var->list_base->next->next;
			}

			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
				parse_in->ptr += 2;
			}
			else if (strcmp(parse_in->word, "IO_port") == 0) {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
				getWord(parse_in);
				sprintf_s(IO_list.name[IO_list.ptr].name, "%s", parse_in->word);
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
				getWord(parse_in);
				INT64 number;
				get_integer(&number, parse_in->word);
				IO_list.addr[IO_list.ptr] = number;
				IO_list.ptr++;
			}
			else if (strcmp(parse_in->word, "#include") == 0) {
				getWord(parse_in);
				if (strcmp(parse_in->word, "OS.h") == 0) {

				}
			}
			else if (parse_FunctionB(compiler_var->f_list_base, parse_in, &l_control, reg_table)) {
				current_function = compiler_var->f_list_base->last;
				current_function->addr = logical_PC;
				sprint_label(parse_out, logical_PC, (char*)"label_", current_function->name, param);
//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: label_%s \n", logical_PC, current_function->name);
				reg_table->arg = 10;
				reg_table->temp = 5;
				reg_table->saved = 8;
				if (logical_PC < 0x80000000 && logical_PC >0x00002000) {
					sp_offset = 0;
				}
				else {
					sp_offset = -64;
				}
				// using subroutine for now. arg registers are set prior to entry, no need to pop the stack
				while (parse_in->line[parse_in->index].line[parse_in->ptr] != ')')parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {// argument collection complete, exit
					parse_in->ptr++;
				}
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {// argument collection complete, exit
					l_control.depth++;
					l_control.line_valid[l_control.depth] = 0;
				}

				if (strcmp(current_function->name, "main") == 0) {
					VariableListEntry* funct_list_gp = compiler_var->global_list_base->next;
					if (get_VariableListEntryB(&funct_list_gp, (char*)"funct_list_gp", compiler_var->global_list_base)) {
			//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD 0x0000000000000000 // address for \"main\"\n", funct_list_gp->addr - 8); // entry address
						parse_out->ptr = 0; 
						sprint_addr_p2(parse_out, funct_list_gp->addr - 8, param);
						strcat_p2(parse_out, (char*)" DD 0x0000000000000000 // address for \"main\"\n"); parse_out->index++; parse_out->ptr = 0;
			//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD 0x%016I64x // address for \"main\"\n", funct_list_gp->addr - 2 * 8, logical_PC); // entry address
						UINT64 base = funct_list_gp->addr;
						char word[0x80];
						sprintf_s(word, " DD 0x%016I64x // address for \"main\"\n", logical_PC); // entry address
						sprint_addr_p2(parse_out, funct_list_gp->addr - 2 * 8, param);
						strcat_p2(parse_out, word); parse_out->index++; parse_out->ptr = 0;
						for (UINT i = 0; i < 0x010; i++) {// name
					//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD 0x0000000000000000 \n", base - 8 * (i + 8) + 8);// passed variables
					//		sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD 0x0000000000000000 \n", base - 8 * (i + 8));// passed variables
							sprint_addr_p2(parse_out, base - 8 * (i + 8) + 8, param);
							strcat_p2(parse_out, (char*)" DD 0x0000000000000000 \n"); parse_out->index++; parse_out->ptr = 0;
							sprint_addr_p2(parse_out, base - 8 * (i + 8), param);
							strcat_p2(parse_out, (char*)" DD 0x0000000000000000 \n"); parse_out->index++; parse_out->ptr = 0;
						}
					}
					else {
						debug++;
					}
				}
			}
			else if (get_VariableTypeNameB(compiler_var->global_list_base, parse_in, reg_table, &l_control, parse_out, Masm)) {
				current = compiler_var->global_list_base->last;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
					parse_in->ptr++;
					getWord(parse_in);
					INT64 number;
					if (get_integer(&number, parse_in->word)) {
						current->addr = pointers->OS_gp + compiler_var->OS_gp_offset;
						current->addr = (current->addr & (-0x80));
						sprint_label(parse_out, current->addr, (char*)"", current->name, param);
		//				sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: %s \n", current->addr, current->name);
						compiler_var->OS_gp_offset = current->addr - pointers->OS_gp;
						compiler_var->OS_gp_offset -= 8 * number;
						current->pointer = 1;
						current->size = number;
					}
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
						parse_in->ptr++;
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
							parse_in->ptr++;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
								parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								UINT8 sign_flag = 0;
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == '-') {
									parse_in->ptr++;
									sign_flag = 1;
								}
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								if (get_integer(&number, parse_in->word)) {
									switch (current->type) {
									case int8_enum:
									case uint8_enum:
										if (sign_flag)
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DB 0x%02x \n", current->addr - 1, (char)-number);
										else
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DB 0x%02x \n", current->addr - 1, (char)number);
										break;
									case int16_enum:
									case uint16_enum:
									case fp16_enum:
										if (sign_flag)
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DW 0x%04x \n", current->addr - 2, -number);
										else
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DW 0x%04x \n", current->addr - 2, number);
										break;
									case int64_enum:
									case uint64_enum:
									case fp64_enum:
										if (sign_flag)
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD 0x%016x \n", current->addr - 8, -number);
										else
											sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD 0x%016x \n", current->addr - 8, number);
										break;
									default:
										debug++;
										break;
									}
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
									UINT i = 2;
									while (parse_in->line[parse_in->index].line[parse_in->ptr] != '}') {
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\0')
											debug++;
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',') {
											parse_in->ptr++;
										}
										else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\n') {
											parse_in->index++;
											parse_in->ptr = 0;
											sprintf_s(parse_out->line[parse_out->index++].line, "//\t%d\t%s", parse_in->index, parse_in->line[parse_in->index].line);
										}
										else {
											debug++;
										}
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
										sign_flag = 0;
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == '-') {
											parse_in->ptr++;
											sign_flag = 1;
										}
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
										getWord(parse_in);
										if (get_integer(&number, parse_in->word)) {
											switch (current->type) {
											case int8_enum:
											case uint8_enum:
												if (sign_flag)
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DB 0x%02x \n", current->addr - i, -number);
												else
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DB 0x%02x \n", current->addr - i, number);
												break;
											case int16_enum:
											case uint16_enum:
											case fp16_enum:
												if (sign_flag)
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DH 0x%04x \n", current->addr - 2 * i, -number);
												else
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DH 0x%04x \n", current->addr - 2 * i, number);
												break;
											case int64_enum:
											case uint64_enum:
											case fp64_enum:
												if (sign_flag)
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD 0x%016x \n", current->addr - 8 * i, -number);
												else
													sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x DD 0x%016x \n", current->addr - 8 * i, number);
												break;
											default:
												debug++;
												break;
											}
											i++;
										}
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
									}
									debug++;
								}
								else {
									debug++;
								}
							}
							else {
								debug++;
							}
						}
					}
					else {
						debug++;
					}
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
					parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
						parse_in->ptr++;
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						getWord(parse_in);
						data_type_enum type;
						control_status_reg_type csr_reg;
						if (type_decode(&type, parse_in->word)) {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*')
								parse_in->ptr++;
							else
								debug++;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
								parse_in->ptr++;
							else
								debug++;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							if (strcmp(parse_in->word, "malloc") == 0) {
								//						parse_in->ptr += 6;
							}
							else
								debug++;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(')
								parse_in->ptr++;
							else
								debug++;
							INT64 number;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							if (get_integer(&number, parse_in->word)) {
							}
							else {
								debug++;
							}
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*')
								parse_in->ptr++;
							else
								debug++;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							if (strcmp(parse_in->word, "sizeof") == 0) {
								//						parse_in->ptr += 6;
							}
							else
								debug++;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(')
								parse_in->ptr++;
							else
								debug++;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							if (strcmp(parse_in->word, "UINT8") == 0 || strcmp(parse_in->word, "_fp16") == 0) {
								//						parse_in->ptr += 5;
							}
							else
								debug++;

							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
								parse_in->ptr++;
							else
								debug++;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
								parse_in->ptr++;
							else
								debug++;
							current->addr = pointers->gp + compiler_var->gp_offset - (2 * number);
							sprint_label(parse_out, current->addr, (char*)"", current->name, param);
						//	sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: %s \n", current->addr, current->name);
							compiler_var->gp_offset -= (2 * number);
							current->pointer = 0;
						}
						else if (RISC_V_csr_decode(&csr_reg, parse_in->word)) {
							debug++;
						}
						else
							debug++;
					}
					else {
						debug++;
					}
				}
				else {
					current->addr = pointers->OS_gp + compiler_var->OS_gp_offset - 8;
					sprint_label(parse_out, current->addr, (char*)"", current->name, param);
		//			sprintf_s(parse_out->line[parse_out->index++].line, "0x%016I64x // label: %s \n", current->addr, current->name);
					compiler_var->OS_gp_offset -= 8;
					current->pointer = 0;
				}
			}
			else {
				debug++;
			}
		}
			  break;
		default:
			ParseFunctionCode(parse_out, parse_in, &l_control, &logical_PC, control_type, modifier_type, branch_table, csr_list, csr_list_count, op_table,
				reg_table, memory_space, pointers, base, compiler_var, &sp_offset, current_function, &IO_list, param, Masm, unit_debug);
			break;
		}
	}
	print_parse_out(Masm, parse_out);
//	for (UINT i = 0; i < 0x10; i++)free(IO_list.name[i]);
	free(reg_table);
	free(compiler_var->list_base);
	free(compiler_var->f_list_base);
}