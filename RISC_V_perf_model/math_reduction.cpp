// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

// need to recode, evaluate every bracket pair, not just inner most
// * detect each '(' set depth
// check inner operator (*,+,- are ok, << must keep parenthesis)

UINT8 exec_switch(parse_struct2* parse_out, parse_struct2* parse_in, INT multiplier, int* depth);

op_type decode_op(char* parse_in, UINT* ptr) {
	op_type op = none_op;
	switch (parse_in[ptr[0]]) {
	case '+':
		if (parse_in[ptr[0] + 1] != '+' && parse_in[ptr[0] + 1] != '=') {
			op = add_op;
			ptr[0]++;
		}
		break;
	case '-':
		if (parse_in[ptr[0] + 1] != '-' && parse_in[ptr[0] + 1] != '=') {
			op = sub_op;
			ptr[0]++;
		}
		break;
	case '*':
		if (parse_in[ptr[0] + 1] != '=') {
			op = mul_op;
			ptr[0]++;
		}
		break;
	case '<':
		if (parse_in[ptr[0] + 1] == '<' && parse_in[ptr[0] + 2] != '=') {
			op = sl_op;
			ptr[0] += 2;
		}
		break;
	case '>':
		if (parse_in[ptr[0] + 1] == '>' && parse_in[ptr[0] + 2] != '=') {
			op = sr_op;
			ptr[0] += 2;
		}
		break;
	case '=':
		if (parse_in[ptr[0] + 1] == '=') {
			op = eq_log;
			ptr[0] += 2;
		}
		break;
	case '|':
		if (parse_in[ptr[0] + 1] == '|') {
			op = or_log;
			ptr[0] += 2;
		}
		else if (parse_in[ptr[0] + 1] != '=') {
			op = or_op;
			ptr[0]++;
		}
		break;
	case '^':
		if (parse_in[ptr[0] + 1] != '^' && parse_in[ptr[0] + 1] != '=') {
			op = xor_op;
			ptr[0]++;
		}
		break;
	case '&':
		if (parse_in[ptr[0] + 1] == '&') {
			op = and_log;
			ptr[0] += 2;
		}
		else if (parse_in[ptr[0] + 1] != '=') {
			op = and_op;
			ptr[0]++;
		}
		break;
	default:
		break;
	}
	return op;
}
char* op2word(op_type op) {
	char word[8];
	switch (op) {
	case add_op:
		sprintf_s(word, "+");
		break;
	case sub_op:
		sprintf_s(word, "-");
		break;
	case mul_op:
		sprintf_s(word, "*");
		break;
	case and_op:
		sprintf_s(word, "&");
		break;
	case or_op:
		sprintf_s(word, "|");
		break;
	case xor_op:
		sprintf_s(word, "^");
		break;
	case sl_op:
		sprintf_s(word, "<<");
		break;
	case sr_op:
		sprintf_s(word, ">>");
		break;
	case and_log:
		sprintf_s(word, "&&");
		break;
	case or_log:
		sprintf_s(word, "||");
		break;
	case eq_log:
		sprintf_s(word, "==");
		break;
	default:
		word[0] = '\0';
		break;
	}
	return word;
}

void copy_ptr_char(char* parse_out, UINT* parse_out_ptr, char* parse_in, UINT* parse_in_ptr) {
	parse_out[parse_out_ptr[0]++] = parse_in[parse_in_ptr[0]++];
	parse_out[parse_out_ptr[0]] = '\0';
}
void copy_ptr_char(char* parse_out, UINT* parse_out_ptr, char letter) {
	parse_out[parse_out_ptr[0]++] = letter;
	parse_out[parse_out_ptr[0]] = '\0';
}

void strcpy_word(char* out, parse_struct2* parse) {
	for (UINT8 i = 0; i < parse->word_ptr; i++) {
		out[i] = parse->word[i];
	}
	out[parse->word_ptr] = '\0';
}

