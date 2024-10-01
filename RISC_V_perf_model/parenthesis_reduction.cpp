// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

UINT8 assoc_ops(op_type a, op_type b) {
	UINT8 result = 0;
	switch (a) {
	case add_op:
		if (b == add_op || b == sub_op) {
			result = 1;
		}
		break;
	case mul_op:
		if (b == mul_op) {
			result = 1;
		}
		break;
	case sl_op:
	case sr_op:
		if (b == sl_op || b == sr_op) {
			result = 1;
		}
		break;
	case or_op:
		if (b == or_op) {
			result = 1;
		}
		break;
	case or_log:
		if (b == or_log) {
			result = 1;
		}
		break;
	case and_op:
		if (b == and_op) {
			result = 1;
		}
		break;
	case and_log:
		if (b == and_log) {
			result = 1;
		}
		break;
	case xor_op:
	case eq_log:
	default:
		break;
	}
	return result;
}

UINT8 getOperatorM(UINT8* op_match, parse_struct2* parse, operator_type* op_table) {
	UINT8 result = 0;
	char word[0x100];
	UINT8 size = 0;

	while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == 0x09) parse->ptr++;// strip off blank spaces and tabs
	UINT16 ptr = parse->ptr;
	while (parse->line[parse->index].line[ptr] == '>' || parse->line[parse->index].line[ptr] == '<' || parse->line[parse->index].line[ptr] == '=' || parse->line[parse->index].line[ptr] == '!' ||
		parse->line[parse->index].line[ptr] == '*' || parse->line[parse->index].line[ptr] == '+' || parse->line[parse->index].line[ptr] == '-' || parse->line[parse->index].line[ptr] == '&' || parse->line[parse->index].line[ptr] == '|' || parse->line[parse->index].line[ptr] == '^') {
		word[size++] = parse->line[parse->index].line[ptr++];
	}
	word[size++] = '\0';
	for (UINT8 match = 0; match < 0x10; match++) {
		if (strcmp(word, op_table[match].symbol) == 0) {
			op_match[0] = match;
			parse->ptr = ptr;
			while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == 0x09) parse->ptr++;// strip off blank spaces and tabs
			match = 0x10;
			result = 1;
		}
	}
	return result;
}
UINT8 double_bracket_reduction(parse_struct2* parse_out, parse_struct2* parse_in) {// ((...)) => (..);
	UINT8 touched = 0;
	UINT depth = 0;
	parse_in->ptr = parse_out->ptr = 0;
	parse_struct2 temp;
	temp.index = 0;
	temp.line = (line_struct*)malloc(sizeof(line_struct));
//	temp.ptr = 0;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
			copy_char(parse_out, parse_in);
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
				copy_char(parse_out, parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\"') {
			copy_char(parse_out, parse_in);
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\"' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
				copy_char(parse_out, parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '(' &&
			parse_in->line[parse_in->index].line[parse_in->ptr + 2] != '(') {
			parse_in->ptr += 2;
//			char temp[0x100];
			temp.ptr = 0;
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != ')' || depth > 0) {
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(')
					depth++;
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
					depth--;
//				temp[ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
				copy_char(&temp, parse_in);
			}
	//		temp[ptr] = '\0';
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ')') {
				strcat_p2(parse_out,(char*) '(');
				strcat_p2(parse_out, &temp);
				parse_in->ptr++;
				touched = 1;
			}
			else {
				strcat_p2(parse_out, (char*)"((");
				strcat_p2(parse_out, &temp);
			}
		}

		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '(' && parse_in->line[parse_in->index].line[parse_in->ptr + 2] != '(') {
			parse_in->ptr += 2;

			temp.ptr = 0;
			while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
				copy_char(&temp, parse_in);

			getWord(parse_in);

			if (parse_in->word_ptr != 0) {
				strcat_p2(&temp, parse_in->word);
			}

			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
				strcat_p2(parse_out, (char*)"[");
				strcat_p2(parse_out, &temp);
				parse_in->ptr++;
				touched = 1;
			}
			else {
				strcat_p2(parse_out, (char*)"[(");
				strcat_p2(parse_out, &temp);
			}
		}
		else {
			getWord(parse_in);
			if (parse_in->word_ptr != 0)
				strcat_p2(parse_out, parse_in->word);
			else
				copy_char(parse_out, parse_in);
		}
	}
	free(temp.line);
	return touched;
}
UINT8 prep_parenthesis_from_blanks(parse_struct2* parse_out, parse_struct2* parse_in) {
	UINT8 touched = 0;
	parse_in->ptr = 0;
	parse_out->ptr = 0;
	char word2[0x100], word_out[0x100];
	while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 
		copy_char(parse_out, parse_in);
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
				copy_char(parse_out, parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '\t')) {
			copy_char(parse_out, parse_in);
			parse_in->ptr++;
			touched = 1;
		}
		else if ((parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ')') {
			parse_in->ptr++;
			copy_char(parse_out, parse_in);
			touched = 1;
		}
		else if ((parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') && (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '\t')) {
			parse_in->ptr++;
			copy_char(parse_out, parse_in);
		}
		else {
			copy_char(parse_out, parse_in);
		}
	}
	return touched;
}
UINT8 parenthesis_clear_unecessary_brackets(parse_struct2* parse_out, parse_struct2* parse_in) {// elliminate unecessary brackets [(...)] => [...]
	UINT8 touched = 0;
	int debug = 0;
	parse_in->ptr = 0; parse_out->ptr = 0;
	char word1[0x100], word2[0x100], word_out[0x100];
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
				copy_char(parse_out, parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '(') {
			parse_in->ptr += 2;
			INT8 depth = 0;
			UINT16 ptr_hold = parse_in->ptr;
			UINT loop_complete = 0;
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0' && !loop_complete && depth >= 0) {
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
					depth++;
					parse_in->ptr++;
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ']' && depth == 0) {
					parse_out->line[parse_out->index].line[parse_out->ptr++] = '[';
					for (UINT16 i = ptr_hold; i < parse_in->ptr; i++)
						parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[i];
					parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
					parse_in->ptr++;
					copy_char(parse_out, parse_in);
					loop_complete = 1;
					touched = 1;
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == ']' && depth == 0) {
					parse_out->line[parse_out->index].line[parse_out->ptr++] = '[';
					parse_out->line[parse_out->index].line[parse_out->ptr++] = '(';
					for (UINT16 i = ptr_hold; i < parse_in->ptr; i++)
						parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[i];
					copy_char(parse_out, parse_in);
					loop_complete = 1;
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
					depth--;
					parse_in->ptr++;
				}
				else {
					parse_in->ptr++;
				}
			}
			if (depth < 0) {
				parse_in->ptr = ptr_hold;
				parse_in->ptr -= 2;
				copy_char(parse_out, parse_in);
				copy_char(parse_out, parse_in);
			}
			else if (!loop_complete)
				debug++;
		}
		else {
			copy_char(parse_out, parse_in);
		}
	}
	return touched;
}
UINT8 parenthesis_add_assoc(parse_struct2* parse_out, parse_struct2* parse_in) {
	UINT8 touched = 0;
	int debug = 0;
	parse_in->ptr = 0;
	parse_out->ptr = 0;
	char word2[0x100], word_out[0x100];

	UINT hold_operator, last_op = ' ';
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	
			copy_char(parse_out, parse_in);
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
				copy_char(parse_out, parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\"') {
			copy_char(parse_out, parse_in);
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\"')
				copy_char(parse_out, parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '<' || parse_in->line[parse_in->index].line[parse_in->ptr] == '>' ||
			parse_in->line[parse_in->index].line[parse_in->ptr] == '+' || parse_in->line[parse_in->index].line[parse_in->ptr] == '-' ||
			parse_in->line[parse_in->index].line[parse_in->ptr] == '*' || parse_in->line[parse_in->index].line[parse_in->ptr] == '/') {
			last_op = parse_in->line[parse_in->index].line[parse_in->ptr];
			copy_char(parse_out, parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
			getWord( parse_in);
			INT64 number1;
			if (get_integer(&number1, parse_in->word)) {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')		parse_in->ptr++;
				if ((parse_in->line[parse_in->index].line[parse_in->ptr] == '+' || parse_in->line[parse_in->index].line[parse_in->ptr] == '-') &&
					parse_in->line[parse_in->index].line[parse_in->ptr + 1] != '+' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] != '-' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] != '=' &&
					last_op != '<' && last_op != '>' && last_op != '*' && last_op != '/') {

					hold_operator = parse_in->line[parse_in->index].line[parse_in->ptr];
					parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
						parse_in->ptr++;
					char buffer[0x100];
					sprintf_s(buffer, "%s", parse_in->word);
					getWord( parse_in);
					INT64 number2;
					int depth = 0;
					if (get_integer(&number2, parse_in->word)) {// pre-execute math before generating assembly
						if (hold_operator == '+') {
							sprintf_s(buffer, "0x%x", number1 + number2);
						}
						else {
							if (number1 >= number2)
								sprintf_s(buffer, "0x%x", number1 - number2);
							else {
								sprintf_s(buffer, "-0x0%x", number2 - number1);
							}
						}
						parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
						strcat_p2(parse_out, buffer);
						touched = 1;
					}
					else if (parse_in->word_ptr !=0) { // variables in front of numbers
						strcat_p2(parse_out, parse_in->word);
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[') {
							while (parse_in->line[parse_in->index].line[parse_in->ptr] != ']')
								parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
							parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
						}
						parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
						parse_out->line[parse_out->index].line[parse_out->ptr++] = hold_operator;
						parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
						parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
						strcat_p2(parse_out, buffer);
						touched = 1;
					}
					else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
						depth++;
						if (hold_operator == '-') {
							parse_out->line[parse_out->index].line[parse_out->ptr++] = '-';
							parse_out->line[parse_out->index].line[parse_out->ptr++] = '1';
							parse_out->line[parse_out->index].line[parse_out->ptr++] = '*';
							parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
						}
						copy_char(parse_out, parse_in);
						while (depth > 0) {
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(')
								depth++;
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')')
								depth--;
							copy_char(parse_out, parse_in);
						}
						parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
						if (hold_operator == '-') {
							parse_out->line[parse_out->index].line[parse_out->ptr++] = '+';
							parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
						}
						else
							parse_out->line[parse_out->index].line[parse_out->ptr++] = hold_operator;
						parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
						parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
						strcat_p2(parse_out, buffer);
						touched = 1;
					}
					else {
						debug++;
					}
				}
				else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' || parse_in->line[parse_in->index].line[parse_in->ptr] == ';') {
					strcat_p2(parse_out, parse_in->word);
					copy_char(parse_out, parse_in);
				}
				else {
					strcat_p2(parse_out, parse_in->word);
					if (parse_in->line[parse_in->index].line[parse_in->ptr] != ']')
						parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
					copy_char(parse_out, parse_in);
				}
			}
			else if (parse_in->word_ptr != 0) {
				strcat_p2(parse_out, parse_in->word);
			}
			else {
				copy_char(parse_out, parse_in);
			}
		}
	}
	return touched;
}
UINT8 parenthesis_num_plus_var(parse_struct2* parse_out, parse_struct2* parse_in) {//		-> reorder arithmetics from "number + variable" to "variable + number"; this will yield "number + number" if available
	UINT8 touched = 0;
	parse_in->ptr = 0;
	parse_out->ptr = 0;
	char  word2[0x100], word_out[0x100];

	UINT debug = 0;
	int shift_valid = 0;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')		
			copy_char(parse_out, parse_in);
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
				copy_char(parse_out,  parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\"') {
			copy_char(parse_out, parse_in);
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\"')
				copy_char(parse_out, parse_in);
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == '\"')
				copy_char(parse_out, parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '<' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '<') {
			copy_char(parse_out, parse_in);
			copy_char(parse_out, parse_in);
			shift_valid = 1;
		}
		else {
			getWord( parse_in);
			INT64 number1;
			if (get_integer(&number1, parse_in->word)) {
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
					parse_in->ptr++;
				if ((parse_in->line[parse_in->index].line[parse_in->ptr] == '+' || parse_in->line[parse_in->index].line[parse_in->ptr] == '-') && !shift_valid &&
					(parse_in->line[parse_in->index].line[parse_in->ptr + 1] != '+' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] != '-' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] != '=')) {
					char local_operator = parse_in->line[parse_in->index].line[parse_in->ptr];
					parse_in->ptr++;
					if (number1 == 0) {
						debug++;
						// do nothing: parse next character
					}
					else {
						UINT depth = 0;
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')	parse_in->ptr++;
						if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
							depth++;
							copy_char(parse_out,  parse_in);
							while (depth > 0) {
								if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
									depth++;
								}
								else if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
									depth--;
								}
								copy_char(parse_out, parse_in);
							}
							parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
							parse_out->line[parse_out->index].line[parse_out->ptr++] = local_operator;
							parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
							parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
							strcat_p2(parse_out, parse_in->word);
							touched = 1;
						}
						else {
							strcat_p2(parse_out, parse_in->word);
							parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
							parse_out->line[parse_out->index].line[parse_out->ptr++] = local_operator;
							parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
							parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
						}
					}
				}
				else {
					strcat_p2(parse_out, parse_in->word);
					if (parse_in->line[parse_in->index].line[parse_in->ptr] != ' ' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\t' &&
						parse_in->line[parse_in->index].line[parse_in->ptr] != ']' && parse_in->line[parse_in->index].line[parse_in->ptr] != ')') {
						parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
						parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
					}
					shift_valid = 0;
				}
			}
			else if (parse_in->word_ptr!=0) {
				strcat_p2(parse_out, parse_in->word);
			}
			else {
				copy_char(parse_out, parse_in);
			}
		}
	}
	return touched;
}
UINT8 parenthesis_a_little_math(parse_struct2* parse_out, parse_struct2* parse_in, operator_type* op_table) {
	UINT8 touched = 0;
	parse_in->ptr = 0;
	parse_out->ptr = 0;
	char  word2[0x100], word_out[0x100];

	int debug = 0;

	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		while (parse_in->line[parse_in->index].line[parse_in->ptr] != '[' && parse_in->line[parse_in->index].line[parse_in->ptr] != '(' && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') 	parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];

		if (parse_out->ptr >= 0x200)
			debug++;

		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '(') {
			UINT ptr_hold = parse_in->ptr;
			UINT ptr = parse_in->ptr + 2;
			UINT8 hit = 0;

			UINT depth = 2;
			while (parse_in->line[parse_in->index].line[ptr] != '\0' && !hit && depth > 1) { // try outside loop reduction first
				if (parse_in->line[parse_in->index].line[ptr] == '(') depth++;
				else if (parse_in->line[parse_in->index].line[ptr] == ')') depth--;
				if (parse_in->line[parse_in->index].line[ptr] == ')' && depth == 1) {
					if (parse_in->line[parse_in->index].line[ptr + 1] == ')') {
						parse_out->ptr = 0;
						for (UINT16 i = 0; i <= ptr_hold; i++)
							parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[i];
						for (UINT16 i = ptr_hold + 2; i <= ptr; i++)
							parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[i];
						parse_in->ptr = ptr + 2;
						while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
							copy_char(parse_out, parse_in);
						hit = 1;
						touched = 1;
					}
				}
				ptr++;
			}
			if (!hit) { // then try inside loop reduction
				ptr = parse_in->ptr + 2;
				while (parse_in->line[parse_in->index].line[ptr] != '(' && parse_in->line[parse_in->index].line[ptr] != ')' && parse_in->line[parse_in->index].line[ptr] != '\0') ptr++;
				if (parse_in->line[parse_in->index].line[ptr] == ')' && parse_in->line[parse_in->index].line[ptr + 1] == ')') {
					for (UINT i = parse_in->ptr + 1; i < ptr + 1; i++) 	parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[i];
					parse_in->ptr = ptr + 2;
					parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
					touched = 1;
				}
				else if (parse_in->line[parse_in->index].line[ptr] == ')') {
					UINT8 op_match;
					if (getOperatorM(&op_match, parse_in, op_table)) {
						for (UINT i = parse_in->ptr + 1; i < ptr; i++) 	parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[i];
						parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
						strcat_p2(parse_out, op_table[op_match].symbol);
						touched = 1;
					}
					else {
						copy_char(parse_out, parse_in);
					}
				}
				else {
					copy_char(parse_out, parse_in);;
				}
			}
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '[' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '(') {
			UINT ptr = parse_in->ptr + 2;
			while (parse_in->line[parse_in->index].line[ptr] != '(' && parse_in->line[parse_in->index].line[ptr] != ')' && parse_in->line[parse_in->index].line[ptr] != '\0') ptr++;
			if (parse_in->line[parse_in->index].line[ptr] == ')' && parse_in->line[parse_in->index].line[ptr + 1] == ']') {
				parse_out->line[parse_out->index].line[parse_out->ptr++] = '[';
				for (UINT i = parse_in->ptr + 2; i < ptr; i++) 	
					parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[i];
				parse_out->line[parse_out->index].line[parse_out->ptr++] = ']';
				parse_in->ptr = ptr + 2;
				parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
				touched = 1;
			}
			else if (parse_in->line[parse_in->index].line[ptr] == ')') {
				ptr++;
				UINT8 op_match;
				if (getOperatorM(&op_match, parse_in, op_table)) {
					for (UINT i = parse_in->ptr + 1; i < ptr - 1; i++) 	parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[i];
					parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
					strcat_p2(parse_out, op_table[op_match].symbol);
					touched = 1;
				}
				else {
					copy_char(parse_out, parse_in);
				}
			}
			else {
				copy_char(parse_out, parse_in);
			}
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
			UINT hold_ptr = parse_in->ptr;
			parse_in->ptr++;
			getWord( parse_in);
			INT64 number1, number2;
			UINT8 op_match;
			UINT hit = 0;
			if (get_integer(&number1, parse_in->word)) {
				if (getOperatorM(&op_match, parse_in, op_table)) {
					if (number1 == 0) {
						switch (op_match) {
						case 0:// +
						case 4:// |
						case 5:// &
							parse_out->line[parse_out->index].line[parse_out->ptr++] = '(';
							parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
							hold_ptr = parse_in->ptr;
							break;
						case 1:// -
							parse_out->line[parse_out->index].line[parse_out->ptr++] = '(';
							parse_out->line[parse_out->index].line[parse_out->ptr++] = '-';
							parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
							hold_ptr = parse_in->ptr;
							break;
						case 2:// *
						case 3:// /
						case 7:// <<
							getWord( parse_in);
							if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')') {
								parse_out->line[parse_out->index].line[parse_out->ptr++] = '0';
								parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
								parse_in->ptr++;
								hold_ptr = parse_in->ptr; // (0 op var) => 0
							}
							else {
								debug++;
							}
							break;
						default:
							debug++;
							break;
						}
					}
					else {
						getWord( parse_in);
						if (get_integer(&number2, parse_in->word)) {
							if (parse_in->line[parse_in->index].line[parse_in->ptr++] == ')') {
								hit = 1;
								touched = 1;
								parse_out->line[parse_out->index].line[parse_out->ptr++] = '(';
								parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
								switch (op_match) {
								case 0:
									number1 = number1 + number2;
									break;
								case 1:
									number1 = number1 - number2;
									break;
								case 2:
									number1 = number1 * number2;
									break;
								case 3:
									number1 = number1 / number2;
									break;
								case 4:
									number1 = number1 & number2;
									break;
								case 5:
									number1 = number1 | number2;
									break;
								case 7:
									number1 = number1 << number2;
									break;
								default:
									debug++;
									break;
								}
								char temp_line[0x100];
								sprintf_s(temp_line, "%#x", number1);
								strcat_p2(parse_out, temp_line);
								parse_out->line[parse_out->index].line[parse_out->ptr++] = ')';
								parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
							}
						}
					}
				}
			}
			if (!hit) {
				parse_in->ptr = hold_ptr;
				copy_char(parse_out, parse_in);
			}
		}

		else if (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
			copy_char(parse_out, parse_in);
		}
	}
	parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
	return touched;
}
UINT8 parenthesis_inner_set(parse_struct2* parse_out, parse_struct2* parse_in) {// + (a + b) + => + a + b +
	UINT8 touched = 0;
	int debug = 0;
	parse_in->ptr = 0;
	parse_out->ptr = 0;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 	parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
				copy_char(parse_out, parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '*') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0' && !(parse_in->line[parse_in->index].line[parse_in->ptr] == '*' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/')) parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
			if (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
				copy_char(parse_out, parse_in); 
				copy_char(parse_out, parse_in);
			}
		}
		else {
			switch (parse_in->line[parse_in->index].line[parse_in->ptr]) {
			case '(':
			case '=':
			case '+':
				copy_char(parse_out, parse_in);
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 	
					copy_char(parse_out, parse_in);
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
					UINT parse_in_ptr2 = parse_in->ptr;
					UINT stop = 0;
					parse_in_ptr2++;
					while (parse_in->line[parse_in->index].line[parse_in_ptr2] != '\0' && !stop) {
						switch (parse_in->line[parse_in->index].line[parse_in_ptr2]) {
						case '*': // may or may not break, to be on safe side we exit, need better parsing technique
						case '/':
						case '>':
						case '<':
						case '&':
						case '|':
						case '^':
						case ',':// comma indicates function call need better parsing technique
							stop = 1;
							break;
						case '(': // advance to next checkpoint
							while (parse_in->ptr < parse_in_ptr2)parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
							parse_in_ptr2++;
							//								repeat = 1;
							break;
						case ')': // drop parenthesis 
							// need to check that parenthesis is not followed by  a '*' r '/', etc.
							parse_in_ptr2++;
							while (parse_in->line[parse_in->index].line[parse_in_ptr2] == ' ' || parse_in->line[parse_in->index].line[parse_in_ptr2] == '\t')parse_in_ptr2++;
							if (parse_in->line[parse_in->index].line[parse_in_ptr2] == '+' || parse_in->line[parse_in->index].line[parse_in_ptr2] == '-' || parse_in->line[parse_in->index].line[parse_in_ptr2] == ')' || parse_in->line[parse_in->index].line[parse_in_ptr2] == ']' || parse_in->line[parse_in->index].line[parse_in_ptr2] == ';') {
								parse_in->ptr++;
								while (parse_in->line[parse_in->index].line[parse_in->ptr] != ')')parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
								parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
								parse_in->ptr++;
								stop = 1;
								touched = 1;
							}
							else {
								stop = 1;
							}
							break;
						default:
							parse_in_ptr2++;
							break;
						}
					}
				}
				break;
			case '-':
				if (parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '=' || parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '-') {
					copy_char(parse_out, parse_in);
					copy_char(parse_out, parse_in);
				}
				else {
					parse_in->ptr++;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 	parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
						UINT parse_in_ptr2 = parse_in->ptr;
						UINT stop = 0;
						parse_in_ptr2++;
						while (parse_in->line[parse_in->index].line[parse_in_ptr2] != '\0' && !stop) {
							switch (parse_in->line[parse_in->index].line[parse_in_ptr2]) {
							case '*': // may or may not break, to be on safe side we exit, need better parsing technique
							case '/':
							case '>':
							case '<':
							case '&':
							case '|':
							case '^':
							case ',':// comma indicates function call need better parsing technique
								parse_out->line[parse_out->index].line[parse_out->ptr++] = '-';
								parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
								parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
								stop = 1;
								break;
							case '(': // advance to next checkpoint
								parse_out->line[parse_out->index].line[parse_out->ptr++] = '-';
								parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
								parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
								while (parse_in->ptr < parse_in_ptr2)parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
								parse_in_ptr2++;
								//								repeat = 1;
								break;
							case ')': {// drop parenthesis
								// need to check that parenthesis is not followed by  a '*' r '/', etc.
								UINT ptr_hold = parse_in_ptr2;
								parse_in_ptr2++;
								while (parse_in->line[parse_in->index].line[parse_in_ptr2] == ' ' || parse_in->line[parse_in->index].line[parse_in_ptr2] == '\t')parse_in_ptr2++;
								if (parse_in->line[parse_in->index].line[parse_in_ptr2] == '<') {
									stop = 1;
								}
								else {
									parse_in->ptr++;
									parse_out->line[parse_out->index].line[parse_out->ptr++] = '-';
									parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
									parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
									parse_in_ptr2 = ptr_hold;

									while (parse_in->ptr < parse_in_ptr2) {
										if (parse_in->line[parse_in->index].line[parse_in->ptr] == '+') {
											parse_out->line[parse_out->index].line[parse_out->ptr++] = '-';
											parse_in->ptr++;
										}
										else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '-') {
											parse_out->line[parse_out->index].line[parse_out->ptr++] = '+';
											parse_in->ptr++;
										}
										else {
											parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
										}
									}
									parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
									parse_in->ptr++;
									parse_in_ptr2++;
									stop = 1;
									touched = 1;
								}
							}
									break;
							default:
								parse_in_ptr2++;
								break;
							}
						}
					}
					else {
						parse_out->line[parse_out->index].line[parse_out->ptr++] = '-';
						parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
						parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
					}
				}
				break;
			default: {
		//		char word1[0x80];
				getWord( parse_in);
				if (parse_in->word_ptr == 0) {
					copy_char(parse_out, parse_in);
				}
				else {
					parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
					strcat_p2(parse_out, parse_in->word);
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 
						copy_char(parse_out, parse_in);
					if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(')
						copy_char(parse_out, parse_in); // function call, do not clear parenthesis
				}
			}
				   break;
			}
		}
	}
	return touched;
}
struct p_record_type {
	op_type last_op;
	UINT16 ptr_in, ptr_out;
};
UINT8 parenthesis_depth_based(parse_struct2* parse_out, parse_struct2* parse_in) {// + (a + b) + => + a + b +
	UINT8 touched = 0;
	int debug = 0;
//	UINT parse_in_ptr = 0, parse_out_ptr = 0;
	parse_in->ptr = parse_out->ptr = 0;
	INT depth = 0;
	p_record_type p_record[0x10];
	p_record[depth].last_op = none_op;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t')
			copy_char(parse_out, parse_in);
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') 
				copy_char(parse_out, parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '*') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0' && !(parse_in->line[parse_in->index].line[parse_in->ptr] == '*' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/')) 
				copy_char(parse_out, parse_in);
			if (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
				copy_char(parse_out, parse_in); 
				copy_char(parse_out, parse_in);
			}
		}
		else {
			switch (parse_in->line[parse_in->index].line[parse_in->ptr]) {
			case '(':
				depth++;
				p_record[depth].ptr_in = parse_in->ptr;
				p_record[depth].ptr_out = parse_out->ptr;
				p_record[depth].last_op = none_op;
				copy_char(parse_out, parse_in);
				break;
			case ')': {
				depth--;
				if (depth < 0)
					debug++;
				UINT ptr_hold = parse_in->ptr;
				copy_char(parse_out, parse_in);
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 	
					copy_char(parse_out, parse_in);
				op_type last_op = decode_op(parse_in->line[parse_in->index].line, &parse_in->ptr);
				if (last_op == none_op) {
					// continue, no issues
				}
				else if (last_op == p_record[depth + 1].last_op && assoc_ops(last_op, p_record[depth].last_op)) {
					parse_in->ptr = p_record[depth + 1].ptr_in + 1;
					parse_out->ptr = p_record[depth + 1].ptr_out;
					while (parse_in->ptr < ptr_hold)
						copy_char(parse_out, parse_in);
					parse_in->ptr++;
					touched = 1;
					while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0')
						copy_char(parse_out, parse_in);
				}
				else {
					strcat_p2(parse_out, op2word(last_op));
				}
				p_record[depth].last_op = last_op;
				p_record[depth + 1].last_op = none_op;
			}
					break;
			case '*':
			case '+':
			case '-':
			case '<':
			case '|':
				p_record[depth].last_op = decode_op(parse_in->line[parse_in->index].line, &parse_in->ptr);
				if (p_record[depth].last_op == none_op) {
					getWord( parse_in);
					if (parse_in->word[0] == '\0') {
						copy_char(parse_out, parse_in);
					}
					else {
						parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
						strcat_p2(parse_out, parse_in->word);
						while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 	
							copy_char(parse_out, parse_in);
					}
				}
				else {
					parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
					strcat_p2(parse_out, op2word(p_record[depth].last_op));
				}
				break;
			default: {
				getWord( parse_in);
				if (parse_in->word[0] == '\0') {
					copy_char(parse_out, parse_in);
				}
				else {
					parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
					strcat_p2(parse_out, parse_in->word);
					while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 	
						copy_char(parse_out, parse_in);
				}
			}
				   break;
			}
		}
	}
	return touched;
}


