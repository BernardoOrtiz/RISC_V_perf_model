// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "front_end.h"

void incr_decode32_ptr(ROB_Type* ROB, decode_shifter_struct* shifter, decode_type* decode_vars) {
	UINT debug = 0;
	ROB->q[ROB->decode_ptr].state = ROB_allocate_0;
	ROB->decode_ptr = ((ROB->decode_ptr + 1) & 0xff);
	
	shifter->index += 2;
	decode_vars->index++;
}
UINT8 fence_check(ROB_entry_Type* ROB_ptr, UINT8* flush_write_posters, ROB_Type* ROB, csr_type* csr, UINT8 prefetcher_idle, UINT stop_prefetches, UINT stores_active, UINT64 clock, store_buffer_type* store_buffer, UINT mhartid, UINT debug_unit, param_type* param, FILE* debug_stream) {
	UINT8 stop = 0;
	if (ROB->decode_ptr == ROB->retire_ptr_in || ROB->q[ROB->retire_ptr_in].state == ROB_fault) {// synchronize instruction execution

		UINT load_pending = 0;
		UINT q_id;
		for (q_id = 0; q_id < 0x100 && !load_pending; q_id++) {
			if (ROB->q[q_id].map == LOAD_map && ROB->q[q_id].state == ROB_inflight)
				load_pending = 1;
		}
		q_id--;
		// need to chack for prefetcher xaction pending
		if (load_pending) {// need to flush load buffers
			if (debug_unit) {
				fprintf(debug_stream, "DECODE(%lld): Fence Instr; Load Pending: mstatus: 0x%08x, mcause: 0x%08x,  return addr: 0x%08x, clock: 0x%04llx\n",
					mhartid, csr[csr_mstatus].value, csr[csr_mcause].value, csr[csr_sepc].value, clock);
			}
			stop = 1;
		}
		else if (stores_active) {// need to flush load buffers
			flush_write_posters[0] = 1;
			if (debug_unit) {
				if (store_buffer[0].status)
					fprintf(debug_stream, "DECODE(%lld): Fence Instr delayed; Store Pending 0x%04x mstatus: 0x%08x, mcause: 0x%08x, return addr: 0x%08x, clock: 0x%04llx\n",
						mhartid, store_buffer[0].xaction_id, csr[csr_mstatus].value, csr[csr_mcause].value, csr[csr_sepc].value, clock);
				if (store_buffer[1].status)
					fprintf(debug_stream, "DECODE(%lld): Fence Instr delayed; Store Pending 0x%04x mstatus: 0x%08x, mcause: 0x%08x, return addr: 0x%08x, clock: 0x%04llx\n",
						mhartid, store_buffer[1].xaction_id, csr[csr_mstatus].value, csr[csr_mcause].value, csr[csr_sepc].value, clock);
				if (store_buffer[2].status)
					fprintf(debug_stream, "DECODE(%lld): Fence Instr delayed; Store Pending 0x%04x mstatus: 0x%08x, mcause: 0x%08x, return addr: 0x%08x, clock: 0x%04llx\n",
						mhartid, store_buffer[2].xaction_id, csr[csr_mstatus].value, csr[csr_mcause].value, csr[csr_sepc].value, clock);
				if (store_buffer[3].status)
					fprintf(debug_stream, "DECODE(%lld): Fence Instr delayed; Store Pending 0x%04x mstatus: 0x%08x, mcause: 0x%08x, return addr: 0x%08x, clock: 0x%04llx\n",
						mhartid, store_buffer[3].xaction_id, csr[csr_mstatus].value, csr[csr_mcause].value, csr[csr_sepc].value, clock);
			}
			stop = 1;
		}
		else if (!prefetcher_idle && stop_prefetches) {
			if (debug_unit) {
				fprintf(debug_stream, "DECODE(%lld): Fence Instr; Fetch Pending (ROB entry: 0x%02x): mstatus: 0x%08x, mcause: 0x%08x, return addr: 0x%08x, clock: 0x%04llx\n",
					mhartid, q_id, csr[csr_mstatus].value, csr[csr_mcause].value, csr[csr_sepc].value, clock);
			}
			stop = 1;
		}
		else {
			ROB_ptr->q_select = store_q_select;
			ROB_ptr->map = MISC_MEM_map;
			ROB_ptr->uop = uop_FENCE;// flush write posters
			ROB_ptr->branch_num = ROB->branch_start = ROB->branch_stop;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "FENCE.%d ", ROB_ptr->funct3);
				fprintf(debug_stream, ", clock: 0x%04llx\n", clock);
			}
			ROB_ptr->state = ROB_allocate_0;
		}
	}
	else {
		stop = 1;
	}
	return stop;
}
UINT8 decode_32b(ROB_Type* ROB, UINT64 *perf_reg,UINT8* flush_write_posters, UINT8 priviledge, decode_shifter_struct* shifter, decode_type* decode_vars, UINT8 prefetcher_idle, UINT stores_pending, UINT load_pending,
	UINT64 clock, branch_vars* branch, store_buffer_type* store_buffer, UINT* uPC, UINT mhartid, UINT debug_core, param_type* param, FILE* debug_stream) {
	UINT8 debug = 0;
	UINT8 stop = 0;

	UINT debug_unit = (param->decoder == 1) && debug_core;
	UINT debug_branch = (param->decoder || param->branch) && debug_core;
	UINT debug_page_walk_unit = (param->decoder == 1 || param->PAGE_WALK == 1 || param->PAGE_FAULT == 1) && debug_core;

	ROB_entry_Type* ROB_ptr = &ROB->q[ROB->decode_ptr];
	if (ROB_ptr->state != ROB_free)
		debug++;

	INT buffer = (shifter->buffer[shifter->index + 1] << 16) | shifter->buffer[shifter->index + 0];
	UINT64 tag = shifter->tag[shifter->index + 0];
	INT buffer1 = 0;
	UINT64 tag1 = 0;
	if (decode_vars->index < (param->decode_width - 1)) {
		buffer1 = (shifter->buffer[shifter->index + 3] << 16) | shifter->buffer[shifter->index + 2];
		tag1 = shifter->tag[shifter->index + 2];
	}

	UINT8 funct2 = (buffer >> 25) & 3; // float size: 0-S,1-D, 2-reserved, 3-Q(128 bit)
	UINT8 funct7 = (buffer >> 25) & 0x7f;

	ROB_ptr->addr = tag;
	ROB_ptr->map = (opcode_map)((buffer >> 2) & 0x1f);
	ROB_ptr->funct3 = ((buffer >> 12) & 0x07);

	ROB_ptr->rd = (buffer >> 7) & 0x1f;
	ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
	ROB_ptr->rs2 = (buffer >> 20) & 0x1f;
	ROB_ptr->rs3 = (buffer >> 27) & 0x1f;

	ROB_ptr->branch_num = ROB->branch_stop;
	ROB_ptr->bytes = 4;
	switch (ROB_ptr->map) {
	case LOAD_map: {// load
		perf_reg[load_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[sload_count]++;
			break;
		case 2:
			perf_reg[hload_count]++;
			break;
		case 3:
			perf_reg[mload_count]++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->q_select = load_q_select;
		ROB_ptr->uop = uop_LOAD;
		ROB_ptr->reg_type = (ROB_ptr->funct3 >> 2) & 1;
		ROB_ptr->imm = (buffer >> 20);
		if (debug_unit) {
			fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
			switch (ROB_ptr->funct3) {
			case 0:
				fprintf(debug_stream, "LB x%02d,  ", ROB_ptr->rd);
				break;
			case 1:
				fprintf(debug_stream, "LH x%02d,  ", ROB_ptr->rd);
				break;
			case 2:
				fprintf(debug_stream, "LW x%02d,  ", ROB_ptr->rd);
				break;
			case 3:
				fprintf(debug_stream, "LD x%02d,  ", ROB_ptr->rd);
				break;
			case 4:
				fprintf(debug_stream, "LBU x%02d,  ", ROB_ptr->rd);
				break;
			case 5:
				fprintf(debug_stream, "LHU x%02d,  ", ROB_ptr->rd);
				break;
			case 6:
				fprintf(debug_stream, "LWU x%02d,  ", ROB_ptr->rd);
				break;
			case 7:
				fprintf(debug_stream, "LDU x%02d,  ", ROB_ptr->rd);
				break;
			default:
				debug++;
				break;
			}
			if (ROB_ptr->rs1 == 2) {
				fprintf(debug_stream, "0x%03x(sp) ", ROB_ptr->imm);
			}
			else if (ROB_ptr->rs1 == 3) {
				fprintf(debug_stream, "0x%03x(gp) ", ROB_ptr->imm);
			}
			else if (ROB_ptr->rs1 == 4) {
				fprintf(debug_stream, "0x%03x(tp) ", ROB_ptr->imm);
			}
			else {
				fprintf(debug_stream, "0x%03x(x%02d) ", ROB_ptr->imm, ROB_ptr->rs1);
			}
			fprintf(debug_stream, "0x%08x, clock: 0x%04llx\n", buffer, clock);
		}
		incr_decode32_ptr(ROB, shifter, decode_vars);
	}
				 break;
	case LOAD_FP_map: {//FLW; with SIMD extension
		perf_reg[load_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[sload_count]++;
			break;
		case 2:
			perf_reg[hload_count]++;
			break;
		case 3:
			perf_reg[mload_count]++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->q_select = load_q_select;
		ROB_ptr->uop = uop_F_LOAD;
		ROB_ptr->reg_type = 2;
	//	ROB_ptr->rd = (buffer >> 7) & 0x1f;
//		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->imm = (buffer >> 20);
		if (debug_unit) {
			fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
			switch (ROB_ptr->funct3) {
			case 0:
				fprintf(debug_stream, "FLB fp%02d,  ", ROB_ptr->rd);
			case 1:
				fprintf(debug_stream, "FLH fp%02d,  ", ROB_ptr->rd);
			case 2:
				fprintf(debug_stream, "FLW fp%02d,  ", ROB_ptr->rd);
			case 3:
				fprintf(debug_stream, "FLD fp%02d,  ", ROB_ptr->rd);
			case 4:
				fprintf(debug_stream, "FLQ fp%02d,  ", ROB_ptr->rd);
			case 5:
				fprintf(debug_stream, "FLQ2 fp%02d,  ", ROB_ptr->rd);
			case 6:
				fprintf(debug_stream, "FLQ4 fp%02d,  ", ROB_ptr->rd);
			case 7:
				fprintf(debug_stream, "FLQ8 fp%02d,  ", ROB_ptr->rd);
			default:
				debug++;
				break;
			}
			//					fprintf(debug_stream, "FLOAD fp%02d,  ", ROB_ptr->rd);
			if (ROB_ptr->rs1 == 2) {
				fprintf(debug_stream, "0x%03x(sp) ", ROB_ptr->imm);
			}
			else if (ROB_ptr->rs1 == 3) {
				fprintf(debug_stream, "0x%03x(gp) ", ROB_ptr->imm);
			}
			else if (ROB_ptr->rs1 == 4) {
				fprintf(debug_stream, "0x%03x(tp) ", ROB_ptr->imm);
			}
			else {
				fprintf(debug_stream, "0x%03x(reg%02d) ", ROB_ptr->imm, ROB_ptr->rs1);
			}
			fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
		}
		incr_decode32_ptr(ROB, shifter, decode_vars);
	}
					break;
	case MISC_MEM_map: {//fence - parameters??? 
		switch (ROB_ptr->funct3) {
		case 0:
		case 1: {// fence
			if (ROB->decode_ptr == ROB->retire_ptr_in || ROB->q[ROB->retire_ptr_in].state == ROB_fault) {// synchronize instruction execution

				UINT load_pending = 0;
				UINT q_id;
				for (q_id = 0; q_id < 0x100 && !load_pending; q_id++) {
					if (ROB->q[q_id].map == LOAD_map && ROB->q[q_id].state == ROB_inflight)
						load_pending = 1;
				}
				q_id--;
				// need to chack for prefetcher xaction pending
				if (load_pending) {// need to flush load buffers
					if (debug_unit) {
						fprintf(debug_stream, "DECODE(%lld): Fence Instr; Load Pending clock: 0x%04llx\n", mhartid, clock);
					}
					stop = 1;
				}
				else if (stores_pending) {// need to flush load buffers
					flush_write_posters[0] = 1;
					if (debug_unit) {
						if (store_buffer[0].status)
							fprintf(debug_stream, "DECODE(%lld): Fence Instr delayed; Store Pending 0x%04x clock: 0x%04llx\n", mhartid, store_buffer[0].xaction_id, clock);
						if (store_buffer[1].status)
							fprintf(debug_stream, "DECODE(%lld): Fence Instr delayed; Store Pending 0x%04x clock: 0x%04llx\n", mhartid, store_buffer[1].xaction_id, clock);
						if (store_buffer[2].status)
							fprintf(debug_stream, "DECODE(%lld): Fence Instr delayed; Store Pending 0x%04x clock: 0x%04llx\n", mhartid, store_buffer[2].xaction_id, clock);
						if (store_buffer[3].status)
							fprintf(debug_stream, "DECODE(%lld): Fence Instr delayed; Store Pending 0x%04x clock: 0x%04llx\n", mhartid, store_buffer[3].xaction_id, clock);
					}
					stop = 1;
				}
				else if (!prefetcher_idle) {
					if (debug_unit) {
						fprintf(debug_stream, "DECODE(%lld): Fence Instr; Fetch Pending (ROB entry: 0x%02x) clock: 0x%04llx\n", mhartid, q_id, clock);
					}
					stop = 1;
				}
				else {
					ROB_ptr->q_select = store_q_select;
					ROB_ptr->map = MISC_MEM_map;
					ROB_ptr->uop = uop_FENCE;// flush write posters
					ROB_ptr->branch_num = ROB->branch_start = ROB->branch_stop;
					if (debug_unit) {
						fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
						fprintf(debug_stream, "FENCE.%d, clock: 0x%04llx\n", ROB_ptr->funct3, clock);
					}
					ROB_ptr->state = ROB_allocate_0;
				}
			}
			else {
				stop = 1;
			}
			if (!stop) {
				switch (ROB_ptr->funct3) {// bit 0 = I cache, 1= D cache, 2 = L2 cache
				case 0: // fence
					break;
				case 1: // Zifencei - flush instruction cache as well
					break;
				default:
					break;
				}
				incr_decode32_ptr(ROB, shifter, decode_vars);
			}
		}
			  break;
		case 2:
		case 3: {
			perf_reg[load_count]++;
			switch (priviledge) {
			case 0:
				break;
			case 1:
				perf_reg[sload_count]++;
				break;
			case 2:
				perf_reg[hload_count]++;
				break;
			case 3:
				perf_reg[mload_count]++;
				break;
			default:
				debug++;
				break;
			}
			ROB_ptr->q_select = load_q_select;
			ROB_ptr->map = LOAD_map;
			ROB_ptr->uop = uop_LOAD;
			ROB_ptr->reg_type = ROB_ptr->funct3 & 1;
			ROB_ptr->rd = (buffer >> 7) & 0x1f;
			ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
			ROB_ptr->imm = (buffer >> 20);
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				if (ROB_ptr->reg_type)
					fprintf(debug_stream, "LQU x%02d,  ", ROB_ptr->rd);
				else
					fprintf(debug_stream, "LQ x%02d,  ", ROB_ptr->rd);
				if (ROB_ptr->rs1 == 2) {
					fprintf(debug_stream, "0x%03x(sp) ", ROB_ptr->imm);
				}
				else if (ROB_ptr->rs1 == 3) {
					fprintf(debug_stream, "0x%03x(gp) ", ROB_ptr->imm);
				}
				else if (ROB_ptr->rs1 == 4) {
					fprintf(debug_stream, "0x%03x(tp) ", ROB_ptr->imm);
				}
				else {
					fprintf(debug_stream, "0x%03x(x%02d) ", ROB_ptr->imm, ROB_ptr->rs1);
				}
				fprintf(debug_stream, "0x%08x, clock: 0x%04llx\n", buffer, clock);
			}
			ROB_ptr->funct3 = (ROB_ptr->reg_type) ? 12 : 8;
			incr_decode32_ptr(ROB, shifter, decode_vars);
		}
			  break;
		default:
			debug++;
			break;
		}
	}
		break;
	case OP_IMM_map:{ // Integer op rs1 to imm
		perf_reg[int_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[sint_count]++;
			break;
		case 2:
			perf_reg[hint_count]++;
			break;
		case 3:
			perf_reg[mint_count]++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
		decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));

		ROB_ptr->rs_count = 1;
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->imm = (buffer >> 20);//signed 

		ROB_ptr->reg_type = 0;
		switch (ROB_ptr->funct3) {
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
			incr_decode32_ptr(ROB, shifter, decode_vars);
			break;
		case 1: // SLLI - shift logical left
			ROB_ptr->uop = uop_SLLI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "SLLI(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			break;
		case 2: { //SLTI signed compare less than
			ROB_ptr->uop = uop_SLTI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "SLTI(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
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
			incr_decode32_ptr(ROB, shifter, decode_vars);
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
			incr_decode32_ptr(ROB, shifter, decode_vars);
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
			incr_decode32_ptr(ROB, shifter, decode_vars);
			break;
		case 6: //ORI
			ROB_ptr->uop = uop_ORI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "ORI(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			break;
		case 7: //ANDI
			ROB_ptr->uop = uop_ANDI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "ANDI(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			break;
		default: // error
			debug++;
			break;
		}
	}
	break;
	case AUIPC_map: {//AUIPC - load PC relative address for 32b unconditional jump
		if (decode_vars->index == 0) {
			if ((opcode_map)((buffer1 >> 2) & 0x1f) != JALR_map) {
				perf_reg[int_count]++;
				switch (priviledge) {
				case 0:
					break;
				case 1:
					perf_reg[sint_count]++;
					break;
				case 2:
					perf_reg[hint_count]++;
					break;
				case 3:
					perf_reg[mint_count]++;
					break;
				default:
					debug++;
					break;
				}
				ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
				decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));
				ROB_ptr->uop = uop_AUIPC;
				ROB_ptr->imm = buffer & 0xfffff000;
				ROB_ptr->rs_count = 0;
				if (debug_unit) {
					fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
					fprintf(debug_stream, "AUIPC(%d) xr%02d, 0x%016I64x", ROB_ptr->q_select, ROB_ptr->rd, (INT64)ROB_ptr->imm);
					fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
				}
				incr_decode32_ptr(ROB, shifter, decode_vars);
			}
			else {// fused instruction
				ROB_ptr->rd = (buffer1 >> 7) & 0x1f;
				ROB_ptr->addr = tag1;
				if (ROB_ptr->rd == 0) {
					ROB_ptr->rs1 = (buffer1 >> 15) & 0x1f;

					ROB_ptr->q_select = none_q_select;
					ROB_ptr->map = JAL_map; // skip register usage
					ROB_ptr->uop = uop_JAL;
					ROB_ptr->imm = buffer & 0xfffff000;
					ROB_ptr->imm |= buffer >> 20;
					ROB_ptr->rs_count = 1;

					shifter->response.msg = halt;
					ROB_ptr->state = ROB_allocate_0;
					if (debug_unit) {
						fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
						fprintf(debug_stream, "JAL xr%02d, offset = 0x%08x", ROB_ptr->rd, ((ROB_ptr->imm >> 11) & 0xfff00000) | (ROB_ptr->rd_ptr, ROB_ptr->imm & 0x000ff000) | ((ROB_ptr->imm >> 20) & 0x7fe) | ((ROB_ptr->imm >> 20) & 0x800));
						fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
					}
					incr_decode32_ptr(ROB, shifter, decode_vars);
				}
				else if (ROB_ptr->rd == 1) { // ERROR: wrong encoding
					ROB_ptr->map = SYSTEM_map;
					ROB_ptr->uop = uop_CSRRCI;
					ROB_ptr->rd = 1;
					ROB_ptr->rs_count = 0;
					ROB_ptr->csr = csr_uepc;// assumes target is in user space, need to fix
					ROB_ptr->imm = 0;
					ROB_ptr->addr = tag1;
					ROB_ptr->state = ROB_allocate_0;
					if (debug_unit) {
						fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
						if (ROB_ptr->rd == 2)
							fprintf(debug_stream, "CSRRWI sp xr%02d, ", ROB_ptr->rd);
						else
							fprintf(debug_stream, "CSRRWI xr%02d, ", ROB_ptr->rd);
						fprintf(debug_stream, "csr: uepc, 0x%02x ", ROB_ptr->imm);

						fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer1, clock);
					}
					incr_decode32_ptr(ROB, shifter, decode_vars);

					ROB_ptr = &ROB->q[ROB->decode_ptr];
					perf_reg[store_count]++;
					switch (priviledge) {
					case 0:
						break;
					case 1:
						perf_reg[sstore_count]++;
						break;
					case 2:
						perf_reg[hstore_count]++;
						break;
					case 3:
						perf_reg[mstore_count]++;
						break;
					default:
						debug++;
						break;
					}
					ROB_ptr->q_select = store_q_select;
					ROB_ptr->map = STORE_map;
					ROB_ptr->uop = uop_STORE;
					ROB_ptr->rs1 = 2;// stack pointer
					ROB_ptr->rs2 = 1;
					ROB_ptr->imm = -4;
					ROB_ptr->addr = tag1;
					ROB_ptr->state = ROB_allocate_0;
					if (debug_unit) {
						fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
						print_uop_decode(debug_stream, ROB_ptr[0], clock);
						fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer1, clock);
					}
					incr_decode32_ptr(ROB, shifter, decode_vars);

					ROB_ptr = &ROB->q[ROB->decode_ptr];
					perf_reg[store_count]++;
					switch (priviledge) {
					case 0:
						break;
					case 1:
						perf_reg[sstore_count]++;
						break;
					case 2:
						perf_reg[hstore_count]++;
						break;
					case 3:
						perf_reg[mstore_count]++;
						break;
					default:
						debug++;
						break;
					}
					ROB_ptr->q_select = store_q_select;
					ROB_ptr->map = STORE_map;
					ROB_ptr->uop = uop_STORE;
					ROB_ptr->rs1 = 2;// stack pointer
					ROB_ptr->rs2 = 2;
					ROB_ptr->imm = -8;// stack pointer
					ROB_ptr->addr = tag;
					ROB_ptr->state = ROB_allocate_0;
					if (debug_unit) {
						fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
						print_uop_decode(debug_stream, ROB_ptr[0], clock);
						fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer1, clock);
					}
					incr_decode32_ptr(ROB, shifter, decode_vars);

					ROB_ptr = &ROB->q[ROB->decode_ptr];		perf_reg[store_count]++;
					switch (priviledge) {
					case 0:
						break;
					case 1:
						perf_reg[sstore_count]++;
						break;
					case 2:
						perf_reg[hstore_count]++;
						break;
					case 3:
						perf_reg[mstore_count]++;
						break;
					default:
						debug++;
						break;
					}
					ROB_ptr->q_select = store_q_select;
					ROB_ptr->map = STORE_map;
					ROB_ptr->uop = uop_STORE;
					ROB_ptr->rs1 = 2;// stack pointer
					ROB_ptr->rs2 = 3;
					ROB_ptr->imm = -12;// stack pointer
					ROB_ptr->addr = tag1;
					ROB_ptr->state = ROB_allocate_0;
					if (debug_unit) {
						fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
						//								fprintf(debug_stream, "DECODE(%lld): ROB entry: 0x%02x-0x%02x 0x%016I64x ", mhartid, ROB->decode_ptr, ROB->retire_ptr_in, ROB_ptr->addr);
						print_uop_decode(debug_stream, ROB_ptr[0], clock);
						fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer1, clock);
					}
					incr_decode32_ptr(ROB, shifter, decode_vars);

					ROB_ptr = &ROB->q[ROB->decode_ptr];		perf_reg[store_count]++;
					switch (priviledge) {
					case 0:
						break;
					case 1:
						perf_reg[sstore_count]++;
						break;
					case 2:
						perf_reg[hstore_count]++;
						break;
					case 3:
						perf_reg[mstore_count]++;
						break;
					default:
						debug++;
						break;
					}
					ROB_ptr->q_select = store_q_select;
					ROB_ptr->map = STORE_map;
					ROB_ptr->uop = uop_STORE;
					ROB_ptr->rs1 = 2;// stack pointer
					ROB_ptr->rs2 = 4;
					ROB_ptr->imm = -16;// stack pointer
					ROB_ptr->addr = tag1;
					ROB_ptr->state = ROB_allocate_0;
					if (debug_unit) {
						fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
						print_uop_decode(debug_stream, ROB_ptr[0], clock);
						fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer1, clock);
					}
					incr_decode32_ptr(ROB, shifter, decode_vars);
					ROB_ptr = &ROB->q[ROB->decode_ptr];

					ROB_ptr->rd = (buffer >> 7) & 0x1f;
					ROB_ptr->rs1 = (buffer >> 15) & 0x1f;

					ROB_ptr->map = JALR_map;
					ROB_ptr->uop = uop_JALR;
					ROB_ptr->imm = buffer & 0xfffff000;
					ROB_ptr->imm |= buffer >> 20;
					ROB_ptr->rs_count = 1;
					shifter->response.msg = halt;// flush and restart prefetcher
					stop = 1;
					ROB_ptr->state = ROB_allocate_0;
					if (debug_unit) {
						fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
						print_uop_decode(debug_stream, ROB_ptr[0], clock);
						fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
					}
					incr_decode32_ptr(ROB, shifter, decode_vars);
				}
				else {
					debug++;
				}
			}
		}
		else {
			decode_vars->index--;
			stop = 1;
		}
	}
		break;
	case OP_IMM64_map: // Integer op rs1 to imm
	{
		perf_reg[int_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[sint_count]++;
			break;
		case 2:
			perf_reg[hint_count]++;
			break;
		case 3:
			perf_reg[mint_count]++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
		decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));

		ROB_ptr->rs_count = 1;
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->imm = (buffer >> 20);//signed 

		ROB_ptr->reg_type = 0;
		switch (ROB_ptr->funct3) {
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
			incr_decode32_ptr(ROB, shifter, decode_vars);
			break;
		case 1: // SLLI - shift logical left
			ROB_ptr->uop = uop_SLLI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "SLLI_64(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			break;
		case 2: { //SLTI signed compare less than
			ROB_ptr->uop = uop_SLTI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "SLTI_64(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
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
			incr_decode32_ptr(ROB, shifter, decode_vars);
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
			incr_decode32_ptr(ROB, shifter, decode_vars);
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
			incr_decode32_ptr(ROB, shifter, decode_vars);
			break;
		case 6: //ORI
			ROB_ptr->uop = uop_ORI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "ORI_64(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			break;
		case 7: //ANDI
			ROB_ptr->uop = uop_ANDI;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, "ANDI_64(%d)", ROB_ptr->q_select);
				fprintf(debug_stream, "xr%02d, xr%02d, 0x03x", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->imm);
				fprintf(debug_stream, "// 0x08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			break;
		default: // error
			debug++;
			break;
		}
	}
	break;
	case STORE_map: {// store
		perf_reg[store_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[sstore_count]++;
			break;
		case 2:
			perf_reg[hstore_count]++;
			break;
		case 3:
			perf_reg[mstore_count]++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->q_select = store_q_select;
		ROB_ptr->uop = uop_STORE;
		ROB_ptr->imm = ((INT)(buffer & 0xfe000000) >> 20) | ((buffer >> 7) & 0x1f);
		ROB_ptr->rd = 0;
		ROB_ptr->rd_ptr = 0;

		if (debug_unit) {
			fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
			switch (ROB_ptr->funct3) {
			case 0:
				fprintf(debug_stream, "SB x%02d, ", ROB_ptr->rs2);
				break;
			case 1:
				fprintf(debug_stream, "SH x%02d, ", ROB_ptr->rs2);
				break;
			case 2:
				fprintf(debug_stream, "SW x%02d, ", ROB_ptr->rs2);
				break;
			case 3:
				fprintf(debug_stream, "SD x%02d, ", ROB_ptr->rs2);
				break;
			case 4:
				fprintf(debug_stream, "SQ x%02d, ", ROB_ptr->rs2);
				break;
			default:
				debug++;
				fprintf(debug_stream, "STORE x%02d, ", ROB_ptr->rs2);
				break;
			}
			switch (ROB_ptr->rs1) {
			case 2:
				fprintf(debug_stream, "0x%03x(sp) ", ROB_ptr->imm);
				break;
			case 3:
				fprintf(debug_stream, "0x%03x(gp) ", ROB_ptr->imm);
				break;
			case 4:
				fprintf(debug_stream, "0x%03x(tp) ", ROB_ptr->imm);
				break;
			default:
				fprintf(debug_stream, "0x%03x(x%02d) ", ROB_ptr->imm, ROB_ptr->rs1);
				break;
			}
			fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
		}
		incr_decode32_ptr(ROB, shifter, decode_vars);
	}
				  break;
	case STORE_FP_map: {//FSW: floating point store with SIMD extension
		perf_reg[store_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[sstore_count]++;
			break;
		case 2:
			perf_reg[hstore_count]++;
			break;
		case 3:
			perf_reg[mstore_count]++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->q_select = store_q_select;
		ROB_ptr->uop = uop_F_STORE;
		ROB_ptr->imm = ((INT)(buffer & 0xfe000000) >> 20) | ((buffer >> 7) & 0x1f);
		ROB_ptr->rd = 0;
		ROB_ptr->rd_ptr = 0;
		if (debug_unit) {
			fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
			switch (ROB_ptr->funct3) {
			case 0:
				fprintf(debug_stream, "FSB x%02d, ", ROB_ptr->rs2);
				break;
			case 1:
				fprintf(debug_stream, "FSH x%02d, ", ROB_ptr->rs2);
				break;
			case 2:
				fprintf(debug_stream, "FSW x%02d, ", ROB_ptr->rs2);
				break;
			case 3:
				fprintf(debug_stream, "FSD x%02d, ", ROB_ptr->rs2);
				break;
			case 4:
				fprintf(debug_stream, "FSQ x%02d, ", ROB_ptr->rs2);
				break;
			case 5:
				fprintf(debug_stream, "FSQ2 x%02d, ", ROB_ptr->rs2);
				break;
			case 6:
				fprintf(debug_stream, "FSQ4 x%02d, ", ROB_ptr->rs2);
				break;
			case 7:
				fprintf(debug_stream, "FSQ8 x%02d, ", ROB_ptr->rs2);
				break;
			default:
				debug++;
				fprintf(debug_stream, "F STORE x%02d, ", ROB_ptr->rs2);
				break;
			}
			switch (ROB_ptr->rs1) {
			case 2:
				fprintf(debug_stream, "0x%03x(sp) ", ROB_ptr->imm);
				break;
			case 3:
				fprintf(debug_stream, "0x%03x(gp) ", ROB_ptr->imm);
				break;
			case 4:
				fprintf(debug_stream, "0x%03x(tp) ", ROB_ptr->imm);
				break;
			default:
				fprintf(debug_stream, "0x%03x(x%02d) ", ROB_ptr->imm, ROB_ptr->rs1);
				break;
			}
			fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
		}
		incr_decode32_ptr(ROB, shifter, decode_vars);
	}
					 break;
	case AMO_map: { // atomic instructions - serialize bus, not core
		perf_reg[atomic_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[satomic_count]++;
			break;
		case 2:
			perf_reg[hatomic_count]++;
			break;
		case 3:
			perf_reg[matomic_count]++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->q_select = store_q_select;
		if (clock == 0x1502)
			debug++;
		ROB_ptr->imm = buffer >> 25;
		UINT8 width = ROB_ptr->funct3;
		switch ((buffer >> 27) & 0x1f) {
		case 0://AMOADD.W
			debug++;
			break;
		case 1: // AMOSWAP.W
			debug++;
			break;
		case 2:// LR.W
			ROB_ptr->uop = uop_LR;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				if (ROB_ptr->funct3 == 2) {
					fprintf(debug_stream, "LR.W xr%02d,  ", ROB_ptr->rd);
				}
				else if (ROB_ptr->funct3 == 3) {
					fprintf(debug_stream, "LR.D xr%02d,  ", ROB_ptr->rd);
				}
				else {
					debug++; // syntax error
				}
				if (ROB_ptr->rs1 == 2) {
					fprintf(debug_stream, "0x%03x(sp) ", ROB_ptr->imm);
				}
				else if (ROB_ptr->rs1 == 3) {
					fprintf(debug_stream, "0x%03x(gp) ", ROB_ptr->imm);
				}
				else if (ROB_ptr->rs1 == 4) {
					fprintf(debug_stream, "0x%03x(tp) ", ROB_ptr->imm);
				}
				else {
					fprintf(debug_stream, "0x%03x(reg%02d) ", ROB_ptr->imm, ROB_ptr->rs1);
				}
				fprintf(debug_stream, "// 0x%08x, clock: 0x%04llx\n", buffer, clock);
			}
			break;
		case 3:// SC.W
			ROB_ptr->uop = uop_SC;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				if (ROB_ptr->funct3 == 2) {
					fprintf(debug_stream, "SC.W xr%02d, ", ROB_ptr->rd);		//	error response
				}
				else if(ROB_ptr->funct3 == 3) {
					fprintf(debug_stream, "SC.D xr%02d, ", ROB_ptr->rd);		//	error response
				}
				else {
					debug++; // syntax error
				}
				fprintf(debug_stream, "xr%02d, ", ROB_ptr->rs1);		//	addr
				fprintf(debug_stream, "xr%02d ", ROB_ptr->rs2);		//	data
				fprintf(debug_stream, "// 0x%08x, clock: 0x%04llx\n", buffer, clock);
			}
			break;
		case 4:// AMOXOR.W
			debug++;
			break;
		case 0x08:// AMOOR.W
			debug++;
			break;
		case 0x0c:// AMOAND.W
			debug++;
			break;
		case 0x10: // AMOMIN.W
			debug++;
			break;
		case 0x14: // AMOMAX.W
			debug++;
			break;
		case 0x18: // AMOMINU.W
			debug++;
			break;
		case 0x1C: // AMOMAXU.W
			debug++;
			break;
		default:
			break;
		}
		incr_decode32_ptr(ROB, shifter, decode_vars);
	}
				break;
	case OP_map:// 32b
	case OP_64_map: {// 64b
		perf_reg[int_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[sint_count]++;
			break;
		case 2:
			perf_reg[hint_count]++;
			break;
		case 3:
			perf_reg[mint_count]++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->rs_count = 2;
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->rs2 = (buffer >> 20) & 0x1f;

		ROB_ptr->reg_type = 0;

		if (funct7 != 0 && funct7 != 0x20)
			debug++;

		if (((buffer >> 25) & 1) == 0) {
			ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
			decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));

			switch (ROB_ptr->funct3) {
			case 0: //ADD/SUB
				if ((buffer >> 27) & 8) {// 1=sub, 0=add
					ROB_ptr->uop = uop_SUB;
				}
				else {
					ROB_ptr->uop = uop_ADD;
				}
				break;
			case 1: //SLL - shift logical left
				ROB_ptr->uop = uop_SLL;
				break;
			case 2: //SLT - less than immediate
				ROB_ptr->uop = uop_SLT;
				break;
			case 3: //SLTU - less than immediate unsigned
				ROB_ptr->uop = uop_SLTU;
				break;
			case 4: //XOR
				ROB_ptr->uop = uop_XOR;
				break;
			case 5: //SRL/SRA	

				if ((buffer >> 27) & 8) {
					ROB_ptr->uop = uop_SRA;
				}
				else {
					ROB_ptr->uop = uop_SRL;
				}
				break;
			case 6: //OR
				ROB_ptr->uop = uop_OR;
				break;
			case 7: //AND
				ROB_ptr->uop = uop_AND;
				break;
			default: // error
				break;
			}

		}
		else {
			ROB_ptr->q_select = iMUL_q_select;
			switch (ROB_ptr->funct3) {
			case 0://mul
				ROB_ptr->uop = uop_MUL;
				break;
			case 1://MULH
				break;
			case 2://MULHSU
				break;
			case 3://MULHU
				break;
			case 4://div
				ROB_ptr->uop = uop_DIV;
				break;
			case 5://divu
				ROB_ptr->uop = uop_DIVU;
				break;
			case 6://rem
				ROB_ptr->uop = uop_REM;
				break;
			case 7://remu
				ROB_ptr->uop = uop_REMU;
				break;
			default:
				debug++;
				break;
			}
		}
		if (debug_unit) {
			fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
			print_uop_decode(debug_stream, ROB_ptr[0], clock);
			fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
		}
		incr_decode32_ptr(ROB, shifter, decode_vars);
	}
				  break;
	case LUI_map: {//LUI - 
		perf_reg[int_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[sint_count]++;
			break;
		case 2:
			perf_reg[hint_count]++;
			break;
		case 3:
			perf_reg[mint_count]++;
			break;
		default:
			debug++;
			break;
		}

		if (decode_vars->index < (param->decode_width - 1)) {
			ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
			decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));
			if (mhartid == 0)
				debug++;
			ROB_ptr->uop = uop_LUI;// unconditional load immediate bits 31:12; use ADDI to load bits 11-0 for a full 32b immediate load
			if ((buffer1 & 0x7077) == 0x6013 && ((buffer1 >> 15) & 0x1f) == ROB_ptr->rd) {// or imm
				ROB_ptr->imm = buffer & 0xfffffffffffff000;
				ROB_ptr->imm |= (buffer1 >> 20);
				ROB_ptr->rd = (buffer1 >> 7) & 0x1f;
				ROB_ptr->rs_count = 0;
				ROB_ptr->bytes = 8;
				if (debug_unit) {
					fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
					fprintf(debug_stream, "LUI-ORI fused(%d) xr%02d, 0x%016I64x", ROB_ptr->q_select, ROB_ptr->rd, (INT64)ROB_ptr->imm);
					fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
				}
				incr_decode32_ptr(ROB, shifter, decode_vars);
				shifter->index += 2;
				decode_vars->index++;
				if (ROB->decode_ptr == ROB->retire_ptr_in)
					debug++;
			}
			else if ((buffer1 & 0x7077) == 0x7013 && ((buffer1 >> 15) & 0x1f) == ROB_ptr->rd) {// and imm
				ROB_ptr->imm = buffer & 0xfffffffffffff000;
				ROB_ptr->imm |= 0x00000fff;
				ROB_ptr->imm &= (buffer1 >> 20);
				ROB_ptr->rd = (buffer1 >> 7) & 0x1f;
				ROB_ptr->rs_count = 0;
				ROB_ptr->bytes = 8;
				if (debug_unit) {
					fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
					fprintf(debug_stream, "LUI-ANDi fused(%d) xr%02d, 0x%016I64x", ROB_ptr->q_select, ROB_ptr->rd, (INT64)ROB_ptr->imm);
					fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
				}
				incr_decode32_ptr(ROB, shifter, decode_vars);
				shifter->index += 2;
				decode_vars->index++;
				if (ROB->decode_ptr == ROB->retire_ptr_in)
					debug++;
			}
			else if ((buffer1 & 0x7077) == 0x0067 && ((buffer1 >> 15) & 0x1f) == ROB_ptr->rd) {// JALR
				ROB_ptr->imm = buffer & 0x000fffff000;
				ROB_ptr->imm |= ((buffer1 >> 20) & 0x0fff);
				ROB_ptr->rd = (buffer1 >> 7) & 0x1f;
				ROB_ptr->rs2 = 0;
				ROB_ptr->rs3 = 1; // branch taken
				ROB_ptr->rs_count = 1;
				ROB_ptr->bytes = 8;

				ROB_ptr->map = JALR_map;
				ROB_ptr->uop = uop_JALR;
				shifter->response.msg = halt;// flush and restart prefetcher

				if (debug_unit) {
					fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
					fprintf(debug_stream, "LUI-JALR fused(%d) xr%02d = 0x%08x, imm 0x%08x", ROB_ptr->q_select, ROB_ptr->rd, ROB_ptr->imm + ROB_ptr->addr, ROB_ptr->imm);
					fprintf(debug_stream, "// buffer1, buffer2 0x%08x, 0x%08x, clock: 0x%04llx\n", buffer, buffer1, clock);
				}
				incr_decode32_ptr(ROB, shifter, decode_vars);
				shifter->index += 2;
				decode_vars->index++;
				if (ROB->decode_ptr == ROB->retire_ptr_in)
					debug++;
			}
			else {
				ROB_ptr->imm = buffer & 0xfffffffffffff000;
				ROB_ptr->rs_count = 0;
				if (debug_unit) {
					fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
					fprintf(debug_stream, "LUI(%d) xr%02d, 0x%016I64x", ROB_ptr->q_select, ROB_ptr->rd, (INT64)ROB_ptr->imm);
					fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
				}
				incr_decode32_ptr(ROB, shifter, decode_vars);
			}
		}
		else {
			stop = 1;
		}
	}
				break;
	case MADD_map: {
		perf_reg[float_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[sfloat_count]++;
			break;
		case 2:
			perf_reg[hfloat_count]++;;
			break;
		case 3:
			perf_reg[mfloat_count]++;
			break;
		default:
			debug++;
			break;
		}
		debug++;
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->rs2 = (buffer >> 20) & 0x1f;
		ROB_ptr->rs3 = (buffer >> 27) & 0x1f;
		ROB_ptr->rs_count = 3;
		ROB_ptr->funct3 = (buffer >> 12) & 0x07;//rounding mode
		ROB_ptr->funct7 = (buffer >> 25) & 0x7f;// size
		ROB_ptr->q_select = op_fp_select;
		ROB_ptr->uop = uop_FMADD;
		if (debug_unit) {
			fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
			switch (ROB_ptr->funct7 & 3 == 0) {
			case 0:
				fprintf(debug_stream, "FMADD.s fp%d, fp%d, fp%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->rs2, ROB_ptr->rs3);
				break;
			case 1:
				fprintf(debug_stream, "FMADD.d fp%d, fp%d, fp%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->rs2, ROB_ptr->rs3);
				break;
			case 3:
				fprintf(debug_stream, "FMADD.q fp%d, fp%d, fp%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->rs2, ROB_ptr->rs3);
				break;
			case 2:
				fprintf(debug_stream, "FMADD.h fp%d, fp%d, fp%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->rs2, ROB_ptr->rs3);
				break;
			default:
				debug++;
				break;
			}
			fprintf(debug_stream, "//  0x%08x, clock: 0x%04llx\n", buffer, clock);
		}
		incr_decode32_ptr(ROB, shifter, decode_vars);
	}
				 break;
	case MSUB_map: { //FMSUB - 3 entry
		perf_reg[float_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[sfloat_count]++;
			break;
		case 2:
			perf_reg[hfloat_count]++;;
			break;
		case 3:
			perf_reg[mfloat_count]++;
			break;
		default:
			debug++;
			break;
		}
		debug++;
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->rs2 = (buffer >> 20) & 0x1f;
		ROB_ptr->rs3 = (buffer >> 27) & 0x1f;
		ROB_ptr->rs_count = 3;
		ROB_ptr->funct3 = (buffer >> 12) & 0x07;//rounding mode
		ROB_ptr->funct7 = (buffer >> 25) & 0x7f;// size
		ROB_ptr->q_select = op_fp_select;
		ROB_ptr->uop = uop_FMSUB;
		if (debug_unit) {
			fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
			fprintf(debug_stream, "FMSUB fp%d, fp%d, fp%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->rs2, ROB_ptr->rs3);
			fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
		}
		incr_decode32_ptr(ROB, shifter, decode_vars);
	}
				 break;
	case NMSUB_map: {//FNMSUB - 3 entry
		perf_reg[float_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[sfloat_count]++;
			break;
		case 2:
			perf_reg[hfloat_count]++;;
			break;
		case 3:
			perf_reg[mfloat_count]++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->rs2 = (buffer >> 20) & 0x1f;
		ROB_ptr->rs3 = (buffer >> 27) & 0x1f;
		ROB_ptr->rs_count = 3;
		ROB_ptr->funct3 = (buffer >> 12) & 0x07;//rounding mode
		ROB_ptr->funct7 = (buffer >> 25) & 0x7f;// size
		ROB_ptr->q_select = op_fp_select;
		ROB_ptr->uop = uop_FNMSUB;
		if (debug_unit) {
			fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
			fprintf(debug_stream, "FNMSUB fp%d, fp%d, fp%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->rs2, ROB_ptr->rs3);
			fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
		}
		incr_decode32_ptr(ROB, shifter, decode_vars);
	}
				  break;
	case NMADD_map: { //FNMADD - 3 entry
		perf_reg[float_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[sfloat_count]++;
			break;
		case 2:
			perf_reg[hfloat_count]++;;
			break;
		case 3:
			perf_reg[mfloat_count]++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->rs2 = (buffer >> 20) & 0x1f;
		ROB_ptr->rs3 = (buffer >> 27) & 0x1f;
		ROB_ptr->rs_count = 3;
		ROB_ptr->funct3 = (buffer >> 12) & 0x07;//rounding mode
		ROB_ptr->funct7 = (buffer >> 25) & 0x7f;// size
		ROB_ptr->q_select = op_fp_select;
		ROB_ptr->uop = uop_FNMADD;
		if (debug_unit) {
			fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
			fprintf(debug_stream, "FNMADD fp%d, fp%d, fp%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->rs2, ROB_ptr->rs3);
			fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
		}
		incr_decode32_ptr(ROB, shifter, decode_vars);
	}
				  break;
	case OP_FP_map: {//float
		perf_reg[float_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[sfloat_count]++;
			break;
		case 2:
			perf_reg[hfloat_count]++;;
			break;
		case 3:
			perf_reg[mfloat_count]++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->rs2 = (buffer >> 20) & 0x1f;
		ROB_ptr->funct3 = (buffer >> 12) & 0x07;
		ROB_ptr->funct7 = (buffer >> 25) & 0x7f;// need to decode partially - which q

		ROB_ptr->rs_count = 2;
		if (debug_unit) {
			fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
		}
		char fp_size;
		switch (ROB_ptr->funct7 & 3 == 0) {
		case 0:
			fp_size = 's';//32b
			break;
		case 1:
			fp_size = 'd';//64b
			break;
		case 3:
			fp_size = 'q';//128b
			break;
		case 2:
			fp_size = 'h';//16b
			break;
		default:
			debug++;
			break;
		}
		switch ((buffer >> 27) & 0x1f) {
		case 0:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FADD;
			if (debug_unit) {
				fprintf(debug_stream, "FADD fp%d, fp%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->rs2);
			}
			break;
		case 1:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FSUB;
			if (debug_unit) {
				fprintf(debug_stream, "FSUB fp%d, fp%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->rs2);
			}
			break;
		case 2:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FMUL;
			if (debug_unit) {
				fprintf(debug_stream, "FMUL fp%d, fp%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->rs2);
			}
			break;
		case 3:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FDIV;
			if (debug_unit) {
				fprintf(debug_stream, "FDIV fp%d, fp%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->rs2);
			}
			break;
		case 4:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FSGN;
			if (debug_unit) {
				fprintf(debug_stream, "FSGN fp%d, fp%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->rs2);
			}
			break;
		case 5:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = (buffer & 0x00001000) ? uop_FMAX : uop_FMIN;
			if (debug_unit) {
				fprintf(debug_stream, "FMAX/FMIN fp%d, fp%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1, ROB_ptr->rs2);
			}
			break;
		case 0x18:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FCVTi2f;// int to float
			if (debug_unit) {
				fprintf(debug_stream, "FCVTi2f.%c fp%d, x%d", fp_size, ROB_ptr->rd, ROB_ptr->rs1);
			}
			break;
		case 0x1a:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FCVTf2i;//float to int
			if (debug_unit) {
				fprintf(debug_stream, "FCVTf2i x%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1);
			}
			break;
		case 0x1c:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FMVi2f;
			if (debug_unit) {
				fprintf(debug_stream, "FMVi2f fp%d, x%d", ROB_ptr->rd, ROB_ptr->rs1);
			}
			break;
		case 0x1e:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FMVf2i;//float to int
			if (debug_unit) {
				fprintf(debug_stream, "FMVf2i x%d, fp%d", ROB_ptr->rd, ROB_ptr->rs1);
			}
			break;
		default:
			debug++;
			break;
		}
		if (debug_unit) {
			fprintf(debug_stream, "// 0x%08x, clock: 0x%04llx\n", buffer, clock);
		}
		incr_decode32_ptr(ROB, shifter, decode_vars);
	}
				  break;
	case BRANCH_map: { // branch conditional (4KB range) - only 1 per decode block 
		//		if (decode_vars->index == 0) {
		perf_reg[branch_count]++;
		switch (priviledge) {
		case 0:
			break;
		case 1:
			perf_reg[sbranch_count]++;
			break;
		case 2:
			perf_reg[hbranch_count]++;;
			break;
		case 3:
			perf_reg[mbranch_count]++;
			break;
		default:
			debug++;
			break;
		}
		ROB_ptr->rd = 0;
		ROB_ptr->imm = ((buffer >> 31) & ~0x0eff) | (((buffer << 4) & 0x800) | ((buffer >> 20) & 0x7e0) | ((buffer >> 7) & 0x1e));
		// error: need bit for prediction value
		if (branch->count[ROB_ptr->addr & 0xfff] == branch->total[ROB_ptr->addr & 0xfff] || branch->count[ROB_ptr->addr & 0xfff] == 0) {
			if (branch->pred[ROB_ptr->addr & 0xfff])
				ROB_ptr->rs3 = 3; // branches taken
			else
				ROB_ptr->rs3 = 1; // branches not taken
		}
		else {
			if (branch->pred[ROB_ptr->addr & 0xfff])
				ROB_ptr->rs3 = 3; // branches taken
			else
				ROB_ptr->rs3 = 1; // branches not taken
		}
		ROB_ptr->branch_num = ROB->branch_stop;
		ROB_ptr->rs_count = 2;
		ROB->branch_stop = ((ROB->branch_stop + 1) & 3);
		if (mhartid == 0)
			debug++;
		switch (ROB_ptr->funct3) {// decode funct3 in branch unit, not here
		case 0:// BEQ 
			ROB_ptr->uop = uop_BEQ;
			break;
		case 1://BNE
			ROB_ptr->uop = uop_BNE;
			break;
		case 4://BLT
			ROB_ptr->uop = uop_BLT;
			break;
		case 5:// BGE
			ROB_ptr->uop = uop_BGE;
			break;
		case 6:// BLTU
			ROB_ptr->uop = uop_BLTU;
			break;
		case 7:// BGEU
			ROB_ptr->uop = uop_BGEU;
			break;
		default: // error
			break;
		}
		shifter->response.msg = (branch_response)ROB_ptr->rs3;
		if (ROB_ptr->rs3 == 3) {
			shifter->response.msg = taken;
		}
		shifter->response.addr = ROB_ptr->imm + ROB_ptr->addr;
		if (debug_branch) {
			fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
			fprintf(debug_stream, " BRANCH(start:stop) (%d:%d)", ROB->branch_start, ROB->branch_stop);

			switch (ROB_ptr->uop) {
			case uop_BEQ:
				fprintf(debug_stream, "BEQ x%02d, x%02d, %#06x ", ROB_ptr->rs1, ROB_ptr->rs2, ROB_ptr->imm);
				break;
			case uop_BNE:
				fprintf(debug_stream, "BNE x%02d, x%02d, %#06x ", ROB_ptr->rs1, ROB_ptr->rs2, ROB_ptr->imm);
				break;
			case uop_BGE:
				fprintf(debug_stream, "BGE x%02d, x%02d, %#06x ", ROB_ptr->rs1, ROB_ptr->rs2, ROB_ptr->imm);
				break;
			case uop_BLT:
				fprintf(debug_stream, "BLT x%02d, x%02d, %#06x ", ROB_ptr->rs1, ROB_ptr->rs2, ROB_ptr->imm);
				break;
			default:
				debug++;
				break;
			}
			fprintf(debug_stream, ", target addr: 0x%016x, taken: %d // 0x%08x,clock: 0x%04llx\n", ROB_ptr->imm + ROB_ptr->addr, ROB_ptr->rs3 >> 1, buffer, clock);
		}
		incr_decode32_ptr(ROB, shifter, decode_vars);
		stop = 1;
	}
	// NOTE: need to have transaction queue and match addresses in case of dropped prefetch - do not latch data.
	break;
	case JALR_map: {// JALR - unconditional jump and link register (32b)
		shifter->response.msg = halt;// flush and restart prefetcher
		if ((ROB_ptr->rd == 1 || ROB_ptr->rd == 5) && (ROB_ptr->rs1 == 1 || ROB_ptr->rs1 == 5)) { // return to caller	

			ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
			decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));

			ROB_ptr->map = OP_IMM64_map;
			ROB_ptr->uop = uop_ADDI; // copy RA to reg 31
			ROB_ptr->rd = 31;
			ROB_ptr->rs1 = 1;// RA
			ROB_ptr->rs_count = 1;
			ROB_ptr->funct3 = 3;
			ROB_ptr->imm = 0;
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			ROB_ptr->state = ROB_allocate_0;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, "0x%08x, clock: 0x%04llx return to caller \n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			ROB_ptr = &ROB->q[ROB->decode_ptr];

			ROB_ptr->q_select = load_q_select;
			ROB_ptr->map = LOAD_map;
			ROB_ptr->uop = uop_LOAD;
			ROB_ptr->rd = 1;
			ROB_ptr->rs1 = 2;// stack pointer
			ROB_ptr->funct3 = 3;
			ROB_ptr->imm = -8;
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			ROB_ptr->state = ROB_allocate_0;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				//							fprintf(debug_stream, "DECODE(%lld): ROB entry: 0x%02x-0x%02x 0x%016I64x ", mhartid, ROB->decode_ptr, ROB->retire_ptr_in, ROB_ptr->addr);
				if (ROB_ptr->reg_type)
					fprintf(debug_stream, "LQU x%02d,  ", ROB_ptr->rd);
				else
					fprintf(debug_stream, "LQ x%02d,  ", ROB_ptr->rd);
				if (ROB_ptr->rs1 == 2) {
					fprintf(debug_stream, "0x%03x(sp) ", ROB_ptr->imm);
				}
				else if (ROB_ptr->rs1 == 3) {
					fprintf(debug_stream, "0x%03x(gp) ", ROB_ptr->imm);
				}
				else if (ROB_ptr->rs1 == 4) {
					fprintf(debug_stream, "0x%03x(tp) ", ROB_ptr->imm);
				}
				else {
					fprintf(debug_stream, "0x%03x(x%02d) ", ROB_ptr->imm, ROB_ptr->rs1);
				}
				fprintf(debug_stream, "0x%08x, clock: 0x%04llx return to caller \n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			ROB_ptr->q_select = load_q_select;
			ROB_ptr->map = LOAD_map;
			ROB_ptr->uop = uop_LOAD;
			ROB_ptr->rd = 3;
			ROB_ptr->rs1 = 2;// stack pointer
			ROB_ptr->funct3 = 3;
			ROB_ptr->imm = -0x18;
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			//							ROB_ptr->instr_count = 1;
			ROB_ptr->state = ROB_allocate_0;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, "0x%08x, clock: 0x%04llx return to caller \n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			ROB_ptr->q_select = load_q_select;
			ROB_ptr->map = LOAD_map;
			ROB_ptr->uop = uop_LOAD;
			ROB_ptr->rd = 4;
			ROB_ptr->rs1 = 2;// stack pointer
			ROB_ptr->funct3 = 3;
			ROB_ptr->imm = -0x20;
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			//							ROB_ptr->instr_count = 1;
			ROB_ptr->state = ROB_allocate_0;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				//							fprintf(debug_stream, "DECODE(%lld): ROB entry: 0x%02x-0x%02x 0x%016I64x ", mhartid, ROB->decode_ptr, ROB->retire_ptr_in, ROB_ptr->addr);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, "0x%08x, clock: 0x%04llx return to caller \n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			ROB_ptr->q_select = load_q_select;
			ROB_ptr->map = LOAD_map;
			ROB_ptr->uop = uop_LOAD;
			ROB_ptr->rd = 2;
			ROB_ptr->rs1 = 2;// stack pointer
			ROB_ptr->funct3 = 3;
			ROB_ptr->imm = -0x10;
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			ROB_ptr->state = ROB_allocate_0;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				//							fprintf(debug_stream, "DECODE(%lld): ROB entry: 0x%02x-0x%02x 0x%016I64x ", mhartid, ROB->decode_ptr, ROB->retire_ptr_in, ROB_ptr->addr);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, "0x%08x, clock: 0x%04llx return to caller \n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			
			perf_reg[branch_count]++;
			switch (priviledge) {
			case 0:
				break;
			case 1:
				perf_reg[sbranch_count]++;
				break;
			case 2:
				perf_reg[hbranch_count]++;;
				break;
			case 3:
				perf_reg[mbranch_count]++;
				break;
			default:
				debug++;
				break;
			}
			ROB_ptr = &ROB->q[ROB->decode_ptr];
			ROB_ptr->rd = (buffer >> 7) & 0x1f;
			ROB_ptr->map = JALR_map;
			ROB_ptr->uop = uop_JALR;
			ROB_ptr->imm = buffer >> 20;
			ROB_ptr->rs_count = 1;
			ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
			shifter->response.msg = halt;// flush and restart prefetcher

			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 4;
			ROB_ptr->state = ROB_allocate_0;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, ", 0x%08x// return to caller clock: 0x%04x\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
		}
		else if (ROB_ptr->rd == 1 || ROB_ptr->rd == 5) {// load up TLB's with copy, be able to detect page walk
			ROB_ptr->map = SYSTEM_map;
			ROB_ptr->uop = uop_HALT;// halt prefetching, wait for JALR
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			ROB_ptr->rs_count = 0;
			ROB_ptr->state = ROB_allocate_0;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			// disable interrupts and save previous state - hardwired to occur immediately; not disturb reg bank
//			csr[csr_mstatus].value = ~0x80;
//			csr[csr_mstatus].value = (csr[csr_mstatus].value << 4) & 0x80;
//			csr[csr_mstatus].value = ~0x08;
			ROB_ptr->q_select = store_q_select;
			ROB_ptr->map = STORE_map;
			ROB_ptr->uop = uop_STORE;
			ROB_ptr->rs1 = 2;// stack pointer
			ROB_ptr->rs2 = 1;
			ROB_ptr->imm = -4;
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			ROB_ptr->state = ROB_allocate_0;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			ROB_ptr->q_select = store_q_select;
			ROB_ptr->map = STORE_map;
			ROB_ptr->uop = uop_STORE;
			ROB_ptr->rs1 = 2;// stack pointer
			ROB_ptr->rs2 = 2;
			ROB_ptr->imm = -8;// stack pointer
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			ROB_ptr->state = ROB_allocate_0;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			ROB_ptr->q_select = store_q_select;
			ROB_ptr->map = STORE_map;
			ROB_ptr->uop = uop_STORE;
			ROB_ptr->rs1 = 2;// stack pointer
			ROB_ptr->rs2 = 3;
			ROB_ptr->imm = -12;// stack pointer
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			//							ROB_ptr->instr_count = 1;
			ROB_ptr->state = ROB_allocate_0;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			ROB_ptr->q_select = store_q_select;
			ROB_ptr->map = STORE_map;
			ROB_ptr->uop = uop_STORE;
			ROB_ptr->rs1 = 2;// stack pointer
			ROB_ptr->rs2 = 4;
			ROB_ptr->imm = -16;// stack pointer
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			ROB_ptr->state = ROB_allocate_0;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			ROB_ptr->map = SYSTEM_map;
			ROB_ptr->uop = uop_CSRRWI;
			ROB_ptr->rd = (buffer >> 15) & 0x1f;
			ROB_ptr->rs_count = 0;
			ROB_ptr->imm = 0;
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			ROB_ptr->state = ROB_allocate_0;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			ROB_ptr->rd = (buffer >> 7) & 0x1f;
			ROB_ptr->rs1 = (buffer >> 15) & 0x1f;

			ROB_ptr->map = JALR_map;
			ROB_ptr->uop = uop_JALR;
			ROB_ptr->imm = buffer >> 20;
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 4;
			ROB_ptr->rs_count = 1;
			stop = 1;
			ROB_ptr->state = ROB_allocate_0;
			shifter->response.msg = halt;// flush and restart prefetcher

			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			ROB_ptr->rs1 = (buffer >> 15) & 0x1f;

			ROB_ptr->map = JALR_map;
			ROB_ptr->uop = uop_JALR;
			ROB_ptr->imm = buffer >> 20;
			ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
			ROB_ptr->rs_count = 1;

			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 4;
			ROB_ptr->state = ROB_allocate_0;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx return to caller\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
		}
		else {
			ROB_ptr->imm = buffer >> 20;
			ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
			perf_reg[branch_count]++;
			switch (priviledge) {
			case 0:
				break;
			case 1:
				perf_reg[sbranch_count]++;
				break;
			case 2:
				perf_reg[hbranch_count]++;;
				break;
			case 3:
				perf_reg[mbranch_count]++;
				break;
			default:
				debug++;
				break;
			}

			ROB_ptr->map = JALR_map;
			ROB_ptr->uop = uop_JALR;
			ROB_ptr->imm = buffer >> 20;
			ROB_ptr->rs_count = 1;
			shifter->response.msg = halt;// flush and restart prefetcher

			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 4;
			ROB_ptr->state = ROB_allocate_0;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, "unit (%d), 0x%08x, clock: 0x%04llx return to caller\n", decode_vars->int_index, buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
			shifter->response.msg = halt;
		}
		stop = 1;
	}
		break;
	case JAL_map: {// JAL - unconditional jump to immediate address (20b)
		if (ROB_ptr->rd == 1 || ROB_ptr->rd == 5) {
			if (decode_vars->index == 0) {
				perf_reg[store_count]++;
				switch (priviledge) {
				case 0:
					break;
				case 1:
					perf_reg[sstore_count]++;
					break;
				case 2:
					perf_reg[hstore_count]++;
					break;
				case 3:
					perf_reg[mstore_count]++;
					break;
				default:
					debug++;
					break;
				}
				ROB_ptr->q_select = store_q_select;
				ROB_ptr->map = STORE_map;
				ROB_ptr->uop = uop_STORE;
				ROB_ptr->rs1 = 2;// stack pointer
				ROB_ptr->rs2 = 1;
				ROB_ptr->funct3 = 3;
				ROB_ptr->imm = -8;
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				ROB_ptr->state = ROB_allocate_0;
				if (debug_unit) {
					fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
					print_uop_decode(debug_stream, ROB_ptr[0], clock);
					fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
				}
				incr_decode32_ptr(ROB, shifter, decode_vars);
				ROB_ptr = &ROB->q[ROB->decode_ptr];		
				perf_reg[store_count]++;
				switch (priviledge) {
				case 0:
					break;
				case 1:
					perf_reg[sstore_count]++;
					break;
				case 2:
					perf_reg[hstore_count]++;
					break;
				case 3:
					perf_reg[mstore_count]++;
					break;
				default:
					debug++;
					break;
				}
				ROB_ptr->q_select = store_q_select;
				ROB_ptr->map = STORE_map;
				ROB_ptr->uop = uop_STORE;
				ROB_ptr->rs1 = 2;// stack pointer
				ROB_ptr->rs2 = 2;
				ROB_ptr->funct3 = 3;
				ROB_ptr->imm = -16;// stack pointer
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				ROB_ptr->state = ROB_allocate_0;
				if (debug_unit) {
					fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
					print_uop_decode(debug_stream, ROB_ptr[0], clock);
					fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
				}
				incr_decode32_ptr(ROB, shifter, decode_vars);
				ROB_ptr = &ROB->q[ROB->decode_ptr];
				perf_reg[store_count]++;
				switch (priviledge) {
				case 0:
					break;
				case 1:
					perf_reg[sstore_count]++;
					break;
				case 2:
					perf_reg[hstore_count]++;
					break;
				case 3:
					perf_reg[mstore_count]++;
					break;
				default:
					debug++;
					break;
				}
				ROB_ptr->q_select = store_q_select;
				ROB_ptr->map = STORE_map;
				ROB_ptr->uop = uop_STORE;
				ROB_ptr->rs1 = 2;// stack pointer
				ROB_ptr->rs2 = 3;
				ROB_ptr->funct3 = 3;
				ROB_ptr->imm = -24;// stack pointer
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				ROB_ptr->state = ROB_allocate_0;
				if (debug_unit) {
					fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
					print_uop_decode(debug_stream, ROB_ptr[0], clock);
					fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
				}
				incr_decode32_ptr(ROB, shifter, decode_vars);
				ROB_ptr = &ROB->q[ROB->decode_ptr];
				perf_reg[store_count]++;
				switch (priviledge) {
				case 0:
					break;
				case 1:
					perf_reg[sstore_count]++;
					break;
				case 2:
					perf_reg[hstore_count]++;
					break;
				case 3:
					perf_reg[mstore_count]++;
					break;
				default:
					debug++;
					break;
				}
				ROB_ptr->q_select = store_q_select;
				ROB_ptr->map = STORE_map;
				ROB_ptr->uop = uop_STORE;
				ROB_ptr->rs1 = 2;// stack pointer
				ROB_ptr->rs2 = 4;
				ROB_ptr->funct3 = 3;
				ROB_ptr->imm = -32;// stack pointer
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				ROB_ptr->state = ROB_allocate_0;
				if (debug_unit) {
					fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
					print_uop_decode(debug_stream, ROB_ptr[0], clock);
					fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
				}
				incr_decode32_ptr(ROB, shifter, decode_vars);
				ROB_ptr = &ROB->q[ROB->decode_ptr];

				ROB_ptr->map = SYSTEM_map;
				ROB_ptr->uop = uop_CSRRCI;
				ROB_ptr->rd = 1;
				ROB_ptr->rs_count = 0;
				ROB_ptr->imm = 0;
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				ROB_ptr->state = ROB_allocate_0;
				if (debug_unit) {
					fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
					print_uop_decode(debug_stream, ROB_ptr[0], clock);
					fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
				}
				incr_decode32_ptr(ROB, shifter, decode_vars);
				ROB_ptr = &ROB->q[ROB->decode_ptr];

				ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
				decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));

				ROB_ptr->map = OP_IMM64_map;
				ROB_ptr->uop = uop_ADDI;
				ROB_ptr->rd = 1;
				ROB_ptr->rs1 = 1;
				ROB_ptr->imm = 4;
				ROB_ptr->rs_count = 1;
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				ROB_ptr->state = ROB_allocate_0;
				if (debug_unit) {
					fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
					print_uop_decode(debug_stream, ROB_ptr[0], clock);
					fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
				}
				incr_decode32_ptr(ROB, shifter, decode_vars);
				ROB_ptr = &ROB->q[ROB->decode_ptr];
				perf_reg[branch_count]++;
				switch (priviledge) {
				case 0:
					break;
				case 1:
					perf_reg[sbranch_count]++;
					break;
				case 2:
					perf_reg[hbranch_count]++;;
					break;
				case 3:
					perf_reg[mbranch_count]++;
					break;
				default:
					debug++;
					break;
				}

				ROB_ptr->rd = (buffer >> 7) & 0x1f;

				ROB_ptr->q_select = none_q_select;
				ROB_ptr->map = JAL_map;
				ROB_ptr->uop = uop_JAL;
				ROB_ptr->imm = buffer >> 20;
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 4;
				//						ROB_ptr->instr_count = 1;
				ROB_ptr->rs_count = 0;

				ROB_ptr->imm = buffer & 0xff000;
				ROB_ptr->imm |= ((buffer >> 9) & 0x800);
				ROB_ptr->imm |= ((buffer >> 20) & 0x7fe);
				ROB_ptr->imm |= ((buffer >> 11) & 0xfff00000);

				shifter->response.msg = unconditional;
				shifter->response.addr = ROB_ptr->imm + tag;

				ROB_ptr->state = ROB_allocate_0;
				if (debug_unit) {
					fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
					fprintf(debug_stream, "JAL, 0x%08x, clock: 0x%04llx\n", buffer, clock);
				}
				incr_decode32_ptr(ROB, shifter, decode_vars);
			}
			else {
				decode_vars->index--;
			}
		}
		else {
			perf_reg[branch_count]++;
			switch (priviledge) {
			case 0:
				break;
			case 1:
				perf_reg[sbranch_count]++;
				break;
			case 2:
				perf_reg[hbranch_count]++;;
				break;
			case 3:
				perf_reg[mbranch_count]++;
				break;
			default:
				debug++;
				break;
			}

			ROB_ptr->map = JAL_map;
			ROB_ptr->uop = uop_JAL;
			ROB_ptr->q_select = none_q_select;
			ROB_ptr->rs_count = 0;

			ROB_ptr->imm = buffer & 0xff000;
			ROB_ptr->imm |= ((buffer >> 9) & 0x800);
			ROB_ptr->imm |= ((buffer >> 20) & 0x7fe);
			ROB_ptr->imm |= ((buffer >> 11) & 0xfff00000);

			ROB_ptr->addr = tag;

			shifter->response.msg = unconditional;
			shifter->response.addr = ((INT64)ROB_ptr->imm) + tag;

			ROB_ptr->state = ROB_retire_out;
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				fprintf(debug_stream, " JAL(%d) ", decode_vars->index);
				fprintf(debug_stream, "target: 0x%016I64x, Imm: 0x%08x, OpCode: 0x%08x, clock: 0x%04llx\n",
					shifter->response.addr, ROB_ptr->imm, buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
		}
		stop = 1;
	}
		break;
	case SYSTEM_map: {// Control Register access (fenced operations)
		ROB_ptr->csr = (control_status_reg_type)((buffer >> 20) & 0x0fff);
		ROB_ptr->rs_count = 0;
		if (mhartid == 0)
			debug++;
		switch (ROB_ptr->funct3) {
		case 0: // operating system instructions for use with debugger
			if (ROB->decode_ptr == ROB->retire_ptr_in) {
				if (ROB_ptr->rd != 0 || ROB_ptr->rs1 != 0)
					debug++;
				switch (buffer >> 20) {
				case 0x000: {// ECALL - register push occurs in software; return address in _cause register
					shifter->response.msg = halt;// flush and restart prefetcher
					ROB_ptr->map = SYSTEM_map;
					ROB_ptr->uop = uop_ECALL;
					if (debug_unit) {
						fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
						fprintf(debug_stream, "ECALL Exception - jump to OS or machine code handler; clock: 0x%04llx\n", clock);
					}
					incr_decode32_ptr(ROB, shifter, decode_vars);
					ROB_ptr->state = ROB_execute;
					ROB_ptr = &ROB->q[ROB->decode_ptr];
					stop = 1;
				}
						  shifter->response.fault_in_service = 0;
						  break;
				case 0x001:
					ROB_ptr->uop = uop_EBREAK;
					break;
				case 0x002:
					ROB_ptr->uop = uop_uret;
					break;
				case 0x102: {// uop_SRET
					ROB_ptr->map = SYSTEM_map;
					ROB_ptr->uop = uop_SRET;
					ROB_ptr->rd = 2;//stack pointer
					ROB_ptr->rs_count = 0;
					ROB_ptr->imm = 0;
					ROB_ptr->bytes = 4;
					ROB_ptr->state = ROB_allocate_0;
					if (debug_unit) {
						fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
						fprintf(debug_stream, ",SRET microcode start; clock: 0x%04llx\n", clock);
					}
					incr_decode32_ptr(ROB, shifter, decode_vars);
					ROB_ptr = &ROB->q[ROB->decode_ptr];

					uPC[0] = 4;

					shifter->response.fault_in_service = 0;
					shifter->response.msg = halt;// flush and restart prefetcher

					stop = 1;
				}
						  break;
				case 0x104:
					ROB_ptr->uop = uop_FENCE;
					shifter->response.fault_in_service = 0;
					break;
				case 0x105:
					ROB_ptr->uop = uop_WFI;
					shifter->response.msg = halt;// WFI
					shifter->response.fault_in_service = 0;
					break;
				case 0x202:
					ROB_ptr->uop = uop_hret;
					shifter->response.fault_in_service = 0;
					break;
				case 0x302: {// mret
					UINT PC = 0;
					ROB_ptr = &ROB->q[ROB->decode_ptr];
					ROB_ptr->map = SYSTEM_map;
					ROB_ptr->uop = uop_MRET;
					ROB_ptr->rs_count = 0;
					ROB_ptr->addr = tag;
					ROB_ptr->bytes = 4;
					ROB_ptr->state = ROB_allocate_0;

					shifter->response.fault_in_service = 0;
				}
						  break;
				default:
					debug++;
					break;
				}
			}
			else {
				stop = 1;
			}
			break;
		case 1:// CSRRW
			if (ROB->decode_ptr == ROB->retire_ptr_in) {
				ROB_ptr->map = SYSTEM_map;
				ROB_ptr->q_select = store_q_select;
				ROB_ptr->uop = uop_CSRRW;
				ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
				ROB_ptr->rs_count = 1;
				ROB_ptr->csr = (control_status_reg_type)((buffer >> 20) & 0x0fff);
				ROB_ptr->imm = 0;
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 4;
				ROB_ptr->state = ROB_allocate_0;
			}
			else {
				stop = 1;
			}
			break;
		case 2:// CSRRS
			ROB_ptr->uop = uop_CSRRS;
			ROB_ptr->rs_count = 1;
			break;
		case 3:// CSRRC
			ROB_ptr->uop = uop_CSRRC;
			ROB_ptr->rs_count = 1;
			break;
		case 5:// CSRRWI
			if (ROB->decode_ptr == ROB->retire_ptr_in) {
				ROB_ptr->uop = uop_CSRRWI;
				ROB_ptr->imm = ROB_ptr->rs1;
				ROB_ptr->rs_count = 0;
			}
			else {
				stop = 1;
			}
			break;
		case 6:// CSRRSI
			ROB_ptr->uop = uop_CSRRSI;
			ROB_ptr->imm = ROB_ptr->rs1;
			ROB_ptr->rs_count = 0;
			break;
		case 7:// CSRRCI
			ROB_ptr->uop = uop_CSRRCI;
			ROB_ptr->imm = ROB_ptr->rs1;
			ROB_ptr->rs_count = 0;
			break;
		default:
			break;
		}
		if (!stop) {
			if (debug_unit) {
				fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
				print_uop_decode(debug_stream, ROB_ptr[0], clock);
				fprintf(debug_stream, ", 0x%08x, clock: 0x%04llx\n", buffer, clock);
			}
			incr_decode32_ptr(ROB, shifter, decode_vars);
		}
	}
		break;
	case reserved0_map:
	case reserved1_map:
	case reserved2_map:
	case custom0_map:
	case custom1_map:
	case custom2_map:
	case custom3_map:
	default://coding error
		fprint_decode_header(debug_stream, ROB, ROB_ptr, mhartid, param);
		fprintf(debug_stream, "ERROR: opcode not implemented, 0x%08x, clock: 0x%04llx\n", buffer, clock);
		incr_decode32_ptr(ROB, shifter, decode_vars);
		debug++;
		break;
	}
	return stop;
}

UINT8 decode_32b2(ROB_Type* ROB, UINT8* flush_write_posters, INT buffer, UINT64 tag, shifter_response* response, UINT8 prefetcher_idle, UINT stores_pending, UINT load_pending,
	UINT64 clock, branch_vars* branch, store_buffer_type* store_buffer, UINT* uPC) {
	UINT8 debug = 0;
	UINT8 stop = 0;

	ROB_entry_Type* ROB_ptr = &ROB->q[ROB->decode_ptr];
	if (ROB_ptr->state != ROB_free)
		debug++;

	UINT8 funct2 = (buffer >> 25) & 3; // float size: 0-S,1-D, 2-reserved, 3-Q(128 bit)
	UINT8 funct7 = (buffer >> 25) & 0x7f;

	ROB_ptr->addr = tag;
	ROB_ptr->map = (opcode_map)((buffer >> 2) & 0x1f);
	ROB_ptr->funct3 = ((buffer >> 12) & 0x07);

	ROB_ptr->rd = (buffer >> 7) & 0x1f;
	ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
	ROB_ptr->rs2 = (buffer >> 20) & 0x1f;
	ROB_ptr->rs3 = (buffer >> 27) & 0x1f;

	ROB_ptr->branch_num = ROB->branch_stop;
	ROB_ptr->bytes = 4;
	switch (ROB_ptr->map) {
	case LOAD_map: {// load
		ROB_ptr->q_select = load_q_select;
		ROB_ptr->uop = uop_LOAD;
		ROB_ptr->reg_type = (ROB_ptr->funct3 >> 2) & 1;
		ROB_ptr->imm = (buffer >> 20);
	}
				 break;
	case LOAD_FP_map: {//FLW; with SIMD extension
		ROB_ptr->q_select = load_q_select;
		ROB_ptr->uop = uop_F_LOAD;
		ROB_ptr->reg_type = 2;
		ROB_ptr->rd = (buffer >> 7) & 0x1f;
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->imm = (buffer >> 20);
	}
					break;
	case MISC_MEM_map: {//fence - parameters??? 
		switch (ROB_ptr->funct3) {
		case 0:
		case 1: {
//			stop |= fence_check(ROB_ptr, flush_write_posters, ROB, csr, prefetcher_idle, 0, stores_pending, clock, store_buffer, mhartid, debug_unit, param, debug_stream);
			if (ROB->decode_ptr == ROB->retire_ptr_in || ROB->q[ROB->retire_ptr_in].state == ROB_fault) {// synchronize instruction execution

				UINT load_pending = 0;
				UINT q_id;
				for (q_id = 0; q_id < 0x100 && !load_pending; q_id++) {
					if (ROB->q[q_id].map == LOAD_map && ROB->q[q_id].state == ROB_inflight)
						load_pending = 1;
				}
				q_id--;
				// need to chack for prefetcher xaction pending
				if (load_pending) {// need to flush load buffers
					stop = 1;
				}
				else if (stores_pending) {// need to flush load buffers
					flush_write_posters[0] = 1;
					stop = 1;
				}
				else if (!prefetcher_idle) {
					stop = 1;
				}
				else {
					ROB_ptr->q_select = store_q_select;
					ROB_ptr->map = MISC_MEM_map;
					ROB_ptr->uop = uop_FENCE;// flush write posters
					ROB_ptr->branch_num = ROB->branch_start = ROB->branch_stop;
					ROB_ptr->state = ROB_allocate_0;
				}
			}
			else {
				stop = 1;
			}

			if (!stop) {
				switch (ROB_ptr->funct3) {// bit 0 = I cache, 1= D cache, 2 = L2 cache
				case 0: // fence
					break;
				case 1: // Zifencei - flush instruction cache as well
					break;
				default:
					break;
				}
			}
		}
			  break;
		case 2:
		case 3: {
			ROB_ptr->q_select = load_q_select;
			ROB_ptr->map = LOAD_map;
			ROB_ptr->uop = uop_LOAD;
			ROB_ptr->reg_type = ROB_ptr->funct3 & 1;
			ROB_ptr->rd = (buffer >> 7) & 0x1f;
			ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
			ROB_ptr->imm = (buffer >> 20);
			ROB_ptr->funct3 = (ROB_ptr->reg_type) ? 12 : 8;
		}
			  break;
		default:
			debug++;
			break;
		}
	}
					 break;
	case OP_IMM_map: // Integer op rs1 to imm
	{
		ROB_ptr->q_select = (q_select_type)(OP_q_select0);
		//		ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
		//		decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));

		ROB_ptr->rs_count = 1;
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->imm = (buffer >> 20);//signed 

		ROB_ptr->reg_type = 0;
		switch (ROB_ptr->funct3) {
		case 0: //ADDI
			ROB_ptr->uop = uop_ADDI;
			if (ROB_ptr->rd == 0 && ROB_ptr->rs1 == 0 && ROB_ptr->imm == 0)
				ROB_ptr->uop = uop_NOP;
			break;
		case 1: // SLLI - shift logical left
			ROB_ptr->uop = uop_SLLI;
			break;
		case 2: { //SLTI signed compare less than
			ROB_ptr->uop = uop_SLTI;
			break;
		}
		case 3: { //SLTIU
			ROB_ptr->imm = (buffer >> 20) & 0x0fff;//unsigned 
			ROB_ptr->uop = uop_SLTIU;
			ROB_ptr->reg_type = 1;
			break;
		}
		case 4: { //XORI
			ROB_ptr->uop = uop_XORI;
			break;
		}
		case 5: // SRLI/SRAI
			ROB_ptr->rs2 = ((buffer >> 20) & 0x1f); // shamt - shift amount

			ROB_ptr->uop = ((buffer >> 27) & 8) ? uop_SRAI : uop_SRLI;
			break;
		case 6: //ORI
			ROB_ptr->uop = uop_ORI;
			break;
		case 7: //ANDI
			ROB_ptr->uop = uop_ANDI;
			break;
		default: // error
			debug++;
			break;
		}
	}
	break;
	case AUIPC_map://AUIPC - load PC relative address for 32b unconditional jump
		//		ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
		ROB_ptr->q_select = (q_select_type)(OP_q_select0);
		//		decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));
		ROB_ptr->uop = uop_AUIPC;
		ROB_ptr->imm = buffer & 0xfffff000;
		ROB_ptr->rs_count = 0;
		break;
	case OP_IMM64_map: // Integer op rs1 to imm
	{
		//		ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
		ROB_ptr->q_select = (q_select_type)(OP_q_select0);
		//		decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));

		ROB_ptr->rs_count = 1;
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->imm = (buffer >> 20);//signed 

		ROB_ptr->reg_type = 0;
		switch (ROB_ptr->funct3) {
		case 0: //ADDI
			ROB_ptr->uop = uop_ADDI;
			if (ROB_ptr->rd == 0 && ROB_ptr->rs1 == 0 && ROB_ptr->imm == 0)
				ROB_ptr->uop = uop_NOP;
			break;
		case 1: // SLLI - shift logical left
			ROB_ptr->uop = uop_SLLI;
			break;
		case 2: { //SLTI signed compare less than
			ROB_ptr->uop = uop_SLTI;
			break;
		}
		case 3: { //SLTIU
			ROB_ptr->imm = (buffer >> 20) & 0x0fff;//unsigned 
			ROB_ptr->uop = uop_SLTIU;
			ROB_ptr->reg_type = 1;
			break;
		}
		case 4: { //XORI
			ROB_ptr->uop = uop_XORI;
			break;
		}
		case 5: // SRLI/SRAI
			ROB_ptr->rs2 = ((buffer >> 20) & 0x1f); // shamt - shift amount

			ROB_ptr->uop = ((buffer >> 27) & 8) ? uop_SRAI : uop_SRLI;
			break;
		case 6: //ORI
			ROB_ptr->uop = uop_ORI;
			break;
		case 7: //ANDI
			ROB_ptr->uop = uop_ANDI;
			break;
		default: // error
			debug++;
			break;
		}
	}
	break;
	case STORE_map: {// store
		ROB_ptr->q_select = store_q_select;
		ROB_ptr->uop = uop_STORE;
		ROB_ptr->imm = ((INT)(buffer & 0xfe000000) >> 20) | ((buffer >> 7) & 0x1f);
		ROB_ptr->rd = 0;
		ROB_ptr->rd_ptr = 0;
	}
				  break;
	case STORE_FP_map: {//FSW: floating point store with SIMD extension
		ROB_ptr->q_select = store_q_select;
		ROB_ptr->uop = uop_F_STORE;
		ROB_ptr->imm = ((INT)(buffer & 0xfe000000) >> 20) | ((buffer >> 7) & 0x1f);
		ROB_ptr->rd = 0;
		ROB_ptr->rd_ptr = 0;
	}
					 break;
	case AMO_map: { // atomic instructions - serialize bus, not core
		ROB_ptr->q_select = store_q_select;
		if (ROB_ptr->funct3 == 2) {// 32b
			//	xxx.W
		}
		else if (ROB_ptr->funct3 == 3) { // 64b
			// xxx.D
		}
		ROB_ptr->imm = buffer >> 25;
		UINT8 width = ROB_ptr->funct3;
		switch ((buffer >> 27) & 0x1f) {
		case 0://AMOADD.W
			break;
		case 1: // AMOSWAP.W
			break;
		case 2:// LR.W
			ROB_ptr->uop = uop_LR;
			break;
		case 3:// SC.W
			ROB_ptr->uop = uop_SC;
			break;
		case 4:// AMOXOR.W
			break;
		case 0x08:// AMOOR.W
			break;
		case 0x0c:// AMOAND.W
			break;
		case 0x10: // AMOMIN.W
			break;
		case 0x14: // AMOMAX.W
			break;
		case 0x18: // AMOMINU.W
			break;
		case 0x1C: // AMOMAXU.W
			break;
		default:
			break;
		}
	}
				break;
	case OP_map:// 32b
	case OP_64_map: {// 64b
		ROB_ptr->rs_count = 2;
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->rs2 = (buffer >> 20) & 0x1f;

		ROB_ptr->reg_type = 0;

		if (funct7 != 0 && funct7 != 0x20)
			debug++;

		if (((buffer >> 25) & 1) == 0) {
			ROB_ptr->q_select = (q_select_type)(OP_q_select0);
			//			ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
			//			decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));

			switch (ROB_ptr->funct3) {
			case 0: //ADD/SUB
				if ((buffer >> 27) & 8) {// 1=sub, 0=add
					ROB_ptr->uop = uop_SUB;
				}
				else {
					ROB_ptr->uop = uop_ADD;
				}
				break;
			case 1: //SLL - shift logical left
				ROB_ptr->uop = uop_SLL;
				break;
			case 2: //SLT - less than immediate
				ROB_ptr->uop = uop_SLT;
				break;
			case 3: //SLTU - less than immediate unsigned
				ROB_ptr->uop = uop_SLTU;
				break;
			case 4: //XOR
				ROB_ptr->uop = uop_XOR;
				break;
			case 5: //SRL/SRA	

				if ((buffer >> 27) & 8) {
					ROB_ptr->uop = uop_SRA;
				}
				else {
					ROB_ptr->uop = uop_SRL;
				}
				break;
			case 6: //OR
				ROB_ptr->uop = uop_OR;
				break;
			case 7: //AND
				ROB_ptr->uop = uop_AND;
				break;
			default: // error
				break;
			}
		}
		else {
			ROB_ptr->q_select = iMUL_q_select;
			switch (ROB_ptr->funct3) {
			case 0://mul
				ROB_ptr->uop = uop_MUL;
				break;
			case 1://MULH
				break;
			case 2://MULHSU
				break;
			case 3://MULHU
				break;
			case 4://div
				ROB_ptr->uop = uop_DIV;
				break;
			case 5://divu
				ROB_ptr->uop = uop_DIVU;
				break;
			case 6://rem
				ROB_ptr->uop = uop_REM;
				break;
			case 7://remu
				ROB_ptr->uop = uop_REMU;
				break;
			default:
				debug++;
				break;
			}
		}
	}
				  break;
	case LUI_map: {//LUI - 
		ROB_ptr->q_select = (q_select_type)(OP_q_select0);
		//		ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
		//		decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));
		ROB_ptr->uop = uop_LUI;// unconditional load immediate bits 31:12; use ADDI to load bits 11-0 for a full 32b immediate load
		ROB_ptr->imm = buffer & 0xfffffffffffff000;
		ROB_ptr->rs_count = 0;
	}
				break;
	case MADD_map: {
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->rs2 = (buffer >> 20) & 0x1f;
		ROB_ptr->rs3 = (buffer >> 27) & 0x1f;
		ROB_ptr->rs_count = 3;
		ROB_ptr->funct3 = (buffer >> 12) & 0x07;//rounding mode
		ROB_ptr->funct7 = (buffer >> 25) & 0x7f;// size
		ROB_ptr->q_select = op_fp_select;
		ROB_ptr->uop = uop_FMADD;
	}
				 break;
	case MSUB_map: { //FMSUB - 3 entry
		debug++;
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->rs2 = (buffer >> 20) & 0x1f;
		ROB_ptr->rs3 = (buffer >> 27) & 0x1f;
		ROB_ptr->rs_count = 3;
		ROB_ptr->funct3 = (buffer >> 12) & 0x07;//rounding mode
		ROB_ptr->funct7 = (buffer >> 25) & 0x7f;// size
		ROB_ptr->q_select = op_fp_select;
		ROB_ptr->uop = uop_FMSUB;
	}
				 break;
	case NMSUB_map: {//FNMSUB - 3 entry
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->rs2 = (buffer >> 20) & 0x1f;
		ROB_ptr->rs3 = (buffer >> 27) & 0x1f;
		ROB_ptr->rs_count = 3;
		ROB_ptr->funct3 = (buffer >> 12) & 0x07;//rounding mode
		ROB_ptr->funct7 = (buffer >> 25) & 0x7f;// size
		ROB_ptr->q_select = op_fp_select;
		ROB_ptr->uop = uop_FNMSUB;
	}
				  break;
	case NMADD_map: { //FNMADD - 3 entry
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->rs2 = (buffer >> 20) & 0x1f;
		ROB_ptr->rs3 = (buffer >> 27) & 0x1f;
		ROB_ptr->rs_count = 3;
		ROB_ptr->funct3 = (buffer >> 12) & 0x07;//rounding mode
		ROB_ptr->funct7 = (buffer >> 25) & 0x7f;// size
		ROB_ptr->q_select = op_fp_select;
		ROB_ptr->uop = uop_FNMADD;
	}
				  break;
	case OP_FP_map: {//float
		ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
		ROB_ptr->rs2 = (buffer >> 20) & 0x1f;
		ROB_ptr->funct3 = (buffer >> 12) & 0x07;
		ROB_ptr->funct7 = (buffer >> 25) & 0x7f;// need to decode partially - which q

		ROB_ptr->rs_count = 2;
		char fp_size;
		switch (ROB_ptr->funct7 & 3 == 0) {
		case 0:
			fp_size = 's';//32b
			break;
		case 1:
			fp_size = 'd';//64b
			break;
		case 3:
			fp_size = 'q';//128b
			break;
		case 2:
			fp_size = 'h';//16b
			break;
		default:
			debug++;
			break;
		}
		switch ((buffer >> 27) & 0x1f) {
		case 0:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FADD;
			break;
		case 1:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FSUB;
			break;
		case 2:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FMUL;
			break;
		case 3:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FDIV;
			break;
		case 4:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FSGN;
			break;
		case 5:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = (buffer & 0x00001000) ? uop_FMAX : uop_FMIN;
			break;
		case 0x18:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FCVTi2f;// int to float
			break;
		case 0x1a:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FCVTf2i;//float to int
			break;
		case 0x1c:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FMVi2f;
			break;
		case 0x1e:
			ROB_ptr->q_select = op_fp_select;
			ROB_ptr->uop = uop_FMVf2i;//float to int
			break;
		default:
			debug++;
			break;
		}
	}
				  break;
	case BRANCH_map:{ // branch conditional (4KB range) - only 1 per decode block 
		ROB_ptr->rd = 0;
		ROB_ptr->imm = ((buffer >> 31) & ~0x0eff) | (((buffer << 4) & 0x800) | ((buffer >> 20) & 0x7e0) | ((buffer >> 7) & 0x1e));
		// error: need bit for prediction value
		if (branch->count[ROB_ptr->addr & 0xfff] == branch->total[ROB_ptr->addr & 0xfff] || branch->count[ROB_ptr->addr & 0xfff] == 0) {
			if (branch->pred[ROB_ptr->addr & 0xfff])
				ROB_ptr->rs3 = 3; // branches taken
			else
				ROB_ptr->rs3 = 1; // branches not taken
		}
		else {
			if (branch->pred[ROB_ptr->addr & 0xfff])
				ROB_ptr->rs3 = 3; // branches taken
			else
				ROB_ptr->rs3 = 1; // branches not taken
		}
		ROB_ptr->branch_num = ROB->branch_stop;
		ROB_ptr->rs_count = 2;
		ROB->branch_stop = ((ROB->branch_stop + 1) & 3);
		switch (ROB_ptr->funct3) {// decode funct3 in branch unit, not here
		case 0:// BEQ 
			ROB_ptr->uop = uop_BEQ;
			break;
		case 1://BNE
			ROB_ptr->uop = uop_BNE;
			break;
		case 4://BLT
			ROB_ptr->uop = uop_BLT;
			break;
		case 5:// BGE
			ROB_ptr->uop = uop_BGE;
			break;
		case 6:// BLTU
			ROB_ptr->uop = uop_BLTU;
			break;
		case 7:// BGEU
			ROB_ptr->uop = uop_BGEU;
			break;
		default: // error
			break;
		}
		response->msg = (branch_response)ROB_ptr->rs3;
		if (ROB_ptr->rs3 == 3) {
			response->msg = taken;
		}
		response->addr = ROB_ptr->imm + ROB_ptr->addr;
		//		}
		stop = 1;
	}
	// NOTE: need to have transaction queue and match addresses in case of dropped prefetch - do not latch data.
	break;
	case JALR_map:// JALR - unconditional jump and link register (32b)
			response->msg = halt;// flush and restart prefetcher
			if ((ROB_ptr->rd == 1 || ROB_ptr->rd == 5) && (ROB_ptr->rs1 == 1 || ROB_ptr->rs1 == 5)) { // return to caller	

				ROB_ptr->q_select = (q_select_type)(OP_q_select0);// s/b branch unit!!!
				//				ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
				//				decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));

				ROB_ptr->map = OP_IMM64_map;
				ROB_ptr->uop = uop_ADDI; // copy RA to reg 31
				ROB_ptr->rd = 31;
				ROB_ptr->rs1 = 1;// RA
				ROB_ptr->rs_count = 1;
				ROB_ptr->funct3 = 3;
				ROB_ptr->imm = 0;
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				ROB_ptr->state = ROB_allocate_0;
				ROB_ptr = &ROB->q[ROB->decode_ptr];

				ROB_ptr = &ROB->q[ROB->decode_ptr];

				ROB_ptr->q_select = load_q_select;
				ROB_ptr->map = LOAD_map;
				ROB_ptr->uop = uop_LOAD;
				ROB_ptr->rd = 1;
				ROB_ptr->rs1 = 2;// stack pointer
				ROB_ptr->funct3 = 3;
				ROB_ptr->imm = -8;
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				//							ROB_ptr->instr_count = 1;
				ROB_ptr->state = ROB_allocate_0;
				ROB_ptr = &ROB->q[ROB->decode_ptr];

				ROB_ptr->q_select = load_q_select;
				ROB_ptr->map = LOAD_map;
				ROB_ptr->uop = uop_LOAD;
				ROB_ptr->rd = 3;
				ROB_ptr->rs1 = 2;// stack pointer
				ROB_ptr->funct3 = 3;
				ROB_ptr->imm = -0x18;
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				//							ROB_ptr->instr_count = 1;
				ROB_ptr->state = ROB_allocate_0;
				ROB_ptr = &ROB->q[ROB->decode_ptr];

				ROB_ptr->q_select = load_q_select;
				ROB_ptr->map = LOAD_map;
				ROB_ptr->uop = uop_LOAD;
				ROB_ptr->rd = 4;
				ROB_ptr->rs1 = 2;// stack pointer
				ROB_ptr->funct3 = 3;
				ROB_ptr->imm = -0x20;
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				//							ROB_ptr->instr_count = 1;
				ROB_ptr->state = ROB_allocate_0;
				ROB_ptr = &ROB->q[ROB->decode_ptr];
				ROB_ptr->q_select = load_q_select;
				ROB_ptr->map = LOAD_map;
				ROB_ptr->uop = uop_LOAD;
				ROB_ptr->rd = 2;
				ROB_ptr->rs1 = 2;// stack pointer
				ROB_ptr->funct3 = 3;
				ROB_ptr->imm = -0x10;
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				ROB_ptr->state = ROB_allocate_0;
				ROB_ptr = &ROB->q[ROB->decode_ptr];
				ROB_ptr->rd = (buffer >> 7) & 0x1f;
				//							ROB_ptr->rs1 = (buffer >> 15) & 0x1f;

//						ROB_ptr->q_select = none_q_select;
//				ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
//				ROB_ptr->q_select = (q_select_type)(OP_q_select0);// s/b branch unit
				ROB_ptr->map = JALR_map;
				ROB_ptr->uop = uop_JALR;
				ROB_ptr->imm = buffer >> 20;
				ROB_ptr->rs_count = 1;
				ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
				response->msg = halt;// flush and restart prefetcher

				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 4;
				//							ROB_ptr->instr_count = 1;
				ROB_ptr->state = ROB_allocate_0;
				//				decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));
			}
			else if (ROB_ptr->rd == 1 || ROB_ptr->rd == 5) {// load up TLB's with copy, be able to detect page walk
				ROB_ptr->map = SYSTEM_map;
				ROB_ptr->uop = uop_HALT;// halt prefetching, wait for JALR
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				ROB_ptr->rs_count = 0;
				ROB_ptr->state = ROB_allocate_0;
				ROB_ptr = &ROB->q[ROB->decode_ptr];

				// disable interrupts and save previous state - hardwired to occur immediately; not disturb reg bank
//				csr[csr_mstatus].value = ~0x80;
//				csr[csr_mstatus].value = (csr[csr_mstatus].value << 4) & 0x80;
//				csr[csr_mstatus].value = ~0x08;
				ROB_ptr->q_select = store_q_select;
				ROB_ptr->map = STORE_map;
				ROB_ptr->uop = uop_STORE;
				ROB_ptr->rs1 = 2;// stack pointer
				ROB_ptr->rs2 = 1;
				ROB_ptr->imm = -4;
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				//							ROB_ptr->instr_count = 1;
				ROB_ptr->state = ROB_allocate_0;
				ROB_ptr = &ROB->q[ROB->decode_ptr];

				ROB_ptr->q_select = store_q_select;
				ROB_ptr->map = STORE_map;
				ROB_ptr->uop = uop_STORE;
				ROB_ptr->rs1 = 2;// stack pointer
				ROB_ptr->rs2 = 2;
				ROB_ptr->imm = -8;// stack pointer
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				//							ROB_ptr->instr_count = 1;
				ROB_ptr->state = ROB_allocate_0;
				ROB_ptr = &ROB->q[ROB->decode_ptr];

				ROB_ptr->q_select = store_q_select;
				ROB_ptr->map = STORE_map;
				ROB_ptr->uop = uop_STORE;
				ROB_ptr->rs1 = 2;// stack pointer
				ROB_ptr->rs2 = 3;
				ROB_ptr->imm = -12;// stack pointer
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				//							ROB_ptr->instr_count = 1;
				ROB_ptr->state = ROB_allocate_0;
				ROB_ptr = &ROB->q[ROB->decode_ptr];

				ROB_ptr->q_select = store_q_select;
				ROB_ptr->map = STORE_map;
				ROB_ptr->uop = uop_STORE;
				ROB_ptr->rs1 = 2;// stack pointer
				ROB_ptr->rs2 = 4;
				ROB_ptr->imm = -16;// stack pointer
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				//							ROB_ptr->instr_count = 1;
				ROB_ptr->state = ROB_allocate_0;
				ROB_ptr = &ROB->q[ROB->decode_ptr];

				ROB_ptr->map = SYSTEM_map;
				ROB_ptr->uop = uop_CSRRWI;
				ROB_ptr->rd = (buffer >> 15) & 0x1f;
				ROB_ptr->rs_count = 0;
				ROB_ptr->imm = 0;
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 0;
				ROB_ptr->state = ROB_allocate_0;
				ROB_ptr = &ROB->q[ROB->decode_ptr];

				ROB_ptr->rd = (buffer >> 7) & 0x1f;
				ROB_ptr->rs1 = (buffer >> 15) & 0x1f;

				//				ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
//				ROB_ptr->q_select = (q_select_type)(OP_q_select0);

				ROB_ptr->map = JALR_map;
				ROB_ptr->uop = uop_JALR;
				ROB_ptr->imm = buffer >> 20;
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 4;
				ROB_ptr->rs_count = 1;
				stop = 1;
				ROB_ptr->state = ROB_allocate_0;
				response->msg = halt;// flush and restart prefetcher

				//				decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));
				ROB_ptr->rs1 = (buffer >> 15) & 0x1f;

				//				ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
	//			ROB_ptr->q_select = (q_select_type)(OP_q_select0);
				ROB_ptr->map = JALR_map;
				ROB_ptr->uop = uop_JALR;
				ROB_ptr->imm = buffer >> 20;
				ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
				ROB_ptr->rs_count = 1;

				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 4;
				ROB_ptr->state = ROB_allocate_0;
				//				decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));
			}
			else {
				ROB_ptr->imm = buffer >> 20;
				ROB_ptr->rs1 = (buffer >> 15) & 0x1f;

				//				ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
	//			ROB_ptr->q_select = (q_select_type)(OP_q_select0);
				ROB_ptr->map = JALR_map;
				ROB_ptr->uop = uop_JALR;
				ROB_ptr->imm = buffer >> 20;
				ROB_ptr->rs_count = 1;
				response->msg = halt;// flush and restart prefetcher

				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 4;
				ROB_ptr->state = ROB_allocate_0;
				//			decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));
				response->msg = halt;
			}
		stop = 1;
		break;
	case JAL_map:// JAL - unconditional jump to immediate address (20b)
		if (ROB_ptr->rd == 1 || ROB_ptr->rd == 5) {
			ROB_ptr->q_select = store_q_select;
			ROB_ptr->map = STORE_map;
			ROB_ptr->uop = uop_STORE;
			ROB_ptr->rs1 = 2;// stack pointer
			ROB_ptr->rs2 = 1;
			ROB_ptr->funct3 = 3;
			ROB_ptr->imm = -8;
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			ROB_ptr->state = ROB_allocate_0;
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			ROB_ptr->q_select = store_q_select;
			ROB_ptr->map = STORE_map;
			ROB_ptr->uop = uop_STORE;
			ROB_ptr->rs1 = 2;// stack pointer
			ROB_ptr->rs2 = 2;
			ROB_ptr->funct3 = 3;
			ROB_ptr->imm = -16;// stack pointer
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			//						ROB_ptr->instr_count = 1;
			ROB_ptr->state = ROB_allocate_0;
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			ROB_ptr->q_select = store_q_select;
			ROB_ptr->map = STORE_map;
			ROB_ptr->uop = uop_STORE;
			ROB_ptr->rs1 = 2;// stack pointer
			ROB_ptr->rs2 = 3;
			ROB_ptr->funct3 = 3;
			ROB_ptr->imm = -24;// stack pointer
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			//						ROB_ptr->instr_count = 1;
			ROB_ptr->state = ROB_allocate_0;
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			ROB_ptr->q_select = store_q_select;
			ROB_ptr->map = STORE_map;
			ROB_ptr->uop = uop_STORE;
			ROB_ptr->rs1 = 2;// stack pointer
			ROB_ptr->rs2 = 4;
			ROB_ptr->funct3 = 3;
			ROB_ptr->imm = -32;// stack pointer
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			//						ROB_ptr->instr_count = 1;
			ROB_ptr->state = ROB_allocate_0;
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			//					ROB_ptr->q_select = store_q_select;
			ROB_ptr->map = SYSTEM_map;
			ROB_ptr->uop = uop_CSRRCI;
			ROB_ptr->rd = 1;
			ROB_ptr->rs_count = 0;
			ROB_ptr->imm = 0;
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			ROB_ptr->state = ROB_allocate_0;
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			//						ROB_ptr->q_select = (q_select_type)(OP_q_select0 | (decode_vars->index & (param->decode_width - 1)));
//				ROB_ptr->q_select = (q_select_type)(OP_q_select0 | decode_vars->int_index);
			ROB_ptr->q_select = (q_select_type)(OP_q_select0);
			//				decode_vars->int_index = ((decode_vars->int_index + 1) & (param->decode_width - 1));

			ROB_ptr->map = OP_IMM64_map;
			ROB_ptr->uop = uop_ADDI;
			ROB_ptr->rd = 1;
			ROB_ptr->rs1 = 1;
			ROB_ptr->imm = 4;
			ROB_ptr->rs_count = 1;
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 0;
			ROB_ptr->state = ROB_allocate_0;
			ROB_ptr = &ROB->q[ROB->decode_ptr];

			ROB_ptr->rd = (buffer >> 7) & 0x1f;

			ROB_ptr->q_select = none_q_select;
			ROB_ptr->map = JAL_map;
			ROB_ptr->uop = uop_JAL;
			ROB_ptr->imm = buffer >> 20;
			ROB_ptr->addr = tag;
			ROB_ptr->bytes = 4;
			//						ROB_ptr->instr_count = 1;
			ROB_ptr->rs_count = 0;

			ROB_ptr->imm = buffer & 0xff000;
			ROB_ptr->imm |= ((buffer >> 9) & 0x800);
			ROB_ptr->imm |= ((buffer >> 20) & 0x7fe);
			ROB_ptr->imm |= ((buffer >> 11) & 0xfff00000);

			response->msg = unconditional;
			response->addr = ROB_ptr->imm + tag;

			ROB_ptr->state = ROB_allocate_0;
		}
		else {
			ROB_ptr->map = JAL_map;
			ROB_ptr->uop = uop_JAL;
			ROB_ptr->q_select = none_q_select;
			ROB_ptr->rs_count = 0;

			ROB_ptr->imm = buffer & 0xff000;
			ROB_ptr->imm |= ((buffer >> 9) & 0x800);
			ROB_ptr->imm |= ((buffer >> 20) & 0x7fe);
			ROB_ptr->imm |= ((buffer >> 11) & 0xfff00000);

			ROB_ptr->addr = tag;

			response->msg = unconditional;
			response->addr = ((INT64)ROB_ptr->imm) + tag;

			ROB_ptr->state = ROB_retire_out;
		}
		stop = 1;
		break;
	case SYSTEM_map: {// Control Register access (fenced operations)
		ROB_ptr->csr = (control_status_reg_type)((buffer >> 20) & 0x0fff);
		ROB_ptr->rs_count = 0;
		switch (ROB_ptr->funct3) {
		case 0: // operating system instructions for use with debugger
			if (ROB->decode_ptr == ROB->retire_ptr_in) {
				if (ROB_ptr->rd != 0 || ROB_ptr->rs1 != 0)
					debug++;
				switch (buffer >> 20) {
				case 0x000: {// ECALL - register push occurs in software; return address in _cause register
					response->msg = unconditional;
					/*
					switch (retire->priviledge) {
					case 0:
						csr[csr_mcause].value |= 0x00000100;										// set bit 13, instruction page fault
						csr[csr_scause].value |= 0x00000100;

						retire->priviledge = 1;  //supervisor mode
						csr[0x141].value = csr[csr_stvec].value;
						response->addr = csr[csr_stvec].value;
						break;
					case 1:
						csr[csr_mcause].value |= 0x00000200;										// set bit 13, instruction page fault
						csr[csr_scause].value |= 0x00000200;

						retire->priviledge = 1;  //supervisor mode
						csr[0x141].value = csr[csr_stvec].value;
						response->addr = csr[csr_stvec].value;
						break;
					case 2:
						csr[csr_mcause].value |= 0x00000200;										// set bit 13, instruction page fault
						csr[csr_scause].value |= 0x00000200;

						retire->priviledge = 2;  //supervisor mode
						csr[0x241].value = csr[csr_mtvec].value;
						response->addr = csr[csr_mtvec].value;
						break;
					case 3:
						csr[csr_mcause].value |= 0x00000200;										// set bit 13, instruction page fault
						csr[csr_scause].value |= 0x00000200;

						retire->priviledge = 3;  //supervisor mode
						csr[0x341].value = csr[csr_mtvec].value;
						response->addr = csr[csr_mtvec].value;
						break;
					default:
						debug++;
						break;
					}
					/**/
					ROB_ptr->uop = uop_ECALL;
					stop = 1;
				}
						  response->fault_in_service = 0;
						  break;
				case 0x001:
					ROB_ptr->uop = uop_EBREAK;
					break;
				case 0x002:
					ROB_ptr->uop = uop_uret;
					break;
				case 0x102: {// uop_SRET
					ROB_ptr->map = SYSTEM_map;
					ROB_ptr->uop = uop_SRET;
					ROB_ptr->rd = 2;//stack pointer
					ROB_ptr->rs_count = 0;
					ROB_ptr->imm = 0;
					ROB_ptr->bytes = 4;
					ROB_ptr->state = ROB_allocate_0;
					ROB_ptr = &ROB->q[ROB->decode_ptr];

					uPC[0] = 4;
//					SRET_addr[0] = tag;

					response->fault_in_service = 0;
					response->msg = halt;// flush and restart prefetcher
					stop = 1;
				}
						  break;
				case 0x104:
					ROB_ptr->uop = uop_FENCE;
					response->fault_in_service = 0;
					break;
				case 0x105:
					ROB_ptr->uop = uop_WFI;
					response->msg = halt;// WFI
//					retire->priviledge = 0; // open up to all interrupts 
					response->fault_in_service = 0;
					break;
				case 0x202:
					ROB_ptr->uop = uop_hret;
					response->fault_in_service = 0;
					break;
				case 0x302: {// mret
					UINT PC = 0;
					ROB_ptr = &ROB->q[ROB->decode_ptr];
					ROB_ptr->map = SYSTEM_map;
					ROB_ptr->uop = uop_MRET;
					ROB_ptr->rs_count = 0;
					ROB_ptr->addr = tag;
					ROB_ptr->bytes = 4;
					ROB_ptr->state = ROB_allocate_0;

					response->fault_in_service = 0;
				}
						  break;
				default:
					debug++;
					break;
				}
			}
			else {
				stop = 1;
			}
			break;
		case 1:// CSRRW
			if (ROB->decode_ptr == ROB->retire_ptr_in) {
				ROB_ptr->map = SYSTEM_map;
				ROB_ptr->q_select = store_q_select;
				ROB_ptr->uop = uop_CSRRW;
				ROB_ptr->rs1 = (buffer >> 15) & 0x1f;
				ROB_ptr->rs_count = 1;
				ROB_ptr->csr = (control_status_reg_type)((buffer >> 20) & 0x0fff);
				ROB_ptr->imm = 0;
				ROB_ptr->addr = tag;
				ROB_ptr->bytes = 4;
				ROB_ptr->state = ROB_allocate_0;
			}
			else {
				stop = 1;
			}
			break;
		case 2:// CSRRS
			ROB_ptr->uop = uop_CSRRS;
			ROB_ptr->rs_count = 1;
			break;
		case 3:// CSRRC
			ROB_ptr->uop = uop_CSRRC;
			ROB_ptr->rs_count = 1;
			break;
		case 5:// CSRRWI
			if (ROB->decode_ptr == ROB->retire_ptr_in) {
				ROB_ptr->uop = uop_CSRRWI;
				ROB_ptr->imm = ROB_ptr->rs1;
				ROB_ptr->rs_count = 0;
			}
			else {
				stop = 1;
			}
			break;
		case 6:// CSRRSI
			ROB_ptr->uop = uop_CSRRSI;
			ROB_ptr->imm = ROB_ptr->rs1;
			ROB_ptr->rs_count = 0;
			break;
		case 7:// CSRRCI
			ROB_ptr->uop = uop_CSRRCI;
			ROB_ptr->imm = ROB_ptr->rs1;
			ROB_ptr->rs_count = 0;
			break;
		default:
			break;
		}
	}
				   break;
				   //	case n64b_map:
	case reserved0_map:
	case reserved1_map:
	case reserved2_map:
	case custom0_map:
	case custom1_map:
	case custom2_map:
	case custom3_map:
	default://coding error
		debug++;
		break;
	}
	return stop;
}
