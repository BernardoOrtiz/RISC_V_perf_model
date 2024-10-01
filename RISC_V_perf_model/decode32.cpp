// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

//#include "ROB.h"
#include "front_end.h"

void fprint_csr(FILE* debug_stream,control_status_reg_type csr ) {
	switch (csr) {
	case csr_uepc:
		fprintf(debug_stream, "csr: uepc, ");
		break;
	case csr_sepc:
		fprintf(debug_stream, "csr: sepc, ");
		break;
	case csr_mepc:
		fprintf(debug_stream, "csr: mepc, ");
		break;
	case csr_sedeleg:
		fprintf(debug_stream, "csr: sedeleg, ");
		break;
	case csr_medeleg:
		fprintf(debug_stream, "csr: medeleg, ");
		break;
	case csr_sideleg:
		fprintf(debug_stream, "csr: sideleg, ");
		break;
	case csr_mideleg:
		fprintf(debug_stream, "csr: mideleg, ");
		break;
	case csr_satp:
		fprintf(debug_stream, "csr: satp, "); // supervisor address translation pointer: points as base of top level page directory table
		break;
	case csr_mie:
		fprintf(debug_stream, "csr: mie, ");
		break;
	case csr_stvec:// supervisor trap vector
		fprintf(debug_stream, "csr: stvec, ");
		break;
	case csr_mtvec:// machine trap vector
		fprintf(debug_stream, "csr: mtvec, ");
		break;
	case csr_mtval:// machine trap value - offending address in page faults
		fprintf(debug_stream, "csr: mtval, ");
		break;
	case csr_scause:// interrupt cause
		fprintf(debug_stream, "csr: stval, ");
		break;
	case csr_stval:// machine trap value - offending address in page faults
		fprintf(debug_stream, "csr: stval, ");
		break;
	case csr_mscratch:
		fprintf(debug_stream, "csr: mscratch, ");
		break;
	case csr_sscratch:
		fprintf(debug_stream, "csr: sscratch, ");
		break;
	case csr_mstatus:
		fprintf(debug_stream, "csr: mstatus, ");
		break;
	case csr_mcause:
		fprintf(debug_stream, "csr: mcause, ");
		break;
	case csr_mhartid:
		fprintf(debug_stream, "csr: mhartid, ");
		break;
	case csr_iobase:
		fprintf(debug_stream, "csr: iobase, ");
		break;
	case csr_mbound:
		fprintf(debug_stream, "csr: mbound, ");
		break;
	case csr_sbound:
		fprintf(debug_stream, "csr: sbound, ");
		break;
//	case csr_msp:
//		fprintf(debug_stream, "csr: msp, ");
//		break;
//	case csr_ssp:
//		fprintf(debug_stream, "csr: ssp, ");
//		break;
	default:
		fprintf(debug_stream, "csr: unknown, csr# %#06x", csr);
		break;
	}
}
void fprint_integer_size(FILE* debug_stream,UINT8 size ) {
	switch (size) {
	case 0:
		fprintf(debug_stream, "INT8");
		break;
	case 1:
		fprintf(debug_stream, "INT16");
		break;
	case 2:
		fprintf(debug_stream, "INT32");
		break;
	case 3:
		fprintf(debug_stream, "INT64");
		break;
	case 4:
		fprintf(debug_stream, "UINT8");
		break;
	case 5:
		fprintf(debug_stream, "UINT16");
		break;
	case 6:
		fprintf(debug_stream, "UINT32");
		break;
	case 7:
		fprintf(debug_stream, "UINT64");
		break;
	default:
		fprintf(debug_stream, "integer size error");
		break;
	}
}
void fprint_float_size(FILE* debug_stream,UINT8 size) {
	switch (size) {
	case 0:
		fprintf(debug_stream, "8b invalid");
		break;
	case 1:
		fprintf(debug_stream, "16b invalid");
		break;
	case 2:
		fprintf(debug_stream, "32b float - SP");
		break;
	case 3:
		fprintf(debug_stream, "64b float - DP");
		break;
	case 4:
		fprintf(debug_stream, "128b float - QP");
		break;
	default:
		fprintf(debug_stream, "floatr size error");
		break;
	}
}
void fprint_decode_header(FILE* debug_stream, ROB_Type* ROB, ROB_entry_Type* ROB_ptr, UINT8 mhartid, param_type* param) {
	fprintf(debug_stream, "DECODE(%lld): ROB entry: 0x%02x-0x%02x ", mhartid, ROB->decode_ptr, ROB->retire_ptr_in);
	fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
}
void print_uop_decode(FILE* debug_stream, ROB_entry_Type entry, UINT clock) {
	int debug = 0;
	switch (entry.uop) {
	case uop_JAL:
		fprintf(debug_stream, "JAL xr%02d.%x, offset = %#010x", entry.rd, entry.rd_ptr, ((entry.imm >> 11) & 0xfff00000) | (entry.rd_ptr, entry.imm & 0x000ff000) | ((entry.imm >> 20) & 0x7fe) | ((entry.imm >> 20) & 0x800));
		break;
	case uop_JALR:
		fprintf(debug_stream, "JALR xr%02d, xr%02d, %#010x ", entry.rd, entry.rs1, entry.imm);
		break;
	case uop_LUI:
		fprintf(debug_stream, "LUI xr%02d, 0x%016I64x ", entry.rd, (INT64)entry.imm);
		break;
	case uop_AUIPC:
		fprintf(debug_stream, "AUIPC xr%02d,  %#010x ", entry.rd, entry.imm);
		break;
	case uop_BEQ:
		fprintf(debug_stream, "BEQ xr%02d, xr%02d, %#06x ", entry.rs1, entry.rs2, entry.imm);
		break;
	case uop_BNE:
		fprintf(debug_stream, "BNE xr%02d, xr%02d, %#06x ", entry.rs1, entry.rs2, entry.imm);
		break;
	case uop_BGE:
		fprintf(debug_stream, "BGE xr%02d, xr%02d, %#06x ", entry.rs1, entry.rs2, entry.imm);
		break;
	case uop_BLT:
		fprintf(debug_stream, "BLT xr%02d, xr%02d, %#06x ", entry.rs1, entry.rs2, entry.imm);
		break;
	case uop_STORE:// need to work on funct3
		fprintf(debug_stream, "STORE xr%02d, ", entry.rs2);
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(reg%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_LOAD:// need to work on funct3
		//		fprintf(debug_stream, "LOAD xr%02d.%01d,  ", entry.rd, entry.rd_ptr);
		switch (entry.funct3) {
		case 0:
			fprintf(debug_stream, "LB xr%02d,  ", entry.rd);
			break;
		case 1:
			fprintf(debug_stream, "LH xr%02d,  ", entry.rd);
			break;
		case 2:
			fprintf(debug_stream, "LW xr%02d,  ", entry.rd);
			break;
		case 3:
			fprintf(debug_stream, "LD xr%02d,  ", entry.rd);
			break;
		case 8:
			fprintf(debug_stream, "LQ xr%02d,  ", entry.rd);
			break;
		case 4:
			fprintf(debug_stream, "ULB xr%02d,  ", entry.rd);
			break;
		case 5:
			fprintf(debug_stream, "ULH xr%02d,  ", entry.rd);
			break;
		case 6:
			fprintf(debug_stream, "ULW xr%02d,  ", entry.rd);
			break;
		case 7:
			fprintf(debug_stream, "ULD xr%02d,  ", entry.rd);
			break;
		case 12:
			fprintf(debug_stream, "ULQ xr%02d,  ", entry.rd);
			break;
		default:
			debug++;
			break;
		}
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(xr%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_LR:
		fprintf(debug_stream, "LR xr%02d.%x,  ", entry.rd, entry.rd_ptr);
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(reg%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_SC:
		fprintf(debug_stream, "SC xr%02d, ", entry.rd);		//	error response
		fprintf(debug_stream, "xr%02d, ", entry.rs1);		//	addr
		fprintf(debug_stream, "xr%02d ", entry.rs2);		//	data
	break;	
	case uop_F_STORE:// need to work on funct3
		fprintf(debug_stream, "FSTORE fp%02d, ", entry.rs2);
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(reg%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_F_LOAD:// need to work on funct3
		fprintf(debug_stream, "FLOAD fp%02d,  ", entry.rd);
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(reg%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_ADDI:
		fprintf(debug_stream, "ADDI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d, xr%02d, %d", entry.rd, entry.rs1, entry.imm);
		break;
	case uop_ORI:
		fprintf(debug_stream, "ORI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d, xr%02d, %d", entry.rd, entry.rs1, entry.imm);
		break;
	case uop_XORI:
		fprintf(debug_stream, "XORI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d, xr%02d, %d", entry.rd, entry.rs1, entry.imm);
		break;
	case uop_ANDI:
		fprintf(debug_stream, "ANDI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d, xr%02d, %d", entry.rd, entry.rs1, entry.imm);
		break;
	case uop_SLLI:
		fprintf(debug_stream, "SLLI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d, xr%02d, %d", entry.rd, entry.rs1, entry.imm);
		break;
	case uop_SRLI:
		fprintf(debug_stream, "SRLI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d, xr%02d, %d", entry.rd, entry.rs1, entry.imm);
		break;
	case uop_SRAI:
		fprintf(debug_stream, "SRAI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d, xr%02d, %d", entry.rd, entry.rs1, entry.imm);
		break;
	case uop_ADD:
		fprintf(debug_stream, "ADD ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		else if (entry.funct3 & 8)
			fprintf(debug_stream, "_128 ");
		fprintf(debug_stream, "xr%02d, xr%02d, xr%02d", entry.rd, entry.rs1, entry.rs2);
		break;
	case uop_SUB:
		fprintf(debug_stream, "SUB ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		else if (entry.funct3 & 8)
			fprintf(debug_stream, "_128 ");
		fprintf(debug_stream, "xr%02d, xr%02d, xr%02d", entry.rd, entry.rs1, entry.rs2);
	break;	case uop_OR:
		fprintf(debug_stream, "OR ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d, xr%02d, xr%02d", entry.rd, entry.rs1, entry.rs2);
		break;
	break;	case uop_XOR:
		fprintf(debug_stream, "XOR ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d, xr%02d, xr%02d", entry.rd, entry.rs1, entry.rs2);
		break;
	case uop_AND:
		fprintf(debug_stream, "AND ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d, xr%02d, xr%02d", entry.rd, entry.rs1, entry.rs2);
		break;
	case uop_SLL:
		fprintf(debug_stream, "SLL ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d, xr%02d, xr%02d", entry.rd, entry.rs1, entry.rs2);
		break;
	case uop_SRL:
		fprintf(debug_stream, "SRL ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d, xr%02d, xr%02d", entry.rd, entry.rs1, entry.rs2);
		break;
	case uop_MUL:
		fprintf(debug_stream, "MUL ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%02d, xr%02d.%02d, xr%02d.%02d", entry.rd, entry.rs1, entry.rs2);
		break;
	case uop_FCVTf2i:
		fprintf(debug_stream, "FCVT ");
		fprintf(debug_stream, "f%02d, xr%02d", entry.rd, entry.rs1);
		break;
	case uop_FCVTi2f:
		fprintf(debug_stream, "FCVT ");
		fprintf(debug_stream, "xr%02d, f%02d", entry.rd, entry.rs1);
		break;
	case uop_FMVf2i:
		fprintf(debug_stream, "FMV f2i ");
		fprintf(debug_stream, "xr%02d, f%02d", entry.rd, entry.rs1);
		break;
	case uop_FMVi2f:
		fprintf(debug_stream, "FMV i2f ");
		fprintf(debug_stream, "f%02d, xr%02d", entry.rd, entry.rs1);
		break;
	case uop_CSRRW:
		fprintf(debug_stream, "CSRRW xr%02d, ", entry.rd);
		fprint_csr(debug_stream, entry.csr);
		fprintf(debug_stream, "xr%02d ", entry.rs1);
		break;
	case uop_CSRRC:
		fprintf(debug_stream, "CSRRC xr%02d, ", entry.rd);
		fprint_csr(debug_stream, entry.csr);
		fprintf(debug_stream, "xr%02d ", entry.rs1);
		break;
	case uop_CSRRS:
		fprintf(debug_stream, "CSRRS rd = xr%02d, ", entry.rd);
		fprint_csr(debug_stream, entry.csr);
		fprintf(debug_stream, "rs1 = xr%02d ", entry.rs1);
		break;
	case uop_CSRRWI:
		if (entry.rd == 2)
			fprintf(debug_stream, "CSRRWI sp xr%02d, ", entry.rd);
		else
			fprintf(debug_stream, "CSRRWI xr%02d, ", entry.rd);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "%#04x ", entry.imm);
		break;
	case uop_CSRRCI:
		if (entry.rd == 2)
			fprintf(debug_stream, "CSRRCI sp xr%02d, ", entry.rd);
		else
			fprintf(debug_stream, "CSRRCI xr%02d, ", entry.rd);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "%#04x ", entry.imm);
		break;
	case uop_CSRRSI:
		fprintf(debug_stream, "CSRRSI xr%02d, ", entry.rd);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "%#04x ", entry.imm);
		break;
	case uop_FENCE:
		fprintf(debug_stream, "FENCE.%d ", entry.funct3);
		break;
	case uop_uret:
		fprintf(debug_stream, "uret ");
		break;
	case uop_MRET:
		fprintf(debug_stream, "MRET ");
		break;
	case uop_SRET:
		fprintf(debug_stream, "SRET ");
		break;
	case uop_ECALL:
		fprintf(debug_stream, "ECALL ");
		break;
	case uop_WFI:
		fprintf(debug_stream, "WFI ");
		break;
	case uop_HALT:
		fprintf(debug_stream, "HALT ");
		break;
	case uop_NOP:
		fprintf(debug_stream, "NOP ");
		break;
	case uop_shifti_add:
		fprintf(debug_stream, "SHIFTI ADD");
		break;
	case uop_or_add:
		fprintf(debug_stream, "OR ADD");
		break;
	case uop_or_addi:
		fprintf(debug_stream, "OR ADDI xr%02d, xr%02d, xr%02d, 0x%03x", entry.rd, entry.rs1, entry.rs2, entry.imm);
		break;
	case uop_imadd:
		fprintf(debug_stream, "integer MADD xr%02d, xr%02d, xr%02d, xr%02d", entry.rd, entry.rs1, entry.rs2, entry.rs3);
		break;
	case uop_inmadd:
		fprintf(debug_stream, "integer NMADD xr%02d, xr%02d, xr%02d, xr%02d", entry.rd, entry.rs1, entry.rs2, entry.rs3);
		break;
	default:
		debug++;
		break;
	}
	fprintf(debug_stream, "   // ");

}

// seperate out decode unit and microcode block
// need to add latches to have 1 clock delay switching from decoder to microcode as uop source

void decode32_RISC_V(ROB_Type* ROB, Q_type* exec_q, reg_bus* rd, UINT8* flush_write_posters, UINT* ecall_out, UINT ecall_in, decode_shifter_struct* shifter, UINT8 prefetcher_idle, decode_type* decode_vars, branch_vars* branch, IntUnitType* IntUnitVars, UINT stores_pending, UINT load_pending, retire_type*retire,
	UINT64 clock, csr_type* csr, UINT* uPC, store_buffer_type* store_buffer, UINT mhartid, UINT debug_core, param_type *param, FILE* debug_stream) {
	UINT debug = 0;
	UINT8 stop = 0;

	UINT debug_unit = (param->decoder == 1) && debug_core;
	UINT debug_branch = (param->decoder || param->branch) && debug_core;
	UINT debug_page_walk_unit = (param->decoder == 1 || param->PAGE_WALK == 1 || param->PAGE_FAULT == 1) && debug_core;

	shifter->response.msg = inactive;
	decode_vars->fault_release = 0;
	if (mhartid == 0) {
		if (clock >= 0x0060)
			debug++;
	}
	ecall_out[0] = 0;
	flush_write_posters[0] = 0;
	if (csr[csr_mcause].value == 0)
		shifter->response.fault_in_service = 0;
	shifter->index = 0;
	rd->strobe = 0;

	if ((((csr[csr_mstatus].value & 0x08) == 0x08) || ((csr[csr_mstatus].value & 0x02) == 0x02)) &&
		(csr[csr_mcause].value != 0) && ROB->q[ROB->retire_ptr_in].uop != uop_SRET && shifter->response.fault_in_service == 0) {

		// need to chack for prefetcher xaction pending
		if (load_pending) {
			if (debug_page_walk_unit) {
				fprintf(debug_stream, "DECODE(%lld): Exception/Interrupt detected, load pending;  mstatus: %#010x, mcause: %#010x, bad addr: %#010x, return addr: %#010x, clock: 0x%04llx\n",
					mhartid, csr[csr_mstatus].value, csr[csr_mcause].value, csr[csr_mtval].value, csr[csr_sepc].value, clock);
			}
		}
		else {
			rd->strobe = 1;
			rd->data = csr[csr_stvec].value;
			rd->data_H = 0;
			ROB_entry_Type* ROB_ptr = &ROB->q[ROB->decode_ptr];
//			if (decode_vars->fault_delay) {
				ROB_ptr->addr = 0;
				stop |= fence_check(ROB_ptr, flush_write_posters, ROB, csr, prefetcher_idle, (ecall_in == 0) ? 1 : 0, stores_pending, clock, store_buffer, mhartid, debug_unit, param, debug_stream);
				ROB_ptr->bytes = 4;
				decode_vars->index = 0;

				if (stop) {
					// fence conditions not met
	//				decode_vars->index--;
				}
				else {
					uPC[0] = 4;
					if (debug_page_walk_unit) {
						fprintf(debug_stream, "DECODE(%lld): Exception/Interrupt detected: mstatus: %#010x, mcause: %#010x, return addr: %#010x, clock: 0x%04llx\n",
							mhartid, csr[csr_mstatus].value, csr[csr_mcause].value, csr[csr_sepc].value, clock);
					}
					csr[csr_sepc].value = retire->PC;// save last address retired (return address)
					shifter->response.msg = service_fault;// exception/interrupt handler starting

					decode_vars->fault_release = 1;
					ROB->fault_halt = 0;
					ROB->fence = 0;

					csr[csr_mstatus].value &= ~0xa0;
					csr[csr_mstatus].value |= ((csr[csr_mstatus].value << 4) & 0xa0);
					csr[csr_mstatus].value &= ~0x0a;// 8=machine, 2=supervisor

					csr[csr_mstatus].value &= ~(0x1900);
					csr[csr_mstatus].value |= (((retire->priviledge & 3) << 11) | ((retire->priviledge & 1) << 8));// save current priviledge
					csr[csr_sstatus].value = csr[csr_mstatus].value;

//					csr[0x0f41].value = 0x0000;

					csr[csr_mstatus].value &= ~0x0a;
					csr[csr_mstatus].value |= ((csr[csr_mstatus].value & 0xa0) >> 4);
					csr[csr_mstatus].value &= ~0xa0; // clear bits

					csr[csr_mstatus].value |= (1 << 11);
					retire->priviledge = (csr[csr_mstatus].value >> 11) & 3;
					csr[csr_mstatus].value &= ~(0x1900);// clear bits

					shifter->response.fault_in_service = 1;
					shifter->response.addr = csr[csr_stvec].value;
					ROB->decode_ptr = ROB->retire_ptr_in;
					rd->strobe = 2;
					rd->data = csr[csr_stvec].value;
					rd->data_H = 0;

//					decode_vars->fault_delay = 0;
				}
//			}
//			else {
//				decode_vars->fault_delay = 1;
//			}
		}
	}
	else if (shifter->valid == 1) {
		if ((shifter->buffer[shifter->index + 0] & 3) != 3) {
			debug++; // error: 16b opcodes not coded
			exit(0);
		}
		else if (((shifter->buffer[shifter->index + 0] >> 2) & 0x7) != 7) {// 32b code
			for (decode_vars->index = 0; decode_vars->index < param->decode_width && !stop && ((ROB->decode_ptr + 1) & 0xff) != ROB->retire_ptr_in && ((ROB->decode_ptr + 2) & 0xff) != ROB->retire_ptr_in && shifter->response.msg != halt; ) {
				stop = decode_32b(ROB, decode_vars->perf_reg, flush_write_posters, retire->priviledge, shifter, decode_vars, prefetcher_idle, stores_pending, load_pending,
					clock, branch, store_buffer, uPC, mhartid, debug_core, param, debug_stream);
			}
		}
		else if ((shifter->buffer[shifter->index + 0] & 0x3f) == 0x1f) {//48bcode
			debug++;
			exit(0);
		}
		else {
			debug++; // error: opcodes larger than 32b not coded
			exit(0);
		}
		/*
		for (decode_vars->index = 0; decode_vars->index < param->decode_width && !stop && ((ROB->decode_ptr + 1) & 0xff) != ROB->retire_ptr_in && ((ROB->decode_ptr + 2) & 0xff) != ROB->retire_ptr_in && shifter->response.msg != halt; ) {
			INT buffer = (shifter->buffer[shifter->index + 1] << 16) | shifter->buffer[shifter->index + 0];
			if ((buffer & 3) != 3) {
				debug++; // error: 16b opcodes not coded
				exit(0);
			}
			else if (((buffer>>2)&0x7)!=7){// 32b code
				stop = decode_32b(ROB, decode_vars->perf_reg, flush_write_posters, retire->priviledge, shifter, decode_vars, csr, prefetcher_idle, stores_pending, load_pending,
					clock, branch, store_buffer, uPC, mhartid, debug_core, param, debug_stream);
			}
			else if ((buffer&0x3f) == 0x1f) {//48bcode
				debug++;
				exit(0);
			}
			else {
				debug++; // error: opcodes larger than 32b not coded
				exit(0);
			}
		}
		/**/
	}
}

