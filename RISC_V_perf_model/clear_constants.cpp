// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

void copy_ptr_char(parse_struct2* parse_out, char letter) {
	parse_out->line[parse_out->index].line[parse_out->ptr++] = letter;
	parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
}
void copy_ptr_char(parse_struct2* parse_out, parse_struct2* parse_in, UINT index, UINT* ptr) {
	parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[index].line[ptr[0]++];
	parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
}
UINT8 replace_word(UINT stop_line, UINT depth, parse_struct2* parse_a, parse_struct2* parse_b, char* target_word, char* target_value) {
	int debug = 0;
	UINT8 replacement_count = 0;
	INT8 depth_latch = depth;

	//	for (parse_a->index = 0, parse_b->index = 0; parse_a->index < stop_line && depth >= depth_latch; parse_a->index++) {
	for (; parse_a->index < stop_line && depth >= depth_latch; parse_a->index++) {
		parse_a->ptr = 0;
		parse_b->ptr = 0;
		parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
		while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')
			copy_char(parse_b, parse_a);
		getWord(parse_a);
		if (strcmp(target_word, parse_a->word) == 0) {
			strcpy_p2(parse_b, parse_a);
			parse_b->index++;
			strcpy_p2(parse_b, (char*) "\tERROR: cannot set a value to a constant variable after it has been initialized\n\0");
			parse_b->index++;
			strcpy_p2(parse_b, (char*) "\n\0");
			parse_b->index++;
		}
		else {
			strcat_p2(parse_b, parse_a->word);
			while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')
				copy_char(parse_b, parse_a);
			if (parse_a->line[parse_a->index].line[parse_a->ptr] == '"') {
				parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
				parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
				while (parse_a->line[parse_a->index].line[parse_a->ptr] != '"') {
					copy_char(parse_b, parse_a);
					if (parse_a->line[parse_a->index].line[parse_a->ptr] == '\0')
						debug++;
				}
				copy_char(parse_b, parse_a);
			}
			while ((parse_a->line[parse_a->index].line[parse_a->ptr] != '\n' || parse_a->line[parse_a->index].line[parse_a->ptr + 1] != '\0') &&
				parse_a->line[parse_a->index].line[parse_a->ptr] != '\0') {

				switch (parse_a->line[parse_a->index].line[parse_a->ptr]) {
				case ' ':
				case '\t':
				case ',':
				case ':':
				case ';':
				case '*':
				case '+':
				case '-':
				case '&':
				case '|':
				case '^':
				case '[':
				case ']':
				case '(':
				case ')':
				case '<':
				case '>':
				case '=':
					copy_char(parse_b, parse_a);
					break;
				case '/':
					if (parse_a->line[parse_a->index].line[parse_a->ptr] == '/' && parse_a->line[parse_a->index].line[parse_a->ptr + 1] == '/') {
						while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\n' || parse_a->line[parse_a->index].line[parse_a->ptr + 1] != '\0')
							copy_char(parse_b, parse_a);
					}
					else
						copy_char(parse_b, parse_a);
					break;
				case '{':
					copy_char(parse_b, parse_a);
					depth++;
					break;
				case '}':
					copy_char(parse_b, parse_a);
					depth--;
					break;
				default:
					getWord(parse_a);
					if (strcmp(target_word, parse_a->word) == 0) {
						strcat_p2(parse_b, target_value);
						replacement_count++;
					}
					else if (parse_a->word[0] =='\0') { // catch errors
						debug++;
					}
					else {
						strcat_p2(parse_b, parse_a->word);
					}
					break;
				}
			}
			if ((parse_a->line[parse_a->index].line[parse_a->ptr] != '\n' || parse_a->line[parse_a->index].line[parse_a->ptr + 1] != '\0') &&
				parse_a->line[parse_a->index].line[parse_a->ptr] != '\0')
				debug++;
			copy_char(parse_b, parse_a);
		}
		parse_b->index++;
	}
	return replacement_count;
}

