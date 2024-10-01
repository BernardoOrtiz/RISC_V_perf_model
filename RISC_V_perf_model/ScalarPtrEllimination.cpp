// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

struct ptr_entry_type {
	short depth, offset;
	char type[0x100], name[0x100], alias[0x100];
};
void skip_blanks(parse_struct2* parse_out, parse_struct2* parse_in) {
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
		parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
	parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
}
void copy_char(parse_struct2* parse_out, parse_struct2* parse_in) {
	parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
	parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
}
void copy2end(parse_struct2* parse_out, parse_struct2* parse_in) {
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
		parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
	parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
}
void ScalarPtrEllimination(const char* dst_name, const char* src_name, parse_struct2_set* parse, UINT8 unit_debug) {
	int debug = 0;
	FILE* src;
	fopen_s(&src, src_name, "r");

	UINT  line_count = 0;

	while (fgets(parse->a.line[line_count++].line, 0x200, src) != NULL) {}
	line_count--;

	fclose(src);
	parse_struct2* parse_in = &parse->a;
	parse_struct2* parse_out = &parse->b;
	//	UINT touched = 1;
	UINT8 parse_ptr = 1;
	char depth;
	ptr_entry_type ptr_entry[0x10];
	char ptr_index = 0;
	parse_ptr = ((++parse_ptr) & 1);

	depth = 0;
	ptr_entry[ptr_index].depth = 0;
	for (parse_in->index = 0, parse_out->index = 0; parse_in->index < line_count; parse_in->index++, parse_out->index++) {
		parse_in->ptr = 0;
		parse_out->ptr = 0;
		if (parse_in->index >= 6342)
			debug++;
		while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
			skip_blanks(parse_out, parse_in);
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
				depth++;
				copy_char(parse_out, parse_in);
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '}') {
				depth--;
				copy_char(parse_out, parse_in);
				ptr_index = 0;
			}
			else {
				getWord(parse_in);
				if (parse_in->word[0]!='\0') {
					if (strcmp(parse_in->word, "_fp16") == 0) {
						strcat_p2(parse_out, parse_in->word);
						skip_blanks(parse_out, parse_in);
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
							sprintf_s(ptr_entry[ptr_index].type,"%s", parse_in->word);
							copy_char(parse_out, parse_in);
							skip_blanks(parse_out, parse_in);
							getWord(parse_in);
							strcat_p2(parse_out, parse_in->word);
							sprintf_s(ptr_entry[ptr_index].name,"%s", parse_in->word);
							skip_blanks(parse_out, parse_in);
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
								copy_char(parse_out, parse_in);
								skip_blanks(parse_out, parse_in);
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == '&') {
									copy_char(parse_out, parse_in);
									skip_blanks(parse_out, parse_in);
									ptr_entry[ptr_index].depth = depth;
									getWord(parse_in);
									strcat_p2(parse_out, parse_in->word);
									sprintf_s(ptr_entry[ptr_index].alias,"%s", parse_in->word);
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
										copy_char(parse_out, parse_in);
										skip_blanks(parse_out, parse_in);
										char word[0x100];
										int ptr = 0;
										while (parse_in->line[parse_in->index].line[parse_in->ptr] != ']') {
											word[ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr];
											copy_char(parse_out, parse_in);
										}
										word[ptr] = '\0';
										INT64 number;
										if (get_integer(&number, word)) {
											if (number < 0x0800) {
												ptr_entry[ptr_index++].offset = number;
												parse_out->index--;
												parse_in->ptr = strlen(parse_in->line[parse_in->index].line);
											}
											else {
												copy2end(parse_out, parse_in);
											}
										}
										else {
											copy2end(parse_out, parse_in);
										}
									}
									else {// syntax error??
										debug++;
									}
								}
								else { // pointer to a pointer??
									copy2end(parse_out, parse_in);
								}
							}
							else {
								copy2end(parse_out, parse_in);
							}
						}
						else {
							if (ptr_index == 0)
								copy2end(parse_out, parse_in);
							else {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
									getWord(parse_in);
									if (parse_in->word_ptr != 0) {
										char hit = 0;
										for (UINT i = 0; i < ptr_index && hit == 0; i++) {
											if (strcmp(parse_in->word, ptr_entry[i].name) == 0) {
												hit = 1;
												strcat_p2(parse_out, ptr_entry[i].alias);
												if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
													copy_char(parse_out, parse_in);
													while (parse_in->line[parse_in->index].line[parse_in->ptr] != ']')
														copy_char(parse_out, parse_in);
													char offset[0x80];
													sprintf_s(offset, " + 0x%03x", ptr_entry[i].offset);
													strcat_p2(parse_out, offset);
													copy_char(parse_out, parse_in);
												}
												else {
													debug++;
												}
											}
										}
										if (hit == 0) {
											strcat_p2(parse_out, parse_in->word);
										}
									}
									else {
										strcat_p2(parse_out, parse_in->word);
									}
								}
							}
						}
					}
					else {
						if (ptr_index == 0) {
							strcat_p2(parse_out, parse_in->word);
						}
						else {
							strcat_p2(parse_out, parse_in->word);
							while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
								skip_blanks(parse_out, parse_in);
								getWord(parse_in);
								if (parse_in->word_ptr != 0) {
									char hit = 0;
									for (UINT i = 0; i < ptr_index && hit == 0; i++) {
										if (strcmp(parse_in->word, ptr_entry[i].name) == 0) {
											hit = 1;
											strcat_p2(parse_out, ptr_entry[i].alias);
											if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
												copy_char(parse_out, parse_in);
												while (parse_in->line[parse_in->index].line[parse_in->ptr] != ']')
													copy_char(parse_out, parse_in);
												char offset[0x80];
												sprintf_s(offset, " + 0x%03x", ptr_entry[i].offset);
												strcat_p2(parse_out, offset);
												copy_char(parse_out, parse_in);
											}
											else {
												debug++;
											}
										}
									}
									if (hit == 0) {
										strcat_p2(parse_out, parse_in->word);
									}
								}
								else {
									parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
									parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
								}
							}
						}
					}
				}
				else {
					parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
					parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
				}
			}
		}
	}
//	FILE* dest = fopen(dst_name, "w");
	line_count = parse_out->index;
	FILE* dest;
	fopen_s(&dest, dst_name, "w");

	for (parse_out->index = 0; parse_out->index < line_count; parse_out->index++) {
		fprintf(dest, "%s", parse_out->line[parse_out->index].line);
	}
	UINT ptr = strlen(parse_out->line[line_count - 1].line);
	parse_out->line[line_count - 1].line[ptr] = '\0';

	fclose(dest);
}