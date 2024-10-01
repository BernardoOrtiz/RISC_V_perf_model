// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

enum word_type :UINT8 {
	word_unknown = 0,
	word_number = 1,
	word_hex = 2,
	word_scalar = 3,
	word_vector = 4,
	word_function = 5
};
struct var_entry {
	char type[0x80];
	char name[0x80];
	INT64 size;
	char ptr;
};
struct var_list {
	var_entry entry[0x10];
	UINT8 count;
};
word_type get_Word_V(parse_struct2* parse) {
	int debug = 0;
	UINT8 length = strlen(parse->line[parse->index].line);
	UINT8 i;
	word_type result = word_unknown;

	while ((parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\t') && parse->line[parse->index].line[parse->ptr] != '\0')parse->ptr++;

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
	if (parse->word[0]!='\0') {
		switch (parse->word[0]) {
		case 0:
			if (parse->word[1] == 'x')
				result = word_hex; // hex
			else
				result = word_number; // number
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			result = word_number; // number
			break;
		default:
			result = word_scalar; // scalar 
			if (parse->line[parse->index].line[parse->ptr] == '[') {
				result = word_vector; // vector
			}
			else if (parse->line[parse->index].line[parse->ptr] == '(') {
				result = word_function; // function
			}
			// check for type define
			break;
		}
	}
	return result;
}
void store_load_ellimination(const char* dst_name, const char* src_name, operator_type* op_table, var_type_struct* var_type) {
	int debug = 0;
	parse_struct2 parse_1;
	parse_struct2 parse_2;
	parse_struct2 parse_a_next;
	parse_1.line = (line_struct*)malloc(0x400 * sizeof(line_struct));
	parse_2.line = (line_struct*)malloc(0x400 * sizeof(line_struct));
	parse_a_next.line = (line_struct*)malloc(0x400 * sizeof(line_struct));
	parse_1.index = 0;
	parse_2.index = 0;

	parse_struct2 parse_t;
	parse_t.line = (line_struct*)malloc(sizeof(line_struct));
	parse_t.index = 0;

	UINT  line_count = 0;
	FILE* src;
	fopen_s(&src,src_name, "r");
	while (fgets(parse_1.line[line_count++].line, 0x200, src) != NULL) {}
	line_count--;
	fclose(src);

	int depth = 0;

	parse_struct2* parse_a = &parse_1;
	parse_struct2* parse_b = &parse_2;

	parse_struct2 parse_c;
	for (UINT pass = 0; pass < 1; pass++) {//6
		if (pass & 1) {
			parse_b = &parse_1;
			parse_a = &parse_2;
		}
		else {
			parse_a = &parse_1;
			parse_b = &parse_2;
		}
		parse_b->index = 0;
		parse_c.line = parse_a->line;
		for (parse_a->index = 0; parse_a->index < line_count; parse_a->index++) {
			sprintf_s(parse_a_next.line[parse_a->index].line, "%s", parse_a->line[parse_a->index].line);
		}

		var_list list;
		list.count = 0;
		for (parse_a->index = 0; parse_a->index < line_count; parse_a->index++, parse_b->index++) {
			if (parse_a->index >= 360)
				debug++;
			parse_a->ptr = 0;
			parse_b->ptr = 0;
			while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')
				copy_char(parse_b, parse_a);
			if (parse_a->line[parse_a->index].line[parse_a->ptr] == '}') {
				list.count = 0;
				strcpy_p2(parse_b, parse_a);
				depth--;
				if (depth < 0)
					debug++;
			}
			else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{') {
				depth++;
				if (depth > 0x10)
					debug++;
			}
			else {
				getWord(parse_a);
				if (parse_a->line[parse_a->index].line[parse_a->ptr] == '[') {
					UINT match = 0;
					for (UINT i = 0; i < list.count; i++) {
						if (strcmp(list.entry[i].name, parse_a->word) == 0) {
							match = 1;
							// create tag, variable +index that must be matched in further lines to replace;
							char tag[0x80];
							strcpy_word(tag, parse_a);
							UINT ptr = strlen(tag);
							while (parse_a->line[parse_a->index].line[parse_a->ptr] != ']') tag[ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
							tag[ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
							tag[ptr] = '\0';
							while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')parse_a->ptr++;
							if (parse_a->line[parse_a->index].line[parse_a->ptr] == '=') {
								parse_a->ptr++;
							}
							else {
								debug++;
							}
							while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')parse_a->ptr++;
							// check next line for prescense of tag
							parse_a_next.index = parse_a->index + 1;
							parse_a_next.ptr = 0;
							parse_t.ptr = 0;
							UINT stop = 0;
							while (parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] != '\0' && !stop) {
								while (parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] == '\t' || parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] == ' ')	
									parse_t.line[0].line[parse_t.ptr++] = parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr++];
								parse_t.line[0].line[parse_t.ptr] = '\0';
								switch (parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr]) {
								case '}':
									strcpy_p2(parse_b, parse_a);
									stop = 1;
									break;
								default:
									debug++;
									while (parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] != '=' && parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] != '\0')	
										parse_t.line[0].line[parse_t.ptr++] = parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr++];
									parse_t.line[0].line[parse_t.ptr] = '\0';
									if (parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] == '=') {
										parse_t.line[0].line[parse_t.ptr++] = parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr++];
										parse_t.line[0].line[parse_t.ptr] = '\0';
										UINT repeat = 1;
										while (repeat) {
											repeat = 0;
											while (parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] == '\t' || parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] == ' ' ||
												parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] == '(' || parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] == '*')	parse_t.line[0].line[parse_t.ptr++] = parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr++];
											parse_t.line[0].line[parse_t.ptr] = '\0';
											getWord(&parse_a_next);
											if (strcmp(list.entry[i].name, parse_a_next.word) == 0) {
												char next_tag[0x80];
												strcpy_word(next_tag, &parse_a_next);
												char next_ptr = strlen(next_tag);

												while (parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] != ']') next_tag[next_ptr++] = parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr++];
												next_tag[next_ptr++] = parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr++];
												next_tag[next_ptr] = '\0';
												debug++;
												if (strcmp(tag, next_tag) == 0) {
													strcat_p2(parse_b, list.entry[i].type);
													strcat_p2(parse_b,(char*) " ");
													strcat_p2(parse_b, list.entry[i].name);
													strcat_p2(parse_b, (char*)"_reg = ");
										//			parse_b->ptr = strlen(parse_b->line[parse_b->index].line);
													while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0') 
														parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
													parse_b->line[parse_b->index++].line[parse_b->ptr] = '\0';
													debug++; // parse all subsequent lines for substitution until '}' is found
													UINT parse_next_line = 1;
													while (parse_next_line) {
														parse_a->index++;
														if (parse_a->index >= 360)
															debug++;
														parse_a->ptr = 0;
														parse_b->ptr = 0;

														parse_a_next.index = parse_a->index + 1;
														parse_a_next.ptr = 0;

														while (parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] != '\0' && parse_next_line) {
															if (parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] == '}') {
																parse_next_line = 0;
																stop = 1;
																parse_a_next.ptr = 0;
																while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0' && parse_a->line[parse_a->index].line[parse_a->ptr] != '=') parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
																if (parse_a->line[parse_a->index].line[parse_a->ptr] == '=') parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
																parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
															}
															else {
																parse_a_next.ptr++;
															}
														}

														while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0') {

															while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t') parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
															parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
															getWord(parse_a);
															if (parse_a->word[0]!='\0') {
																if (strcmp(list.entry[i].name, parse_a->word) == 0) {
																	strcpy_word(next_tag, parse_a);
																	char next_ptr = strlen(next_tag);

																	if (parse_a->line[parse_a->index].line[parse_a->ptr] != '[')
																		debug++;

																	while (parse_a->line[parse_a->index].line[parse_a->ptr] != ']') next_tag[next_ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
																	next_tag[next_ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
																	next_tag[next_ptr] = '\0';
																	if (strcmp(tag, next_tag) == 0) {
																		strcat_p2(parse_b, list.entry[i].name);
																		strcat_p2(parse_b, (char*)"_reg");
																	}
																	else {
																		strcat_p2(parse_b, next_tag);
																	}
																}
																else {
																	strcat_p2(parse_b, parse_a->word);
																}
																parse_b->ptr = strlen(parse_b->line[parse_b->index].line);
															}
															else {
																parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
																parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
															}
														}
														if (!parse_next_line) {
															debug++;
														}
														parse_b->index++;
													}
													parse_b->index--;
												}
											}
											else {
												debug++;
												repeat = 1;
											}
										}
									}
									break;
								}
							}
							while (parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] == '\t' || parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] == ' ')parse_a_next.ptr++;
							if (parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] == '}') {
								parse_a->ptr = 0;
								parse_b->ptr = 0;
							}
							else {
								getWord(&parse_a_next);
								if (strcmp(list.entry[i].name, parse_a_next.word) == 0) {
									char next_tag[0x80];
									strcpy_word(next_tag, &parse_a_next);
									char next_ptr = strlen(next_tag);

									while (parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr] != ']') next_tag[next_ptr++] = parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr++];
									next_tag[next_ptr++] = parse_a_next.line[parse_a_next.index].line[parse_a_next.ptr++];
									next_tag[next_ptr] = '\0';
									debug++;
								}
							}
						}
					}
					if (!match) {
						strcpy_p2(parse_b, parse_a);
					}
				}
				else {
					strcat_p2(parse_b, parse_a->word);
					parse_b->ptr = strlen(parse_b->line[parse_b->index].line);
					UINT hit = 0;
					for (UINT8 i = 0; i < var_type_count && !hit; i++) {
						if (strcmp(parse_a->word, var_type[i].name) == 0) {
							hit = 1;
							strcpy_word(list.entry[list.count].type, parse_a);
							while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')	parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
							parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
							getWord(parse_a);
							strcat_p2(parse_b, parse_a->word);
							while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')parse_a->ptr++;
							if (parse_a->line[parse_a->index].line[parse_a->ptr] == '(') { // function definition
								UINT8 stop_loop = 0;
								parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
								parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
								while (!stop_loop) {
									stop_loop = 1;
									while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')	parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
									parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
									getWord(parse_a);
									UINT8 hit = 0;
									for (UINT8 j = 0; j < var_type_count; j++) {
										if (strcmp(parse_a->word, var_type[j].name) == 0) {
											hit = 1;
											strcpy_word(list.entry[list.count].type, parse_a);
											while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')	parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
											parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
											if (parse_a->line[parse_a->index].line[parse_a->ptr] == '*') {
												parse_a->ptr++;
												list.entry[list.count].ptr = 1;
											}
											else {
												list.entry[list.count].ptr = 0;
											}
											getWord(parse_a);
											strcpy_word(list.entry[list.count++].name, parse_a);
											if (parse_a->line[parse_a->index].line[parse_a->ptr] == '[') {
												list.entry[list.count].ptr = 1;
												while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0' || parse_a->line[parse_a->index].line[parse_a->ptr] != ']')	parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
												parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
												parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
											}
											while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')	parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
											parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
											if (parse_a->line[parse_a->index].line[parse_a->ptr] == ',') {
												parse_a->ptr++;
												stop_loop = 0;
											}
											else if (parse_a->line[parse_a->index].line[parse_a->ptr] == ')') {
												parse_a->ptr++;
												while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')	parse_a->ptr++;
												if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{') {
													depth++;
													parse_a->ptr++;
												}
											}
											else {
												debug++;
											}
										}
									}
									if (!hit)
										debug++;
								}
								strcpy_p2(parse_b, parse_a);
							}
							else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '[') {
								parse_a->ptr++;
								// integer or float, if float, count < 16, check visibility, replace vector with variable expansion (not stores)
								strcpy_word(list.entry[list.count].name, parse_a);
								list.entry[list.count].ptr = 1;
								getWord(parse_a);
								UINT scan_end = 0;
								int running_total = 0;
								while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')parse_a->ptr++;
								if (parse_a->line[parse_a->index].line[parse_a->ptr] == ']') {
									parse_a->ptr++;
									if (get_integer(&list.entry[list.count].size, parse_a->word)) {
										running_total += list.entry[list.count].size;
										if (list.entry[list.count].size <= 0x16 && strcmp(list.entry[list.count].type, "_fp16") == 0) {
											debug++;// check for '{' till you find  '}'
											for (parse_c.index = parse_a->index + 1; parse_c.index < line_count && scan_end == 0; parse_c.index++) {
												parse_c.ptr = 0;
												while (parse_c.line[parse_c.index].line[parse_c.ptr] != '\0') {
													if (parse_c.line[parse_c.index].line[parse_c.ptr] == '{') {
														scan_end = 1;
													}
													else if (parse_c.line[parse_c.index].line[parse_c.ptr] == '}') {
														scan_end = 2;
													}
													parse_c.ptr++;
												}
											}
										}
									}
								}
								if (scan_end == 2) {
									UINT count_start = list.count;
									char header[0x80];
									UINT k = 0;
									for (; k <= depth; k++)header[k] = '\t';
									header[k++] = '_';
									header[k++] = 'f';
									header[k++] = 'p';
									header[k++] = '1';
									header[k++] = '6';
									header[k++] = ' ';
									header[k] = '\0';
									sprintf_s(parse_b->line[parse_b->index++].line, "%s %s_0;\n",header, list.entry[list.count].name);
									debug++;
									for (UINT k = 1; k < list.entry[list.count].size; k++) {
										sprintf_s(parse_b->line[parse_b->index++].line, "%s %s_%d;\n",header, list.entry[list.count].name, k);
									}
									list.count++;

																		// complete gathering variables
									while (parse_a->line[parse_a->index].line[parse_a->ptr] == ',') {
										parse_a->ptr++;
										sprintf_s(list.entry[list.count].type,"%s", list.entry[list.count - 1].type);
										while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')parse_a->ptr++;
										getWord(parse_a);
										strcpy_word(list.entry[list.count].name, parse_a);
										if (parse_a->line[parse_a->index].line[parse_a->ptr] == '[') {
											list.entry[list.count].ptr = 1;
											parse_a->ptr++;
											getWord(parse_a);
											if (parse_a->line[parse_a->index].line[parse_a->ptr] == ']') {
												parse_a->ptr++;
												if (get_integer(&list.entry[list.count].size, parse_a->word)) {
													running_total += list.entry[list.count].size;
													if (running_total > 20)
														debug++;
													for (UINT k = 0; k < list.entry[list.count].size; k++) {
														sprintf_s(parse_b->line[parse_b->index++].line, "%s %s_%d;\n",header, list.entry[list.count].name, k);
													}
													list.count++;
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
									for (parse_a->index++; parse_a->index < parse_c.index; parse_a->index++, parse_b->index++) {
										parse_a->ptr = 0;
										parse_b->ptr = 0;
										if (parse_a->index >= 360)
											debug++;
										while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0') {
											while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')	parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
											parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
											getWord(parse_a);
											if (parse_a->word[0]!='\0') {
												if (parse_a->line[parse_a->index].line[parse_a->ptr] == '[') {
													parse_a->ptr++;
													strcat_p2(parse_b, parse_a->word);
													char entry_hit = 0;
													for (UINT k = count_start; k < list.count; k++) {
														if (strcmp(list.entry[k].name, parse_a->word) == 0) {
															entry_hit = 1;
															getWord(parse_a);
															INT64 number;
															if (get_integer(&number, parse_a->word)) {
																strcat_p2(parse_b, (char*)"_");
																strcat_p2(parse_b, parse_a->word);
																parse_a->ptr++;
															}
															else {
																debug++;
															}
														}
													}
													if (!entry_hit) {
														parse_b->line[parse_b->index].line[parse_b->ptr++] = '[';
														while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0' && parse_a->line[parse_a->index].line[parse_a->ptr] != ']')	parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
														if (parse_a->line[parse_a->index].line[parse_a->ptr] == ']')	parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
														parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
													}
												}
												else {
													strcat_p2(parse_b, parse_a->word);
												}
											}
											else {
												parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
												parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
											}
										}
										debug++;
									}
									list.count = 0;
									parse_a->index--;
									parse_b->index--;
									debug++;
								}
								else {
									list.count++;
									strcpy_p2(parse_b, parse_a);
								}
							}
							else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '=' || parse_a->line[parse_a->index].line[parse_a->ptr] == '*' || parse_a->line[parse_a->index].line[parse_a->ptr] == ';') {
								strcpy_p2(parse_b, parse_a);
							}
							else {
								debug++;
							}
						}
					}
					if (!hit) {
						while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0') {
							if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{')
								depth++;
							parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
						}
						parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
					}
				}
			}
		}
	}
	FILE* dest;
	fopen_s(&dest,dst_name, "w");
	for (UINT i = 0; i < parse_b->index; i++) {
		fprintf(dest, "%s", parse_b->line[i]);
	}
	fclose(dest);
	free(parse_1.line);
	free(parse_2.line);
	free(parse_a_next.line);

}