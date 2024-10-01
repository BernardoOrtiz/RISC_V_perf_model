// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "ROB.h"
void rd_update(Reg_Table_Type* reg_table, reg_bus* rd_JALR, reg_bus* rd, ROB_Type* ROB, Q_type* exec_q, csr_type* csr, UINT64 clock, UINT mhartid, UINT debug_core, param_type* param, FILE* debug_stream) {
	int debug = 0;
	if (rd[sys_q_id].strobe) {
		ROB_entry_Type* ROB_ptr = &ROB->q[ROB->retire_ptr_in];
		ROB_ptr->state = ROB_retire_out;
		if (ROB_ptr->uop == uop_SRET) {
			if (param->csr && debug_core) {
				fprintf(debug_stream, "SYS(%lld): ROB entry: 0x%02x ", mhartid, ROB->retire_ptr_in);
				fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
				fprintf(debug_stream, " SRET clk: 0x%llx\n", clock);
			}
		}
		else if (ROB_ptr->uop == uop_ECALL) {
			ROB_ptr->state = ROB_free;
			if (param->csr && debug_core) {
				fprintf(debug_stream, "SYS(%lld): ROB entry: 0x%02x ", mhartid, ROB->retire_ptr_in);
				fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
				fprintf(debug_stream, " ECALL end (never retires) clk: 0x%llx\n", clock);
			}
		}
		else {
			reg_table->x_reg[ROB_ptr->rd].data_H[ROB_ptr->rd_ptr] = rd[sys_q_id].data_H;
			reg_table->x_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr] = rd[sys_q_id].data;
			reg_table->x_reg[ROB_ptr->rd].valid[ROB_ptr->rd_ptr] = reg_valid_in;
			exec_q[sys_q_id].start_ptr = ((exec_q[sys_q_id].start_ptr + 1) & (exec_q[sys_q_id].count - 1));
			if (param->csr && debug_core) {
				fprintf(debug_stream, "SYS(%lld): ROB entry: 0x%02x ", mhartid, ROB->retire_ptr_in);
				fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
				fprintf(debug_stream, " SYS map reg unit: rd(xr%02d.%x) = 0x%016I64x, clk: 0x%llx\n",
					ROB_ptr->rd, ROB_ptr->rd_ptr, reg_table->x_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr], clock);
			}
		}
	}
	if (rd[load_q_id].strobe == 1) {
		ROB_entry_Type* ROB_ptr = &ROB->q[rd[load_q_id].ROB_id];
		if (ROB_ptr->map == LOAD_map) {
			reg_table->x_reg[ROB_ptr->rd].data_H[ROB_ptr->rd_ptr] = rd[load_q_id].data_H;
			reg_table->x_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr] = rd[load_q_id].data;
			reg_table->x_reg[ROB_ptr->rd].valid[ROB_ptr->rd_ptr] = reg_valid_in;
			if (param->load && debug_core) {
				fprintf(debug_stream, "LOAD(%lld): ROB entry: %#04x ", mhartid, rd[load_q_id].ROB_id);
				fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
				fprintf(debug_stream, " LOAD reg unit: rd(xr%02d.%x) = 0x%016I64x, clk: 0x%llx\n",
					ROB_ptr->rd, ROB_ptr->rd_ptr, reg_table->x_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr], clock);
			}
		}
		else if (ROB_ptr->map == LOAD_FP_map) {
			reg_table->f_reg[ROB_ptr->rd].data_H[ROB_ptr->rd_ptr] = rd[load_q_id].data_H;
			reg_table->f_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr] = rd[load_q_id].data;
			reg_table->f_reg[ROB_ptr->rd].valid[ROB_ptr->rd_ptr] = reg_valid_in;
			if (param->load && debug_core) {
				fprintf(debug_stream, "LOAD(%lld): ROB entry: %#04x ", mhartid, rd[load_q_id].ROB_id);
				fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
				fprintf(debug_stream, " FLOAD reg unit: rd(xr%02d.%x) = 0x%016I64x, clk: 0x%llx\n",
					ROB_ptr->rd, ROB_ptr->rd_ptr, reg_table->f_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr], clock);
			}
		}
		else {
			debug++;
		}
		ROB_ptr->state = ROB_retire_out;
		for (UINT i = 0; i < 0x20; i++)
			exec_q[i].curr_ptr = exec_q[i].start_ptr;
	}
	else if (rd[load_q_id].strobe == 2) {
		ROB_entry_Type* ROB_ptr = &ROB->q[rd[load_q_id].ROB_id];
		reg_table->f_reg[ROB_ptr->rd].data_H[ROB_ptr->rd_ptr] = rd[load_q_id].data_H;
		reg_table->f_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr] = rd[load_q_id].data;
		reg_table->f_reg[ROB_ptr->rd].valid[ROB_ptr->rd_ptr] = reg_valid_in;
		ROB_ptr->state = ROB_retire_out;
		exec_q[fmul_q_id0].curr_ptr = exec_q[fmul_q_id0].start_ptr;
		if (param->load && debug_core) {
			fprintf(debug_stream, "LOAD(%lld): ROB entry: %#04x ", mhartid, rd[load_q_id].ROB_id);
			fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
			fprintf(debug_stream, " LOAD reg unit: rd(fp%d.%x) = 0x%016I64x, clk: 0x%llx\n",
				ROB_ptr->rd, ROB_ptr->rd_ptr, reg_table->f_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr], clock);
		}
		for (UINT i = 0; i < 0x20; i++)
			exec_q[i].curr_ptr = exec_q[i].start_ptr;
	}
	if (rd[store_q_id].strobe == 1) {
		ROB_entry_Type* ROB_ptr = &ROB->q[rd[store_q_id].ROB_id];
		switch (ROB_ptr->uop) {
		case uop_LR:
		case uop_SC:
			reg_table->x_reg[ROB_ptr->rd].data_H[ROB_ptr->rd_ptr] = rd[store_q_id].data_H;
			reg_table->x_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr] = rd[store_q_id].data;
			reg_table->x_reg[ROB_ptr->rd].valid[ROB_ptr->rd_ptr] = reg_valid_in;
			ROB_ptr->state = ROB_retire_out;
			for (UINT i = 0; i < 0x20; i++)	exec_q[i].curr_ptr = exec_q[i].start_ptr;
			if (param->store && debug_core) {
				fprintf(debug_stream, "STORE(%lld): ROB entry: %#04x ", mhartid, rd[store_q_id].ROB_id);
				fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
				fprintf(debug_stream, " LR/SC reg unit: rd(xr%02d.%x) = 0x%016I64x, clk: 0x%llx\n",
					ROB_ptr->rd, ROB_ptr->rd_ptr, reg_table->x_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr], clock);
			}
			break;
		defualt:
			debug++;
			break;
		}
	}
	for (UINT i = 0; i < 8; i++) {
		if (rd[OP_q_id0 + i].strobe == 1 || rd[OP_q_id0 + i].strobe == 2) {
			ROB_entry_Type* ROB_ptr = &ROB->q[rd[OP_q_id0 + i].ROB_id];
			if (ROB_ptr->map == JALR_map) {
				rd_JALR->strobe = 1;
				rd_JALR->data = rd[OP_q_id0 + i].data;
				rd_JALR->data_H = rd[OP_q_id0 + i].data_H;
				rd_JALR->ROB_id = rd[OP_q_id0 + i].ROB_id;
			}
			reg_table->x_reg[ROB_ptr->rd].data_H[ROB_ptr->rd_ptr] = rd[OP_q_id0 + i].data_H;
			reg_table->x_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr] = rd[OP_q_id0 + i].data;
			reg_table->x_reg[ROB_ptr->rd].valid[ROB_ptr->rd_ptr] = reg_valid_in;
			ROB_ptr->state = ROB_retire_out;
			//			exec_q[load_q_id].curr_ptr = exec_q[load_q_id].start_ptr;
			if (param->intx && debug_core) {
				fprintf(debug_stream, "INT(%lld): ROB entry: %#04x 0x%016I64x, INT unit %d rd updated (xr%02d.%x) = 0x%016I64x, clk: 0x%llx\n",
					mhartid, rd[OP_q_id0 + i].ROB_id, ROB_ptr->addr, i, ROB_ptr->rd, ROB_ptr->rd_ptr, reg_table->x_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr], clock);
			}
		}
		if (rd[fadd_q_id0 + i].strobe == 1 || rd[fadd_q_id0 + i].strobe == 2) {
			ROB_entry_Type* ROB_ptr = &ROB->q[rd[fadd_q_id0 + i].ROB_id];
			if (ROB_ptr->uop == uop_FCVTf2i || ROB_ptr->uop == uop_FMVf2i) {
				reg_table->x_reg[ROB_ptr->rd].data_H[ROB_ptr->rd_ptr] = rd[OP_q_id0 + i].data_H;
				reg_table->x_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr] = rd[OP_q_id0 + i].data;
				reg_table->x_reg[ROB_ptr->rd].valid[ROB_ptr->rd_ptr] = reg_valid_in;
				if (param->fp && debug_core) {
					fprintf(debug_stream, "FADD(%lld): ROB entry: %#04x 0x%016I64x, FADD unit %d rd updated (xr%02d.%x) = 0x%016I64x, clk: 0x%llx\n",
						mhartid, rd[fadd_q_id0 + i].ROB_id, ROB_ptr->addr, i, ROB_ptr->rd, ROB_ptr->rd_ptr, reg_table->x_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr], clock);
				}
				//				exec_q[load_q_id].curr_ptr = exec_q[load_q_id].start_ptr;
			}
			else {
				reg_table->f_reg[ROB_ptr->rd].data_H[ROB_ptr->rd_ptr] = rd[fadd_q_id0 + i].data_H;
				reg_table->f_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr] = rd[fadd_q_id0 + i].data;
				reg_table->f_reg[ROB_ptr->rd].valid[ROB_ptr->rd_ptr] = reg_valid_in;
				if (param->fp && debug_core) {
					fprintf(debug_stream, "FADD(%lld): ROB entry: %#04x 0x%016I64x, FADD unit %d reg unit: rd(fp%02d.%x) = 0x%016I64x, clk: 0x%llx\n",
						mhartid, rd[fadd_q_id0 + i].ROB_id, ROB_ptr->addr, i, ROB_ptr->rd, ROB_ptr->rd_ptr, reg_table->x_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr], clock);
				}
				exec_q[fmul_q_id0].curr_ptr = exec_q[fmul_q_id0].start_ptr;
			}
			ROB_ptr->state = ROB_retire_out;
		}
	}
	for (UINT i = 0; i < 4; i++) {
		if (rd[fmul_q_id0 + i].strobe == 1 || rd[fmul_q_id0 + i].strobe == 2) {
			ROB_entry_Type* ROB_ptr = &ROB->q[rd[fmul_q_id0 + i].ROB_id];
			reg_table->f_reg[ROB_ptr->rd].data_H[ROB_ptr->rd_ptr] = rd[fmul_q_id0 + i].data_H;
			reg_table->f_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr] = rd[fmul_q_id0 + i].data;
			reg_table->f_reg[ROB_ptr->rd].valid[ROB_ptr->rd_ptr] = reg_valid_in;
			ROB_ptr->state = ROB_retire_out;
			if (param->fp && debug_core) {
				fprintf(debug_stream, "FMADD(%lld): ROB entry: %#04x 0x%016I64x, FMADD reg unit(%d): rd(fp%02d.%x) = 0x%016I64x, clk: 0x%llx\n",
					mhartid, rd[fmul_q_id0 + i].ROB_id, ROB_ptr->addr, i, ROB_ptr->rd, ROB_ptr->rd_ptr, reg_table->x_reg[ROB_ptr->rd].data[ROB_ptr->rd_ptr], clock);
			}
			exec_q[fmul_q_id0].curr_ptr = exec_q[fmul_q_id0].start_ptr;
		}
	}
	rd[branch_q_id2].strobe = 0;
	if (rd[branch_q_id].strobe) {
		ROB_entry_Type* ROB_ptr = &ROB->q[exec_q[branch_q_id].ROB_ptr[exec_q[branch_q_id].start_ptr]];
		ROB_ptr->state = ROB_retire_out;
		ROB_ptr->imm = rd[branch_q_id].data;
		if (rd[branch_q_id].strobe == 2) {
			for (UINT i = 0; i < 0x20; i++)	exec_q[i].end_ptr = exec_q[i].start_ptr;
			for (UINT i = 0; i < 0x20; i++) reg_table->x_reg[i].current_ptr = reg_table->x_reg[i].retire_ptr;
			for (UINT i = 0; i < 0x20; i++) reg_table->f_reg[i].current_ptr = reg_table->f_reg[i].retire_ptr;
			ROB->decode_ptr = ROB->allocate_ptr = (rd[branch_q_id].ROB_id + 1) & 0x00ff;
			rd[branch_q_id2].strobe = 1;
			rd[branch_q_id2].data = rd[branch_q_id].data;
			rd[branch_q_id2].ROB_id = rd[branch_q_id].ROB_id;
			if (param->branch && debug_core) {
				fprintf(debug_stream, "BRANCH(%lld): ROB entry: %#04x ", mhartid, rd[branch_q_id].ROB_id);
				fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
				fprintf(debug_stream, " BRANCH MISS reg unit new addr = ");
				fprint_addr_coma(debug_stream, rd[branch_q_id2].data, param);
				fprintf(debug_stream, "  clk: 0x%llx\n", clock);
			}
		}
		else if (rd[branch_q_id].strobe == 3) {
			rd[branch_q_id2].strobe = 1;
			rd[branch_q_id2].data = rd[branch_q_id].data;
			rd[branch_q_id2].ROB_id = rd[branch_q_id].ROB_id;
			if (param->branch && debug_core) {
				fprintf(debug_stream, "BRANCH(%lld): ROB entry: %#04x ", mhartid, rd[branch_q_id].ROB_id);
				fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
				fprintf(debug_stream, " JALR reg unit new addr = ");
				fprint_addr_coma(debug_stream, rd[branch_q_id2].data, param);
				fprintf(debug_stream, "  clk: 0x%llx\n", clock);
			}
		}
	}
	if (rd[decode_q_id].strobe) {
		for (int i = 0; i < 0x100; i++)ROB->q[i].state = ROB_free;
		ROB->decode_ptr = ROB->allocate_ptr = ROB->retire_ptr_in = ROB->retire_ptr_out;
		for (UINT8 j = 0; j <= 0x20; j++) 	exec_q[j].end_ptr = exec_q[j].start_ptr;
		for (UINT8 j = 0; j < 32; j++) {
			reg_table->x_reg[j].current_ptr = reg_table->x_reg[j].retire_ptr;
			for (UINT8 i = 0; i < reg_rename_size; i++)
				reg_table->x_reg[j].valid[i] = reg_free;
			reg_table->x_reg[j].valid[reg_table->x_reg[j].retire_ptr] = reg_valid_out;

			reg_table->f_reg[j].current_ptr = reg_table->f_reg[j].retire_ptr;
			for (UINT8 i = 0; i < reg_rename_size; i++)
				reg_table->f_reg[j].valid[i] = reg_free;
			reg_table->f_reg[j].valid[reg_table->f_reg[j].retire_ptr] = reg_valid_out;
		}
	}
}
void reg_unit_house_cleaning(ROB_Type* ROB, csr_type* csr, Q_type* exec_q, R_type* load_exec, UINT16* exec_rsp, reg_bus* rd,UINT8 priviledge, UINT64 clock, UINT debug_core, UINT mhartid, param_type* param, FILE* debug_stream) {
	int debug = 0;
	for (UINT i = 0; i < 0x20; i++) {
		if (i == store_q_id)
			debug++;
		if (exec_rsp[i] ) {
			if (i == fmul_q_id0 || i == fmul_q_id1 || i == fmul_q_id2 || i == fmul_q_id3) {
				ROB->q[exec_rsp[i] & 0xff].state = ROB_inflight;
				while (ROB->q[exec_q[i].ROB_ptr[exec_q[i].start_ptr]].state != ROB_execute && exec_q[i].start_ptr != exec_q[i].end_ptr) {
					int hold = exec_q[i].start_ptr;
					exec_q[i].start_ptr = ((exec_q[i].start_ptr + 1) & (exec_q[i].count - 1));
					if (exec_q[i].curr_ptr == hold)exec_q[i].curr_ptr = exec_q[i].start_ptr;
				}
			}
			else if (i == load_q_id) {
				if (exec_rsp[load_q_id] & 0x4000) {
					if (csr[csr_mcause].value == 0) {// hack, need to put interrupt addr into a fifo
						if (ROB->retire_ptr_out == (exec_rsp[load_q_id] & 0x00ff)) {
							load_exec->strobe = 1;
							load_exec->ROB_id = (exec_rsp[load_q_id] & 0x00ff);

							ROB->q[exec_rsp[load_q_id] & 0x00ff].state = ROB_fault;
							csr[csr_mcause].value |= 0x00002000;										// set bit 13, instruction page fault
							csr[csr_scause].value |= 0x00002000;										// set bit 13, instruction page fault
							csr[csr_mepc].value = ROB->q[exec_rsp[load_q_id] & 0x00ff].addr;
							csr[csr_mtval].value = csr[csr_stval].value = rd[load_q_id].data;// save offending address - ISSUE, need to drop addr from data
							csr[csr_sepc].value = ROB->q[exec_rsp[load_q_id] & 0x00ff].addr;
							if (param->load && debug_core) {
								fprintf(debug_stream, "LOAD(%d): ROB entry: 0x%02x, ", mhartid, exec_rsp[load_q_id] & 0x00ff);
								fprint_addr_coma(debug_stream, csr[csr_mepc].value, param);
								fprintf(debug_stream, " Page Fault reg unit addr: ");
								fprint_addr_coma(debug_stream, rd[load_q_id].data, param);
								fprintf(debug_stream, " clock: 0x%04llx\n", clock);
							}
						}
						else {
							debug++;
						}
					}
					else {
						load_exec->strobe = 1;
						load_exec->ROB_id = (exec_rsp[load_q_id] & 0x00ff);

						fprintf(debug_stream, "LOAD(%d): ROB entry: 0x%02x, ERROR: Load Page Fault reg unit; triggered without servicing previous fault 0x%04x, fault addr: 0x%016I64x return addr: 0x%016I64x clock: 0x%04llx\n",
							csr[csr_mhartid].value, exec_rsp[load_q_id] & 0x00ff, csr[csr_mcause].value, ROB->q[exec_rsp[load_q_id] & 0x00ff].addr, csr[csr_mepc].value, clock);
						debug++;
					}
				}
				else if (exec_rsp[load_q_id] & 0x2000) {
					exec_q[load_q_id].curr_ptr = (exec_q[load_q_id].curr_ptr + 1) & (exec_q[load_q_id].count - 1);
					exec_q[load_q_id].curr_ptr = (exec_q[load_q_id].curr_ptr == exec_q[load_q_id].end_ptr) ? exec_q[load_q_id].start_ptr : exec_q[load_q_id].curr_ptr;
				}
				else {
					ROB->q[exec_rsp[i] & 0xff].state = ROB_inflight;
					csr[csr_load_issued].value++;
					switch (priviledge) {
					case 0:
						break;
					case 1:
						csr[csr_sload_issued].value++;
						break;
					case 2:
						csr[csr_hload_issued].value++;
						break;
					case 0x0f:
					case 3:
						csr[csr_mload_issued].value++;
						break;
					default:
						debug++;
						break;
					}
					while (ROB->q[exec_q[i].ROB_ptr[exec_q[i].start_ptr]].state != ROB_execute && exec_q[i].start_ptr != exec_q[i].end_ptr) {
						int hold = exec_q[i].start_ptr;
						exec_q[i].start_ptr = ((exec_q[i].start_ptr + 1) & (exec_q[i].count - 1));
						if (exec_q[i].curr_ptr == hold)exec_q[i].curr_ptr = exec_q[i].start_ptr;
					}

				}
			}
			else if (i == branch_q_id && exec_q[i].start_ptr != exec_q[i].end_ptr) {
				ROB->q[exec_rsp[i] & 0xff].state = ROB_retire_out;
				ROB->branch_start = ((ROB->branch_start + 1) & 3);
				exec_q[i].start_ptr = ((exec_q[i].start_ptr + 1) & (exec_q[i].count - 1));
			}
			else if (i == store_q_id) {
				if (exec_rsp[i] & 0x2000) {// write poster hit
					ROB->q[exec_rsp[i] & 0xff].state = ROB_retire_out;
					exec_q[i].start_ptr = ((exec_q[i].start_ptr + 1) & (exec_q[i].count - 1));
				}
				else if (exec_q[i].start_ptr != exec_q[i].end_ptr) {
					ROB->q[exec_rsp[i] & 0xff].state = ROB_inflight;
					exec_q[i].start_ptr = ((exec_q[i].start_ptr + 1) & (exec_q[i].count - 1));
				}
			}
			else if (i == store_q_id2) {
				if (exec_rsp[i] & 0x4000) {
					if (csr[csr_mcause].value == 0) {// hack, need to put interrupt addr into a fifo
						if (ROB->retire_ptr_out == (exec_rsp[store_q_id2] & 0x00ff)) {
							ROB->q[exec_rsp[store_q_id2] & 0x00ff].state = ROB_fault;
							csr[csr_mcause].value |= 0x00008000;										// set bit 13, instruction page fault
							csr[csr_scause].value |= 0x00008000;										// set bit 13, instruction page fault
							csr[csr_mepc].value = ROB->q[exec_rsp[store_q_id2] & 0x00ff].addr;
							csr[csr_mtval].value = csr[csr_stval].value = rd[store_q_id].data;// save offending address - ISSUE, need to drop addr from data
							csr[csr_sepc].value = ROB->q[exec_rsp[store_q_id2] & 0x00ff].addr;
							if (param->store && debug_core) {
								fprintf(debug_stream, "STORE(%d): ROB entry: 0x%02x, ", mhartid, exec_rsp[store_q_id2] & 0x00ff);
								fprint_addr_coma(debug_stream, csr[csr_mepc].value, param);
								fprintf(debug_stream, " Reg unit Page Fault addr ");
								fprint_addr_coma(debug_stream, rd[store_q_id].data, param);
								fprintf(debug_stream, " return addr ");
								fprint_addr_coma(debug_stream, csr[csr_mepc].value, param);
								fprintf(debug_stream, "  clock: 0x%04llx\n", clock);
							}
						}
						else {
							debug++;
							fprintf(debug_stream, "STORE(%d): ROB entry: 0x%02x, ERROR: Store Page Fault triggered, not ready to service 0x%04x, fault addr: 0x%016I64x return addr: 0x%016I64x clock: 0x%04llx\n",
								mhartid, exec_rsp[store_q_id2] & 0x00ff, csr[csr_mcause].value, ROB->q[exec_rsp[store_q_id2] & 0x00ff].addr, csr[csr_mepc].value, clock);
						}
					}
					else {
						fprintf(debug_stream, "STORE(%d): ROB entry: 0x%02x, ERROR: Store Page Fault triggered without servicing previous fault 0x%04x, fault addr: 0x%016I64x return addr: 0x%016I64x clock: 0x%04llx\n",
							mhartid, exec_rsp[store_q_id2] & 0x00ff, csr[csr_mcause].value, ROB->q[exec_rsp[store_q_id2] & 0x00ff].addr, csr[csr_mepc].value, clock);
						debug++;
					}
				}
				else if (exec_rsp[i]) {
					if (ROB->q[exec_rsp[i] & 0x00ff].map == STORE_map || ROB->q[exec_rsp[i] & 0x00ff].map == STORE_FP_map ||
						ROB->q[exec_rsp[i] & 0x00ff].map == AMO_map)
						ROB->q[exec_rsp[i] & 0x00ff].state = ROB_retire_out;
					else
						debug++;
				}
			}
			else if (exec_q[i].start_ptr != exec_q[i].end_ptr) {
				exec_q[i].start_ptr = ((exec_q[i].start_ptr + 1) & (exec_q[i].count - 1));
			}
		}
		while (ROB->q[exec_q[i].ROB_ptr[exec_q[i].start_ptr]].state == ROB_free && exec_q[i].start_ptr != exec_q[i].end_ptr) exec_q[i].start_ptr = ((exec_q[i].start_ptr + 1) & (exec_q[i].count - 1));
	}
}
void full_fence(ROB_Type* ROB, Reg_Table_Type* reg_table, Q_type* exec_q, reg_bus* rd, csr_type* csr, UINT8 prefetcher_idle, UINT stores_active, UINT64 clock, store_buffer_type* store_buffer, UINT debug_unit, param_type* param, FILE* debug_stream) {
	if (rd[decode_q_id].strobe) {// reg reset
		// error: need to check LOAD complete before resetting queues
		UINT8 ROB_id = (rd[branch_q_id].strobe) ? rd[branch_q_id].ROB_id : ROB->retire_ptr_out;
		for (UINT8 j = 0; j <= 0x20; j++) 	exec_q[j].end_ptr = exec_q[j].start_ptr;
		ROB->decode_ptr = ROB->allocate_ptr = ((ROB_id + 1) & 0x00ff);
		ROB->branch_stop = ROB->q[ROB_id].branch_num; // sets all new decodes to current branch on miss-predictions
		UINT8 flush_write_posters;
//		UINT8 check = fence_check( &ROB->q[ROB_id], &flush_write_posters, ROB,csr, prefetcher_idle, 1, stores_active, clock,  store_buffer, csr[csr_mhartid].value, debug_unit, param, debug_stream);
	}
}
void reg_unit(R_type* load_exec, R3_type* store_exec, branch_type* branch_exec, R_type* fp_exec, R3_type* fmul_exec, R_type* OP_exec, R_type* sys_exec, R_type* LUI_AUIPC_exec,
	Reg_Table_Type* reg_table, reg_bus* rd_JALR, reg_bus* rd, UINT16* exec_rsp, ROB_Type* ROB, Q_type* exec_q, UINT8 prefetcher_idle, UINT stores_active, UINT8 block_loads, UINT8 priviledge,
	UINT64 clock, csr_type* csr, store_buffer_type* store_buffer, UINT debug_core, param_type *param, FILE* debug_stream) {
	int debug = 0;
	UINT mhartid = csr[csr_mhartid].value;

	load_exec->strobe = 0;
	full_fence(ROB, reg_table, exec_q, rd, csr, prefetcher_idle, stores_active, clock, store_buffer, 1, param, debug_stream);

	rd_update(reg_table, rd_JALR, rd, ROB, exec_q, csr, clock, mhartid, debug_core, param, debug_stream);
	reg_unit_house_cleaning(ROB, csr, exec_q, load_exec, exec_rsp, rd, priviledge, clock, debug_core, mhartid, param, debug_stream);
	for (UINT8 i = 0; i < reg_rename_size; i++) {
		reg_table->x_reg[0].data[i] = reg_table->x_reg[0].data_H[i] = 0;
	}
	branch_exec->strobe = 0;
	if (exec_q[branch_q_id].start_ptr != exec_q[branch_q_id].end_ptr) {
		ROB_entry_Type* ROB_ptr = &ROB->q[exec_q[branch_q_id].ROB_ptr[exec_q[branch_q_id].start_ptr]];
		UINT8 rs1_strobe = (reg_table->x_reg[ROB_ptr->rs1].valid[ROB_ptr->rs1_ptr] == reg_valid_in) ? 1 : 0;
		branch_exec->rs1 = reg_table->x_reg[ROB_ptr->rs1].data[ROB_ptr->rs1_ptr];
		branch_exec->rs1_h = reg_table->x_reg[ROB_ptr->rs1].data_H[ROB_ptr->rs1_ptr];
		UINT8 rs2_strobe = (reg_table->x_reg[ROB_ptr->rs2].valid[ROB_ptr->rs2_ptr] == reg_valid_in) ? 1 : 0;
		branch_exec->rs2 = reg_table->x_reg[ROB_ptr->rs2].data[ROB_ptr->rs2_ptr];
		branch_exec->rs2_h = reg_table->x_reg[ROB_ptr->rs2].data_H[ROB_ptr->rs2_ptr];
		if (ROB_ptr->rs_count == 0 || ROB_ptr->rs_count == 1) {
			branch_exec->strobe = 3;// 3 taken, 1 nott taken
			branch_exec->ROB_id = exec_q[branch_q_id].ROB_ptr[exec_q[branch_q_id].start_ptr];
			branch_exec->uop = ROB_ptr->uop;
			branch_exec->addr = ROB_ptr->addr;
			branch_exec->addr_h = ROB_ptr->addr_H;
			branch_exec->rs2 = ROB_ptr->imm & 0x00ffffffff;
			if (param->branch && debug_core) {
				fprintf(debug_stream, "BRANCH(%lld): ROB entry: 0x%02x ", mhartid, exec_q[branch_q_id].ROB_ptr[exec_q[branch_q_id].start_ptr] );
				fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
				fprintf(debug_stream, " JALR reg unit: next addr ");
				fprint_addr_coma(debug_stream, branch_exec->rs1 + branch_exec->rs2, param);
				fprintf(debug_stream, " clk: 0x%llx\n", clock);
			}
		}
		else if (rs1_strobe && rs2_strobe) {
			branch_exec->strobe = ROB_ptr->rs3;// 3 taken, 1 nott taken
			branch_exec->ROB_id = exec_q[branch_q_id].ROB_ptr[exec_q[branch_q_id].start_ptr];
			branch_exec->uop = ROB_ptr->uop;
			branch_exec->addr = ROB_ptr->addr;
			branch_exec->addr_h = ROB_ptr->addr_H;
			branch_exec->offset = ROB_ptr->imm;
			if (param->branch && debug_core) {
				fprintf(debug_stream, "BRANCH(%lld): ROB entry: 0x%02x ", mhartid, exec_q[branch_q_id].ROB_ptr[exec_q[branch_q_id].start_ptr]);
				fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
				fprintf(debug_stream, " branch reg unit: rs1 xr%02d.%x, rs1 xr%02d.%x, taken addr ",
					ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr);
				fprint_addr_coma(debug_stream, ROB_ptr->addr + ROB_ptr->imm, param);
				fprintf(debug_stream, " clk: 0x%llx\n", clock);
			}
		}
	}
	sys_exec->strobe = 0;
	if ((ROB->q[ROB->retire_ptr_in & 0x00ff].map == SYSTEM_map ) && 
		ROB->q[ROB->retire_ptr_in & 0x00ff].state == ROB_execute) {

		ROB_entry_Type* ROB_ptr = &ROB->q[ROB->retire_ptr_in & 0x00ff];
		sys_exec->rs2 = ROB_ptr->csr;
		sys_exec->rs2_h = 0;
		if (ROB_ptr->rs_count == 1) {
			UINT8 rs1_strobe = (reg_table->x_reg[ROB_ptr->rs1].valid[ROB_ptr->rs1_ptr] == reg_valid_in) ? 1 : 0;
			sys_exec->rs1 = reg_table->x_reg[ROB_ptr->rs1].data[ROB_ptr->rs1_ptr];
			sys_exec->rs1_h = reg_table->x_reg[ROB_ptr->rs1].data_H[ROB_ptr->rs1_ptr];

			if (rs1_strobe) {
				sys_exec->strobe = 1;
				sys_exec->ROB_id = ROB->retire_ptr_in & 0x00ff;
				sys_exec->uop = ROB_ptr->uop;
				if (param->csr && debug_core) {
					fprintf(debug_stream, "SYS(%lld): ROB entry: %#04x 0x%016I64x, csr access reg unit: rs1(xr%02d.%x) = 0x%016I64x, clk: 0x%llx\n",
						mhartid, ROB->retire_ptr_in, ROB_ptr->addr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, sys_exec->rs1, clock);
				}
			}
		}
		else {
			sys_exec->rs1 = ROB_ptr->imm;
			sys_exec->rs1_h = 0;
			sys_exec->strobe = 1;
			sys_exec->ROB_id = ROB->retire_ptr_in & 0x00ff;
			sys_exec->uop = ROB_ptr->uop;
			if (param->csr && debug_core) {
				fprintf(debug_stream, "SYS(%lld): ROB entry: %#04x 0x%016I64x, csr start access reg unit: imm = 0x%3x, clk: 0x%llx\n",
					mhartid, ROB->retire_ptr_in, ROB_ptr->addr, ROB_ptr->imm, clock);
			}
		}
	}
	//UINT8 load_exec = 0;
	if (exec_q[load_q_id].start_ptr != exec_q[load_q_id].end_ptr &&  load_exec->strobe == 0 && block_loads == 0 && ROB->q[ROB->retire_ptr_out].state != ROB_fault) {
		ROB_entry_Type* ROB_ptr = &ROB->q[exec_q[load_q_id].ROB_ptr[exec_q[load_q_id].curr_ptr]];
//		ROB_entry_Type* ROB_ptr = &ROB->q[exec_q[load_q_id].ROB_ptr[exec_q[load_q_id].start_ptr]];
		if ((ROB_ptr->map == LOAD_map || ROB_ptr->map == LOAD_FP_map) ) {
			if (ROB_ptr->state == ROB_execute) {
				UINT8 rs1_strobe = (reg_table->x_reg[ROB_ptr->rs1].valid[ROB_ptr->rs1_ptr] == reg_valid_in) ? 1 : 0;
				load_exec->rs1 = reg_table->x_reg[ROB_ptr->rs1].data[ROB_ptr->rs1_ptr];
				load_exec->rs1_h = reg_table->x_reg[ROB_ptr->rs1].data_H[ROB_ptr->rs1_ptr];
				load_exec->rs2 = ROB_ptr->imm;
				load_exec->rs2_h = 0;
				if (rs1_strobe) {
					load_exec->strobe = 1;
					load_exec->ROB_id = exec_q[load_q_id].ROB_ptr[exec_q[load_q_id].curr_ptr];
					//				load_exec->ROB_id = exec_q[load_q_id].ROB_ptr[exec_q[load_q_id].start_ptr];
					load_exec->uop = ROB_ptr->uop;
					load_exec->size = ROB_ptr->funct3;
					if (param->load && debug_core) {
						fprintf(debug_stream, "LOAD(%lld): ROB entry: %#04x ", mhartid, exec_q[load_q_id].ROB_ptr[exec_q[load_q_id].curr_ptr]);
						//					fprintf(debug_stream, "LOAD(%lld): ROB entry: %#04x ", mhartid, exec_q[load_q_id].ROB_ptr[exec_q[load_q_id].start_ptr]);
						fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
						fprintf(debug_stream, " LOAD reg unit: rs1(xr%02d.%x) = 0x%016I64x, clk: 0x%llx\n",
							ROB_ptr->rs1, ROB_ptr->rs1_ptr, load_exec->rs1, clock);
					}
				}
			}
			else if (ROB_ptr->state == ROB_inflight || ROB_ptr->state == ROB_retire_in || ROB_ptr->state == ROB_retire_out) {
				exec_q[load_q_id].curr_ptr = (exec_q[load_q_id].curr_ptr + 1) & (exec_q[load_q_id].count - 1);
				exec_q[load_q_id].curr_ptr = (exec_q[load_q_id].curr_ptr == exec_q[load_q_id].end_ptr) ? exec_q[load_q_id].start_ptr : exec_q[load_q_id].curr_ptr;

			}
			else {
				debug++;
			}
		}
//		exec_q[load_q_id].curr_ptr = (exec_q[load_q_id].curr_ptr + 1) & (exec_q[load_q_id].count - 1);
//		exec_q[load_q_id].curr_ptr = (exec_q[load_q_id].curr_ptr == exec_q[load_q_id].end_ptr) ? exec_q[load_q_id].start_ptr : exec_q[load_q_id].curr_ptr;
	}
	store_exec->strobe = 0;
	if (exec_q[store_q_id].start_ptr != exec_q[store_q_id].end_ptr && block_loads == 0) {
		ROB_entry_Type* ROB_ptr = &ROB->q[exec_q[store_q_id].ROB_ptr[exec_q[store_q_id].start_ptr]];
		if (exec_q[store_q_id].ROB_ptr[exec_q[store_q_id].start_ptr] == ROB->retire_ptr_in)
			ROB_ptr->branch_num = ROB->branch_start;

		UINT8 rs1_strobe = (reg_table->x_reg[ROB_ptr->rs1].valid[ROB_ptr->rs1_ptr] == reg_valid_in) ? 1 : 0;
		store_exec->rs1 = reg_table->x_reg[ROB_ptr->rs1].data[ROB_ptr->rs1_ptr];
		store_exec->rs1_h = reg_table->x_reg[ROB_ptr->rs1].data_H[ROB_ptr->rs1_ptr];
		store_exec->rs3 = ROB_ptr->imm;
		store_exec->ROB_id = exec_q[store_q_id].ROB_ptr[exec_q[store_q_id].start_ptr];
		store_exec->uop = ROB_ptr->uop;
		store_exec->size = ROB_ptr->funct3;
		if (ROB_ptr->uop == uop_LR) {
			if (rs1_strobe) {
				store_exec->strobe = 1;
				if (param->store && debug_core) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x ", mhartid, store_exec->ROB_id);
					fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
					fprintf(debug_stream, "LR reg unit: 0x%03x(rs1 xr%02d.%x), addr ", ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
					fprint_addr_coma(debug_stream, reg_table->x_reg[ROB_ptr->rs1].data[ROB_ptr->rs1_ptr] + ROB_ptr->imm, param);
					fprintf(debug_stream, " clk: 0x%llx\n",	 clock);
				}
			}
		}
		else if (ROB_ptr->uop == uop_SC) {
			UINT8 rs2_strobe = (reg_table->x_reg[ROB_ptr->rs2].valid[ROB_ptr->rs2_ptr] == reg_valid_in) ? 1 : 0;
			store_exec->rs2 = reg_table->x_reg[ROB_ptr->rs2].data[ROB_ptr->rs2_ptr];
			store_exec->rs2_h = reg_table->x_reg[ROB_ptr->rs2].data_H[ROB_ptr->rs2_ptr];
			if (rs1_strobe && rs2_strobe) {
				store_exec->strobe = 1;
				if (param->store && debug_core) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: %#04x ", mhartid, store_exec->ROB_id);
					fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
					if (ROB_ptr->funct3 == 2) {
						fprintf(debug_stream, "SC.W reg unit: rs2 xr%02d.%x 0x%03x(rs1 xr%02d.%x), addr,data ",
							ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
						fprint_addr_coma(debug_stream, reg_table->x_reg[ROB_ptr->rs1].data[ROB_ptr->rs1_ptr] + ROB_ptr->imm, param);
						fprintf(debug_stream, "0x%08I64x clk: 0x%llx\n", reg_table->x_reg[ROB_ptr->rs2].data[ROB_ptr->rs2_ptr], clock);
					}
					else if (ROB_ptr->funct3 == 3) {
						fprintf(debug_stream, "SC.D reg unit: rs2 xr%02d.%x 0x%03x(rs1 xr%02d.%x), addr,data ",
							ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
						fprint_addr_coma(debug_stream, reg_table->x_reg[ROB_ptr->rs1].data[ROB_ptr->rs1_ptr] + ROB_ptr->imm, param);
						fprintf(debug_stream, "0x%016I64x clk: 0x%llx\n", reg_table->x_reg[ROB_ptr->rs2].data[ROB_ptr->rs2_ptr], clock);
					}

				}
			}
		}
		else if (ROB_ptr->map == STORE_map || ROB_ptr->map == AMO_map || ROB_ptr->map == MISC_MEM_map) {
			UINT8 rs2_strobe = (reg_table->x_reg[ROB_ptr->rs2].valid[ROB_ptr->rs2_ptr] == reg_valid_in) ? 1 : 0;
			store_exec->rs2 = reg_table->x_reg[ROB_ptr->rs2].data[ROB_ptr->rs2_ptr];
			store_exec->rs2_h = reg_table->x_reg[ROB_ptr->rs2].data_H[ROB_ptr->rs2_ptr];
			if (rs1_strobe && rs2_strobe) {
				store_exec->strobe = 1;
				if (param->store && debug_core) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: %#04x ", mhartid, store_exec->ROB_id);
					fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
					fprintf(debug_stream, " STORE reg unit: rs2 xr%02d.%x 0x%03x(rs1 xr%02d.%x), addr,data ",
						 ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
					fprint_addr_coma(debug_stream, reg_table->x_reg[ROB_ptr->rs1].data[ROB_ptr->rs1_ptr] + ROB_ptr->imm, param);
					fprintf(debug_stream, "0x%016I64x  clk: 0x%llx\n", reg_table->x_reg[ROB_ptr->rs2].data[ROB_ptr->rs2_ptr], clock);
				}
			}
		}
		else if (ROB_ptr->map == STORE_FP_map) {
			UINT8 rs2_strobe = (reg_table->f_reg[ROB_ptr->rs2].valid[ROB_ptr->rs2_ptr] == reg_valid_in) ? 1 : 0;
			store_exec->rs2 = reg_table->f_reg[ROB_ptr->rs2].data[ROB_ptr->rs2_ptr];
			store_exec->rs2_h = reg_table->f_reg[ROB_ptr->rs2].data_H[ROB_ptr->rs2_ptr];
			if (rs1_strobe && rs2_strobe) {
				store_exec->strobe = 1;
				if (param->store && debug_core) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: %#04x ", mhartid, store_exec->ROB_id);
					fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
					fprintf(debug_stream, " STORE FP reg unit: rs2 xr%02d.%x 0x%03x(rs1 xr%02d.%x), addr,data ",
						 ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
					fprint_addr_coma(debug_stream, reg_table->x_reg[ROB_ptr->rs1].data[ROB_ptr->rs1_ptr] + ROB_ptr->imm, param);
					fprintf(debug_stream, "0x%016I64x  clk: 0x%llx\n", reg_table->f_reg[ROB_ptr->rs2].data[ROB_ptr->rs2_ptr], clock);
				}
			}
		}
		else {
			debug++;
		}
	}
	for (UINT i = 0; i < 8; i++) {
		OP_exec[i].strobe = 0;
		if (exec_q[OP_q_id0 + i].start_ptr != exec_q[OP_q_id0 + i].end_ptr) {
			ROB_entry_Type* ROB_ptr = &ROB->q[exec_q[OP_q_id0 + i].ROB_ptr[exec_q[OP_q_id0 + i].start_ptr]];
			if (ROB_ptr->uop == uop_LUI || ROB_ptr->uop == uop_AUIPC) {
				OP_exec[i].rs1 = ((UINT64)ROB_ptr->imm) & 0x00000000ffffffff;
				OP_exec[i].rs1_h = 0;
				if (ROB_ptr->uop == uop_LUI) {
					OP_exec[i].rs2 = 0;
					OP_exec[i].rs2_h = 0;
				}
				else {
					OP_exec[i].rs2 = ROB_ptr->addr;
					OP_exec[i].rs2_h = ROB_ptr->addr_H;
				}
				OP_exec[i].strobe = 1;
				OP_exec[i].ROB_id = exec_q[OP_q_id0 + i].ROB_ptr[exec_q[OP_q_id0 + i].start_ptr];
				OP_exec[i].uop = ROB_ptr->uop;
				if (param->intx && debug_core) {
					if (ROB_ptr->uop == uop_LUI)
						fprintf(debug_stream, "INT(%lld): ROB entry: %#04x 0x%016I64x, LUI reg unit(%d): imm = 0x%08x, clk: 0x%llx\n",
							mhartid, exec_q[OP_q_id0 + i].ROB_ptr[exec_q[OP_q_id0 + i].curr_ptr], ROB_ptr->addr, i, OP_exec[i].rs1, clock);
					else
						fprintf(debug_stream, "INT(%lld): ROB entry: %#04x 0x%016I64x, AUIPC reg unit(%d): imm = 0x%08llx, addr = 0x%016I64x clk: 0x%llx\n",
							mhartid, exec_q[OP_q_id0 + i].ROB_ptr[exec_q[OP_q_id0 + i].curr_ptr], ROB_ptr->addr, i,
							OP_exec[i].rs1, OP_exec[i].rs2, clock);
				}
			}
			else if (ROB_ptr->map == OP_map || ROB_ptr->map == OP_IMM_map || ROB_ptr->map == OP_64_map || ROB_ptr->map == OP_IMM64_map ) {
				UINT8 rs1_strobe = (reg_table->x_reg[ROB_ptr->rs1].valid[ROB_ptr->rs1_ptr] == reg_valid_in) ? 1 : 0;
				OP_exec[i].rs1 = reg_table->x_reg[ROB_ptr->rs1].data[ROB_ptr->rs1_ptr];
				OP_exec[i].rs1_h = reg_table->x_reg[ROB_ptr->rs1].data_H[ROB_ptr->rs1_ptr];
				if (ROB_ptr->rs_count == 2) {
					UINT8 rs2_strobe = (reg_table->x_reg[ROB_ptr->rs2].valid[ROB_ptr->rs2_ptr] == reg_valid_in) ? 1 : 0;
					OP_exec[i].rs2 = reg_table->x_reg[ROB_ptr->rs2].data[ROB_ptr->rs2_ptr];
					OP_exec[i].rs2_h = reg_table->x_reg[ROB_ptr->rs2].data_H[ROB_ptr->rs2_ptr];
					if (rs1_strobe && rs2_strobe) {
						OP_exec[i].strobe = 1;
						OP_exec[i].ROB_id = exec_q[OP_q_id0 + i].ROB_ptr[exec_q[OP_q_id0 + i].start_ptr];
						OP_exec[i].uop = ROB_ptr->uop;
						if (param->intx && debug_core) {
							fprintf(debug_stream, "INT(%lld): ROB entry: %#04x 0x%016I64x, INT reg unit(%d): rs1(xr%02d.%x) = 0x%016I64x, rs2(xr%02d.%x) = 0x%016I64x, clk: 0x%llx\n",
								mhartid, exec_q[OP_q_id0 + i].ROB_ptr[exec_q[OP_q_id0 + i].curr_ptr], ROB_ptr->addr, i,
								ROB_ptr->rs1, ROB_ptr->rs1_ptr, OP_exec[i].rs1, ROB_ptr->rs2, ROB_ptr->rs2_ptr, OP_exec[i].rs2, clock);
						}
					}
				}
				else if (ROB_ptr->rs_count == 1) {
					OP_exec[i].rs2 = ROB->q[exec_q[OP_q_id0 + i].ROB_ptr[exec_q[OP_q_id0 + i].start_ptr]].imm;
					OP_exec[i].rs2_h = 0;
					if (rs1_strobe) {
						OP_exec[i].strobe = 1;
						OP_exec[i].ROB_id = exec_q[OP_q_id0 + i].ROB_ptr[exec_q[OP_q_id0 + i].start_ptr];
						OP_exec[i].uop = ROB_ptr->uop;
						if (param->intx && debug_core) {
							fprintf(debug_stream, "OP_int(%lld): ROB entry: %#04x 0x%016I64x, INT reg unit(%d): rs1(xr%02d.%x) = 0x%016I64x, imm = 0x%03llx, clk: 0x%llx\n",
								mhartid, exec_q[OP_q_id0 + i].ROB_ptr[exec_q[OP_q_id0 + i].curr_ptr], ROB_ptr->addr, i,
								ROB_ptr->rs1, ROB_ptr->rs1_ptr, OP_exec[i].rs1, OP_exec[i].rs2, clock);
						}
					}
				}
				else {
					debug++;
				}
			}
			else {
				debug++;
			}
		}
		fp_exec[i].strobe = 0;
		if (exec_q[fadd_q_id0 + i].start_ptr != exec_q[fadd_q_id0 + i].end_ptr) {
			ROB_entry_Type* ROB_ptr = &ROB->q[exec_q[fadd_q_id0 + i].ROB_ptr[exec_q[fadd_q_id0 + i].start_ptr]];
			if (ROB_ptr->map == OP_FP_map) {
				if (ROB_ptr->state == ROB_inflight && ROB_ptr->state == ROB_execute) {
					exec_q[fadd_q_id0 + i].start_ptr = (exec_q[fadd_q_id0 + i].start_ptr + 1) & (exec_q[fadd_q_id0 + i].count - 1);
					ROB_ptr = &ROB->q[exec_q[fadd_q_id0 + i].ROB_ptr[exec_q[fadd_q_id0 + i].start_ptr]];
				}
				UINT8 rs1_strobe = (reg_table->f_reg[ROB_ptr->rs1].valid[ROB_ptr->rs1_ptr] == reg_valid_in) ? 1 : 0;
				fp_exec[i].rs1 = reg_table->f_reg[ROB_ptr->rs1].data[ROB_ptr->rs1_ptr];
				fp_exec[i].rs1_h = reg_table->f_reg[ROB_ptr->rs1].data_H[ROB_ptr->rs1_ptr];
				fp_exec[i].size = ROB_ptr->funct7 & 3;
				if (ROB_ptr->rs_count == 2) {
					UINT8 rs2_strobe = (reg_table->f_reg[ROB_ptr->rs2].valid[ROB_ptr->rs2_ptr] == reg_valid_in) ? 1 : 0;
					fp_exec[i].rs2 = reg_table->f_reg[ROB_ptr->rs2].data[ROB_ptr->rs2_ptr];
					fp_exec[i].rs2_h = reg_table->f_reg[ROB_ptr->rs2].data_H[ROB_ptr->rs2_ptr];
					if (rs1_strobe && rs2_strobe) {
						fp_exec[i].strobe = 1;
						fp_exec[i].ROB_id = exec_q[fadd_q_id0 + i].ROB_ptr[exec_q[fadd_q_id0 + i].start_ptr];
						fp_exec[i].uop = ROB_ptr->uop;
						//						exec_strobe[fadd_q_id0 + i] = 0x8000 | exec_q[fadd_q_id0 + i].ROB_ptr[exec_q[fadd_q_id0 + i].start_ptr];
					}
				}
				else if (ROB_ptr->rs_count == 1) {
					if (ROB_ptr->uop == uop_FCVTi2f || ROB_ptr->uop == uop_FMVi2f) {
						rs1_strobe = (reg_table->x_reg[ROB_ptr->rs1].valid[ROB_ptr->rs1_ptr] == reg_valid_in) ? 1 : 0;
						fp_exec[i].rs1 = reg_table->x_reg[ROB_ptr->rs1].data[ROB_ptr->rs1_ptr];
						fp_exec[i].rs1_h = reg_table->x_reg[ROB_ptr->rs1].data_H[ROB_ptr->rs1_ptr];
					}
					if (rs1_strobe) {
						//						exec_strobe[fadd_q_id0 + i] = 0x8000 | exec_q[fadd_q_id0 + i].ROB_ptr[exec_q[fadd_q_id0 + i].start_ptr];
						fp_exec[i].strobe = 1;
						fp_exec[i].ROB_id = exec_q[fadd_q_id0 + i].ROB_ptr[exec_q[fadd_q_id0 + i].start_ptr];
						fp_exec[i].uop = ROB_ptr->uop;
					}
				}
				else {
					debug++;
				}
			}
			else {
				exec_q[fadd_q_id0 + i].start_ptr = ((exec_q[fadd_q_id0 + i].start_ptr + 1) & (exec_q[fadd_q_id0 + i].count - 1));
			}
		}
	}
	for (UINT i = 0; i < 4; i++) {
		fmul_exec[i].strobe = 0;
		if (exec_q[fmul_q_id0 + i].start_ptr != exec_q[fmul_q_id0 + i].end_ptr) {
			ROB_entry_Type* ROB_ptr = &ROB->q[exec_q[fmul_q_id0 + i].ROB_ptr[exec_q[fmul_q_id0 + i].curr_ptr]];
			if (ROB_ptr->map == MADD_map || ROB_ptr->map == MSUB_map || ROB_ptr->map == NMSUB_map || ROB_ptr->map == NMADD_map || ROB_ptr->uop == uop_FMUL) {
				if (ROB_ptr->state == ROB_inflight) {
					if (exec_q[fmul_q_id0 + i].curr_ptr == exec_q[fmul_q_id0 + i].start_ptr) {
						exec_q[fmul_q_id0 + i].start_ptr = (exec_q[fmul_q_id0 + i].start_ptr + 1) & (exec_q[fmul_q_id0 + i].count - 1);
						exec_q[fmul_q_id0 + i].curr_ptr = exec_q[fmul_q_id0 + i].start_ptr;
					}
					else {
						exec_q[fmul_q_id0 + i].curr_ptr = (exec_q[fmul_q_id0 + i].curr_ptr + 1) & (exec_q[fmul_q_id0 + i].count - 1);
						exec_q[fmul_q_id0 + i].curr_ptr = (exec_q[fmul_q_id0 + i].curr_ptr == exec_q[fmul_q_id0 + i].end_ptr) ? exec_q[fmul_q_id0 + i].start_ptr : exec_q[fmul_q_id0 + i].curr_ptr;
					}
					if (exec_q[fmul_q_id0 + i].start_ptr == exec_q[fmul_q_id0 + i].end_ptr) {
						debug++;
					}
					ROB_ptr = &ROB->q[exec_q[fmul_q_id0 + i].ROB_ptr[exec_q[fmul_q_id0 + i].curr_ptr]];
				}
				UINT8 rs1_strobe = (reg_table->f_reg[ROB_ptr->rs1].valid[ROB_ptr->rs1_ptr] == reg_valid_in) ? 1 : 0;
				fmul_exec[i].rs1 = reg_table->f_reg[ROB_ptr->rs1].data[ROB_ptr->rs1_ptr];
				fmul_exec[i].rs1_h = reg_table->f_reg[ROB_ptr->rs1].data_H[ROB_ptr->rs1_ptr];

				UINT8 rs2_strobe = (reg_table->f_reg[ROB_ptr->rs2].valid[ROB_ptr->rs2_ptr] == reg_valid_in) ? 1 : 0;
				fmul_exec[i].rs2 = reg_table->f_reg[ROB_ptr->rs2].data[ROB_ptr->rs2_ptr];
				fmul_exec[i].rs2_h = reg_table->f_reg[ROB_ptr->rs2].data_H[ROB_ptr->rs2_ptr];

				fmul_exec[i].ROB_id = 0x8000 | exec_q[fmul_q_id0 + i].ROB_ptr[exec_q[fmul_q_id0 + i].curr_ptr];
				fmul_exec[i].uop = ROB_ptr->uop;
				fmul_exec[i].size = ROB_ptr->funct7 & 3;
				if (ROB_ptr->rs_count == 3) {
					UINT strobe = (reg_table->f_reg[ROB_ptr->rs3].valid[ROB_ptr->rs3_ptr] == reg_valid_in) ? 1 : 0;
					fmul_exec[i].rs3 = reg_table->f_reg[ROB_ptr->rs3].data[ROB_ptr->rs3_ptr];
					fmul_exec[i].rs3_h = reg_table->f_reg[ROB_ptr->rs3].data_H[ROB_ptr->rs3_ptr];
					if (rs1_strobe && rs2_strobe && strobe) {
						fmul_exec[i].strobe = 1;
						if (param->fp && debug_core) {
							fprintf(debug_stream, "FMADD(%lld): ROB entry: %#04x 0x%016I64x, FP reg unit(%d): rs1 xr%02d.%x rs2 xr%02d.%x rs3 xr%02d.%x clk: 0x%llx\n",
								mhartid, exec_q[fmul_q_id0 + i].ROB_ptr[exec_q[fmul_q_id0 + i].curr_ptr], ROB_ptr->addr, i, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB_ptr->rs3, ROB_ptr->rs3_ptr, clock);
						}
					}
				}
				else {
					if (rs1_strobe && rs2_strobe) {
						fmul_exec[i].strobe = 1;
						if (param->fp && debug_core) {
							fprintf(debug_stream, "FMADD(%lld): ROB entry: %#04x 0x%016I64x, FMUL reg unit(%d): rs1 xr%02d.%x rs2 xr%02d.%x clk: 0x%llx\n",
								mhartid, exec_q[fmul_q_id0 + i].ROB_ptr[exec_q[fmul_q_id0 + i].curr_ptr], ROB_ptr->addr, i, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr, clock);
						}
					}
				}
			}
			exec_q[fmul_q_id0 + i].curr_ptr = ((exec_q[fmul_q_id0 + i].curr_ptr + 1) & (exec_q[fmul_q_id0 + i].count - 1));
			exec_q[fmul_q_id0 + i].curr_ptr = (exec_q[fmul_q_id0 + i].curr_ptr == exec_q[fmul_q_id0 + i].end_ptr) ? exec_q[fmul_q_id0 + i].start_ptr : exec_q[fmul_q_id0 + i].curr_ptr;
		}
	}
}
