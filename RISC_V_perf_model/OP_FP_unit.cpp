// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "ROB.h"

#include <intrin.h>
using namespace std;

#pragma intrinsic(__ll_lshift)

UINT64 fadd_unit2(UINT64 rs1, UINT64 rs2, UINT8 size) {
	int debug = 0;
	UINT64 rd = 0;
	switch (size) {
	case 0:
		debug++;
		break;
	case 1:
		debug++;
		break;
	case 2: {// half precesion
		UINT8 sign_a = (rs1 >> 15) & 1;
		UINT8 exp_a = (rs1 >> 10) & 0x1f;
		UINT16 man_a = (rs1 & 0x3ff) | 0x400;
		UINT8 sign_b = (rs2 >> 15) & 1;
		UINT8 exp_b = (rs2 >> 10) & 0x1f;
		UINT16 man_b = (rs2 & 0x3ff) | 0x400;
		if (exp_a >= exp_b) {
			man_b >>= (exp_a - exp_b);
			if (sign_a ^ sign_b)
				man_a = man_a + (~man_b) + 1;
			else
				man_a = man_a + man_b + 0;
			sign_a = (sign_a ^ ((man_a >> 15) & 1)) & 1;
		}
		else {
			man_a >>= (exp_b - exp_a);
			if ((sign_a ^ sign_b) & 1)
				man_a = man_b + (~man_a) + 1;
			else
				man_a = man_b + man_a + 0;
			sign_a = (sign_b ^ ((man_a >> 15) & 1)) & 1;
			exp_a = exp_b;
		}
		rd = (sign_a << 15) | ((exp_a << 10) & 0x7c00) | (man_a & 0x3ff);
	}
		  break;
	case 3:
		debug++;
		break;
	default:
		debug++;
		break;
	}
	return rd;
}
UINT64 FMADD16_unit(UINT64 rs1, UINT64 rs2, UINT64 rs3, UINT8 size) {
	UINT16 sign = ((rs1 >> 15) ^ (rs2 >> 15)) & 1;
	UINT16 exp = (((rs1 >> 10) & 0x1f) + (rs2 >> 10) & 0x1f) - 0x0f;
	UINT16 exp3 = ((rs3 >> 10) & 0x1f);
	UINT16 sum0;
	UINT16 sum8_0 = (rs2 & 0x03ff) | 0x0400;
	UINT16 sum8_1 = ((rs1 & 0x200) ? sum8_0 >> 1 : 0) + ((rs1 & 0x100) ? sum8_0 >> 2 : 0);
	UINT16 sum8_2 = ((rs1 & 0x080) ? sum8_0 >> 3 : 0) + ((rs1 & 0x040) ? sum8_0 >> 4 : 0);
	UINT16 sum8_3 = ((rs1 & 0x020) ? sum8_0 >> 5 : 0) + ((rs1 & 0x010) ? sum8_0 >> 6 : 0);
	UINT16 sum8_4 = ((rs1 & 0x008) ? sum8_0 >> 7 : 0) + ((rs1 & 0x004) ? sum8_0 >> 8 : 0);
	UINT16 sum8_5 = ((rs1 & 0x002) ? sum8_0 >> 9 : 0) + ((rs1 & 0x001) ? sum8_0 >> 10 : 0);
	sum8_0 += (((rs3 & 0x03ff) | 0x0400) >> (exp - exp3));

	UINT16 sum4_0 = sum8_0 + sum8_1;
	UINT16 sum4_1 = sum8_2 + sum8_3;
	UINT16 sum4_2 = sum8_4 + sum8_5;

	UINT16 sum2_0 = sum4_0;
	UINT16 sum2_1 = sum4_1 + sum4_2;

	sum0 = sum2_0 + sum2_1;
	while (sum0 >= 0x800) {
		sum0 >>= 1;
		exp++;
	}
	return(sign << 15) | (exp << 10) | (sum0 & 0x03ff);
}
// need to normalize input before 


