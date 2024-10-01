// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

struct loop4_type {
	char name[0x80];
	char alias[0x80];
	int init, inc, tag, alias_depth;
};
struct list_type {
	loop4_type entry[0x10];
	char count;
};
void stripVarDeclarations(list_type* list, parse_struct2* parse_out, parse_struct2* parse_in, list_type* loop4, char depth_check, int line_count) {
	int debug = 0;
	int depth = depth_check;
	parse_out->ptr = 0;
	parse_struct2 parse;
	parse.index = parse_in->index + 1;
	parse.ptr = 0;
	parse.line = parse_in->line;
	if (parse.index >= 1280)
		debug++;
	while (depth >= depth_check && parse.index < line_count) {
		if (parse.index >= 2390)
			debug++;
		if (parse.index >= 1280)
			debug++;
		while (parse.line[parse.index].line[parse.ptr] != '\0') {
			if (parse.index >= 2438)
				debug++;
			skip_blanks(parse_out, &parse);
			if (parse.line[parse.index].line[parse.ptr] == '/' && parse.line[parse.index].line[parse.ptr + 1] == '/') {
				copy2end(parse_out, &parse);
			}
			else if (parse.line[parse.index].line[parse.ptr] == '{') {
				depth++;
				copy_char(parse_out, &parse);
			}
			else if (parse.line[parse.index].line[parse.ptr] == '}') {
				depth--;
				copy_char(parse_out, &parse);
				if (depth == depth_check && loop4->count >= 2) {
					parse_out->line[parse_out->index].line[0] = '\0';
					for (UINT i = 0; i < depth; i++) {
						strcat_p2(parse_out, (char *)"\t");
					}
					char word[0x80];
					sprintf_s(word, "%s -= 0x%x;// stack order\n", loop4->entry[1].name, loop4->entry[1].inc);
					strcat_p2(parse_out, parse_in->word);
					parse_out->index++;
					strcpy_p2(parse_out, &parse);
				}
				if (list->count > 0) {
					if (strlen(list->entry[list->count - 1].alias) > 0 && list->entry[list->count - 1].alias_depth > depth) {
						list->count--;
					}
				}
			}
			else {
				getWord(&parse);
				int rename = 0;
				for (UINT i = 0; i < list->count; i++) {
					if (strlen(list->entry[i].alias)) {
						if (strcmp(list->entry[i].alias, parse.word) == 0) {
							strcat_p2(parse_out, list->entry[i].name);
							rename = 1;
						}
					}
				}
				if (rename == 0) {
					if (strcmp(parse.word, "_fp16") == 0) {
						strcat_p2(parse_out, parse.word);
						skip_blanks(parse_out, &parse);
						if (parse.line[parse.index].line[parse.ptr] == '*')
							copy_char(parse_out, &parse);
						skip_blanks(parse_out, &parse);
						getWord(&parse);
						UINT skip = 0;
						for (UINT i = 0; i < list->count; i++) {
							if (strcmp(list->entry[i].name, parse.word) == 0) {
								if (list->entry[i].tag != parse.index) {// name match && not self
									if (parse.index == 2667)
										debug++;
									sprintf_s(list->entry[list->count].name, "%s_%d", parse.word, list->count);
									sprintf_s(list->entry[list->count].alias,"%s", parse.word);
									list->entry[list->count].alias_depth = depth;
									if (i > 9)
										debug++;
								
									skip = 1;
									char word[0x80];
									sprintf_s(word, "%s = %s ", list->entry[list->count++].name, list->entry[i].name);
									strcat_p2(parse_out, word);
									int ptr = 0;
									while (parse.line[list->entry[i].tag].line[ptr] == parse.line[parse.index].line[ptr])ptr++;
									while (parse.line[list->entry[i].tag].line[ptr] == ' ' || parse.line[list->entry[i].tag].line[ptr] == '\t')
										parse_out->line[parse_out->index].line[parse_out->ptr++] = parse.line[list->entry[i].tag].line[ptr++];
									if (parse.line[parse.index].line[ptr] != ']' && parse.line[parse.index].line[ptr] != ';') {
										while (parse.line[parse.index].line[ptr] != '|' && parse.line[parse.index].line[ptr] != '+' && parse.line[parse.index].line[ptr] != '+') ptr--;
									}
									int ptr2 = ptr;
									char hit_offset = 0;
									if (parse.line[list->entry[i].tag].line[ptr] == '-') {
										parse_out->line[parse_out->index].line[parse_out->ptr++] = '+';
										ptr++;
										hit_offset = 1;
									}
									else if (parse.line[list->entry[i].tag].line[ptr] == '|' || parse.line[list->entry[i].tag].line[ptr] == '+') {
										parse_out->line[parse_out->index].line[parse_out->ptr++] = '-';
										ptr++;
										hit_offset = 1;
									}
									else {
										debug++;
									}
									if (hit_offset)
										parse_out->line[parse_out->index].line[parse_out->ptr++] = '(';
									while (parse.line[list->entry[i].tag].line[ptr] != ']' && parse.line[list->entry[i].tag].line[ptr] != '\0')
										parse_out->line[parse_out->index].line[parse_out->ptr++] = parse.line[list->entry[i].tag].line[ptr++];
									if (hit_offset)
										parse_out->line[parse_out->index].line[parse_out->ptr++] = ')';

									//							hit_offset = 0;
									if (parse.line[parse.index].line[ptr2] == ']' || parse.line[parse.index].line[ptr2] == ';') {
										// pass, do nothing
									}
									else if (parse.line[parse.index].line[ptr2] == '-') {
										parse_out->line[parse_out->index].line[parse_out->ptr++] = '-';
										ptr2++;
										//								hit_offset = 1;
									}
									else if (parse.line[parse.index].line[ptr2] == '|' || parse.line[parse.index].line[ptr2] == '+') {
										parse_out->line[parse_out->index].line[parse_out->ptr++] = '+';
										ptr2++;
										//								hit_offset = 1;
									}
									else {
										debug++;
									}
									//								if (hit_offset)
									//									parse_out->line[parse_out->index].line[parse_out->ptr++] = '(';
									while (parse.line[parse.index].line[ptr2] != ']' && parse.line[parse.index].line[ptr2] != ';' && parse.line[parse.index].line[ptr2] != '\0') {
										if (parse.line[parse.index].line[ptr2] == '|') {
											parse_out->line[parse_out->index].line[parse_out->ptr++] = '+';
											ptr2++;
										}
										else
											parse_out->line[parse_out->index].line[parse_out->ptr++] = parse.line[parse.index].line[ptr2++];
									}
									//								if (hit_offset)
									//									parse_out->line[parse_out->index].line[parse_out->ptr++] = ')';
									parse_out->line[parse_out->index].line[parse_out->ptr++] = ';';
									parse_out->line[parse_out->index].line[parse_out->ptr++] = '\n';
									parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
									parse.ptr = strlen(parse.line[parse.index].line);
								}
								else {
									debug++;
									parse.ptr = strlen(parse.line[parse.index].line);
									parse_out->ptr = 0;
									parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
									parse.word[0] = '\0';
								}
							}
						}
						if (!skip) {
							strcat_p2(parse_out, parse.word);
							skip_blanks(parse_out, &parse);
							if (parse.line[parse.index].line[parse.ptr] == '=') {
								copy_char(parse_out, &parse);
								skip_blanks(parse_out, &parse);
								if (parse.line[parse.index].line[parse.ptr] == '&')
									copy_char(parse_out, &parse);
								else
									debug++;
								skip_blanks(parse_out, &parse);
								getWord(&parse);
								if (parse.word[0]!='\0') {
									strcat_p2(parse_out, parse.word);
								}
								else {
									debug++;
								}
								skip_blanks(parse_out, &parse);
								if (parse.line[parse.index].line[parse.ptr] == '[') {
									copy_char(parse_out, &parse);
									skip_blanks(parse_out, &parse);
									while (parse.line[parse.index].line[parse.ptr] != ']' && parse.line[parse.index].line[parse.ptr] != '\0') {
										getWord(&parse);
										if (parse.word[0] != '\0') {
											if (strcmp(parse.word, loop4->entry[0].name) == 0) {
												char word[0x80];
												sprintf_s(word, "0x%x", loop4->entry[0].init);
												strcat_p2(parse_out, word);
												int hit = 0;
												while (parse.line[parse.index].line[parse.ptr] != ']' && parse.line[parse.index].line[parse.ptr] != '\0') {
													skip_blanks(parse_out, &parse);
													if (parse.line[parse.index].line[parse.ptr] == '<' && parse.line[parse.index].line[parse.ptr + 1] == '<') {
														copy_char(parse_out, &parse);
														copy_char(parse_out, &parse);
														skip_blanks(parse_out, &parse);
														getWord(&parse);
														if (parse.word[0] != '\0') {
															INT64 number;
															strcat_p2(parse_out, word);
															if (get_integer(&number, parse.word)) {
																hit = 1;
															}
															else {
																debug++;
															}
														}
														else
															debug++;
													}
													else if (parse.line[parse.index].line[parse.ptr] == ')') {
														copy_char(parse_out, &parse);
													}
													else {
														copy_char(parse_out, &parse);
													}
												}
												while (parse.line[parse.index].line[parse.ptr] != '\0') copy_char(parse_out, &parse);
												if (hit) {
													parse_out->index--;
													parse.ptr = strlen(parse.line[parse.index].line);
													parse_out->ptr = strlen(parse_out->line[parse_out->index].line);
												}
											}
											else {
												strcat_p2(parse_out, parse.word);
											}
										}
										else {
											copy_char(parse_out, &parse);
										}
										skip_blanks(parse_out, &parse);
									}
									if (parse.line[parse.index].line[parse.ptr] == ']')
										copy_char(parse_out, &parse);
									else
										debug++;
									if (parse.line[parse.index].line[parse.ptr] == ';')
										copy_char(parse_out, &parse);
									else
										debug++;
									if (parse.line[parse.index].line[parse.ptr] == '\n')
										copy_char(parse_out, &parse);
									else
										debug++;
								}
								else {
									copy2end(parse_out, &parse);
								}
							}
							else if (parse.line[parse.index].line[parse.ptr] == '[') {
								for (UINT i = 0; i < list->count; i++) {
									if (strcmp(list->entry[i].name, parse.word) == 0 && list->entry[i].tag == parse.index) {
										parse_out->index--;
										parse.ptr = strlen(parse.line[parse.index].line);
										parse_out->ptr = strlen(parse_out->line[parse_out->index].line);
									}
								}
							}
							else
								copy2end(parse_out, &parse);
						}
					}
					else if (parse.word[0] != '\0') {
						strcat_p2(parse_out, parse.word);
						if (list->entry[list->count - 1].alias[0]!='\0') {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
								getWord(&parse);
								if (parse.word[0] != '\0') {
									if (strcmp(parse.word, list->entry[list->count - 1].alias) == 0) {
										strcat_p2(parse_out, list->entry[list->count - 1].name);
									}
									else {
										strcat_p2(parse_out, parse.word);
									}
								}
								else {
									copy_char(parse_out, parse_in);
								}
							}
						}
						else {
							copy2end(parse_out, &parse);
						}
					}
					else {
						copy_char(parse_out, &parse);
					}
				}
			}
		}
		if (depth >= depth_check) {
			parse.index++;
			parse.ptr = 0;
			parse_out->index++;
			parse_out->ptr = 0;
		}
		else if (list->count == 0) {
			parse.index++;
			parse.ptr = 0;
		}/**/
	}
	parse_in->index = parse.index;
	parse_in->ptr = parse.ptr;
}
void decode4(list_type* loop4, parse_struct2* parse_in) {
	int debug = 0;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(')
		parse_in->ptr++;
	else
		debug++;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
	getWord(parse_in);
	if (parse_in->word[0]!='\0') {
		if (strcmp(parse_in->word, "UINT") == 0 || strcmp(parse_in->word, "UINT8") == 0 || strcmp(parse_in->word, "UINT16") == 0 || strcmp(parse_in->word, "int") == 0) {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
			getWord(parse_in);
			if (parse_in->word_ptr != 0) {
				strcpy_word(loop4->entry[loop4->count].name, parse_in);
			}
			else
				debug++;
		}
		else
			debug++;
	}
	else
		debug++;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=')
		parse_in->ptr++;
	else
		debug++;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
	getWord(parse_in);
	if (parse_in->word_ptr != 0) {
		INT64 number;
		if (get_integer(&number, parse_in->word))
			loop4->entry[loop4->count].init = number;
		else
			debug++;
	}
	else
		debug++;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';')
		parse_in->ptr++;
	else
		debug++;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != ';' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')parse_in->ptr++;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';')
		parse_in->ptr++;
	else
		debug++;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
	getWord(parse_in);
	if (parse_in->word_ptr != 0) {
		if (strcmp(parse_in->word, loop4->entry[loop4->count].name) != 0)
			debug++;
	}
	else {
		debug++;
	}
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '+') {
		parse_in->ptr += 2;
		loop4->entry[loop4->count].inc = 1;
	}
	else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
		parse_in->ptr += 2;
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
		getWord(parse_in);
		INT64 number;
		if (get_integer(&number, parse_in->word))
			loop4->entry[loop4->count].inc = number;
		else
			debug++;
	}
	else
		debug++;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != ')' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')parse_in->ptr++;
	if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
		parse_in->ptr++;
	else
		debug++;
	loop4->count++;
}