// simple cleanup (a) =>a; func(a) =>func(a)
UINT8 parenthesis_simple_cleanup(parse_struct2* parse_out, parse_struct2* parse_in) {
	UINT8 touched = 0;
	int debug = 0;
	parse_in->ptr = 0;
	parse_out->ptr = 0;
	while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
		while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 	
			copy_char(parse_out, parse_in);
		if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') 
				copy_char(parse_out, parse_in);
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '/' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '*') {
			while (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0' && !(parse_in->line[parse_in->index].line[parse_in->ptr] == '*' && parse_in->line[parse_in->index].line[parse_in->ptr + 1] == '/')) parse_out->line[parse_out->index].line[parse_out->ptr++] = parse_in->line[parse_in->index].line[parse_in->ptr++];
			if (parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
				copy_char(parse_out, parse_in);
				copy_char(parse_out, parse_in);
			}
		}
		else if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
		//	UINT parse_in_ptr2 = parse_in->ptr + 1;
			UINT parse_in_ptr2 = parse_in->ptr;
			UINT stop = 0;
			data_type_enum type;
			UINT type_match = 0;
			parse_in->ptr++;
			while (!stop && parse_in->line[parse_in->index].line[parse_in->ptr] != '\0') {
				switch (parse_in->line[parse_in->index].line[parse_in->ptr]) {
				case '(':
				case ')':
				case '+':
				case '-':
				case '*':
				case '/':
				case '&':
				case '|':
				case '^':
				case '=':
				case '<':
				case '>':
					stop = 1;
					break;
				default:
					getWord( parse_in);
					if (parse_in->word_ptr == 0) {
						parse_in->ptr++;
					}
					else if (type_decode(&type, parse_in->word)) {
						stop = 1;/// exit on type cast
						type_match = 1;
					}
					else {
						// get word handled pointer advancement, keep going
					}
					break;
				}
			}
			if (parse_in->line[parse_in->index].line[parse_in->ptr] == ')' && !type_match) {
				parse_in->ptr = parse_in_ptr2;
				parse_in->ptr++;
				while (parse_in->line[parse_in->index].line[parse_in->ptr] != ')')
					copy_char(parse_out, parse_in);
				parse_in->ptr++;
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(') {
					parse_out->line[parse_out->index].line[parse_out->ptr++] = ' ';
				}
				parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
				touched = 1;
			}
			else {
				parse_in->ptr = parse_in_ptr2;
				copy_char(parse_out, parse_in);
			}
		}
		else {
			data_type_enum type;
			getWord( parse_in);
			if (parse_in->word_ptr == 0) {
				copy_char(parse_out, parse_in);
			}
			else {
				parse_out->line[parse_out->index].line[parse_out->ptr] = '\0';
				strcat_p2(parse_out, parse_in->word);
				while (parse_in->line[parse_in->index].line[parse_in->ptr] == ' ' || parse_in->line[parse_in->index].line[parse_in->ptr] == '\t') 
					copy_char(parse_out, parse_in);
				if (parse_in->line[parse_in->index].line[parse_in->ptr] == '(')
					copy_char(parse_out, parse_in);
			}
		}
	}
	return touched;
}
void parenthesis_reduction(const char* dst_name, const char* src_name, operator_type* op_table, parse_struct2_set* parse, UINT debug_unit) {
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

		while (parse->a.line[parse->a.index].line[parse->a.ptr] != '\0') {
			if (parse->a.line[parse->a.index].line[parse->a.ptr] == '(' && (parse->a.line[parse->a.index].line[parse->a.ptr + 1] == ' ' || parse->a.line[parse->a.index].line[parse->a.ptr + 1] == '\t')) {
				copy_char(&parse->b, &parse->a);
				parse->a.ptr++;
			}
			else if ((parse->a.line[parse->a.index].line[parse->a.ptr] == ' ' || parse->a.line[parse->a.index].line[parse->a.ptr] == '\t') && parse->a.line[parse->a.index].line[parse->a.ptr + 1] == ')') {
				parse->a.ptr++;
				copy_char(&parse->b, &parse->a);
			}
			if (parse->a.line[parse->a.index].line[parse->a.ptr] == '?' && (parse->a.line[parse->a.index].line[parse->a.ptr + 1] == ' ' || parse->a.line[parse->a.index].line[parse->a.ptr + 1] == '\t')) {
				copy_char(&parse->b, &parse->a);
				parse->a.ptr++;
			}
			else if ((parse->a.line[parse->a.index].line[parse->a.ptr] == ' ' || parse->a.line[parse->a.index].line[parse->a.ptr] == '\t') && parse->a.line[parse->a.index].line[parse->a.ptr + 1] == '?') {
				parse->a.ptr++;
				copy_char(&parse->b, &parse->a);
			}
			else {
				parse->b.line[parse->b.index].line[parse->b.ptr] = '\0';
				copy_char(&parse->b, &parse->a);
			}
		}
		parse->b.index++;
	}

	UINT8 touched, touched2;
	for (parse->a.index = 0, parse->b.index = 0; parse->b.index < line_count; parse->b.index++) {
		UINT flip = 0;
		parse_struct2* parse_in = &parse->a;
		parse_struct2* parse_out = &parse->b;
		touched2 = 1;

		if (parse->a.index >= 0x31)
			debug++;
		if (parse->a.index >= 549)
			debug++;
		if (parse->a.index >= 380)
			debug++;

		while (touched2 == 1) {
			touched2 = 0;

			if (flip == 0) {
				parse_in = &parse->a;
				parse_out = &parse->b;
			}
			else {
				parse_in = &parse->b;
				parse_out = &parse->a;
			}
			parse_out->line[parse_out->index].line[0] = '\0';
			prep_parenthesis_from_blanks(parse_out, parse_in);// ") " => ')' also elliminates double blanks
			flip = flip ^ 0x01;

			touched = 1;
			while (touched) {
				if (flip == 0) {
					parse_in = &parse->a;
					parse_out = &parse->b;
				}
				else {
					parse_in = &parse->b;
					parse_out = &parse->a;
				}
				parse_out->line[parse_out->index].line[0] = '\0';
				touched = parenthesis_clear_unecessary_brackets(parse_out, parse_in);// elliminate unecessary brackets [(...)] => [...]
				touched2 |= touched;
				flip = flip ^ 0x01;
			}
			touched = 1;
			UINT loops = 0;
			while (touched) {
				if (flip == 0) {
					parse_in = &parse->a;
					parse_out = &parse->b;
				}
				else {
					parse_in = &parse->b;
					parse_out = &parse->a;
				}
				parse_out->line[parse_out->index].line[0] = '\0';
				touched = parenthesis_depth_based(parse_out, parse_in);// [((...)| number1)|number2] => [(...)| number1|number2] 
				touched2 |= touched;
				flip = flip ^ 0x01;
				loops++;
			}
			touched = 1;
			while (touched) {
				if (flip == 0) {
					parse_in = &parse->a;
					parse_out = &parse->b;
				}
				else {
					parse_in = &parse->b;
					parse_out = &parse->a;
				}
				parse_out->line[parse_out->index].line[0] = '\0';
				touched = parenthesis_inner_set(parse_out, parse_in);// 
				touched2 |= touched;
				flip = flip ^ 0x01;
			}
			touched = 1;
			while (touched) {
				if (flip == 0) {
					parse_in = &parse->a;
					parse_out = &parse->b;
				}
				else {
					parse_in = &parse->b;
					parse_out = &parse->a;
				}
				parse_out->line[parse_out->index].line[0] = '\0';
				touched = parenthesis_simple_cleanup(parse_out, parse_in);// simple cleanup (a) =>a; func(a) =>func(a)
				touched2 |= touched;
				flip = flip ^ 0x01;
			}
			touched = 1;
			while (touched) {
				if (flip == 0) {
					parse_in = &parse->a;
					parse_out = &parse->b;
				}
				else {
					parse_in = &parse->b;
					parse_out = &parse->a;
				}
				parse_out->line[parse_out->index].line[0] = '\0';
				touched = parenthesis_add_assoc(parse_out, parse_in);
				touched2 |= touched;
				flip = flip ^ 0x01;
			}
			touched = 1;
			while (touched) {
				if (flip == 0) {
					parse_in = &parse->a;
					parse_out = &parse->b;
				}
				else {
					parse_in = &parse->b;
					parse_out = &parse->a;
				}
				parse_out->line[parse_out->index].line[0] = '\0';
				touched = double_bracket_reduction(parse_out, parse_in);// ((...)) => (..);
				touched2 |= touched;
				flip = flip ^ 0x01;
			}
			touched = 1;
			while (touched) {
				if (flip == 0) {
					parse_in = &parse->a;
					parse_out = &parse->b;
				}
				else {
					parse_in = &parse->b;
					parse_out = &parse->a;
				}
				parse_out->line[parse_out->index].line[0] = '\0';
				touched = parenthesis_num_plus_var(parse_out, parse_in);
				touched2 |= touched;
				flip = flip ^ 0x01;
			}
			touched = 1;
			while (touched) {
				if (flip == 0) {
					parse_in = &parse->a;
					parse_out = &parse->b;
				}
				else {
					parse_in = &parse->b;
					parse_out = &parse->a;
				}
				parse_out->line[parse_out->index].line[0] = '\0';
				touched = parenthesis_a_little_math(parse_out, parse_in, op_table);
				touched2 |= touched;
				flip = flip ^ 0x01;
			}
		}
		strcpy_p2(&parse->a, parse_out);
		parse->a.index++;
	}
	//	free(parse->b.line);
	FILE* dest;
	fopen_s(&dest, dst_name, "w");
	for (UINT i = 0; i < line_count; i++) {
		fprintf(dest, "%s", parse->a.line[i].line);
	}
	fclose(dest);
	//	free(parse->a.line);
}