// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>


struct FunctionListEntry2 {
	char name[0x100];
	char type[0x10]; // label, UINT8, UINT16,... enumerated
	UINT8 type_index;
	VariableListEntry argument[8];
	char match[8][0x100];
	char match_content[8][0x100];
	UINT8 arg_count;

	FunctionListEntry2* next, * last;
};

UINT8 get_integer2(INT64* number, char* input) {
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
UINT8 match_list2(UINT8* index, char* word, char** list, UINT8 list_count) {
	UINT8 result = 0;
	for (index[0] = 0; index[0] < list_count && !result; index[0]++) {
		if (strcmp(word, list[index[0]]) == 0) {
			result = 1;
		}
	}
	if (result) {
		index[0]--;
	}
	return result;
}
void getWord2(parse_type* parse) {
	int debug = 0;
	UINT16 length = strlen(parse->line);
	UINT16 i;

	for (i = parse->ptr; i < length && debug == 0; i++) {
		if (parse->line[i] != ' ' && parse->line[i] != '\t' && parse->line[i] != '\0' && parse->line[i] != '\n' && parse->line[i] != ',' && parse->line[i] != ';' && parse->line[i] != ':' && parse->line[i] != '=' && parse->line[i] != '?' && parse->line[i] != '&' &&
			parse->line[i] != '|' && parse->line[i] != '^' && parse->line[i] != '+' && parse->line[i] != '-' && parse->line[i] != '*' && parse->line[i] != '/' && parse->line[i] != '<' && parse->line[i] != '>' &&
			parse->line[i] != '(' && parse->line[i] != ')' && parse->line[i] != '{' && parse->line[i] != '}' && parse->line[i] != '[' && parse->line[i] != ']' && parse->line[i] != '\"') {
			parse->word[i - parse->ptr] = parse->line[i];
		}
		else {
			debug++;
		}
	}
	parse->word[i - parse->ptr - 1] = '\0';
	parse->ptr += strlen(parse->word);
}

UINT8 get_func_arg(VariableListEntry* list_base, parse_type* parse, UINT k, char** var_type) {
	UINT8 result = 0;
	int debug = 0;
	VariableListEntry* current = (VariableListEntry*)malloc(sizeof(VariableListEntry));

	if (parse->line[parse->ptr] == '*') {
		current->pointer = 1;
		parse->ptr++;
	}
	else {
		current->pointer = 0;
	}
	current->atomic = 0;
	current->function = 0;
	current->addr = 0;

	getWord2(parse);
	sprintf_s(current->name, "%s", parse->word);
	switch (k) {
	default:
		debug++;
		break;
	}
	if (parse->line[parse->ptr] == '(' && parse->line[parse->ptr + 1] == ')') {
		current->function = 1;
		parse->ptr += 2;
	}

	// add to variable list
	current->last = list_base->last;
	current->next = list_base;
	list_base->last->next = current;
	list_base->last = current;

	result = 1;
	return result;
}
UINT8 get_VariableType2(char** var_type, parse_type* parse, VariableListEntry* list_base) {
	UINT8 result = 0;
	for (UINT k = 0; k < var_type_count && result == 0; k++) {
		if (strcmp(parse->word, var_type[k]) == 0) {
			VariableListEntry* current = (VariableListEntry*)malloc(sizeof(VariableListEntry));

			if (parse->line[parse->ptr] == '*') {
				current->pointer = 1;
				parse->ptr++;
			}
			else {
				current->pointer = 0;
			}
			current->atomic = 0;
			//			current->level = 0;
			current->addr = 0;
			while (parse->line[parse->ptr] == ' ')parse->ptr++;
			result = (parse->line[parse->ptr] == ')') ? 1 : -1;
			parse->ptr++;
			while (parse->line[parse->ptr] == ' ' || parse->line[parse->ptr] == '\t')	parse->ptr++;
		}
	}
	return result;
}

UINT8 get_FunctionListEntry22(FunctionListEntry2** current, char* word, FunctionListEntry2* list_base) {
	UINT8 result = 0;
	for (current[0] = list_base->next; current[0] != list_base & !result; current[0] = current[0]->next) {
		if (strcmp(word, current[0]->name) == 0) {
			result = 1;
		}
	}
	if (result) {
		current[0] = current[0]->last;
	}
	return result;

}
UINT8 get_VariableListEntry2(VariableListEntry** current, char* word, VariableListEntry* list_base) {
	UINT8 result = 0;
	for (current[0] = list_base->next; current[0] != list_base & !result; current[0] = current[0]->next) {
		if (strcmp(word, current[0]->name) == 0) {
			result = 1;
		}
	}
	if (result) {
		current[0] = current[0]->last;
	}
	return result;
}
//need to add in destination pointer into arguments

void get_modifier2(UINT8* modifier_index, parse_type* parse, char modifier_type[2][0x100]) {
	for (UINT k = 0; k < 2; k++) {
		if (strcmp(parse->word, modifier_type[k]) == 0) {
			modifier_index[0] = k;
			getWord2(parse);
			k = 0x10;
		}
	}
}
void getWord_local(parse_struct2* parse) {
	int debug = 0;
	//	char result[0x100];
	UINT16 length = strlen(parse->line[parse->index].line);
	UINT16 i;
	if (parse->ptr >= length) {
		debug++;
	}
	else {
		while (parse->line[parse->index].line[parse->ptr] == '=' || parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\"' || parse->line[parse->index].line[parse->ptr] == '\t')parse->ptr++;

		for (i = parse->ptr; (i < length) && (debug == 0); i++) {
			if (parse->line[parse->index].line[i] != ' ' && parse->line[parse->index].line[i] != '\t' && parse->line[parse->index].line[i] != '\0' && parse->line[parse->index].line[i] != '\n' && parse->line[parse->index].line[i] != ',' && parse->line[parse->index].line[i] != ';' && parse->line[parse->index].line[i] != ':' && parse->line[parse->index].line[i] != '=' && parse->line[parse->index].line[i] != '?' && parse->line[parse->index].line[i] != '&' &&
				parse->line[parse->index].line[i] != '|' && parse->line[parse->index].line[i] != '^' && parse->line[parse->index].line[i] != '+' && parse->line[parse->index].line[i] != '-' && parse->line[parse->index].line[i] != '*' && parse->line[parse->index].line[i] != '/' && parse->line[parse->index].line[i] != '<' && parse->line[parse->index].line[i] != '>' &&
				parse->line[parse->index].line[i] != '(' && parse->line[parse->index].line[i] != ')' && parse->line[parse->index].line[i] != '{' && parse->line[parse->index].line[i] != '}' && parse->line[parse->index].line[i] != '[' && parse->line[parse->index].line[i] != ']' && parse->line[parse->index].line[i] != '\"') {
				parse->word[i - parse->ptr] = parse->line[parse->index].line[i];
				parse->word[i - parse->ptr+1] = '\0';
			}
			else {
				debug++;
			}
		}
		parse->ptr +=strlen(parse->word);
	}
}
UINT8 find_variable(VariableListEntry* VariableList, parse_struct2* parse, var_type_struct* var_type, UINT depth) {
	UINT8 response = 0;
	int debug = 0;
	UINT hold = parse->ptr;
	while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\t')parse->ptr++;
	getWord_local(parse);
	for (UINT8 i = 0; i < var_type_count & !response; i++) {
		if (strcmp(parse->word, var_type[i].name) == 0) {
			VariableListEntry* entry = (VariableListEntry*)malloc(sizeof(VariableListEntry));
			response = 1;
			entry->type = var_type[i].type;
			/*
			switch (i) {
			case 0:
				entry->type = void_enum;
				break;
			case 0x01:
				entry->type = int16_enum;
				break;
			case 0x02:
				entry->type = int32_enum;
				break;
			case 0x03:
				entry->type = int32_enum;
				break;
			case 0x04:
				entry->type = int8_enum;
				break;
			case 0x05:
				entry->type = int16_enum;
				break;
			case 0x06:
				entry->type = int32_enum;
				break;
			case 0x07:
				entry->type = int64_enum;
				break;
			case 0x08:
				entry->type = int128_enum;
				break;
			case 0x09:
				entry->type = uint32_enum;
				break;
			case 0x0a:
				entry->type = uint8_enum;
				break;
			case 0x0b:
				entry->type = uint16_enum;
				break;
			case 0x0c:
				entry->type = uint32_enum;
				break;
			case 0x0d:
				entry->type = uint64_enum;
				break;
			case 0x0e:
				entry->type = uint128_enum;
				break;
			case 0x0f:
				entry->type = fp32_enum;
				break;
			case 0x10:
				entry->type = fp64_enum;
				break;
			case 0x11:
				entry->type = fp16_enum;
				break;
			default:
				debug++;
				break;
			}
			/**/
			entry->depth = depth;
			while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\t')parse->ptr++;
			if (parse->line[parse->index].line[parse->ptr] == '*') {
				entry->pointer = 1;
				parse->ptr++;
				while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\t')parse->ptr++;
			}
			else {
				entry->pointer = 0;
			}
			getWord(parse);
			sprintf_s(entry->name, "%s", parse->word);
			sprintf_s(entry->alias, "%s", parse->word);
			if (parse->line[parse->index].line[parse->ptr] == '[')
				entry->pointer = 1;
			else if (parse->line[parse->index].line[parse->ptr] == '(') {// function call
				response = 0;
				i = 10;
				parse->ptr = hold;
			}
			if (response == 1) {
				VariableList->last->next = entry;
				entry->last = VariableList->last;
				VariableList->last = entry;
				entry->next = VariableList;
			}
			else {
				free(entry);
			}
		}
	}
	if (response == 0)
		parse->ptr = hold;
	return response;
}
UINT8 find_function(FunctionListEntry2* entry, parse_struct2* parse, var_type_struct* var_type) {
	UINT8 result = 0;
	int debug = 0;

	while (parse->line[parse->index].line[parse->ptr] == '=' || parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\"' || parse->line[parse->index].line[parse->ptr] == '\t')parse->ptr++;
	getWord(parse);
	INT8 index = -1;
	for (UINT8 i = 0; i < var_type_count && index == -1; i++) {
		if (strcmp(parse->word, var_type[i].name) == 0) {
			index = i;
			while (parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\t')parse->ptr++;
			getWord(parse);
			sprintf_s(entry->name, "%s", parse->word);
			sprintf_s(entry->type, "%s", var_type[i]);
			entry->type_index = i;
			entry->arg_count = 0;
			while (parse->line[parse->index].line[parse->ptr] == ' ')parse->ptr++;
			if (parse->line[parse->index].line[parse->ptr] == '(') {
				result = 1;
				parse->ptr++;
				while (parse->line[parse->index].line[parse->ptr] != ')') {
					while (parse->line[parse->index].line[parse->ptr] == '=' || parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\"' || parse->line[parse->index].line[parse->ptr] == '\t')parse->ptr++;
					getWord(parse);
					for (UINT8 j = 0; j < var_type_count; j++) {
						if (strcmp(parse->word, var_type[j].name) == 0) {
							entry->argument[entry->arg_count].type = var_type[j].type;
							while (parse->line[parse->index].line[parse->ptr] == ' ')parse->ptr++;
							entry->argument[entry->arg_count].pointer = (parse->line[parse->index].line[parse->ptr] == '*') ? 1 : 0;
							if (parse->line[parse->index].line[parse->ptr] == '*')parse->ptr++;
							while (parse->line[parse->index].line[parse->ptr] == '=' || parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\"' || parse->line[parse->index].line[parse->ptr] == '\t')parse->ptr++;
							getWord(parse);
							sprintf_s(entry->argument[entry->arg_count].name,"%s", parse->word);
							if (parse->line[parse->index].line[parse->ptr] == '[') {
								if (entry->argument[entry->arg_count].pointer) {
									debug++;
								}
								else {
									entry->argument[entry->arg_count].pointer = 1;
								}
								while (parse->line[parse->index].line[parse->ptr] != ']')parse->ptr++;
								parse->ptr++;
							}
							entry->arg_count++;
						}
					}
					if (parse->line[parse->index].line[parse->ptr] == ',') {
						parse->ptr++;
					}
					else if (parse->line[parse->index].line[parse->ptr] != ')') {
						debug++;
					}
				}
				if (entry->arg_count > 8)
					debug++;
			}
		}
	}
	return result;
}
void embed_code(parse_struct2* parse_b, parse_struct2* parse_a, VariableListEntry* FuncVariableList, VariableListEntry* VariableList, var_type_struct* var_type, FunctionListEntry2 entry, UINT f_line_start, UINT f_line_num, INT8* depth) {
	int debug = 0;
	// copy function call/ subroutine line for reference (commented out)
	sprintf_s(parse_b->line[parse_b->index++].line, "// %s", parse_a->line[parse_a->index].line);
	// handle return variable from function call
	UINT hold = parse_a->ptr;
	parse_a->ptr = 0;
	while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t') parse_a->ptr++;
	getWord(parse_a);
	if (strcmp(parse_a->word, "UINT8") == 0) { // function call only
		// re-formating line tabs
		parse_b->ptr = 0;
		for (UINT i = 0; i < depth[0]; i++) strcat_p2(parse_b,(char *) "\t");

		strcat_p2(parse_b, parse_a->word);
		parse_b->ptr = strlen(parse_b->line[parse_b->index].line);
		while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t') {
			copy_char(parse_b, parse_a);
		}
		getWord(parse_a);
		strcat_p2(parse_b, parse_a->word);
		parse_b->line[parse_b->index].line[parse_b->ptr++] = ';';
		parse_b->line[parse_b->index].line[parse_b->ptr++] = '\n';
		parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
		parse_b->index++;
		parse_b->ptr = 0;
		parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';

		parse_a->ptr = hold;
		strcpy_p2(parse_b, (char*)"");
		while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t') {
			strcat_p2(parse_b, &parse_a->line[parse_a->index].line[parse_a->ptr++]);
			parse_b->index++;
		}

		parse_a->ptr = hold;
		parse_b->ptr = 0;
		// initialize passed arguments
	}
	else {
		debug++;
	}
	if (parse_a->line[parse_a->index].line[parse_a->ptr] == '(') {
		parse_a->ptr++;
	}
	else {
		debug++;
	}
	for (UINT i = 0; i < entry.arg_count; i++) {// get argument replacements
		while (parse_a->line[parse_a->index].line[parse_a->ptr] == '=' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\"' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
		UINT hold = parse_a->ptr;
		getWord(parse_a);
		if (parse_a->line[parse_a->index].line[parse_a->ptr] == ',' || parse_a->line[parse_a->index].line[parse_a->ptr] == ')') {
			sprintf_s(entry.match[i], "%s", parse_a->word);
			entry.match_content[i][0] = '\0';
		}
		else {
			entry.match_content[i][0] = '\0';
			if (entry.argument[i].pointer) {
				if (parse_a->line[parse_a->index].line[parse_a->ptr] == '&') {
					parse_a->ptr++;
					getWord(parse_a);
					if (parse_a->line[parse_a->index].line[parse_a->ptr] == '[') {
						sprintf_s(entry.match[i], "%s", parse_a->word);
						parse_a->ptr++;
						UINT index = 0;
						entry.match_content[i][index++] = '(';
						while (parse_a->line[parse_a->index].line[parse_a->ptr] != ']')
							entry.match_content[i][index++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
						entry.match_content[i][index++] = ')';
						entry.match_content[i][index++] = '+';
						entry.match_content[i][index++] = '\0';
						if (parse_a->line[parse_a->index].line[parse_a->ptr] == ']')parse_a->ptr++;
					}
					else {
						sprintf_s(entry.match[i],"&%s", parse_a->word);
					}
				}
				else {
					sprintf_s(entry.match[i], "%s", parse_a->word);
					entry.match_content[i][0] = '\0';
				}
			}
			else {
				UINT index = 0;
				entry.match[i][index++] = '(';
				UINT brackets = 0;
				while (parse_a->line[parse_a->index].line[hold] != ',' && !(parse_a->line[parse_a->index].line[hold] == ')' && brackets == 0) && parse_a->line[parse_a->index].line[hold] != '\0') {
					if (parse_a->line[parse_a->index].line[hold] == '(') brackets++;
					else if (parse_a->line[parse_a->index].line[hold] == ')') brackets--;
					entry.match[i][index++] = parse_a->line[parse_a->index].line[hold++];
				}
				entry.match[i][index++] = ')';
				entry.match[i][index++] = '\0';
				parse_a->ptr = hold;
			}
		}
		if (parse_a->line[parse_a->index].line[parse_a->ptr] == ',') {
			parse_a->ptr++;
		}
	}
	if (parse_a->line[parse_a->index].line[parse_a->ptr] == ')') {
		parse_a->ptr++;
	}
	else {
		debug++;
	}
	// add protective brackets to keep variable declarations from spilling over
//	parse_b->line[parse_b->index].line[0] = '\0';
	parse_b->ptr = 0;
	for (UINT i = 0; i < depth[0]; i++) strcat_p2(parse_b,(char*) "\t");
	strcat_p2(parse_b,(char*) "{ \n");
	parse_b->index++;
	depth[0]++;
	// copy called code into callee
	parse_struct2 parse_funct;
	parse_funct.line = parse_a->line;
	parse_funct.ptr = 0;
	for (parse_funct.index = f_line_start; parse_funct.index < f_line_num - 1; parse_funct.index++) {
		// reformat tabbing of copied lines for easier debug
		parse_funct.ptr = 0;
		while (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ' ' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '\t')
			parse_funct.ptr++;

		parse_b->ptr = 0;

		for (UINT i = 0; i < depth[0]; i++) parse_b->line[parse_b->index].line[parse_b->ptr++] = '\t';
		parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';

		UINT hit = 0;
		if (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '/' && parse_funct.line[parse_funct.index].line[parse_funct.ptr + 1] == '/') {
			strcat_p2(parse_b, parse_funct.line[parse_funct.index].line);
		}
		else {
			if (find_variable(FuncVariableList, &parse_funct, var_type, depth[0])) {// load up variable list
				VariableListEntry* current = FuncVariableList->last;
				for (VariableListEntry* ref = VariableList->next; ref != VariableList && !hit; ref = ref->next) {
					if (strcmp(ref->name, current->alias) == 0) {
						char word[0x80];
						sprintf_s(word, "%s_1", current->alias);
						sprintf_s(current->alias, "%s", word);
						ref = VariableList;
						switch (current->type) {
						case uint8_enum:
							strcat_p2(parse_b, (char*)"UINT8");
							break;
						case uint16_enum:
							strcat_p2(parse_b, (char*)"UINT16");
							break;
						default:
							debug++;
							break;
						}
						strcat_p2(parse_b, (char*)" ");
						strcat_p2(parse_b, current->alias);
						strcat_p2(parse_b, (char*)";\n");
						hit = 1;
						parse_funct.ptr = strlen(parse_funct.line[parse_funct.index].line);
					}
				}
			}
			if (!hit) {
				// reformat tabbing of copied lines for easier debug
				parse_funct.ptr = 0;
				while (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ' ' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '\t')
					parse_funct.ptr++;

				parse_b->ptr = 0;
				for (UINT i = 0; i < depth[0]; i++) parse_b->line[parse_b->index].line[parse_b->ptr++] = '\t';
				parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
			}
			while (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ' ' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '\t' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '\n' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ';') {
				copy_char(parse_b, &parse_funct);
			}
			while (parse_funct.line[parse_funct.index].line[parse_funct.ptr] != '\0') {
				UINT match = 0;
				getWord(&parse_funct);
				if (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '/' && parse_funct.line[parse_funct.index].line[parse_funct.ptr + 1] == '/') {
					while (parse_funct.line[parse_funct.index].line[parse_funct.ptr] != '\0') {
						copy_char(parse_b, &parse_funct);
					}
				}
				else if (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '/') {
					copy_char(parse_b, &parse_funct);
				}
				if (parse_funct.line[parse_funct.index].line[parse_funct.ptr] != '\0') {
					if (strcmp("return", parse_funct.word) == 0) {
						parse_a->ptr = 0;
						while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')
							parse_a->ptr++;

						parse_b->ptr = 0;
						for (UINT i = 0; i < depth[0]; i++) parse_b->line[parse_b->index].line[parse_b->ptr++] = '\t';
						parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';

						getWord(parse_a);
						if (strcmp(parse_a->word, "UINT8") == 0) {
							while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t') {
								copy_char(parse_b, parse_a);
							}
							getWord(parse_a);
						}
						strcat_p2(parse_b, parse_a->word);
						strcat_p2(parse_b, (char*)" = ");

						while (parse_funct.line[parse_funct.index].line[parse_funct.ptr] != '\0') {
							while (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ' ' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '\t' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '\n' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ';') {
								copy_char(parse_b, &parse_funct);
								copy_char(parse_b, &parse_funct);
							}
							UINT match = 0;
							getWord(&parse_funct);
							for (UINT i = 0; i < entry.arg_count; i++) {
								if (strcmp(entry.argument[i].name, parse_funct.word) == 0) {
									match = 1;
									strcat_p2(parse_b, entry.match[i]);
								}
							}
							if (!match) {
								for (VariableListEntry* ref = FuncVariableList->next; ref != FuncVariableList && !match; ref = ref->next) {
									if (strcmp(ref->name, parse_funct.word) == 0) {
										strcat_p2(parse_b, ref->alias);
										match = 1;
									}
								}
								if (!match) {
									strcat_p2(parse_b, parse_funct.word);
								}
							}
							while (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ' ' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '\t' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '\n' ||
								parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '{' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '}' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ',' ||
								parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '(' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ')' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '|' ||
								parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '[' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ']' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '/' ||
								parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '<' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '>' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '^' ||
								parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '-' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '+' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '=' ||
								parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '*' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '&' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ';') {
								if (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '{')
									depth[0]++;
								else if (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '}') {
									parse_b->ptr--;
									depth[0]--;
								}
								copy_char(parse_b, &parse_funct);
							}
						}
						parse_b->line[parse_b->index].line[parse_b->ptr++] = '\0';
						while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0') parse_a->ptr++;
					}
					else {
						match = 0;
						for (UINT i = 0; i < entry.arg_count && !match; i++) {
							if (strcmp(entry.argument[i].name, parse_funct.word) == 0) {
								match = 1;
								strcat_p2(parse_b, entry.match[i]);
								if (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '[') {
									parse_funct.ptr++;
									strcat_p2(parse_b, (char*)"[");
									strcat_p2(parse_b, entry.match_content[i]);
									strcat_p2(parse_b, (char*)"(");
								}
							}
						}
						if (!match) {
							for (VariableListEntry* ref = FuncVariableList->next; ref != FuncVariableList && !match; ref = ref->next) {
								if (strcmp(ref->name, parse_funct.word) == 0) {
									strcat_p2(parse_b, ref->alias);
									match = 1;
								}
							}
							if (!match) {
								if (parse_funct.word[0]!='\0') {
									strcat_p2(parse_b, parse_funct.word);
								}
							}
						}
						if (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '[') {
							parse_funct.ptr++;
							strcat_p2(parse_b, (char*)"[(");
						}
						else if (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ']') {
							parse_funct.ptr++;
							strcat_p2(parse_b, (char*)")]");
						}
						while (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ' ' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '\t' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '\n' ||
							parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '{' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '}' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ',' ||
							parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '(' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ')' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '|' ||
							parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '?' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ':' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '!' ||
							parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '<' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '>' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '^' ||
							parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '-' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '+' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '=' ||
							parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '*' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '&' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ';') {
							if (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '{')
								depth[0]++;
							else if (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '}') {
								if (parse_funct.line[parse_funct.index].line[parse_funct.ptr - 1] != '{')
									parse_b->ptr--;
								depth[0]--;
							}
							copy_char(parse_b,&parse_funct);
						}
					}
				}
			}
		}
		parse_b->index++;
	}
	// add protective brackets to keep variable declarations from spilling over - close brackets
	depth[0]--;
//	parse_b->line[parse_b->index].line[0] = '\0';
	parse_b->ptr = 0;
	for (UINT i = 0; i < depth[0]; i++) strcat_p2(parse_b,(char*) "\t");
	strcat_p2(parse_b, (char*)"} \n");
	parse_b->index++;
	parse_b->ptr = 0;
	while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == ';')parse_a->ptr++;
}
void inline_C(const char* dst_name, const char* src_name, INT flags, var_type_struct* var_type, parse_struct2_set* parse, int pass_count) {
	int debug = 0;
	FILE* src;
	fopen_s(&src, src_name, "r");
	char modifier_type[2][0x100];
	sprintf_s(modifier_type[0], "none");
	sprintf_s(modifier_type[1], "atomic");

	UINT  line_count = 0;

	INT8 depth = 0;

	while (fgets(parse->a.line[line_count++].line, 0x100, src) != NULL) {}

	fclose(src);
	line_count--;
	FunctionListEntry2* current_function;
	VariableListEntry* VariableList = (VariableListEntry*)malloc(sizeof(VariableListEntry));
	VariableList->next = VariableList;
	VariableList->last = VariableList;
	VariableListEntry* FuncVariableList = (VariableListEntry*)malloc(sizeof(VariableListEntry));
	FuncVariableList->next = FuncVariableList;
	FuncVariableList->last = FuncVariableList;

	parse_struct2* parse_a = &parse->a;
	parse_struct2* parse_b = &parse->b;
	UINT8 		loop_hit = 1;
	for (UINT pass = 0; pass < pass_count && loop_hit; pass++) {//6
//	for (UINT pass = 0; pass < 1 && loop_hit; pass++) {//6
		if (pass & 1) {
			parse_b = &parse->a;
			parse_a = &parse->b;
		}
		else {
			parse_a = &parse->a;
			parse_b = &parse->b;
		}
		loop_hit = 0;
		depth = 0;

		for (parse_a->index = 0, parse_b->index = 0; parse_a->index < line_count; parse_a->index++) {
			parse_a->ptr = 0;
			parse_b->ptr = 0;

			if (parse_a->index == 14)
				debug++;
			FunctionListEntry2 entry;
			if (find_function(&entry, parse_a, var_type)) {
				parse_a->ptr++;
				while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')parse_a->ptr++;
				if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{')depth++;
				else if (parse_a->line[parse_a->index].line[parse_a->ptr] == ';')
					debug++;
				else
					debug++;
				UINT f_line_num = parse_a->index + 1;
				UINT f_line_start = parse_a->index + 1;
				while (depth > 0 && f_line_num < line_count) {// find end of function
					for (UINT i = 0; i < 0x100 && parse_a->line[f_line_num].line[i] != '\n'; i++) {
						if (parse_a->line[f_line_num].line[i] == '{')depth++;
						else if (parse_a->line[f_line_num].line[i] == '}')depth--;
					}
					f_line_num++;
					if (depth < 0)
						debug++;
				}
				for (parse_a->index = f_line_num; parse_a->index < line_count; parse_a->index++) {// copy rest of code after function call
					parse_a->ptr = 0;
					while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')parse_a->ptr++;
					if (parse_a->index == 100)
						debug++;

					if (find_variable(VariableList, parse_a, var_type, depth)) {// load up variable list
						debug++;
					}
					UINT hit = 0;
					while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\n' && parse_a->line[parse_a->index].line[parse_a->ptr] != '\0') { // check for function name matches
						while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ')parse_a->ptr++;
						if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{') {
							depth++;
							parse_a->ptr++;
						}
						else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '}') {
							depth--;
							parse_a->ptr++;
							for (VariableListEntry* current = VariableList->next; current != VariableList; current = current->next) {
								if (current->depth > depth) {
									current->last->next = current->next;
									current->next->last = current->last;
									VariableListEntry* temp = current->last;
									free(current);
									current = temp;
								}
							}
						}
						if (depth < 0) {
							debug++;
						}
						else {
							switch (parse_a->line[parse_a->index].line[parse_a->ptr]) {
							case '(': {
								parse_a->ptr++;
								if (parse_a->line[parse_a->index].line[parse_a->ptr] == '/' && parse_a->line[parse_a->index].line[parse_a->ptr + 1] == '/') {
									while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\n')parse_a->ptr++;
								}
								else {
									while (parse_a->line[parse_a->index].line[parse_a->ptr] != ')' && parse_a->line[parse_a->index].line[parse_a->ptr] != '\n')parse_a->ptr++;
									parse_a->ptr++;
								}
							}
									break;
							case '|':
							case '^':
							case '&':
							case '[':
							case ']':
							case '<':
							case '>':
							case '+':
							case '-':
							case '*':
							case ';':
							case ',':
							case ')':
							case '?':
							case ':':
								parse_a->ptr++;
								break;
							case '/':
								if (parse_a->line[parse_a->index].line[parse_a->ptr + 1] == '/') {
									while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\n')parse_a->ptr++;
								}
								else {
									parse_a->ptr++;
								}
								break;
							default:
								debug++;
								break;
							}
						}
						if (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0') {
							while (parse_a->line[parse_a->index].line[parse_a->ptr] == '=' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\"' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
							// subroutine (does not return value)
							UINT depth_state = depth;
							if (strcmp(parse_a->word, entry.name) == 0) {// function call (returns a value)
								hit = 1;
								loop_hit = 1;
								embed_code(parse_b, parse_a, FuncVariableList, VariableList, var_type, entry, f_line_start, f_line_num, &depth);
							}
							getWord(parse_a);
							if (strcmp(parse_a->word, entry.name) == 0) {// function call (returns a value)
								hit = 1;
								loop_hit = 1;
								embed_code(parse_b, parse_a, FuncVariableList, VariableList, var_type, entry, f_line_start, f_line_num, &depth);
							}
							if (depth != depth_state)
								debug++;
						}
					}
					if (!hit) {
						strcpy_p2(parse_b, parse_a);
						parse_b->index++;
					}
				}
				parse_a->index = line_count;  // exit loop
			}
			else {
				strcpy_p2(parse_b, parse_a);
				parse_b->index++;
			}
		}
		for (VariableListEntry* current = FuncVariableList->next; current != FuncVariableList; current = current->next) {
			current->last->next = current->next;
			current->next->last = current->last;
			VariableListEntry* temp = current->last;
			free(current);
			current = temp;
		}
		line_count = parse_b->index;
	}
	FILE* dest;
	fopen_s(&dest, dst_name, "w");

	for (UINT line_number = 0; line_number < parse_b->index; line_number++) {
		fprintf(dest, "%s", parse_b->line[line_number].line);
	}
	fclose(dest);
	//	free(parse_1.line);
	//	free(parse_2.line);
	while (VariableList->next != VariableList) {
		VariableListEntry* temp = VariableList->next;
		VariableList->next->next->last = VariableList;
		VariableList->next = VariableList->next->next;
		free(temp);
	}
	while (FuncVariableList->next != FuncVariableList) {
		VariableListEntry* temp = FuncVariableList->next;
		FuncVariableList->next->next->last = FuncVariableList;
		FuncVariableList->next = FuncVariableList->next->next;
		free(temp);
	}
	free(VariableList);
	free(FuncVariableList);
}