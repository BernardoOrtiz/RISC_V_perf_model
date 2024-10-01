// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>
struct array_entries {
	char type[0x10];
	char name[0x80];
	char depth;
};
struct array_list_struct {
	array_entries entry[0x10];
	char count;
};

// candidate only accepts on :
// * valid loop index
// * index addressing range < 0x800 Bytes (12b signed offset)
struct candidate_type2 {
	char type[0x80];
	char surname[0x80];
	parse_struct2 base;
	int range_high;
	int range_low;
	int range_inc;

	int size;
	int size_shift;
	char in_out; // 0=unknown, 1=in only, 2=out only, 3= budirectional
	char op[4];
	int depth; // visibility
	char alias[0x80];
	char use_alias;
	int offset[0x10];
	int offset_count;
	int offset_used;
	int step;
};
struct candidate_set_type2 {
	candidate_type2 entry[0x10];
	UINT8 count;
};

void add_candidate3(candidate_set_type2* candidate, int range_low, int range_high, int skip) {
	candidate->entry[candidate->count].range_low = range_low;
	candidate->entry[candidate->count].range_high = range_high;
	candidate->entry[candidate->count].offset_used = 0;
	candidate->entry[candidate->count].offset_count = 0;
	if (!skip) {
		UINT priors_exist = 0;
		for (UINT i = 0; i < candidate->count && !priors_exist; i++) {
			if (strcmp(candidate->entry[i].base.line[0].line, candidate->entry[candidate->count].base.line[0].line) == 0 &&
				((candidate->entry[candidate->count].range_low - candidate->entry[i].range_low) < 0x800) && ((candidate->entry[candidate->count].range_low - candidate->entry[i].range_low) >= -0x800)) {
				priors_exist = 1;
				if (candidate->entry[candidate->count].range_low < candidate->entry[i].range_low) {
					candidate->entry[i].range_low = candidate->entry[candidate->count].range_low;
					for (UINT j = 0; j < candidate->entry[candidate->count].offset_count; j++)
						candidate->entry[i].offset[j] += candidate->entry[i].range_low - candidate->entry[candidate->count].range_low;
					candidate->entry[candidate->count].offset[candidate->entry[candidate->count].offset_count] = 0;
				}
				else {
					candidate->entry[candidate->count].offset[candidate->entry[candidate->count].offset_count] = candidate->entry[candidate->count].range_low - candidate->entry[i].range_low;
				}
				candidate->entry[candidate->count].offset_count++;

				if (candidate->entry[candidate->count].range_high > candidate->entry[i].range_high)
					candidate->entry[i].range_high = candidate->entry[candidate->count].range_high;
			}
		}
		// need to check for repeats such as "vector[i+0] + vector[i+1]" where increment = 1, range > 1;
		// entry[].offset[0x10]; where >= 9 is aborted, use vector form only no variable substitution
		if (!priors_exist) {// culls duplicate ranges
			char word[0x80];
			sprintf_s(word, "%s_ptr%d", candidate->entry[candidate->count].surname, candidate->count);
			sprintf_s(candidate->entry[candidate->count].alias, "%s", word);
			candidate->entry[candidate->count].offset[0] = 0;
			candidate->entry[candidate->count].offset_count = 1;
			//			if (candidate->entry[candidate->count].range_high - candidate->entry[candidate->count].range_low > 0x800)
			candidate->count++;
		}
	}
}
void parse_candidate3(candidate_set_type2* candidate, parse_struct2* parse_temp, loop_control_type* l_control, array_list_struct* array_list, char in_out) {
	UINT debug = 0;
	sprintf_s(candidate->entry[candidate->count].surname, "%s", parse_temp->word);
	//	sprintf_s(candidate->entry[candidate->count].base.line[0].line, "%s[", parse_temp->word);
	if (candidate->count == 6)
		debug++;
	parse_struct2* base = &candidate->entry[candidate->count].base;
	base->index = 0;
	//	candidate->entry[candidate->count].base.ptr = 0;
	strcpy_p2(base, parse_temp->word);
	base->line[0].line[base->ptr++] = '[';
	base->line[0].line[base->ptr] = '\0';

	candidate->entry[candidate->count].offset_used = 0;
	candidate->entry[candidate->count].offset_count = 0;

	candidate->entry[candidate->count].depth = l_control->depth_out;
	candidate->entry[candidate->count].in_out = in_out;
	candidate->entry[candidate->count].size = 0;
	parse_temp->ptr++;

	char hit = 0;
	for (UINT i = 0; i < array_list->count && !hit; i++) {
		if (strcmp(array_list->entry[i].name, parse_temp->word) == 0) {
			hit = 1;
			sprintf_s(candidate->entry[candidate->count].type, "%s", array_list->entry[i].type);
		}
	}
	if (parse_temp->index >= 390)
		debug++;
	int stop = 0;
	int depth2 = 0;
	char force_candidate = 0;

	while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] != ']' && !stop || depth2 > 0) {
		getWord(parse_temp);
		if (parse_temp->word_ptr != 0) {
			if (l_control->depth_out > 0) {
				if (l_control->index[l_control->depth_out - 1].name[0] != '\0') {
					if (strcmp(parse_temp->word, l_control->index[l_control->depth_out - 1].name) == 0) {
						force_candidate = 1;
						if (candidate->count == 6)
							debug++;
						strcat_p2(&candidate->entry[candidate->count].base, parse_temp->word);
						while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ')') {
							parse_temp->ptr++;
							strcat_p2(&candidate->entry[candidate->count].base, (char*)")");
						}
						if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '<' && parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '<') {
							parse_temp->ptr += 2;
							strcat_p2(&candidate->entry[candidate->count].base, (char*)"<<");
							while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ') {
								parse_temp->ptr++;
								strcat_p2(&candidate->entry[candidate->count].base, (char*)" ");
							}
							getWord(parse_temp);
							INT64 number;
							if (get_integer(&number, parse_temp->word)) {
								candidate->entry[candidate->count].size = l_control->index[l_control->depth_out - 1].limit << number;
								candidate->entry[candidate->count].size_shift = number;
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
			if (strcmp(parse_temp->word, l_control->index[l_control->depth_out].name) == 0) {
				while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t') parse_temp->ptr++;
				candidate->entry[candidate->count].range_low = l_control->index[l_control->depth_out].initial;
				candidate->entry[candidate->count].range_high = l_control->index[l_control->depth_out].limit - l_control->index[l_control->depth_out].increment;
				candidate->entry[candidate->count].range_inc = l_control->index[l_control->depth_out].increment;
				candidate->entry[candidate->count].in_out |= in_out;
				char latch = '-';
				switch (parse_temp->line[parse_temp->index].line[parse_temp->ptr]) {
				case ')':
					if (parse_temp->line[parse_temp->index].line[parse_temp->ptr + 1] == ']') {
						candidate->entry[candidate->count].op[0] = '\0';
						if (candidate->entry[candidate->count].range_high - candidate->entry[candidate->count].range_low > 0x800 || force_candidate)
							add_candidate3(candidate, candidate->entry[candidate->count].range_low, candidate->entry[candidate->count].range_high, 0);
					}
					strcat_p2(&candidate->entry[candidate->count].base, (char*)")");
					sprintf_s(candidate->entry[candidate->count].op, ")");
					parse_temp->ptr++;
					break;
				case ']': {
					candidate->entry[candidate->count].op[0] = '\0';
					if (candidate->entry[candidate->count].range_high - candidate->entry[candidate->count].range_low > 0x80 || force_candidate)
						add_candidate3(candidate, candidate->entry[candidate->count].range_low, candidate->entry[candidate->count].range_high, 0);
				}
						break;
				case '<':
					candidate->entry[candidate->count].op[0] = '\0';
					if (parse_temp->line[parse_temp->index].line[parse_temp->ptr + 1] == '<') {
						parse_temp->ptr += 2;
						while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t') parse_temp->ptr++;
						getWord(parse_temp);
						INT64 number;
						if (get_integer(&number, parse_temp->word)) {
							int range_low = l_control->index[l_control->depth_out].initial << number;
							int range_high = (l_control->index[l_control->depth_out].limit - l_control->index[l_control->depth_out].increment) << number;
							UINT skip = 0;
							if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ')') {// hack: likely source of errors
								UINT ptr = strlen(candidate->entry[candidate->count].base.line[0].line);
								candidate->entry[candidate->count].base.line[0].line[ptr] = parse_temp->line[parse_temp->index].line[parse_temp->ptr++];
								candidate->entry[candidate->count].base.line[0].line[ptr] = '\0';
							}
							if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '+') {
								sprintf_s(candidate->entry[candidate->count].op, "+");
								parse_temp->ptr++;
								while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t') parse_temp->ptr++;
								getWord(parse_temp);
								if (get_integer(&number, parse_temp->word)) {
									range_low += number;
									range_high += number;
								}
								else {
									debug++;
									skip = 1;
								}
							}
							else if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '|') {
								sprintf_s(candidate->entry[candidate->count].op, "|");
								parse_temp->ptr++;
								while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t') parse_temp->ptr++;
								getWord(parse_temp);
								if (get_integer(&number, parse_temp->word)) {
									range_low |= number;
									range_high |= number;
								}
								else {
									debug++;
									skip = 1;
								}

							}
							if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] != ']') {
								debug++;
								skip = 1;
							}
							if (number > 0x0800 || (range_high - range_low) > 0x0800 || force_candidate)
								add_candidate3(candidate, range_low, range_high, skip);
						}
						else {
							debug++;
						}
					}
					else {// conditional statement, abort candidate search
						stop = 1;
					}
					break;
				case '+':
					latch = '+';
				case '-': {
					candidate->entry[candidate->count].op[0] = parse_temp->line[parse_temp->index].line[parse_temp->ptr];
					candidate->entry[candidate->count].op[1] = '\0';
					parse_temp->ptr++;
					//					skip_white_spaces(candidate, parse_temp);
					while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t') parse_temp->ptr++;
					getWord(parse_temp);
					INT64 number;
					if (get_integer(&number, parse_temp->word)) {
						if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ']') {
							UINT hit = 0;
							for (UINT i = 0; i < candidate->count && !hit; i++) {
								if (strcmp(candidate->entry[i].base.line[0].line, candidate->entry[candidate->count].base.line[0].line) == 0) {
									if (parse_temp->index == 0x2a)
										debug++;
									if (l_control->index[l_control->depth_out].initial + number >= candidate->entry[i].range_low) {
										if (l_control->index[l_control->depth_out].initial + number - candidate->entry[i].range_low < 0x1000 &&
											l_control->index[l_control->depth_out].limit + number - candidate->entry[i].range_high < 0x1000) {
											hit = 1;
											UINT duplicate = 0;
											for (UINT j = 0; j < candidate->entry[i].offset_count; j++) {
												if (candidate->entry[i].offset[j] == (l_control->index[l_control->depth_out].initial + number - candidate->entry[i].range_low))
													duplicate = 1;
											}
											if (!duplicate)
												candidate->entry[i].offset[candidate->entry[i].offset_count++] = l_control->index[l_control->depth_out].initial + number - candidate->entry[i].range_low;
										}
									}
									else {
										if (candidate->entry[i].range_low - (l_control->index[l_control->depth_out].initial + number) < 0x1000 &&
											candidate->entry[i].range_high - (l_control->index[l_control->depth_out].limit + number) < 0x1000) {
											hit = 1;
											UINT duplicate = 0;
											for (UINT j = 0; j < candidate->entry[i].offset_count; j++) {
												if (candidate->entry[i].offset[j] == (candidate->entry[i].range_low - (l_control->index[l_control->depth_out].initial + number)))
													duplicate = 1;
											}
											if (!duplicate) {
												for (UINT j = 0; j < candidate->entry[i].offset_count; j++)
													candidate->entry[i].offset[j] += (candidate->entry[i].range_low - (l_control->index[l_control->depth_out].initial + number));
												candidate->entry[i].offset[candidate->entry[i].offset_count++] = candidate->entry[i].range_low - (l_control->index[l_control->depth_out].initial + number);
												candidate->entry[i].range_low = l_control->index[l_control->depth_out].initial + number;
											}
										}
									}
								}
							}
							if (!hit) {
								if (latch == '+') {
									candidate->entry[candidate->count].range_low = l_control->index[l_control->depth_out].initial + number;
									candidate->entry[candidate->count].range_high = l_control->index[l_control->depth_out].limit + number;
								}
								else {
									candidate->entry[candidate->count].range_low = l_control->index[l_control->depth_out].initial - number;
									candidate->entry[candidate->count].range_high = l_control->index[l_control->depth_out].limit - number;
								}
								candidate->entry[candidate->count].offset[0] = 0;
								candidate->entry[candidate->count].offset_count = 1;
								char word[0x80];
								sprintf_s(word, "%s_ptr%d", candidate->entry[candidate->count].surname, candidate->count);
								sprintf_s(candidate->entry[candidate->count].alias, "%s", word);
								if (candidate->entry[candidate->count].range_high - candidate->entry[candidate->count].range_low > 0x800 || force_candidate)
									candidate->count++;
								candidate->entry[candidate->count].size = 0;
								candidate->entry[candidate->count].in_out = 0;
								if (candidate->count > 8)
									candidate->count = 9;
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
						break;
				case '|': {
					candidate->entry[candidate->count].op[0] = parse_temp->line[parse_temp->index].line[parse_temp->ptr];
					candidate->entry[candidate->count].op[1] = '\0';
					parse_temp->ptr++;
					//
					// error, need to validate that both sides of the 'or' affect different bits
					//
					while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t') parse_temp->ptr++;
					getWord(parse_temp);
					INT64 number;
					if (get_integer(&number, parse_temp->word)) {
						if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ')')parse_temp->ptr++;
						if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ']') {
							UINT hit = 0;
							for (UINT i = 0; i < candidate->count && !hit; i++) {
								if (strcmp(candidate->entry[i].base.line[0].line, candidate->entry[candidate->count].base.line[0].line) == 0)
									hit = 1;
							}
							if (!hit) {
								candidate->entry[candidate->count].offset[0] = 0;
								candidate->entry[candidate->count].offset_count = 1;
								if (candidate->entry[candidate->count].range_high - candidate->entry[candidate->count].range_low > 0x800 || force_candidate)
									candidate->count++;
								if (candidate->count > 8)
									candidate->count = 9;
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
						break;
				default:
					debug++;
					break;
				}
			}
			else {
				if (candidate->count == 6)
					debug++;
				strcat_p2(&candidate->entry[candidate->count].base, parse_temp->word);
			}
		}
		else {
			if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '[')
				depth2++;
			else if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ']')
				depth2--;
			copy_char(&candidate->entry[candidate->count].base, parse_temp);
		}
	}
	parse_temp->ptr++;
}
void nonaligned_load128_preload(const char* dst_name, const char* src_name, var_type_struct* var_type, parse_struct2_set* parse, compiler_var_type* compiler_var) {
	int debug = 0;
	FILE* src;
	fopen_s(&src, src_name, "r");
	UINT  line_count = 0;

	loop_control_type l_control;
	l_control.depth_test = 0;
	l_control.depth_out = 0;
	for (UINT i = 0; i < 0x10; i++) 	l_control.index[i].name[0] = '\0';

	while (fgets(parse->a.line[line_count++].line, 0x200, src) != NULL) {}
	line_count--;

	fclose(src);
	parse_struct2* parse_in = &parse->a;
	parse_struct2* parse_out = &parse->b;

	candidate_set_type2 candidate;
	candidate.count = 0;
	for (UINT i = 0; i < 0x10; i++) {
		candidate.entry[i].base.line = (line_struct*)malloc(sizeof(line_struct));
		candidate.entry[i].size = 0;
		candidate.entry[i].in_out = 0;
	}

	array_list_struct array_list;
	array_list.count = 0;
	parse_struct2* parse_temp = (parse_struct2*)malloc(sizeof(parse_struct2));
	UINT funct_code = 0;
	for (parse_in->index = 0, parse_out->index = 0; parse_in->index < line_count; parse_in->index++, parse_out->index++) {
		parse_in->ptr = 0;
		parse_out->ptr = 0;
		parse_out->line[parse_out->index].line[0] = '\0';
		if (parse_in->index >= 684)
			debug++;
		if (parse_in->index >= 0x16d)
			debug++;
		if (l_control.depth_out >= 0x10)
			debug++;
		if (candidate.count >= 0x10)
			debug++;
		if (funct_code) {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
				if (parse_in->ptr > strlen(parse_in->line[parse_in->index].line))
					debug++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {// detect comment - skip
					while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')		copy_char(parse_out, parse_in);
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\"') {// detect string - skip
					copy_char(parse_out, parse_in);
					while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\"' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')	copy_char(parse_out, parse_in);
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
					l_control.index[l_control.depth_out].addr = parse_in->index;
					l_control.index[l_control.depth_out].out_addr = parse_out->index;
					l_control.depth_out++;
					copy_char(parse_out, parse_in);
					l_control.index[l_control.depth_out].name[0] = '\0';
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '}') {
					l_control.depth_out--;
					copy_char(parse_out, parse_in);
					if (candidate.count > 0)
						debug++;
					if (l_control.depth_out <= 0)
						funct_code = 0;
					if (array_list.count > 0) {
						while (array_list.count > 0 && array_list.entry[array_list.count - 1].depth > l_control.depth_out) {
							array_list.count--;
						}
					}
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\n') {
					copy_char(parse_out, parse_in);
				}
				else {
					getWord(parse_in);// for loop detect
					if (parse_in->word[0] != '\0') {// need to chack for "for", "if", or a function call "func("
						char match_index;
						if (VariableTypeMatch(&match_index, parse_in->word, var_type)) {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
							getWord(parse_in);
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
								sprintf_s(array_list.entry[array_list.count].type, "%s", var_type[match_index].name);
								sprintf_s(array_list.entry[array_list.count].name, "%s", parse_in->word);
								array_list.entry[array_list.count].depth = l_control.depth_out;
								array_list.count++;
								if (array_list.count >= 0x10)
									debug++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] != ']' && parse_in->line[parse_in->index].line[parse_in->ptr] == '\0')	parse_in->ptr++;
								parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
							}
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ',') {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
								getWord(parse_in);
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
									sprintf_s(array_list.entry[array_list.count].type, "%s", var_type[match_index].name);
									sprintf_s(array_list.entry[array_list.count].name, "%s", parse_in->word);
									array_list.entry[array_list.count].depth = l_control.depth_out;
									array_list.count++;
									if (array_list.count >= 0x10)
										debug++;
									while (parse_in->line[parse_in->index].line[parse_in->ptr] != ']' && parse_in->line[parse_in->index].line[parse_in->ptr] == '\0')	parse_in->ptr++;
									parse_in->ptr++;
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
								}

							}
							if (array_list.count >= 0x10)
								debug++;
							parse_out->ptr = 0;
							strcat_p2(parse_out, parse_in);
						}
						else if (strcmp("for", parse_in->word) == 0) {
							if (parse_in->index >= 562)
								debug++;
							parse_temp->index = parse_in->index;
							parse_temp->line = parse_in->line;
							decode_for(&l_control, parse_in, parse_temp->index);
							UINT depth_check = l_control.depth_out;
							for (parse_temp->index++; parse_temp->index < line_count && l_control.depth_out >= depth_check; parse_temp->index++) {
								if (parse_temp->index >= 47)
									debug++;
								if (parse_temp->index >= 0x194)
									debug++;
								parse_temp->ptr = 0;
								while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t')	parse_temp->ptr++;
								while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] != '\0') {
									if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '/' && parse_temp->line[parse_temp->index].line[parse_temp->ptr + 1] == '/') {// detect comment - skip
										while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] != '\0')		parse_temp->ptr++;
									}
									else if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\"') {// detect string - skip
										parse_temp->ptr++;
										while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] != '\"' && parse_temp->line[parse_temp->index].line[parse_temp->ptr] != '\0')	parse_temp->ptr++;
									}
									else if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t') {
										parse_temp->ptr++;
										while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t')	parse_temp->ptr++;
									}
									else if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '{') {
										l_control.index[l_control.depth_out].addr = parse_temp->index;
										l_control.index[l_control.depth_out].out_addr = parse_out->index;
										l_control.depth_out++;
										parse_temp->ptr++;
										l_control.index[l_control.depth_out].name[0] = '\0';
									}
									else if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '}') {
										l_control.depth_out--;
										parse_temp->ptr++;
										while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] != '\0') parse_temp->ptr++;
									}
									else if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\n' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' ||
										parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '*') {
										parse_temp->ptr++;
									}
									else if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '(') {
										parse_temp->ptr++;
										UINT hold = parse_temp->ptr;
										getWord(parse_temp);
										if (strcmp(parse_temp->word, "_fp16") == 0) {
											if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ')') {
												parse_temp->ptr++;
											}
											else {
												debug++;
											}
										}
										else {
											parse_temp->ptr = hold;
										}
									}
									else {
										getWord(parse_temp);// for loop detect
										if (parse_temp->word[0] != '\0') {// need to chack for "for", "if", or a function call "func("
											if (VariableTypeMatch(&match_index, parse_temp->word, var_type)) {
												while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t')	parse_temp->ptr++;
												getWord(parse_temp);
												while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t')	parse_temp->ptr++;
												if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '[') {
													sprintf_s(array_list.entry[array_list.count].type, "%s", var_type[match_index].name);
													sprintf_s(array_list.entry[array_list.count].name, "%s", parse_temp->word);
													array_list.entry[array_list.count].depth = l_control.depth_out;
													array_list.count++;
													if (array_list.count >= 0x10)
														debug++;
													while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] != ']' && parse_temp->line[parse_temp->index].line[parse_temp->ptr] != '\0')	parse_temp->ptr++;
													parse_temp->ptr++;
													while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t')	parse_temp->ptr++;
												}
												while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ',') {
													parse_temp->ptr++;
													while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t')	parse_temp->ptr++;
													getWord(parse_temp);
													while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t')	parse_temp->ptr++;
													if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '[') {
														sprintf_s(array_list.entry[array_list.count].type, "%s", var_type[match_index].name);
														sprintf_s(array_list.entry[array_list.count].name, "%s", parse_temp->word);
														array_list.entry[array_list.count].depth = l_control.depth_out;
														array_list.count++;
														if (array_list.count >= 0x10)
															debug++;
														while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] != ']' && parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\0')	parse_temp->ptr++;
														parse_temp->ptr++;
														while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t')	parse_temp->ptr++;
													}

												}
												if (array_list.count >= 0x10)
													debug++;
												while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t')	parse_temp->ptr++;
												if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '=')
													parse_temp->ptr++;
												if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ';')parse_temp->ptr++;
											}
											else if (strcmp("for", parse_temp->word) == 0) {
												if (parse_temp->index >= 565)
													debug++;
												for (; parse_in->index < parse_temp->index; ) {
													strcpy_p2(parse_out, parse_in);
													parse_in->index++;
													parse_out->index++;
													parse_out->ptr = 0;
												}
												parse_in->ptr = parse_temp->ptr;
												strcpy_word(parse_in->word, parse_temp);
												decode_for(&l_control, parse_in, parse_temp->index);
												depth_check = l_control.depth_out;
												parse_out->ptr = 0;
												strcat_p2(parse_out, parse_in);
												parse_temp->index++;
												parse_temp->ptr = 0;
												while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t')	parse_temp->ptr++;
												if (parse_temp->index >= 47)
													debug++;
											}
											else {
												if (parse_temp->index >= 104)
													debug++;
												if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '[') {
													parse_candidate3(&candidate, parse_temp, &l_control, &array_list, 2);
													for (VariableListEntry* current = compiler_var->global_list_base->next; current != compiler_var->global_list_base; current = current->next) {
														if (strcmp(current->name, candidate.entry[candidate.count - 1].surname) == 0) {
															switch (current->type) {
															case int8_enum:
																sprintf_s(candidate.entry[candidate.count - 1].type, "int8");
																break;
															case uint8_enum:
																sprintf_s(candidate.entry[candidate.count - 1].type, "uint8");
																break;
															case int64_enum:
																sprintf_s(candidate.entry[candidate.count - 1].type, "int64");
																break;
															case uint64_enum:
																sprintf_s(candidate.entry[candidate.count - 1].type, "uint64");
																break;
															default:
																debug++;
																break;
															}
														}
													}
												}
												else {
												}
												while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] != '\0') {
													if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '/' && parse_temp->line[parse_temp->index].line[parse_temp->ptr + 1] == '/') {// detect comment - skip
														while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] != '\0')		parse_temp->ptr++;
													}
													else if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\"') {// detect string - skip
														parse_temp->ptr++;
														while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] != '\"' && parse_temp->line[parse_temp->index].line[parse_temp->ptr] != '\0')	parse_temp->ptr++;
													}
													else if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '{') {
														l_control.index[l_control.depth_out].addr = parse_temp->index;
														l_control.index[l_control.depth_out].out_addr = parse_out->index;
														l_control.depth_out++;
														parse_temp->ptr++;
														l_control.index[l_control.depth_out].name[0] = '\0';
													}
													else if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '}') {
														l_control.depth_out--;
														parse_temp->ptr++;
													}
													else if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\n' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '[' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ']' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '(' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ')' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '?' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ':' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ';' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ',' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '=' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '*' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '-' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '+' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '&' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '|' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '^' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '<' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '>' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' ||
														parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t') {
														parse_temp->ptr++;
													}
													else {
														getWord(parse_temp);
														if (parse_temp->word[0] != '\0') {
															if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '[') {
																parse_candidate3(&candidate, parse_temp, &l_control, &array_list, 1);
															}
															else {
															}
														}
														else {
															debug++;
														}
													}
												}
												debug++;
											}
										}
										else {
											debug++;
										}
									}
								}
							}
							if (parse_temp->index >= 0x60)
								debug++;
							if (candidate.count > 0 && candidate.count < 9) {
								// check 
								char skip = 0;
								char in_count = 0;
								if (l_control.index[l_control.depth_out].addr == l_control.index[l_control.depth_out + 1].addr - 1) {
									parse_temp->ptr = 0;
									while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == ' ' || parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\t')parse_temp->ptr++;
									if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '}') {
										for (UINT i = 0; i < candidate.count; i++) {
											if (candidate.entry[i].in_out == 1) {
												in_count++;
												if (strcmp(candidate.entry[i].type, "_fp16") == 0) {
													if (candidate.entry[i].size >= 0x800)
														skip = 1;
												}
												else {
													debug++;
												}
											}
										}
									}
								}
								if (in_count == 0)
									skip = 1;
								if (skip == 0) {
									debug++;
									char word[0x100];
									sprintf_s(word, "// inline\n%s", parse_out->line[l_control.index[0].out_addr].line);
									sprintf_s(parse_out->line[l_control.index[0].out_addr].line, "%s", word);
									sprintf_s(parse_out->line[parse_out->index - 1].line, "//%s", parse_in->line[l_control.index[l_control.depth_out].addr].line);
								}
								sprintf_s(parse_out->line[parse_out->index++].line, "//%s", parse_in->line[parse_in->index++].line);
								parse_out->line[parse_out->index].line[0] = '\0';
								parse_out->ptr = 0;

								INT depth_out = l_control.depth_out + 1;

								char loop_tab[0x80];
								UINT i = 0;
								for (; i < depth_out; i++) loop_tab[i] = '\t';
								loop_tab[i] = '\0';

								//	strcpy_p2(parse_out, loop_tab);								
								sprintf_s(parse_out->line[parse_out->index++].line, "%s{	// %d \n", loop_tab, depth_out);

								char word[0x80];
								int use_offsets = 0;
								if (skip == 0) {
									for (UINT i = 0; i < candidate.count; i++) {
										sprintf_s(parse_out->line[parse_out->index].line, "%s%s *%s = &",
											loop_tab, candidate.entry[i].type, candidate.entry[i].alias);
										//	strcpy_p2(parse_out, loop_tab);
										//	strcat_p2(parse_out, candidate.entry[i].type);
										//	strcat_p2(parse_out, (char *)" *");
										//	strcat_p2(parse_out, candidate.entry[i].alias);
										//	strcat_p2(parse_out, (char *)" = &");
										parse_struct2* base = &candidate.entry[i].base;
										base->ptr = 0;
										base->index = 0;
										while (base->line[0].line[base->ptr] != '[')
											copy_char(parse_out, base);
										copy_char(parse_out, base);

										getWord(base);
										int p_count = 0;
										while (strcmp(base->word, l_control.index[l_control.depth_out].name)) {
											if (base->word[0] != '\0') {
												strcat_p2(parse_out, base->word);
											}
											else {
												if (base->line[0].line[base->ptr] == '(')
													p_count++;
												else if (base->line[0].line[base->ptr] == ')')
													p_count--;
												copy_char(parse_out, base);
											}
											getWord(base);
										}
										strcat_p2(parse_out, (char*)"0");
										while (candidate.entry[i].base.line[0].line[candidate.entry[i].base.ptr] != '\0') {
											if (base->line[0].line[base->ptr] == '(')
												p_count++;
											else if (base->line[0].line[base->ptr] == ')')
												p_count--;
											parse_out->line[parse_out->index].line[parse_out->ptr++] = base->line[0].line[base->ptr++];
										}
										parse_out->line[parse_out->index].line[parse_out->ptr++] = base->line[0].line[base->ptr++];
										//		if (p_count == 1)
										//			sprintf_s(word, "0x%03x)];\n", candidate.entry[i].range_low);
										//		else
										sprintf_s(word, "0x%03x];\n", candidate.entry[i].range_low);
										strcat_p2(parse_out, word);
										parse_out->index++;
										parse_out->ptr = 0;
										//										}
									}
								}
								else {
									for (UINT i = 0; i < candidate.count; i++) {
										if (i == 6)
											debug++;
										candidate.entry[i].use_alias = 0;
										if ((candidate.entry[i].range_high >= 0x800) || (candidate.entry[i].range_high < -0x800)) {
											parse_out->ptr = 0;
											candidate.entry[i].use_alias = 1;

											for (UINT j = 0; j < candidate.entry[i].offset_count && candidate.entry[i].offset_count>1; j++) {
												if (candidate.entry[i].offset[j] > 0x7ff) {
													use_offsets = 1;
												}
											}
											if (use_offsets) {
												candidate.entry[i].offset_used = 0;
												candidate.entry[i].step = 0x1000;
												for (UINT j = 0; j < candidate.entry[i].offset_count; j++) {
													for (UINT k = j + 1; k < candidate.entry[i].offset_count; k++) {
														if (candidate.entry[i].offset[j] - candidate.entry[i].offset[k] > 0) {
															if (candidate.entry[i].offset[j] - candidate.entry[i].offset[k] < candidate.entry[i].step)
																candidate.entry[i].step = candidate.entry[i].offset[j] - candidate.entry[i].offset[k];
														}
														else {
															if (candidate.entry[i].offset[k] - candidate.entry[i].offset[j] < candidate.entry[i].step)
																candidate.entry[i].step = candidate.entry[i].offset[k] - candidate.entry[i].offset[j];
														}
													}
												}
												if (candidate.entry[i].step < (candidate.entry[i].range_high - candidate.entry[i].range_low)) {
													for (UINT j = 0; j < candidate.entry[i].offset_count && candidate.entry[i].offset_count > 1 && candidate.count < 6; j++) {
														strcpy_p2(parse_out, loop_tab);
														strcat_p2(parse_out, candidate.entry[i].type);
														strcat_p2(parse_out, (char*)" ");
														strcat_p2(parse_out, candidate.entry[i].alias);
														sprintf_s(word, "_%d = ", j);
														strcat_p2(parse_out, word);
														strcat_p2(parse_out, candidate.entry[i].base.line[0].line);
														sprintf_s(word, "0x%03x];\n", candidate.entry[i].range_low + candidate.entry[i].offset[j]);
														strcat_p2(parse_out, word);
														parse_out->index++;
														parse_out->ptr = 0;

														candidate.entry[i].offset_used = 1;
													}
												}
											}
											else {
												sprintf_s(parse_out->line[parse_out->index].line, "%s%s *%s = ",
													loop_tab, candidate.entry[i].type, candidate.entry[i].alias);
												parse_out->ptr = strlen(parse_out->line[parse_out->index].line);

												//		strcpy_p2(parse_out, loop_tab);
												//		strcat_p2(parse_out, candidate.entry[i].type);
												//		strcat_p2(parse_out, (char*)" ");
												//		strcat_p2(parse_out, candidate.entry[i].alias);

												if (i != 0 && strcmp(candidate.entry[0].base.line[0].line, candidate.entry[i].base.line[0].line) == 0) {
													//			strcat_p2(parse_out, (char*)" = ");
													strcat_p2(parse_out, candidate.entry[0].alias);
													if (candidate.entry[i].range_low - candidate.entry[0].range_low >= 0)
														sprintf_s(word, "+ 0x%08x;\n", candidate.entry[i].range_low - candidate.entry[0].range_low);
													else
														sprintf_s(word, "- 0x%08x;\n", candidate.entry[0].range_low - candidate.entry[i].range_low);
													strcat_p2(parse_out, word);
													parse_out->index++;
													parse_out->ptr = 0;
												}
												else if (i > 1 && strcmp(candidate.entry[1].base.line[0].line, candidate.entry[i].base.line[0].line) == 0) {
													strcat_p2(parse_out, candidate.entry[1].alias);
													if (candidate.entry[i].range_low - candidate.entry[1].range_low >= 0)
														sprintf_s(word, "+ 0x%08x;\n", candidate.entry[i].range_low - candidate.entry[1].range_low);
													else
														sprintf_s(word, "- 0x%08x;\n", candidate.entry[1].range_low - candidate.entry[i].range_low);
													strcat_p2(parse_out, word);
													parse_out->index++;
													parse_out->ptr = 0;
												}
												else {
													strcat_p2(parse_out, (char*)"&");
													strcat_p2(parse_out, candidate.entry[i].base.line[0].line);
													if (candidate.entry[i].range_low < 0) {
														while (parse_out->line[parse_out->index].line[parse_out->ptr - 1] == ' ')parse_out->ptr--;
														if (parse_out->line[parse_out->index].line[parse_out->ptr - 1] == '+') {
															parse_out->line[parse_out->index].line[parse_out->ptr - 1] = '\0';
															parse_out->ptr--;
														}
														else
															debug++;
														sprintf_s(word, "- 0x%03x];\n", -candidate.entry[i].range_low);
													}
													else {
														sprintf_s(word, "0x%03x];\n", candidate.entry[i].range_low);
													}
													strcat_p2(parse_out, word);
													parse_out->index++;
													parse_out->ptr = 0;
												}
											}
										}
										else {
											UINT ptr = strlen(candidate.entry[i].base.line[0].line);
											if (candidate.entry[i].base.line[0].line[ptr - 1] == '(') ptr--;

											sprintf_s(parse_out->line[parse_out->index].line, "%s%s *%s = ",
												loop_tab, candidate.entry[i].type, candidate.entry[i].alias);
											parse_out->ptr = strlen(parse_out->line[parse_out->index].line);

											if (i > 0 && strcmp(candidate.entry[0].base.line[0].line, candidate.entry[i].base.line[0].line) == 0) {
												strcat_p2(parse_out, candidate.entry[0].alias);
												if (candidate.entry[i].range_low - candidate.entry[0].range_low >= 0)
													sprintf_s(word, "+ 0x%08x;\n", candidate.entry[i].range_low - candidate.entry[0].range_low);
												else
													sprintf_s(word, "- 0x%08x;\n", candidate.entry[0].range_low - candidate.entry[i].range_low);
												strcat_p2(parse_out, word);
												parse_out->index++;
												parse_out->ptr = 0;
											}
											else if (i > 1 && strcmp(candidate.entry[1].base.line[0].line, candidate.entry[i].base.line[0].line) == 0) {
												strcat_p2(parse_out, candidate.entry[1].alias);
												if (candidate.entry[i].range_low - candidate.entry[1].range_low >= 0)
													sprintf_s(word, "+ 0x%08x;\n", candidate.entry[i].range_low - candidate.entry[1].range_low);
												else
													sprintf_s(word, "- 0x%08x;\n", candidate.entry[1].range_low - candidate.entry[i].range_low);
												strcat_p2(parse_out, word);
												parse_out->index++;
												parse_out->ptr = 0;
											}
											else {
												strcat_p2(parse_out, (char*)"&");
												strcat_p2(parse_out, candidate.entry[i].base.line[0].line);
												while (parse_out->line[parse_out->index].line[parse_out->ptr - 1] == ' ' || parse_out->line[parse_out->index].line[parse_out->ptr - 1] == '\t' ||
													parse_out->line[parse_out->index].line[parse_out->ptr - 1] == '(' || parse_out->line[parse_out->index].line[parse_out->ptr - 1] == '|') parse_out->ptr--;
												if (parse_out->line[parse_out->index].line[parse_out->ptr - 1] == '+' || parse_out->line[parse_out->index].line[parse_out->ptr - 1] == '-' ||
													parse_out->line[parse_out->index].line[parse_out->ptr - 1] == '|') {
													parse_out->ptr--;
												}
												else {
													debug++;
												}
												parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
												// check parenthesis
												parse_out->ptr = 0;
												UINT check = 0;
												while (parse_out->line[parse_out->index].line[parse_out->ptr] != '[')parse_out->ptr++;
												while (parse_out->line[parse_out->index].line[parse_out->ptr] != '\0') {
													if (parse_out->line[parse_out->index].line[parse_out->ptr] == '(')check++;
													else if (parse_out->line[parse_out->index].line[parse_out->ptr] == ')')check--;
													parse_out->ptr++;
												}
												while (check > 0) {
													parse_out->line[parse_out->index].line[parse_out->ptr++] = ')';
													check--;
												}
												parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
												char word[0x80];
												if (candidate.entry[i].range_low < (-0x800))
													sprintf_s(word, " - 0x%x];\n", -candidate.entry[i].range_low);
												else if (candidate.entry[i].range_low >= 0x800)
													sprintf_s(word, " + 0x%x];\n", candidate.entry[i].range_low);
												else
													sprintf_s(word, "];\n");
												if (parse_out->line[parse_out->index].line[parse_out->ptr - 1] == '[')
													strcat_p2(parse_out, (char*)"0");
												strcat_p2(parse_out, word);
												parse_out->index++;
												parse_out->ptr = 0;
											}
											candidate.entry[i].use_alias = 1;
										}
									}
								}
								parse_struct2 parse_loop;
								parse_loop.line = parse_in->line;

								parse_temp->index--;

								if (skip == 0) {
									for (UINT loop1 = l_control.index[depth_out - 1].initial, loop1_offset = 0; loop1 < l_control.index[depth_out - 1].limit; loop1 += l_control.index[depth_out - 1].increment) {
										for (UINT i = 0; i < candidate.count && loop1 != l_control.index[depth_out - 1].initial; i++) {
											if (candidate.entry[i].size >= 0x1000) {
												if (strcmp(candidate.entry[i].type, "_fp16") == 0)
													sprintf_s(word, "%s += 0x%x;\n", candidate.entry[i].alias, 2 * (l_control.index[depth_out - 1].increment << candidate.entry[i].size_shift));
												else
													debug++;
												strcpy_p2(parse_out, loop_tab);
												strcat_p2(parse_out, word);
												parse_out->index++;
												parse_out->ptr = 0;
											}
										}
										for (UINT loop2 = l_control.index[depth_out].initial, loop2_offset = 0; loop2 < l_control.index[depth_out].limit; loop2 += l_control.index[depth_out].increment) {

											if (strcmp(candidate.entry[0].type, "uint64") == 0) {
												if (loop2 == 0x100 + l_control.index[depth_out].initial + loop2_offset) {
													sprintf_s(parse_out->line[parse_out->index++].line, "%s -= 0x0800;\n", candidate.entry[0].alias);
													loop2_offset += 0x100;
												}
											}
											for (parse_loop.index = parse_in->index; parse_loop.index < parse_temp->index; parse_loop.index++, parse_out->index++) {
												parse_out->ptr = 0;
												parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
												parse_loop.ptr = 0;
												if (parse_loop.index >= 302)
													debug++;
												while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != '\0') {
													while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ' ' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '\t') {
														parse_out->ptr = strlen(parse_out->line[parse_out->index].line);
														parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
														parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
													}
													if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '/' && parse_loop.line[parse_loop.index].line[parse_loop.ptr + 1] == '/') {// detect comment - skip
														while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != '\0')		parse_loop.ptr++;
													}
													else if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '\"') {// detect string - skip
														parse_loop.ptr++;
														while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != '\"' && parse_loop.line[parse_loop.index].line[parse_loop.ptr] != '\0')	parse_loop.ptr++;
													}
													else {
														getWord(&parse_loop);
														if (parse_loop.word[0] != '\0') {
															UINT hit = 0;
															if (strcmp(parse_loop.word, l_control.index[depth_out - 1].name) == 0) {
																hit = 1;
																sprintf_s(word, "0x%03x", loop1);
																strcat_p2(parse_out, word);
															}
															else if (strcmp(parse_loop.word, l_control.index[depth_out].name) == 0) {
																hit = 1;
																sprintf_s(word, "0x%03x", loop2);
																strcat_p2(parse_out, word);
															}
															for (UINT i = 0; i < var_type_count & !hit; i++) {
																if (strcmp(parse_loop.word, var_type[i].name) == 0) {
																	hit = 1;
																	if (loop2 == l_control.index[depth_out].initial || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ')')
																		strcat_p2(parse_out, word);
																	else {
																		while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ' ' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '\t') {
																			parse_out->ptr = strlen(parse_out->line[parse_out->index].line);
																			parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
																			parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
																		}
																		getWord(&parse_loop);
																		if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ';') {
																			parse_loop.index++;
																			parse_loop.ptr = 0;
																			parse_out->ptr = 0;
																			parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
																		}
																		else {
																			parse_loop.ptr -= strlen(parse_loop.word);
																		}
																	}
																}
															}
															parse_struct2 parse_offset;
															parse_offset.index = parse_loop.index;
															parse_offset.ptr = parse_loop.ptr;
															parse_offset.line = parse_loop.line;
															parse_offset.word[0] = '\0';
															INT64 offset;
															char sign = '+';
															if (parse_offset.line[parse_offset.index].line[parse_offset.ptr] == '[') {
																int depth = 0;
																parse_offset.ptr++;
																while (parse_offset.line[parse_offset.index].line[parse_offset.ptr] != ']' || depth > 0) {
																	if (parse_offset.line[parse_offset.index].line[parse_offset.ptr] == '[') {
																		depth++;
																	}
																	else if (parse_offset.line[parse_offset.index].line[parse_offset.ptr] == ']') {
																		depth--;
																	}
																	else if (parse_offset.line[parse_offset.index].line[parse_offset.ptr] == '+')
																		sign = '+';
																	else if (parse_offset.line[parse_offset.index].line[parse_offset.ptr] == '-')
																		sign = '-';
																	getWord(&parse_offset);
																	if (parse_offset.word[0] == '\0') {
																		parse_offset.ptr++;
																	}
																	while (parse_offset.line[parse_offset.index].line[parse_offset.ptr] == ' ' || parse_offset.line[parse_offset.index].line[parse_offset.ptr] == '\t')parse_offset.ptr++;
																}
																if (get_integer(&offset, parse_offset.word)) {
																	if (sign == '-')offset *= -1;
																}
															}
															// get base
															for (UINT i = 0; i < candidate.count && !hit; i++) {
																if (strcmp(candidate.entry[i].surname, parse_loop.word) == 0 && (candidate.entry[i].use_alias == 0 || ((candidate.entry[i].range_low >> 12) == (offset >> 12)) ||
																	(candidate.entry[i].range_low < 0x800 && candidate.entry[i].range_low >= -0x800 && offset < 0x800 && offset >= -0x800))) {
																	hit = 1;
																	UINT error = 0;
																	UINT ptr_hold = parse_loop.ptr;
																	for (UINT j = strlen(candidate.entry[i].surname); j < strlen(candidate.entry[i].base.line[0].line); j++) {
																		if (candidate.entry[i].base.line[0].line[j] != parse_loop.line[parse_loop.index].line[parse_loop.ptr++])
																			error++;
																	}
																	if (error == 0) {
																		if (candidate.entry[i].use_alias) {
																			strcat_p2(parse_out, candidate.entry[i].alias);
																			if (candidate.entry[i].op[0] == '\0' || candidate.entry[i].range_low >= 0x1000)
																				if (candidate.entry[i].size <= 0x1000)
																					sprintf_s(word, "[0x%03x]", loop2 * candidate.entry[i].range_inc + (loop1 << candidate.entry[i].size_shift) + offset - candidate.entry[i].range_low - loop2_offset);
																				else
																					sprintf_s(word, "[0x%03x]", loop2 * candidate.entry[i].range_inc + offset - candidate.entry[i].range_low - loop2_offset);
																			else {
																				parse_loop.ptr += strlen(l_control.index[depth_out].name);
																				while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ' ' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '\t')parse_loop.ptr++;
																				if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '+' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '-' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '|')parse_loop.ptr++;
																				else if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '<') {
																					parse_loop.ptr++;
																					if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '<')
																						parse_loop.ptr++;
																				}
																				else
																					debug++;
																				while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ' ' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '\t')parse_loop.ptr++;
																				getWord(&parse_loop);
																				if (parse_loop.word[0] == '\0')
																					sprintf_s(parse_loop.word, "0");
																				sprintf_s(word, "[0x%03x %s %s]", loop2 * candidate.entry[i].range_inc, candidate.entry[i].op, parse_loop.word);
																			}
																			strcat_p2(parse_out, word);
																			while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != ']')parse_loop.ptr++;
																			parse_loop.ptr++;
																		}
																		else {
																			strcat_p2(parse_out, candidate.entry[i].base.line[0].line);
																			if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '(') {
																				strcat_p2(parse_out, (char*)"(");
																				parse_loop.ptr++;
																			}
																			parse_loop.ptr += strlen(l_control.index[depth_out].name);

																			getWord(&parse_loop); // should be index

																			sprintf_s(word, "0x%03x", loop2 * candidate.entry[i].range_inc);
																			strcat_p2(parse_out, word);
																			while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != ']')
																				parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
																			parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
																			parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
																		}
																	}
																	else {
																		debug++;
																	}
																}
															}
															if (!hit) {
																strcat_p2(parse_out, parse_loop.word);
															}
														}
														else {
															parse_out->ptr = strlen(parse_out->line[parse_out->index].line);
															parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
															parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
														}
													}
												}
											}
										}
									}
								}
								else if (use_offsets) {
									for (UINT loop = l_control.index[depth_out].initial; loop < l_control.index[depth_out].limit; loop += l_control.index[depth_out].increment) {
										for (UINT i = 0; i < candidate.count; i++) {
											if (loop != l_control.index[depth_out].initial && candidate.entry[i].offset_used) {
												for (UINT j = 0; j < candidate.entry[i].offset_count && candidate.entry[i].offset_count > 1; j++) {
													if (candidate.entry[i].offset[j] < loop) {
														candidate.entry[i].offset[j] += candidate.entry[i].offset_count;

														strcpy_p2(parse_out, loop_tab);
														strcat_p2(parse_out, candidate.entry[i].alias);
														sprintf_s(word, "_%d = ", j);
														strcat_p2(parse_out, word);
														strcat_p2(parse_out, candidate.entry[i].base.line[0].line);
														sprintf_s(word, "0x%03x];\n", candidate.entry[i].range_low + candidate.entry[i].offset[j]);
														strcat_p2(parse_out, word);
														parse_out->index++;
														parse_out->ptr = 0;
													}
												}
											}
										}
										for (parse_loop.index = parse_in->index; parse_loop.index < parse_temp->index; parse_loop.index++, parse_out->index++) {
											parse_out->ptr = 0;
											parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
											parse_loop.ptr = 0;
											if (parse_loop.index >= 790)
												debug++;
											while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != '\0') {
												while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ' ' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '\t') {
													parse_out->ptr = strlen(parse_out->line[parse_out->index].line);
													parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
													parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
												}
												if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '/' && parse_temp->line[parse_temp->index].line[parse_temp->ptr + 1] == '/') {// detect comment - skip
													while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] != '\0')		parse_temp->ptr++;
												}
												else if (parse_temp->line[parse_temp->index].line[parse_temp->ptr] == '\"') {// detect string - skip
													parse_temp->ptr++;
													while (parse_temp->line[parse_temp->index].line[parse_temp->ptr] != '\"' && parse_temp->line[parse_temp->index].line[parse_temp->ptr] != '\0')	parse_temp->ptr++;
												}
												else {
													getWord(&parse_loop);
													if (parse_loop.word[0] != '\0') {
														UINT hit = 0;
														if (strcmp(parse_loop.word, l_control.index[depth_out].name) == 0) {
															hit = 1;
															sprintf_s(word, "0x%03x", loop);
															strcat_p2(parse_out, word);
														}
														for (UINT i = 0; i < var_type_count & !hit; i++) {
															if (strcmp(parse_loop.word, var_type[i].name) == 0) {
																hit = 1;
																if (loop == l_control.index[depth_out].initial)
																	strcat_p2(parse_out, parse_loop.word);
															}
														}
														parse_struct2 parse_offset;
														parse_offset.index = parse_loop.index;
														parse_offset.ptr = parse_loop.ptr;
														parse_offset.line = parse_loop.line;
														parse_offset.word[0] = '\0';
														INT64 offset;
														if (parse_offset.line[parse_offset.index].line[parse_offset.ptr] == '[') {
															while (parse_offset.line[parse_offset.index].line[parse_offset.ptr] != ']') {
																getWord(&parse_offset);
																if (parse_offset.word[0] == '\0') {
																	parse_offset.ptr++;
																}
																while (parse_offset.line[parse_offset.index].line[parse_offset.ptr] == ' ' || parse_offset.line[parse_offset.index].line[parse_offset.ptr] == '\t')parse_offset.ptr++;
															}
															if (get_integer(&offset, parse_offset.word)) {
															}
														}
														// get base
														for (UINT i = 0; i < candidate.count && !hit; i++) {
															if (strcmp(candidate.entry[i].surname, parse_loop.word) == 0 && (candidate.entry[i].use_alias == 0 || ((candidate.entry[i].range_low >> 12) == (offset >> 12)))) {
																hit = 1;
																UINT error = 0;
																UINT ptr_hold = parse_loop.ptr;
																for (UINT j = strlen(candidate.entry[i].surname); j < strlen(candidate.entry[i].base.line[0].line); j++) {
																	if (candidate.entry[i].base.line[0].line[j] != parse_loop.line[parse_loop.index].line[parse_loop.ptr++])
																		error++;
																}
																if (error == 0) {
																	if (candidate.entry[i].use_alias) {
																		strcat_p2(parse_out, candidate.entry[i].alias);
																		if (candidate.entry[i].op[0] == '\0' || candidate.entry[i].range_low >= 0x1000) {
																			if (candidate.entry[i].offset_used) {
																				for (UINT j = 0; j < candidate.entry[i].offset_count; j++) {
																					if (loop * candidate.entry[i].range_inc + offset - candidate.entry[i].range_low == candidate.entry[i].offset[j]) {
																						sprintf_s(word, "_%d", j);
																					}
																				}
																			}
																			else {
																				sprintf_s(word, "[0x%03x]", loop * candidate.entry[i].range_inc + offset - candidate.entry[i].range_low);
																			}
																		}
																		else {
																			parse_loop.ptr += strlen(l_control.index[depth_out].name);
																			while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ' ' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '\t')parse_loop.ptr++;
																			if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '+' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '-' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '|')parse_loop.ptr++;
																			else if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '<') {
																				parse_loop.ptr++;
																				if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '<')
																					parse_loop.ptr++;
																			}
																			else
																				debug++;
																			while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ' ' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '\t')parse_loop.ptr++;
																			getWord(&parse_loop);
																			sprintf_s(word, "[0x%03x %s %s]", loop * candidate.entry[i].range_inc, candidate.entry[i].op, parse_loop.word);
																		}
																		strcat_p2(parse_out, word);
																		while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != ']')parse_loop.ptr++;
																		parse_loop.ptr++;
																	}
																	else {
																		strcat_p2(parse_out, candidate.entry[i].base.line[0].line);
																		if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '(') {
																			strcat_p2(parse_out, (char*)"(");;
																			parse_loop.ptr++;
																		}
																		parse_loop.ptr += strlen(l_control.index[depth_out].name);

																		getWord(&parse_loop); // should be index

																		sprintf_s(word, "0x%03x", loop * candidate.entry[i].range_inc);
																		strcat_p2(parse_out, word);
																		while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != ']')
																			parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
																		parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
																		parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
																	}
																}
																else {
																	debug++;
																}
															}
														}
														if (!hit) {
															strcat_p2(parse_out, parse_loop.word);
														}
													}
													else {
														parse_out->ptr = strlen(parse_out->line[parse_out->index].line);
														parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
														parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
													}
												}
											}
										}
									}
								}
								else {
									for (UINT loop = l_control.index[depth_out].initial, loop_offset = 0; loop < l_control.index[depth_out].limit; loop += l_control.index[depth_out].increment) {

										if (strcmp(candidate.entry[0].type, "uint64") == 0) {
											if (loop == 0x100 + l_control.index[depth_out].initial + loop_offset) {
												//		strcpy_p2(parse_out, candidate.entry[0].alias);
												//		strcat(parse_out->line[parse_out->index++].line, " -= 0x0800;\n");
												sprintf_s(parse_out->line[parse_out->index++].line, "%s -= 0x0800;\n", candidate.entry[0].alias);
												loop_offset += 0x100;
											}
										}
										for (parse_loop.index = parse_in->index; parse_loop.index < parse_temp->index; parse_loop.index++, parse_out->index++) {
											parse_out->ptr = 0;
											parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
											parse_loop.ptr = 0;
											if (parse_loop.index >= 64)
												debug++;
											while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != '\0') {
												copy_blanks(parse_out, &parse_loop);
												if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '/' && parse_loop.line[parse_loop.index].line[parse_loop.ptr + 1] == '/') {// detect comment - skip
													while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != '\0')		parse_loop.ptr++;
												}
												else if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '\"') {// detect string - skip
													parse_loop.ptr++;
													while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != '\"' && parse_loop.line[parse_loop.index].line[parse_loop.ptr] != '\0')	parse_loop.ptr++;
												}
												else {
													getWord(&parse_loop);
													if (parse_loop.word[0] != '\0') {
														UINT hit = 0;
														if (strcmp(parse_loop.word, l_control.index[depth_out].name) == 0) {
															hit = 1;
															sprintf_s(word, "0x%03x", loop);
															strcat_p2(parse_out, word);
														}
														for (UINT i = 0; i < var_type_count & !hit; i++) {
															if (strcmp(parse_loop.word, var_type[i].name) == 0) {
																hit = 1;
																if (loop == l_control.index[depth_out].initial || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ')')
																	strcat_p2(parse_out, parse_loop.word);
																else {
																	while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ' ' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '\t') {
																		parse_out->ptr = strlen(parse_out->line[parse_out->index].line);
																		parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
																		parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
																	}
																	getWord(&parse_loop);
																	if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ';') {
																		parse_loop.index++;
																		parse_loop.ptr = 0;
																		parse_out->ptr = 0;
																		parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
																	}
																	else {
																		parse_loop.ptr -= strlen(parse_loop.word);
																	}
																}
															}
														}
														parse_struct2 parse_offset;
														parse_offset.index = parse_loop.index;
														parse_offset.ptr = parse_loop.ptr;
														parse_offset.line = parse_loop.line;
														parse_offset.word[0] = '\0';
														INT64 offset;
														char sign = '+';
														if (parse_offset.line[parse_offset.index].line[parse_offset.ptr] == '[') {
															int depth = 0;
															parse_offset.ptr++;
															while (parse_offset.line[parse_offset.index].line[parse_offset.ptr] != ']' || depth > 0) {
																if (parse_offset.line[parse_offset.index].line[parse_offset.ptr] == '[') {
																	depth++;
																}
																else if (parse_offset.line[parse_offset.index].line[parse_offset.ptr] == ']') {
																	depth--;
																}
																else if (parse_offset.line[parse_offset.index].line[parse_offset.ptr] == '+')
																	sign = '+';
																else if (parse_offset.line[parse_offset.index].line[parse_offset.ptr] == '-')
																	sign = '-';
																getWord(&parse_offset);
																if (parse_offset.word[0] == '\0') {
																	parse_offset.ptr++;
																}
																while (parse_offset.line[parse_offset.index].line[parse_offset.ptr] == ' ' || parse_offset.line[parse_offset.index].line[parse_offset.ptr] == '\t')parse_offset.ptr++;
															}
															if (get_integer(&offset, parse_offset.word)) {
																if (sign == '-')offset *= -1;
															}
														}
														// get base
														for (UINT i = 0; i < candidate.count && !hit; i++) {
															if (strcmp(candidate.entry[i].surname, parse_loop.word) == 0 && (candidate.entry[i].use_alias == 0 || ((candidate.entry[i].range_low >> 11) == (offset >> 11)) ||
																(candidate.entry[i].range_low < 0x800 && candidate.entry[i].range_low >= -0x800 && offset < 0x800 && offset >= -0x800))) {
																hit = 1;
																UINT error = 0;
																UINT ptr_hold = parse_loop.ptr;
																for (UINT j = strlen(candidate.entry[i].surname); j < strlen(candidate.entry[i].base.line[0].line); j++) {
																	if (candidate.entry[i].base.line[0].line[j] != parse_loop.line[parse_loop.index].line[parse_loop.ptr++])
																		error++;
																}
																if (error == 0) {
																	if (candidate.entry[i].use_alias) {
																		parse_out->ptr = strlen(parse_out->line[parse_out->index].line);
																		strcat_p2(parse_out, candidate.entry[i].alias);
																		if (candidate.entry[i].op[0] == '\0' || candidate.entry[i].range_low >= 0x1000)
																			sprintf_s(word, "[0x%03x]", loop * candidate.entry[i].range_inc + offset - candidate.entry[i].range_low - loop_offset);
																		else {
																			parse_loop.ptr += strlen(l_control.index[depth_out].name);
																			while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ' ' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '\t')parse_loop.ptr++;
																			char operator_cache = ' ';
																			if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '+' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '-' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '|') {
																				operator_cache = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
																			}
																			else if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '<') {
																				parse_loop.ptr++;
																				if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '<')
																					parse_loop.ptr++;
																			}
																			else
																				debug++;
																			while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ' ' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '\t')parse_loop.ptr++;
																			getWord(&parse_loop);
																			INT64 number;
																			if (parse_loop.word[0] == '\0') {
																				sprintf_s(parse_loop.word, "0");
																			}
																			else if (get_integer(&number, parse_loop.word)) {
																				if (operator_cache == '-')
																					sprintf_s(word, "[0x%03x + 0x%x]", loop * candidate.entry[i].range_inc, -number - candidate.entry[i].range_low);
																				else
																					sprintf_s(word, "[0x%03x + 0x%x]", loop * candidate.entry[i].range_inc, number - candidate.entry[i].range_low);
																			}
																			else {
																				sprintf_s(word, "[0x%03x]", loop * candidate.entry[i].range_inc);
																			}
																		}
																		strcat_p2(parse_out, word);
																		while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != ']')parse_loop.ptr++;
																		parse_loop.ptr++;
																	}
																	else {
																		strcat_p2(parse_out, candidate.entry[i].base.line[0].line);
																		if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '(') {
																			strcat_p2(parse_out, (char*)"(");;
																			parse_loop.ptr++;
																		}
																		parse_loop.ptr += strlen(l_control.index[depth_out].name);

																		getWord(&parse_loop); // should be index

																		sprintf_s(word, "0x%03x", loop * candidate.entry[i].range_inc);
																		strcat_p2(parse_out, word);
																		while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != ']')
																			parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
																		parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
																		parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
																	}
																}
																else {
																	debug++;
																}
															}
														}
														if (!hit) {
															strcat_p2(parse_out, parse_loop.word);
														}
													}
													else {
														parse_out->ptr = strlen(parse_out->line[parse_out->index].line);
														parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
														parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
													}
												}
											}
										}
									}
								}
								if (skip == 0) {
									strcpy_p2(parse_out, (char*)"//");
									strcat_p2(parse_out, &parse_loop);
									parse_out->index++;
									parse_loop.index++;
									parse_out->ptr = 0;
								}
								else {
									strcpy_p2(parse_out, &parse_loop);
									parse_out->index++;
									parse_loop.index++;
									parse_out->ptr = 0;
								}
								parse_in->index = parse_loop.index;
								parse_in->ptr = 0;
								parse_out->ptr = 0;
								if (parse_in->index >= 66)
									debug++;
							}
							else {
								char loop_tab[0x80];
								//		loop_tab[0] = '\0';
								INT depth_out = l_control.depth_out + 1;
								UINT i = 0;
								for (; i < depth_out; i++) loop_tab[i] = '\t';
								loop_tab[i] = '\0';
								strcpy_p2(parse_out, loop_tab);
								strcat_p2(parse_out, (char*)"//");
								strcat_p2(parse_out, parse_in);
								parse_out->index++;
								parse_in->index++;
								strcpy_p2(parse_out, loop_tab);
								char word[0x80];
								sprintf_s(word, "{	// %d \n", depth_out);
								strcat_p2(parse_out, word);
								parse_out->index++;
								parse_out->ptr = 0;

								parse_struct2 parse_loop;
								parse_loop.line = parse_in->line;

								for (UINT loop = l_control.index[depth_out].initial, loop_offset = 0; loop < l_control.index[depth_out].limit; loop += l_control.index[depth_out].increment) {
									for (parse_loop.index = parse_in->index; parse_loop.index < parse_temp->index - 1; parse_loop.index++, parse_out->index++) {
										parse_out->ptr = 0;
										parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
										parse_loop.ptr = 0;
										if (parse_loop.index >= 34)
											debug++;
										while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != '\0') {
											while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == ' ' || parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '\t') {
												parse_out->ptr = strlen(parse_out->line[parse_out->index].line);
												parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
												parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
											}
											if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '/' && parse_loop.line[parse_loop.index].line[parse_loop.ptr + 1] == '/') {// detect comment - skip
												while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != '\0')		parse_loop.ptr++;
											}
											else if (parse_loop.line[parse_loop.index].line[parse_loop.ptr] == '\"') {// detect string - skip
												parse_loop.ptr++;
												while (parse_loop.line[parse_loop.index].line[parse_loop.ptr] != '\"' && parse_loop.line[parse_loop.index].line[parse_loop.ptr] != '\0')	parse_loop.ptr++;
											}
											else {
												getWord(&parse_loop);
												if (parse_loop.word[0] != '\0') {
													UINT hit = 0;
													if (strcmp(parse_loop.word, l_control.index[depth_out].name) == 0) {
														hit = 1;
														sprintf_s(word, "0x%03x", loop);
														strcat_p2(parse_out, word);
													}
													else {
														strcat_p2(parse_out, parse_loop.word);
													}
												}
												else {
													parse_out->ptr = strlen(parse_out->line[parse_out->index].line);
													parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_loop.line[parse_loop.index].line[parse_loop.ptr++];
													parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
												}
											}
										}
									}
								}
								strcpy_p2(parse_out, &parse_loop);//do not comment out paenthesis,  needed to contain variable declarations
								parse_out->index++;
								parse_loop.index++;

								parse_in->index = parse_loop.index;
								parse_in->ptr = 0;
								parse_out->ptr = 0;
							}
							UINT hit = 1;
							while (candidate.count > 0 && hit) {
								if (candidate.count == 9) {
									if (candidate.entry[0].depth > l_control.depth_out)
										candidate.count = 0;
									hit = 0;
								}
								else if (candidate.entry[candidate.count - 1].depth > l_control.depth_out) {
									candidate.entry[candidate.count].offset_count = 0;
									candidate.entry[candidate.count].offset_used = 0;
									candidate.count--;
								}
								else {
									hit = 0;
								}
							}
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
			if (l_control.depth_out <= 0) {
				funct_code = 0;
				array_list.count = 0;
			}
		}
		else {// funct_code
			if (parse_in->index >= 0x137)
				debug++;
			strcpy_p2(parse_out, parse_in);
			parse_in->ptr = 0;
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {// detect comment - skip
					while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')		parse_in->ptr++;
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\"') {// detect string - skip
					parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\"' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')	parse_in->ptr++;
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
					l_control.index[l_control.depth_out].addr = parse_in->index;
					l_control.index[l_control.depth_out].out_addr = parse_out->index;
					l_control.depth_out++;
					parse_in->ptr++;
					l_control.index[l_control.depth_out].name[0] = '\0';
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '}') {
					l_control.depth_out--;
					l_control.index[l_control.depth_out].name[0] = '\0';
					parse_in->ptr++;
					if (candidate.count > 0)
						debug++;
				}
				else {
					//					parse_in->ptr++;
					getWord(parse_in);
					char match_index;
					if (VariableTypeMatch(&match_index, parse_in->word, var_type)) {
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						getWord(parse_in);
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
							parse_in->ptr++;
							funct_code = 1;
							//							array_list.count = 0;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {// detect comment - skip
									while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')		parse_in->ptr++;
								}
								else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\"') {// detect string - skip
									parse_in->ptr++;
									while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\"' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')	parse_in->ptr++;
								}
								else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
									l_control.index[l_control.depth_out].addr = parse_in->index;
									l_control.index[l_control.depth_out].out_addr = parse_out->index;
									l_control.depth_out++;
									parse_in->ptr++;
									l_control.index[l_control.depth_out].name[0] = '\0';
								}
								else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '}') {
									l_control.depth_out--;
									parse_in->ptr++;
									if (candidate.count > 0)
										debug++;
								}
								else {
									getWord(parse_in);
									if (parse_in->word[0] != '\0') {
										char match_index;
										if (VariableTypeMatch(&match_index, parse_in->word, var_type)) {
											while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
											if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
												parse_in->ptr++;
												getWord(parse_in);
												sprintf_s(array_list.entry[array_list.count].type, "%s", var_type[match_index].name);
												strcpy_word(array_list.entry[array_list.count].name, parse_in);
												array_list.entry[array_list.count++].depth = l_control.depth_out + 1;
												while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
											}
											else {
												getWord(parse_in);
											}
											while (parse_in->line[parse_in->index].line[parse_in->ptr] == ',') {
												parse_in->ptr++;
												while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
												getWord(parse_in);
												if (parse_in->word[0] != '\0') {
													char match_index;
													if (VariableTypeMatch(&match_index, parse_in->word, var_type)) {
														while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
														if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
															parse_in->ptr++;
															getWord(parse_in);
															sprintf_s(array_list.entry[array_list.count].type, "%s", var_type[match_index].name);
															strcpy_word(array_list.entry[array_list.count].name, parse_in);
															array_list.entry[array_list.count++].depth = l_control.depth_out + 1;
															while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
														}
														else {
															getWord(parse_in);
														}
													}
												}
											}
											while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
												if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
													l_control.index[l_control.depth_out].addr = parse_in->index;
													l_control.index[l_control.depth_out].out_addr = parse_out->index;
													l_control.depth_out++;
													l_control.index[l_control.depth_out].name[0] = '\0';
												}
												parse_in->ptr++;
											}
											if (array_list.count >= 0x10)
												debug++;
											parse_out->ptr = 0;
											strcat_p2(parse_out, parse_in->line[parse_in->index].line);
										}
										else {
											strcat_p2(parse_out, parse_in->word);
										}
									}
									else {
										parse_in->ptr++;
									}
								}
							}
						}
						else {
							parse_in->ptr = strlen(parse_in->line[parse_in->index].line);
						}
					}
					else {
						parse_in->ptr = strlen(parse_in->line[parse_in->index].line);
					}
				}
			}
		}
	}

	FILE* dest;
	fopen_s(&dest, dst_name, "w");

	for (parse_in->index = 0; parse_in->index < parse_out->index; parse_in->index++) {
		fprintf(dest, "%s", parse_out->line[parse_in->index].line);
	}
	fclose(dest);
	free(parse_temp);
	for (UINT i = 0; i < 0x10; i++) {
		free(candidate.entry[i].base.line);
	}
}