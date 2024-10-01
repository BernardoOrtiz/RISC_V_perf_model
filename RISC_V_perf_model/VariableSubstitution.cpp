// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

struct vector_entry_latch_type {// load vector once into a variable - reduce load pressure
	char name[0x80];// vector + index
	char latch[0x80];// variable substitution
	UINT depth; // copy from vector declaration - clear when out of cotext
	UINT in_addr; // copy from vector declaration - clear when out of cotext
	UINT out_addr; // copy from vector declaration - clear when out of cotext
	UINT hit; // do not replace if only 1 instance 
};
struct vector_set_type {
	vector_entry_latch_type entry[0x20];
	UINT count;
};
void getVector(parse_struct2* parse) {
	int debug = 0;
	UINT16 length = strlen(parse->line[parse->index].line);
	UINT16 i;
	if (parse->ptr < length) {
		i = parse->ptr;
		if (parse->line[parse->index].line[i] == '-')
			parse->word[i - parse->ptr] = parse->line[parse->index].line[i++];
		for (; i < length && debug == 0; i++) {
			if (parse->line[parse->index].line[i] != ' ' && parse->line[parse->index].line[i] != '\t' && parse->line[parse->index].line[i] != '\0' && parse->line[parse->index].line[i] != '\n' && parse->line[parse->index].line[i] != ',' && parse->line[parse->index].line[i] != ';' && parse->line[parse->index].line[i] != ':' && parse->line[parse->index].line[i] != '=' && parse->line[parse->index].line[i] != '?' && parse->line[parse->index].line[i] != '&' &&
				parse->line[parse->index].line[i] != '|' && parse->line[parse->index].line[i] != '^' && parse->line[parse->index].line[i] != '+' && parse->line[parse->index].line[i] != '-' && parse->line[parse->index].line[i] != '*' && parse->line[parse->index].line[i] != '/' && parse->line[parse->index].line[i] != '<' && parse->line[parse->index].line[i] != '>' &&
				parse->line[parse->index].line[i] != '(' && parse->line[parse->index].line[i] != ')' && parse->line[parse->index].line[i] != '{' && parse->line[parse->index].line[i] != '}' && parse->line[parse->index].line[i] != '[' && parse->line[parse->index].line[i] != ']' && parse->line[parse->index].line[i] != '\"') {
				parse->word[i - parse->ptr] = parse->line[parse->index].line[i];
			}
			else {
				debug++;
			}
		}
		i = i - 1;
		if (parse->line[parse->index].line[i] == '[') {
			while (parse->line[parse->index].line[i] != ']') parse->word[i - parse->ptr] = parse->line[parse->index].line[i++];
			parse->word[i - parse->ptr] = parse->line[parse->index].line[i++];
		}
		parse->word[i - parse->ptr] = '\0';
		parse->ptr += strlen(parse->word);
	}
	else {
		parse->word[0] = '\0';
	}
}

void latch_release(UINT* touched, vector_set_type* vs, parse_struct2* parse_out, parse_struct2* parse_in, INT depth) {
	UINT debug = 0;
	UINT i = 0;
	for (; i < 0x10; i++) { // change to tags rather than 
		if (vs->entry[i].depth > depth) {
			if (vs->entry[i].hit > 1 && !touched[0] && vs->count < 0x10) {
				debug++;
				touched[0] = 1;
				UINT in_limit = parse_in->index + 1;
				parse_in->index = vs->entry[i].in_addr;
				parse_out->index = vs->entry[i].out_addr;
				// error, need unit type
				sprintf_s(parse_out->line[parse_out->index++].line, "\tUINT %s = %s;\n", vs->entry[i].latch, vs->entry[i].name);
				for (; parse_in->index < in_limit; parse_in->index++, parse_out->index++) {
					parse_in->ptr = 0;
					parse_out->ptr = 0;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] != '=' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')	copy_char(parse_out, parse_in);
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
						copy_char(parse_out, parse_in);
						while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {// detect comment - skip
								while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')		copy_char(parse_out, parse_in);
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\"') {// detect string - skip
								copy_char(parse_out, parse_in);
								while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\"' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')	copy_char(parse_out, parse_in);
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
								//								depth++;
								copy_char(parse_out, parse_in);
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '}') {
								//								depth--;
								copy_char(parse_out, parse_in);
							}
							else {
								switch (parse_in->line[parse_in->index].line[parse_in->ptr]) {
								case '<':
								case '>':
								case '(':
								case ')':
								case '|':
								case '&':
								case '+':
								case '-':
								case '*':
								case '/':
								case '?':
								case ',':
								case ':':
								case ';':
								case '\n':
									copy_char(parse_out, parse_in);
									break;
								default:
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	copy_char(parse_out, parse_in);
									getVector(parse_in);
									if (strcmp(parse_in->word, vs->entry[i].name) == 0) {
										strcat_p2(parse_out, vs->entry[i].latch);
									}
									else {
										strcat_p2(parse_out, parse_in->word);
									}
									break;
								}
							}

						}
					}
				}
				strcpy_p2(parse_out, parse_in);
			}
			vs->entry[i].name[0] = '\0';
			vs->entry[i].latch[0] = '\0';
			vs->entry[i].depth = 0;
			vs->entry[i].in_addr = 0;
			vs->entry[i].out_addr = 0;
			vs->entry[i].hit = 0;
		}
	}
}

