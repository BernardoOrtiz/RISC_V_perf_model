// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

struct int128_target_type {
	char name[0x80];
	char depth;
	char valid;
};

void array16toReg128(const char* dst_name, const char* src_name, parse_struct2_set* parse, UINT8 unit_debug) {
	int debug = 0;
//	FILE* src = fopen(src_name, "r");
	FILE* src;
	fopen_s(&src, src_name, "r");

	UINT  line_count = 0;

	while (fgets(parse->a.line[line_count++].line, 0x200, src) != NULL) {}
	line_count--;

	fclose(src);
	parse_struct2* parse_in = &parse->a;
	parse_struct2* parse_out = &parse->b;
	char depth;
	char tab[0x80];
	char rhs, pass;

	int128_target_type int128_target;
	int128_target.valid = 0;

	depth = 0;
	for (parse_in->index = 0, parse_out->index = 0; parse_in->index < line_count; parse_in->index++, parse_out->index++) {
		parse_in->ptr = 0;
		parse_out->ptr = 0;
		rhs = 0;
		pass = 0;
		if (parse_in->index == 440)
			debug++;
		skip_blanks(parse_out, parse_in);
		sprintf_s(tab,"%s", parse_out->line[parse_out->index].line);
		while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
			skip_blanks(parse_out, parse_in);
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
				depth++;
				copy_char(parse_out, parse_in);
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '}') {
				depth--;
				copy_char(parse_out, parse_in);
				if (int128_target.valid) {
					if (int128_target.depth > depth)
						int128_target.valid = 0;
				}
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') 	copy_char(parse_out, parse_in);
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
				rhs = 1;
				copy_char(parse_out, parse_in);
			}
			else {
				getWord(parse_in);
				if (parse_in->word[0]!='\0') {
					char word[0x80];
					if (strcmp(parse_in->word, "_fp16") == 0) {
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						getWord(parse_in);
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
							parse_in->ptr++;
							//		char name[0x80];
							if (int128_target.valid)
								debug++;
							sprintf_s(int128_target.name,"%s", parse_in->word);
							int128_target.depth = depth;
							getWord(parse_in);
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
								parse_in->ptr++;
								INT64 number;
								if (get_integer(&number, parse_in->word)) {
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] != ';') {
										sprintf_s(word, "_fp16 %s[%s] ", int128_target.name, parse_in->word);
										strcat_p2(parse_out, word);
										debug++;
									}
									else if (number <= 8) {
										debug++;
									}
									else if (number <= 16) {
										sprintf_s(word, "UINT128 %s_i128_a;\n", int128_target.name);
										strcpy_p2(parse_out, word);
										parse_out->index++;
										sprintf_s(word, "%sUINT128 %s_i128_b;\n", tab, int128_target.name);
										strcpy_p2(parse_out, word);
										parse_out->index++;

										sprintf_s(word, "%sUINT16 %s_i16_a;\n", tab, int128_target.name);
										strcpy_p2(parse_out, word);
										parse_out->index++;

										sprintf_s(word, "%sUINT16 %s_i16_b;\n", tab, int128_target.name);
										strcpy_p2(parse_out, word);

										int128_target.valid = 1;
										parse_in->ptr = strlen(parse_in->line[parse_in->index].line);
									}
									else {
										strcpy_p2(parse_out, parse_in);
									}
								}
								else {
									strcpy_p2(parse_out, parse_in);
								}
							}
							else {
								strcpy_p2(parse_out, parse_in);
							}
						}
						else {
							sprintf_s(word, "_fp16 %s ", parse_in->word);
							strcat_p2(parse_out, word);
						}
					}
					else if (int128_target.valid) {
						if (strcmp(int128_target.name, parse_in->word) == 0) {
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
								parse_in->ptr++;
								getWord(parse_in);
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
									parse_in->ptr++;
									INT64 number;
									if (get_integer(&number, parse_in->word)) {
										if (rhs) {
											char temp_line[0x80];
											sprintf_s(temp_line, "%s", parse_out->line[parse_out->index].line);
											if (pass == 0) {
												sprintf_s(word, "%s%s_i16_a = ", tab, int128_target.name);
											}
											else if (pass == 1) {
												sprintf_s(word, "%s%s_i16_b = ", tab, int128_target.name);
											}
											else {
												debug++;
											}
											strcpy_p2(parse_out, word);
											if (number < 8) {
												sprintf_s(word, "(%s_i128_a>>%d)&0x0000ffff;\n", int128_target.name, number * 16);
											}
											else {
												sprintf_s(word, "(%s_i128_b>>%d)&0x0000ffff;\n", int128_target.name, number * 16 - 128);
											}
											strcat_p2(parse_out, word);
											parse_out->index++;
											strcpy_p2(parse_out, temp_line);
											if (pass == 0) {
												sprintf_s(word, "(_fp16) (%s_i16_a)", int128_target.name);
											}
											else {
												sprintf_s(word, "(_fp16) (%s_i16_b)", int128_target.name);
											}
											strcat_p2(parse_out, word);
											pass++;
										}
										else {
											sprintf_s(word, "%s_i16_a", int128_target.name);
											strcat_p2(parse_out, word);
											while (parse_in->line[parse_in->index].line[parse_in->ptr] != '=')
												copy_char(parse_out, parse_in);
											copy_char(parse_out, parse_in);
											strcat_p2(parse_out, (char *) " (UINT16)(");
											while (parse_in->line[parse_in->index].line[parse_in->ptr] != ';')
												copy_char(parse_out, parse_in);
											strcat_p2(parse_out, (char*) ")");
											while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
												copy_char(parse_out, parse_in); 
											if (number < 8) {
												sprintf_s(word, "%s%s_i128_a |= (%s_i16_a<<%d);\n", tab, int128_target.name, int128_target.name, number * 16);
												strcpy_p2(parse_out, word);
											}
											else if (number < 16) {
												sprintf_s(word, "%s%s_i128_b |= (%s_i16_a<<%d);\n", tab, int128_target.name, int128_target.name, (number * 16) - 128);
												strcpy_p2(parse_out, word);
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
						}
						else {
							strcat_p2(parse_out,parse_in->word);
						}
					}
					else {
						strcpy_p2(parse_out, parse_in);
					}
				}
				else {
					copy_char(parse_out, parse_in);
				}
			}
		}
	}
	line_count = parse_out->index;
	FILE* dest;
	fopen_s(&dest,dst_name, "w");

	for (parse_out->index = 0; parse_out->index < line_count; parse_out->index++) {
		fprintf(dest, "%s", parse_out->line[parse_out->index].line);
	}
	fclose(dest);
	UINT ptr = strlen(parse_out->line[line_count - 1].line);
	parse_out->line[line_count - 1].line[ptr] = '\0';

}