// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "ROB.h"
void missed_branch(branch_vars* branch, branch_type* branch_exec, UINT64 clock, UINT mhartid, UINT debug_unit, FILE* debug_stream) {
	int debug = 0;
	branch->total[branch_exec->addr & 0x0fff] = branch->count[branch_exec->addr & 0x0fff];
	branch->count[branch_exec->addr & 0x0fff] = 1;

	if (clock >= 0x1b22)
		debug++;
	branch->ROB_entry = branch_exec->ROB_id;
	if (debug_unit) {
		fprintf(debug_stream, "BRANCH unit(%lld) ROB entry: 0x%02x, 0x%016I64x ; Branch Miss - request queues cleared; next addr: 0x%016I64x ;clock: 0x%04llx\n",
			mhartid, branch_exec->ROB_id, branch_exec->addr, branch_exec->addr, clock);
	}
}
void hit_code(branch_type* branch_exec, UINT8 taken, UINT64 clock, UINT mhartid, UINT debug_unit, param_type* param, FILE* debug_stream) {
	if (debug_unit) {
		fprintf(debug_stream, "BRANCH unit(%lld) ROB entry: 0x%02x, ", mhartid, branch_exec->ROB_id);
		fprint_addr_coma(debug_stream, branch_exec->addr, param);
		switch (branch_exec->uop) {
		case uop_BEQ:
			fprintf(debug_stream, "BEQ");
			break;
		case uop_BNE:
			fprintf(debug_stream, "BNE");
			break;
		case uop_BLT:
			fprintf(debug_stream, "BLT");
			break;
		case uop_BGE:
			fprintf(debug_stream, "BGE");
			break;
		case uop_BLTU:
			fprintf(debug_stream, "BLTU");
			break;
		case uop_BGEU:
			fprintf(debug_stream, "BGEU");
			break;
		default:
			break;
		}
		if (taken) {
			fprintf(debug_stream, " Branch Taken; branch addr: 0x%016I64x, rs1: 0x%016I64x, rs2: 0x%016I64x, clock 0x%04x\n",
				branch_exec->addr + branch_exec->offset, branch_exec->rs1, branch_exec->rs2, clock);
		}
		else {
			fprintf(debug_stream, " Branch Not Taken; branch addr: 0x%016I64x, rs1: 0x%016I64x, rs2: 0x%016I64x, clock%#x\n",
				branch_exec->addr + branch_exec->offset, branch_exec->rs1, branch_exec->rs2, clock);
		}
	}
}
void branch_taken(UINT16* exec_rsp, reg_bus* rd, UINT8* block_loads, branch_vars* branch, branch_type* branch_exec, UINT load_pending, UINT store_pending, UINT16 retire_index, UINT64 clock, UINT mhartid, UINT debug_unit, param_type* param, FILE* debug_stream) {
	int debug = 0;
//	INT64 addr = branch_exec->addr + (INT64)branch_exec->offset;
	rd->data = branch_exec->addr + branch_exec->offset;
	if (branch_exec->strobe == 3) {
		branch->count[branch_exec->addr & 0x0fff]++;
		hit_code(branch_exec, 1, clock, mhartid, debug_unit,param, debug_stream);
		exec_rsp[0] = 0x8000 | branch_exec->ROB_id;
		rd->strobe = 1;
		rd->ROB_id = branch_exec->ROB_id;
	}
	else if (retire_index == branch_exec->ROB_id) {
		if (load_pending) {
			block_loads[0] = 1;
			if (debug_unit) {
				fprintf(debug_stream, "BRANCH(%lld) ROB entry: 0x%02x, 0x%016I64x; Branch Taken Miss stall load pending; next addr 0x%016I64x ;clock 0x%04llx\n",
					mhartid, branch_exec->ROB_id, branch_exec->addr, rd->data, clock);
			}
		}
		else if (store_pending) {
			block_loads[0] = 1;
			if (debug_unit) {
				fprintf(debug_stream, "BRANCH(%lld) ROB entry: 0x%02x, 0x%016I64x; Branch Taken Miss stall store pending; next addr 0x%016I64x ;clock 0x%04llx\n",
					mhartid, branch_exec->ROB_id, branch_exec->addr, rd->data, clock);
			}
		}
		else {
			if (branch->count[branch_exec->addr & 0x0fff] == 0 &&
				branch->total[branch_exec->addr & 0x0fff] != branch->count[branch_exec->addr & 0x0fff])
				branch->pred[branch_exec->addr & 0x0fff] = 1;
			rd->strobe = 2;
			rd->ROB_id = branch_exec->ROB_id;
			missed_branch(branch, branch_exec, clock, mhartid, debug_unit, debug_stream);
			hit_code(branch_exec, 0, clock, mhartid, debug_unit, param, debug_stream);
		}
	}
	else {
		if (debug_unit) {
			fprintf(debug_stream, "BRANCH(%lld) ROB entry: 0x%02x, 0x%016I64x; Branch Taken Miss stall, wait until top of retire queue; next addr 0x%016I64x ;clock 0x%04llx\n",
				mhartid, branch_exec->ROB_id, branch_exec->addr, rd->data, clock);
		}
		debug++;
	}
}
void branch_not_taken(UINT16* exec_rsp, reg_bus* rd, UINT8* block_loads, branch_vars* branch, branch_type* branch_exec, UINT load_pending, UINT store_pending, UINT16 retire_index, UINT64 clock, UINT mhartid, UINT debug_unit, param_type* param, FILE* debug_stream) {
	int debug = 0;
	rd->data = branch_exec->addr + 4;
	//	if (branch->pred[branch_exec->addr & 0x0fff] == 0) {
	if (branch_exec->strobe == 1) {
		branch->count[branch_exec->addr & 0x0fff]++;
		hit_code(branch_exec, 0, clock, mhartid, debug_unit, param, debug_stream);
		exec_rsp[0] = 0x8000 | branch_exec->ROB_id;
		rd->strobe = 1;
		rd->ROB_id = branch_exec->ROB_id;
	}
	else if (retire_index == branch_exec->ROB_id) {
		if (load_pending == 0 && (store_pending == 0|| store_pending == 1)) {
			if (branch->count[branch_exec->addr & 0x0fff] == 0 &&
				branch->total[branch_exec->addr & 0x0fff] != branch->count[branch_exec->addr & 0x0fff])
				branch->pred[branch_exec->addr & 0x0fff] = 0;
			rd->strobe = 2;
			rd->ROB_id = branch_exec->ROB_id;
			missed_branch(branch, branch_exec, clock, mhartid, debug_unit, debug_stream);
		}
		else {
			block_loads[0] = 1;
			if (debug_unit) {
				fprintf(debug_stream, "BRANCH(%lld) ROB entry: 0x%02x, 0x%016I64x ; Branch Not Taken Miss stall loads; next addr 0x%016I64x; clock 0x%04llx\n",
					mhartid, branch_exec->ROB_id, branch_exec->addr, rd->data, clock);
			}
		}
	}
	else {
		debug++;
	}
}
void branch_unit(reg_bus* rd, branch_vars* branch, UINT16* exec_rsp, UINT8* block_loads, branch_type* branch_exec, UINT load_pending, UINT16 retire_index, retire_type *retire, UINT8 stores_pending, UINT64 sbound, UINT64 mbound,
	UINT64 clock, UINT mhartid, UINT debug_core, param_type *param, FILE* debug_stream) {
	int debug = 0;

	UINT debug_unit = param->branch && debug_core;

	if (mhartid == 0)
		if (clock >= 0x3753)
			debug++;
	exec_rsp[0] = 0;
	rd->strobe = 0;
	block_loads[0] = 0;
	if (branch_exec->strobe) {
		switch (branch_exec->uop) {
		case uop_BEQ:
			if (branch_exec->rs1 == branch_exec->rs2) {
				branch_taken(exec_rsp, rd, block_loads, branch, branch_exec, load_pending, stores_pending, retire_index, clock, mhartid, debug_unit, param, debug_stream);
			}
			else {
				branch_not_taken(exec_rsp, rd, block_loads, branch, branch_exec, load_pending, stores_pending, retire_index, clock, mhartid, debug_unit, param, debug_stream);
			}
			break;
		case uop_BNE:
			if (branch_exec->rs1 != branch_exec->rs2) {
				branch_taken(exec_rsp, rd, block_loads, branch, branch_exec, load_pending, stores_pending, retire_index, clock, mhartid, debug_unit, param, debug_stream);
			}
			else {
				branch_not_taken(exec_rsp, rd, block_loads, branch, branch_exec, load_pending, stores_pending, retire_index, clock, mhartid, debug_unit, param, debug_stream);
			}
			break;
		case uop_BLT:
			if (branch_exec->rs1 < branch_exec->rs2) {
				branch_taken(exec_rsp, rd, block_loads, branch, branch_exec, load_pending, stores_pending, retire_index, clock, mhartid, debug_unit, param, debug_stream);
			}
			else {
				branch_not_taken(exec_rsp, rd, block_loads, branch, branch_exec, load_pending, stores_pending, retire_index, clock, mhartid, debug_unit, param, debug_stream);
			}
			break;
		case uop_BGE:
			if (branch_exec->rs1 >= branch_exec->rs2) {
				branch_taken(exec_rsp, rd, block_loads, branch, branch_exec, load_pending, stores_pending, retire_index, clock, mhartid, debug_unit, param, debug_stream);
			}
			else {
				branch_not_taken(exec_rsp, rd, block_loads, branch, branch_exec, load_pending, stores_pending, retire_index, clock, mhartid, debug_unit, param, debug_stream);
			}
			break;
		case uop_BLTU:
			if (branch_exec->rs1 < branch_exec->rs2) {
				branch_taken(exec_rsp, rd, block_loads, branch, branch_exec, load_pending, stores_pending, retire_index, clock, mhartid, debug_unit, param, debug_stream);
			}
			else {
				branch_not_taken(exec_rsp, rd, block_loads, branch, branch_exec, load_pending, stores_pending, retire_index, clock, mhartid, debug_unit, param, debug_stream);
			}
			break;
		case uop_BGEU:
			if (branch_exec->rs1 >= branch_exec->rs2) {
				branch_taken(exec_rsp, rd, block_loads, branch, branch_exec, load_pending, stores_pending, retire_index, clock, mhartid, debug_unit, param, debug_stream);
			}
			else {
				branch_not_taken(exec_rsp, rd, block_loads, branch, branch_exec, load_pending, stores_pending, retire_index, clock, mhartid, debug_unit, param, debug_stream);
			}
			break;
		case uop_JALR:
			if (branch_exec->ROB_id == retire_index) {
				if (rd->data >= sbound) {//0x86400000
					if (retire->priviledge != 0 && stores_pending)
						debug++;
					else {
						exec_rsp[0] = 0x8000 | branch_exec->ROB_id;
						rd->strobe = 3;
						rd->ROB_id = branch_exec->ROB_id;
						rd->data = branch_exec->rs1 + branch_exec->rs2;
						if (debug_unit) {
							fprintf(debug_stream, "BRANCH(%lld): ROB entry: 0x%02x, ", mhartid, branch_exec->ROB_id);
							fprint_addr_coma(debug_stream, branch_exec->addr, param);
							fprintf(debug_stream, "JALR branch addr ");
							fprint_addr_coma(debug_stream, rd->data, param);
							fprintf(debug_stream, " imm 0x%016I64x, clock 0x%04x\n", branch_exec->rs2, clock);
						}
					}
					retire->next_priviledge = 0;
				}
				else if (rd->data >= mbound) {//0x80000000
					if (retire->priviledge != 1 && stores_pending)
						debug++;
					else {
						exec_rsp[0] = 0x8000 | branch_exec->ROB_id;
						rd->strobe = 3;
						rd->ROB_id = branch_exec->ROB_id;
						rd->data = branch_exec->rs1 + branch_exec->rs2;
						if (debug_unit) {
							fprintf(debug_stream, "BRANCH(%lld): ROB entry: 0x%02x, ", mhartid, branch_exec->ROB_id);
							fprint_addr_coma(debug_stream, branch_exec->addr, param);
							fprintf(debug_stream, "JALR branch addr ");
							fprint_addr_coma(debug_stream, rd->data, param);
							fprintf(debug_stream, " imm 0x%016I64x, clock 0x%04x\n", branch_exec->rs2, clock);
						}
					}
					retire->next_priviledge = 1;
				}
				else {
					if (retire->priviledge != 3 && stores_pending)
						debug++;
					else {
						exec_rsp[0] = 0x8000 | branch_exec->ROB_id;
						rd->strobe = 3;
						rd->ROB_id = branch_exec->ROB_id;
						rd->data = branch_exec->rs1 + branch_exec->rs2;
						if (debug_unit) {
							fprintf(debug_stream, "BRANCH(%lld): ROB entry: 0x%02x, ", mhartid, branch_exec->ROB_id);
							fprint_addr_coma(debug_stream, branch_exec->addr, param);
							fprintf(debug_stream, "JALR branch addr ");
							fprint_addr_coma(debug_stream, rd->data, param);
							fprintf(debug_stream, " imm 0x%016I64x, clock 0x%04x\n", branch_exec->rs2, clock);
						}
					}
					retire->next_priviledge = 3;
				}

			}
			break;
		default:
			debug++;
			break;
		}
	}
}