// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "ROB.h"

#include <intrin.h>
using namespace std;

#pragma intrinsic(__ll_lshift)


void OP_unit(reg_bus* rd, UINT16* exec_rsp, R_type* op_exec, UINT64 clock, UINT8 q_num, UINT mhartid, UINT debug_core, param_type param, FILE* debug_stream) {
	int debug = 0;
	if (mhartid == 0)
		if (clock >= 0x0109)
			debug++;
	int stop = 0;
	rd->strobe = 0;
	exec_rsp[0] = 0;
	if (op_exec->strobe) {
		rd->ROB_id = op_exec->ROB_id;
		if (mhartid == 0)
			if (clock >= 0xeee9)//0x82ee//8331
				debug++;
		UINT64 rs2n = -(op_exec->rs2);

		UINT64 rs1_0 = op_exec->rs1 & 0x00000000ffffffff;
		UINT64 rs1_1 = (op_exec->rs1 >> 32) & 0x00000000ffffffff;
		UINT64 rs1_2 = op_exec->rs1_h & 0x00000000ffffffff;
		UINT64 rs1_3 = (op_exec->rs1_h >> 32) & 0x00000000ffffffff;

		UINT64 rs2_0 = op_exec->rs2 & 0x00000000ffffffff;
		UINT64 rs2_1 = (op_exec->rs2 >> 32) & 0x00000000ffffffff;
		UINT64 rs2_2 = op_exec->rs2_h & 0x00000000ffffffff;
		UINT64 rs2_3 = (op_exec->rs2_h >> 32) & 0x00000000ffffffff;

		UINT64 rd0, rd1, rd2, rd3;

		switch (op_exec->uop) {
		case uop_ADD:
		case uop_ADDI:
		case uop_LUI:
		case uop_AUIPC:
//		case uop_JALR:
			rd0 = rs1_0 + rs2_0;
			rd1 = rs1_1 + rs2_1 + ((rd0 & 0x0000000100000000) ? 1 : 0);
			rd2 = rs1_2 + rs2_2 + ((rd1 & 0x0000000100000000) ? 1 : 0);
			rd3 = rs1_3 + rs2_3 + ((rd2 & 0x0000000100000000) ? 1 : 0);
			rd0 = rd0 & 0x00000000ffffffff;
			rd1 = rd1 & 0x00000000ffffffff;
			rd2 = rd2 & 0x00000000ffffffff;
			rd3 = rd3 & 0x00000000ffffffff;
			rd->data = (rd1 << 32) | rd0;
			rd->data_H = (rd3 << 32) | rd2;
			break;
		case uop_SUB:
			rs2_0 = (~rs2_0) & 0x00000000ffffffff;
			rs2_1 = (~rs2_1) & 0x00000000ffffffff;
			rs2_2 = (~rs2_2) & 0x00000000ffffffff;
			rs2_3 = (~rs2_3) & 0x00000000ffffffff;

			rd0 = rs1_0 + rs2_0 + 1;
			rd1 = rs1_1 + rs2_1 + ((rd0 & 0x0000000100000000) ? 1 : 0);
			rd2 = rs1_2 + rs2_2 + ((rd1 & 0x0000000100000000) ? 1 : 0);
			rd3 = rs1_3 + rs2_3 + ((rd2 & 0x0000000100000000) ? 1 : 0);
			rd0 = rd0 & 0x00000000ffffffff;
			rd1 = rd1 & 0x00000000ffffffff;
			rd2 = rd2 & 0x00000000ffffffff;
			rd3 = rd3 & 0x00000000ffffffff;
			rd->data = (rd1 << 32) | rd0;
			rd->data_H = (rd3 << 32) | rd2;
			break;
		case uop_SLL: {
			if (op_exec->rs2 > 128 || op_exec->rs2 < -128 || op_exec->rs2_h != 0) {
				debug++;
			}
			else if (op_exec->rs2 == 0) {
				rd->data_H = op_exec->rs1_h;
				rd->data = op_exec->rs1;
			}
			else if (op_exec->rs2 > 0) {
				rd->data_H = (op_exec->rs1_h << op_exec->rs2);
				rd->data = op_exec->rs1 << op_exec->rs2;
				rd->data_H |= (op_exec->rs1 >> (64 - op_exec->rs2) | (~(((UINT64)-1) << op_exec->rs2)));
			}
			else {
				rd->data_H = (op_exec->rs1_h << op_exec->rs2);
				rd->data = op_exec->rs1 << op_exec->rs2;
				rd->data_H |= (op_exec->rs1 >> (64 - op_exec->rs2) | (~(((UINT64)-1) << op_exec->rs2)));
			}
		}
					break;
		case uop_SLT:
			rd->data = (op_exec->rs1 < op_exec->rs2) ? 1 : 0;
			break;
		case uop_SLTU:
			rd->data = (((UINT64)op_exec->rs1) < ((UINT64)op_exec->rs2)) ? 1 : 0;
			break;
		case uop_XOR:
		case uop_XORI: //XORI
			rd->data = op_exec->rs1 ^ op_exec->rs2;
			break;
		case uop_SRLI:
		case uop_SRL: {
			INT64 mask = -1;
			mask = mask << (64 - (op_exec->rs2));
			mask = ~mask;
			if (op_exec->rs2 == 0)
				mask = -1;
			rd->data = (op_exec->rs1 >> (op_exec->rs2)) & mask;
		}
					break;
		case uop_OR:
		case uop_ORI: //ORI
			rd->data = op_exec->rs1 | op_exec->rs2;
			break;
		case uop_or_add:
			INT64 temp;
			temp = op_exec->rs1 | op_exec->rs2;
			rd->data = temp + op_exec->rs2;
			break;
		case uop_AND:
		case uop_ANDI: //ANDI
			rd->data = op_exec->rs1 & op_exec->rs2;
			break;
		case uop_NOP:
			break;
		case uop_SLLI: // SLLI - shift logical left
			if (op_exec->rs2 > 128 || op_exec->rs2 < -128) {
				debug++;
			}
			else if (op_exec->rs2 == 0) {
				rd->data_H = op_exec->rs1_h;
				rd->data = op_exec->rs1;
			}
			else if (op_exec->rs2 > 0) {
				rd->data_H = (op_exec->rs1_h << op_exec->rs2);
				rd->data = op_exec->rs1 << op_exec->rs2;
				rd->data_H |= (op_exec->rs1 >> (64 - op_exec->rs2) & (~(((UINT64)-1) << op_exec->rs2)));
			}
			else {
				rd->data_H = (op_exec->rs1_h >> op_exec->rs2);
				rd->data = op_exec->rs1 >> op_exec->rs2;
				rd->data |= (op_exec->rs1_h << (64 - op_exec->rs2) & ((((UINT64)-1) << op_exec->rs2)));
			}
			break;
		case uop_SLTI:
			rd->data = (op_exec->rs1 < (op_exec->rs2 & 0x3f)) ? 1 : 0;
			break;
		case uop_SLTIU:
			rd->data = (((UINT64)op_exec->rs1) < (op_exec->rs2 & 0x3f)) ? 1 : 0;
			break;
		case uop_SRAI: {
			UINT64 mask = -1;
			mask <<= (0x40 - (op_exec->rs2 & 0x3f));
			rd->data = op_exec->rs1 >> (op_exec->rs2 & 0x3f);
			rd->data |= mask;
		}
					 break;
		default:
			debug++;
			break;
		}
		if (param.intx && debug_core) {
			fprintf(debug_stream, "INT(%lld): ROB entry: %#04x ", mhartid, rd->ROB_id);
			switch (op_exec->uop) {
//			case uop_JALR:
//				fprintf(debug_stream, "JALR unit %d", q_num);
//				fprintf(debug_stream, " //	result = 0x%016I64x, rs1 = 0x%016I64x, clock 0x%04llx\n", rd->data, op_exec->rs1, clock);
//				break;
			case uop_LUI:
				fprintf(debug_stream, "LUI unit %d", q_num);
				fprintf(debug_stream, " //	result = 0x%016I64x, rs1 = 0x%016I64x, clock 0x%04llx\n", rd->data, op_exec->rs1, clock);
				break;
			case uop_AUIPC:
				fprintf(debug_stream, "AUIPC unit %d", q_num);
				fprintf(debug_stream, " //	result = 0x%016I64x, rs1 = 0x%016I64x, source 2 = 0x%016I64x, clock 0x%04llx\n",
					rd->data, op_exec->rs1, op_exec->rs2, clock);
				break;
			case uop_ADD:
			case uop_ADDI:
				fprintf(debug_stream, "ADD unit %d", q_num);
				fprintf(debug_stream, " //	result = 0x%016I64x, rs1 = 0x%016I64x, source 2 = 0x%016I64x, clock 0x%04llx\n",
					rd->data, op_exec->rs1, op_exec->rs2, clock);
				break;
			case uop_AND:
			case uop_ANDI:
				fprintf(debug_stream, "AND unit %d", q_num);
				fprintf(debug_stream, " //	result = 0x%016I64x, rs1 = 0x%016I64x, source 2 = 0x%016I64x, clock 0x%04llx\n",
					rd->data, op_exec->rs1, op_exec->rs2, clock);
				break;
			case uop_SUB:
				fprintf(debug_stream, "SUB unit %d", q_num);
				fprintf(debug_stream, " //	result = 0x%016I64x, rs1 = 0x%016I64x, source 2 = 0x%016I64x, clock 0x%04llx\n",
					rd->data, op_exec->rs1, op_exec->rs2, clock);
				break;
			case uop_OR:
				fprintf(debug_stream, "OR unit %d", q_num);
				fprintf(debug_stream, " //	result = 0x%016I64x, rs1 = 0x%016I64x, source 2 = 0x%016I64x, clock 0x%04llx\n",
					rd->data, op_exec->rs1, op_exec->rs2, clock);
			case uop_SRL:
			case uop_SRLI:
				fprintf(debug_stream, "SRL unit %d", q_num);
				fprintf(debug_stream, "// result = 0x%016I64x, rs1 = 0x%016I64x, source 2 = 0x%016I64x, clock 0x%04llx\n",
					rd->data, op_exec->rs1, op_exec->rs2, clock);
				break;
			case uop_XORI:
				fprintf(debug_stream, "XORI unit %d", q_num);
				fprintf(debug_stream, "// result = 0x%016I64x, rs1 = 0x%016I64x, imm = 0x%03x, clock 0x%04llx\n",
					rd->data, op_exec->rs1, op_exec->rs2, clock);
				break;
			case uop_SLLI:
				fprintf(debug_stream, "SLLI unit %d", q_num);
				fprintf(debug_stream, "// result = 0x%016I64x, rs1 = 0x%016I64x, imm = 0x%03x, clock 0x%04llx\n",
					rd->data, op_exec->rs1, op_exec->rs2, clock);
				break;
			case uop_SRAI:
				fprintf(debug_stream, "SRAI unit %d", q_num);
				fprintf(debug_stream, "// rd = 0x%016I64x, rs1 = 0x%016I64x, imm = 0x%03x, clock 0x%04llx\n",
					rd->data, op_exec->rs1, op_exec->rs2, clock);
				break;
			default:
				debug++;
				break;
			}
		}
		rd->strobe = 1;
		exec_rsp[0] = 1;
		stop++;
	}
}