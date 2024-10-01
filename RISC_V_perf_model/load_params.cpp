// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

#include "internal_bus.h"

struct parse_type {
	char line[0x100], word[0x100], vector[0x100];
	UINT8 ptr;
};
struct parse_transfer_type {
	parse_type* data;
	UINT count;
};
void getWord_P(char* word, char* line, UINT8 start) {
	int debug = 0;
	UINT8 length = strlen(line);
	UINT8 i;
	while (line[start] == ' ' || line[start] == '(' || line[start] == ',')start++;
	for (i = start; i < length && debug == 0; i++) {
		if (line[i] != ' ' && line[i] != '\t' && line[i] != ',' && line[i] != '(' && line[i] != ')' && line[i] != '\0' && line[i] != '\n') {
			word[i - start] = line[i];
			word[i - start + 1] = '\0';
		}
		else {
			debug++;
		}
	}
}
UINT8 get_integer_P(INT64* number, char* input) {
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
				if (input[i] != '_') {
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
void load_params(param_type* param, const char* src_name) {
	int debug = 0;

	char word2[0x100];
	INT64 number0;
	parse_transfer_type parse_transfer2;
	parse_transfer2.data = (parse_type*)malloc(0x100 * sizeof(parse_type));
	parse_transfer2.count = 0;
	//	if (parse_transfer2.count == 0) {
	FILE* src;
	fopen_s(&src, src_name, "r");
	while (fgets(parse_transfer2.data[parse_transfer2.count++].line, 0x100, src) != NULL) {}
	parse_transfer2.count--;

	param->satp = 0;
	param->misa = 0;
	param->start_time = 0;
	param->core = 1;
	param->imul = 0;
	param->intx = 0;

	param->align = 0;
	param->cache_line = 0;
	param->decode_width = 8;
	param->fmac = 1;

	for (UINT line_number = 0; line_number < parse_transfer2.count; line_number++) {
		int ptr = 0;
		while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
		getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
		if (strcmp(word2, "MISA") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->misa = number0;
		}
		else if (strcmp(word2, "MXL") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->mxl = number0;
		}
		else if (strcmp(word2, "sv") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			if (strcmp(word2, "sv32") == 0) {
				param->misa &= ~0x10;
				param->misa |= 0x100;
				param->misa |= 0x40000000;
				param->satp |= 0x80000000;
			}
			else if (strcmp(word2, "sv39") == 0) {
				param->misa &= ~0x10;
				param->misa |= 0x100;
				param->misa |= 0x80000000;
				param->satp &= ~0xf000000000000000;
				param->satp |= 0x8000000000000000;
			}
			else if (strcmp(word2, "sv48") == 0) {
				param->misa &= ~0x10;
				param->misa |= 0x100;
				param->misa |= 0x80000000;
				param->satp &= ~0xf000000000000000;
				param->satp |= 0x9000000000000000;
			}
			else if (strcmp(word2, "sv57") == 0) {
				param->misa &= ~0x10;
				param->misa |= 0x100;
				param->misa |= 0x80000000;
				param->satp &= ~0xf000000000000000;
				param->satp |= 0xa000000000000000;
			}
			else if (strcmp(word2, "sv64") == 0) {
				param->misa &= ~0x10;
				param->misa |= 0x100;
				param->misa |= 0x80000000;
				param->satp &= ~0xf000000000000000;
				param->satp |= 0xb000000000000000;
			}
		}
		else if (strcmp(word2, "page_table_ptr") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->satp &= ~0x00000FFFffffFFFF;
			param->satp |= ((number0 >> 12) & 0x00000FFFffffFFFF);
		}
		else if (strcmp(word2, "int_size") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			//			get_integer_P(&number0, word2);
			param->misa &= ~0xc0000000;
			if (strcmp(word2, "32E") == 0) {
				param->misa |= 0x10;
				param->misa &= ~0x100;
			}
			else {
				param->misa &= ~0x10;
				param->misa |= 0x100;
				if (strcmp(word2, "32") == 0) {
					param->misa |= 0x40000000;
				}
				else if (strcmp(word2, "64") == 0) {
					param->misa |= 0x80000000;
				}
				else if (strcmp(word2, "128") == 0) {
					param->misa |= 0xC0000000;
				}
				else {
					debug++;
				}
			}
		}
		else if (strcmp(word2, "fp_size") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			param->misa &= ~0x00010008;
			if (strcmp(word2, "0") == 0) {
			}
			else if (strcmp(word2, "64") == 0) {
				param->misa |= 8;
			}
			else if (strcmp(word2, "128") == 0) {
				param->misa |= 0x00010000;
			}
			else {
				debug++;
			}
		}
		else if (strcmp(word2, "align") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			if (strcmp(word2, "128b") == 0) {
				param->align = 16;
			}
			else if (strcmp(word2, "128B") == 0) {
				param->align = 128;
			}
			else {
				debug++;
			}
		}
		else if (strcmp(word2, "cache_line") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			if (strcmp(word2, "128B") == 0) {
				param->cache_line = 128;
			}
			else {
				debug++;
			}
		}
		else if (strcmp(word2, "decode_width") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			if (strcmp(word2, "8") == 0) {
				param->decode_width = 8;
			}
			else if (strcmp(word2, "4") == 0) {
				param->decode_width = 4;
			}
			else {
				debug++;
			}
		}
		else if (strcmp(word2, "fmac") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			if (strcmp(word2, "4") == 0) {
				param->fmac = 4;
			}
			else if (strcmp(word2, "2") == 0) {
				param->fmac = 2;
			}
			else if (strcmp(word2, "1") == 0) {
				param->fmac = 1;
			}
			else {
				debug++;
			}
		}
		else if (strcmp(word2, "start_time") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->start_time = number0;
		}
		else if (strcmp(word2, "stop_time") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->stop_time = number0;
		}
		else if (strcmp(word2, "unit") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->core = number0;
		}
		else if (strcmp(word2, "prefetcher") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->prefetcher = number0;
		}
		else if (strcmp(word2, "decoder") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->decoder = number0;
		}
		else if (strcmp(word2, "allocator") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->allocator = number0;
		}
		else if (strcmp(word2, "store_bus") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->store_bus = number0;
		}
		else if (strcmp(word2, "store") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->store = number0;
		}
		else if (strcmp(word2, "load_bus") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->load_bus = number0;
		}
		else if (strcmp(word2, "load") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->load = number0;
		}
		else if (strcmp(word2, "csr") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->csr = number0;
		}
		else if (strcmp(word2, "int") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->intx = number0;
		}
		else if (strcmp(word2, "imul") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->imul = number0;
		}
		else if (strcmp(word2, "fp") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->fp = number0;
		}
		else if (strcmp(word2, "branch") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->branch = number0;
		}
		else if (strcmp(word2, "caches") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->caches = number0;
		}
		else if (strcmp(word2, "L0C") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->L0C = number0;
		}
		else if (strcmp(word2, "L0D") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->L0D = number0;
		}
		else if (strcmp(word2, "L2") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->L2 = number0;
		}
		else if (strcmp(word2, "L3") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->L3 = number0;
		}
		else if (strcmp(word2, "MEM") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->mem = number0;
		}
		else if (strcmp(word2, "TLB") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->TLB = number0;
		}
		else if (strcmp(word2, "PAGE_WALK") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->PAGE_WALK = number0;
		}
		else if (strcmp(word2, "PAGE_FAULT") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->PAGE_FAULT = number0;
		}
		else if (strcmp(word2, "EXT_SNOOP") == 0) {
			ptr += strlen(word2); while (parse_transfer2.data[line_number].line[ptr] == ' ' || parse_transfer2.data[line_number].line[ptr] == '\t')ptr++;
			getWord_P(word2, parse_transfer2.data[line_number].line, ptr);
			get_integer_P(&number0, word2);
			param->EXT_SNOOP = number0;
		}
		else {
			debug++;
		}
	}
	fclose(src);
	free(parse_transfer2.data);
}