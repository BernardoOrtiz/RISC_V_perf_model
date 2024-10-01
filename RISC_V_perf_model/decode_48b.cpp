// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "front_end.h"
void incr_decode48_ptr(ROB_Type* ROB, decode_shifter_struct* shifter, decode_type* decode_vars, csr_type* csr, UINT8 priviledge) {
	UINT debug = 0;
	ROB->q[ROB->decode_ptr].state = ROB_allocate_0;
	ROB->decode_ptr = ((ROB->decode_ptr + 1) & 0xff);

	csr[csr_hpmcounter3].value++;
	switch (priviledge) {
	case 0:
		break;
	case 1:
		csr[csr_shpmcounter3].value++;
		break;
	case 2:
		csr[csr_hhpmcounter3].value++;
		break;
	case 3:
		csr[csr_mhpmcounter3].value++;
		break;
	default:
		debug++;
		break;
	}
	shifter->index += 3;
	decode_vars->index++;
}
UINT8 decode_64b(ROB_Type* ROB, UINT8* flush_write_posters, decode_shifter_struct* shifter, decode_type* decode_vars, csr_type* csr, UINT8 prefetcher_idle, UINT stores_pending, UINT load_pending,UINT8 priviledge,
	UINT64 clock, branch_vars* branch, store_buffer_type* store_buffer, Reg_Table_Type* reg_table, UINT* uPC, UINT mhartid, UINT debug_core, param_type* param, FILE* debug_stream) {
	int debug = 0;
	UINT8 stop = 0;

	UINT debug_unit = (param->decoder == 1) && debug_core;
	UINT debug_branch = (param->decoder || param->branch) && debug_core;
	UINT debug_page_walk_unit = (param->decoder == 1 || param->PAGE_WALK == 1 || param->PAGE_FAULT == 1) && debug_core;

	INT buffer = (shifter->buffer[shifter->index + 2] << 32) | (shifter->buffer[shifter->index + 1] << 16) | shifter->buffer[shifter->index + 0];
	ROB_entry_Type* ROB_ptr = &ROB->q[ROB->decode_ptr];

	ROB_ptr->rd = (buffer >> 11) & 0x1f;
	UINT8 vtype = (buffer >> 16) & 0x0f;// reg size
	ROB_ptr->rs1 = (buffer >> 21) & 0x1f;
	UINT vgran = (buffer >> 26) & 0x0f;// reg size
	ROB_ptr->rs2 = (buffer >> 30) & 0x1f;
	ROB_ptr->rs3 = (buffer >> 35) & 0x1f;

	ROB_ptr->map = (opcode_map)((buffer >> 6) & 0x1f);
	switch (ROB_ptr->map) {
	case LOAD_map: {// load
		csr[csr_hpmcounter9].value++;//load counter
		switch (priviledge) {
		case 0:
			break;
		case 1:
			csr[csr_shpmcounter9].value++;
			break;
		case 2:
			csr[csr_hhpmcounter9].value++;
			break;
		case 3:
			csr[csr_mhpmcounter9].value++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->q_select = load_q_select;
		ROB_ptr->uop = uop_LOAD;
//		ROB_ptr->reg_type = (ROB_ptr->funct3 >> 2) & 1;
		ROB_ptr->imm = (buffer >> 26)&0x01fffff;
		if (debug_unit) {
			fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
			switch (vtype) {// bit 0x10 saturation math vs. overflow/underflow exceptions
			case 0x00:
				fprintf(debug_stream, "LB x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x01:
				fprintf(debug_stream, "LH x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x02:
				fprintf(debug_stream, "LW x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x03:
				fprintf(debug_stream, "LD x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x04:
				fprintf(debug_stream, "LQ x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x05:
				fprintf(debug_stream, "LQ2 x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x06:
				fprintf(debug_stream, "LQ4 x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x07:
				fprintf(debug_stream, "LQ8 x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x08:
				fprintf(debug_stream, "LBU x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x09:
				fprintf(debug_stream, "LHU x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x0a:
				fprintf(debug_stream, "LWU x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x0b:
				fprintf(debug_stream, "LDU x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x0c:
				fprintf(debug_stream, "LQU x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			case 0x0d:
				fprintf(debug_stream, "LQ2U x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			case 0x0e:
				fprintf(debug_stream, "LQ4U x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			case 0x0f:
				fprintf(debug_stream, "LQ8U x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x10:
				fprintf(debug_stream, "LBS x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x11:
				fprintf(debug_stream, "LHS x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x12:
				fprintf(debug_stream, "LWS x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x13:
				fprintf(debug_stream, "LDS x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x14:
				fprintf(debug_stream, "LQS x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x15:
				fprintf(debug_stream, "LQ2S x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x16:
				fprintf(debug_stream, "LQ4S x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x17:
				fprintf(debug_stream, "LQ8S x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x18:
				fprintf(debug_stream, "LBUS x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x19:
				fprintf(debug_stream, "LHUS x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x1a:
				fprintf(debug_stream, "LWUS x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x1b:
				fprintf(debug_stream, "LDUS x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
				break;
			case 0x1c:
				fprintf(debug_stream, "LQUS x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			case 0x1d:
				fprintf(debug_stream, "LQ2US x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			case 0x1e:
				fprintf(debug_stream, "LQ4US x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			case 0x1f:
				fprintf(debug_stream, "LQ8US x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;			
			default:
				debug++;
				break;
			}
			if (ROB_ptr->rs1 == 2) {
				fprintf(debug_stream, "0x%06x(sp) ", ROB_ptr->imm);
			}
			else if (ROB_ptr->rs1 == 3) {
				fprintf(debug_stream, "0x%06x(gp) ", ROB_ptr->imm);
			}
			else if (ROB_ptr->rs1 == 4) {
				fprintf(debug_stream, "0x%06x(tp) ", ROB_ptr->imm);
			}
			else {
				fprintf(debug_stream, "0x%06x(x%02d) ", ROB_ptr->imm, ROB_ptr->rs1);
			}
			fprintf(debug_stream, "0x%08x, clock: 0x%04llx\n", buffer, clock);
		}
		incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
	}
		break;
	case LOAD_FP_map: {
		fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
		switch (vtype) {// bit 0x10 saturation math vs. overflow/underflow exceptions
		case 0x01:
			fprintf(debug_stream, "FLH x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;
		case 0x02:
			fprintf(debug_stream, "FLW x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;
		case 0x03:
			fprintf(debug_stream, "FLD x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;
		case 0x04:
			fprintf(debug_stream, "FLQ x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;
		case 0x05:
			fprintf(debug_stream, "FLQ2 x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;
		case 0x06:
			fprintf(debug_stream, "FLQ4 x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;
		case 0x07:
			fprintf(debug_stream, "FLQ8 x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;
		case 0x11:
			fprintf(debug_stream, "VLH x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;
		case 0x12:
			fprintf(debug_stream, "VLW x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;
		case 0x13:
			fprintf(debug_stream, "VLD x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;
		case 0x14:
			fprintf(debug_stream, "VLQ x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;
		case 0x15:
			fprintf(debug_stream, "VLQ2 x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;
		case 0x16:
			fprintf(debug_stream, "VLQ4 x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;
		case 0x17:
			fprintf(debug_stream, "VLQ8 x%02d,  ", ROB_ptr->rd, ROB_ptr->rd_ptr);
			break;
			break;
		default:
			debug++;
			break;
		}
		if (ROB_ptr->rs1 == 2) {
			fprintf(debug_stream, "0x%06x(sp) ", ROB_ptr->imm);
		}
		else if (ROB_ptr->rs1 == 3) {
			fprintf(debug_stream, "0x%06x(gp) ", ROB_ptr->imm);
		}
		else if (ROB_ptr->rs1 == 4) {
			fprintf(debug_stream, "0x%06x(tp) ", ROB_ptr->imm);
		}
		else {
			fprintf(debug_stream, "0x%06x(x%02d) ", ROB_ptr->imm, ROB_ptr->rs1);
		}
		fprintf(debug_stream, "0x%08x, clock: 0x%04llx\n", buffer, clock);
		incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
	}
		break;
	case OP_IMM_map: {//32B - change to 128b???
		ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
		decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));

		ROB_ptr->rs_count = 1;
		ROB_ptr->imm = (buffer >> 31);//signed 

		ROB_ptr->reg_type = 0;
		switch (vtype&0x07) {
		case 0: //ADDI
			ROB_ptr->uop = uop_ADDI;
			if (ROB_ptr->rd == 0 && ROB_ptr->rs1 == 0 && ROB_ptr->imm == 0)
				ROB_ptr->uop = uop_NOP;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "ADDI(%d) ", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x%08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		case 1: // SLLI - shift logical left
			ROB_ptr->uop = uop_SLLI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "SLLI(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		case 2: { //SLTI signed compare less than
			ROB_ptr->uop = uop_SLTI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "SLTI(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		}
		case 3: { //SLTIU
			ROB_ptr->imm = (buffer >> 20) & 0x0fff;//unsigned 
			ROB_ptr->uop = uop_SLTIU;
			ROB_ptr->reg_type = 1;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "SLTIU(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		}
		case 4: { //XORI
			ROB_ptr->uop = uop_XORI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "XORI(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		}
		case 5: // SRLI/SRAI
			ROB_ptr->rs2 = ((buffer >> 20) & 0x1f); // shamt - shift amount

			ROB_ptr->uop = ((buffer >> 27) & 8) ? uop_SRAI : uop_SRLI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "SRLI(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		case 6: //ORI
			ROB_ptr->uop = uop_ORI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "ORI(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		case 7: //ANDI
			ROB_ptr->uop = uop_ANDI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "ANDI(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		default: // error
			debug++;
			break;
		}

	}
		break;
	case AUIPC_map:
		break;
	case OP_IMM64_map: {//32B - change to 128b???
		ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
		decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));

		ROB_ptr->rs_count = 1;
		ROB_ptr->imm = (buffer >> 31);//signed 

		ROB_ptr->reg_type = 0;
		switch (vtype & 0x07) {
		case 0: //ADDI
			ROB_ptr->uop = uop_ADDI;
			if (ROB_ptr->rd == 0 && ROB_ptr->rs1 == 0 && ROB_ptr->imm == 0)
				ROB_ptr->uop = uop_NOP;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "ADDI_64(%d) ", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x%08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		case 1: // SLLI - shift logical left
			ROB_ptr->uop = uop_SLLI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "SLLI_64(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		case 2: { //SLTI signed compare less than
			ROB_ptr->uop = uop_SLTI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "SLTI_64(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		}
		case 3: { //SLTIU
			ROB_ptr->imm = (buffer >> 20) & 0x0fff;//unsigned 
			ROB_ptr->uop = uop_SLTIU;
			ROB_ptr->reg_type = 1;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "SLTIU_64(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		}
		case 4: { //XORI
			ROB_ptr->uop = uop_XORI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "XORI_64(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		}
		case 5: // SRLI/SRAI
			ROB_ptr->rs2 = ((buffer >> 20) & 0x1f); // shamt - shift amount

			ROB_ptr->uop = ((buffer >> 27) & 8) ? uop_SRAI : uop_SRLI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "SRLI_64(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		case 6: //ORI
			ROB_ptr->uop = uop_ORI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "ORI_64(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		case 7: //ANDI
			ROB_ptr->uop = uop_ANDI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "ANDI_64(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
			break;
		default: // error
			debug++;
			break;
		}

	}
		break;
	case STORE_map: {// store
		csr[csr_hpmcounter11].value++;//load counter
		switch (priviledge) {
		case 0:
			break;
		case 1:
			csr[csr_shpmcounter11].value++;
			break;
		case 2:
			csr[csr_hhpmcounter11].value++;
			break;
		case 3:
			csr[csr_mhpmcounter11].value++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->q_select = store_q_select;
		ROB_ptr->uop = uop_STORE;
		ROB_ptr->imm = (((buffer) >> 25) & 0xffd00) | (((buffer) >> 20) & 0x3e0) | ((buffer >> 11) & 0x1f);
		ROB_ptr->rd = 0;
		ROB_ptr->rd_ptr = 0;

		if (debug_unit) {
			fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
			switch (vtype) {// bit 0x10 saturation math vs. overflow/underflow exceptions
			case 0x00:
				fprintf(debug_stream, "SB x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x01:
				fprintf(debug_stream, "SH x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x02:
				fprintf(debug_stream, "SW x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x03:
				fprintf(debug_stream, "SD x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x04:
				fprintf(debug_stream, "SQ x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x05:
				fprintf(debug_stream, "SQ2 x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x06:
				fprintf(debug_stream, "SQ4 x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x07:
				fprintf(debug_stream, "SQ8 x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x08:
				fprintf(debug_stream, "SBU x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x09:
				fprintf(debug_stream, "SHU x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x0a:
				fprintf(debug_stream, "SWU x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x0b:
				fprintf(debug_stream, "SDU x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x0c:
				fprintf(debug_stream, "SQU x%02d,  ", ROB_ptr->rs2);
			case 0x0d:
				fprintf(debug_stream, "SQ2U x%02d,  ", ROB_ptr->rs2);
			case 0x0e:
				fprintf(debug_stream, "SQ4U x%02d,  ", ROB_ptr->rs2);
			case 0x0f:
				fprintf(debug_stream, "SQ8U x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x10:
				fprintf(debug_stream, "SBS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x11:
				fprintf(debug_stream, "SHS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x12:
				fprintf(debug_stream, "SWS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x13:
				fprintf(debug_stream, "SDS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x14:
				fprintf(debug_stream, "SQS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x15:
				fprintf(debug_stream, "SQ2S x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x16:
				fprintf(debug_stream, "SQ4S x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x17:
				fprintf(debug_stream, "SQ8S x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x18:
				fprintf(debug_stream, "SBUS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x19:
				fprintf(debug_stream, "SHUS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x1a:
				fprintf(debug_stream, "SWUS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x1b:
				fprintf(debug_stream, "SDUS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x1c:
				fprintf(debug_stream, "SQUS x%02d,  ", ROB_ptr->rs2);
			case 0x1d:
				fprintf(debug_stream, "SQ2US x%02d,  ", ROB_ptr->rs2);
			case 0x1e:
				fprintf(debug_stream, "SQ4US x%02d,  ", ROB_ptr->rs2);
			case 0x1f:
				fprintf(debug_stream, "SQ8US x%02d,  ", ROB_ptr->rs2);
				break;
			default:
				debug++;
				break;
			}
			if (ROB_ptr->rs1 == 2) {
				fprintf(debug_stream, "0x%06x(sp) ", ROB_ptr->imm);
			}
			else if (ROB_ptr->rs1 == 3) {
				fprintf(debug_stream, "0x%06x(gp) ", ROB_ptr->imm);
			}
			else if (ROB_ptr->rs1 == 4) {
				fprintf(debug_stream, "0x%06x(tp) ", ROB_ptr->imm);
			}
			else {
				fprintf(debug_stream, "0x%06x(x%02d) ", ROB_ptr->imm, ROB_ptr->rs1);
			}
			fprintf(debug_stream, "0x%08x, clock: 0x%04llx\n", buffer, clock);
		}
		incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
	}
		 break;
	case STORE_FP_map: {// store
		csr[csr_hpmcounter11].value++;//load counter
		switch (priviledge) {
		case 0:
			break;
		case 1:
			csr[csr_shpmcounter11].value++;
			break;
		case 2:
			csr[csr_hhpmcounter11].value++;
			break;
		case 3:
			csr[csr_mhpmcounter11].value++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->q_select = store_q_select;
		ROB_ptr->uop = uop_F_STORE;
		ROB_ptr->imm = (((buffer) >> 25) & 0xffd00) | (((buffer) >> 20) & 0x3e0) | ((buffer >> 11) & 0x1f);
		ROB_ptr->rd = 0;
		ROB_ptr->rd_ptr = 0;

		if (debug_unit) {
			fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
			switch (vtype) {// bit 0x10 saturation math vs. overflow/underflow exceptions
			case 0x00:
				fprintf(debug_stream, "FSB x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x01:
				fprintf(debug_stream, "FSH x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x02:
				fprintf(debug_stream, "FSW x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x03:
				fprintf(debug_stream, "FSD x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x04:
				fprintf(debug_stream, "FSQ x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x05:
				fprintf(debug_stream, "FSQ2 x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x06:
				fprintf(debug_stream, "FSQ4 x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x07:
				fprintf(debug_stream, "FSQ8 x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x08:
				fprintf(debug_stream, "FSBU x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x09:
				fprintf(debug_stream, "FSHU x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x0a:
				fprintf(debug_stream, "FSWU x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x0b:
				fprintf(debug_stream, "FSDU x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x0c:
				fprintf(debug_stream, "FSQU x%02d,  ", ROB_ptr->rs2);
			case 0x0d:
				fprintf(debug_stream, "FSQ2U x%02d,  ", ROB_ptr->rs2);
			case 0x0e:
				fprintf(debug_stream, "FSQ4U x%02d,  ", ROB_ptr->rs2);
			case 0x0f:
				fprintf(debug_stream, "FSQ8U x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x10:
				fprintf(debug_stream, "FSBS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x11:
				fprintf(debug_stream, "FSHS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x12:
				fprintf(debug_stream, "FSWS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x13:
				fprintf(debug_stream, "FSDS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x14:
				fprintf(debug_stream, "FSQS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x15:
				fprintf(debug_stream, "FSQ2S x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x16:
				fprintf(debug_stream, "FSQ4S x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x17:
				fprintf(debug_stream, "FSQ8S x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x18:
				fprintf(debug_stream, "FSBUS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x19:
				fprintf(debug_stream, "FSHUS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x1a:
				fprintf(debug_stream, "FSWUS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x1b:
				fprintf(debug_stream, "FSDUS x%02d,  ", ROB_ptr->rs2);
				break;
			case 0x1c:
				fprintf(debug_stream, "FSQUS x%02d,  ", ROB_ptr->rs2);
			case 0x1d:
				fprintf(debug_stream, "FSQ2US x%02d,  ", ROB_ptr->rs2);
			case 0x1e:
				fprintf(debug_stream, "FSQ4US x%02d,  ", ROB_ptr->rs2);
			case 0x1f:
				fprintf(debug_stream, "FSQ8US x%02d,  ", ROB_ptr->rs2);
				break;
			default:
				debug++;
				break;
			}
			if (ROB_ptr->rs1 == 2) {
				fprintf(debug_stream, "0x%06x(sp) ", ROB_ptr->imm);
			}
			else if (ROB_ptr->rs1 == 3) {
				fprintf(debug_stream, "0x%06x(gp) ", ROB_ptr->imm);
			}
			else if (ROB_ptr->rs1 == 4) {
				fprintf(debug_stream, "0x%06x(tp) ", ROB_ptr->imm);
			}
			else {
				fprintf(debug_stream, "0x%06x(x%02d) ", ROB_ptr->imm, ROB_ptr->rs1);
			}
			fprintf(debug_stream, "0x%08x, clock: 0x%04llx\n", buffer, clock);
		}
		incr_decode48_ptr(ROB, shifter, decode_vars, csr, priviledge);
	}
		break;
	case AMO_map:
		break;
	case OP_map:
		break;
	case OP_64_map:
		break;
	case LUI_map:
		break;
	case MADD_map:
		break;
	case MSUB_map:
		break;
	case NMSUB_map:
		break;
	case NMADD_map:
		break;
	case OP_FP_map:
		break;
	case BRANCH_map:
		break;
	case JALR_map:
		break;
	case JAL_map:
		break;
	case SYSTEM_map:
		break;
	default:
		debug++;
		break;
	}
	return stop;
}