UINT64 fp_neg_value(UINT64 fp_in, UINT8 size) {
	int debug = 0;
	UINT64 fp_out;
	switch (size) {
	case 0:
		debug++;
		break;
	case 1:
		debug++;
		break;
	case 2:// half precision
		if (fp_in == 0)
			fp_out = 0;
		else {
			fp_out = (~fp_in) & 0x8000;
			fp_out |= fp_in & 0x7c00;
			fp_out |= ((~((fp_in & 0x3ff) | 0x400)) + 1) & 0x3fff;// 2's complement + 1
		}
		break;
	case 3:
		debug++;
		break;
	default:
		debug++;
		break;
	}
	return fp_out;
}
void FMUL64_adders(reg_bus* rd, UINT16* exec_rsp, R3_type* fmul_exec, fp_vars* vars, UINT64 clock, UINT8 q_num, UINT mhartid, UINT debug_core, param_type param, FILE* debug_stream) {
	int debug = 0;
	int stop = 0;
	exec_rsp[0] = 0;
	if (mhartid == 0) {
		if (clock >= 0x40e8)
			debug++;
	}
	UINT debug_unit = param.fp && debug_core;
	//	debug_unit = 0;

	vars->stage6 = vars->stage5[1] + vars->stage5[0];//64b
	for (UINT i = 0; i < 2; i++) vars->stage5[i] = vars->stage4[2 * i] + vars->stage4[2 * i + 1];// 32b
	for (UINT i = 0; i < 4; i++) vars->stage4[i] = vars->stage3[2 * i] + vars->stage3[2 * i + 1];// 16b
	for (UINT i = 0; i < 8; i++) vars->stage3[i] = vars->stage2[2 * i] + vars->stage2[2 * i + 1];
	for (UINT i = 0; i < 16; i++) vars->stage2[i] = vars->stage1[2 * i] + vars->stage1[2 * i + 1];
	for (UINT i = 0; i < 32; i++) vars->stage1[i] = vars->stage0[2 * i] + vars->stage0[2 * i + 1];
	for (UINT i = 0; i < 64; i++)vars->stage0[i] = 0;
	for (UINT i = 6; i > 0; i--) {
		vars->tracker[i].mode = vars->tracker[i - 1].mode;
		vars->tracker[i].sign = vars->tracker[i - 1].sign;
		vars->tracker[i].exp16[0] = vars->tracker[i - 1].exp16[0];
		vars->tracker[i].exp16[1] = vars->tracker[i - 1].exp16[1];
		vars->tracker[i].exp16[2] = vars->tracker[i - 1].exp16[2];
		vars->tracker[i].exp16[3] = vars->tracker[i - 1].exp16[3];
		vars->tracker[i].exp32[0] = vars->tracker[i - 1].exp32[0];
		vars->tracker[i].exp32[1] = vars->tracker[i - 1].exp32[1];
		vars->tracker[i].exp64 = vars->tracker[i - 1].exp64;
		vars->tracker[i].valid = vars->tracker[i - 1].valid;
		vars->tracker[i].ROB_ptr[0] = vars->tracker[i - 1].ROB_ptr[0];
		vars->tracker[i].ROB_ptr[1] = vars->tracker[i - 1].ROB_ptr[1];
		vars->tracker[i].ROB_ptr[2] = vars->tracker[i - 1].ROB_ptr[2];
		vars->tracker[i].ROB_ptr[3] = vars->tracker[i - 1].ROB_ptr[3];
		vars->tracker[i].uop[0] = vars->tracker[i - 1].uop[0];
		vars->tracker[i].uop[1] = vars->tracker[i - 1].uop[1];
		vars->tracker[i].uop[2] = vars->tracker[i - 1].uop[2];
		vars->tracker[i].uop[3] = vars->tracker[i - 1].uop[3];
	}

	// stage 0 -- onset

	UINT count = 0;
	vars->tracker[0].mode = 0;
	vars->tracker[0].sign = 0;
	vars->tracker[0].valid = 0;

	if (mhartid == 0) {
		if (clock >= 0x27da)
			debug++;
	}
	rd->strobe = 0;
	if (fmul_exec->strobe) {
		switch (fmul_exec->size) {
		case 0://sp
			debug++;
			if (debug_unit) {
				switch (fmul_exec->uop) {
				case uop_FMUL:
					fprintf(debug_stream, "FMADD start(%d): ROB entry: 0x%02x ERROR: not implemented yet, FMUL.s unit %d //	clock 0x%04llx\n", mhartid, fmul_exec->ROB_id, q_num, clock);
					break;
				case uop_FMADD:
					fprintf(debug_stream, "FMADD start(%d): ROB entry: 0x%02x ERROR: not implemented yet, FMADD.s unit %d //	clock 0x%04llx\n", mhartid, fmul_exec->ROB_id, q_num, clock);
					break;
				case uop_FMSUB:
					fprintf(debug_stream, "FMADD start(%d): ROB entry: 0x%02x ERROR: not implemented yet, FSUB.s unit %d //	clock 0x%04llx\n", mhartid, fmul_exec->ROB_id, q_num, clock);
					break;
				case uop_FNMADD:
					fprintf(debug_stream, "FMADD start(%d): ROB entry: 0x%02x ERROR: not implemented yet, FNMADD.s unit %d //	clock 0x%04llx\n", mhartid, fmul_exec->ROB_id, q_num, clock);
					break;
				case uop_FNMSUB:
					fprintf(debug_stream, "FMADD start(%d): ROB entry: 0x%02x ERROR: not implemented yet, FNMSUB.s unit %d //	clock 0x%04llx\n", mhartid, fmul_exec->ROB_id, q_num, clock);
					break;
				default:
					debug++;
					break;
				}
			}
			break;
		case 1://dp
			debug++;
			break;
		case 2: {// half precision - do not use 64b integers - 4x faster adds, 1 clk per fp16 with transistor sizing (not minimum std cell)
			vars->stage0[0] = (fmul_exec->rs1 & 0x03ff) | 0x400;
			vars->stage0[1] = (fmul_exec->rs2 & 0x0200) ? ((fmul_exec->rs1 & 0x03ff) | 0x400) >> 1 : 0;
			vars->stage0[2] = (fmul_exec->rs2 & 0x0100) ? ((fmul_exec->rs1 & 0x03ff) | 0x400) >> 2 : 0;
			vars->stage0[3] = (fmul_exec->rs2 & 0x0080) ? ((fmul_exec->rs1 & 0x03ff) | 0x400) >> 3 : 0;
			vars->stage0[4] = (fmul_exec->rs2 & 0x0040) ? ((fmul_exec->rs1 & 0x03ff) | 0x400) >> 4 : 0;
			vars->stage0[5] = (fmul_exec->rs2 & 0x0020) ? ((fmul_exec->rs1 & 0x03ff) | 0x400) >> 5 : 0;
			vars->stage0[6] = (fmul_exec->rs2 & 0x0010) ? ((fmul_exec->rs1 & 0x03ff) | 0x400) >> 6 : 0;
			vars->stage0[7] = (fmul_exec->rs2 & 0x0008) ? ((fmul_exec->rs1 & 0x03ff) | 0x400) >> 7 : 0;
			vars->stage0[8] = (fmul_exec->rs2 & 0x0004) ? ((fmul_exec->rs1 & 0x03ff) | 0x400) >> 8 : 0;
			vars->stage0[9] = (fmul_exec->rs2 & 0x0002) ? ((fmul_exec->rs1 & 0x03ff) | 0x400) >> 9 : 0;
			vars->stage0[10] = (fmul_exec->rs2 & 0x0001) ? 1 : 0;
			vars->tracker[0].mode = 2;
			vars->tracker[0].sign = (fmul_exec->rs1 >> 15) ^ (fmul_exec->rs2 >> 15);
			vars->tracker[0].exp64 = (fmul_exec->rs1 >> 10) & 0x1f + (fmul_exec->rs2 >> 10) & 0x1f;
			vars->tracker[0].valid = 1;
			vars->tracker[0].ROB_ptr[0] = fmul_exec->ROB_id;
			vars->tracker[0].uop[0] = fmul_exec->uop;
			if (debug_unit) {
				switch (fmul_exec->uop) {
				case uop_FMUL:
					fprintf(debug_stream, "FMADD start(%d): ROB entry: 0x%02x FMUL.h unit %d //	clock 0x%04llx\n", mhartid, fmul_exec->ROB_id, q_num, clock);
					break;
				case uop_FMADD:
					fprintf(debug_stream, "FMADD start(%d): ROB entry: 0x%02x FMADD.h unit %d //	clock 0x%04llx\n", mhartid, fmul_exec->ROB_id, q_num, clock);
					break;
				case uop_FMSUB:
					fprintf(debug_stream, "FMADD start(%d): ROB entry: 0x%02x FSUB.h unit %d //	clock 0x%04llx\n", mhartid, fmul_exec->ROB_id, q_num, clock);
					break;
				case uop_FNMADD:
					fprintf(debug_stream, "FMADD start(%d): ROB entry: 0x%02x FNMADD.h unit %d //	clock 0x%04llx\n", mhartid, fmul_exec->ROB_id, q_num, clock);
					break;
				case uop_FNMSUB:
					fprintf(debug_stream, "FMADD start(%d): ROB entry: 0x%02x FNMSUB.h unit %d //	clock 0x%04llx\n", mhartid, fmul_exec->ROB_id, q_num, clock);
					break;
				default:
					debug++;
					break;
				}
			}
			exec_rsp[0] = 0x8000 | fmul_exec->ROB_id;
			stop++;
		}
			  break;
		case 3:
			debug++;
			break;
		default:
			debug++;
			break;
		}
	}
	//	}
		// stage 0 -- onset
		// note: next level, each adder is split as well, to increase output of fp16
		// complete computation - dequeue
	if (vars->tracker[4].valid == 1) {
		if (vars->tracker[4].mode == 2) {// half precision
			while (vars->stage4[0] >= (1 << 12)) {
				vars->stage4[0] >> 1;
				vars->tracker[4].exp16[0]++;
			}
			while (vars->stage4[1] >= (1 << 12)) {
				vars->stage4[1] >> 1;
				vars->tracker[4].exp16[1]++;
			}
			while (vars->stage4[2] >= (1 << 12)) {
				vars->stage4[2] >> 1;
				vars->tracker[4].exp16[2]++;
			}
			while (vars->stage4[3] >= (1 << 12)) {
				vars->stage4[3] >> 1;
				vars->tracker[4].exp16[3]++;
			}
			rd->ROB_id = vars->tracker[4].ROB_ptr[0];
			UINT64 rtemp;
			switch (vars->tracker[4].uop[0]) {
			case uop_FMUL:
				rd->data = (((vars->tracker[4].sign) & 1) << 15) | (vars->tracker[4].exp16[0] << 10) | (vars->stage4[0] & 0x3ff);
				rd->strobe = 1;
				if (debug_unit) {
					fprintf(debug_stream, "FMADD stop(%d): ROB entry: 0x%02x FMUL.h unit %d //	clock 0x%04llx\n", mhartid, vars->tracker[4].ROB_ptr[0], q_num, clock);
				}
				break;
			case uop_FMADD:// need clock delay
				rtemp = (((vars->tracker[4].sign) & 1) << 15) | (vars->tracker[4].exp16[0] << 10) | (vars->stage4[0] & 0x3ff);
				rd->data = fadd_unit2(rtemp, fmul_exec->rs3, 2);
				if (debug_unit) {
					fprintf(debug_stream, "FMADD stop(%d): ROB entry: 0x%02x FMADD.h unit %d //	clock 0x%04llx\n", mhartid, vars->tracker[4].ROB_ptr[0], q_num, clock);
				}
				rd->strobe = 2;// update start_ptr on retire
				stop++;
				break;
			case uop_FNMSUB:// need clock delay
				rtemp = (((vars->tracker[4].sign) & 1) << 15) | (vars->tracker[4].exp16[0] << 10) | (vars->stage4[0] & 0x3ff);
				rtemp = fp_neg_value(rtemp, 2);
				rd->data = fadd_unit2(rtemp, fmul_exec->rs3, 2);
				if (debug_unit) {
					fprintf(debug_stream, "FMADD stop(%d): ROB entry: 0x%02x FNMSUB.h unit %d //	clock 0x%04llx\n", mhartid, vars->tracker[4].ROB_ptr[0], q_num, clock);
				}
				rd->strobe = 2;// update start_ptr on retire
				stop++;
				break;
			default:
				fprintf(debug_stream, "FMADD stop(%d): ROB entry: 0x%02x  unit %d ", mhartid, vars->tracker[4].ROB_ptr[0], q_num);
				fprintf(debug_stream, "ERROR not FMUL or FMAD style instruction //	clock 0x%04llx\n", clock);
				debug++;
				break;
			}
		}
	}
}
//void OP_FP_unit(reg_bus* rd, UINT8* exec_rsp, UINT16 fp_exec, reg_bus rs1, reg_bus rs2, ROB_Type* ROB, UINT64 clock, UINT mhartid, param_type param, FILE* debug_stream, char q_num) {
void OP_FP_unit(reg_bus* rd, UINT16* exec_rsp, R_type* fp_exec, UINT64 clock, char q_num, UINT mhartid, UINT debug_core, param_type param, FILE* debug_stream) {
	int debug = 0;
	int stop = 0;
	if (mhartid == 0 && q_num == 7) {
		if (clock >= 0x26fe)
			debug++;
	}
	UINT debug_unit = param.fp && debug_core;

	rd->strobe = 0;
	exec_rsp[0] = 0;
	if (fp_exec->strobe) {
		rd->ROB_id = fp_exec->ROB_id;
		UINT64 rs2a = 0;
		if (q_num >= param.decode_width)
			debug++;
		switch (fp_exec->uop) {
		case uop_FADD:
			rd->data = fadd_unit2(fp_exec->rs1, fp_exec->rs2, fp_exec->size);
			if (debug_unit) {
				fprintf(debug_stream, "OP_FP(%d): ROB entry: 0x%02x FADD", mhartid, fp_exec->ROB_id);
				fprintf(debug_stream, " // unit %d,	clock 0x%04llx\n", q_num, clock);
			}
			rd->strobe = 1;// update start_ptr on retire
			exec_rsp[0] = 1;
			stop++;
			break;
		case uop_FSUB:
			rs2a = fp_neg_value(fp_exec->rs2, fp_exec->size);
			rd->data = fadd_unit2(fp_exec->rs1, rs2a, fp_exec->size);
			if (debug_unit) {
				fprintf(debug_stream, "OP_FP(%d): ROB entry: 0x%02x FSUB", mhartid, fp_exec->ROB_id);
				fprintf(debug_stream, " // unit %d,	clock 0x%04llx\n", q_num, clock);
			}
			rd->strobe = 1;// update start_ptr on retire
			exec_rsp[0] = 1;
			stop++;
			break;
			// need seperate queue to be able to load 4 fp16 instr at a time

		case uop_FMUL: // FMUL
			debug++;
			break;
		case uop_FDIV:
			debug++;
			break;
		case uop_FSGN:
			debug++;
			break;
		case uop_FMIN:
			switch (fp_exec->size) {
			case 0:
				debug++;
				break;
			case 1:
				debug++;
				break;
			case 2: {// half precision
				if (fp_exec->rs2 == 0)
					rs2a = 0;
				else {
					rs2a = (~fp_exec->rs2) & 0x8000;
					rs2a |= fp_exec->rs2 & 0x7c00;
					rs2a |= ((~((fp_exec->rs2 & 0x3ff) | 0x400)) + 1) & 0x3fff;// 2's complement + 1
				}
				UINT result = fadd_unit2(fp_exec->rs1, rs2a, fp_exec->size);
				if (result & 0x8000)
					rd->data = fp_exec->rs1;
				else
					rd->data = fp_exec->rs2;
			}
				  break;
			case 3:
				debug++;
				break;
			default:
				debug++;
				break;
			}
			if (debug_unit) {
				fprintf(debug_stream, "OP_FP(%d): ROB entry: 0x%02x FMIN", mhartid, fp_exec->ROB_id);
				fprintf(debug_stream, " // unit %d,	clock 0x%04llx\n", q_num, clock);
			}
			rd->strobe = 1;// update start_ptr on retire
			exec_rsp[0] = 1;
			stop++;
			break;
		case uop_FMAX:
			switch (fp_exec->size) {
			case 0:
				debug++;
				break;
			case 1:
				debug++;
				break;
			case 2: {// half precision
				if (fp_exec->rs2 == 0)
					rs2a = 0;
				else {
					rs2a = (~fp_exec->rs2) & 0x8000;
					rs2a |= fp_exec->rs2 & 0x7c00;
					rs2a |= ((~((fp_exec->rs2 & 0x3ff) | 0x400)) + 1) & 0x3fff;// 2's complement + 1
				}
				UINT result = fadd_unit2(fp_exec->rs1, rs2a, fp_exec->size);
				if (result & 0x8000)
					rd->data = fp_exec->rs2;
				else
					rd->data = fp_exec->rs1;
			}
				  break;
			case 3:
				debug++;
				break;
			default:
				debug++;
				break;
			}
			if (debug_unit) {
				fprintf(debug_stream, "OP_FP(%d): ROB entry: 0x%02x FMAX", mhartid, fp_exec->ROB_id);
				fprintf(debug_stream, " // unit %d,	clock 0x%04llx\n", q_num, clock);
			}
			rd->strobe = 1;// update start_ptr on retire
			exec_rsp[0] = 1;
			stop++;
			break;
		case uop_FSQRT: // FSQRT
			//			if (ROB_ptr->rs2 != 0)
			debug++; // error
			break;
		case uop_FEQ:// F compare
			rd->data = (fp_exec->rs2 = fp_exec->rs1) ? 1 : 0;
			break;
		case uop_FLE:// F compare
			rd->data = (fp_exec->rs2 <= fp_exec->rs1) ? 1 : 0;
			break;
		case uop_FLT:// F compare
			rd->data = (fp_exec->rs2 < fp_exec->rs1) ? 1 : 0;
			break;
		case uop_FCVTi2f: {// fmv
			switch (fp_exec->size) {
			case 0: {
				debug++;
			}
				  break;
			case 1: {
				debug++;
			}
				  break;
			case 2:
				if (fp_exec->rs1 > 0x10000)
					rd->data = 0x7c00;
				else if (fp_exec->rs1 < (-0x10000))
					rd->data = 0xfc00;
				else if (fp_exec->rs1 == 0)
					rd->data = 0;
				else {
					UINT16 exp = 0x0f;
					short temp = fp_exec->rs1;
					UINT man = fp_exec->rs1 << 10;
					while (temp > 1) {
						temp >>= 1;
						exp++;
						man >>= 1;
					}
					rd->data = (temp & 0x8000) | (exp << 10) | (man & 0x3ff);
				}
				break;
			case 3:
				debug++; // not yet implemented, must emulate with int 
				break;
			default:
				debug++;// syntax
				break;
			}
			if (debug_unit) {
				fprintf(debug_stream, "OP_FP(%d): ROB entry: 0x%02x FCVTi2f", mhartid, fp_exec->ROB_id);
				fprintf(debug_stream, " // unit %d,	clock 0x%04llx\n", q_num, clock);
			}
			rd->strobe = 1;// update start_ptr on retire
			exec_rsp[0] = 1;
			stop++;
		}
						break;
		case uop_FCVTf2i: // fmv
			switch (fp_exec->size) {
			case 0: {
				debug++;
			}
				  break;
			case 1: {
				debug++;
			}
				  break;
			case 2:
				if (fp_exec->rs1 == 0)
					rd->data = 0;
				else {
					rd->data = (0x4000 | (fp_exec->rs1 & 0x3ff)) << (((fp_exec->rs1 >> 10) & 0x1f) - 0x19);
				}
				break;
			case 3:
				debug++; // not yet implemented, must emulate with int 
				break;
			default:
				debug++;// syntax
				break;
			}
			if (debug_unit) {
				fprintf(debug_stream, "OP_FP(%d): ROB entry: 0x%02x FCVTf2i", mhartid, fp_exec->ROB_id);
				fprintf(debug_stream, " // unit %d,	clock 0x%04llx\n", q_num, clock);
			}
			rd->strobe = 1;// update start_ptr on retire
			exec_rsp[0] = 1;
			stop++;
			break;
		case uop_FMVi2f: // fmv

			switch (fp_exec->size) {
			case 0: {
				debug++;
			}
				  break;
			case 1: {
				debug++;
			}
				  break;
			case 2:
				rd->data = fp_exec->rs1 & 0x00ffff;
				break;
			case 3:
				debug++; // not yet implemented, must emulate with int 
				break;
			default:
				debug++;// syntax
				break;
			}
			if (debug_unit) {
				fprintf(debug_stream, "OP_FP(%d): ROB entry: 0x%02x FMVi2f", mhartid, fp_exec->ROB_id);
				fprintf(debug_stream, " // unit %d,	clock 0x%04llx\n", q_num, clock);
			}
			rd->strobe = 1;// update start_ptr on retire
			exec_rsp[0] = 1;
			stop++;
			break;
		case uop_FMVf2i: {// fmv
			void* ptr = &rd->data;
			switch (fp_exec->size) {
			case 0:
				ptr = &rd->data;
				break;
			case 1:
				ptr = &rd->data;
				break;
			case 3:
				debug++;// not implemented yet
				break;
			default:
				debug++;
				break;
			}
//			INT64* int_ptr = (INT64*)ptr;
//			rd->data = int_ptr[0];
			rd->data = ((UINT64*)ptr)[0];
			if (debug_unit) {
				fprintf(debug_stream, "OP_FP(%d): ROB entry: 0x%02x FMVf2i", mhartid, fp_exec->ROB_id);
				fprintf(debug_stream, " // unit %d,	clock 0x%04llx\n", q_num, clock);
			}
			rd->strobe = 1;
			exec_rsp[0] = 1;
			stop++;
		}
					   break;
		case 0x71: // FCLASS - always unsigned response
			debug++;
			// rs1 classification, rd unsigned integer
			// 0: -infinty
			// 1: neg normal
			// 2: neg subnormal
			// 3: -0
			// 4: +0
			// 5: pos subnormal
			// 6: pos normal
			// 7: +infinity
			// 8: signaling NaN
			// 9: quiet NaN

			// UINT8 signed = ! (rs2&1);
			// UINT8 long_word = ((rs2>>1)&1); // 1-64b, 0 32b integer

			break;
		default: // encoding error, generate fault
			debug++;
			break;
		}
		//		}
	}
}
void fdiv_unit(Q_type* fdiv_q, ROB_Type* ROB, Reg_Table_Type* Reg, UINT8* delay) {
	if (delay[0] != 0)
		delay[0]--;
	else {
		int stop = 0;
		for (UINT16 k = fdiv_q->start_ptr; k != fdiv_q->end_ptr && !stop; k = ((k + 1) & (fdiv_q->count - 1))) { // no pipelining
			UINT j = fdiv_q->ROB_ptr[k];
			if (ROB->q[j].state == ROB_execute) {
				ROB->q[j].state = ROB_inflight;
				delay[0] = 14;
				stop = 1;
			}
			else if (ROB->q[j].state == ROB_inflight) {
				if (ROB->q[j].uop == 0x0d) {// fdiv
					Reg->f_reg[fdiv_q->ROB_ptr[k]].data[ROB->q[j].rd_ptr] = Reg->f_reg[fdiv_q->ROB_ptr[k]].data[ROB->q[j].rs1_ptr] / Reg->f_reg[fdiv_q->ROB_ptr[k]].data[ROB->q[j].rs2_ptr]; // need to check size
					Reg->f_reg[fdiv_q->ROB_ptr[k]].valid[ROB->q[j].rd_ptr] = reg_valid_out;
				}
				//				ROB->q[j].state = ROB_retire_out;
				stop = 1;
			}
		}
	}
}
