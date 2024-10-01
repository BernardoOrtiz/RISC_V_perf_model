// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

void sprint_addr_p2(parse_struct2* parse_out, INT64 addr, param_type* param) {
	UINT8 debug = 0;
	char buffer[0x20];
	switch (param->satp >> 60 & 0x0f) {
	case 8://39
		sprintf(buffer, "0x%010I64x ", addr);
		for (UINT i = 0; i < 12; i++)
			parse_out->line[parse_out->index].line[parse_out->ptr++] = buffer[i];
		break;
	case 9://48
		//		fprintf(debug_stream, "0x%012I64x, ", addr);
		sprintf(buffer, "0x%04x_%08x ", addr >> 32, addr & 0x0000ffffffff);
		for (UINT i = 0; i < 15; i++)
			parse_out->line[parse_out->index].line[parse_out->ptr++] = buffer[i];
		break;
	case 0x0a://57
		sprintf(buffer, "0x%015I64x ", addr);
		for (UINT i = 0; i < 17; i++)
			parse_out->line[parse_out->index].line[parse_out->ptr++] = buffer[i];
		break;
	case 0x0b://64
		sprintf(buffer, "0x%016I64x ", addr);
		for (UINT i = 0; i < 18; i++)
			parse_out->line[parse_out->index].line[parse_out->ptr++] = buffer[i];
		break;
	default:
		debug++;
		break;
	}
	parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
}
void get_wordP(parse_type* entry) {
	UINT16 length = strlen(entry->line);
	UINT8 i = 0;
	while (entry->line[entry->ptr] == ' ' || entry->line[entry->ptr] == '\t')entry->ptr++;
	UINT8 stop = 0;
	entry->word[i] = '\0';
	while (entry->ptr < length && stop == 0) {
		if (!(entry->line[entry->ptr] == ' ' || entry->line[entry->ptr] == '\t' || entry->line[entry->ptr] == '\n' || entry->line[entry->ptr] == '\0')) {
			entry->word[i++] = entry->line[entry->ptr++];
			entry->word[i] = '\0';
		}
		else {
			stop = 1;
		}
	}
}
void getWordB(char* word, char* line, UINT8 start) {
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
UINT8 get_csr_numberB(UINT16* number, char* word) {
	UINT8 result = 1;
	if (strcmp(word, "uepc") == 0)	number[0] = 0x041;
	else if (strcmp(word, "mepc") == 0)	number[0] = 0x341;
	else if (strcmp(word, "sstatus") == 0)	number[0] = 0x100;
	else if (strcmp(word, "sedeleg") == 0)	number[0] = 0x102;
	else if (strcmp(word, "sideleg") == 0)	number[0] = 0x103;
	else if (strcmp(word, "stvec") == 0)	number[0] = 0x105;
	else if (strcmp(word, "scause") == 0)	number[0] = 0x142;
	else if (strcmp(word, "stval") == 0)	number[0] = 0x143;
	else if (strcmp(word, "satp") == 0)	number[0] = 0x180;
	else if (strcmp(word, "mstatus") == 0)	number[0] = 0x300;
	else if (strcmp(word, "medeleg") == 0)	number[0] = 0x302;
	else if (strcmp(word, "mideleg") == 0)	number[0] = 0x303;
	else if (strcmp(word, "mie") == 0)	number[0] = 0x304;
	else if ((strcmp(word, "mtvec") == 0))	number[0] = 0x305;
	else if (strcmp(word, "mcause") == 0)	number[0] = 0x342;
	else if ((strcmp(word, "mtval") == 0))	number[0] = 0x343;
	else if (strcmp(word, "mscratch") == 0)	number[0] = 0x340;
	else if (strcmp(word, "sscratch") == 0)	number[0] = 0x140;
	else if (strcmp(word, "mhartid") == 0)	number[0] = 0xf14;
	else if (strcmp(word, "iobase") == 0)	number[0] = 0x7c0;
	else if (strcmp(word, "mbound") == 0)	number[0] = 0x381;
	else if (strcmp(word, "sbound") == 0)	number[0] = 0x181;
	else if (strcmp(word, "ssp") == 0)	number[0] = 0x9c1;
	else if (strcmp(word, "msp") == 0)	number[0] = 0xbc1;
	else result = 0;
	return result;
}
UINT8 get_reg_number_UABI(UINT8* number, char* word) {//UABI
	UINT8 result = 1;
	if (strcmp(word, "zero") == 0)			number[0] = 0;
	else if (strcmp(word, "ra") == 0)		number[0] = 1;
	else if (strcmp(word, "sp") == 0)		number[0] = 2;
	else if (strcmp(word, "gp") == 0)		number[0] = 3;
	else if (strcmp(word, "tp") == 0)		number[0] = 4;
	else if (strcmp(word, "t0") == 0)		number[0] = 5;
	else if (strcmp(word, "t1") == 0)		number[0] = 6;
	else if (strcmp(word, "t2") == 0)		number[0] = 7;
	else if (strcmp(word, "s0") == 0)		number[0] = 8;
	else if (strcmp(word, "s1") == 0)		number[0] = 9;
	else if (strcmp(word, "a0") == 0)		number[0] = 10;
	else if (strcmp(word, "a1") == 0)		number[0] = 11;
	else if (strcmp(word, "a2") == 0)		number[0] = 12;
	else if (strcmp(word, "a3") == 0)		number[0] = 13;
	else if (strcmp(word, "a4") == 0)		number[0] = 14;
	else if (strcmp(word, "a5") == 0)		number[0] = 15;
	else if (strcmp(word, "a6") == 0)		number[0] = 16;
	else if (strcmp(word, "a7") == 0)		number[0] = 17;
	else if (strcmp(word, "s2") == 0)		number[0] = 18;
	else if (strcmp(word, "s3") == 0)		number[0] = 19;
	else if (strcmp(word, "s4") == 0)		number[0] = 20;
	else if (strcmp(word, "s5") == 0)		number[0] = 21;
	else if (strcmp(word, "s6") == 0)		number[0] = 22;
	else if (strcmp(word, "s7") == 0)		number[0] = 23;
	else if (strcmp(word, "s8") == 0)		number[0] = 24;
	else if (strcmp(word, "s9") == 0)		number[0] = 25;
	else if (strcmp(word, "s10") == 0)		number[0] = 26;
	else if (strcmp(word, "s11") == 0)		number[0] = 27;
	else if (strcmp(word, "t3") == 0)		number[0] = 28;
	else if (strcmp(word, "t4") == 0)		number[0] = 29;
	else if (strcmp(word, "t5") == 0)		number[0] = 30;
	else if (strcmp(word, "t6") == 0)		number[0] = 31;
	else if (strcmp(word, "x0") == 0)		number[0] = 0;
	else if (strcmp(word, "x1") == 0)		number[0] = 1;
	else if (strcmp(word, "x2") == 0)		number[0] = 2;
	else if (strcmp(word, "x3") == 0)		number[0] = 3;
	else if (strcmp(word, "x4") == 0)		number[0] = 4;
	else if (strcmp(word, "x5") == 0)		number[0] = 5;
	else if (strcmp(word, "x6") == 0)		number[0] = 6;
	else if (strcmp(word, "x7") == 0)		number[0] = 7;
	else if (strcmp(word, "x8") == 0)		number[0] = 8;
	else if (strcmp(word, "x9") == 0)		number[0] = 9;
	else if (strcmp(word, "x10") == 0)		number[0] = 10;
	else if (strcmp(word, "x11") == 0)		number[0] = 11;
	else if (strcmp(word, "x12") == 0)		number[0] = 12;
	else if (strcmp(word, "x13") == 0)		number[0] = 13;
	else if (strcmp(word, "x14") == 0)		number[0] = 14;
	else if (strcmp(word, "x15") == 0)		number[0] = 15;
	else if (strcmp(word, "x16") == 0)		number[0] = 16;
	else if (strcmp(word, "x17") == 0)		number[0] = 17;
	else if (strcmp(word, "x18") == 0)		number[0] = 18;
	else if (strcmp(word, "x19") == 0)		number[0] = 19;
	else if (strcmp(word, "x20") == 0)		number[0] = 20;
	else if (strcmp(word, "x21") == 0)		number[0] = 21;
	else if (strcmp(word, "x22") == 0)		number[0] = 22;
	else if (strcmp(word, "x23") == 0)		number[0] = 23;
	else if (strcmp(word, "x24") == 0)		number[0] = 24;
	else if (strcmp(word, "x25") == 0)		number[0] = 25;
	else if (strcmp(word, "x26") == 0)		number[0] = 26;
	else if (strcmp(word, "x27") == 0)		number[0] = 27;
	else if (strcmp(word, "x28") == 0)		number[0] = 28;
	else if (strcmp(word, "x29") == 0)		number[0] = 29;
	else if (strcmp(word, "x30") == 0)		number[0] = 30;
	else if (strcmp(word, "x31") == 0)		number[0] = 31;
	else result = 0;
	return result;
}
UINT8 get_freg_numberB(UINT8* number, char* word) {
	UINT8 result = 1;
	if (strcmp(word, "ft0") == 0)			number[0] = 0;
	else if (strcmp(word, "ft1") == 0)		number[0] = 1;
	else if (strcmp(word, "ft2") == 0)		number[0] = 2;
	else if (strcmp(word, "ft3") == 0)		number[0] = 3;
	else if (strcmp(word, "ft4") == 0)		number[0] = 4;
	else if (strcmp(word, "ft5") == 0)		number[0] = 5;
	else if (strcmp(word, "ft6") == 0)		number[0] = 6;
	else if (strcmp(word, "ft7") == 0)		number[0] = 7;
	else if (strcmp(word, "fs0") == 0)		number[0] = 8;
	else if (strcmp(word, "fs1") == 0)		number[0] = 9;
	else if (strcmp(word, "a0") == 0)		number[0] = 10;
	else if (strcmp(word, "fa1") == 0)		number[0] = 11;
	else if (strcmp(word, "fa2") == 0)		number[0] = 12;
	else if (strcmp(word, "fa3") == 0)		number[0] = 13;
	else if (strcmp(word, "fa4") == 0)		number[0] = 14;
	else if (strcmp(word, "fa5") == 0)		number[0] = 15;
	else if (strcmp(word, "fa6") == 0)		number[0] = 16;
	else if (strcmp(word, "fa7") == 0)		number[0] = 17;
	else if (strcmp(word, "fs2") == 0)		number[0] = 18;
	else if (strcmp(word, "fs3") == 0)		number[0] = 19;
	else if (strcmp(word, "fs4") == 0)		number[0] = 20;
	else if (strcmp(word, "fs5") == 0)		number[0] = 21;
	else if (strcmp(word, "fs6") == 0)		number[0] = 22;
	else if (strcmp(word, "fs7") == 0)		number[0] = 23;
	else if (strcmp(word, "fs8") == 0)		number[0] = 24;
	else if (strcmp(word, "fs9") == 0)		number[0] = 25;
	else if (strcmp(word, "fs10") == 0)		number[0] = 26;
	else if (strcmp(word, "fs11") == 0)		number[0] = 27;
	else if (strcmp(word, "ft8") == 0)		number[0] = 28;
	else if (strcmp(word, "ft9") == 0)		number[0] = 29;
	else if (strcmp(word, "ft10") == 0)		number[0] = 30;
	else if (strcmp(word, "ft11") == 0)		number[0] = 31;
	else if (strcmp(word, "f0") == 0 || strcmp(word, "f00") == 0)		number[0] = 0;
	else if (strcmp(word, "f1") == 0 || strcmp(word, "f01") == 0)		number[0] = 1;
	else if (strcmp(word, "f2") == 0 || strcmp(word, "f02") == 0)		number[0] = 2;
	else if (strcmp(word, "f3") == 0 || strcmp(word, "f03") == 0)		number[0] = 3;
	else if (strcmp(word, "f4") == 0 || strcmp(word, "f04") == 0)		number[0] = 4;
	else if (strcmp(word, "f5") == 0 || strcmp(word, "f05") == 0)		number[0] = 5;
	else if (strcmp(word, "f6") == 0 || strcmp(word, "f06") == 0)		number[0] = 6;
	else if (strcmp(word, "f7") == 0 || strcmp(word, "f07") == 0)		number[0] = 7;
	else if (strcmp(word, "f8") == 0 || strcmp(word, "f08") == 0)		number[0] = 8;
	else if (strcmp(word, "f9") == 0 || strcmp(word, "f09") == 0)		number[0] = 9;
	else if (strcmp(word, "f10") == 0)		number[0] = 10;
	else if (strcmp(word, "f11") == 0)		number[0] = 11;
	else if (strcmp(word, "f12") == 0)		number[0] = 12;
	else if (strcmp(word, "f13") == 0)		number[0] = 13;
	else if (strcmp(word, "f14") == 0)		number[0] = 14;
	else if (strcmp(word, "f15") == 0)		number[0] = 15;
	else if (strcmp(word, "f16") == 0)		number[0] = 16;
	else if (strcmp(word, "f17") == 0)		number[0] = 17;
	else if (strcmp(word, "f18") == 0)		number[0] = 18;
	else if (strcmp(word, "f19") == 0)		number[0] = 19;
	else if (strcmp(word, "f20") == 0)		number[0] = 20;
	else if (strcmp(word, "f21") == 0)		number[0] = 21;
	else if (strcmp(word, "f22") == 0)		number[0] = 22;
	else if (strcmp(word, "f23") == 0)		number[0] = 23;
	else if (strcmp(word, "f24") == 0)		number[0] = 24;
	else if (strcmp(word, "f25") == 0)		number[0] = 25;
	else if (strcmp(word, "f26") == 0)		number[0] = 26;
	else if (strcmp(word, "f27") == 0)		number[0] = 27;
	else if (strcmp(word, "f28") == 0)		number[0] = 28;
	else if (strcmp(word, "f29") == 0)		number[0] = 29;
	else if (strcmp(word, "f30") == 0)		number[0] = 30;
	else if (strcmp(word, "f31") == 0)		number[0] = 31;
	else result = 0;
	return result;
}

UINT8 get_reg_number_EABI(UINT8* number, char* word) {//EABI
	UINT8 result = 1;
	if (strcmp(word, "zero") == 0)										number[0] = 0;
	else if (strcmp(word, "ra") == 0)									number[0] = 1;
	else if (strcmp(word, "sp") == 0)									number[0] = 2;
	else if (strcmp(word, "gp") == 0)									number[0] = 3;
	else if (strcmp(word, "tp") == 0)									number[0] = 4;
	else if (strcmp(word, "t0") == 0 || strcmp(word, "t00") == 0)		number[0] = 5;
	else if (strcmp(word, "s3") == 0 || strcmp(word, "s03") == 0)		number[0] = 6;
	else if (strcmp(word, "s4") == 0 || strcmp(word, "s04") == 0)		number[0] = 7;
	else if (strcmp(word, "s0") == 0 || strcmp(word, "s00") == 0)		number[0] = 8;
	else if (strcmp(word, "s1") == 0 || strcmp(word, "s01") == 0)		number[0] = 9;
	else if (strcmp(word, "a0") == 0)									number[0] = 10;
	else if (strcmp(word, "a1") == 0)									number[0] = 11;
	else if (strcmp(word, "a2") == 0)									number[0] = 12;
	else if (strcmp(word, "a3") == 0)									number[0] = 13;
	else if (strcmp(word, "s2") == 0 || strcmp(word, "s02") == 0)		number[0] = 14;
	else if (strcmp(word, "t1") == 0 || strcmp(word, "t01") == 0)		number[0] = 15;
	else if (strcmp(word, "s5") == 0 || strcmp(word, "s05") == 0)		number[0] = 16;

	else if (strcmp(word, "s6") == 0 || strcmp(word, "s06") == 0)		number[0] = 17;
	else if (strcmp(word, "s7") == 0 || strcmp(word, "s07") == 0)		number[0] = 18;
	else if (strcmp(word, "s8") == 0 || strcmp(word, "s08") == 0)		number[0] = 19;
	else if (strcmp(word, "s9") == 0 || strcmp(word, "s09") == 0)		number[0] = 20;
	else if (strcmp(word, "s10") == 0)									number[0] = 21;
	else if (strcmp(word, "s11") == 0)									number[0] = 22;
	else if (strcmp(word, "s12") == 0)									number[0] = 23;
	else if (strcmp(word, "s13") == 0)									number[0] = 24;
	else if (strcmp(word, "s14") == 0)									number[0] = 25;
	else if (strcmp(word, "s15") == 0)									number[0] = 26;
	else if (strcmp(word, "s16") == 0)									number[0] = 27;
	else if (strcmp(word, "s17") == 0)									number[0] = 28;
	else if (strcmp(word, "s18") == 0)									number[0] = 29;
	else if (strcmp(word, "s19") == 0)									number[0] = 30;
	else if (strcmp(word, "s20") == 0)									number[0] = 31;

	else if (strcmp(word, "x0") == 0 || strcmp(word, "x00") == 0)		number[0] = 0;
	else if (strcmp(word, "x1") == 0 || strcmp(word, "x01") == 0)		number[0] = 1;
	else if (strcmp(word, "x2") == 0 || strcmp(word, "x02") == 0)		number[0] = 2;
	else if (strcmp(word, "x3") == 0 || strcmp(word, "x03") == 0)		number[0] = 3;
	else if (strcmp(word, "x4") == 0 || strcmp(word, "x04") == 0)		number[0] = 4;
	else if (strcmp(word, "x5") == 0 || strcmp(word, "x05") == 0)		number[0] = 5;
	else if (strcmp(word, "x6") == 0 || strcmp(word, "x06") == 0)		number[0] = 6;
	else if (strcmp(word, "x7") == 0 || strcmp(word, "x07") == 0)		number[0] = 7;
	else if (strcmp(word, "x8") == 0 || strcmp(word, "x08") == 0)		number[0] = 8;
	else if (strcmp(word, "x9") == 0 || strcmp(word, "x09") == 0)		number[0] = 9;
	else if (strcmp(word, "x10") == 0)									number[0] = 10;
	else if (strcmp(word, "x11") == 0)									number[0] = 11;
	else if (strcmp(word, "x12") == 0)									number[0] = 12;
	else if (strcmp(word, "x13") == 0)									number[0] = 13;
	else if (strcmp(word, "x14") == 0)									number[0] = 14;
	else if (strcmp(word, "x15") == 0)									number[0] = 15;
	else if (strcmp(word, "x16") == 0)									number[0] = 16;
	else if (strcmp(word, "x17") == 0)									number[0] = 17;
	else if (strcmp(word, "x18") == 0)									number[0] = 18;
	else if (strcmp(word, "x19") == 0)									number[0] = 19;
	else if (strcmp(word, "x20") == 0)									number[0] = 20;
	else if (strcmp(word, "x21") == 0)									number[0] = 21;
	else if (strcmp(word, "x22") == 0)									number[0] = 22;
	else if (strcmp(word, "x23") == 0)									number[0] = 23;
	else if (strcmp(word, "x24") == 0)									number[0] = 24;
	else if (strcmp(word, "x25") == 0)									number[0] = 25;
	else if (strcmp(word, "x26") == 0)									number[0] = 26;
	else if (strcmp(word, "x27") == 0)									number[0] = 27;
	else if (strcmp(word, "x28") == 0)									number[0] = 28;
	else if (strcmp(word, "x29") == 0)									number[0] = 29;
	else if (strcmp(word, "x30") == 0)									number[0] = 30;
	else if (strcmp(word, "x31") == 0)									number[0] = 31;
	else result = 0;
	return result;
}
UINT16 find_labelB(INT64* addr, const char* word, label_type* labels, UINT count) {
	UINT8 found = 0;
	for (UINT i = 0; i < count && !found; i++) {
		if (strcmp(word, labels[i].name) == 0) {
			found = 1;
			addr[0] = labels[i].addr;
		}
	}
	return found;
}
UINT16 find_labelB(INT64* number1, char* word, parse_struct2* parse_in, UINT8 pointer, UINT line_count) {
	UINT16 result = 0;
	char word2[0x100];
	for (UINT j = pointer; j < line_count && result == 0; j++) {
		char* data = parse_in->line[j].line;
		if (data[18] == ' ' && data[19] == '/' && data[20] == '/' &&
			data[21] == ' ' && data[22] == 'l' && data[23] == 'a' && data[24] == 'b' &&
			data[25] == 'e' && data[26] == 'l' && data[27] == ':' && data[28] == ' ') {
			getWordB(word2, data, 29);
			if (strcmp(word, word2) == 0) {
				getWordB(word2, data, 0);
				get_integer(number1, word2);
				result = 1;
			}
		}
	}
	for (INT j = pointer; j >= 0 && result == 0; j--) {
		char* data = parse_in->line[j].line;
		if (data[18] == ' ' && data[19] == '/' && data[20] == '/' &&
			data[21] == ' ' && data[22] == 'l' && data[23] == 'a' && data[24] == 'b' &&
			data[25] == 'e' && data[26] == 'l' && data[27] == ':' && data[28] == ' ') {
			getWordB(word2, data, 29);
			if (strcmp(word, word2) == 0) {
				getWordB(word2, data, 0);
				get_integer(number1, word2);
				result = 1;
			}
		}
	}
	return result;
}
UINT8 get_asm_numberB(UINT* number, char* word) {
	UINT8 result = 0;
	if (strcmp(word, "add") == 0) {
		number[0] = 0x00000033;
		result = 1;
	}
	else if (strcmp(word, "sub") == 0) {
		number[0] = 0x40000033;
		result = 1;
	}
	else if (strcmp(word, "sll") == 0) {
		number[0] = 0x00001033;
		result = 1;
	}
	else if (strcmp(word, "slt") == 0) {
		number[0] = 0x00002033;
		result = 1;
	}
	else if (strcmp(word, "sltu") == 0) {
		number[0] = 0x00003033;
		result = 1;
	}
	else if (strcmp(word, "xor") == 0) {
		number[0] = 0x00004033;
		result = 1;
	}
	else if (strcmp(word, "srl") == 0) {
		number[0] = 0x00005033;
		result = 1;
	}
	else if (strcmp(word, "sra") == 0) {
		number[0] = 0x40005033;
		result = 1;
	}
	else if (strcmp(word, "or") == 0) {
		number[0] = 0x00006033;
		result = 1;
	}
	else if (strcmp(word, "and") == 0) {
		number[0] = 0x00007033;
		result = 1;
	}
	else if (strcmp(word, "addi") == 0) {
		number[0] = 0x00000013;
		result = 1;
	}
	else if (strcmp(word, "slli") == 0) {
		number[0] = 0x00001013;
		result = 1;
	}
	else if (strcmp(word, "slti") == 0) {
		number[0] = 0x00002013;
		result = 1;
	}
	else if (strcmp(word, "sltui") == 0) {
		number[0] = 0x00003013;
		result = 1;
	}
	else if (strcmp(word, "xori") == 0) {
		number[0] = 0x00004013;
		result = 1;
	}
	else if (strcmp(word, "srli") == 0) {
		number[0] = 0x00005013;
		result = 1;
	}
	else if (strcmp(word, "srai") == 0) {
		number[0] = 0x40005013;
		result = 1;
	}
	else if (strcmp(word, "ori") == 0) {
		number[0] = 0x00006013;
		result = 1;
	}
	else if (strcmp(word, "andi") == 0) {
		number[0] = 0x00007013;
		result = 1;
	}
	return result;
}
UINT8 get_shortB(short* number, char* input) {
	int debug = 0;
	UINT8 result = 1;
	//	int number;
	number[0] = 0;
	UINT8 hex = 0;
	if (strlen(input) > 2) {
		if (input[0] == '-') {
			if (input[1] == '0' && input[2] == 'x') {
				hex = 1;
				for (UINT8 i = 3; i < strlen(input) && result; i++) {
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
						number[0] += 10;
						break;
					case 'b':
						number[0] += 11;
						break;
					case 'c':
						number[0] += 12;
						break;
					case 'd':
						number[0] += 13;
						break;
					case 'e':
						number[0] += 14;
						break;
					case 'f':
						number[0] += 15;
						break;
					default:
						debug++;
						result = 0;
						break;
					}
				}
			}
			number[0] *= -1;
		}
		else {
			if (input[0] == '0' && input[1] == 'x') {
				hex = 1;
				for (UINT8 i = 2; i < strlen(input) && result; i++) {
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
						number[0] += 10;
						break;
					case 'b':
						number[0] += 11;
						break;
					case 'c':
						number[0] += 12;
						break;
					case 'd':
						number[0] += 13;
						break;
					case 'e':
						number[0] += 14;
						break;
					case 'f':
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
	if (!hex) {
		if (input[0] == '-') {
			for (UINT8 i = 1; i < strlen(input) && result; i++) {
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
			number[0] *= -1;
		}
		else {
			for (UINT8 i = 0; i < strlen(input) && result; i++) {
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
	}
	return result;
}

void print_rd(UINT64* code, parse_struct2* parse_out, parse_struct2* parse_in) {
	int debug = 0;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
	getWord(parse_in);
	UINT8 number2;
	char word[0x10];
	if (get_reg_number_EABI(&number2, parse_in->word)) {
		sprintf_s(word, " x%d, ", number2);
		code[0] |= number2 << 7;
	}
	else if (get_freg_numberB(&number2, parse_in->word)) {
		sprintf_s(word, " f%d, ", number2);
		code[0] |= number2 << 7;
	}
	else {
		debug++;
	}
	strcat_p2(parse_out, word);
}
void print_rs1(UINT64* code, parse_struct2* parse_out, parse_struct2* parse_in) {
	int debug = 0;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ',' || parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
	getWord(parse_in);
	UINT8 number2;
	short number1;
	char word[0x10];
	if (get_reg_number_EABI(&number2, parse_in->word)) {
		sprintf_s(word, "x%d, ", number2);
		code[0] |= number2 << 15;
	}
	else if (get_freg_numberB(&number2, parse_in->word)) {
		sprintf_s(word, "f%d, ", number2);
		code[0] |= number2 << 15;
	}
	else if (get_shortB(&number1, parse_in->word)) {
		sprintf_s(word, "0x%03x, ", number1);
//		strcat_p2(parse_out, word);
		code[0] |= number1 << 15;
		if (number1 >= 0x20)
			debug++;// syntax error
	}
	else {
		debug++;
	}
	strcat_p2(parse_out, word);
}
void print_rs2(UINT64* code, parse_struct2* parse_out, parse_struct2* parse_in, INT64 *number1, label_type* labels, UINT label_count) {
	int debug = 0;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ',' || parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
	getWord(parse_in);
	short number12 = 0;
	UINT8 number2;
	char word[0x10];
	if (get_shortB(&number12, parse_in->word)) {
		sprintf_s(word, "0x%03x ", number12);
		code[0] |= number12 << 20;
	}
	else if (get_reg_number_EABI(&number2, parse_in->word)) {
		sprintf_s(word, "x%d ", number2);
		code[0] |= number2 << 20;
	}
	else if (get_freg_numberB(&number2, parse_in->word)) {
		sprintf_s(word, "f%d ", number2);
		code[0] |= number2 << 20;
	}
	else if (find_labelB(number1, parse_in->word, labels, label_count)) {// auipc
		number1[0] = number1[0] & 0x0fff;
		sprintf_s(word, "0x%x ", number1);
		code[0] |= number1[0] << 20;
	}
	else {
		debug++;
	}
	strcat_p2(parse_out, word);
}
void compile_to_RISCV_asm(memory_space_type* memory_space, const char* dst_name, const char* src_name, parse_struct2_set* parse, INT64 physical_PC, INT64 logical_PC, label_type* labels, param_type* param, UINT8 print_asm) {
	int debug = 0;
	FILE* src;
	fopen_s(&src, src_name, "r");

	UINT8 last_char = 0;
	char word2[0x100];
	UINT8 length;
	INT64 number0;
	INT64  number1;
	parse_struct2* parse_in = &parse->a;
	parse_struct2* parse_out = &parse->b;
	if (parse_in->index == 0) {
		while (fgets(parse_in->line[parse_in->index++].line, 0x100, src) != NULL) {}
		parse_in->index--;
	}
	fclose(src);
	UINT64 base_address;
	page_4K_type* page;

	//	label_type *labels = (label_type *)malloc(0x10000*sizeof(label_type));
	UINT label_count = 0;
	UINT line_count = parse_in->index;
	for (parse_in->index = 0; parse_in->index < line_count; parse_in->index++) {
		parse_in->ptr = 0;

		if (parse_in->line[parse_in->index].line[0] == '0' && parse_in->line[parse_in->index].line[1] == 'x') {
			getWord(parse_in);
			if (get_integer(&labels[label_count].addr, parse_in->word)) {
			}
			else {
				debug++;
			}
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' && parse_in->line[parse_in->index].line[parse_in->ptr+1] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr+2] == '/') {
				parse_in->ptr += 3;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				getWord(parse_in);
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == ':') {
					parse_in->ptr++;
					parse_in->word[parse_in->word_ptr++] = ':';
					parse_in->word[parse_in->word_ptr] = '\0';
				}
				if (strcmp(parse_in->word, "label:") == 0) {
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
					getWord(parse_in);
					sprintf_s(labels[label_count++].name,"%s", parse_in->word);
					if (label_count >= 0x10000)
						debug++;
				}
			}
		}
	}
	for (parse_in->index = 0, parse_out->index = 0; parse_in->index < line_count; parse_in->index++, parse_out->index++) {
		parse_in->ptr = 0;
		parse_out->ptr = 0;
		if (parse_in->index >= 66484)
			debug++;
		if (parse_in->line[parse_in->index].line[0] == '/' && parse_in->line[parse_in->index].line[1] == '/') {
			strcpy_p2(parse_out, parse_in);
		}
		else {
			getWord(parse_in);
			get_integer(&number0, parse_in->word);
			if (number0 == 0)
				debug++;
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == 'd' && parse_in->line[parse_in->index].line[parse_in->ptr+1] == 'q') {
				sprintf_s(parse_out->line[parse_out->index].line, "%s", parse_in->line[parse_in->index].line);

				if (number0 < 0x2000000) {
					base_address = 0x00001000;
					page = memory_space->reset;
				}
				else if (number0 < 0x10000000) {
					base_address = 0x2000000;
					page = memory_space->clic;
				}
				else {
					base_address = 0x80000000;// need to add data translation
					page = memory_space->page;
					if ((number0 & ((UINT64)~0xfff)) == (logical_PC & ((UINT64)~0xfff))) {
						base_address += (physical_PC - logical_PC);
					}
				}

				UINT page_index = (number0 - base_address) >> 21;
				UINT memory_index = ((number0 - base_address) >> 3) & 0x3ffff;

				parse_in->ptr = 21;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
				getWord(parse_in);
				INT64  number2;
				if (get_integer(&number2, parse_in->word)) {
					page[page_index].memory[memory_index] = number2;
				}
				else {
					find_labelB(&number1, "label_switch_start", labels, label_count);
					if (find_labelB(&number2, parse_in->word, labels, label_count)) {
						number2 = number2 - number1;
						page[page_index].memory[memory_index] = number2;
					}
					else if (find_labelB(&number2, "label_default", labels, label_count)) {
						number2 = number2 - number1;
						page[page_index].memory[memory_index] = number2;
					}
					else {
						debug++;
					}
				}
			}
			else {
				if (number0 < 0x2000000) {
					base_address = 0x00001000;
					page = memory_space->reset;
				}
				else if (number0 < 0x10000000) {
					base_address = 0x2000000;
					page = memory_space->clic;
				}
				else {
					base_address = 0x80000000;// need to add data translation
					page = memory_space->page;
					if ((number0 & ((UINT64)~0xfff)) == (logical_PC & ((UINT64)~0xfff))) {
						base_address += (physical_PC - logical_PC);
					}
				}

				UINT page_index = (number0 - base_address) >> 21;
				UINT memory_index = ((number0 - base_address) >> 3) & 0x3ffff;//0x40000
				UINT shift_block32 = (number0 & 0x04) ? 5 : 0;
				if (page_index == 0)
					debug++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr+1] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr+2] == ' ' &&
					parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'l' && parse_in->line[parse_in->index].line[parse_in->ptr + 4] == 'a' && parse_in->line[parse_in->index].line[parse_in->ptr + 5] == 'b' && parse_in->line[parse_in->index].line[parse_in->ptr + 6] == 'e' && parse_in->line[parse_in->index].line[parse_in->ptr + 7] == 'l' && parse_in->line[parse_in->index].line[parse_in->ptr + 8] == ':' && parse_in->line[parse_in->index].line[parse_in->ptr + 9] == ' ') {
					strcpy_p2(parse_out, parse_in);
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == 'r' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'e' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 't') {
					sprint_addr_p2(parse_out, number0, param);
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == 'u') {
						strcat_p2(parse_out, (char*)"uret");
						page[page_index].memory[memory_index] = 0x00200073 << shift_block32;
					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr] == 's') {
						strcat_p2(parse_out, (char*)"sret");
						if (shift_block32 == 0) {
							page[page_index].memory[memory_index] = 0x10200073;
						}
						else {
							page[page_index].memory[memory_index] &= 0x00000000ffffffff;
							page[page_index].memory[memory_index] |= 0x1020007300000000;
						}
					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr] == 'h') {
						strcat_p2(parse_out,(char*) "hret");
						page[page_index].memory[memory_index] = 0x20200073 << shift_block32;
					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr] == 'm') {
						strcat_p2(parse_out,(char*) "mret");
						page[page_index].memory[memory_index] = 0x30200073 << shift_block32;
					}
					char word[0x20];
					sprintf_s(word, "// ");
					strcat_p2(parse_out, word);
					sprint_addr_p2(parse_out, number0, param);
					sprintf_s(word, " DW 0x%08x\n", page[page_index].memory[memory_index]);//DD - 64b, DW - 32b, DH - 16b
					strcat_p2(parse_out, word);
				}
				else {
					char word[0x20];
					sprint_addr_p2(parse_out, number0, param);
					parse_out->line[parse_out->index].line[parse_out->ptr++] = ' '; 
					parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
					UINT64 code = 0;
					char print_tail = 1;
					switch (parse_in->line[parse_in->index].line[parse_in->ptr]) {
					case '/': {
						if (parse_in->line[parse_in->index].line[parse_in->ptr+1] == '/') {
							debug++;
						}
						else {
							debug++; // syntax error
						}
					}
						break;
					case 'a': {
						if (parse_in->line[parse_in->index].line[parse_in->ptr+1] == 'u' && parse_in->line[parse_in->index].line[parse_in->ptr+2] == 'i' && parse_in->line[parse_in->index].line[parse_in->ptr+3] == 'p' && parse_in->line[parse_in->index].line[parse_in->ptr+4] == 'c' && parse_in->line[parse_in->index].line[parse_in->ptr+5] == ' ') {
							getWord(parse_in);
							sprintf_s(parse_out->line[parse_out->index].line, "%s ", parse_in->word);
							code = 0x0017;

							UINT8 number2;
							print_rd(&code, parse_out, parse_in);

							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							if (get_integer(&number1, parse_in->word)) {
								sprintf_s(parse_out->line[parse_out->index].line, "%#x ", number1);
								code |= number1 << 12;
							}
							else if (find_labelB(&number1, parse_in->word, labels, label_count)) {// auipc
								if (number1 & 0x800) {
									number1 = (number1 - number0) >> 12;
									number1++;
								}
								else {
									number1 = (number1 - number0) >> 12;
								}
								sprintf_s(parse_out->line[parse_out->index].line, "%#x ", number1);
								code |= number1 << 12;
							}
							else {
								debug++;
							}
						}
						else {
							getWord(parse_in);
							strcat_p2(parse_out, parse_in->word);
							UINT number32;
							if (get_asm_numberB(&number32, parse_in->word)) {
								code = number32 | 0x08;// 08 for 64b vs 32b
							}
							else {
								code = 0;
							}
							// need to match instruction to generate binary
							print_rd(&code, parse_out, parse_in);
							print_rs1(&code, parse_out, parse_in);
							print_rs2(&code, parse_out, parse_in,&number1,labels, label_count);
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						}
					}
						break;
					case 'b':{//branch
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						code = 0x63;
						if (parse_in->line[parse_in->index].line[parse_in->ptr+1] == 'e') {
							code |= 0x0000;
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == 'n') {
							code |= 0x1000;
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == 'l' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 't' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == ' ') {
							code |= 0x4000;
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == 'l' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 't' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'u') {
							code |= 0x6000;
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == 'g' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'e' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == ' ') {
							code |= 0x5000;
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == 'g' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'e' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'u') {
							code |= 0x7000;
						}
						else {
							debug++;
						}
						getWord(parse_in);
						strcat_p2(parse_out, parse_in->word);
						parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
						parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
						print_rs1(&code, parse_out, parse_in);
						print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);
						
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ',' || parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						getWord(parse_in);

						if (find_labelB(&number1, parse_in->word, labels, label_count)) {
							number1 = number1 - number0;
							if (number1 < 0x1000) {
								sprintf_s(word, "0x%03x // ", number1);
								strcat_p2(parse_out, word);
								strcat_p2(parse_out, parse_in->word);
								code |= ((number1 & 0x1000) << 19) | ((number1 & 0x800) >> 4) | ((number1 & 0x7e0) << 20) | ((number1 & 0x01e) << 7);// rd = reg0
							}
							else {
								strcat_p2(parse_out, (char*)"\n ERROR: branch to address beyond 12b range \n");
							}
						}
						else {
							sprintf_s(word, "  ERROR: no label match %s ERROR ", parse_in->word);
							strcat_p2(parse_out, word);
						}
					}
					break;
					case 'c':
						parse_in->ptr++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == 's' && parse_in->line[parse_in->index].line[parse_in->ptr+1] == 'r' && parse_in->line[parse_in->index].line[parse_in->ptr+2] == 'r') {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							parse_in->ptr += 3;
							switch (parse_in->line[parse_in->index].line[parse_in->ptr++]) {
				 			case 'w':
								code = 0x001073;
								strcat_p2(parse_out, (char*)"csrrw");
								break;
							case 's':
								code = 0x002073;
								strcat_p2(parse_out, (char*)"csrrs");
								break;
							case 'c':
								code = 0x003073;
								strcat_p2(parse_out, (char*)"csrrc");
								break;
							default:
								debug++;
								code = 0x000073;// SYSTEM
								break;
							}
							if (parse_in->line[parse_in->index].line[parse_in->ptr++] == 'i') {
								code |= 0x004000; 
								strcat_p2(parse_out, (char*)"i ");
							}
							else {
								strcat_p2(parse_out, (char*)" ");
							}
							print_rd(&code, parse_out, parse_in);
							print_rs1(&code, parse_out, parse_in);

							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ',' || parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							sprintf_s(word, "%s ", parse_in->word);
							strcat_p2(parse_out, word);
							UINT16 number3;
							if (get_csr_numberB(&number3, parse_in->word)) {
								code |= number3 << 20;
							}
							else {
								debug++;
							}
						}
						else {
							debug++;
						}
						break;
					case 'd':
					case 'D': {
						switch (parse_in->line[parse_in->index].line[parse_in->ptr + 1]) {
						case 'B':// byte - 8 bits
							parse_in->ptr += 2;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							if (find_labelB(&number1, parse_in->word, labels, label_count)) {
							}
							else if (get_integer(&number1, parse_in->word)) {
							}
							else {
								debug++;
							}
							strcat_p2(parse_out, (char*)"DB ");
							sprintf_s(word, "0x%02x ", number1);
							strcat_p2(parse_out, word);
							switch (number0 & 7) {
							case 0:
								page[page_index].memory[memory_index] &= (~(UINT64)0x00000000000000ff);
								page[page_index].memory[memory_index] |= (number1 & 0x00000000000000ff);
								break;
							case 1:
								page[page_index].memory[memory_index] &= (~(UINT64)0x000000000000ff00);
								page[page_index].memory[memory_index] |= ((number1 << 8) & 0x000000000000ff00);
								break;
							case 2:
								page[page_index].memory[memory_index] &= (~(UINT64)0x0000000000ff0000);
								page[page_index].memory[memory_index] |= ((number1 << 16) & 0x0000000000ff0000);
								break;
							case 3:
								page[page_index].memory[memory_index] &= (~(UINT64)0x00000000ff000000);
								page[page_index].memory[memory_index] |= ((number1 << 24) & 0x00000000ff000000);
								break;
							case 4:
								page[page_index].memory[memory_index] &= (~0x000000ff00000000);
								page[page_index].memory[memory_index] |= ((number1 << 32) & 0x000000ff00000000);
								break;
							case 5:
								page[page_index].memory[memory_index] &= (~0x0000ff0000000000);
								page[page_index].memory[memory_index] |= ((number1 << 40) & 0x0000ff0000000000);
								break;
							case 6:
								page[page_index].memory[memory_index] &= (~0x00ff000000000000);
								page[page_index].memory[memory_index] |= ((number1 << 48) & 0x00ff000000000000);
								break;
							case 7:
								page[page_index].memory[memory_index] &= (~0xff00000000000000);
								page[page_index].memory[memory_index] |= ((number1 << 56) & 0xff00000000000000);
								break;
							default:
								debug++;
								break;
							}
							print_tail = 0;
							break;
						case 'H':// half - 16 bits
							parse_in->ptr += 2;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							if (find_labelB(&number1, parse_in->word, labels, label_count)) {
							}
							else if (get_integer(&number1, parse_in->word)) {
							}
							else {
								debug++;
							}
							strcat_p2(parse_out, (char*)"DH ");
							sprintf_s(word, "0x%04x ", number1);
							strcat_p2(parse_out, word);
							print_tail = 0;
							switch (number0 & 7) {
							case 0:
								page[page_index].memory[memory_index] &= (~0x000000000000ffff);
								page[page_index].memory[memory_index] |= (number1 & 0x000000000000ffff);
								break;
							case 2:
								page[page_index].memory[memory_index] &= (~0x00000000ffff0000);
								page[page_index].memory[memory_index] |= (number1 & 0x00000000ffff0000);
								break;
							case 4:
								page[page_index].memory[memory_index] &= (~0x0000ffff00000000);
								page[page_index].memory[memory_index] |= (number1 & 0x0000ffff00000000);
								break;
							case 6:
								page[page_index].memory[memory_index] &= (~0xffff000000000000);
								page[page_index].memory[memory_index] |= (number1 & 0xffff000000000000);
								break;
							default:
								debug++;
								break;
							}
							break;
						case 'W':// word - 32 bits
							parse_in->ptr += 2;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							if (find_labelB(&number1, parse_in->word, labels, label_count)) {
								debug++;
							}
							else if (get_integer(&number1, parse_in->word)) {
								debug++;
							}
							else {
								debug++;
							}
							strcat_p2(parse_out, (char*)"DW ");
							sprintf_s(word, "0x%08x ", number1);
							strcat_p2(parse_out, word);
		//					code = number1;
							print_tail = 0;
							if (number0 & 0x7 == 0) {
								page[page_index].memory[memory_index] &= (~0x00000000ffffffff);
								page[page_index].memory[memory_index] |= (number1 & 0x00000000ffffffff);
							}
							else {
								page[page_index].memory[memory_index] &= (0x00000000ffffffff);
								page[page_index].memory[memory_index] |= (number1 & (~0x00000000ffffffff));
							}
							break;
						case 'd':
						case 'D': {// double word - 64 bits
							if (number0 < 0x2000000) {
								base_address = 0x00001000;
								page = memory_space->reset;
							}
							else if (number0 < 0x10000000) {
								base_address = 0x2000000;
								page = memory_space->clic;
							}
							else {
								base_address = 0x80000000;// need to add data translation
								page = memory_space->page;
								if ((number0 & ((UINT64)~0xfff)) == (logical_PC & ((UINT64)~0xfff))) {
									base_address += (physical_PC - logical_PC);
								}
							}

							UINT page_index = (number0 - base_address) >> 21;
							UINT memory_index = ((number0 - base_address) >> 3) & 0x3ffff;
							UINT shift_block32 = (number0 & 0x04) ? 5 : 0;

							getWord(parse_in);
							strcat_p2(parse_out, parse_in->word); // DD - define double word
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							INT64  number2;
							find_labelB(&number1, "label_switch_start", labels, label_count);
							if (get_integer(&number2, parse_in->word)) {
								debug++;
							}
							else if (find_labelB(&number2, parse_in->word, labels, label_count)) {
								number2 = number2 - number1;
							}
							else if (find_labelB(&number2, "label_default", labels, label_count)) {
//								number2 = number2 - number1;
							}
							else {
								debug++;
							}
							sprintf_s(word, " 0x%016x\n", number2);
							strcat_p2(parse_out, word);
							print_tail = 0;
							code = number2;
							page[page_index].memory[memory_index] = number2;
						}
							break;
						case 'Q':// quad word - 128 bits
							parse_in->ptr += 2;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							// note: llx means 64 bit, not primatives for 128b
							debug++;
							break;
						default:
							debug++;
							break;
						}
					}
						break;
					case 'e':
						if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == 'c' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'a' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'l' && parse_in->line[parse_in->index].line[parse_in->ptr + 4] == 'l') {
							code = 0x00000073;// SYSTEM
							code |= 0x00000000;// ecall - generate an interrupt
							strcat_p2(parse_out, (char*)"ecall ");
						}
						else {
							debug++;
						}
						break;
					case 'E':
						if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == 'R' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'R' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'O' && parse_in->line[parse_in->index].line[parse_in->ptr + 4] == 'R' && parse_in->line[parse_in->index].line[parse_in->ptr + 5] == ':' && parse_in->line[parse_in->index].line[parse_in->ptr + 6] == ' ') {
							debug++;
						}
						else {
							debug++;
						}
						break;
					case 'f':// floating point
						switch (parse_in->line[parse_in->index].line[parse_in->ptr + 1]) {
						case 'a': {
							if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'd' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'd' && parse_in->line[parse_in->index].line[parse_in->ptr + 4] == '.') {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								switch (parse_in->line[parse_in->index].line[parse_in->ptr + 5]) {
								case 'h':
									code = 0x04000053;
									break;
								case 'w':
									code = 0x00000053;
									break;
								case 'd':
									code = 0x02000053;
									break;
								case 'q':
									code = 0x06000053;
									break;
								default:
									debug++;
									break;
								}
								UINT8 number1a;
								getWord(parse_in);
								strcat_p2(parse_out, parse_in->word);
								print_rd(&code, parse_out, parse_in);
								print_rs1(&code, parse_out, parse_in);
								print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);
							}
							else {
								debug++;
							}
						}
								break;
						case 'c':
						{
							if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'v' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 't') {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								switch (parse_in->line[parse_in->index].line[parse_in->ptr + 5]) {
								case 'w':
									code = 0xd0000053;
									break;
								case 'd':
									code = 0xd2000053;
									break;
								case 'h':
									code = 0xd4000053;
									break;
								case 'q':
									code = 0xd6000053;
									break;
								case 'x':
									switch (parse_in->line[parse_in->index].line[parse_in->ptr + 7]) {
									case 'w':
										code = 0xc0000053;
										break;
									case 'd':
										code = 0xc2000053;
										break;
									case 'h':
										code = 0xc4000053;
										break;
									case 'q':
										code = 0xc6000053;
										break;
									case 'x':
										break;
									default:
										debug++;
										break;
									}
									break;
								default:
									debug++;
									break;
								}
								UINT8 number1a;
						//		if (parse_in->line[parse_in->index].line[parse_in->ptr + 5] == 'x') {
						//			getWord(parse_in);
						//			strcat_p2(parse_out, parse_in->word);
						//			print_rd(&code, parse_out, parse_in);
						//			print_rs1(&code, parse_out, parse_in);
						//		}
						//		else {
									getWord(parse_in);
									strcat_p2(parse_out, parse_in->word);
									print_rd(&code, parse_out, parse_in);
									print_rs1(&code, parse_out, parse_in);
						//		}
							}
							else
								debug++;
						}
						break;
						case 'e':
							if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == 'e' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'n' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'c' && parse_in->line[parse_in->index].line[parse_in->ptr + 4] == 'e' && parse_in->line[parse_in->index].line[parse_in->ptr + 5] == ' ' && parse_in->line[parse_in->index].line[parse_in->ptr + 6] == '0') {
								strcat_p2(parse_out, (char*)"fence 0 // normal fence - synch (complete)  all memory instructions");
								code = 0x8000000f;// normal fence
							}
							else {
								debug++;
							}
							break;
						case 'l': {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							strcat_p2(parse_out, parse_in->word);//fp load
							switch (parse_in->word[2]) {
							case 'h':
								code = 0x1007;// half precision
								break;
							case 'w':
								code = 0x2007;//single precision
								break;
							case 'd':
								code = 0x3007;//double precision
								break;
							case 'q':
								code = 0x4007;// quad precision
								break;
							default:
								debug++;
								break;
							}

							print_rd(&code, parse_out, parse_in);

							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',')parse_in->ptr++;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							short number2;
							get_shortB(&number2, parse_in->word);
							code |= ((number2) << 20);

							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(')parse_in->ptr++;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							UINT8 number3;
							get_reg_number_EABI(&number3, parse_in->word);
							sprintf_s(word, "x%d, ", number3);
							strcat_p2(parse_out, word);
							sprintf_s(word, "%d ", number2);
							strcat_p2(parse_out, word);
							code |= number3 << 15;
						}
								break;
						case 'm':
						{
							if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'a' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'x') {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								switch (parse_in->line[parse_in->index].line[parse_in->ptr + 5]) {
								case 'h':
									code = 0x2c001053;
									break;
								case 'w':
									code = 0x28001053;
									break;
								case 'd':
									code = 0x2a001053;
									break;
								case 'q':
									code = 0x2e001053;
									break;
								default:
									debug++;
									break;
								}
								getWord(parse_in);
								strcat_p2(parse_out, parse_in->word);
								UINT8 number1a;
								print_rd(&code, parse_out, parse_in);
								print_rs1(&code, parse_out, parse_in);
								print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'a' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'd' && parse_in->line[parse_in->index].line[parse_in->ptr + 4] == 'd') {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								switch (parse_in->line[parse_in->index].line[parse_in->ptr + 6]) {
								case 'h':
									code = 0x04000043;
									break;
								case 'w':
									code = 0x00000043;
									break;
								case 'd':
									code = 0x02000043;
									break;
								case 'q':
									code = 0x06000043;
									break;
								default:
									debug++;
									break;
								}
								UINT8 number1a;
								getWord(parse_in);
								strcat_p2(parse_out, parse_in->word);
								print_rd(&code, parse_out, parse_in);
								print_rs1(&code, parse_out, parse_in);
								print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);

								if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',')parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								get_freg_numberB(&number1a, parse_in->word);
								sprintf_s(word, "f%d ", number1a);
								strcat_p2(parse_out, word);
								code |= number1a << 27;
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'i' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'n') {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								//								code= 0x2c000053;
								switch (parse_in->line[parse_in->index].line[parse_in->ptr + 5]) {
								case 'h':
									code = 0x2c000053;
									break;
								case 'w':
									code = 0x28000053;
									break;
								case 'd':
									code = 0x2a000053;
									break;
								case 'q':
									code = 0x2e000053;
									break;
								default:
									debug++;
									break;
								}
								UINT8 number1a;
								getWord(parse_in);
								strcat_p2(parse_out, parse_in->word);
								print_rd(&code, parse_out, parse_in);
								print_rs1(&code, parse_out, parse_in);
								print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'v') {
								// fmv
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								if (parse_in->line[parse_in->index].line[parse_in->ptr + 4] == 'w' || parse_in->line[parse_in->index].line[parse_in->ptr + 4] == 'd' || parse_in->line[parse_in->index].line[parse_in->ptr + 4] == 'h' || parse_in->line[parse_in->index].line[parse_in->ptr + 4] == 'q') {
									switch (parse_in->line[parse_in->index].line[parse_in->ptr + 4]) {
									case 'h':
										code = 0xf4000053;
										break;
									case 'w':
										code = 0xf0000053;
										break;
									case 'd':
										code = 0xf2000053;
										break;
									case 'q':
										code = 0xf6000053;
										break;
									default:
										debug++;
										break;
									}
									UINT8 number1a;
									getWord(parse_in);
									strcat_p2(parse_out, parse_in->word);
									print_rd(&code, parse_out, parse_in);

									if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',')parse_in->ptr++;
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
									getWord(parse_in);
									get_freg_numberB(&number1a, parse_in->word);
									if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "f%d, ", number1a);
									code |= number1a << 15;
								}
								else {
									switch (parse_in->line[parse_in->index].line[parse_in->ptr + 6]) {
									case 'h':
										code = 0xe4000053;
										break;
									case 'w':
										code = 0xe0000053;
										break;
									case 'd':
										code = 0xe2000053;
										break;
									case 'q':
										code = 0xe6000053;
										break;
									default:
										debug++;
										break;
									}
									UINT8 number1a;
									getWord(parse_in);
									strcat_p2(parse_out, parse_in->word);
									print_rd(&code, parse_out, parse_in);

									if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',')parse_in->ptr++;
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
									getWord(parse_in);
									get_reg_number_EABI(&number1a, parse_in->word);
									sprintf_s(word, "x%d, ", number1a);
									strcat_p2(parse_out, word);
									code |= number1a << 15;
								}
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'u' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'l') {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								//								code= 0x2c000053;
								switch (parse_in->line[parse_in->index].line[parse_in->ptr + 5]) {
								case 'h':
									code = 0x14001053;
									break;
								case 'w':
									code = 0x10001053;
									break;
								case 'd':
									code = 0x12001053;
									break;
								case 'q':
									code = 0x16001053;
									break;
								default:
									debug++;
									break;
								}
								UINT8 number1a;
								getWord(parse_in);
								strcat_p2(parse_out, parse_in->word);
								print_rd(&code, parse_out, parse_in);
								print_rs1(&code, parse_out, parse_in);
								print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);
							}
							else
								debug++;
						}
						break;
						case 'n':
							if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'm' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'a' && parse_in->line[parse_in->index].line[parse_in->ptr + 4] == 'd' && parse_in->line[parse_in->index].line[parse_in->ptr + 5] == 'd') {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								//								code= 0x2c000053;
								switch (parse_in->line[parse_in->index].line[parse_in->ptr + 7]) {
								case 'h':
									code = 0x0400004f;
									break;
								case 'w':
									code = 0x0000004f;
									break;
								case 'd':
									code = 0x0200004f;
									break;
								case 'q':
									code = 0x0600004f;
									break;
								default:
									debug++;
									break;
								}
								UINT8 number1a;
								getWord(parse_in);
								strcat_p2(parse_out, parse_in->word);
								print_rd(&code, parse_out, parse_in);
								print_rs1(&code, parse_out, parse_in);
								print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);

								if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',')parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								get_freg_numberB(&number1a, parse_in->word);
								if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "f%d ", number1a);
								code |= number1a << 27;
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'm' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 's' && parse_in->line[parse_in->index].line[parse_in->ptr + 4] == 'u' && parse_in->line[parse_in->index].line[parse_in->ptr + 5] == 'b') {
								switch (parse_in->line[parse_in->index].line[parse_in->ptr + 7]) {
								case 'h':
									code = 0x0400004b;
									break;
								case 'w':
									code = 0x0000004b;
									break;
								case 'd':
									code = 0x0200004b;
									break;
								case 'q':
									code = 0x0600004b;
									break;
								default:
									debug++;
									break;
								}
								UINT8 number1a;
								getWord(parse_in);
								strcat_p2(parse_out, parse_in->word);
								print_rd(&code, parse_out, parse_in);
								print_rs1(&code, parse_out, parse_in);
								print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);

								if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',')parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								get_freg_numberB(&number1a, parse_in->word);
								sprintf_s(word, "f%d ", number1a);
								strcat_p2(parse_out, word);
								code |= number1a << 27;
							}
							else {
								debug++;
							}
							break;
						case 's': {//fs
							if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'u' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'b') {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								strcat_p2(parse_out, parse_in->word);
								switch (parse_in->line[parse_in->index].line[parse_in->ptr + 5]) {
								case 'h':
									code = 0x0c000053;// half precision
									break;
								case 'w':
									code = 0x08000053;//single precision
									break;
								case 'd':
									code = 0x0a000053;//double precision
									break;
								case 'q':
									code = 0x0e000053;// quad precision
									break;
								default:
									debug++;
									break;
								}
								UINT8 number1a;
								print_rd(&code, parse_out, parse_in);
								print_rs1(&code, parse_out, parse_in);
								print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);
							}
							else {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								switch (parse_in->line[parse_in->index].line[parse_in->ptr + 2]) {
								case 'h':
									code = 0x1027;// half precision
									break;
								case 'w':
									code = 0x2027;//single precision
									break;
								case 'd':
									code = 0x3027;//double precision
									break;
								case 'q':
									code = 0x4027;// quad precision
									break;
								default:
									debug++;
									break;
								}
								getWord(parse_in);
								strcat_p2(parse_out, parse_in->word);
								print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);

								if (parse_in->line[parse_in->index].line[parse_in->ptr] == ',')parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								INT64 number2;
								get_integer(&number2, parse_in->word);
								sprintf_s(word, "0x%03x ", number2);
								strcat_p2(parse_out, word);
								code |= ((number2 & 0x0fe0) << 20) | ((number2 & 0x001f) << 7);

								if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(')parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								UINT8 number3;
								get_reg_number_EABI(&number3, parse_in->word);
								sprintf_s(word, "(x%d) ", number3);
								strcat_p2(parse_out, word);
								code |= number3 << 15;
							}
						}
								break;
						default:
							debug++;
							break;
						}
						break;
					case 'j':// jalr rd, r1, imm
						if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == 'a' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'l' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'r') {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							strcat_p2(parse_out, (char*)"jalr ");
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							UINT8 num, num2;
							get_reg_number_EABI(&num, parse_in->word);

							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ',' || parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							get_integer(&number1, parse_in->word);
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == '(' || parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							get_reg_number_EABI(&num2, parse_in->word);

							sprintf_s(word, "x%d, x%d, %d ", num, num2, number1);
							strcat_p2(parse_out, word);

							code = 0x000067;// JALR
							code |= num << 7;// rd 	
							code |= num2 << 15;// rs1		
							code |= number1 << 20;// imm		
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == 'a' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'l' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == ' ') {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							UINT8 num;
							get_reg_number_EABI(&num, parse_in->word);

							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ',' || parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							if (get_integer(&number1, parse_in->word)) {
								sprintf_s(word, "jal   x%d, %d ", num, number1);
								strcat_p2(parse_out, word);
								code = 0x00006F;// JAL
								code |= num << 7;// rd 		
								code |= ((number1 & 0x100000) << 11) | (number1 & 0xff000) | ((number1 & 0x800) << 9) | ((number1 & 0x07fe) << 20);// rd = reg0
							}
							else if (find_labelB(&number1, parse_in->word, labels, label_count)) {
								number1 = number1 - number0;
								sprintf_s(word, "jal   x%d, %d ", num, number1);
								strcat_p2(parse_out, word);
								code = 0x00006F;// JAL
								code |= num << 7;// rd 		
								code |= ((number1 & 0x100000) << 11) | (number1 & 0xff000) | ((number1 & 0x800) << 9) | ((number1 & 0x07fe) << 20);// rd = reg0
							}
							else {
								debug++;
							}
						}
						else {
							UINT no_return = 1;
							if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ' ') {
								no_return = 1;
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == 'a' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'l') {
								no_return = 0;
							}
							else
								debug++;//syntax error
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							if (find_labelB(&number1, parse_in->word, labels, label_count)) {
								number1 = number1 - number0;
							}
							else if (get_integer(&number1, parse_in->word)) {
							}
							else {
								debug++;
							}
							if (number1 < 0x80000 && number1 > -0x80000) {
								code = 0x00006F;// JAL
								if (no_return) {
									sprintf_s(word, "jal x0, 0x%08x", number1);
									code |= 0x00 << 7;// rd = reg0
								}
								else  {
									sprintf_s(word, "jal x1, 0x%08x", number1);
									code |= 0x01 << 7;// rd = reg1
								}
								strcat_p2(parse_out, word);
								code |= ((number1 << 11) & 0xffffffff80000000) | (number1 & 0xff000) | ((number1 & 0x800) << 9) | ((number1 & 0x07fe) << 20);// rd = reg0
							}
							else {// AUIPC + JALR
								parse_struct2 parse_next;
								parse_next.line = parse_in->line;
								parse_next.index = parse_in->index + 1;
								parse_next.ptr = 0;
								getWord(&parse_next);
								while (parse_next.line[parse_next.index].line[parse_next.ptr] == ' ' || parse_next.line[parse_next.index].line[parse_next.ptr] == '\t')parse_next.ptr++;
								getWord(&parse_next);
								while (parse_next.line[parse_next.index].line[parse_next.ptr] == ' ' || parse_next.line[parse_next.index].line[parse_next.ptr] == '\t')parse_next.ptr++;
								if (strcmp(parse_next.word, "nop"))
									debug++;
								getWord(&parse_next);
								while (parse_next.line[parse_next.index].line[parse_next.ptr] == ' ' || parse_next.line[parse_next.index].line[parse_next.ptr] == '\t')parse_next.ptr++;
								INT64  number2;
								if (get_integer(&number2, parse_next.word)) {
								}
								else {
									debug++;
								}
									sprintf_s(parse_out->line[parse_out->index].line, "auipc  x%d, 0x%x ", number2, (number1 + 0x800) & (~0x0fff));
									sprintf_s(parse_out->line[parse_out->index].line, "jalr zero, 0x%03x(x%d)  ", number1 & (0x0fff), number2);
								code = 0x000017 | (number2 << 7) | ((number1 + 0x800) & (~0x0fff) << 12);
								page[page_index].memory[memory_index + 1] = 0x000067 | (number2 << 15) | (number1 & (0x0fff) << 20);
								parse_in->index++;
								debug++;
							}
						}
						break;
					case 'l':
						if (parse_in->line[parse_in->index].line[parse_in->ptr+1] == 'i' && parse_in->line[parse_in->index].line[parse_in->ptr+2] == ' ') {
							strcat_p2(parse_out, (char*)"lui ");
							code = 0x0037;

							UINT8 number2;
							print_rd(&code, parse_out, parse_in);

							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							if (get_integer(&number1, parse_in->word)) {
								if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "%#x ", number1);
								code |= number1 & ~((UINT64)0x0FFF);
							}
							else {
								debug++;
							}

							if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "\n");
							if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "0x%016I64x ", number0 + 4);

							if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "addi ");
							page[page_index].memory[memory_index + 1] = 0x0013;

							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							get_reg_number_EABI(&number2, parse_in->word);
							if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "x%d, ", number2);
							page[page_index].memory[memory_index + 1] |= number2 << 7;

							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							get_reg_number_EABI(&number2, parse_in->word);
							if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "x%d, ", number2);
							page[page_index].memory[memory_index + 1] |= number2 << 15;

							getWord(parse_in);
							if (get_integer(&number1, parse_in->word)) {
								if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "%#x ", number1 & ((UINT64)0x0FFF));
								page[page_index].memory[memory_index + 1] |= (number1 & ((UINT64)0x0FFF)) << 20;
							}
							else {
								debug++;
							}
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr+1] == 'u' && parse_in->line[parse_in->index].line[parse_in->ptr+2] == 'i' && parse_in->line[parse_in->index].line[parse_in->ptr+3] == ' ') {
							strcat_p2(parse_out, (char*)"lui ");
							code = 0x0037;

							UINT8 number2;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							print_rd(&code, parse_out, parse_in);

							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ',' || parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							if (get_integer(&number1, parse_in->word)) {
								sprintf_s(word, "0x%x ", number1);
								code |= number1 << 12;
							}
							else if (find_labelB(&number1, parse_in->word, labels, label_count)) {// auipc
								if (number1 & 0x800) {
									number1 = number1 >> 12;
									number1++;
								}
								else {
									number1 = number1 >> 12;
								}
								sprintf_s(word, "0x%x ", number1);
								code |= number1 << 12;
							}
							else {
								debug++;
							}
							strcat_p2(parse_out, word);
						}
						else {
							switch (parse_in->line[parse_in->index].line[parse_in->ptr+1]) {
							case 'r': {//lr.x.aq.rl
								code = 0x1000002f;
								if (parse_in->line[parse_in->index].line[parse_in->ptr+3] == 'w') {
									code |= 0x2000;
								}
								else if (parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'd') {
									code |= 0x3000;
								}
								else {
									debug++;
								}
								if (parse_in->line[parse_in->index].line[parse_in->ptr + 4] == '.') {// lock
									if (parse_in->line[parse_in->index].line[parse_in->ptr + 5] == 'a' && parse_in->line[parse_in->index].line[parse_in->ptr + 6] == 'q') {
										code |= (1 << 26);
										if (parse_in->line[parse_in->index].line[parse_in->ptr + 7] == '.' && parse_in->line[parse_in->index].line[parse_in->ptr + 8] == 'r' && parse_in->line[parse_in->index].line[parse_in->ptr + 9] == 'l') {
											code |= (1 << 25);
										}
										else if (parse_in->line[parse_in->index].line[parse_in->ptr + 7] == ' ') {//no lock release
										}
										else {
											debug++;//syntax
										}
									}
									else if (parse_in->line[parse_in->index].line[parse_in->ptr + 5] == 'r' && parse_in->line[parse_in->index].line[parse_in->ptr + 6] == 'l') {
										code |= (1 << 25);
									}
									else {
										debug++;//syntax
									}
								}
								else if (parse_in->line[parse_in->index].line[parse_in->ptr + 4] == ' ') {//no lock
								}
								else {
									debug++;//syntax
								}
							}
									break;
							case 'q': {// lq
								code = 0x0000200f;
				//				if (parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'w') {
				//					code |= 0x2000;
				//				}
				//				else if (parse_in->line[parse_in->index].line[parse_in->ptr + 3] == 'd') {
				//					code |= 0x3000;
				//				}
								if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'u') {
									code |= 0x1000;
								}
							}
								break;
							case 'f': {
								code = 0x07;
							}
									break;
							case 'b': // lb
								code = 0x0003;
								if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'u') {
									code |= 0x4000;
								}
								break;
							case 'h':// lh
								code = 0x1003;
								if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'u') {
									code |= 0x4000;
								}
								break;
							case 'w':// lw
								code = 0x2003;
								if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'u') {
									code |= 0x4000;
								}
								break;
							case 'd':
								code = 0x3003;
								if (parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'u') {
									code |= 0x4000;
								}
								break;
							default:
								debug++;
								break;
							}
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							strcat_p2(parse_out, parse_in->word);
							print_rd(&code, parse_out, parse_in); // rd

							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ',' || parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							short number2;
							get_shortB(&number2, parse_in->word);// r1
							code |= ((number2) << 20);

							while (parse_in->line[parse_in->index].line[parse_in->ptr] == '(' || parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							UINT8 number3;
							get_reg_number_EABI(&number3, parse_in->word); // imm (offset)
							sprintf_s(word, "x%d, ", number3);
							strcat_p2(parse_out, word);
							sprintf_s(word, "%d ", number2);
							strcat_p2(parse_out, word);
							code |= number3 << 15;
						}
						break;
					case 'm':
						if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == 'u' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] == 'l' && parse_in->line[parse_in->index].line[parse_in->ptr + 3] == ' ') {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "%s ", parse_in->word);
							code = 0x02000033;
							print_rd(&code, parse_out, parse_in);

							print_rs1(&code, parse_out, parse_in);
							print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						}
						else {
							debug++;
						}
						break;
					case 'n':
						if (parse_in->line[parse_in->index].line[parse_in->ptr+1] == 'o' && parse_in->line[parse_in->index].line[parse_in->ptr+2] == 'p') {
							strcat_p2(parse_out, (char*)"addi zero, zero, 0 // NOP");
							code = 0x0013;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);
						}
						break;
					case 'o': {
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						getWord(parse_in);
						strcat_p2(parse_out, parse_in->word);
						UINT number32;
						if (get_asm_numberB(&number32, parse_in->word)) {
							code = number32 | 0x08;// 08 for 64b vs 32b
						}
						else {
							code = 0;
						}
						// need to match instruction to generate binary
						print_rd(&code, parse_out, parse_in);
						print_rs1(&code, parse_out, parse_in);
						print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
					}
							break;
					case 'p':
						if (parse_in->line[parse_in->index].line[parse_in->ptr+1] == 'r' && parse_in->line[parse_in->index].line[parse_in->ptr+2] == 'i' && parse_in->line[parse_in->index].line[parse_in->ptr+3] == 'n' && parse_in->line[parse_in->index].line[parse_in->ptr+4] == 't' && parse_in->line[parse_in->index].line[parse_in->ptr+5] == ' ') {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							if (find_labelB(&number1, parse_in->word, labels, label_count)) {
								if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "%#010x ", number1);
								code = number1;// 
							}
						}
						else {
							debug++;
						}
						break;
					case 's':
						switch (parse_in->line[parse_in->index].line[parse_in->ptr+1]) {
						case 'b':
						case 'h':
						case 'w':
						case 'd':
						case 'q': {//fs

							char size_hold = parse_in->line[parse_in->index].line[parse_in->ptr + 1];
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
//							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
//							getWord(parse_in);

							if (strncmp(parse_in->word, "label", 5) == 0) {
								INT64 target_addr;
								if (find_labelB(&target_addr, parse_in->word, labels, label_count)) {
									UINT index0 = (target_addr & 0x800) ? (-(target_addr & 0x0fff)) | 0x800 : (target_addr & 0x0fff);
									UINT64 carry = (target_addr & 0x800) << 1;
									UINT index1 = (((target_addr + carry) >> 12) & 0x0fffff);
									carry = (target_addr & 0x80000000) << 1;

									UINT index2 = (target_addr & 0x80000000000) ? (UINT)((-1 * (INT)(((target_addr + carry) >> 32) & 0x0fff)) | 0x800) : (((target_addr + carry) >> 32) & 0x0fff);
									carry = (target_addr & 0x80000000000) ? 1 : 0;
									UINT index3 = (((target_addr + carry) >> 44) & 0x0fffff); // only valid to 64b

									if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "lui t6, %d\n", index3); number0 += 4;
									code = 0x37;//LUI
									code |= (0x1f << 7);//t6
									code |= (index3 << 12);//data
									memory_index++;

									if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "0x%016I64x addi t6, t6, %d\n", number0, index2); number0 += 4;
									if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "0x%016I64x slli t6, t6, 32\n", number0); number0 += 4;
									if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "0x%016I64x lui t5, %d\n", number0, index1); number0 += 4;
									code = 0x37;//LUI
									code |= (0x1e << 7);//t6
									code |= (index1 << 12);//data
									memory_index++;

									if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "0x%016I64x addi t5, t5, %d\n", number0, index0); number0 += 4;
									if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "0x%016I64x add t6, t6, t5\n", number0); number0 += 4;
								}
								else {
									debug++;//syntax
								}


								if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "0x%016I64x ", number0);
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "%s ", parse_in->word);
								code = 0x23;
								switch (size_hold) {
								case 'b':
									if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "sb ");
									code |= 0x0000;
									break;
								case 'h':
									if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "sh ");
									code |= 0x1000;
									break;
								case 'w':
									if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "sw ");
									code |= 0x2000;
									break;
								case 'd':
									if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "sd ");
									code |= 0x3000;
									break;
								default:
									debug++;//syntax
									break;
								}
								if (print_asm == 1) sprintf_s(parse_out->line[parse_out->index].line, "t6, 0(tp) ");

							}
							else {
								code = 0x23;
								switch (size_hold) {
								case'b':
									strcat_p2(parse_out,(char*)"sb ");
									code |= 0x0000;
									break;
								case'h':
									strcat_p2(parse_out, (char*)"sh ");
									code |= 0x1000;
									break;
								case'w':
									strcat_p2(parse_out, (char*)"sw ");
									code |= 0x2000;
									break;
								case'd':
									strcat_p2(parse_out, (char*)"sd ");
									code |= 0x3000;
									break;
								case'q':
									strcat_p2(parse_out, (char*)"sq ");
									code |= 0x4000;
									break;
								default:
									debug++;//syntax error
									break;
								}
								print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);

								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ',' || parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								INT64 number2;
								get_integer(&number2, parse_in->word);
								sprintf_s(word, ", 0x%03x ", number2);
								strcat_p2(parse_out, word);
								code |= ((number2 & 0x0fe0) << 20) | ((number2 & 0x001f) << 7);

								while (parse_in->line[parse_in->index].line[parse_in->ptr] == '(' || parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								UINT8 number3;
								get_reg_number_EABI(&number3, parse_in->word);
								sprintf_s(word, "(x%d) ", number3);
								strcat_p2(parse_out, word);
								code |= number3 << 15;
							}
						}
							break;
						case 'c': {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							code = 0x1800002f;
							if (parse_in->line[parse_in->index].line[parse_in->ptr+3] == 'w') {//32b
								code |= 2 << 12;
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr+3] == 'd') {//64b
								code |= 3 << 12;
							}
							else {
								debug++;
							}
							if (parse_in->line[parse_in->index].line[parse_in->ptr+4] == '.') {// lock
								code |= 3 << 25;
							}
							getWord(parse_in);
							strcat_p2(parse_out, parse_in->word);
							print_rd(&code, parse_out, parse_in);
							print_rs1(&code, parse_out, parse_in);
							print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);
						}
							break;
						case 'r': {// shift right
							if (parse_in->line[parse_in->index].line[parse_in->ptr+2] == 'a') { // arithmetic
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								strcat_p2(parse_out, parse_in->word);
								parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';;
								parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
								UINT number32;
								if (get_asm_numberB(&number32, parse_in->word)) {
									code = number32 | 0x08;// 08 for 64b vs 32b
								}
								else {
									code = 0;
								}
								print_rd(&code, parse_out, parse_in);

								print_rs1(&code, parse_out, parse_in);
								print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr+2] == 'l') { // logcal
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								getWord(parse_in);
								strcat_p2(parse_out, parse_in->word);
								UINT number32;
								if (get_asm_numberB(&number32, parse_in->word)) {
									code = number32 | 0x08;// 08 for 64b vs 32b
								}
								else {
									code = 0;
								}
								print_rd(&code, parse_out, parse_in);
								print_rs1(&code, parse_out, parse_in);
								print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							}
						}
							break;
						default: {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							getWord(parse_in);
							strcat_p2(parse_out, parse_in->word);
							UINT number32;
							if (get_asm_numberB(&number32, parse_in->word)) {
								code = number32 | 0x08;// 08 for 64b vs 32b
							}
							else {
								code = 0;
							}
							print_rd(&code, parse_out, parse_in);
							print_rs1(&code, parse_out, parse_in);
							print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						}
							break;
						}
						break;
					case 'w':
						if (parse_in->line[parse_in->index].line[parse_in->ptr+1] == 'f' && parse_in->line[parse_in->index].line[parse_in->ptr+2] == 'i') {
							code = 0x00000073;// SYSTEM
							code |= 0x10500000;// wfi - wait for interrupt
							strcat_p2(parse_out,(char*) "wfi ");
						}
						else {
							debug++;
						}
						break;
					case 'x': {
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
						getWord(parse_in);
						strcat_p2(parse_out, parse_in->word);
						UINT number32;
						if (get_asm_numberB(&number32, parse_in->word)) {
							code = number32 | 0x08;// 08 for 64b vs 32b
						}
						else {
							code = 0;
						}
						print_rd(&code, parse_out, parse_in);
						print_rs1(&code, parse_out, parse_in);
						print_rs2(&code, parse_out, parse_in, &number1, labels, label_count);
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
					}
							break;
					default:
						debug++;
						break;
					}
					if (print_tail) {
						sprintf_s(word, "// ");
						strcat_p2(parse_out, word);
						sprint_addr_p2(parse_out, number0, param);
						sprintf_s(word, " DW 0x%08x\n", code);//DD - 64b, DW - 32b, DH - 16b
						strcat_p2(parse_out, word);
						if (shift_block32 == 0) {
							page[page_index].memory[memory_index] &= 0XFFFFFFFF00000000;
							page[page_index].memory[memory_index] |= (code & 0X00000000FFFFFFFF);
						}
						else {
							page[page_index].memory[memory_index] &= 0X00000000FFFFFFFF;
							page[page_index].memory[memory_index] |= (code << 32);
						}
					}
					debug++;
				}
			}
		}
	}
	if (print_asm == 1) {
		FILE* dst;
		fopen_s(&dst, dst_name, "w");
		print_parse_out(dst, parse_out);
		fclose(dst);
	}
	debug++;
	//	free(labels);
}