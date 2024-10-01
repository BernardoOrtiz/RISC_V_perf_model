// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

void find_next_op(parse_struct2_set* parse) {
	while (parse->a.line[parse->a.index].line[parse->a.ptr] != '\0' && parse->a.line[parse->a.index].line[parse->a.ptr] != '{' &&
		parse->a.line[parse->a.index].line[parse->a.ptr] != '*' &&
		parse->a.line[parse->a.index].line[parse->a.ptr] != '+' && parse->a.line[parse->a.index].line[parse->a.ptr] != '-') {
		if (parse->a.line[parse->a.index].line[parse->a.ptr] == '[') {
			int depth = 0;
			while (parse->a.line[parse->a.index].line[parse->a.ptr] != ']' || depth > 1) {
				if (parse->a.line[parse->a.index].line[parse->a.ptr] == '[')
					depth++;
				else if (parse->a.line[parse->a.index].line[parse->a.ptr] == ']')
					depth--;
				copy_char(&parse->b, &parse->a);
			}
		}
		else if (parse->a.line[parse->a.index].line[parse->a.ptr] == '(') {
			int depth = 0;
			while (parse->a.line[parse->a.index].line[parse->a.ptr] != ')' || depth > 1) {
				if (parse->a.line[parse->a.index].line[parse->a.ptr] == '(')
					depth++;
				else if (parse->a.line[parse->a.index].line[parse->a.ptr] == ')')
					depth--;
				copy_char(&parse->b, &parse->a);
			}
		}
		copy_char(&parse->b, &parse->a);
	}
}

