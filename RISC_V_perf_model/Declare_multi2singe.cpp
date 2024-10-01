// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

void declare_multi2single(parse_struct2* parse_out, parse_struct2* parse_in, const char* dst_name, const char* src_name) {
	int debug = 0;
	FILE* src;
	fopen_s(&src, src_name, "r");
//	FILE* dest = fopen(dst_name, "w");
	parse_struct2 parse_tabs;
	parse_tabs.line = (line_struct*)malloc(sizeof(line_struct));
	parse_in->index = 0;
	parse_out->index = 0;
	parse_tabs.index = 0;

	UINT  line_count = 0;
	while (fgets(parse_in->line[line_count++].line, 0x200, src) != NULL) {}
	line_count--;
	fclose(src);
	for (parse_in->index = 0; parse_in->index < line_count; parse_in->index++, parse_out->index++) {
		parse_in->ptr = 0;
		parse_out->ptr = 0;
		parse_tabs.ptr = 0;
		if (parse_in->index >= 1976)
			debug++;
		while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') {
				parse_tabs.line[0].line[parse_tabs.ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr];
				parse_tabs.line[0].line[parse_tabs.ptr] = '\0';
				copy_char(parse_out, parse_in);
			}
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
					copy_char(parse_out, parse_in);
			}
			else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '*') {
				copy_char(parse_out, parse_in);
				copy_char(parse_out, parse_in);
				while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0' ||
					(parse_in->line[parse_in->index].line[parse_in->ptr] == '*' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/'))
					copy_char(parse_out, parse_in);
				if (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
					copy_char(parse_out, parse_in);
					copy_char(parse_out, parse_in);
				}
			}
			else {
				getWord(parse_in);
				if (strcmp(parse_in->word, "int") == 0) {
					strcat_p2(parse_out, parse_in->word);
					while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
						while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0' && parse_in->line[parse_in->index].line[parse_in->ptr] != ',' &&
							!(parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/')) {
							copy_char(parse_out, parse_in);
						}
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',') {
							copy_ptr_char(parse_out, ';');
							copy_ptr_char(parse_out, '\n');
							parse_out->index++;
							parse_in->ptr++;
							strcpy_p2(parse_out, parse_tabs.line[0].line);
							parse_out->ptr = parse_tabs.ptr;
							strcat_p2(parse_out, parse_in->word);
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/') {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
								copy_char(parse_out, parse_in);
						}
					}
				}
				else {
					strcpy_p2(parse_out, parse_in);
				}
			}
		}
	}
	FILE* dest;
	fopen_s(&dest, dst_name, "w");
	for (UINT i = 0; i < parse_out->index; i++) {
		fprintf(dest, "%s", parse_out->line[i].line);
	}
	fclose(dest);
	//	free(parse_in->line);
	free(parse_tabs.line);
}