void pullVarDeclarations(list_type* list, parse_struct2* parse_out, parse_struct2* parse_in, list_type* loop4, char depth_check, int line_count) {
	int debug = 0;
	int depth = depth_check;
	parse_out->ptr = 0;
	parse_struct2 parse;
	parse.index = parse_in->index + 1;
	parse.ptr = 0;
	parse.line = parse_in->line;
	while (depth >= depth_check && parse.index < line_count) {
		while (parse.line[parse.index].line[parse.ptr] != '\0') {
			skip_blanks(parse_out, &parse);
			if (parse.line[parse.index].line[parse.ptr] == '/' && parse.line[parse.index].line[parse.ptr + 1] == '/') {
				copy2end(parse_out, &parse);
			}
			else if (parse.line[parse.index].line[parse.ptr] == '{') {
				depth++;
				copy_char(parse_out, &parse);
			}
			else if (parse.line[parse.index].line[parse.ptr] == '}') {
				depth--;
				copy_char(parse_out, &parse);
			}
			else {
				getWord(&parse);
				if (strcmp(parse.word, "_fp16") == 0) {
					strcat_p2(parse_out, parse.word);
					skip_blanks(parse_out, &parse);
					int pointer = 0;
					if (parse.line[parse.index].line[parse.ptr] == '*') {
						copy_char(parse_out, &parse);
						pointer = 1;
					}
					skip_blanks(parse_out, &parse);
					getWord(&parse);
					int skip = 0;
					for (UINT i = 0; i < list->count && !skip; i++) {
						if (strcmp(list->entry[i].name, parse.word) == 0)
							skip++;
					}
					if (parse.index >= 956)
						debug++;
					if (skip) {
						strcpy_p2(parse_out, &parse);
					}
					else {
						strcpy_word(list->entry[list->count].name, &parse);
						list->entry[list->count].alias[0]='\0';
						list->entry[list->count].tag = parse.index;
						strcat_p2(parse_out, parse.word);
						skip_blanks(parse_out, &parse);
						if (parse.line[parse.index].line[parse.ptr] == '=') {
							copy_char(parse_out, &parse);
							skip_blanks(parse_out, &parse);
							if (parse.line[parse.index].line[parse.ptr] == '&') {
								copy_char(parse_out, &parse);
								skip_blanks(parse_out, &parse);
								getWord(&parse);
								if (parse.word[0] != '\0') {
									strcat_p2(parse_out, parse.word);
								}
								else {
									debug++;
								}
								skip_blanks(parse_out, &parse);
								if (parse.line[parse.index].line[parse.ptr] == '[') {
									copy_char(parse_out, &parse);
								}
								else {
									debug++;
								}
								skip_blanks(parse_out, &parse);
								int hit = 0;
								while (parse.line[parse.index].line[parse.ptr] != ']' && parse.line[parse.index].line[parse.ptr] != '\0') {
									getWord(&parse);
									if (parse.word[0] != '\0') {
										UINT hit2 = 0;
										for (UINT i = 0; i < loop4->count; i++) {
											if (strcmp(parse.word, loop4->entry[i].name) == 0) {
												hit2 = 1;
												char word[0x80];
												sprintf_s(word, "0x%x", loop4->entry[i].init);
												strcat_p2(parse_out, word);
												skip_blanks(parse_out, &parse);
												if (parse.line[parse.index].line[parse.ptr] == ')') {
													copy_char(parse_out, &parse);
													skip_blanks(parse_out, &parse);
												}
												if (parse.line[parse.index].line[parse.ptr] == '<' && parse.line[parse.index].line[parse.ptr + 1] == '<') {
													copy_char(parse_out, &parse);
													copy_char(parse_out, &parse);
													skip_blanks(parse_out, &parse);
													getWord(&parse);
													if (parse.word[0] != '\0') {
														INT64 number;
														strcat_p2(parse_out, parse.word);
														if (get_integer(&number, parse.word)) {
															list->entry[list->count].init = loop4->entry[0].init << number;
															list->entry[list->count++].inc = loop4->entry[0].inc << number;
															if (list->count >= 0x10)
																debug++;
															hit = 1;
														}
														else {
															debug++;
														}
													}
													else
														debug++;
												}
												else if (parse.line[parse.index].line[parse.ptr] == ')') {
													copy_char(parse_out, &parse);
												}
												else {
													copy_char(parse_out, &parse);
												}
											}
										}
										if (hit2 == 0) {
											strcat_p2(parse_out, parse.word);
										}
									}
									else {
										copy_char(parse_out, &parse);
									}
									skip_blanks(parse_out, &parse);
								}
								if (parse.line[parse.index].line[parse.ptr] == ']')
									copy_char(parse_out, &parse);
								else
									debug++;
								if (parse.line[parse.index].line[parse.ptr] == ';')
									copy_char(parse_out, &parse);
								else
									debug++;
								if (parse.line[parse.index].line[parse.ptr] == '\n')
									copy_char(parse_out, &parse);
								else
									debug++;
								if (hit) {
									parse_out->index++;
									parse_out->ptr = 0;
								}
							}
							else if (pointer == 1) {
								skip_blanks(parse_out, &parse);
								getWord(&parse);
								for (UINT i = 0; i < list->count && !skip; i++) {
									if (strcmp(list->entry[i].name, parse.word) == 0) {
										skip++;
										list->entry[list->count].init = list->entry[i].init;
										list->entry[list->count++].inc = list->entry[i].inc;
									}
								}
								skip = 0;
								strcpy_p2(parse_out, &parse);
								parse_out->index++;
								parse_out->line[parse_out->index].line[0] = '\0';
								parse_out->ptr = 0;
							}
							else {
								copy2end(parse_out, &parse);
							}
						}
						else if (parse.line[parse.index].line[parse.ptr] == '[') {
							copy_char(parse_out, &parse);
							skip_blanks(parse_out, &parse);
							getWord(&parse);
							strcat_p2(parse_out, parse.word);
							skip_blanks(parse_out, &parse);
							if (parse.line[parse.index].line[parse.ptr] == ']' && parse.line[parse.index].line[parse.ptr + 1] == ';') {
								copy2end(parse_out, &parse);
								INT64 number;
								if (get_integer(&number, parse.word)) {
									sprintf_s(list->entry[list->count].alias,"%s", list->entry[list->count].name);
									list->entry[list->count].tag = parse.index;
									list->entry[list->count].init = 0;
									list->entry[list->count++].inc = 0;
									parse_out->index++;
									parse_out->ptr = 0;
									if (list->count >= 0x10)
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
						else
							copy2end(parse_out, &parse);
					}
				}
				else if (strcmp(parse.word, "for") == 0) {
					decode4(loop4, &parse);
					if (parse.line[parse.index].line[parse.ptr] == '{') {
						depth++;
						parse.ptr++;
					}
					if (parse.line[parse.index].line[parse.ptr] == '/' && parse.line[parse.index].line[parse.ptr + 1] == '/') {
						while (parse.line[parse.index].line[parse.ptr] != '\0') 	copy_char(parse_out, &parse);
					}
					else if (parse.line[parse.index].line[parse.ptr] != '\n')
						debug++;
					strcpy_p2(parse_out, &parse);
				}
				else {
					copy2end(parse_out, &parse);
				}
			}
		}
		if (depth >= depth_check) {
			parse.index++;
			parse.ptr = 0;
			parse_out->ptr = 0;
		}
	}
//	parse_out->index--;
}
void VarDeclarationEllimination4loop(const char* dst_name, const char* src_name, parse_struct2_set* parse, UINT8 unit_debug) {
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
	list_type ptr_list, loop4;
	ptr_list.count = 0;
	loop4.count = 0;
	parse_ptr = ((++parse_ptr) & 1);

	depth = 0;
	for (parse_in->index = 0, parse_out->index = 0; parse_in->index < line_count; parse_in->index++, parse_out->index++) {
		parse_in->ptr = 0;
		parse_out->ptr = 0;
		if (parse_in->index == 728)
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
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') 	copy_char(parse_out, parse_in);
			}
			else {
				getWord(parse_in);
				if (parse_in->word_ptr != 0) {
					if (strcmp(parse_in->word, "for") == 0) {
						if (parse_in->index >= 1285)
							debug++;

						ptr_list.count = 0;
						loop4.count = 0;
						decode4(&loop4, parse_in);
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
							depth++;
							parse_in->ptr++;
						}
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') 	copy_char(parse_out, parse_in);
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr] != '\n')
							debug++;
						pullVarDeclarations(&ptr_list, parse_out, parse_in, &loop4, depth, line_count);
						// place for loop statement
						strcpy_p2(parse_out, parse_in);
						parse_out->index++;
						stripVarDeclarations(&ptr_list, parse_out, parse_in, &loop4, depth, line_count);
						for (int i = 0; i < ptr_list.count; i++) {
							if (ptr_list.entry[i].inc != 0) {
								UINT tab[0x10];
								UINT j = 0;
								for (; j < depth; j++) tab[j] = '\t';
								tab[j] = '\0';
								if (i == 0 && ptr_list.count > 1) {
									if (ptr_list.entry[1].inc == ptr_list.entry[0].inc) {
										sprintf_s(parse_out->line[parse_out->index++].line, "%sINT %s_inc = 0x%x;\n", 
											tab, ptr_list.entry[i].name, ptr_list.entry[0].inc);
										sprintf_s(parse_out->line[parse_out->index++].line, "%s%s -= %s_inc;// remember stack order\n", 
											tab, ptr_list.entry[0].name, ptr_list.entry[0].name);
										sprintf_s(parse_out->line[parse_out->index++].line, "%s%s -= %s_inc;// remember stack order\n",
											tab, ptr_list.entry[1].name, ptr_list.entry[0].name);
										parse_in->ptr = 0;
										i = 2;
										while (ptr_list.entry[i].inc == ptr_list.entry[0].inc && i < ptr_list.count) {
											sprintf_s(parse_out->line[parse_out->index++].line, "%s%s -= %s_inc;// remember stack order\n",
												tab, ptr_list.entry[i++].name, ptr_list.entry[0].name);
										}
									}
								}
								if (i < ptr_list.count) {
									sprintf_s(parse_out->line[parse_out->index++].line, "%s%s -= 0x%x;// remember stack order\n",tab, ptr_list.entry[i].name, ptr_list.entry[i].inc);
									parse_in->ptr = 0;
									parse_out->ptr = 0;
								}
							}
						}
						// place var incr
						debug++;// allow parser to continue with closing parenthesis
					}
					else {
						strcat_p2(parse_out, parse_in->word);
					}
				}
				else {
					copy_char(parse_out, parse_in);
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
	fclose(dest);
	UINT ptr = strlen(parse_out->line[line_count - 1].line);
	parse_out->line[line_count - 1].line[ptr] = '\0';

}