void parse_body(parse_struct2_set* parse, char* header) {
	int debug = 0;
	while (parse->a.line[parse->a.index].line[parse->a.ptr] == ' ' || parse->a.line[parse->a.index].line[parse->a.ptr] == '\t') copy_char(&parse->b, &parse->a);

	switch (parse->a.line[parse->a.index].line[parse->a.ptr]) {
	case '=':
		find_next_op(parse);
		if (parse->a.line[parse->a.index].line[parse->a.ptr] == '{') {
			while (parse->a.line[parse->a.index].line[parse->a.ptr] != '\0')copy_char(&parse->b, &parse->a);
		}
		if (parse->a.line[parse->a.index].line[parse->a.ptr] == '\0')
			parse->b.index++;
		else {
			copy_char(&parse->b, &parse->a);
			find_next_op(parse);
			if (parse->a.line[parse->a.index].line[parse->a.ptr] == '\0')
				parse->b.index++;
			else if (parse->a.line[parse->a.index].line[parse->a.ptr] == '*') {
				copy_char(&parse->b, &parse->a);
				while (parse->a.line[parse->a.index].line[parse->a.ptr] != '\0' &&
					parse->a.line[parse->a.index].line[parse->a.ptr] != '+' && parse->a.line[parse->a.index].line[parse->a.ptr] != '-')
					copy_char(&parse->b, &parse->a);
				if (parse->a.line[parse->a.index].line[parse->a.ptr] != '\0')
					debug++;
				else
					parse->b.index++;
			}
			else {
				parse->b.line[parse->b.index].line[parse->b.ptr++] = ';';
				parse->b.line[parse->b.index].line[parse->b.ptr++] = '\n';
				parse->b.line[parse->b.index++].line[parse->b.ptr] = '\0';
				while ((parse->a.line[parse->a.index].line[parse->a.ptr] != '\0') && (parse->a.ptr < strlen(parse->a.line[parse->a.index].line))) {
					strcpy_p2(&parse->b, header);
					parse->b.ptr = strlen(parse->b.line[parse->b.index].line);
					char op_latch = parse->a.line[parse->a.index].line[parse->a.ptr];
					copy_char(&parse->b, &parse->a);
					parse->b.line[parse->b.index].line[parse->b.ptr++] = '=';
					parse->b.line[parse->b.index].line[parse->b.ptr++] = ' ';
					find_next_op(parse);
					if ((op_latch == '+' || op_latch == '-') && parse->a.line[parse->a.index].line[parse->a.ptr] == '*') {
						copy_char(&parse->b, &parse->a);
						find_next_op(parse);
					}
					if ((parse->a.line[parse->a.index].line[parse->a.ptr] != '\0')) {
						parse->b.line[parse->b.index].line[parse->b.ptr++] = ';';
						parse->b.line[parse->b.index].line[parse->b.ptr++] = '\n';
						parse->b.line[parse->b.index++].line[parse->b.ptr] = '\0';
					}
					else {
						parse->b.index++;
					}
				}
			}
		}
		break;
	case '+':
		debug++;
		strcpy_p2(&parse->b, &parse->a);
		parse->b.index++;
		break;
	case '-':
		debug++;
		strcpy_p2(&parse->b, &parse->a);
		parse->b.index++;
		break;
	case '*':
		debug++;
		strcpy_p2(&parse->b, &parse->a);
		parse->b.index++;
		break;
	default:
		strcpy_p2(&parse->b, &parse->a);
		parse->b.index++;
		break;
	}
}
char match_var_type(short* type, char* word, var_type_struct* var_type) {
	char hit = 0;
	for (UINT i = 0; i < var_type_count && hit == 0; i++) {
		if (strcmp(word, var_type[i].name) == 0) {
			type[0] = i;
			hit = 1;
		}
	}
	return hit;
}
void split_line(const char* dst_name, const char* src_name, parse_struct2_set* parse, var_type_struct* var_type) {
	int debug = 0;
	FILE* src;
	fopen_s(&src, src_name, "r");

	UINT  line_count = 0;
	while (fgets(parse->a.line[line_count++].line, 0x200, src) != NULL) {}
	line_count--;
	fclose(src);
	for (parse->a.index = 0, parse->b.index = 0; parse->a.index < line_count; parse->a.index++) {// "( x" => "(x" also "x )" => "x)"
		parse->a.ptr = 0;									// elliminate blank spaces for easier parseing
		parse->b.ptr = 0;
		while (parse->a.line[parse->a.index].line[parse->a.ptr] == ' ' || parse->a.line[parse->a.index].line[parse->a.ptr] == '\t') copy_char(&parse->b, &parse->a);

		if (parse->a.index >= 98)
			debug++;
		getWord(&parse->a);
		short type;
		if (match_var_type(&type, parse->a.word, var_type)) {
			while (parse->a.line[parse->a.index].line[parse->a.ptr] == ' ' || parse->a.line[parse->a.index].line[parse->a.ptr] == '\t') copy_char(&parse->b, &parse->a);

			getWord(&parse->a);
			if (parse->a.word[0]!='\0') {
				char header[0x100];
				sprintf_s(header, "%s%s", parse->b.line[parse->b.index].line, parse->a.word);
				strcat_p2(&parse->b, var_type[type].name);
				strcat_p2(&parse->b, (char*)" ");
				strcat_p2(&parse->b, parse->a.word);
				parse->b.ptr = strlen(parse->b.line[parse->b.index].line);
				if (parse->a.line[parse->a.index].line[parse->a.ptr] == '[') {
					int depth = 0;
					while (parse->a.line[parse->a.index].line[parse->a.ptr] != ']' && depth < 1) {
						if (parse->a.line[parse->a.index].line[parse->a.ptr] == '[')
							depth++;
						else if (parse->a.line[parse->a.index].line[parse->a.ptr] == ']')
							depth--;
						copy_char(&parse->b, &parse->a);
					}
					copy_char(&parse->b, &parse->a);
				}

				parse_body(parse, header);
			}
			else {
				strcpy_p2(&parse->b, &parse->a);
				parse->b.index++;
			}
		}
		else {
			if (parse->a.word[0] != '\0') {
				strcat_p2(&parse->b, parse->a.word);
				char header[0x100];
				sprintf_s(header,"%s", parse->b.line[parse->b.index].line);
				parse->b.ptr = strlen(parse->b.line[parse->b.index].line);

				parse_body(parse, header);
			}
			else {
				strcpy_p2(&parse->b, &parse->a);
				parse->b.index++;
			}
		}
	}

	FILE* dest;
	fopen_s(&dest, dst_name, "w");
	for (UINT i = 0; i < parse->b.index; i++) {
		fprintf(dest, "%s", parse->b.line[i].line);
	}
	fclose(dest);
}