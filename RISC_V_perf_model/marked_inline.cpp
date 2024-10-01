// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

data_type_enum getUnitType3(UINT8 select) {
	data_type_enum UnitType;
	int debug = 0;
	switch (select) {
	case 0:
		UnitType = void_enum;
		break;
	case 0x01:
		UnitType = int16_enum;
		break;
	case 0x02:
		UnitType = int32_enum;
		break;
	case 0x03:
		UnitType = int32_enum;
		break;
	case 0x04:
		UnitType = int8_enum;
		break;
	case 0x05:
		UnitType = int16_enum;
		break;
	case 0x06:
		UnitType = int32_enum;
		break;
	case 0x07:
		UnitType = int64_enum;
		break;
	case 0x08:
		UnitType = uint32_enum;
		break;
	case 0x09:
		UnitType = uint8_enum;
		break;
	case 0x0a:
		UnitType = uint16_enum;
		break;
	case 0x0b:
		UnitType = uint32_enum;
		break;
	case 0x0c:
		UnitType = uint64_enum;
		break;
	case 0x0d:
		UnitType = fp32_enum;
		break;
	case 0x0e:
		UnitType = fp64_enum;
		break;
	case 0x0f:
		UnitType = fp16_enum;
		break;
	default:
		debug++;
		break;
	}
	return UnitType;
}
UINT8 find_function3(FunctionListEntry3* entry, parse_struct2* parse, var_type_struct* var_type) {
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
			sprintf_s(entry->type, "%s", var_type[i].name);
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
							entry->argument[entry->arg_count].type = getUnitType3(j);
							while (parse->line[parse->index].line[parse->ptr] == ' ')parse->ptr++;
							entry->argument[entry->arg_count].pointer = (parse->line[parse->index].line[parse->ptr] == '*') ? 1 : 0;
							if (parse->line[parse->index].line[parse->ptr] == '*')parse->ptr++;
							while (parse->line[parse->index].line[parse->ptr] == '=' || parse->line[parse->index].line[parse->ptr] == ' ' || parse->line[parse->index].line[parse->ptr] == '\"' || parse->line[parse->index].line[parse->ptr] == '\t')parse->ptr++;
							getWord(parse);
							sprintf_s(entry->argument[entry->arg_count].name, "%s", parse->word);
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
void embed_code3(parse_struct2* parse_b, parse_struct2* parse_a, VariableListEntry* FuncVariableList, VariableListEntry* VariableList, var_type_struct* var_type, FunctionListEntry3 entry, UINT f_line_start, UINT f_line_num, INT8* depth) {
	int debug = 0;
	// copy function call/ subroutine line for reference (commented out)
	strcpy_p2(parse_b, (char*)"// ");
	strcat_p2(parse_b, parse_a);
	parse_b->index++;
	// handle return variable from function call
	UINT hold = parse_a->ptr;
	parse_a->ptr = 0;
	while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t') parse_a->ptr++;
	getWord(parse_a);
	if (strcmp(parse_a->word, "UINT8") == 0) { // function call only
		// re-formating line tabs
		parse_b->ptr = 0;
		for (UINT i = 0; i < depth[0]; i++) strcat_p2(parse_b, (char*)"\t");
		strcat_p2(parse_b, parse_a->word);

		while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t') {
			copy_char(parse_b, parse_a);
		}
		getWord(parse_a);
		strcat_p2(parse_b, parse_a->word);
		strcat_p2(parse_b, (char*)";\n");
		parse_b->index++;

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
						while (parse_a->line[parse_a->index].line[parse_a->ptr] != ']')entry.match_content[i][index++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
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
	for (UINT i = 0; i < depth[0]; i++) strcat_p2(parse_b, (char*) "\t");
	strcat_p2(parse_b, (char*)"{ \n");
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
							strcat_p2(parse_b, (char*) "uint8");
							parse_b->ptr += (2 + 5 + strlen(current->alias));
							break;
						default:
							debug++;
							break;
						}
						strcat_p2(parse_b, (char*)" ");
						strcat_p2(parse_b, current->alias);
						strcat_p2(parse_b, (char*)";\n");
						hit = 1;
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
				parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_funct.line[parse_funct.index].line[parse_funct.ptr++];
				parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
			}
			while (parse_funct.line[parse_funct.index].line[parse_funct.ptr] != '\0') {
				UINT match = 0;
				getWord(&parse_funct);
				if (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '/' && parse_funct.line[parse_funct.index].line[parse_funct.ptr + 1] == '/') {
					while (parse_funct.line[parse_funct.index].line[parse_funct.ptr] != '\0') {
						parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_funct.line[parse_funct.index].line[parse_funct.ptr++];
					}
					parse_b->line[parse_b->index].line[parse_b->ptr++] = '\0';
				}
				else if (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '/') {
					parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_funct.line[parse_funct.index].line[parse_funct.ptr++];
					parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
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
								parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
								parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
							}
							getWord(parse_a);
						}
						strcat_p2(parse_b, parse_a->word);
						parse_b->line[parse_b->index].line[parse_b->ptr++] = ' ';
						parse_b->line[parse_b->index].line[parse_b->ptr++] = '=';
						parse_b->line[parse_b->index].line[parse_b->ptr++] = ' ';
						parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';

						while (parse_funct.line[parse_funct.index].line[parse_funct.ptr] != '\0') {
							//											parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_funct.line[parse_funct.index].line[parse_funct.ptr++];
							while (parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ' ' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '\t' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == '\n' || parse_funct.line[parse_funct.index].line[parse_funct.ptr] == ';') {
								parse_b->line[parse_b->index].line[parse_b->ptr] = parse_funct.line[parse_funct.index].line[parse_funct.ptr];
								parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_funct.line[parse_funct.index].line[parse_funct.ptr++];
								parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
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
								parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_funct.line[parse_funct.index].line[parse_funct.ptr++];
								parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
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
								//	parse_funct.ptr++;
									strcat_p2(parse_b, &parse_funct);
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
								if (strlen(parse_funct.word)) {
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
							parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_funct.line[parse_funct.index].line[parse_funct.ptr++];
							parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
						}
					}
				}
			}
		}
		parse_b->index++;
	}
	// add protective brackets to keep variable declarations from spilling over - close brackets
	depth[0]--;
	parse_b->line[parse_b->index].line[0] = '\0';
	for (UINT i = 0; i < depth[0]; i++) strcat_p2(parse_b, (char*)"\t");
	strcat_p2(parse_b, (char*)"} \n");
	parse_b->index++;
	while (parse_a->line[parse_a->index].line[parse_a->ptr] == '\t' || parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == ';')parse_a->ptr++;
}
void MarkedInline(const char* dst_name, const char* src_name, INT flags, var_type_struct* var_type, parse_struct2_set* parse, int pass_count) {
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
	FunctionListEntry3* current_function;
	VariableListEntry* VariableList = (VariableListEntry*)malloc(sizeof(VariableListEntry));
	VariableList->next = VariableList;
	VariableList->last = VariableList;
	VariableListEntry* FuncVariableList = (VariableListEntry*)malloc(sizeof(VariableListEntry));
	FuncVariableList->next = FuncVariableList;
	FuncVariableList->last = FuncVariableList;

	parse_struct2* parse_a = &parse->a;
	parse_struct2* parse_b = &parse->b;
	//	UINT8 		loop_hit = 1;
	//	for (UINT pass = 0; pass < pass_count && loop_hit; pass++) {//6
	//	for (UINT pass = 0; pass < 1 && loop_hit; pass++) {//6
		/*
			if (pass & 1) {
				parse_b = &parse->a;
				parse_a = &parse->b;
			}
			else {
				parse_a = &parse->a;
				parse_b = &parse->b;
			}

			loop_hit = 0;
			/**/
	for (parse_a->index = 0, parse_b->index = 0; parse_a->index < line_count; parse_a->index++, parse_b->index++) {
		parse_a->ptr = 0;
		parse_b->ptr = 0;

		if (parse_a->index >= 2682)
			debug++;
		while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0') {
			if (parse_a->line[parse_a->index].line[parse_a->ptr] == '/' && parse_a->line[parse_a->index].line[parse_a->ptr + 1] == '/') {
				parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
				parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
				parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
				if (depth == 0) {
					while (parse_a->line[parse_a->index].line[parse_a->ptr] == ' ' || parse_a->line[parse_a->index].line[parse_a->ptr] == '\t')parse_a->ptr++;
					getWord(parse_a);
					if (strcmp(parse_a->word, "inline") == 0) {
						debug++;
						parse_struct2 parse_t;
						parse_t.index = parse_a->index + 1;
						parse_t.ptr = 0;
						parse_t.line = parse_a->line;
						FunctionListEntry3 entry;
						if (find_function3(&entry, &parse_t, var_type)) {
							parse_t.ptr++;
							while (parse_t.line[parse_t.index].line[parse_t.ptr] != '\0') {
								if (parse_t.line[parse_t.index].line[parse_t.ptr] == '{') {
									depth++;
									parse_t.ptr++;
								}
								else if (parse_t.line[parse_t.index].line[parse_t.ptr] == '}') {
									depth--;
									parse_t.ptr++;
								}
								else {
									parse_t.ptr++;
								}
							}
							if (depth < 1)
								debug++;
							while (depth > 0) { // skip target function
								parse_t.index++;
								parse_t.ptr = 0;
								while (parse_t.line[parse_t.index].line[parse_t.ptr] != '\0') {
									if (parse_t.line[parse_t.index].line[parse_t.ptr] == '/' && parse_t.line[parse_t.index].line[parse_t.ptr + 1] == '/') {
										while (parse_t.line[parse_t.index].line[parse_t.ptr] != '\0')parse_t.ptr++;
									}
									else if (parse_t.line[parse_t.index].line[parse_t.ptr] == '{') {
										depth++;
										parse_t.ptr++;
									}
									else if (parse_t.line[parse_t.index].line[parse_t.ptr] == '}') {
										depth--;
										parse_t.ptr++;
									}
									else {
										parse_t.ptr++;
									}
								}
							}
							parse_struct2 parse_s;
							parse_s.line = parse_t.line;
							for (parse_s.index = parse_t.index + 1; parse_s.index < line_count; parse_s.index++, parse_b->index++) {// look for function call
								parse_s.ptr = 0;
								parse_b->ptr = 0;
								parse_b->line[parse_b->index].line[0] = '\0';

								if (parse_s.index == 2104)
									debug++;
								while (parse_s.line[parse_s.index].line[parse_s.ptr] != '\0') {
									while (parse_s.line[parse_s.index].line[parse_s.ptr] == ' ' || parse_s.line[parse_s.index].line[parse_s.ptr] == '\t' || parse_s.line[parse_s.index].line[parse_s.ptr] == '=') {
										parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_s.line[parse_s.index].line[parse_s.ptr++];
										parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
									}
									if (parse_s.line[parse_s.index].line[parse_s.ptr] == '/' && parse_s.line[parse_s.index].line[parse_s.ptr + 1] == '/') {
										while (parse_s.line[parse_s.index].line[parse_s.ptr] != '\0')
											parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_s.line[parse_s.index].line[parse_s.ptr++];
										parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
									}
									getWord(&parse_s);
									if (strlen(parse_s.word)) {
										if (strcmp(parse_s.word, entry.name) == 0) {
											if (parse_s.line[parse_s.index].line[parse_s.ptr] == '(')
												parse_s.ptr++;
											else
												debug++;
											for (UINT i = 0; i < entry.arg_count; i++) {
												getWord(&parse_s);
												if (strlen(parse_s.word))
													sprintf_s(entry.argument[i].alias, "%s", parse_s.word);
												else
													debug++;
												while (parse_s.line[parse_s.index].line[parse_s.ptr] == ' ' || parse_s.line[parse_s.index].line[parse_s.ptr] == '\t')parse_s.ptr++;
												if (parse_s.line[parse_s.index].line[parse_s.ptr] == ',' || parse_s.line[parse_s.index].line[parse_s.ptr] == ')')
													parse_s.ptr++;
												else
													debug++;
											}
											sprintf_s(parse_b->line[parse_b->index++].line, "//%s", &parse_s);
											sprintf_s(parse_b->line[parse_b->index++].line, "//%s", parse_a->line[parse_a->index + 1].line);
											sprintf_s(parse_b->line[parse_b->index++].line, "{\n");
											parse_struct2 parse_r;
											parse_r.line = parse_a->line;
											for (parse_r.index = parse_a->index + 2; parse_r.index < parse_t.index; parse_r.index++, parse_b->index++) {// insert function
												parse_r.ptr = 0;
												parse_b->ptr = 0;
												parse_b->line[parse_b->index].line[0] = '\0';
												while (parse_r.line[parse_r.index].line[parse_r.ptr] != '\0') {
													while (parse_r.line[parse_r.index].line[parse_r.ptr] == ' ' || parse_r.line[parse_r.index].line[parse_r.ptr] == '\t' || parse_r.line[parse_r.index].line[parse_r.ptr] == '=')
														copy_char(parse_b, &parse_r);
													getWord(&parse_r);
													if (strlen(parse_r.word)) {
														char hit = 0;
														for (UINT i = 0; i < entry.arg_count && hit == 0; i++) {
															if (strcmp(parse_r.word, entry.argument[i].name) == 0) {
																strcat_p2(parse_b, entry.argument[i].alias);
																hit = 1;
															}
														}
														if (hit == 0) {
															strcat_p2(parse_b, parse_r.word);
														}
													}
													else {
														copy_char(parse_b, &parse_r);

													}
												}
											}
											while (parse_s.line[parse_s.index].line[parse_s.ptr] != '\0')parse_s.ptr++;
											strcpy_p2(parse_b, (char*)"{{}\n");
										}
										else {
											strcat_p2(parse_b, parse_s.word);
										}
									}
									else if (parse_s.line[parse_s.index].line[parse_s.ptr] != '\0') {
										copy_char(parse_b, &parse_s);
									}
								}
							}
							debug++;
							parse_a->index = line_count - 1;
							parse_a->ptr = strlen(parse_a->line[parse_a->index].line);
							UINT ptr = strlen(parse_b->line[parse_b->index - 1].line);
							parse_b->line[parse_b->index - 1].line[ptr] = '\0';
						}
						else {
							debug++;
						}
					}
				}
				else {
					while (parse_a->line[parse_a->index].line[parse_a->ptr] != '\0')
						copy_char(parse_b, parse_a);
				//		parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
				//	parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
				}
			}
			else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '{') {
				depth++;
				copy_char(parse_b, parse_a);
			//	parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
			//	parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
			}
			else if (parse_a->line[parse_a->index].line[parse_a->ptr] == '}') {
				depth--;
				copy_char(parse_b, parse_a);
//				parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
//				parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
			}
			else {
				copy_char(parse_b, parse_a);
//				parse_b->line[parse_b->index].line[parse_b->ptr++] = parse_a->line[parse_a->index].line[parse_a->ptr++];
//				parse_b->line[parse_b->index].line[parse_b->ptr] = '\0';
			}

		}
	}
	for (VariableListEntry* current = FuncVariableList->next; current != FuncVariableList; current = current->next) {
		current->last->next = current->next;
		current->next->last = current->last;
		VariableListEntry* temp = current->last;
		free(current);
		current = temp;
	}
	line_count = parse_b->index - 1;
	FILE* dest;
	fopen_s(&dest, dst_name, "w");
	parse_b->index--;
	for (UINT line_number = 0; line_number < parse_b->index; line_number++) {
		fprintf(dest, "%s", parse_b->line[line_number].line);
	}
	UINT ptr = strlen(parse_b->line[parse_b->index - 1].line);
	parse_b->line[parse_b->index - 1].line[ptr] = '\0';
	fclose(dest);
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