void clear_constants(const char* dst_name, const char* src_name, var_type_struct* var_type, parse_struct2_set* parse, UINT64 satp) {
	int debug = 0;

	VariableListEntry* VariableList = (VariableListEntry*)malloc(sizeof(VariableListEntry));
	VariableList->next = VariableList;
	VariableList->last = VariableList;

	FILE* src;
	fopen_s(&src, src_name, "r");
	//	parse_struct2 parse_1, parse_2;
	//	parse_1.line = (line_struct*)malloc(0x400 * sizeof(line_struct));
	//	parse_2.line = (line_struct*)malloc(0x400 * sizeof(line_struct));
	UINT  line_count = 0;
	while (fgets(parse->a.line[line_count++].line, 0x100, src) != NULL) {}
	fclose(src);
	line_count--;
	INT8 depth = 0;
	UINT line_number_b = 0;

	parse_struct2* parse_a = &parse->a;
	parse_struct2* parse_b = &parse->b;

	UINT updated = 0;

	//  replace global constants first, then local
	char temp[0x20];
	sprintf_s(temp, "0x%016I64x", satp);
	//	UINT line_number = 0;
	parse_a->index = 0;
	parse_b->index = 0;
	replace_word(line_count, depth, parse_a, parse_b, (char *) "satp", temp);

	for (UINT8 pass = 0; pass < 3; pass++) {
		if ((pass & 1) == 0) {
			parse_b = &parse->a;
			parse_a = &parse->b;
		}
		else {
			parse_a = &parse->a;
			parse_b = &parse->b;
		}
		for (parse_a->index = 0, parse_b->index = 0; parse_a->index < line_count; parse_a->index++) {
			parse_a->ptr = 0;
			parse_b->ptr = 0;
			parse_b->line[parse_b->index].line[0] = '\0';

			if (parse_a->index >= 191)
				debug++;

			while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
			if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{') {
				parse_a->ptr++;
				depth++;
			}
			if (parse_a->line[parse_a->index].line[parse_a->ptr] == '}') {
				parse_a->ptr++;
				depth--;
			}
			getWord(parse_a);
			if (strcmp(parse_a->word, "const") == 0) {
				if (find_variable(VariableList, parse_a, var_type, depth)) {// load up variable list
					while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
					if (parse_a->line[parse_a->index].line[parse_a->ptr] == '=') {
						parse_a->ptr++;
						while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' ||
							parse_a->line[parse_a->index].line[parse_a->ptr] == '(' || parse_a->line[parse_a->index].line[parse_a->ptr] == ')')
							parse_a->ptr++;

						getWord(parse_a);
						INT64 number1, number2;
						if (get_integer(&number1, parse_a->word)) {
							char value[0x100];
							while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' ||
								parse_a->line[parse_a->index].line[parse_a->ptr] == '(' || parse_a->line[parse_a->index].line[parse_a->ptr] == ')')
								parse_a->ptr++;
							while (parse_a->line[parse_a->index].line[parse_a->ptr] != ';') {
								while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' ||
									parse_a->line[parse_a->index].line[parse_a->ptr] == '(' || parse_a->line[parse_a->index].line[parse_a->ptr] == ')')
									parse_a->ptr++;
								if (parse_a->line[parse_a->index].line[parse_a->ptr] == '&') {
									parse_a->ptr++;
									while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' ||
										parse_a->line[parse_a->index].line[parse_a->ptr] == '(' || parse_a->line[parse_a->index].line[parse_a->ptr] == ')')
										parse_a->ptr++;

									getWord(parse_a);
									if (get_integer(&number2, parse_a->word)) {
										number1 = number1 & number2;
									}
									else {
										debug++;
									}
								}
								else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '>' && parse_a->line[parse_a->index].line[parse_a->ptr + 1] == '>') {
									parse_a->ptr += 2;
									while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' ||
										parse_a->line[parse_a->index].line[parse_a->ptr] == '(' || parse_a->line[parse_a->index].line[parse_a->ptr] == ')')
										parse_a->ptr++;

									getWord(parse_a);
									if (get_integer(&number2, parse_a->word)) {
										number1 = number1 >> number2;
									}
									else {
										debug++;
									}
								}
								else
									debug++;
							}
							sprintf_s(value, "0x%016I64x", number1);
							char* name = VariableList->last->name;
							parse_a->index++;
							updated += replace_word(line_count, depth, parse_a, parse_b, name, value);
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
				strcpy_p2(parse_b, parse_a);
				parse_b->index++;
			}
		}
		line_count = parse_b->index;
	}
	free(VariableList);

	FILE* dest;
	fopen_s(&dest, dst_name, "w");
	for (parse_a->index = 0; parse_a->index < parse_b->index; parse_a->index++) {
		strcpy_p2(parse_a, parse_b->line[parse_a->index].line);
		parse_a->ptr = 0;
		fprintf(dest, "%s", parse_b->line[parse_a->index].line);
	}
	fclose(dest);
}