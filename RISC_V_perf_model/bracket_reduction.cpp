// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

struct pair_struct {
	UINT head, tail, next;
	UINT full, valid;
};

void strcat_p2(parse_struct2* parse, char* word) {
	UINT8 ptr = 0;
	while (word[ptr] != '\0' && ptr < 0xff) parse->line[parse->index].line[parse->ptr++] = word[ptr++];
	parse->line[parse->index].line[parse->ptr] = '\0';
}
void strcpy_p2(parse_struct2* parse, char* word) {
	UINT8 ptr = 0;
	parse->ptr = 0;
	while (word[ptr] != '\0' && ptr < 0xff) parse->line[parse->index].line[parse->ptr++] = word[ptr++];
	parse->line[parse->index].line[parse->ptr] = '\0';
}
void strcpy_p2(parse_struct2* parse_out, parse_struct2* parse_in) {
	parse_in->ptr = 0;
	parse_out->ptr = 0;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0' && parse_in->ptr < 0xff) parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
	parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
}
void strcat_p2(parse_struct2* parse_out, parse_struct2* parse_in) {
	parse_in->ptr = 0;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0' && parse_in->ptr < 0xff) parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
	parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
}

void bracket_reduction(const char* dst_name, parse_struct2_set* parse, operator_type* op_table) {
	int debug = 0;
//	FILE* dest = fopen(dst_name, "w");

	FILE* dest;
	fopen_s(&dest, dst_name, "w");

	UINT  line_count = parse->a.index;

	parse_struct2* parse_a ;
	parse_struct2* parse_b ;

	pair_struct pairs[0x100];
	for (UINT i = 0; i < 0x100; i++) {
		pairs[i].full = 0;
		pairs[i].valid = 0;
	}
	UINT pair_ptr = 0;

	UINT8 skip[0x2000];
	for (UINT i = 0; i < 0x2000; i++)skip[i] = 0;

	for (UINT pass = 0; pass < 1; pass++) {//6
		if (pass & 1) {
			parse_b = &parse->a;
			parse_a = &parse->b;
		}
		else {
			parse_a = &parse->a;
			parse_b = &parse->b;
		}
		for (parse_a->index = 0; parse_a->index < line_count; parse_a->index++) {
			if (parse_a->index == 0x24)
				debug++;
			parse_a->ptr = 0;
			parse_b[parse_a->index].ptr = 0;

			strcpy_p2(parse_b, parse_a);
			UINT full = 0;
			while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0' && !(parse_a->line[parse_a->index].line[parse_a->ptr] == '/' && parse_a->line[parse_a->index].line[parse_a->ptr + 1] == '/')) {
				if (parse_a->line[parse_a->index].line[parse_a->ptr] != ' ' && parse_a->line[parse_a->index].line[parse_a->ptr] != '\t' && parse_a->line[parse_a->index].line[parse_a->ptr] != '{') full = 1;
				if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{') {
					pairs[pair_ptr].valid = 1;
					pairs[pair_ptr].full = full;
					parse_struct2 parse_a2;
					parse_a2.index = parse_a->index + 1;
					parse_a2.ptr = 0;
					full = 0;
					while (full == 0) {
						while (parse_a->line[parse_a2.index].line[parse_a2.ptr] != '\0' && !(parse_a->line[parse_a2.index].line[parse_a->ptr] == '/' && parse_a->line[parse_a2.index].line[parse_a2.ptr + 1] == '/')) {
							if (parse_a->line[parse_a2.index].line[parse_a2.ptr] != ' ' && parse_a->line[parse_a2.index].line[parse_a->ptr] != '\t') full = 1;
							parse_a2.ptr++;
						}
						if (full)
							pairs[pair_ptr].next == parse_a2.index;
						else
							parse_a2.index++;
					}
					pairs[pair_ptr++].head = parse_a->index;
				}
				if (parse_a->line[parse_a->index].line[parse_a->ptr] == '}') {
					pair_ptr--;
					pairs[pair_ptr].tail = parse_a->index;
					if (pairs[pair_ptr + 1].valid == 1) {
						if (pairs[pair_ptr + 1].full == 1 && pairs[pair_ptr].full == 0) {
							if (pairs[pair_ptr + 1].head == (pairs[pair_ptr].head + 1)) {
								sprintf_s(parse_b->line[pairs[pair_ptr].head].line, "// %s", parse_a->line[pairs[pair_ptr].head].line);
								sprintf_s(parse_b->line[pairs[pair_ptr].tail].line, "// %s", parse_a->line[pairs[pair_ptr].tail].line);
								skip[pairs[pair_ptr].head] = 1;
								skip[pairs[pair_ptr].tail] = 1;
							}
						}
						else if (pairs[pair_ptr + 1].full == 0 && pairs[pair_ptr].full == 1) {
							sprintf_s(parse_b->line[pairs[pair_ptr + 1].head].line, "// %s", parse_a->line[pairs[pair_ptr + 1].head].line);
							sprintf_s(parse_b->line[pairs[pair_ptr + 1].tail].line, "// %s", parse_a->line[pairs[pair_ptr + 1].tail].line);
							skip[pairs[pair_ptr + 1].head] = 1;
							skip[pairs[pair_ptr + 1].tail] = 1;
						}
					}
				}
				parse_a->ptr++;
			}
			debug++;
		}
	}
	for (UINT i = 0; i < line_count; i++) {
		sprintf_s(parse_a->line[i].line, "%s", parse_b->line[i].line);
		if (skip[i] == 0)
			fprintf(dest, "%s", parse_b[i].line);
	}
	fclose(dest);
}