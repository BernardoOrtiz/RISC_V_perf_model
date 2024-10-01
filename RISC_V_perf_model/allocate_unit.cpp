// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "ROB.h"

UINT8 allocate_f_rd(ROB_entry_Type* ROB_ptr, Reg_Table_Type* reg_table) {
	char error_code = 0;
	reg_table->f_reg[ROB_ptr->rd].current_ptr = ((reg_table->f_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1));
	ROB_ptr->rd_ptr = reg_table->f_reg[ROB_ptr->rd].current_ptr;
	if (reg_table->f_reg[ROB_ptr->rd].valid[reg_table->f_reg[ROB_ptr->rd].current_ptr] != reg_free)
		error_code = 1;;
	reg_table->f_reg[ROB_ptr->rd].valid[reg_table->f_reg[ROB_ptr->rd].current_ptr] = reg_allocated;

	return error_code;
}
void allocate_x_rd(ROB_entry_Type* ROB_ptr, Reg_Table_Type* reg_table, UINT16 index) {
	int debug = 0;
	x_reg_type* reg = &reg_table->x_reg[ROB_ptr->rd];

	reg->current_ptr = ((reg->current_ptr + 1) & (reg_rename_size - 1));
	if (reg->current_ptr == reg->retire_ptr)
		debug++; //	register overrun
	ROB_ptr->rd_ptr = reg->current_ptr;
	reg->valid[reg->current_ptr] = reg_allocated;

	reg->ROB_ptr[reg->current_ptr] = index;

}
// need allocator table
//	* y-axis: reg numbre
//	* x-axis: reg version
//	* content: ROB entry number
// select version by:
// column 1 < column 2: column 1 < x < column 2
// column 1 > column 2: x < column 1 || x > column 2
// 
// 4 entries per reg; retire pointer clears entery -1 (ie at most 3 valid entries at a time)
//
void allocator_iA(Q_type* exec_q, ROB_Type* ROB, UINT8 load_PC, UINT8* fadd_q_id, UINT8* fmac_q_id, Reg_Table_Type* reg_table,
	UINT64 clock, UINT mhartid, UINT debug_core, param_type *param, FILE* debug_stream) {
	int debug = 0;

	if (mhartid == 0) {
		if (clock >= 0x18f0)
			debug++;
	}

	UINT debug_unit = (param->allocator == 1) && debug_core;

	UINT8 dst_reg_block[0x20];
	for (UINT8 i = 0; i < 0x20; i++) dst_reg_block[i] = 0;
	UINT8 stall = 0;

	if (load_PC == 4) {
		ROB->allocate_ptr = ROB->retire_ptr_in;// reset ROB pointers
		for (UINT8 j = 0; j < 0x20; j++) 	exec_q[j].end_ptr = exec_q[j].start_ptr;
		ROB->branch_stop = ROB->branch_start;
	}
	else if (load_PC && load_PC != 7 && load_PC != 0x20) {
		if (exec_q[load_q_id].end_ptr == exec_q[load_q_id].start_ptr) {
			ROB->fence = 0;
			ROB->branch_stop = ROB->branch_start;
			if (debug_unit) {
				fprintf(debug_stream, "ALLOCATE(%lld): alloc reset due to branch clock 0x%04llx\n", mhartid, clock);
			}
		}
	}
	else {
		for (UINT j = ROB->allocate_ptr, alloc_count = 0; j != ROB->decode_ptr && !ROB->fence && j == ROB->allocate_ptr & !stall && alloc_count < param->decode_width; j = ((j + 1) & 0x00ff), alloc_count++) {
			ROB_entry_Type* ROB_ptr = &ROB->q[j];
			x_reg_type* reg = &reg_table->x_reg[ROB_ptr->rd];
			if (j == ROB->retire_ptr_out) {
				if (((reg_table->x_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->x_reg[ROB_ptr->rd].retire_ptr) {
					if (debug_unit) {
						fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x retuop_SC: 0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB_ptr->addr);
						print_uop(debug_stream, ROB->q[ROB->allocate_ptr], reg_table, clock);
						fprintf(debug_stream, "ERROR: top of retire pointer, clear register overflow clock 0x%04llx\n", clock);
					}
					reg_table->x_reg[ROB_ptr->rd].retire_ptr = reg_table->x_reg[ROB_ptr->rd].current_ptr;
				}
			}
			if (((reg_table->x_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->x_reg[ROB_ptr->rd].retire_ptr && ROB_ptr->rd != 0) {
				if (debug_unit) {
					ROB_ptr->rd_ptr = reg_table->x_reg[ROB_ptr->rd].current_ptr;
					switch (ROB_ptr->rs_count) {
					case 1:
						ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
						break;
					case 2:
						ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
						ROB_ptr->rs2_ptr = reg_table->x_reg[ROB_ptr->rs2].current_ptr;
						break;
					case 3:
						ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
						ROB_ptr->rs2_ptr = reg_table->x_reg[ROB_ptr->rs2].current_ptr;
						ROB_ptr->rs3_ptr = reg_table->x_reg[ROB_ptr->rs3].current_ptr;
						break;
					default:
						break;
					}
					fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
					print_uop(debug_stream, ROB->q[ROB->allocate_ptr], reg_table, clock);
					fprintf(debug_stream, "STALL: register overflow clock 0x%04llx\n", clock);
				}
				stall = 1;
			}
			else {
				if (ROB->retire_ptr_in == ROB->allocate_ptr && j == ROB->allocate_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2) {
					switch (ROB_ptr->map) {
					case SYSTEM_map: {
						ROB->fence = 1;
						ROB_ptr->rs_count = 0;
						switch (ROB_ptr->uop) {
						case uop_CSRRW:
						case uop_CSRRS:
						case uop_CSRRC:
							ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
							ROB_ptr->rs_count = 1;
						case uop_CSRRWI:
						case uop_CSRRSI:
						case uop_CSRRCI:
							ROB_ptr->rd_ptr = reg_table->x_reg[ROB_ptr->rd].current_ptr; // fenced operations, no need to increment pointer

							ROB->q[ROB->allocate_ptr].state = ROB_execute; // issue instruction, so that execute unit can see activity
							if (debug_unit) {
								fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
								print_uop(debug_stream, ROB->q[ROB->allocate_ptr], reg_table, clock);
								fprintf(debug_stream, "clock 0x%04llx\n", clock);
							}
							break;
						case uop_MRET:
							ROB->q[ROB->allocate_ptr].state = ROB_execute; // issue instruction, so that execute unit can see activity
							if (debug_unit) {
								fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
								fprintf(debug_stream, "MRET ");
								fprintf(debug_stream, "clock 0x%04llx\n", clock);
							}
							break;
						case uop_SRET:
							ROB->q[ROB->allocate_ptr].state = ROB_execute; // issue instruction, so that execute unit can see activity
							if (debug_unit) {
								fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
								fprintf(debug_stream, "SRET ");
								fprintf(debug_stream, "clock 0x%04llx\n", clock);
							}
							break;
						case uop_WFI:
							//						fence = 1;
							ROB->q[ROB->allocate_ptr].state = ROB_retire_out; // issue instruction, so that execute unit can see activity
							if (debug_unit) {
								fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
								fprintf(debug_stream, "WFI ");
								fprintf(debug_stream, "clock 0x%04llx\n", clock);
							}
							break;
						case uop_HALT:
							//						fence = 1;
							ROB->q[ROB->allocate_ptr].state = ROB_retire_out; // issue instruction, so that execute unit can see activity
							if (debug_unit) {
								fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
								fprintf(debug_stream, "HALT ");
								fprintf(debug_stream, "clock 0x%04llx\n", clock);
							}
							break;
						case uop_ECALL:
							ROB->q[ROB->allocate_ptr].state = ROB_execute; // issue instruction, so that execute unit can see activity
							if (ROB->q[ROB->allocate_ptr].rs1 != 0) {
								ROB->q[ROB->allocate_ptr].imm = reg_table->x_reg[ROB->q[ROB->allocate_ptr].rs1].data[reg_table->x_reg[ROB_ptr->rs1].current_ptr];
							}
							if (debug_unit) {
								fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
								fprintf(debug_stream, "ECALL ");
								fprintf(debug_stream, "clock 0x%04llx\n", clock);
							}
							break;
						default:
							debug++;
							break;
						}
					}
								   break;
					case AMO_map:
						if (((exec_q[store_q_id].end_ptr + 1) & (exec_q[store_q_id].count - 1)) != exec_q[store_q_id].start_ptr) {
							ROB->q[ROB->allocate_ptr].state = ROB_execute; // issue instruction, so that execute unit can see activity
							ROB->q[ROB->allocate_ptr].q_select = store_q_select; // issue instruction, so that execute unit can see activity
							exec_q[store_q_id].ROB_ptr[exec_q[store_q_id].end_ptr] = ROB->allocate_ptr;
							exec_q[store_q_id].ROB_ptr_valid[exec_q[store_q_id].end_ptr] = 1;
							exec_q[store_q_id].end_ptr = ((exec_q[store_q_id].end_ptr + 1) & (exec_q[store_q_id].count - 1));

							ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
							ROB_ptr->rs2_ptr = reg_table->x_reg[ROB_ptr->rs2].current_ptr;
							allocate_x_rd(ROB_ptr, reg_table, j);

							dst_reg_block[ROB_ptr->rd] = 1;
							if (debug_unit) {
								fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr);
								fprint_addr(debug_stream, ROB_ptr->addr, param);
								fprintf(debug_stream, "AMO rd = xr%02d.%x, rs1 = x%d.%d, rs2 = x%d.%d, ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr);
								fprint_addr(debug_stream, reg_table->x_reg[ROB_ptr->rs1].data[ROB_ptr->rs1_ptr], param);
								fprintf(debug_stream, "// store_id: 0x%02x-0x%02x, clock 0x%04llx\n", exec_q[store_q_id].start_ptr, exec_q[store_q_id].end_ptr, clock);
							}
						}
						else {
							alloc_count = param->decode_width;
						}
						break;
					case JAL_map:
					{
						ROB->q[ROB->allocate_ptr].state = ROB_retire_out; // issue instruction, so that execute unit can see activity
						if (ROB_ptr->rd == 1 || ROB_ptr->rd == 5) {
							ROB_ptr->rd_ptr = reg_table->x_reg[ROB_ptr->rd].current_ptr;
						}
						else {
							allocate_x_rd(ROB_ptr, reg_table, j);
						}
						if (debug_unit) {
							fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
							fprintf(debug_stream, "JAL ");
							fprintf(debug_stream, "rd: x%02d.%d, clock 0x%04llx\n", ROB_ptr->rd, ROB_ptr->rd_ptr, clock);
						}
					}
					break;
					case JALR_map:
					{
						ROB->q[ROB->allocate_ptr].state = ROB_retire_out;
						ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
						if (ROB_ptr->rd == 1 || ROB_ptr->rd == 5) {
							ROB_ptr->rd_ptr = reg_table->x_reg[ROB_ptr->rd].current_ptr;
						}
						else {
							allocate_x_rd(ROB_ptr, reg_table, j);
						}
						if (debug_unit) {
							fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
							fprintf(debug_stream, "JALR ");
							fprintf(debug_stream, "rd: x%02d.%d, rs1: x%02d.%d, clock 0x%04llx\n", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, clock);
						}
					}
					break;
					case BRANCH_map:
					{
						if (((ROB->branch_stop + 1) & 3) != ROB->branch_start) {
							if (debug_unit) {
								fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
								fprintf(debug_stream, "BRANCH ");
								fprintf(debug_stream, "rs1: x%02d.%d, rs2: x%02d.%d, clock 0x%04llx\n", ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr, clock);
							}
							ROB_ptr->state = ROB_execute;
						}
						else {
							alloc_count = param->decode_width; // stop allocating
						}
					}
					break;
					default:
					{
						switch (ROB_ptr->q_select) {
						case iMUL_q_select:
							exec_q[iMUL_q_id].ROB_ptr[exec_q[iMUL_q_id].end_ptr] = j;
							exec_q[iMUL_q_id].ROB_ptr_valid[exec_q[iMUL_q_id].end_ptr] = 1;
							ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
							ROB_ptr->rs2_ptr = reg_table->x_reg[ROB_ptr->rs2].current_ptr;
							if (ROB_ptr->rs_count == 3)
								ROB_ptr->rs3_ptr = reg_table->x_reg[ROB_ptr->rs3].current_ptr;

							ROB_ptr->state = ROB_execute;
							exec_q[iMUL_q_id].end_ptr = ((exec_q[iMUL_q_id].end_ptr + 1) & (exec_q[iMUL_q_id].count - 1));
							allocate_x_rd(ROB_ptr, reg_table, j);
							if (debug_unit) {
								fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
								fprintf(debug_stream, "integer MUL rd = xr%02d.%x, ", ROB_ptr->rd, ROB_ptr->rd_ptr);
								fprintf(debug_stream, "rs1 = x%02d.%x, ", ROB_ptr->rs1, ROB_ptr->rs1_ptr);
								fprintf(debug_stream, "rs2 = x%02d.%x, ", ROB_ptr->rs2, ROB_ptr->rs2_ptr);
								if (ROB_ptr->rs_count == 3)
									fprintf(debug_stream, "rs3 = x%02d.%x, ", ROB_ptr->rs3, ROB_ptr->rs3_ptr);
								fprintf(debug_stream, "clock 0x%04llx\n", clock);
							}
							break;
						case OP_q_select0:
						case OP_q_select1:
						case OP_q_select2:
						case OP_q_select3:
						case OP_q_select4:
						case OP_q_select5:
						case OP_q_select6:
						case OP_q_select7: {
							UINT OP_q_id = OP_q_id0;
							UINT8 q_id = 0;
							switch (ROB_ptr->q_select) {
							case OP_q_select0:
								OP_q_id = OP_q_id0;
								q_id = 0;
								break;
							case OP_q_select1:
								OP_q_id = OP_q_id1;
								q_id = 1;
								break;
							case OP_q_select2:
								OP_q_id = OP_q_id2;
								q_id = 2;
								break;
							case OP_q_select3:
								OP_q_id = OP_q_id3;
								q_id = 3;
								break;
							case OP_q_select4:
								OP_q_id = OP_q_id4;
								q_id = 4;
								break;
							case OP_q_select5:
								OP_q_id = OP_q_id5;
								q_id = 5;
								break;
							case OP_q_select6:
								OP_q_id = OP_q_id6;
								q_id = 6;
								break;
							case OP_q_select7:
								OP_q_id = OP_q_id7;
								q_id = 7;
								break;
							default:
								debug++;
								break;
							}
							exec_q[OP_q_id].ROB_ptr[exec_q[OP_q_id].end_ptr] = j;
							exec_q[OP_q_id].ROB_ptr_valid[exec_q[OP_q_id].end_ptr] = 1;
							ROB_ptr->state = ROB_execute;

							ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
							ROB_ptr->rs2_ptr = reg_table->x_reg[ROB_ptr->rs2].current_ptr;
							exec_q[OP_q_id].end_ptr = ((exec_q[OP_q_id].end_ptr + 1) & (exec_q[OP_q_id].count - 1));
							allocate_x_rd(ROB_ptr, reg_table, j);
							if (debug_unit) {
								fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
								fprintf(debug_stream, "INTEGER%d xr%02d.%x, ", q_id, ROB_ptr->rd, ROB_ptr->rd_ptr);
								fprintf(debug_stream, "xr%02d.%x, ", ROB_ptr->rs1, ROB_ptr->rs1_ptr);
								if (ROB_ptr->rs_count == 1)
									fprintf(debug_stream, "imm 0x%x, ", ROB_ptr->imm);// can be 12b or 20b(JALR) or 32b(LUI)
								else
									fprintf(debug_stream, "xr%02d.%x, ", ROB_ptr->rs2, ROB_ptr->rs2_ptr);

								if (ROB_ptr->rs_count == 3)
									fprintf(debug_stream, "xr%02d.%x, ", ROB_ptr->rs3, ROB_ptr->rs3_ptr);
								fprintf(debug_stream, "clock 0x%04llx\n", clock);
							}
						}
										 break;
						case LUI_AUIPC_q_select: {
							//							exec_q[LUI_AUIPC_q_id].ROB_ptr[exec_q[LUI_AUIPC_q_id].end_ptr] = j;
							//							exec_q[LUI_AUIPC_q_id].ROB_ptr_valid[exec_q[LUI_AUIPC_q_id].end_ptr] = 1;

							UINT OP_q_id = OP_q_id0;
							UINT8 q_id = 0;
							switch (ROB_ptr->q_select) {
							case OP_q_select0:
								OP_q_id = OP_q_id0;
								q_id = 0;
								break;
							case OP_q_select1:
								OP_q_id = OP_q_id1;
								q_id = 1;
								break;
							case OP_q_select2:
								OP_q_id = OP_q_id2;
								q_id = 2;
								break;
							case OP_q_select3:
								OP_q_id = OP_q_id3;
								q_id = 3;
								break;
							case OP_q_select4:
								OP_q_id = OP_q_id4;
								q_id = 4;
								break;
							case OP_q_select5:
								OP_q_id = OP_q_id5;
								q_id = 5;
								break;
							case OP_q_select6:
								OP_q_id = OP_q_id6;
								q_id = 6;
								break;
							case OP_q_select7:
								OP_q_id = OP_q_id7;
								q_id = 7;
								break;
							default:
								debug++;
								break;
							}
							exec_q[OP_q_id].ROB_ptr[exec_q[OP_q_id].end_ptr] = j;
							exec_q[OP_q_id].ROB_ptr_valid[exec_q[OP_q_id].end_ptr] = 1;
							ROB_ptr->state = ROB_execute;
							exec_q[OP_q_id].end_ptr = ((exec_q[OP_q_id].end_ptr + 1) & (exec_q[OP_q_id].count - 1));
							//							exec_q[LUI_AUIPC_q_id].end_ptr = ((exec_q[LUI_AUIPC_q_id].end_ptr + 1) & (exec_q[LUI_AUIPC_q_id].count - 1));
							allocate_x_rd(ROB_ptr, reg_table, j);
							if (debug_unit) {
								fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
								fprintf(debug_stream, "LUI_AUIPC rd = xr%02d.%x, ", ROB_ptr->rd, ROB_ptr->rd_ptr);
								fprintf(debug_stream, "clock 0x%04llx\n", clock);
							}
						}
											   break;
						case load_q_select: {
							if (ROB_ptr->reg_type == 3) {// control status register accesses are implied fence operations
								exec_q[load_q_id].ROB_ptr[exec_q[load_q_id].end_ptr] = j;
								exec_q[load_q_id].ROB_ptr_valid[exec_q[load_q_id].end_ptr] = 1;
								exec_q[load_q_id].state[exec_q[load_q_id].end_ptr] = 1;
								ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->state = ROB_execute;
								if (debug_unit) {
									fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
									fprintf(debug_stream, "LOAD rd = xr%02d.%x, %d(x%02d.%d) ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
									fprintf(debug_stream, "start-stop ptr 0x%02x-0x%02x, ", exec_q[load_q_id].start_ptr, exec_q[load_q_id].end_ptr);
									fprintf(debug_stream, "clock 0x%04llx\n", clock);
								}
								exec_q[load_q_id].end_ptr = ((exec_q[load_q_id].end_ptr + 1) & (exec_q[load_q_id].count - 1));
							}
							else if (ROB_ptr->uop == uop_F_LOAD) {// floating point reg_tableisters
								if (allocate_f_rd(ROB_ptr, reg_table)) {
									//											if (debug_unit) {
									fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
									fprintf(debug_stream, "Performance Bug: F_LOAD rd overload stall, fp%02d.%x, 0x%3x (xr%02d.%x) //", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
									fprintf(debug_stream, "rd current/retire %x/%x, ", reg_table->f_reg[ROB_ptr->rd].current_ptr, reg_table->f_reg[ROB_ptr->rd].retire_ptr);
									fprintf(debug_stream, "start-stop ptr 0x%02x-0x%02x, ", exec_q[load_q_id].start_ptr, exec_q[load_q_id].end_ptr);
									fprintf(debug_stream, "clock 0x%04llx\n", clock);
									//											}
								}
								else {
									exec_q[load_q_id].ROB_ptr[exec_q[load_q_id].end_ptr] = j;
									exec_q[load_q_id].ROB_ptr_valid[exec_q[load_q_id].end_ptr] = 1;
									exec_q[load_q_id].state[exec_q[load_q_id].end_ptr] = 1;
									if (ROB_ptr->rd == 0x1e && mhartid == 0)
										debug++;
									ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
									ROB_ptr->state = ROB_execute;
									allocate_f_rd(ROB_ptr, reg_table);
									if (debug_unit) {
										fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
										fprintf(debug_stream, "FLOAD rd = fp%02d.%x, %d(x%02d.%d) ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
										fprintf(debug_stream, "start-stop ptr 0x%02x-0x%02x, ", exec_q[load_q_id].start_ptr, exec_q[load_q_id].end_ptr);
										fprintf(debug_stream, "clock 0x%04llx\n", clock);
									}
									exec_q[load_q_id].end_ptr = ((exec_q[load_q_id].end_ptr + 1) & (exec_q[load_q_id].count - 1));
								}
							}
							else {
								exec_q[load_q_id].ROB_ptr[exec_q[load_q_id].end_ptr] = j;
								exec_q[load_q_id].ROB_ptr_valid[exec_q[load_q_id].end_ptr] = 1;
								exec_q[load_q_id].state[exec_q[load_q_id].end_ptr] = 1;
								ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->state = ROB_execute;
								allocate_x_rd(ROB_ptr, reg_table, j);
								if (debug_unit) {
									fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
									fprintf(debug_stream, "LOAD rd = xr%02d.%x, %d(x%02d.%d) ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
									fprintf(debug_stream, "start-stop ptr 0x%02x-0x%02x, ", exec_q[load_q_id].start_ptr, exec_q[load_q_id].end_ptr);
									fprintf(debug_stream, "clock 0x%04llx\n", clock);
								}
								exec_q[load_q_id].end_ptr = ((exec_q[load_q_id].end_ptr + 1) & (exec_q[load_q_id].count - 1));
							}
						}
										  break;
						case store_q_select:
							if (((exec_q[store_q_id].end_ptr + 1) & (exec_q[store_q_id].count - 1)) != exec_q[store_q_id].start_ptr) {
								exec_q[store_q_id].ROB_ptr[exec_q[store_q_id].end_ptr] = j;
								exec_q[store_q_id].ROB_ptr_valid[exec_q[store_q_id].end_ptr] = 1;
								exec_q[store_q_id].end_ptr = ((exec_q[store_q_id].end_ptr + 1) & (exec_q[store_q_id].count - 1));

								ROB_ptr->rd = 0;
								ROB_ptr->rd_ptr = 0;
								ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->state = ROB_execute;
								if (debug_unit) {
									fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr);
									fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
								}
								if (ROB_ptr->uop == uop_F_STORE) {
									ROB_ptr->rs2_ptr = reg_table->f_reg[ROB_ptr->rs2].current_ptr;
									// error, need to allocate rs2 float
									if (debug_unit) {
										fprintf(debug_stream, "F_STORE ");
										fprintf(debug_stream, "x%02d.%d, fp%02d.%x, %#05x // ", ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB_ptr->imm);
									}
								}
								else {
									ROB_ptr->rs2_ptr = reg_table->x_reg[ROB_ptr->rs2].current_ptr;
									if (debug_unit) {
										switch (param->mxl) {
										case 0:
											fprintf(debug_stream, "SB ");
											break;
										case 1:
											fprintf(debug_stream, "SH ");
											break;
										case 2:
											fprintf(debug_stream, "SW ");
											break;
										case 3:
											fprintf(debug_stream, "SD ");
											break;
										case 4:
											fprintf(debug_stream, "SQ ");
											break;
										default:
											debug++;
											fprintf(debug_stream, "STORE ");
											break;
										}
										fprintf(debug_stream, "x%02d.%d, x%02d.%d, %#05x // ", ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB_ptr->imm);
									}
								}
								if (debug_unit) {
//									fprintf(debug_stream, "branch_num: %d %d-%d, store q (start/stop):%#x/%#x clock 0x%04llx\n", ROB_ptr->branch_num, ROB->branch_start, ROB->branch_stop, exec_q[store_q_id].start_ptr, exec_q[store_q_id].end_ptr, clock);
									fprintf(debug_stream, "store q (start/stop):%#x/%#x clock 0x%04llx\n", 
										exec_q[store_q_id].start_ptr, exec_q[store_q_id].end_ptr, clock);
								}
							}
							else {// avoid q overflow; stall machine
								alloc_count = param->decode_width;
							}
							break;
						case op_fp_select:
							switch (ROB_ptr->uop) {
							case uop_FCVTi2f: {
								fadd_q_id[0]++;
								fadd_q_id[0] &= (param->decode_width - 1);
								fadd_q_id[0] |= fadd_q_id0;

								exec_q[fadd_q_id[0]].start_ptr = exec_q[fadd_q_id[0]].end_ptr;

								exec_q[fadd_q_id[0]].ROB_ptr[exec_q[fadd_q_id[0]].end_ptr] = j;
								exec_q[fadd_q_id[0]].ROB_ptr_valid[exec_q[fadd_q_id[0]].end_ptr] = 1;
								if (ROB_ptr->rd == 0x1e && mhartid == 0)
									debug++;
								allocate_f_rd(ROB_ptr, reg_table);
								ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs_count = 1;
								if (debug_unit) {
									fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
									fprintf(debug_stream, "FCVTi2f uint %d ", fadd_q_id[0] & (param->decode_width - 1));
									fprintf(debug_stream, "fp%02d.%x, x%02d.%d // ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
									fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fadd_q_id[0]].end_ptr, clock);
								}
								exec_q[fadd_q_id[0]].end_ptr = ((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id1].count - 1));
							}
											break;
							case uop_FMVi2f: {
								fadd_q_id[0]++;
								fadd_q_id[0] &= (param->decode_width - 1);
								fadd_q_id[0] |= fadd_q_id0;
								exec_q[fadd_q_id[0]].start_ptr = exec_q[fadd_q_id[0]].end_ptr;

								exec_q[fadd_q_id[0]].ROB_ptr[exec_q[fadd_q_id[0]].end_ptr] = j;
								exec_q[fadd_q_id[0]].ROB_ptr_valid[exec_q[fadd_q_id[0]].end_ptr] = 1;
								if (ROB_ptr->rd == 0x1e && mhartid == 0)
									debug++;
								allocate_f_rd(ROB_ptr, reg_table);
								ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs_count = 1;
								if (debug_unit) {
									fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
									fprintf(debug_stream, "FMVi2f.");
									switch (ROB_ptr->funct7 & 3 == 0) {
									case 0:
										fprintf(debug_stream, "s ");//32b
										break;
									case 1:
										fprintf(debug_stream, "d ");//64b
										break;
									case 3:
										fprintf(debug_stream, "q ");//128b
										break;
									case 2:
										fprintf(debug_stream, "h ");//16b
										break;
									default:
										debug++;
										break;
									}
									fprintf(debug_stream, "fp%02d.%x, x%02d.%d // ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
									fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fadd_q_id[0]].end_ptr, clock);
								}
								exec_q[fadd_q_id[0]].end_ptr = ((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1));
							}
										   break;
							case uop_FCVTf2i: {
								fadd_q_id[0]++;
								fadd_q_id[0] &= (param->decode_width - 1);
								fadd_q_id[0] |= fadd_q_id0;
								exec_q[fadd_q_id[0]].start_ptr = exec_q[fadd_q_id[0]].end_ptr;

								exec_q[fadd_q_id[0]].ROB_ptr[exec_q[fadd_q_id[0]].end_ptr] = j;
								exec_q[fadd_q_id[0]].ROB_ptr_valid[exec_q[fadd_q_id[0]].end_ptr] = 1;
								allocate_x_rd(ROB_ptr, reg_table, j);
								ROB_ptr->rs1_ptr = reg_table->f_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs_count = 1;
								if (debug_unit) {
									fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
									//							fprintf(debug_stream, "FCVTf2i ");
									fprintf(debug_stream, "FCVTf2i uint %d ", fadd_q_id[0] & (param->decode_width - 1));
									fprintf(debug_stream, "fp%02d.%x, x%02d.%d // ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
									fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fadd_q_id[0]].end_ptr, clock);
								}
								exec_q[fadd_q_id[0]].end_ptr = ((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1));
								ROB_ptr->state = ROB_execute;
							}
											break;
											//							default:
											//								debug++;
											//								break;
							}
							break;
						}
					}
					break;
					}
				}
				else if (ROB_ptr->map == SYSTEM_map) {
					j = ((ROB->decode_ptr - 1) & 0x00ff);
				}
				// x_reg_type* reg = &reg_table->x_reg[ROB_ptr->rd];
				else if (ROB_ptr->rd != 0 && reg->valid[0] != reg_free && reg->valid[1] != reg_free && reg->valid[2] != reg_free && reg->valid[3] != reg_free &&
					reg->valid[4] != reg_free && reg->valid[5] != reg_free && reg->valid[6] != reg_free && reg->valid[7] != reg_free &&
					reg->valid[8] != reg_free && reg->valid[9] != reg_free && reg->valid[10] != reg_free && reg->valid[11] != reg_free &&
					reg->valid[12] != reg_free && reg->valid[13] != reg_free && reg->valid[14] != reg_free && reg->valid[15] != reg_free &&
					(ROB_ptr->map == OP_map || ROB_ptr->map == OP_IMM_map || ROB_ptr->map == OP_64_map || ROB_ptr->map == OP_IMM64_map || ROB_ptr->map == SYSTEM_map || ROB_ptr->map == AMO_map || ROB_ptr->map == LOAD_map || ROB_ptr->map == AUIPC_map || ROB_ptr->map == LUI_map)) {
					debug++;
					j = (ROB->decode_ptr - 1) & 0x00ff;
				}
				else {
					switch (ROB_ptr->map) {
					case JAL_map:
						if (!(((reg_table->x_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->x_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {
							ROB->q[ROB->allocate_ptr].state = ROB_retire_out; // issue instruction, so that execute unit can see activity
							if (ROB_ptr->rd == 1 || ROB_ptr->rd == 5) {
								ROB_ptr->rd_ptr = reg_table->x_reg[ROB_ptr->rd].current_ptr;
							}
							else {
								allocate_x_rd(ROB_ptr, reg_table, j);
							}
							if (debug_unit) {
								fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
								fprintf(debug_stream, "JAL 2 ");
								fprintf(debug_stream, "rd: x%02d.%d, clock 0x%04llx\n", ROB_ptr->rd, ROB_ptr->rd_ptr, clock);
							}
						}
						else {
							j = (ROB->decode_ptr - 1) & 0x00ff;
						}
						break;
					case JALR_map:
						if (!(((reg_table->x_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->x_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {
							ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
							if (ROB_ptr->rd == 1 || ROB_ptr->rd == 5) {
								ROB_ptr->rd_ptr = reg_table->x_reg[ROB_ptr->rd].current_ptr;
							}
							else {
								allocate_x_rd(ROB_ptr, reg_table, j);
							}
							if (debug_unit) {
								fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
								fprintf(debug_stream, "JALR ");
								fprintf(debug_stream, "rd: x%02d.%d, rs1: x%02d.%d, imm  0x%08x // addr? 0x%08x clock 0x%04llx\n", 
									ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->imm, ROB_ptr->imm+ROB_ptr->addr, clock);
							}
								if (mhartid == 0)
									debug++;
								ROB_ptr->state = ROB_execute;
								exec_q[branch_q_id].ROB_ptr[exec_q[branch_q_id].end_ptr] = j;
								exec_q[branch_q_id].end_ptr = ((exec_q[branch_q_id].end_ptr + 1) & 3);
						}
						else {
							j = (ROB->decode_ptr - 1) & 0x00ff;
						}
						break;
					case BRANCH_map: {
						if (((exec_q[branch_q_id].end_ptr + 1) & 3) != exec_q[branch_q_id].start_ptr &&
							((exec_q[branch_q_id].end_ptr + 2) & 3) != exec_q[branch_q_id].start_ptr &&
							!(((reg_table->x_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->x_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {

							exec_q[branch_q_id].ROB_ptr[exec_q[branch_q_id].end_ptr] = j;
							ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
							ROB_ptr->rs2_ptr = reg_table->x_reg[ROB_ptr->rs2].current_ptr;
							exec_q[branch_q_id].end_ptr = ((exec_q[branch_q_id].end_ptr + 1) & 3);
							if (debug_unit) {
								fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
								fprintf(debug_stream, "BRANCH 2 ");
								fprintf(debug_stream, "%s x%02d.%d, %s x%02d.%d, start/stop: %d/%d, clock 0x%04llx\n", reg_table->x_reg[ROB_ptr->rs1].name, ROB_ptr->rs1, ROB_ptr->rs1_ptr, reg_table->x_reg[ROB_ptr->rs2].name, ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB->branch_start, ROB->branch_stop, clock);
							}
							ROB_ptr->state = ROB_execute;
						}
						else {
							alloc_count = param->decode_width; // stop allocating
						}
					}
								   break;
					default: {
						switch (ROB_ptr->q_select) {
						case iMUL_q_select: {
							exec_q[iMUL_q_id].ROB_ptr[exec_q[iMUL_q_id].end_ptr] = j;
							exec_q[iMUL_q_id].ROB_ptr_valid[exec_q[iMUL_q_id].end_ptr] = 1;
							ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
							ROB_ptr->rs2_ptr = reg_table->x_reg[ROB_ptr->rs2].current_ptr;
							if (ROB_ptr->rs_count == 3)
								ROB_ptr->rs3_ptr = reg_table->x_reg[ROB_ptr->rs3].current_ptr;
							if (reg_table->x_reg[ROB_ptr->rd].valid[(reg_table->x_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free &&
								!(((reg_table->x_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->x_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {
								ROB_ptr->state = ROB_execute;
								exec_q[iMUL_q_id].end_ptr = ((exec_q[iMUL_q_id].end_ptr + 1) & (exec_q[iMUL_q_id].count - 1));
								allocate_x_rd(ROB_ptr, reg_table, j);
								if (debug_unit) {
									fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
									fprintf(debug_stream, "integer MUL rd = xr%02d.%x, ", ROB_ptr->rd, ROB_ptr->rd_ptr);
									fprintf(debug_stream, "rs1 = x%02d.%x, ", ROB_ptr->rs1, ROB_ptr->rs1_ptr);
									fprintf(debug_stream, "rs2 = x%02d.%x, ", ROB_ptr->rs2, ROB_ptr->rs2_ptr);
									if (ROB_ptr->rs_count == 3)
										fprintf(debug_stream, "rs3 = x%02d.%x, ", ROB_ptr->rs3, ROB_ptr->rs3_ptr);
									fprintf(debug_stream, "clock 0x%04llx\n", clock);
								}
							}
							else {
								dst_reg_block[ROB_ptr->rd] = 1;
							}
						}
										  break;
						case OP_q_select0:
						case OP_q_select1:
						case OP_q_select2:
						case OP_q_select3:
						case OP_q_select4:
						case OP_q_select5:
						case OP_q_select6:
						case OP_q_select7:
						{
							q_select_type OP_q_id = ROB_ptr->q_select;
							exec_q[OP_q_id].ROB_ptr[exec_q[OP_q_id].end_ptr] = j;
							exec_q[OP_q_id].ROB_ptr_valid[exec_q[OP_q_id].end_ptr] = 1;
							ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
							ROB_ptr->rs2_ptr = reg_table->x_reg[ROB_ptr->rs2].current_ptr;
							if (ROB_ptr->rs_count == 3)
								ROB_ptr->rs3_ptr = reg_table->x_reg[ROB_ptr->rs3].current_ptr;
							if ((reg_table->x_reg[ROB_ptr->rd].valid[(reg_table->x_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free ||
								reg_table->x_reg[ROB_ptr->rd].current_ptr == reg_table->x_reg[ROB_ptr->rd].retire_ptr ||
								ROB->retire_ptr_in == j) &&
								((exec_q[OP_q_id].end_ptr + 1) & (exec_q[OP_q_id].count - 1)) != exec_q[OP_q_id].start_ptr &&
								!(((reg_table->x_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->x_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {

								ROB_ptr->state = ROB_execute;
								exec_q[OP_q_id].end_ptr = ((exec_q[OP_q_id].end_ptr + 1) & (exec_q[OP_q_id].count - 1));
								allocate_x_rd(ROB_ptr, reg_table, j);
								if (debug_unit) {
									fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", 
										mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr);
									fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
									fprintf(debug_stream, "INTEGER(%d) xr%02d.%x, ", OP_q_id, ROB_ptr->rd, ROB_ptr->rd_ptr);
									fprintf(debug_stream, "xr%02d.%x, ", ROB_ptr->rs1, ROB_ptr->rs1_ptr);
									if (ROB_ptr->rs_count == 1)
										fprintf(debug_stream, "0x%03x, ", ROB_ptr->imm);
									else {
										fprintf(debug_stream, "xr%02d.%x, ", ROB_ptr->rs2, ROB_ptr->rs2_ptr);
										if (ROB_ptr->rs_count == 3)
											fprintf(debug_stream, "xr%02d.%x, ", ROB_ptr->rs3, ROB_ptr->rs3_ptr);
									}
									fprintf(debug_stream, "clock 0x%04llx\n", clock);
								}
							}
							else {
								dst_reg_block[ROB_ptr->rd] = 1;
							}
						}
						break;
						case LUI_AUIPC_q_select: {
							if (mhartid == 0)
								if (clock == 0x167b)
									debug++;
							if (reg_table->x_reg[ROB_ptr->rd].valid[(reg_table->x_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free &&
								!(((reg_table->x_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->x_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {
								ROB_ptr->state = ROB_execute;

								UINT OP_q_id = OP_q_id0;
								UINT8 q_id = 0;
								switch (ROB_ptr->q_select) {
								case OP_q_select0:
									OP_q_id = OP_q_id0;
									q_id = 0;
									break;
								case OP_q_select1:
									OP_q_id = OP_q_id1;
									q_id = 1;
									break;
								case OP_q_select2:
									OP_q_id = OP_q_id2;
									q_id = 2;
									break;
								case OP_q_select3:
									OP_q_id = OP_q_id3;
									q_id = 3;
									break;
								case OP_q_select4:
									OP_q_id = OP_q_id4;
									q_id = 4;
									break;
								case OP_q_select5:
									OP_q_id = OP_q_id5;
									q_id = 5;
									break;
								case OP_q_select6:
									OP_q_id = OP_q_id6;
									q_id = 6;
									break;
								case OP_q_select7:
									OP_q_id = OP_q_id7;
									q_id = 7;
									break;
								default:
									debug++;
									break;
								}
								exec_q[OP_q_id].ROB_ptr[exec_q[OP_q_id].end_ptr] = j;
								exec_q[OP_q_id].ROB_ptr_valid[exec_q[OP_q_id].end_ptr] = 1;
								ROB_ptr->state = ROB_execute;
								exec_q[OP_q_id].end_ptr = ((exec_q[OP_q_id].end_ptr + 1) & (exec_q[OP_q_id].count - 1));
								//								exec_q[LUI_AUIPC_q_id].end_ptr = ((exec_q[LUI_AUIPC_q_id].end_ptr + 1) & (exec_q[LUI_AUIPC_q_id].count - 1));
								allocate_x_rd(ROB_ptr, reg_table, j);
								if (debug_unit) {
									fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
									if (ROB_ptr->uop == uop_LUI)
										fprintf(debug_stream, "LUI unit %d rd = xr%02d.%x, ", q_id, ROB_ptr->rd, ROB_ptr->rd_ptr);
									else
										fprintf(debug_stream, "AUIPC rd = xr%02d.%x, ", ROB_ptr->rd, ROB_ptr->rd_ptr);
									fprintf(debug_stream, "clock 0x%04llx\n", clock);
								}
							}
							else {
								dst_reg_block[ROB_ptr->rd] = 1;
							}
						}
											   break;
						case load_q_select:
							if (!(((reg_table->x_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->x_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2 && ROB_ptr->uop == uop_LOAD) &&
								!(((reg_table->f_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->f_reg[ROB->q[j].rd].retire_ptr && ROB_ptr->uop == uop_F_LOAD)) {
								if (exec_q[load_q_id].start_ptr == ((exec_q[load_q_id].end_ptr + 1) & (exec_q[load_q_id].count - 1))) {
									dst_reg_block[ROB_ptr->rd] = 1;
								}
								else {
									if (ROB_ptr->reg_type == 3) {// control status register accesses are implied fence operations
										exec_q[load_q_id].ROB_ptr[exec_q[load_q_id].end_ptr] = j;
										exec_q[load_q_id].ROB_ptr_valid[exec_q[load_q_id].end_ptr] = 1;
										exec_q[load_q_id].state[exec_q[load_q_id].end_ptr] = 1;
										if (exec_q[load_q_id].end_ptr == exec_q[load_q_id].start_ptr)
											exec_q[load_q_id].curr_ptr = exec_q[load_q_id].end_ptr;
										ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
										ROB_ptr->state = ROB_execute;
										exec_q[load_q_id].end_ptr = ((exec_q[load_q_id].end_ptr + 1) & (exec_q[load_q_id].count - 1));
									}
									else if (ROB_ptr->uop == uop_F_LOAD) {// floating point registers
										if (allocate_f_rd(ROB_ptr, reg_table)) {
											//											if (debug_unit) {
											fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
											fprintf(debug_stream, "Performance Bug: F_LOAD rd overload stall, fp%02d.%x, 0x%3x (xr%02d.%x) //", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
											fprintf(debug_stream, "rd current/retire %x/%x, ", reg_table->f_reg[ROB_ptr->rd].current_ptr, reg_table->f_reg[ROB_ptr->rd].retire_ptr);
											fprintf(debug_stream, "start-stop ptr 0x%02x-0x%02x, ", exec_q[load_q_id].start_ptr, exec_q[load_q_id].end_ptr);
											fprintf(debug_stream, "clock 0x%04llx\n", clock);
											//											}
										}
										else {
											exec_q[load_q_id].ROB_ptr[exec_q[load_q_id].end_ptr] = j;
											exec_q[load_q_id].ROB_ptr_valid[exec_q[load_q_id].end_ptr] = 1;
											exec_q[load_q_id].state[exec_q[load_q_id].end_ptr] = 1;
											if (exec_q[load_q_id].end_ptr == exec_q[load_q_id].start_ptr)
												exec_q[load_q_id].curr_ptr = exec_q[load_q_id].end_ptr;
											ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
											ROB_ptr->state = ROB_execute;
											if (ROB_ptr->rd == 0x1e && mhartid == 0)
												debug++;
											if (debug_unit) {
												fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
												fprintf(debug_stream, "F_LOAD, fp%02d.%x, 0x%3x (xr%02d.%x) //", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
												fprintf(debug_stream, "rd current/retire %x/%x, ", reg_table->f_reg[ROB_ptr->rd].current_ptr, reg_table->f_reg[ROB_ptr->rd].retire_ptr);
												fprintf(debug_stream, "start-stop ptr 0x%02x-0x%02x, ", exec_q[load_q_id].start_ptr, exec_q[load_q_id].end_ptr);
												fprintf(debug_stream, "clock 0x%04llx\n", clock);
											}
											exec_q[load_q_id].end_ptr = ((exec_q[load_q_id].end_ptr + 1) & (exec_q[load_q_id].count - 1));
										}
									}
									else {
										exec_q[load_q_id].ROB_ptr[exec_q[load_q_id].end_ptr] = j;
										exec_q[load_q_id].ROB_ptr_valid[exec_q[load_q_id].end_ptr] = 1;
										exec_q[load_q_id].state[exec_q[load_q_id].end_ptr] = 1;
										if (exec_q[load_q_id].end_ptr == exec_q[load_q_id].start_ptr)
											exec_q[load_q_id].curr_ptr = exec_q[load_q_id].end_ptr;
										ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
										ROB_ptr->state = ROB_execute;
										allocate_x_rd(ROB_ptr, reg_table, j);
										if (debug_unit) {
											fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
											fprintf(debug_stream, "LOAD, xr%02d.%x, 0x%3x (xr%02d.%x) //", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
											fprintf(debug_stream, "start-stop ptr 0x%02x-0x%02x, ", exec_q[load_q_id].start_ptr, exec_q[load_q_id].end_ptr);
											fprintf(debug_stream, "clock 0x%04llx\n", clock);
										}
										exec_q[load_q_id].end_ptr = ((exec_q[load_q_id].end_ptr + 1) & (exec_q[load_q_id].count - 1));
									}
								}
							}
							else {
								dst_reg_block[ROB_ptr->rd] = 1;
							}
							break;
						case store_q_select:
							if (((exec_q[store_q_id].end_ptr + 1) & (exec_q[store_q_id].count - 1)) != exec_q[store_q_id].start_ptr &&
								!(((reg_table->x_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->x_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {
								if (ROB_ptr->uop == uop_STORE) {
									ROB_ptr->rd = 0;
									ROB_ptr->rd_ptr = 0;
								}
								ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
								if (ROB_ptr->map != AMO_map)
									ROB_ptr->state = ROB_execute;
								if (ROB_ptr->uop == uop_LR)
									dst_reg_block[ROB_ptr->rd] = 1;
								if (ROB_ptr->state == ROB_execute) {
									exec_q[store_q_id].ROB_ptr[exec_q[store_q_id].end_ptr] = j;
									exec_q[store_q_id].ROB_ptr_valid[exec_q[store_q_id].end_ptr] = 1;
									exec_q[store_q_id].end_ptr = ((exec_q[store_q_id].end_ptr + 1) & (exec_q[store_q_id].count - 1));
									if (debug_unit) {
										fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr);
										fprint_addr_coma(debug_stream, ROB_ptr->addr, param);
									}
									if (ROB_ptr->uop == uop_F_STORE) {
										ROB_ptr->rs2_ptr = reg_table->f_reg[ROB_ptr->rs2].current_ptr;
										if (debug_unit) {
											fprintf(debug_stream, "F_STORE, fp%02d.%x, 0x%3x (xr%02d.%x) //", ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
										}
									}
									else {
										ROB_ptr->rs2_ptr = reg_table->x_reg[ROB_ptr->rs2].current_ptr;
										if (debug_unit) {
//											fprintf(debug_stream, "STORE, xr%02d.%x, 0x%03x(xr%02d.%x) //", ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
											switch (ROB_ptr->funct3) {
											case 0:
												fprintf(debug_stream, "SB");
												break;
											case 1:
												fprintf(debug_stream, "SH");
												break;
											case 2:
												fprintf(debug_stream, "SW");
												break;
											case 3:
												fprintf(debug_stream, "SD");
												break;
											case 4:
												fprintf(debug_stream, "SQ");
												break;
											default:
												fprintf(debug_stream, "STORE");
												break;
											}
											fprintf(debug_stream, " xr%02d.%x, 0x%03x(xr%02d.%x) //", ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB_ptr->imm, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
										}
									}
									if (debug_unit) {
						//				fprintf(debug_stream, "branch_num: %d %d-%d, exec_q (start/stop):%#x/%#x clock 0x%04llx\n", ROB_ptr->branch_num, ROB->branch_start, ROB->branch_stop, exec_q[store_q_id].start_ptr, exec_q[store_q_id].end_ptr, clock);
										fprintf(debug_stream, " exec_q (start/stop):%#x/%#x clock 0x%04llx\n", exec_q[store_q_id].start_ptr, exec_q[store_q_id].end_ptr, clock);
									}
								}
							}
							else {
								alloc_count = param->decode_width; // stop allocating
							}
							break;
						case op_fp_select: {
							if (ROB_ptr->rd == 0x1e && mhartid == 0)
								debug++;
							switch (ROB_ptr->uop) {
							case uop_FCVTf2i: {
								fadd_q_id[0]++;
								fadd_q_id[0] &= (param->decode_width - 1);
								fadd_q_id[0] |= fadd_q_id0;
								exec_q[fadd_q_id[0]].ROB_ptr[exec_q[fadd_q_id[0]].end_ptr] = j;
								exec_q[fadd_q_id[0]].ROB_ptr_valid[exec_q[fadd_q_id[0]].end_ptr] = 1;
								ROB_ptr->rs1_ptr = reg_table->f_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs_count = 1;
								if (reg_table->x_reg[ROB_ptr->rd].valid[(reg_table->x_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free &&
									((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1)) != exec_q[fadd_q_id[0]].start_ptr &&
									!(((reg_table->x_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->x_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {

									ROB_ptr->state = ROB_execute;
									exec_q[fadd_q_id[0]].end_ptr = ((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1));
									allocate_x_rd(ROB_ptr, reg_table, j);
									if (debug_unit) {
										fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
										fprintf(debug_stream, "FCVTf2i uint %d ", fadd_q_id[0] & (param->decode_width - 1));
										fprintf(debug_stream, "fp%02d.%x, fp%02d.%x, fp%02d.%x // ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr);
										fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fadd_q_id[0]].end_ptr, clock);
									}
								}
								else {
									debug++;// mixing int and fp
								}
							}
											break;
							case uop_FCVTi2f: {
								fadd_q_id[0]++;
								fadd_q_id[0] &= (param->decode_width - 1);
								fadd_q_id[0] |= fadd_q_id0;
								exec_q[fadd_q_id[0]].ROB_ptr[exec_q[fadd_q_id[0]].end_ptr] = j;
								exec_q[fadd_q_id[0]].ROB_ptr_valid[exec_q[fadd_q_id[0]].end_ptr] = 1;
								ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs_count = 1;
								if (reg_table->f_reg[ROB_ptr->rd].valid[(reg_table->f_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free &&
									((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1)) != exec_q[fadd_q_id[0]].start_ptr &&
									!(((reg_table->f_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->f_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {

									ROB_ptr->state = ROB_execute;
									exec_q[fadd_q_id[0]].end_ptr = ((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1));
									allocate_f_rd(ROB_ptr, reg_table);
									if (debug_unit) {
										fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
										//	fprintf(debug_stream, "FCVTi2f ");
										fprintf(debug_stream, "FCVTi2f uint %d ", fadd_q_id[0] & (param->decode_width - 1));
										fprintf(debug_stream, "fp%02d.%x, x%02d.%d // ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr);
										fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fadd_q_id[0]].end_ptr, clock);
									}
								}
								else {
									debug++;// mixing int and fp
								}
							}
											break;
							case uop_FMVf2i: {
								fadd_q_id[0]++;
								fadd_q_id[0] &= (param->decode_width - 1);
								fadd_q_id[0] |= fadd_q_id0;
								exec_q[fadd_q_id[0]].ROB_ptr[exec_q[fadd_q_id[0]].end_ptr] = j;
								exec_q[fadd_q_id[0]].ROB_ptr_valid[exec_q[fadd_q_id[0]].end_ptr] = 1;
								ROB_ptr->rs1_ptr = reg_table->f_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs_count = 1;
								if (reg_table->x_reg[ROB_ptr->rd].valid[(reg_table->x_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free &&
									((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1)) != exec_q[fadd_q_id[0]].start_ptr &&
									!(((reg_table->x_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->x_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {

									ROB_ptr->state = ROB_execute;
									exec_q[fadd_q_id[0]].end_ptr = ((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1));
									allocate_x_rd(ROB_ptr, reg_table, j);
									if (debug_unit) {
										fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
										//			fprintf(debug_stream, "FMVf2i ");
										fprintf(debug_stream, "FMVf2i uint %d ", fadd_q_id[0] & (param->decode_width - 1));
										fprintf(debug_stream, "fp%02d.%x, x%02d.%d, fp%02d.%x // ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr);
										fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fadd_q_id[0]].end_ptr, clock);
									}
								}
								else {
									debug++;// mixing int and fp
								}
							}
										   break;
							case uop_FMVi2f: {
								fadd_q_id[0]++;
								fadd_q_id[0] &= (param->decode_width - 1);
								fadd_q_id[0] |= fadd_q_id0;
								exec_q[fadd_q_id[0]].ROB_ptr[exec_q[fadd_q_id[0]].end_ptr] = j;
								exec_q[fadd_q_id[0]].ROB_ptr_valid[exec_q[fadd_q_id[0]].end_ptr] = 1;
								ROB_ptr->rs1_ptr = reg_table->x_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs_count = 1;
								if ((reg_table->f_reg[ROB_ptr->rd].valid[(reg_table->f_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free ||
									reg_table->f_reg[ROB_ptr->rd].current_ptr == reg_table->f_reg[ROB_ptr->rd].retire_ptr) &&
									((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1)) != exec_q[fadd_q_id[0]].start_ptr &&
									!(((reg_table->f_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->f_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {

									ROB_ptr->state = ROB_execute;
									exec_q[fadd_q_id[0]].end_ptr = ((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1));
									allocate_f_rd(ROB_ptr, reg_table);
									if (debug_unit) {
										fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
										fprintf(debug_stream, "FMVi2f ");
										fprintf(debug_stream, "fp%02d.%x, fp%02d.%x, fp%02d.%x // ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr);
										fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fadd_q_id[0]].end_ptr, clock);
									}
								}
								else {
									debug++;// mixing int and fp
								}
							}
										   break;
							case uop_FMIN: {
								fadd_q_id[0]++;
								fadd_q_id[0] &= (param->decode_width - 1);
								fadd_q_id[0] |= fadd_q_id0;
								exec_q[fadd_q_id[0]].ROB_ptr[exec_q[fadd_q_id[0]].end_ptr] = j;
								exec_q[fadd_q_id[0]].ROB_ptr_valid[exec_q[fadd_q_id[0]].end_ptr] = 1;
								ROB_ptr->rs1_ptr = reg_table->f_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs2_ptr = reg_table->f_reg[ROB_ptr->rs2].current_ptr;
								if (reg_table->f_reg[ROB_ptr->rd].valid[(reg_table->f_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free &&
									((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1)) != exec_q[fadd_q_id[0]].start_ptr &&
									!(((reg_table->f_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->f_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {

									ROB_ptr->state = ROB_execute;
									exec_q[fadd_q_id[0]].end_ptr = ((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1));
									allocate_f_rd(ROB_ptr, reg_table);
									if (debug_unit) {
										fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
										fprintf(debug_stream, "FMIN ");
										fprintf(debug_stream, "fp%02d.%x, fp%02d.%x, fp%02d.%x // ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr);
										fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fadd_q_id[0]].end_ptr, clock);
									}
								}
								else {
									debug++;// mixing int and fp
									//								dst_reg_block[ROB_ptr->rd] = 1;
								}
							}
										 break;
							case uop_FMAX: {
								fadd_q_id[0]++;
								fadd_q_id[0] &= (param->decode_width - 1);
								fadd_q_id[0] |= fadd_q_id0;
								exec_q[fadd_q_id[0]].ROB_ptr[exec_q[fadd_q_id[0]].end_ptr] = j;
								exec_q[fadd_q_id[0]].ROB_ptr_valid[exec_q[fadd_q_id[0]].end_ptr] = 1;
								ROB_ptr->rs1_ptr = reg_table->f_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs2_ptr = reg_table->f_reg[ROB_ptr->rs2].current_ptr;
								if (reg_table->f_reg[ROB_ptr->rd].valid[(reg_table->f_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free &&
									((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1)) != exec_q[fadd_q_id[0]].start_ptr &&
									!(((reg_table->f_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->f_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {

									ROB_ptr->state = ROB_execute;
									exec_q[fadd_q_id[0]].end_ptr = ((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1));
									allocate_f_rd(ROB_ptr, reg_table);
									if (debug_unit) {
										fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
										//		fprintf(debug_stream, "FMAX ");
										fprintf(debug_stream, "FMAX uint %d ", fadd_q_id[0] & (param->decode_width - 1));
										fprintf(debug_stream, "fp%02d.%x, fp%02d.%x, fp%02d.%x // ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr);
										fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fadd_q_id[0]].end_ptr, clock);
									}
								}
								else {
									debug++;// mixing int and fp
									//								dst_reg_block[ROB_ptr->rd] = 1;
								}
							}
										 break;

							case uop_FMADD: {
								fmac_q_id[0]++;
								fmac_q_id[0] &= (param->fmac - 1);
								fmac_q_id[0] |= fmul_q_id0;
								exec_q[fmac_q_id[0]].ROB_ptr[exec_q[fmac_q_id[0]].end_ptr] = j;
								exec_q[fmac_q_id[0]].ROB_ptr_valid[exec_q[fmac_q_id[0]].end_ptr] = 1;
								ROB_ptr->rs1_ptr = reg_table->f_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs2_ptr = reg_table->f_reg[ROB_ptr->rs2].current_ptr;
								ROB_ptr->rs3_ptr = reg_table->f_reg[ROB_ptr->rs3].current_ptr;
								ROB_ptr->rs_count = 3;
								if (reg_table->f_reg[ROB_ptr->rd].valid[(reg_table->f_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free &&
									((exec_q[fmac_q_id[0]].end_ptr + 1) & (exec_q[fmac_q_id[0]].count - 1)) != exec_q[fmac_q_id[0]].start_ptr &&
									!(((reg_table->f_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->f_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {

									ROB_ptr->state = ROB_execute;
									exec_q[fmac_q_id[0]].end_ptr = ((exec_q[fmac_q_id[0]].end_ptr + 1) & (exec_q[fmac_q_id[0]].count - 1));
									allocate_f_rd(ROB_ptr, reg_table);
									if (debug_unit) {
										fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
										fprintf(debug_stream, "FMADD ");
										fprintf(debug_stream, "fp%02d.%x, fp%02d.%x, fp%02d.%x, fp%02d.%x // ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB_ptr->rs3, ROB_ptr->rs3_ptr);
										fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fmac_q_id[0]].end_ptr, clock);
									}
								}
								else {
									debug++;// mixing int and fp
									//								dst_reg_block[ROB_ptr->rd] = 1;
								}
							}
										  break;
							case uop_FMSUB: {
								fmac_q_id[0]++;
								fmac_q_id[0] &= (param->fmac - 1);
								fmac_q_id[0] |= fmul_q_id0;
								exec_q[fmac_q_id[0]].ROB_ptr[exec_q[fmac_q_id[0]].end_ptr] = j;
								exec_q[fmac_q_id[0]].ROB_ptr_valid[exec_q[fmac_q_id[0]].end_ptr] = 1;
								ROB_ptr->rs1_ptr = reg_table->f_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs2_ptr = reg_table->f_reg[ROB_ptr->rs2].current_ptr;
								ROB_ptr->rs3_ptr = reg_table->f_reg[ROB_ptr->rs3].current_ptr;
								ROB_ptr->rs_count = 3;
								if (reg_table->f_reg[ROB_ptr->rd].valid[(reg_table->f_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free &&
									((exec_q[fmac_q_id[0]].end_ptr + 1) & (exec_q[fmac_q_id[0]].count - 1)) != exec_q[fmac_q_id[0]].start_ptr &&
									!(((reg_table->f_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->f_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {

									ROB_ptr->state = ROB_execute;
									exec_q[fmac_q_id[0]].end_ptr = ((exec_q[fmac_q_id[0]].end_ptr + 1) & (exec_q[fmac_q_id[0]].count - 1));
									allocate_f_rd(ROB_ptr, reg_table);
									if (debug_unit) {
										fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
										fprintf(debug_stream, "FMSUB ");
										fprintf(debug_stream, "fp%02d.%x, fp%02d.%x, fp%02d.%x, fp%02d.%x // ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB_ptr->rs3, ROB_ptr->rs3_ptr);
										fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fmac_q_id[0]].end_ptr, clock);
									}
								}
								else {
									debug++;// mixing int and fp
									//								dst_reg_block[ROB_ptr->rd] = 1;
								}
							}
										  break;
							case uop_FNMADD: {
								fmac_q_id[0]++;
								fmac_q_id[0] &= (param->fmac - 1);
								fmac_q_id[0] |= fmul_q_id0;
								exec_q[fmac_q_id[0]].ROB_ptr[exec_q[fmac_q_id[0]].end_ptr] = j;
								exec_q[fmac_q_id[0]].ROB_ptr_valid[exec_q[fmac_q_id[0]].end_ptr] = 1;
								ROB_ptr->rs1_ptr = reg_table->f_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs2_ptr = reg_table->f_reg[ROB_ptr->rs2].current_ptr;
								ROB_ptr->rs3_ptr = reg_table->f_reg[ROB_ptr->rs3].current_ptr;
								ROB_ptr->rs_count = 3;
								if (reg_table->f_reg[ROB_ptr->rd].valid[(reg_table->f_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free &&
									((exec_q[fmac_q_id[0]].end_ptr + 1) & (exec_q[fmac_q_id[0]].count - 1)) != exec_q[fmac_q_id[0]].start_ptr &&
									!(((reg_table->f_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->f_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {

									ROB_ptr->state = ROB_execute;
									exec_q[fmac_q_id[0]].end_ptr = ((exec_q[fmac_q_id[0]].end_ptr + 1) & (exec_q[fmac_q_id[0]].count - 1));
									allocate_f_rd(ROB_ptr, reg_table);
									if (debug_unit) {
										fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
										fprintf(debug_stream, "FNMADD ");
										fprintf(debug_stream, "fp%02d.%x, fp%02d.%x, fp%02d.%x, fp%02d.%x // ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB_ptr->rs3, ROB_ptr->rs3_ptr);
										fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fmac_q_id[0]].end_ptr, clock);
									}
								}
								else {
									debug++;// mixing int and fp
									//								dst_reg_block[ROB_ptr->rd] = 1;
								}
							}
										   break;
							case uop_FNMSUB: {
								fmac_q_id[0]++;
								fmac_q_id[0] &= (param->fmac - 1);
								fmac_q_id[0] |= fmul_q_id0;
								exec_q[fmac_q_id[0]].ROB_ptr[exec_q[fmac_q_id[0]].end_ptr] = j;
								exec_q[fmac_q_id[0]].ROB_ptr_valid[exec_q[fmac_q_id[0]].end_ptr] = 1;
								ROB_ptr->rs1_ptr = reg_table->f_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs2_ptr = reg_table->f_reg[ROB_ptr->rs2].current_ptr;
								ROB_ptr->rs3_ptr = reg_table->f_reg[ROB_ptr->rs3].current_ptr;
								ROB_ptr->rs_count = 3;
								if (reg_table->f_reg[ROB_ptr->rd].valid[(reg_table->f_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free &&
									((exec_q[fmac_q_id[0]].end_ptr + 1) & (exec_q[fmac_q_id[0]].count - 1)) != exec_q[fmac_q_id[0]].start_ptr &&
									!(((reg_table->f_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->f_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {

									ROB_ptr->state = ROB_execute;
									exec_q[fmac_q_id[0]].end_ptr = ((exec_q[fmac_q_id[0]].end_ptr + 1) & (exec_q[fmac_q_id[0]].count - 1));
									allocate_f_rd(ROB_ptr, reg_table);
									if (debug_unit) {
										fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, q_id %x, 0x%016I64x ",
											mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, fmac_q_id[0], ROB_ptr->addr);
										fprintf(debug_stream, "FNMSUB fp%02d.%x, fp%02d.%x, fp%02d.%x, fp%02d.%x // ",
											ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr, ROB_ptr->rs3, ROB_ptr->rs3_ptr);
										fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fmac_q_id[0]].end_ptr, clock);
									}
								}
								else {
									debug++;// mixing int and fp
								}
							}
										   break;
							case uop_FMUL: {
								fmac_q_id[0]++;
								fmac_q_id[0] &= (param->fmac - 1);
								fmac_q_id[0] |= fmul_q_id0;
								exec_q[fmac_q_id[0]].ROB_ptr[exec_q[fmac_q_id[0]].end_ptr] = j;
								exec_q[fmac_q_id[0]].ROB_ptr_valid[exec_q[fmac_q_id[0]].end_ptr] = 1;
								ROB_ptr->rs1_ptr = reg_table->f_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs2_ptr = reg_table->f_reg[ROB_ptr->rs2].current_ptr;
								if (reg_table->f_reg[ROB_ptr->rd].valid[(reg_table->f_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free &&
									((exec_q[fmac_q_id[0]].end_ptr + 1) & (exec_q[fmac_q_id[0]].count - 1)) != exec_q[fmac_q_id[0]].start_ptr &&
									!(((reg_table->f_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->f_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {

									ROB_ptr->state = ROB_execute;
									exec_q[fmac_q_id[0]].end_ptr = ((exec_q[fmac_q_id[0]].end_ptr + 1) & (exec_q[fmac_q_id[0]].count - 1));
									allocate_f_rd(ROB_ptr, reg_table);
									if (debug_unit) {
										fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
										fprintf(debug_stream, "FMUL ");
										fprintf(debug_stream, "fp%02d.%x, fp%02d.%x, fp%02d.%x // ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr);
										fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fmac_q_id[0]].end_ptr, clock);
									}
								}
								else {
									debug++;// mixing int and fp
								}
							}
										 break;
							default: {
								fadd_q_id[0]++;
								fadd_q_id[0] &= (param->decode_width - 1);
								fadd_q_id[0] |= fadd_q_id0;
								exec_q[fadd_q_id[0]].ROB_ptr[exec_q[fadd_q_id[0]].end_ptr] = j;
								exec_q[fadd_q_id[0]].ROB_ptr_valid[exec_q[fadd_q_id[0]].end_ptr] = 1;
								ROB_ptr->rs1_ptr = reg_table->f_reg[ROB_ptr->rs1].current_ptr;
								ROB_ptr->rs2_ptr = reg_table->f_reg[ROB_ptr->rs2].current_ptr;
								if (reg_table->f_reg[ROB_ptr->rd].valid[(reg_table->f_reg[ROB_ptr->rd].current_ptr + 1) & (reg_rename_size - 1)] == reg_free &&
									((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1)) != exec_q[fadd_q_id[0]].start_ptr &&
									!(((reg_table->f_reg[ROB->q[j].rd].current_ptr + 1) & (reg_rename_size - 1)) == reg_table->f_reg[ROB->q[j].rd].retire_ptr && ROB->q[ROB->allocate_ptr].state == ROB_allocate_2)) {

									ROB_ptr->state = ROB_execute;
									exec_q[fadd_q_id[0]].end_ptr = ((exec_q[fadd_q_id[0]].end_ptr + 1) & (exec_q[fadd_q_id[0]].count - 1));
									allocate_f_rd(ROB_ptr, reg_table);
									if (debug_unit) {
										fprintf(debug_stream, "ALLOCATE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", mhartid, j, ROB->retire_ptr_in, ROB->decode_ptr, ROB_ptr->addr);
										fprintf(debug_stream, "FP operation ");
										fprintf(debug_stream, "fp%02d.%x, fp%02d.%x, fp%02d.%x // ", ROB_ptr->rd, ROB_ptr->rd_ptr, ROB_ptr->rs1, ROB_ptr->rs1_ptr, ROB_ptr->rs2, ROB_ptr->rs2_ptr);
										fprintf(debug_stream, "end_ptr %#x, clock 0x%04llx\n", exec_q[fadd_q_id[0]].end_ptr, clock);
									}
								}
								else {
									debug++;// mixing int and fp
								}
							}
								   break;
							}
							break;
						}
						default:
							debug++;
							break;
						}
					}
						   break;
					}
				}
				if (ROB->q[ROB->allocate_ptr].state == ROB_execute || ROB->q[ROB->allocate_ptr].state == ROB_retire_out)
					ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
			}
		}
	}
}
