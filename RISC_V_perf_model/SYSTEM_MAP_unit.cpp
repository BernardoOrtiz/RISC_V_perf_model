// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include"ROB.h"

void system_map_unit(csr_type* csr, reg_bus* rd, UINT16* exec_rsp, R_type* sys_exec, retire_type* retire, UINT8 reset, 
	UINT64 clock, UINT debug_core, param_type param, FILE* debug_stream) {
	int debug = 0;
	reg_bus* rd_out = &rd[sys_q_id];
	if (reset) {
		for (UINT16 i = 0; i < 0x20; i++) { // clear performance counters
			csr[csr_cycle | i].value = 0;
			csr[csr_scycle | i].value = 0;
			csr[csr_hcycle | i].value = 0;
			csr[csr_mcycle | i].value = 0;
		}
		csr[csr_mcause].value = 0;
		csr[csr_mstatus].value = 0;
	}
	else {
		csr[csr_cycle].value++;
		switch (retire->priviledge) {
		case 0:
			break;
		case 1:
			csr[csr_scycle].value++;
			break;
		case 2:
			csr[csr_hcycle].value++;
			break;
		case 3:
		case 0x0f:
			csr[csr_mcycle].value++;
			break;
		default:
			debug++;
			break;
		}
	}

	if (clock == 0x15d8)
		debug++;
	rd_out->strobe = 0;
	UINT mhartid = csr[csr_mhartid].value;
	UINT debug_unit = (param.csr == 1) && debug_core;
	if (sys_exec->strobe) {
		rd_out->ROB_id = sys_exec->ROB_id;
		if (mhartid == 0) {
			if (clock == 0x1e82)
				debug++;
		}
		switch (sys_exec->uop) {
		case uop_CSRRWI:
		case uop_CSRRW: {
			rd_out->strobe = 1;
			rd_out->data = csr[sys_exec->rs2].value;
			rd_out->data_H = 0;
			csr[sys_exec->rs2].value = sys_exec->rs1;
			if (debug_unit) {
				fprintf(debug_stream, "SYS(%lld): ROB entry: 0x%02x CSRRW ", mhartid, sys_exec->ROB_id);
				fprint_csr(debug_stream,(control_status_reg_type)sys_exec->rs2);
				fprintf(debug_stream, " csr value: 0x%016I64x, rd value: 0x%016I64x, clock: 0x%#04llx\n", csr[sys_exec->rs2].value, rd_out->data, clock);
			}
			rd_out->strobe = 1;
			exec_rsp[0] = 0x8000 | sys_exec->ROB_id;
			if (sys_exec->rs2 == csr_mstatus && debug_unit) {
				fprintf(debug_stream, "SYS(%lld): mcause value: 0x%016I64x, clock: 0x%#04llx\n", mhartid, csr[csr_mcause].value, clock);
			}
		}
					  break;
		case uop_CSRRCI:
		case uop_CSRRC: {
			rd_out->strobe = 1;
			rd_out->data = csr[sys_exec->rs2].value;
			rd_out->data_H = 0;
			csr[sys_exec->rs2].value = csr[sys_exec->rs2].value & (~sys_exec->rs1);
			if (sys_exec->rs2 == csr_mcause) { // hardwired that when mcause is cleared or set, so is scause (not the inverse)
				csr[csr_scause].value = csr[csr_scause].value & (~sys_exec->rs1);
			}
			if (debug_unit) {
				fprintf(debug_stream, "SYS(%lld): ROB entry: 0x%02x CSRRS ", mhartid, sys_exec->ROB_id);
				fprint_csr(debug_stream,(control_status_reg_type)sys_exec->rs2);
				fprintf(debug_stream, " csr value: 0x%016I64x, rd value: 0x%016I64x, clock: 0x%#04llx\n", csr[sys_exec->rs2].value, rd_out->data, clock);
			}
			rd_out->strobe = 1;
			exec_rsp[0] = 0x8000 | sys_exec->ROB_id;
			if (sys_exec->rs2 == csr_mstatus && debug_unit) {
				fprintf(debug_stream, "SYS(%lld): mcause value: 0x%016I64x, clock: 0x%#04llx\n", mhartid, csr[csr_mcause].value, clock);
			}
		}
					  break;
		case uop_CSRRSI:
		case uop_CSRRS: {
			rd_out->strobe = 1;
			rd_out->data = csr[sys_exec->rs2].value;
			rd_out->data_H = 0;
			csr[sys_exec->rs2].value = csr[sys_exec->rs2].value | sys_exec->rs1;
			if (debug_unit) {
				fprintf(debug_stream, "SYS(%lld): ROB entry: 0x%02x CSRRS ", mhartid, sys_exec->ROB_id);
				fprint_csr(debug_stream,(control_status_reg_type)sys_exec->rs2);
				fprintf(debug_stream, " csr value: 0x%016I64x, rd value: 0x%016I64x, clock: 0x%#04llx\n", csr[sys_exec->rs2].value, rd_out->data, clock);
			}
			rd_out->strobe = 1;
			exec_rsp[0] = 0x8000 | sys_exec->ROB_id;
			if (sys_exec->rs2 == csr_mstatus && debug_unit) {
				fprintf(debug_stream, "SYS(%lld): mcause value: 0x%016I64x, clock: 0x%#04llx\n", mhartid, csr[csr_mcause].value, clock);
			}
		}
					  break;
		case uop_ECALL: // write directly to _cause registers, handle through micocode
			debug++;
			switch (retire->priviledge) {
			case 0:
				if (csr[csr_medeleg].value & 0x100) {
					csr[csr_scause].value |= 0x100;
					csr[csr_mcause].value |= 0x100;
				}
				else {
					csr[csr_mcause].value |= 0x100;
				}
				break;
			case 1:
				if (csr[csr_medeleg].value & 0x200) {
					csr[csr_scause].value |= 0x200;
					csr[csr_mcause].value |= 0x200;
				}
				else {
					csr[csr_mcause].value |= 0x200;
				}
				break;
			case 0x0f:
			case 3:
				if (csr[csr_medeleg].value & 0x800) {
					csr[csr_scause].value |= 0x800;
					csr[csr_mcause].value |= 0x800;
				}
				else {
					csr[csr_mcause].value |= 0x800;
				}
				break;
			default:
				debug++;
				break;
			}
			if (debug_unit) {
				fprintf(debug_stream, "SYS(%lld): ROB entry: 0x%02x ECALL clock: 0x%#04llx\n", mhartid, sys_exec->ROB_id, clock);
			}

			rd_out->strobe = 1;
			exec_rsp[0] = 0x8000 | sys_exec->ROB_id;
			if (sys_exec->rs2 == csr_mstatus && debug_unit) {
				fprintf(debug_stream, "SYS(%lld): mcause value: 0x%016I64x, clock: 0x%#04llx\n", mhartid, csr[csr_mcause].value, clock);
			}
			break;
		case uop_MRET:
			csr[csr_mstatus].value = ~0x08;
			csr[csr_mstatus].value = (csr[csr_mstatus].value >> 4) & 0x08;
			csr[csr_mstatus].value = ~0x80;
			rd_out->strobe = 1;
			exec_rsp[0] = 0x8000 | sys_exec->ROB_id;
			if (debug_unit) {
				fprintf(debug_stream, "SYS(%lld): ROB entry: 0x%02x MRET clock: 0x%#04llx\n", mhartid, sys_exec->ROB_id, clock);
			}
			break;
		case uop_SRET:
			csr[csr_sstatus].value = ~0x08;
			csr[csr_sstatus].value |= (csr[csr_sstatus].value >> 4) & 0x08;
			csr[csr_sstatus].value = ~0x80;
			rd_out->strobe = 1;
			exec_rsp[0] = 0x8000 | sys_exec->ROB_id;
			if (debug_unit) {
				fprintf(debug_stream, "SYS(%lld): ROB entry: 0x%02x SRET clock: 0x%#04llx\n", mhartid, sys_exec->ROB_id, clock);
			}
			break;
			/*
		case uop_JALR: {
			UINT64 addr = sys_exec->rs1 + sys_exec->rs2;
			if (addr >= csr[csr_sbound].value) {//0x86400000
				if (retire->priviledge != 0 && stores_pending)
					debug++;
				else {
					rd_out->strobe = 1;
					exec_rsp[0] = 0x8000 | sys_exec->ROB_id;
					if (debug_unit) {
						fprintf(debug_stream, "SYS(%lld): ROB entry: 0x%02x JALR clock: 0x%#04llx\n", mhartid, sys_exec->ROB_id, clock);
					}
				}
//				retire->priviledge = 0;
			}
			else if (addr >= csr[csr_mbound].value) {//0x80000000
				if (retire->priviledge != 1 && stores_pending)
					debug++;
				else {
					rd_out->strobe = 1;
					exec_rsp[0] = 0x8000 | sys_exec->ROB_id;
					if (debug_unit) {
						fprintf(debug_stream, "SYS(%lld): ROB entry: 0x%02x JALR clock: 0x%#04llx\n", mhartid, sys_exec->ROB_id, clock);
					}
				}

//				retire->priviledge = 1;
			}
			else {
				if (retire->priviledge != 3 && stores_pending)
					debug++;
				else {
					rd_out->strobe = 1;
					exec_rsp[0] = 0x8000 | sys_exec->ROB_id;
					if (debug_unit) {
						fprintf(debug_stream, "SYS(%lld): ROB entry: 0x%02x JALR clock: 0x%#04llx\n", mhartid, sys_exec->ROB_id, clock);
					}
				}
				//				retire->priviledge = 3;
			}

		}
			break;
			/**/
		default:
			debug++;
			break;
		}
	}

	if (rd[prefetch_q_id].strobe) {
		csr[csr_scause].value |= 0x00001000;										// set bit 12, instruction page fault
		csr[csr_mcause].value |= 0x00001000;										// set bit 12, instruction page fault
		csr[csr_mtval].value = csr[csr_stval].value = rd[prefetch_q_id].data;// save offending address - ISSUE, need to drop addr from data
		csr[csr_sepc].value = csr[csr_mepc].value = retire->PC;// save offending address - ISSUE, need to drop addr from data
		if (debug_unit) {
			fprintf(debug_stream, "PREFETCHER(%lld): Instr Page Fault Handler Triggered; addr: 0x%016I64x, return addr: 0x%016I64x, clock: 0x%04llx\n",
				mhartid, csr[csr_mtval].value, csr[csr_mepc].value, clock);
		}
	}
}