void skip_line3(INT* depth, UINT* touched, vector_set_type* vs, parse_struct2* parse_out, parse_struct2* parse_in) {
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {// detect comment - skip
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')		parse_in->ptr++;
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\"') {// detect string - skip
			parse_in->ptr++;
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\"' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')	parse_in->ptr++;
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
			depth[0]++;
			parse_in->ptr++;
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '}') {
			depth[0]--;
			parse_in->ptr++;
			UINT touched2 = 0;
			latch_release(&touched2, vs, parse_out, parse_in, depth[0]);
			touched[0] |= touched2;
		}
		else {
			parse_in->ptr++;
		}
	}
	strcpy_p2(parse_out, parse_in);
}

void variable_substitution(const char* dst_name, const char* src_name, parse_struct2_set* parse) {
	int debug = 0;
	FILE* src;
	fopen_s(&src, src_name, "r");

	UINT  line_count = 0;

	while (fgets(parse->a.line[line_count++].line, 0x200, src) != NULL) {}
	line_count--;

	fclose(src);
	parse_struct2* parse_in = &parse->a;
	parse_struct2* parse_out = &parse->b;
	parse_struct2 parse_temp;
	parse_temp.line = parse_in->line;
	//	UINT synch_check_point = 0;

	UINT touched = 1;
	UINT pass = 0;
	UINT pass_count = 0;
	while (touched && pass_count < 1) {// 0x10
		pass_count++;
		int depth = 0;
		vector_set_type vs;
		vs.count = 0;
		for (UINT i = 0; i < 0x20; i++) {
			vs.entry[i].name[0] = '\0';
			vs.entry[i].latch[0] = '\0';
			vs.entry[i].hit = 0;
			vs.entry[i].depth = 0;
		}

		touched = 0;
		if (pass == 0) {
			parse_in = &parse->a;
			parse_out = &parse->b;
		}
		else {
			parse_in = &parse->b;
			parse_out = &parse->a;
		}
		pass = !pass;
		for (parse_in->index = 0, parse_out->index = 0; parse_in->index < line_count; parse_in->index++, parse_out->index++) {
			parse_in->ptr = 0;
			parse_out->ptr = 0;
			parse_out->line[parse_out->index].line[0] = '\0';

			if (parse_in->index >= 946)
				debug++;
			if (parse_in->index >= 63 && pass_count == 2)
				debug++;
			if (parse_in->index >= 0xa0)
				debug++;
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	copy_char(parse_out, parse_in);
			if (depth > 0) {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0' && parse_in->line[parse_in->index].line[parse_in->ptr] != '=') {
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {// detect comment - skip
						while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')		copy_char(parse_out, parse_in);
					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\"') {// detect string - skip
						copy_char(parse_out, parse_in);
						while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\"' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')	copy_char(parse_out, parse_in);
					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '{') {
						depth++;
						copy_char(parse_out, parse_in);
					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '}') {
						depth--;
						copy_char(parse_out, parse_in);
						UINT touched2 = 0;
						latch_release(&touched2, &vs, parse_out, parse_in, depth);
						touched |= touched2;
					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\n') {
						copy_char(parse_out, parse_in);
					}
					else {
						getWord(parse_in);
						if (parse_in->word_ptr != 0) {// need to chack for "for", "if", or a function call "func("
							if (strcmp("for", parse_in->word) == 0) {
								skip_line3(&depth, &touched, &vs, parse_out, parse_in);
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

				// assumes parse_in->line[parse_in->index].line[parse_in->ptr] != '\0' means right hand side of equation
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
					copy_char(parse_out, parse_in);
					while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	copy_char(parse_out, parse_in);
						while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {// detect comment - skip
								while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')		copy_char(parse_out, parse_in);
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\"') {// detect string - skip
								copy_char(parse_out, parse_in);
								while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\"' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')	copy_char(parse_out, parse_in);
							}
							else {
								switch (parse_in->line[parse_in->index].line[parse_in->ptr]) {
								case '{':
									depth++;
									copy_char(parse_out, parse_in);
									break;
								case '}': {
									depth--;
									copy_char(parse_out, parse_in);
									UINT touched2 = 0;
									latch_release(&touched2, &vs, parse_out, parse_in, depth);
									touched |= touched2;
								}
										break;
								case ' ':
								case '\t':
								case '=':
								case '+':
								case '-':
								case '*':
								case '/':
								case '?':
								case ':':
								case '>':
								case '<':
								case ',':
								case ';':
								case '&':
								case '|':
								case '^':
								case '\n':
								case '(':
								case ')':
									copy_char(parse_out, parse_in);
									break;
								default:
									getWord(parse_in);// change to get ve latch, return 0 if null, 1 if word, 2 if vector latch
									if (parse_in->word_ptr != 0) {
										strcat_p2(parse_out, parse_in->word);
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	copy_char(parse_out, parse_in);
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') { // vector detection
											char sourceID[0x80];
											UINT ptr = 0;
											INT depth2 = 0;
											copy_char(parse_out, parse_in);
											while (parse_in->line[parse_in->index].line[parse_in->ptr] != ']' || depth2 > 0) {
												if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[')
													depth2++;
												else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']')
													depth2--;
												sourceID[ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr];
												copy_char(parse_out, parse_in);
											}
											sourceID[ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr];
											sourceID[ptr] = '\0';
											copy_char(parse_out, parse_in);

											char word[0x80];
											sprintf_s(word,"%s%s", parse_in->word, sourceID);
										//	strcpy(word, parse_in->word);
										//	strcat(word, sourceID);
											UINT8 hit = 0;
											for (UINT8 index = 0; index < 0x10 && !hit; index++) {
												if (strcmp(word, vs.entry[index].name) == 0) {
													hit = 1;
													vs.entry[index].hit++;
												}
											}
											if (hit == 0) {// tag does not exist
												sprintf_s(vs.entry[vs.count].name,"%s", word);
										//		strcpy_word(vs.entry[vs.count].latch, parse_in);
										//		char latchID[0x80];
										//		sprintf_s(latchID, "_latch%d", vs.count);
										//		strcat(vs.entry[vs.count].latch, latchID);
												sprintf_s(vs.entry[vs.count].latch, "%s_latch%d", parse_in, vs.count);

												vs.entry[vs.count].in_addr = parse_in->index;
												vs.entry[vs.count].out_addr = parse_out->index;
												vs.entry[vs.count].depth = depth;
												vs.entry[vs.count].hit = 1;
												vs.count++;
												if (vs.count >= 0x10)
													vs.count = 0x10;
											}
										}
										else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] != '=') {
											// find loads from a vector (memory space)
											// if too close to the write, need to insert a synch instruction
											copy_char(parse_out, parse_in);
										}
										else {
										}
									}
									else {
										debug++;
									}
									break;
								}
							}
						}
					}
				}
			}
			else {
				skip_line3(&depth, &touched, &vs, parse_out, parse_in);
			}
		}
		line_count = parse_out->index;
	}

	FILE* dest;
	fopen_s(&dest, dst_name, "w");

	for (parse_in->index = 0; parse_in->index < parse_out->index; parse_in->index++) {
		fprintf(dest, "%s", parse_out->line[parse_in->index].line);
	}
	fclose(dest);
	//	free(parse_1.line);
	//	free(parse_2.line);
}
