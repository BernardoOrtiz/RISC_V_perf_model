// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

void simple_format(parse_struct2* parse_out, parse_struct2* parse_in) {
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
			case ' ':
			case '\t':
				while (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '\t') parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ')' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ']') parse_in->ptr++;
				copy_char(parse_out, parse_in);
				break;
			case '(':
			case '[':
				copy_char(parse_out, parse_in);
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
				break;
			case '+':
				if (parse_in->line[parse_in->index].line[parse_in->ptr - 1] != ' ' && parse_in->line[parse_in->index].line[parse_in->ptr - 1] != '\t' &&
					parse_in->line[parse_in->index].line[parse_in->ptr - 1] != '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] != '+')
					copy_ptr_char(parse_out, ' ');
				copy_char(parse_out, parse_in);
				break;
			default:
				copy_char(parse_out, parse_in);
				break;
			}
		}
	}

}

void format(const char* dst_name, const char* src_name, var_type_struct* var_type, parse_struct2_set* parse) {
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

	for (parse_in->index = 0, parse_out->index = 0; parse_in->index < line_count; parse_in->index++, parse_out->index++) {
		parse_in->ptr = 0;
		parse_out->ptr = 0;
		if (parse_out->index >= 305)
			debug++;
		parse_out->line[parse_out->index].line[0] = '\0';
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
				getWord(parse_in);
				if (parse_in->word[0]!='\0') {
					UINT hit = 0;
					for (UINT8 i = 0; i < 0x10 && !hit; i++) {
						if (strcmp(parse_in->word, var_type[i].name) == 0) {
							hit = 1;
							strcat_p2(parse_out, parse_in->word);
							while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0' && parse_in->line[parse_in->index].line[parse_in->ptr] != ';') {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
								copy_ptr_char(parse_out, ' ');
								getWord(parse_in);
								strcat_p2(parse_out, parse_in->word);
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
								switch (parse_in->line[parse_in->index].line[parse_in->ptr]) {
								case ';': // end of declaration, start new line
									// skip out
									break;
								case ',':// multiple variable definition, split to different lines
									parse_in->ptr++;
									copy_ptr_char(parse_out, ';'); // seperate out variable definition
									copy_ptr_char(parse_out, '\n');
									parse_out->index++;
									parse_out->ptr = 0;
									copy_ptr_char(parse_out, '\t');
									strcat_p2(parse_out, var_type[i].name);
									break;
								case '[':// vector definition
									copy_char(parse_out, parse_in);
									// need more formating here
									while (parse_in->line[parse_in->index].line[parse_in->ptr] != ']')copy_char(parse_out, parse_in);
									copy_char(parse_out, parse_in);
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
										// skip out
									}
									else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',') {//  split to different lines
										parse_in->ptr++;
										copy_ptr_char(parse_out, ';');// seperate out vector definition
										copy_ptr_char(parse_out, '\n');
										parse_out->index++;
										parse_out->ptr = 0;
										copy_ptr_char(parse_out, '\t');
										strcat_p2(parse_out, var_type[i].name);
									}
									else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {// skip rest of line
										simple_format(parse_out, parse_in);
									}
									else {
										debug++; // syntax
									}
									break;
								case '(':// function call definition
								case '=':// variable (or vector) initialization
								case '*':// pointer definition
									simple_format(parse_out, parse_in);
									break;
								default:
									debug++;
								}
							}
						}
					}
					if (!hit) {
						strcat_p2(parse_out, parse_in->word);
						simple_format(parse_out, parse_in);
					}
				}
				else {
					switch (parse_in->line[parse_in->index].line[parse_in->ptr]) {
					case ' ':
					case '\t':
						while (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '\t') parse_in->ptr++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ')' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ']') parse_in->ptr++;
						copy_char(parse_out, parse_in);
						break;
					case '(':
					case '[':
						copy_char(parse_out, parse_in);
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
						break;
					default:
						copy_char(parse_out, parse_in);
						break;
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
}