UINT8 blank_space_formating(char* parse_out, char* parse_in) {
	UINT8 touched = 0;
	int debug = 0;
	UINT parse_in_ptr = 0;
	UINT parse_out_ptr = 0;
	while (parse_in[parse_in_ptr] == ' ' || parse_in[parse_in_ptr] == '\t') copy_ptr_char(parse_out, &parse_out_ptr, parse_in, &parse_in_ptr);
	while (parse_in[parse_in_ptr] != '\0') {
		if (parse_in[parse_in_ptr] == '/' && parse_in[parse_in_ptr + 1] == '/') {
			while (parse_in[parse_in_ptr] != '\0')
				copy_ptr_char(parse_out, &parse_out_ptr, parse_in, &parse_in_ptr);
		}
		else if (parse_in[parse_in_ptr] == '\"') {
			copy_ptr_char(parse_out, &parse_out_ptr, parse_in, &parse_in_ptr);
			while (parse_in[parse_in_ptr] != '\"' && parse_in[parse_in_ptr] != '\0')
				copy_ptr_char(parse_out, &parse_out_ptr, parse_in, &parse_in_ptr);
		}
		else if (parse_in[parse_in_ptr] == ' ' || parse_in[parse_in_ptr] == '\t') {
			copy_ptr_char(parse_out, &parse_out_ptr, parse_in, &parse_in_ptr);
			while (parse_in[parse_in_ptr] == ' ' || parse_in[parse_in_ptr] == '\t') {
				parse_in++;
			}
			if (parse_in[parse_in_ptr] == ']' || parse_in[parse_in_ptr] == ')') {
				parse_out--;
				copy_ptr_char(parse_out, &parse_out_ptr, parse_in, &parse_in_ptr);
			}
		}
		else if (parse_in[parse_in_ptr] == '[' || parse_in[parse_in_ptr] == '(') {
			copy_ptr_char(parse_out, &parse_out_ptr, parse_in, &parse_in_ptr);
			while (parse_in[parse_in_ptr] == ' ' || parse_in[parse_in_ptr] == '\t') {
				parse_in++;
			}
		}
		else {
			copy_ptr_char(parse_out, &parse_out_ptr, parse_in, &parse_in_ptr);
		}
		if (parse_in_ptr >= 0x200 || parse_out_ptr >= 0x200)
			debug++;
	}
	return touched;
}
/*
void getWordM0(char* word, char* parse_in, UINT* ptr) {
	UINT debug = 0;
	UINT16 length = strlen(parse_in);
	UINT16 i;
	if (ptr[0] >= length) {
		debug++;
	}
	else {

		for (i = ptr[0]; i < length && debug == 0; i++) {
			switch (parse_in[i]) {
			case ' ':
			case '\t':
			case '\0':
			case '\n':
			case '\"':
			case ',':
			case ';':
			case ':':
			case '=':
			case '?':
			case '&':
			case '|':
			case '^':
			case '+':
			case '-':
			case '*':
			case '/':
			case '<':
			case '>':
			case '(':
			case ')':
			case '{':
			case '}':
			case '[':
			case ']':
				debug++;
				break;
			default:
				word[i - ptr[0]] = parse_in[i];
				break;
			}
		}
		word[i - ptr[0] - 1] = '\0';
		ptr[0] = (i - 1);
	}

}
void getWordM(char* word, char* parse_in, UINT* ptr) {
	UINT debug = 0;
	UINT16 length = strlen(parse_in);
	UINT16 i;
	if (ptr[0] >= length) {
		debug++;
	}
	else {
		while (parse_in[ptr[0]] == '=' || parse_in[ptr[0]] == ' ' || parse_in[ptr[0]] == '\"' || parse_in[ptr[0]] == '\t')ptr[0]++;

		for (i = ptr[0]; i < length && debug == 0; i++) {
			switch (parse_in[i]) {
			case ' ':
			case '\t':
			case '\0':
			case '\n':
			case '\"':
			case ',':
			case ';':
			case ':':
			case '=':
			case '?':
			case '&':
			case '|':
			case '^':
			case '+':
			case '-':
			case '*':
			case '/':
			case '<':
			case '>':
			case '(':
			case ')':
			case '{':
			case '}':
			case '[':
			case ']':
				debug++;
				break;
			default:
				word[i - ptr[0]] = parse_in[i];
				break;
			}
		}
		word[i - ptr[0] - 1] = '\0';
		ptr[0] = (i - 1);
	}

}
/**/
// index is present for debug purposes
struct var_entry_type {
	char name[0x80];
	int coef;
	UINT8 addr;
};
struct var_list_type {
	var_entry_type entry[0x10];
	char count;
};
UINT8 simple_variable_math(parse_struct2* parse_out, parse_struct2* parse_in) {
	char touch = 0;

	int debug = 0;

	char neg_flag = 0;
	char mul_flag = 0;
	UINT8 addr_flag = 0;
	INT64 number1 = 0;

	var_list_type var_list;
	var_list.count = 0;

	parse_in->ptr = 0;
	parse_out->ptr = 0;

	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 
		parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') 
				parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
		}
		else {
			getWord(parse_in);
			if (parse_in->word_ptr != 0) {
				if (get_integer(&number1, parse_in->word)) {
					strcat_p2(parse_out, parse_in->word);
				}
				else if (strcmp(parse_in->word, "_fp16") == 0 || strcmp(parse_in->word, "UINT16") == 0) {
					strcat_p2(parse_out, parse_in->word);
				}
				else {
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
						int ptr = strlen(parse_in->word);
						while (parse_in->line[parse_in->index].line[parse_in->ptr] != ']') {
							parse_in->word[ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
						}
						parse_in->word[ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
						parse_in->word[ptr] = '\0';
					}
					strcat_p2(parse_out, parse_in->word);
					if (neg_flag) {
						if (mul_flag)
							var_list.entry[var_list.count].coef = -number1;
						else
							var_list.entry[var_list.count].coef = -1;
					}
					else {
						if (mul_flag)
							var_list.entry[var_list.count].coef = number1;
						else
							var_list.entry[var_list.count].coef = 1;
					}
					var_list.entry[var_list.count].addr = addr_flag;
					if (mul_flag == 0 || number1 != 0) {
						for (UINT8 i = 0; i < var_list.count; i++) {
							if (strcmp(var_list.entry[i].name, parse_in->word) == 0) {
								touch = 1;
								debug++;
								parse_out->ptr = var_list.entry[i].addr;
								if (var_list.entry[var_list.count].coef + var_list.entry[i].coef == 0) {

								}
								else if (var_list.entry[var_list.count].coef + var_list.entry[i].coef < 0) {
									parse_out->line[parse_out->index].line[parse_out->ptr++] = '-';
									parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
									parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
									strcat_p2(parse_out, var_list.entry[i].name);
								}
								else {
									parse_out->line[parse_out->index].line[parse_out->ptr++] = '+';
									parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
									parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
									strcat_p2(parse_out, var_list.entry[i].name);
								}
							}
						}
					}
			//		sprintf_s(var_list.entry[var_list.count++].name, "%s", parse_in->word);
					strcpy_word(var_list.entry[var_list.count++].name, parse_in);
				}
			}
			else {
				if ((parse_in->line[parse_in->index].line[parse_in->ptr] == '|' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '|') ||
					(parse_in->line[parse_in->index].line[parse_in->ptr] == '&' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '&') ||
					(parse_in->line[parse_in->index].line[parse_in->ptr] == '|' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=')) {
					neg_flag = 0;
					mul_flag = 0;
					addr_flag = parse_in->ptr;
					var_list.count = 0;
					parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ';' || parse_in->line[parse_in->index].line[parse_in->ptr] == '=' ||
					parse_in->line[parse_in->index].line[parse_in->ptr] == '&' || parse_in->line[parse_in->index].line[parse_in->ptr] == ',' ||
					parse_in->line[parse_in->index].line[parse_in->ptr] == '(' || parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
					neg_flag = 0;
					mul_flag = 0;
					addr_flag = parse_in->ptr;
					var_list.count = 0;
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+' || parse_in->line[parse_in->index].line[parse_in->ptr] == '|') {
					neg_flag = 0;
					mul_flag = 0;
					addr_flag = parse_in->ptr;
					number1 = 1;
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '-') {
					neg_flag = 1;
					mul_flag = 0;
					addr_flag = parse_in->ptr;
					number1 = 1;
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*')
					mul_flag = 1;
				parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
				parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
			}
		}
	}
	return touch;
}
UINT8 simple_mult(parse_struct2* parse_out, parse_struct2* parse_in) {
	UINT8 touched = 0;
	int debug = 0;
	parse_in->ptr = parse_out->ptr = 0;
	INT64 number1, number2;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 
		parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
	parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') 
				parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
			parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
		}
		else {
			getWord( parse_in);
			char word1[0x100];
//			sprintf_s(word1, "%s", parse_in->word);
			strcpy_word(word1, parse_in);
			if (get_integer(&number1, parse_in->word)) {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 
					parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
				switch (decode_op(parse_in->line[parse_in->index].line, &parse_in->ptr)) {
				case mul_op:
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 
						parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
					getWord( parse_in);
					if (get_integer(&number2, parse_in->word)) {
						char  word_out[0x100];
						sprintf_s(word_out, "0x%x", number1 * number2);
						strcat_p2(parse_out, word_out);
						strcat_p2(parse_out,(char*) " ");
						touched = 1;
					}
					else {
						strcat_p2(parse_out, word1);
						strcat_p2(parse_out, (char*)" * ");
						strcat_p2(parse_out, parse_in->word);
					}
					break;
				case sr_op:
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 
						parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
					getWord( parse_in);
					if (get_integer(&number2, parse_in->word)) {
						char  word_out[0x100];
						sprintf_s(word_out, "0x%x", number1 >> number2);
						strcat_p2(parse_out, word_out);
						strcat_p2(parse_out, (char*)" ");
						touched = 1;
					}
					else {
						strcat_p2(parse_out, word1);
						strcat_p2(parse_out, (char*)" >> ");
						strcat_p2(parse_out, parse_in->word);
					}
					break;
				case sl_op:
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
						parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
					getWord( parse_in);
					if (get_integer(&number2, parse_in->word)) {
						char  word_out[0x100];
						sprintf_s(word_out, "0x%x", number1 << number2);
						strcat_p2(parse_out, word_out);
						strcat_p2(parse_out, (char*)" ");
						touched = 1;
					}
					else {
						strcat_p2(parse_out, word1);
						strcat_p2(parse_out, (char*)" << ");
						strcat_p2(parse_out, parse_in->word);
					}
					break;
				case or_op:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" |");
					break;
				case xor_op:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" ^");
					break;
				case and_op:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" &");
					break;
				case add_op:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" +");
					break;
				case sub_op:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" -");
					break;
				case eq_log:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" ==");
					break;
				case or_log:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" ||");
					break;
				case and_log:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" &&");
					break;
				default:
					strcat_p2(parse_out, word1);
					break;
				}
			}
			else if (word1[0]!='\0') {
				strcat_p2(parse_out, word1);
			}
			else {
				parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
				parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
			}
		}
		if (parse_in->ptr >= 0x200 || parse_out->ptr >= 0x200)
			debug++;
	}
	return touched;
}
UINT8 simple_add2(parse_struct2 *parse_out, parse_struct2 *parse_in) {
	UINT8 touched = 0;
	int debug = 0;
	parse_in->ptr = parse_out->ptr = 0;
//	char word1[0x100], word2[0x100], word_out[0x100];
	INT64 number1, number2;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 
		parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
	parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';

	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') 
				parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
			parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
		}
		else {
			getWord(parse_in);
			char word1[0x100];
	//		sprintf_s(word1, "%s", parse_in->word);
			strcpy_word(word1, parse_in);
			if (get_integer(&number1, parse_in->word)) {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 
					parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
				switch (decode_op(parse_in->line[parse_in->index].line, &parse_in->ptr)) {
				case add_op:
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
						parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
					getWord(parse_in);
					if (get_integer(&number2, parse_in->word)) {
						char  word_out[0x100];
						sprintf_s(word_out, "0x%x", number1 + number2);
						strcat_p2(parse_out, word_out);
						strcat_p2(parse_out, (char*) " ");
						touched = 1;
					}
					else {
						strcat_p2(parse_out, word1);
						strcat_p2(parse_out, (char*)" + ");
						strcat_p2(parse_out, parse_in->word);
					}
					break;
				case or_op:
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 
						parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
					getWord(parse_in);
					if (get_integer(&number2, parse_in->word)) {
						char  word_out[0x100];
						sprintf_s(word_out, "0x%x", number1 | number2);
						strcat_p2(parse_out, word_out);
						strcat_p2(parse_out, (char*)" ");
						touched = 1;
					}
					else {
						strcat_p2(parse_out, word1);
						strcat_p2(parse_out, (char*)" | ");
						strcat_p2(parse_out, parse_in->word);
					}
					break;
				case xor_op:
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 
						parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
					getWord(parse_in);
					if (get_integer(&number2, parse_in->word)) {
						char  word_out[0x100];
						sprintf_s(word_out, "0x%x", number1 ^ number2);
						strcat_p2(parse_out, word_out);
						strcat_p2(parse_out, (char*)" ");
						touched = 1;
					}
					else {
						strcat_p2(parse_out, word1);
						strcat_p2(parse_out, (char*)" ^ ");
						strcat_p2(parse_out, parse_in->word);
					}
					break;
				case and_op:
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 
						parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
					getWord(parse_in);
					if (get_integer(&number2, parse_in->word)) {
						char  word_out[0x100];
						sprintf_s(word_out, "0x%x", number1 & number2);
						strcat_p2(parse_out, word_out);
						strcat_p2(parse_out, (char*)" ");
						touched = 1;
					}
					else {
						strcat_p2(parse_out, word1);
						strcat_p2(parse_out, (char*)" & ");
						strcat_p2(parse_out, parse_in->word);
					}
					break;
				case mul_op:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" *");
					break;
				case sub_op:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" -");
					break;
				case sr_op:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" >>");
					break;
				case sl_op:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" <<");
					break;
				case eq_log:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" ==");
					break;
				case or_log:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" ||");
					break;
				case and_log:
					strcat_p2(parse_out, word1);
					strcat_p2(parse_out, (char*)" &&");
					break;
				default:
					strcat_p2(parse_out, word1);
					break;
				}
			}
			else if (word1[0] != '\0') {
				strcat_p2(parse_out, word1);
			}
			else {
				parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
				parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
			}
		}
		if (parse_in->ptr >= 0x200 || parse_out->ptr >= 0x200)
			debug++;
	}
	return touched;
}
UINT8 simple_logic(parse_struct2* parse_out, parse_struct2* parse_in) {
	UINT8 touched = 0;
	int debug = 0;
//	UINT parse_in->ptr = 0, parse_out->ptr = 0;
	char  word1[0x100];
	INT64 number1, number2;
	UINT8 op_flag = 0;
	parse_in->ptr = parse_out->ptr = 0;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') copy_char(parse_out, parse_in);
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') copy_char(parse_out, parse_in);
		}
		else {
			getWord(parse_in);
	//		sprintf_s(word1, "%s", parse_in->word);
			strcpy_word(word1, parse_in);
			if (get_integer(&number1, parse_in->word) && op_flag == 0) {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') parse_in->ptr++;
				switch (decode_op(parse_in->line[parse_in->index].line, &parse_in->ptr)) {
				case eq_log:
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') copy_char(parse_out, parse_in);
					getWord( parse_in);
					if (get_integer(&number2, parse_in->word)) {
						char  word1[0x100], word_out[0x100];
						sprintf_s(word_out, "%d", (number1 == number2) ? 1 : 0);
						strcat_p2(parse_out, word_out);
						strcat_p2(parse_out, (char*)" ");
						touched = 1;
					}
					else {
						strcat_p2(parse_out, word1);
						strcat_p2(parse_out, (char*)" == ");
						strcat_p2(parse_out, parse_in->word);
					}
					break;
				case or_log:
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') copy_char(parse_out, parse_in);
					getWord(parse_in);
					if (get_integer(&number2, parse_in->word)) {
						char  word1[0x100], word_out[0x100];
						sprintf_s(word_out, "%d", (number1 || number2) ? 1 : 0);
						strcat_p2(parse_out, word_out);
						strcat_p2(parse_out, (char*)" ");
						touched = 1;
					}
					else if (number1 == 0) {
						strcat_p2(parse_out, parse_in->word);
					}
					else if (number1 == 1) {
						touched = 1;
						strcat_p2(parse_out, (char*)"1");
						if (parse_in->word[0]=='\0') {
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
								UINT depth = 0;
								parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] != ')' || depth > 0) {
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(')
										depth++;
									else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
										depth--;
									parse_in->ptr++;
								}
								parse_in->ptr++;
							}
							else {
								debug++;
							}
						}
					}
					else {
						strcat_p2(parse_out, word1);
						strcat_p2(parse_out, (char*)" || ");
						strcat_p2(parse_out, parse_in->word);
					}
					break;
				case and_log:
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') copy_char(parse_out, parse_in);
					getWord(parse_in);
					if (get_integer(&number2, parse_in->word)) {
						char  word1[0x100], word_out[0x100];
						sprintf_s(word_out, "%d", (number1 && number2) ? 1 : 0);
						strcat_p2(parse_out, word_out);
						strcat_p2(parse_out, (char*)" ");
						touched = 1;
					}
					else {
						strcat_p2(parse_out, word1);
						strcat_p2(parse_out, (char*)" && ");
						strcat_p2(parse_out, parse_in->word);
					}
					break;
				case add_op:
					strcat_p2(parse_out, parse_in->word);
					strcat_p2(parse_out, (char*)" +");
					break;
				case or_op:
					strcat_p2(parse_out, parse_in->word);
					strcat_p2(parse_out, (char*)" |");
					break;
				case xor_op:
					strcat_p2(parse_out, parse_in->word);
					strcat_p2(parse_out, (char*)" ^");
					break;
				case and_op:
					strcat_p2(parse_out, parse_in->word);
					strcat_p2(parse_out, (char*)" &");
					break;
				case mul_op:
					strcat_p2(parse_out, parse_in->word);
					strcat_p2(parse_out, (char*)" *");
					break;
				case sub_op:
					strcat_p2(parse_out, parse_in->word);
					strcat_p2(parse_out, (char*)" -");
					break;
				case sr_op:
					strcat_p2(parse_out, parse_in->word);
					strcat_p2(parse_out, (char*)" >>");
					break;
				case sl_op:
					strcat_p2(parse_out, parse_in->word);
					strcat_p2(parse_out, (char*)" <<");
					break;
				default:
					strcat_p2(parse_out, parse_in->word);
					break;
				}
				op_flag = 0;
			}
			else if (word1[0] != '\0') {
				strcat_p2(parse_out, parse_in->word);
				op_flag = 0;
			}
			else {
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '&') {
					op_flag = 1;
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] != ' ' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\t') {
					op_flag = 0;
				}
				copy_char(parse_out, parse_in);
			}
		}
		if (parse_in->ptr >= 0x200 || parse_out->ptr >= 0x200)
			debug++;
	}
	return touched;
}
UINT8 add_assoc_var_v_number_ordering2(parse_struct2* parse_out, parse_struct2* parse_in) {
	int debug = 0;
	UINT8 touched = 0;
	parse_in->ptr = parse_out->ptr = 0;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	
		copy_char(parse_out, parse_in);

	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')	
				copy_char(parse_out, parse_in);
		}
		else {
			switch (parse_in->line[parse_in->index].line[parse_in->ptr]) {
			case '\"':
				copy_char(parse_out, parse_in);
				while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\"' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')	
					copy_char(parse_out, parse_in);
				break;
			case ' ':
			case '\t':
				copy_char(parse_out, parse_in);
				break;
			case '=':
				if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
					copy_char(parse_out, parse_in); 
					copy_char(parse_out, parse_in);
				}
				else {
					parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
					parse_out->line[parse_out->index].line[parse_out->ptr++] = '=';
					parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
					parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '-') {
						parse_in->ptr++;
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
							parse_in->ptr++;
						getWord(parse_in);
						if (parse_in->word[0]!='\0') {
							INT64 number1, number2;
							if (get_integer(&number1, parse_in->word)) {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
								if (number1 == 0) {
									debug++; // do nothing, a+0 = a
								}
								else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
									parse_in->ptr++;
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
										parse_in->ptr++;

									touched = 1;
									char temp[0x100];
					//				sprintf_s(temp,"%s", parse_in->word);
									strcpy_word(temp, parse_in);
									getWord(parse_in);
									if (get_integer(&number2, parse_in->word)) {
										if (number1 > number2)
											parse_out->line[parse_out->index].line[parse_out->ptr++] = '-';
										else
											parse_out->line[parse_out->index].line[parse_out->ptr++] = '+';
										parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
										sprintf_s(temp, "0x%x", number2 - number1);
										strcat_p2(parse_out, temp);
									}
									else {
										strcat_p2(parse_out, (char*)"+ ");
										strcat_p2(parse_out, parse_in->word);
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
											while (parse_in->line[parse_in->index].line[parse_in->ptr] != ']')
												parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
											parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
											parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
										}
										strcat_p2(parse_out, (char*)" - ");
										strcat_p2(parse_out, temp);
									}
								}
								else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '-') {
									parse_in->ptr++;
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
										parse_in->ptr++;

									touched = 1;
									char temp[0x100];
							//		sprintf_s(temp,"%s", parse_in->word);
									strcpy_word(temp, parse_in);
									getWord(parse_in);
									if (get_integer(&number2, parse_in->word)) {
										if (number1 > number2) {
											strcat_p2(parse_out, (char*)"+ ");
										}
										else {
											strcat_p2(parse_out, (char*)"- ");
										}
										sprintf_s(temp, "0x%x", number1 - number2);
										strcat_p2(parse_out, temp);
									}
									else {
										strcat_p2(parse_out, (char*)"- ");
										strcat_p2(parse_out, parse_in->word);
										strcat_p2(parse_out, (char*)" + ");
										strcat_p2(parse_out, temp);
									}
								}
								else {
									strcat_p2(parse_out, (char*)"- ");
									strcat_p2(parse_out, parse_in->word);
								}
							}
							else {
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
									parse_in->ptr++;
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
									char temp[0x100];
							//		sprintf_s(temp, "%s", parse_in->word);
									strcpy_word(temp, parse_in);
									getWord(parse_in);
									INT64 number1;
									if (get_integer(&number1, parse_in->word)) {
										strcat_p2(parse_out, (char*)"- ");
										strcat_p2(parse_out, temp);
										strcat_p2(parse_out, (char*)" + ");
										strcat_p2(parse_out, parse_in->word);
									}
									else {
										strcat_p2(parse_out, parse_in->word);
										strcat_p2(parse_out, (char*)" - ");
										strcat_p2(parse_out, temp);
										touched = 1;
									}
								}
								else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
									strcat_p2(parse_out, (char*)"- ");
									strcat_p2(parse_out, parse_in->word);
									copy_char(parse_out, parse_in);
									UINT8 hold_ptr = parse_out->ptr;
									getWord(parse_in);
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
										strcat_p2(parse_out, parse_in->word);
										copy_char(parse_out, parse_in);
									}
									else {
										parse_out->ptr = hold_ptr;
									}
								}
								else {
									strcat_p2(parse_out, (char*)"- ");
									strcat_p2(parse_out, parse_in->word);
									strcat_p2(parse_out, (char*)" ");
								}
							}
						}
						else {
							strcat_p2(parse_out, (char*)"- ");
							copy_char(parse_out, parse_in);
						}
					}
				}
				break;
			case '+':
				if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '+' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
					parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++]; 
					parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
					parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
				}
				else {
					parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
					getWord( parse_in);
					if (parse_in->word_ptr != 0) {
						INT64 number1, number2;
						if (get_integer(&number1, parse_in->word)) {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
							if (number1 == 0) {
								// do nothing, a+0 = a
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
								parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
									parse_in->ptr++;

								char temp[0x100];
								strcpy_word(temp, parse_in);
								strcat_p2(parse_out, (char*)"+ ");
								getWord( parse_in);
								if (get_integer(&number2, parse_in->word)) {
									sprintf_s(temp, "0x%x", number1 + number2);
									strcat_p2(parse_out, temp);
									touched = 1;
								}
								else if (parse_in->word[0] == '\0') {
									strcat_p2(parse_out, temp);
									strcat_p2(parse_out, (char*)" + ");
								}
								else {
									strcat_p2(parse_out, parse_in->word);
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
										while (parse_in->line[parse_in->index].line[parse_in->ptr] != ']')
											copy_char(parse_out, parse_in);
										copy_char(parse_out, parse_in);
									}
									strcat_p2(parse_out, (char*)" + ");
									strcat_p2(parse_out, temp);
									touched = 1;
								}
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '-') {
								parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
									parse_in->ptr++;

								touched = 1;
								char temp[0x100];
						//		sprintf_s(temp,"%s", parse_in->word);
								strcpy_word(temp, parse_in);
								getWord( parse_in);
								if (get_integer(&number2, parse_in->word)) {
									if (number1 > number2) {
										strcat_p2(parse_out, (char*)"+ ");
									}
									else {
										strcat_p2(parse_out, (char*)"- ");
									}
									sprintf_s(temp, "0x%x", number1 - number2);
									strcat_p2(parse_out, temp);
								}
								else {
									strcat_p2(parse_out, (char*)"- ");
									strcat_p2(parse_out, parse_in->word);
									strcat_p2(parse_out, (char*)" + ");
									strcat_p2(parse_out, temp);
								}
							}
							else {
								strcat_p2(parse_out, (char*)"+ ");
								strcat_p2(parse_out, parse_in->word);
							}
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
							strcat_p2(parse_out, (char*)"+ ");
							strcat_p2(parse_out, parse_in->word);
							copy_char(parse_out, parse_in);
							UINT8 hold_ptr = parse_out->ptr;
							getWord(parse_in);
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
								strcat_p2(parse_out, parse_in->word);
								copy_char(parse_out, parse_in);
							}
							else {
								parse_out->ptr = hold_ptr;
							}
						}
						else {
							strcat_p2(parse_out, (char*)"+ ");
							strcat_p2(parse_out, parse_in->word);
						}
					}
					else {
						strcat_p2(parse_out, (char*)"+ ");
						copy_char(parse_out, parse_in);
					}
				}
				break;
			case '-':
				if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '-' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '>') {
					copy_char(parse_out, parse_in);
					copy_char(parse_out, parse_in);
				}
				else {
					parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
						parse_in->ptr++;
					getWord( parse_in);
					if (parse_in->word_ptr != 0) {
						INT64 number1, number2;
						if (get_integer(&number1, parse_in->word)) {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
							if (number1 == 0) {
								debug++; // do nothing, a+0 = a
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
								parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
									parse_in->ptr++;

								touched = 1;
								char temp[0x100];
						//		sprintf_s(temp, "%s", parse_in->word);
								strcpy_word(temp, parse_in);
								getWord( parse_in);
								if (get_integer(&number2, parse_in->word)) {
									if (number1 > number2)
										parse_out->line[parse_out->index].line[parse_out->ptr++] = '-';
									else
										parse_out->line[parse_out->index].line[parse_out->ptr++] = '+';
									parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
									sprintf_s(temp, "0x%x", number2 - number1);
									strcat_p2(parse_out, temp);
								}
								else {
									strcat_p2(parse_out, (char*)"+ ");
									strcat_p2(parse_out, parse_in->word);
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
										while (parse_in->line[parse_in->index].line[parse_in->ptr] != ']') 
											copy_char(parse_out, parse_in);
										copy_char(parse_out, parse_in);
									}
									strcat_p2(parse_out, (char*)" - ");
									strcat_p2(parse_out, temp);
								}
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '-') {
								parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
									parse_in->ptr++;

								touched = 1;
								char temp[0x100];
						//		sprintf_s(temp, "%s", parse_in->word);
								strcpy_word(temp, parse_in);
								getWord( parse_in);
								if (get_integer(&number2, parse_in->word)) {
									if (number1 > number2) {
										strcat_p2(parse_out, (char*)"+ ");
									}
									else {
										strcat_p2(parse_out, (char*)"- ");
									}
									sprintf_s(temp, "0x%x", number1 - number2);
									strcat_p2(parse_out, temp);
								}
								else {
									strcat_p2(parse_out, (char*)"- ");
									strcat_p2(parse_out, parse_in->word);
									strcat_p2(parse_out, (char*)" + ");
									strcat_p2(parse_out, temp);
								}
							}
							else {
								strcat_p2(parse_out, (char*)"- ");
								strcat_p2(parse_out, parse_in->word);
							}
						}
						else {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
								parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								char temp[0x100];
							//	sprintf_s(temp, "%s", parse_in->word);
								strcpy_word(temp, parse_in);
								getWord( parse_in);
								while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')parse_in->ptr++;
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
									strcat_p2(parse_out, (char*)"- ");
									strcat_p2(parse_out, temp);
									strcat_p2(parse_out, (char*)" + ");
									strcat_p2(parse_out, parse_in->word);
								}
								else {
									INT64 number1;
									if (get_integer(&number1, parse_in->word)) {
										strcat_p2(parse_out, (char*)"- ");
										strcat_p2(parse_out, temp);
										strcat_p2(parse_out, (char*)" + ");
										strcat_p2(parse_out, parse_in->word);
									}
									else {
										strcat_p2(parse_out, (char*)"+ ");
										strcat_p2(parse_out, parse_in->word);
										strcat_p2(parse_out, (char*)" - ");
										strcat_p2(parse_out, temp);
										touched = 1;
									}
								}
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
								strcat_p2(parse_out, (char*)"- ");
								strcat_p2(parse_out, parse_in->word);
								copy_char(parse_out, parse_in);
								UINT8 hold_ptr = parse_out->ptr;
								getWord(parse_in);
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
									strcat_p2(parse_out, parse_in->word);
									copy_char(parse_out, parse_in);
								}
								else {
									parse_out->ptr = hold_ptr;
								}
							}
							else {
								strcat_p2(parse_out, (char*)"- ");
								strcat_p2(parse_out, parse_in->word);
								strcat_p2(parse_out, (char*)" ");
							}
						}
					}
					else {
						strcat_p2(parse_out, (char*)"- ");
						copy_char(parse_out, parse_in);
}
				}
				break;
			case '<':
				if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '<') {
					parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++]; 
					parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
						parse_in->ptr++;
					parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
					getWord( parse_in);

					strcat_p2(parse_out, parse_in->word);
				}
				else {
					parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++]; 
					parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
				}
				break;
			case '|':
				parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '|' || parse_in->line[parse_in->index].line[parse_in->ptr] == '=') {
					parse_out->line[parse_out->index].line[parse_out->ptr++] = '|';
					parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++]; 
					parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
				}
				else {
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
					getWord( parse_in);
					if (parse_in->word_ptr != 0) {
						INT64 number1, number2;
						if (get_integer(&number1, parse_in->word)) {
							if (number1 == 0) {
								// do nothing, a|0 = a
							}
							else {
								parse_out->line[parse_out->index].line[parse_out->ptr++] = '|';
								parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
								strcat_p2(parse_out, parse_in->word);
							}
						}
						else {
							parse_out->line[parse_out->index].line[parse_out->ptr++] = '|';
							parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
							strcat_p2(parse_out, parse_in->word);
						}
					}
					else {
						parse_out->line[parse_out->index].line[parse_out->ptr++] = '|';
						parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
						parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++]; 
						parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
					}
				}
				break;
			default:
				getWord(parse_in);
				if (parse_in->word_ptr != 0) {
					INT64 number1, number2, number3;
					if (get_integer(&number1, parse_in->word)) {
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
							parse_in->ptr++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
							parse_in->ptr++;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
								parse_in->ptr++;
							if (number1 == 0) {
								debug++; // do nothing, 0 + 
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
								parse_type parse_t;
								parse_t.ptr = 0;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] != ')')
									parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++]; 
								parse_out->line[parse_out->index].line[parse_out->ptr++] = '+';
								parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
								strcat_p2(parse_out, parse_in->word);
							}
							else {
								char word_hold[0x100];
								strcpy_word(word_hold, parse_in);
								getWord( parse_in);
								if (get_integer(&number2, parse_in->word)) {
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
										parse_in->ptr++;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '<' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '<') {
										parse_in->ptr += 2;
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
											parse_in->ptr++;
										getWord( parse_in);
										if (get_integer(&number3, parse_in->word)) {
											sprintf_s(word_hold, "0x%x", number1 + (number2 << number3));
											strcat_p2(parse_out, word_hold);
										}
										else {
											debug++;// need to code
										}
									}
									else {
										sprintf_s(word_hold, "0x%x", number1 + number2);
										strcat_p2(parse_out, word_hold);
									}
								}
								else {
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
										UINT ptr = strlen(parse_in->word);
										while (parse_in->line[parse_in->index].line[parse_in->ptr] != ']')
											parse_in->word[ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
										parse_in->word[ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
										parse_in->word[ptr] = '\0';
									}
									strcat_p2(parse_out, parse_in->word);
									strcat_p2(parse_out, (char*)" + ");
									strcat_p2(parse_out, word_hold);
								}
							}
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '|' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '|') {
							parse_in->ptr += 2;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
								parse_in->ptr++;
							if (number1 == 0) {
								debug++; // do nothing, 0 + 
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
								parse_type parse_t;
								parse_t.ptr = 0;
								UINT depth = 0;
								parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++]; 
								while (parse_in->line[parse_in->index].line[parse_in->ptr] != ')' || depth > 0) {
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(')
										depth++;
									else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
										depth--;
									parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++]; 
								}
								parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++]; 
								parse_out->line[parse_out->index].line[parse_out->ptr++] = '|';
								parse_out->line[parse_out->index].line[parse_out->ptr++] = '|';
								parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
								strcat_p2(parse_out, parse_in->word);
							}
							else {
								char word_hold[0x100];
					//			sprintf_s(word_hold,"%s", parse_in->word);
								strcpy_word(word_hold, parse_in);
								getWord( parse_in);
								if (get_integer(&number2, parse_in->word)) {
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
										parse_in->ptr++;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '<' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '<') {
										parse_in->ptr += 2;
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
											parse_in->ptr++;
										getWord( parse_in);
										if (get_integer(&number3, parse_in->word)) {
											sprintf_s(word_hold, "0x%x", number1 | (number2 << number3));
											strcat_p2(parse_out, word_hold);
										}
										else {
											debug++;// need to code
										}
									}
									else {
										//										debug++;// need to code
										sprintf_s(word_hold, "0x%x", number1 | number2);
										strcat_p2(parse_out, word_hold);
									}
								}
								else {
									strcat_p2(parse_out, parse_in->word);
									strcat_p2(parse_out, (char*)" || ");
									strcat_p2(parse_out, word_hold);
								}
							}
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '|') {
							parse_in->ptr++;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
								parse_in->ptr++;
							if (number1 == 0) {
								debug++; // do nothing, 0 + 
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
								parse_type parse_t;
								parse_t.ptr = 0;
								UINT depth = 0;
								parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++]; 
								while (parse_in->line[parse_in->index].line[parse_in->ptr] != ')' || depth > 0) {
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(')
										depth++;
									else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
										depth--;
									parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++]; 
								}
								parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++]; 
								parse_out->line[parse_out->index].line[parse_out->ptr++] = '|';
								parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
								strcat_p2(parse_out, parse_in->word);
							}
							else {
								char word_hold[0x100];
							//	sprintf_s(word_hold,"%s", parse_in->word);
								strcpy_word(word_hold, parse_in);
								getWord( parse_in);
								if (get_integer(&number2, parse_in->word)) {
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
										parse_in->ptr++;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '<' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '<') {
										parse_in->ptr += 2;
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
											parse_in->ptr++;
										getWord( parse_in);
										if (get_integer(&number3, parse_in->word)) {
											sprintf_s(word_hold, "0x%x", number1 | (number2 << number3));
											strcat_p2(parse_out, word_hold);
										}
										else {
											debug++;// need to code
										}
									}
									else {
										//										debug++;// need to code
										sprintf_s(word_hold, "0x%x", number1 | number2);
										strcat_p2(parse_out, word_hold);
									}
								}
								else {
									strcat_p2(parse_out, parse_in->word);
									strcat_p2(parse_out, (char*)" | ");
									strcat_p2(parse_out, word_hold);
									touched = 1;
								}
							}
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
							parse_in->ptr++;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
								parse_in->ptr++;
							if (number1 == 0) {
								debug++; // do nothing, 0 + 
							}
							else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
								strcat_p2(parse_out, parse_in->word);
								strcat_p2(parse_out, (char*)"*");
							}
							else {
								char word_hold[0x100];
								strcpy_word(word_hold, parse_in);
								getWord( parse_in);
								if (strcmp(parse_in->word, "sizeof") == 0) {
									strcat_p2(parse_out, word_hold);
									strcat_p2(parse_out, (char*)" * ");
									strcat_p2(parse_out, parse_in->word);
								}
								else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
									strcat_p2(parse_out, word_hold);
									strcat_p2(parse_out, (char*)" * ");
									strcat_p2(parse_out, parse_in->word);
								}
								else if (get_integer(&number2, parse_in->word)) {
									while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
										parse_in->ptr++;
									if (parse_in->line[parse_in->index].line[parse_in->ptr] == '<' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '<') {
										parse_in->ptr += 2;
										while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
											parse_in->ptr++;
										getWord( parse_in);
										if (get_integer(&number3, parse_in->word)) {
											sprintf_s(word_hold, "0x%x", number1 * (number2 << number3));
											strcat_p2(parse_out, word_hold);
										}
										else {
											debug++;// need to code
										}
									}
									else {
										sprintf_s(word_hold, "0x%x", number1 * number2);
										strcat_p2(parse_out, word_hold);
									}
								}
								else {
									strcat_p2(parse_out, parse_in->word);
									strcat_p2(parse_out, (char*)" * ");
									strcat_p2(parse_out, word_hold);
								}
							}
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '<' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '<') {
							char temp[0x100];
					//		sprintf_s(temp, "%s", parse_in->word);
							strcpy_word(temp, parse_in);
							parse_in->ptr++;
							parse_in->ptr++;
							while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
								parse_in->ptr++;
							getWord( parse_in);
							if (get_integer(&number2, parse_in->word)) {
								if (parse_out->line[parse_out->index].line[parse_out->ptr - 1] == '(' && parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
									parse_out->ptr--;
									parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
								}
								sprintf_s(temp, "0x%x", number1 << number2);
								strcat_p2(parse_out, temp);
								touched = 1;
							}
							else {
								debug++;
							}
						}
						else {
							strcat_p2(parse_out, parse_in->word);
							parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
							copy_char(parse_out, parse_in);
						}
					}
					else if (strcmp(parse_in->word, "UINT16") == 0 || strcmp(parse_in->word, "UINT8") == 0) {
						strcat_p2(parse_out, parse_in->word);
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t' || parse_in->line[parse_in->index].line[parse_in->ptr] == '*')
							copy_char(parse_out, parse_in);
					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
						strcat_p2(parse_out, parse_in->word);
						copy_char(parse_out, parse_in);
						UINT8 hold_ptr = parse_in->ptr;
						getWord(parse_in);
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ']') {
							strcat_p2(parse_out, parse_in->word);
							copy_char(parse_out, parse_in);
						}
						else {
							parse_in->ptr = hold_ptr;
						}
					}
					else {
						strcat_p2(parse_out, parse_in->word);
					}
				}
				else {
					copy_char(parse_out, parse_in);
				}
				break;
			}
		}
	}
	return(touched);
}
UINT8 distribute_code(parse_struct2* parse_out, parse_struct2* parse_in, int multiplier, int* depth) {
	char latch_op;
//	char word[0x100];
	UINT8 touched = 0;
	int debug = 0;

	latch_op = parse_in->line[parse_in->index].line[parse_in->ptr++];
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
	getWord(parse_in);
	if (parse_in->word[0]!='\0') {
		INT64 number1, number2;
		if (get_integer(&number1, parse_in->word)) {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
				depth[0]--;
				parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '<' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '<') {
					parse_in->ptr++;
					parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
					char temp[0x100];
			//		sprintf_s(temp,"%s", parse_in->word);
					strcpy_word(temp, parse_in);
					getWord( parse_in);
					if (get_integer(&number2, parse_in->word)) {
						if (parse_out->line[parse_out->index].line[parse_out->ptr - 1] == ' ' || parse_out->line[parse_out->index].line[parse_out->ptr - 1] == '\t')	parse_out->ptr--;
						copy_ptr_char(parse_out,  ')');
						copy_ptr_char(parse_out, '<');
						copy_ptr_char(parse_out,  '<');
						copy_ptr_char(parse_out, ' ');
						strcat_p2(parse_out, parse_in->word);
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
							depth[0]--;
							copy_char(parse_out, parse_in);
						}
						else {
							debug++;
							UINT8 ptr2 = parse_out->ptr;
							while (parse_out->line[parse_out->index].line[ptr2] != '(')ptr2--;
							UINT ptr3 = ptr2;
							char word[0x80];
							UINT ptr4 = 0;
							while (ptr3 < parse_out->ptr) word[ptr4++] = parse_out->line[parse_out->index].line[ptr3++];
							word[ptr4++] = ')';
							word[ptr4] = '\0';
							parse_out->line[parse_out->index].line[ptr2++] = '(';
							parse_out->line[parse_out->index].line[ptr2] = '\0';
							strcat_p2(parse_out, parse_in->word);
						}
						if (latch_op == '+') {
							if (multiplier < 0) {
								strcat_p2(parse_out, (char*)" - ");
								sprintf_s(temp, "0x%x", -multiplier * (number1 << number2));
							}
							else {
								strcat_p2(parse_out, (char*)" + ");
								sprintf_s(temp, "0x%x", multiplier * (number1 << number2));
							}
						}
						else {
							copy_ptr_char(parse_out,  ' ');
							copy_ptr_char(parse_out,  latch_op);
							copy_ptr_char(parse_out,  ' ');
							if (multiplier == 1) {
								sprintf_s(temp, "0x%x", (number1 << number2));
							}
							else {
								debug++;
							}
						}
						strcat_p2(parse_out, temp);
						touched = 1;
					}
					else {
						copy_ptr_char(parse_out,  ' ');
						copy_ptr_char(parse_out,  latch_op);
						copy_ptr_char(parse_out,  ' ');
						strcat_p2(parse_out, temp);
						parse_out->ptr += strlen(temp);
						copy_ptr_char(parse_out,  ')');
						copy_ptr_char(parse_out,  '<');
						copy_ptr_char(parse_out,  '<');
						copy_ptr_char(parse_out,  ' ');
						strcat_p2(parse_out, parse_in->word);
					}
				}
				else {
					while (parse_out->line[parse_out->index].line[parse_out->ptr - 1] == ' ' || parse_out->line[parse_out->index].line[parse_out->ptr - 1] == '\t')
						parse_out->ptr--;
					copy_ptr_char(parse_out,  ' ');
					copy_ptr_char(parse_out,  latch_op);
					copy_ptr_char(parse_out,  ' ');
					strcat_p2(parse_out, parse_in->word);
					copy_ptr_char(parse_out,  ')');

				}
			}
			else if (latch_op == '-' && parse_in->line[parse_in->index].line[parse_in->ptr] == '*') {
				parse_in->ptr++;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
				strcat_p2(parse_out,(char*) "-");
				strcat_p2(parse_out, parse_in->word);
				strcat_p2(parse_out,(char*) " * ");
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
					if (depth[0] != 0)
						debug++;
					depth[0]++;
					copy_char(parse_out,  parse_in);
					depth[0] = 0;
					while ((parse_in->line[parse_in->index].line[parse_in->ptr] != ')' || depth[0] > 0) && depth[0] >= 0) {
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	
							copy_char(parse_out, parse_in);
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
								copy_char(parse_out, parse_in);
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\"') {
							copy_char(parse_out,  parse_in);
							while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\"' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
								copy_char(parse_out, parse_in);
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
							depth[0]++;
							copy_char(parse_out, parse_in);
						}
						else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
							depth[0]--;
							copy_char(parse_out, parse_in);
						}
						else {
							//							debug++;
							touched |= exec_switch(parse_out, parse_in, -number1, depth);
						}
					}
				}
			}
			else {
				copy_ptr_char(parse_out,  latch_op);
				copy_ptr_char(parse_out,  ' ');
				strcat_p2(parse_out,parse_in->word);
			}
		}
		else {
			copy_ptr_char(parse_out,  latch_op);
			copy_ptr_char(parse_out,  ' ');
			strcat_p2(parse_out, parse_in->word);
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] != ']')copy_char(parse_out,  parse_in );
				copy_char(parse_out, parse_in);
			}
		}
	}
	else {
		copy_ptr_char(parse_out, latch_op);
	}
	return touched;
}
UINT8 exec_switch(parse_struct2* parse_out, parse_struct2* parse_in, INT multiplier, int* depth) {
	UINT8 touched = 0;
	switch (parse_in->line[parse_in->index].line[parse_in->ptr]) {
	case '+':
		if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '+' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
			copy_char(parse_out, parse_in);
			copy_char(parse_out, parse_in);
		}
		else {
			touched |= distribute_code(parse_out, parse_in, multiplier, depth);
		}
		break;
	case '-':
		if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '-' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
			copy_char(parse_out, parse_in);
			copy_char(parse_out, parse_in);
		}
		else {
			touched |= distribute_code(parse_out, parse_in, multiplier, depth);
		}
		break;
	case '|':
		if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '|' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
			copy_char(parse_out, parse_in);
			copy_char(parse_out, parse_in);
		}
		else {
			touched |= distribute_code(parse_out, parse_in, multiplier, depth);
		}
		break;
	case '=':
		copy_char(parse_out, parse_in);
		break;
	default:
		getWord(parse_in);
		if (parse_in->word[0]='\0') {
			strcat_p2(parse_out, parse_in->word);
		}
		else {
			copy_char(parse_out, parse_in);
		}
		break;
	}
	return touched;
}
UINT8 add_shift_distribute_index(parse_struct2* parse_out, parse_struct2* parse_in) {// (a+b)<<c => (a<<c)+(b<<c); where b is a number
	int debug = 0;
	UINT8 touched = 0;
	parse_in->ptr = 0;
	parse_out->ptr = 0;
	//	char word[0x100];

	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	copy_char(parse_out, parse_in);
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
				copy_char(parse_out, parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\"') {
			copy_char(parse_out, parse_in);
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\"' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
				copy_char(parse_out, parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
			copy_char(parse_out, parse_in);
			INT depth_i = 0;//index
			INT depth_s = 0;//subset
			while ((parse_in->line[parse_in->index].line[parse_in->ptr] != ']' && depth_i >= 0) || depth_i > 0) {
				switch (parse_in->line[parse_in->index].line[parse_in->ptr]) {
				case '+':
					if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '+' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
						copy_char(parse_out, parse_in);
						copy_char(parse_out, parse_in);
					}
					else {
						touched |= distribute_code(parse_out, parse_in, 1, &depth_s);
					}
					break;
				case '-':
					if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '-' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
						copy_char(parse_out, parse_in);
						copy_char(parse_out, parse_in);
					}
					else {
						touched |= distribute_code(parse_out, parse_in, 1, &depth_s);
					}
					break;
				case '|':
					if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '|' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=') {
						copy_char(parse_out, parse_in);
						copy_char(parse_out, parse_in);
					}
					else {
						touched |= distribute_code(parse_out, parse_in, 1, &depth_s);
					}
					break;
				case '=':
					copy_char(parse_out, parse_in);
					break;
				case ']':
					depth_i--;
					copy_char(parse_out, parse_in);
					break;
				default:
					getWord( parse_in);
					if (parse_in->word[0]!='\0') {
						strcat_p2(parse_out, parse_in->word);
					}
					else {
						copy_char(parse_out, parse_in);
					}
					break;
				}

			}
		}
		else {
			getWord(parse_in);
			if (parse_in->word_ptr != 0) {
				strcat_p2(parse_out, parse_in->word);
			}
			else {
				copy_char(parse_out, parse_in);
			}
		}
	}
	return touched;
}
UINT8 add_shift_distribute_nonindex(parse_struct2* parse_out, parse_struct2* parse_in) {// (a+b)<<c => (a<<c)+(b<<c); where b is a number
	int debug = 0;
	UINT8 touched = 0;
	parse_in->ptr = 0;
	parse_out->ptr = 0;
	char word[0x100];

	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	copy_char(parse_out, parse_in);
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
				copy_char(parse_out, parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\"') {
			copy_char(parse_out, parse_in);
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\"' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
				copy_char(parse_out, parse_in);
		}
		else {
			int depth = 0;
			touched |= exec_switch(parse_out, parse_in, 1, &depth);
		}
	}
	return touched;
}
void math_reduction(const char* dst_name, const char* src_name, parse_struct2_set* parse, UINT8 unit_debug) {
	int debug = 0;
	FILE* src;
	fopen_s(&src, src_name, "r");

	UINT  line_count = 0;

	while (fgets(parse->a.line[line_count++].line, 0x200, src) != NULL) {}
	line_count--;

	fclose(src);
	parse_struct2* parse_a = &parse->a;
	parse_struct2* parse_b = &parse->b;
	UINT touched = 1, touched2 = 1;
	UINT8 parse_ptr = 1;
	touched2 = 0;
	while (touched) {
		if (parse_ptr == 1) {
			parse_a = &parse->a;
			parse_b = &parse->b;
		}
		else {
			parse_a = &parse->b;
			parse_b = &parse->a;
		}
		parse_ptr = ((++parse_ptr) & 1);
		for (parse_a->index = 0, parse_b->index = 0; parse_a->index < line_count; parse_a->index++) {// elliminate extra brackets ((x+y)) => (x+y); (x+y)+z => x+y+z
			UINT flip = 0;
			parse_struct2* parse_in = &parse->a;
			parse_struct2*parse_out = &parse->b;
			touched2 = 1;

			if (parse_a->index >= 457)
				debug++;
			if (parse_a->index >= 62)
				debug++;

			while (touched2 == 1) {
				touched2 = 0;
				touched = 1;
				while (touched) {
					if (flip == 0) {
						parse_in = parse_a;
						parse_out = parse_b;
					}
					else {
						parse_in = parse_b;
						parse_out = parse_a;
					}
					parse_out->line[parse_out->index].line[0] = '\0';
					touched = simple_mult(parse_out, parse_in);
					touched2 |= touched;
					flip = flip ^ 0x01;
				}
				touched = 1;
				while (touched) {
					if (flip == 0) {
						parse_in = parse_a;
						parse_out = parse_b;
					}
					else {
						parse_in = parse_b;
						parse_out = parse_a;
					}
					parse_out->line[parse_out->index].line[0] = '\0';
					touched = simple_add2(parse_out, parse_in);
					touched2 |= touched;
					flip = flip ^ 0x01;
				}
				touched = 1;
				while (touched) {
					if (flip == 0) {
						parse_in = parse_a;
						parse_out = parse_b;
					}
					else {
						parse_in = parse_b;
						parse_out = parse_a;
					}
					parse_out->line[parse_out->index].line[0] = '\0';
					touched = simple_logic(parse_out, parse_in);
					touched2 |= touched;
					flip = flip ^ 0x01;
				}
				touched = 1;
				while (touched) {
					if (flip == 0) {
						parse_in = parse_a;
						parse_out = parse_b;
					}
					else {
						parse_in = parse_b;
						parse_out = parse_a;
					}
					parse_out->line[parse_out->index].line[0] = '\0';
					touched = add_shift_distribute_index(parse_out, parse_in);
					touched2 |= touched;
					flip = flip ^ 0x01;
				}
				touched = 1;
				while (touched) {
					if (flip == 0) {
						parse_in = parse_a;
						parse_out = parse_b;
					}
					else {
						parse_in = parse_b;
						parse_out = parse_a;
					}
					parse_out->line[parse_out->index].line[0] = '\0';
					touched = add_assoc_var_v_number_ordering2(parse_out, parse_in);//
					touched2 |= touched;
					flip = flip ^ 0x01;
				}
				touched = 1;
				while (touched) {
					if (flip == 0) {
						parse_in = parse_a;
						parse_out = parse_b;
					}
					else {
						parse_in = parse_b;
						parse_out = parse_a;
					}
					parse_out->line[parse_out->index].line[0] = '\0';
					touched = parenthesis_simple_cleanup(parse_out, parse_in);// simple cleanup (a) =>a; func(a) =>func(a)
					touched2 |= touched;
					flip = flip ^ 0x01;
				}
				touched = 1;
				while (touched) {
					if (flip == 0) {
						parse_in = parse_a;
						parse_out = parse_b;
					}
					else {
						parse_in = parse_b;
						parse_out = parse_a;
					}
					parse_out->line[parse_out->index].line[0] = '\0';
					touched = parenthesis_depth_based(parse_out, parse_in);// [((...)| number1)|number2] => [(...)| number1|number2] 
					touched2 |= touched;
					flip = flip ^ 0x01;
				}
				touched = 1;
				while (touched) {
					if (flip == 0) {
						parse_in = parse_a;
						parse_out = parse_b;
					}
					else {
						parse_in = parse_b;
						parse_out = parse_a;
					}
					parse_out->line[parse_out->index].line[0] = '\0';
					touched = simple_variable_math(parse_out, parse_in);// [((...)| number1)|number2] => [(...)| number1|number2] 
					touched2 |= touched;
					flip = flip ^ 0x01;
				}
				if (flip == 0) {
					parse_in = parse_a;
					parse_out = parse_b;
				}
				else {
					parse_in = parse_b;
					parse_out = parse_a;
				}
				parse_out->line[parse_out->index].line[0] = '\0';
				blank_space_formating(parse_out->line[parse_out->index].line, parse_in->line[parse_in->index].line);
				flip = flip ^ 0x01;
			}
			strcpy_p2(parse_b, parse_out);
			parse_b->index++;
		}
	}
	FILE* dest;
	fopen_s(&dest, dst_name, "w");
	parse_a->index = 0;
	for (parse_b->index = 0; parse_b->index < line_count; parse_b->index++, parse_a->index++) {
		fprintf(dest, "%s", parse_b->line[parse_b->index].line);
		strcpy_p2(parse_a, parse_b);
	}
	UINT ptr = strlen(parse_b->line[line_count - 1].line);
	parse_b->line[line_count - 1].line[ptr] = '\0';
	fclose(dest);
}