// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

enum condition_type {
	cond_false = 0,
	cond_true = 1
};
UINT8 get_integer2c(INT64* number, char* input) {
	int debug = 0;
	UINT8 result = 1;
	//	int number;
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
	if (!hex && strlen(input) > 0) {
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
void force_execute_branch_code(parse_struct2* parse_b, parse_struct2* parse_a, int* depth) {
	int debug = 0;
	while (depth[0] > 0) {
		parse_a->index++;
		parse_a->ptr = 0;
		while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0' && depth[0] > 0) {
			if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{') {
				depth[0]++;
			}
			else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '}') {
				depth[0]--;
			}
			parse_a->ptr++;
		}
		if (depth[0] != 0) {
			strcpy_p2(parse_b, parse_a);
			parse_b->index++;
		}
	} // cull tail end of if..else if(...else) statements
	while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
	getWord(parse_a);
	if (strcmp(parse_a->word, "else") == 0) {
		while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0') {
			if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{') {
				depth[0]++;
			}
			else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '}') {
				depth[0]--;
			}
			parse_a->ptr++;
		}
		parse_a->index++;
		while (depth[0] > 0) {
			while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0') {
				if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{') {
					depth[0]++;
				}
				else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '}') {
					depth[0]--;
				}
				parse_a->ptr++;
			}
			if (depth[0] > 0) {
				parse_a->index++;
				parse_a->ptr = 0;
			}
		}
		debug++;
	}
}
/**/
void cull_branches(const char* dst_name, parse_struct2_set* parse) {
	int debug = 0;

//	FILE* src = fopen(src_name, "r");
//	parse_type* parse_1 = (parse_type*)malloc(0x400 * sizeof(parse_type));
//	parse_type* parse_2 = (parse_type*)malloc(0x400 * sizeof(parse_type));
//	UINT  line_count = 0;

//	while (fgets(parse_1[line_count++].line, 0x100, src) != NULL) {}
//	line_count--;

	int depth = 0;
//	fclose(src);

	parse_struct2* parse_a = &parse->a;
	parse_struct2* parse_b = &parse->b;

	UINT8 pass = 0;
	UINT8 touched = 1;
	int line_count = parse->a.index;
//	while (touched && pass < 0x10) {

	while (touched && pass < 2) {
		//	for (UINT8 pass = 0; pass < 1; pass++) {
		if ((pass & 1) == 0) {
			parse_a = &parse->a;
			parse_b = &parse->b;
		}
		else {
			parse_b = &parse->a;
			parse_a = &parse->b;
		}
		touched = 0;
		pass++;
		parse_b->index = 0;
		for (parse_a->index = 0; parse_a->index < line_count; parse_a->index++) {
			parse_a->ptr = 0;
			parse_b->ptr = 0;
			parse_b->line[parse_b->index].line[0] = '\0';

			if (parse_a->index == 30)
				debug++;

			while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
			getWord(parse_a);
			UINT8 hit = 0;
			UINT8 skip = 0;
			skip = 0;
			if (strcmp(parse_a->word, "else") == 0) {
				while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
				getWord(parse_a);
			}
			if (strcmp(parse_a->word, "if") == 0) { // detect conditional statement
				if (parse_a->index >= 30)
					debug++;

				while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
				if (parse_a->line[parse_a->index].line[parse_a->ptr] == '(')
					parse_a->ptr++;
				else
					debug++; // parsing error, requires a condition code
				getWord(parse_a);
				INT64 number1, number2;
				if (get_integer2c(&number1, parse_a->word)) { // first variable
					hit = 1;
				}
				while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
				if (parse_a->line[parse_a->index].line[parse_a->ptr] == '[') {
					while (parse_a->line[parse_a->index].line[parse_a->ptr] != ']')parse_a->ptr++;
					parse_a->ptr++;
					while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
				}
				// get conditional
				condition_type condition = cond_false;
				if (hit) {
					switch (parse_a->line[parse_a->index].line[parse_a->ptr]) {
					case ')':
						if (number1 == 0)
							condition = cond_false;
						else
							condition = cond_true;
						break;
					case '=':
						if (parse_a->line[parse_a->index].line[parse_a->ptr] == '=' && parse_a->line[parse_a->index].line[parse_a->ptr + 1] == '=') { // condition
							parse_a->ptr += 2;
							while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
							getWord(parse_a);
							if (get_integer2c(&number2, parse_a->word)) { // first variable
								if (number1 == number2)
									condition = cond_true;
								else
									condition = cond_false;
							}
							else {
								hit = 0;
							}
						}
						else {
							debug++;
						}
						break;
					default:
						debug++;
						hit = 0;
						break;
					}
				}
				if (hit) {
					touched = 1;
					while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
					if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{') {
						parse_a->ptr++;
						depth++;
					}
					if (condition == cond_true) {

						UINT stop = 0;
						int depth2 = 0;
						parse_a->index++;
						while (parse_a->index < line_count && !stop) {
							parse_a->ptr = 0;
							parse_b->ptr = 0;
							parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
							while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0' && !stop) {
								if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{')
									depth2++;
								else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '}')
									depth2--;
								if (depth2 < 0)stop = 1;
								parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
								parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
							}
							if (!stop) {
								parse_a->index++;
								parse_b->index++;
							}
							else {
								parse_b->ptr = 0;
								parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
							}
						}
						while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
						getWord(parse_a);
						if (strcmp(parse_a->word, "else") == 0) {
							parse_a->index++;
							stop = 0;
							depth2 = 0;
							while (parse_a->index < line_count && !stop) {
								parse_a->ptr = 0;
								while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0' && !stop) {
									if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{')
										depth2++;
									else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '}')
										depth2--;
									if (depth2 < 0)stop = 1;
									parse_a->ptr++;
								}
								if (!stop)
									parse_a->index++;
							}
						}
						else {
							//							parse_a->index++;
						}
					}
					else {// condition false
						debug++;
						parse_a->index++;
						UINT8 stop = 0;
						int depth2 = 0;
						while (parse_a->index < line_count && !stop) {
							parse_a->ptr = 0;
							while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0' && !stop) {
								if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{')
									depth2++;
								else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '}')
									depth2--;
								if (depth2 < 0)stop = 1;
								parse_a->ptr++;
							}
							if (!stop)
								parse_a->index++;
						}
						while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
						if (parse_a->line[parse_a->index].line[parse_a->ptr] == '\n' && parse_a->line[parse_a->index].line[parse_a->ptr+1] == '\0') {
//							parse_a->index++;
							parse_a->ptr = 0;
							while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
						}
						getWord(parse_a);
						if (strcmp(parse_a->word, "else") == 0) {
							while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
							if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{') {
								stop = 0;
								depth2 = 0;
								parse_a->index++;
								while (parse_a->index < line_count && !stop) {
									parse_a->ptr = 0;
									parse_b->ptr = 0;
									parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
									while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0' && !stop) {
										if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{')
											depth2++;
										else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '}')
											depth2--;
										if (depth2 < 0)stop = 1;
										parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
										parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
									}
									if (!stop) {
										parse_a->index++;
										parse_b->index++;
									}
									else {
										parse_b->ptr = 0;
										parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
									}
								}
							}
							else {
								parse_b->ptr = 0;
								parse_b->line[parse_b->index].line[parse_b->ptr++] = '\t';
								while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0')parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
								parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
								parse_b->index++;
								skip = 1;
							}
						}
					}
					if (!skip) {
						parse_a->index++;
						while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
						getWord(parse_a);
						hit = 0;
					}
				}
			}
			if (!hit) {
				strcpy_p2(parse_b, parse_a);
				parse_b->index++;
			}
		}
		line_count = parse_b->index;
	}
	FILE* dest;
	fopen_s(&dest, dst_name, "w");

	for (parse_a->index = 0; parse_a->index <= line_count; parse_a->index++) {
		strcpy_p2(parse_a, parse_b->line[parse_a->index].line);
		fprintf(dest, "%s", parse_b->line[parse_a->index].line);
	}
	fclose(dest);
}