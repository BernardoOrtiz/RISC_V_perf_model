// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

struct funct_param_type {
	char type[0x10];
	char name[0x80];
	char alias[0x80];
	int value;
	UINT8 strobe, init;
};
struct funct_target_type {
	char alias[0x80];
	UINT index;
	funct_param_type param[0x10];
	UINT8 param_count;
};

struct funct_call_type {
	char type[0x10];
	char name[0x80];
	funct_param_type param[0x10];
	UINT8 param_count;
	funct_target_type target[8];
	UINT8 target_count;

	UINT start_index, stop_index;
	UINT hit, modified;
};
struct funct_call_set_type {
	funct_call_type funct[0x10];
	int count;
};
void markup_funct_call(funct_call_type* funct_call, parse_struct2* parse_a) {
	int debug = 0;

	UINT depth = 0;
//	sprintf_s(funct_call->name,"%s", parse_a->word);
	strcpy_word(funct_call->name, parse_a);
	funct_call->start_index = parse_a->index;
	funct_call->param_count = 0;
	if (parse_a->line[parse_a->index].line[parse_a->ptr] == '(')
		parse_a->ptr++;
	else
		debug++;
	while (parse_a->line[parse_a->index].line[parse_a->ptr] != ')') {
		getWord(parse_a);
		if (strcmp(parse_a->word, "INT8") != 0 && strcmp(parse_a->word, "UINT8") != 0 &&
			strcmp(parse_a->word, "UINT16") != 0 && strcmp(parse_a->word, "_fp16") != 0 &&
			strcmp(parse_a->word, "UINT") != 0) {
			debug++;
		}
	//	sprintf_s(funct_call->param[funct_call->param_count].type, "%s",parse_a->word);
		strcpy_word(funct_call->param[funct_call->param_count].type, parse_a);
		while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
		if (parse_a->line[parse_a->index].line[parse_a->ptr] == '*')
			parse_a->ptr++;
		while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
		getWord(parse_a);
		funct_call->param[funct_call->param_count].init = 0;
	//	sprintf_s(funct_call->param[funct_call->param_count++].name,"%s", parse_a->word);
		strcpy_word(funct_call->param[funct_call->param_count++].name, parse_a);
		if (parse_a->line[parse_a->index].line[parse_a->ptr] == ',')
			parse_a->ptr++;
		else if (parse_a->line[parse_a->index].line[parse_a->ptr] != ')')
			debug++;
		while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
	}
	while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0') {
		if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{')
			depth++;
		parse_a->ptr++;
	}
	if (depth != 1)
		debug++;
	while (depth > 0) {
		parse_a->index++;
		parse_a->ptr = 0;
		char check_param = 1;
		while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0') {
			if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{') {
				depth++;
				parse_a->ptr++;
			}
			else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '}') {
				depth--;
				parse_a->ptr++;
			}
			else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '\\' && parse_a->line[parse_a->index].line[parse_a->ptr + 1] == '\\') {
				while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0')parse_a->ptr++;
			}
			else if (check_param) {
				check_param = 1;
				getWord(parse_a);
				if (parse_a->word_ptr == 0)
					parse_a->ptr++;
				else {
					while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
					if (parse_a->line[parse_a->index].line[parse_a->ptr] == '=' || parse_a->line[parse_a->index].line[parse_a->ptr + 1] == '=') {
						for (UINT i = 0; i < funct_call->param_count; i++) {
							if (strcmp(funct_call->param[i].name, parse_a->word) == 0) {
								debug++;
								funct_call->param[i].init = 1;
							}
						}
					}
				}
			}
			else {
				parse_a->ptr++;
			}
		}
	}
	funct_call->stop_index = parse_a->index;
	parse_a->index++;
}
char search4thread_call(parse_struct2* parse_b, funct_call_type* funct_call, parse_struct2* parse_a, UINT  line_count) {
	int debug = 0;
	char hit = 0;
	for (; parse_a->index < line_count; parse_a->index++) {
		parse_a->ptr = 0;
		while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
		if (parse_a->line[parse_a->index].line[parse_a->ptr] == '[' && parse_a->line[parse_a->index].line[parse_a->ptr + 1] == '&' &&
			parse_a->line[parse_a->index].line[parse_a->ptr + 2] == ']' && parse_a->line[parse_a->index].line[parse_a->ptr + 3] == '{') {
			parse_a->ptr += 4;
			while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
			getWord(parse_a);
			if (strcmp(parse_a->word, funct_call->name) == 0) {
				// check for constants, place special funct calls if needed, track replacements
				funct_call->hit++;
				if (parse_a->line[parse_a->index].line[parse_a->ptr] != '(')
					debug++;
				parse_a->ptr++;
				int  entry_ptr = 0;

				while (parse_a->line[parse_a->index].line[parse_a->ptr] != ')') {
					while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
					getWord(parse_a);
					while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
					funct_call->target[funct_call->target_count].param[entry_ptr].strobe = 0;
					if (parse_a->line[parse_a->index].line[parse_a->ptr] == ',' || parse_a->line[parse_a->index].line[parse_a->ptr] == ')') {
						INT64 number;
						sprintf_s(funct_call->target[funct_call->target_count].param[entry_ptr].alias,"%s", parse_a->word);
						if (get_integer(&number, parse_a->word)) {
							debug++;
							funct_call->modified++;
							funct_call->target[funct_call->target_count].param[entry_ptr].strobe = 1;
							funct_call->target[funct_call->target_count].param[entry_ptr].value = number;
							hit++;
						}
						if (parse_a->line[parse_a->index].line[parse_a->ptr] == ',') {
							parse_a->ptr++;
						}
					}
					else {
						while (parse_a->line[parse_a->index].line[parse_a->ptr] != ',' && parse_a->line[parse_a->index].line[parse_a->ptr] != ')')	parse_a->ptr++;
						parse_a->ptr++;
					}
					entry_ptr++;
				}
				if (hit) {
					// place function call header
					parse_b->ptr = 0;
					while (parse_a->line[funct_call->start_index].line[parse_b->ptr] == ' ' || parse_a->line[funct_call->start_index].line[parse_b->ptr] == '\t') {
						parse_b->line[parse_b->index].line[parse_b->ptr] = parse_a->line[funct_call->start_index].line[parse_b->ptr];
						parse_b->ptr++;
					}
					parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
					strcat_p2(parse_b, funct_call->type);
					strcat_p2(parse_b, (char*)" ");
					strcat_p2(parse_b, funct_call->name);
					for (UINT i = 0; i < entry_ptr; i++) {
						strcat_p2(parse_b, (char*)"_");
						if (funct_call->target[funct_call->target_count].param[i].strobe == 0)
							strcat_p2(parse_b, (char*)"x");
						else {
							strcat_p2(parse_b, funct_call->target[funct_call->target_count].param[i].alias);
						}
					}
					strcat_p2(parse_b, (char*)"(");
					UINT ptr = 0;
					while (parse_a->line[funct_call->start_index].line[ptr] != '(')ptr++;
					ptr++;
					for (UINT i = 0; i < entry_ptr; i++) {
						getWord(parse_a, funct_call->start_index, &ptr);
						sprintf_s(funct_call->target[funct_call->target_count].param[i].name, "%s", parse_a->word);
						if (funct_call->target[funct_call->target_count].param[i].strobe == 0) {
							strcat_p2(parse_b, parse_a->word);
							while (parse_a->line[funct_call->start_index].line[ptr] != ',' && parse_a->line[funct_call->start_index].line[ptr] != ')')
								parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[funct_call->start_index].line[ptr++];
							parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[funct_call->start_index].line[ptr++];
							parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
						}
						else {
							while (parse_a->line[funct_call->start_index].line[ptr] != ',' && parse_a->line[funct_call->start_index].line[ptr] != ')')ptr++;
							if (parse_a->line[funct_call->start_index].line[ptr] == ')') {
								parse_b->line[parse_b->index].line[parse_b->ptr - 1] = ')';
							}
							ptr++;
						}
					}
					while (parse_a->line[funct_call->start_index].line[ptr] != '\0')
						parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[funct_call->start_index].line[ptr++];
					parse_b->line[parse_b->index++].line[parse_b->ptr] = '\0';
					// end function call header

					// place initial values
					for (UINT i = 0; i < funct_call->param_count; i++) {
						if (funct_call->param[i].init) {
							char word[0x80];
							sprintf_s(word, "\t%s %s = %s;\n", funct_call->param[i].type, funct_call->param[i].name, funct_call->target[funct_call->target_count].param[i].alias);
							strcpy_p2(parse_b, word);
							parse_b->index++;
						}
					}
					// end initial values
					debug++;
					// replace constants with actual value;
					for (UINT index = funct_call->start_index + 1; index <= funct_call->stop_index; index++) {
						ptr = 0;
						parse_b->ptr = 0;
						while (parse_a->line[index].line[ptr] != '\0') {
							getWord(parse_a, index, &ptr);
							if (parse_a->word[0]=='\0') {
								parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[index].line[ptr++];
								parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
							}
							else {
								UINT8 hit = 0;
								for (UINT i = 0; i < entry_ptr && !hit; i++) {
									if (strcmp(parse_a->word, funct_call->param[i].name) == 0) {
										hit = 1;
										if (funct_call->target[funct_call->target_count].param[i].strobe && funct_call->param[i].init == 0) {
											strcat_p2(parse_b, funct_call->target[funct_call->target_count].param[i].alias);
										}
										else {
											strcat_p2(parse_b, parse_a->word);
										}
									}
								}
								if (!hit) {
									strcat_p2(parse_b, parse_a->word);
								}
								parse_b->ptr = strlen(parse_b->line[parse_b->index].line);
							}
						}
						parse_b->index++;
					}
					funct_call->target_count++;
					debug++;
				}
			}
		}
	}
	return hit;
}
void FunctCall_ConstEllim(const char* dst_name, const char* src_name, INT flags, var_type_struct* var_type, parse_struct2_set* parse, int pass_count) {
	int debug = 0;
	FILE* src;
	fopen_s(&src, src_name, "r");

	UINT  line_count = 0;
	while (fgets(parse->a.line[line_count++].line, 0x100, src) != NULL) {}
	line_count--;
	fclose(src);
	funct_call_set_type* funct_set = (funct_call_set_type*)malloc(sizeof(funct_call_set_type));
	funct_set->count = 0;
	parse_struct2* parse_a = &parse->a;
	parse_struct2* parse_b = &parse->b;
	for (parse_a->index = 0, parse_b->index = 0; parse_a->index < line_count; parse_a->index++) {
		parse_a->ptr = 0;
		parse_b->ptr = 0;
		while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
		if (parse_a->line[parse_a->index].line[parse_a->ptr] == '#') {
			strcpy_p2(parse_b, parse_a);
			parse_b->index++;
		}
		else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '/' && parse_a->line[parse_a->index].line[parse_a->ptr + 1] == '/') {
			strcpy_p2(parse_b, parse_a);
			parse_b->index++;
		}
		else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '\n' && parse_a->line[parse_a->index].line[parse_a->ptr + 1] == '\0') {
			strcpy_p2(parse_b, parse_a);
			parse_b->index++;
		}
		else if (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0') {
			getWord(parse_a);
			if (strcmp(parse_a->word, "using") == 0) {
				strcpy_p2(parse_b, parse_a);
				parse_b->index++;
			}
			else if ((strcmp(parse_a->word, "void") == 0) || (strcmp(parse_a->word, "INT8") == 0) || (strcmp(parse_a->word, "UINT8") == 0) || (strcmp(parse_a->word, "_fp16") == 0)) {
				sprintf_s(funct_set->funct[funct_set->count].type, "%s", parse_a->word);
				funct_set->funct[funct_set->count].target_count = 0;
				while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
				getWord(parse_a);
				if (parse_a->line[parse_a->index].line[parse_a->ptr] == '(') {
					markup_funct_call(&funct_set->funct[funct_set->count], parse_a);
					funct_set->funct[funct_set->count].hit = 0;
					funct_set->funct[funct_set->count].modified = 0;
					if (!search4thread_call(parse_b, &funct_set->funct[funct_set->count], parse_a, line_count)) {
						for (UINT index = funct_set->funct[funct_set->count].start_index; index <= funct_set->funct[funct_set->count].stop_index; index++) {
							parse_a->ptr = 0;
							parse_b->ptr = 0;
							parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
							if (index == 0x251)
								debug++;
							while (parse_a->line[index].line[parse_a->ptr] == ' ' || parse_a->line[index].line[parse_a->ptr] == '\t')
								parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[index].line[parse_a->ptr++];
							while (parse_a->line[index].line[parse_a->ptr] != '\0') {
								getWord(parse_a, index, &parse_a->ptr);
								if (parse_a->word[0] == '\0') {
									parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[index].line[parse_a->ptr++];
									parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
								}
								else {
									for (UINT i = 0; i < funct_set->count; i++) {
										if (strcmp(funct_set->funct[i].name, parse_a->word) == 0) {
											parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
											strcat_p2(parse_b, parse_a->word);
											UINT ptr = parse_a->ptr;
											if (parse_a->line[index].line[ptr] == '(')
												ptr++;
											else
												debug++;
											while (parse_a->line[index].line[ptr] != ')') {
												getWord(parse_a, index, &ptr);
												INT64 number;
												parse_b->line[parse_b->index].line[parse_b->ptr++] = '_';
												parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
												if (get_integer(&number, parse_a->word)) {
													strcat_p2(parse_b, parse_a->word);
													if (parse_a->line[index].line[ptr] != ')') {
														if (parse_a->line[index].line[ptr] == ' ')
															ptr++;
														if (parse_a->line[index].line[ptr] == ',')
															ptr++;
														else
															debug++;
														if (parse_a->line[index].line[ptr] == ' ')
															ptr++;
														else
															debug++;
													}
												}
												else {
													parse_b->line[parse_b->index].line[parse_b->ptr++] = 'x';
													parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
													while (parse_a->line[index].line[ptr] != ')' && parse_a->line[index].line[ptr] != ',')ptr++;
													if (parse_a->line[index].line[ptr] != ')') {
														if (parse_a->line[index].line[ptr] == ',')
															ptr++;
														else
															debug++;
														if (parse_a->line[index].line[ptr] == ' ')
															ptr++;
														else
															debug++;
													}
												}
											}
											parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[index].line[parse_a->ptr++];
											parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
											while (parse_a->line[index].line[parse_a->ptr] != ')') {
												getWord(parse_a, index, &parse_a->ptr);
												INT64 number;
												if (get_integer(&number, parse_a->word)) {
													if (parse_a->line[index].line[parse_a->ptr] == ' ') {
														parse_a->ptr++;
													}
													if (parse_a->line[index].line[parse_a->ptr] != ')') {
														if (parse_a->line[index].line[parse_a->ptr] == ',') {
															parse_a->ptr++;
														}
														else
															debug++;
														if (parse_a->line[index].line[parse_a->ptr] == ' ') {
															parse_a->ptr++;
														}
														else
															debug++;
													}
													else {
														debug++;
														parse_b->ptr -= 2;
														parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
													}
												}
												else {
													strcat_p2(parse_b, parse_a->word);
													while (parse_a->line[index].line[parse_a->ptr] != ')' && parse_a->line[index].line[parse_a->ptr] != ',') {
														parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[index].line[parse_a->ptr++];
														parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
													}
													if (parse_a->line[index].line[parse_a->ptr] != ')') {
														if (parse_a->line[index].line[parse_a->ptr] == ',') {
															parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[index].line[parse_a->ptr++];
															parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
														}
														else
															debug++;
														if (parse_a->line[index].line[parse_a->ptr] == ' ') {
															parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[index].line[parse_a->ptr++];
															parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
														}
														else
															debug++;
													}
												}
											}
											while (parse_a->line[index].line[parse_a->ptr] != '\0')
												parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[index].line[parse_a->ptr++];
											parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
											debug++;
										}
									}
									parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
									if (parse_a->line[index].line[parse_a->ptr] != '\0') {
										strcat_p2(parse_b, parse_a->word);
									}
								}
							}
							parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
							parse_b->index++;
						}
					}
					if (funct_set->funct[funct_set->count].hit > funct_set->funct[funct_set->count].modified) {
						for (UINT index = funct_set->funct[funct_set->count].start_index; index <= funct_set->funct[funct_set->count].stop_index; index++) {
							strcpy_p2(parse_b, parse_a->line[index].line);
							parse_b->index++;
						}
					}
					parse_a->index = funct_set->funct[funct_set->count].stop_index;
					if (funct_set->funct[funct_set->count].hit)
						funct_set->count++;
				}
				else {
					strcpy_p2(parse_b, parse_a);
					parse_b->index++;
				}
			}
			else {
				debug++;
				strcpy_p2(parse_b, parse_a);
				parse_b->index++;
			}
		}
	}
	free(funct_set);
	FILE* dest;
	fopen_s(&dest, dst_name, "w");
	for (UINT line_number = 0; line_number < parse_b->index; line_number++) {
		fprintf(dest, "%s", parse_b->line[line_number].line);
	}
	fclose(dest);
}