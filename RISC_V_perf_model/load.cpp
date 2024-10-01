// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "ROB.h"
void latch_rd(reg_bus* rd, data_bus_type* physical_data, load_buffer* Load_buffer, UINT64 mhartid, UINT64 clock, UINT debug_bus, FILE* debug_stream) {
	int debug = 0;
	UINT8 chunk_addr = (Load_buffer->addr >> 1) & 0x07;// 128 chunks
	rd->data = 0;
	rd->data_H = 0;
	if (Load_buffer->fp) {
		switch (Load_buffer->size) {
		case 1: {// flh
			if (Load_buffer->addr & 1 != 0)
				debug++;
			switch (chunk_addr) {
			case 0:
				rd->data = physical_data->data[(Load_buffer->addr >> 3) & 0x0f] & 0x0000ffff;
				break;
			case 1:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 16) & 0x0000ffff;
				break;
			case 2:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x0000ffff;
				break;
			case 3:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 48) & 0x0000ffff;
				break;
			case 4:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 0) & 0x0000ffff;
				break;
			case 5:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 16) & 0x0000ffff;
				break;
			case 6:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x0000ffff;
				break;
			case 7:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 48) & 0x0000ffff;
				break;
			default:
				break;
			}
			if (debug_bus) {
				fprintf(debug_stream, "FLH");
			}
		}
			  break;
		case 2: {//flw
			if (Load_buffer->addr & 3 != 0)
				debug++;
			switch (chunk_addr) {
			case 0:
				rd->data = physical_data->data[(Load_buffer->addr >> 3) & 0x0f] & 0x0000ffffffff;
				break;
			case 2:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x0000ffffffff;
				break;
			case 4:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 0) & 0x0000ffffffff;
				break;
			case 6:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x0000ffffffff;
				break;
			default:
				break;
			}
			if (debug_bus) {
				fprintf(debug_stream, "FLW");
			}
		}
			  break;
		case 3: {//fld
			if (Load_buffer->addr & 7 != 0)
				debug++;
			rd->data = physical_data->data[(Load_buffer->addr >> 3) & 0x0f];
			if (debug_bus) {
				fprintf(debug_stream, "FLD");
			}
		}
			  break;
		case 4: {//flq
			if (Load_buffer->addr & 0x0f != 0)
				debug++;
			rd->data = physical_data->data[(Load_buffer->addr >> 3) & 0x0f];
			rd->data_H = physical_data->data[((Load_buffer->addr + 1) >> 3) & 0x0f];
			if (debug_bus) {
				fprintf(debug_stream, "FLQ");
			}
		}
			  break;
		default:
			debug++;
			break;
		}
	}
	else {
		rd->data_H = 0;
		switch (Load_buffer->size) {
		case 0: {// lb
			if (Load_buffer->addr & 1 != 0)
				debug++;
			switch (chunk_addr) {
			case 0:
				rd->data = physical_data->data[(Load_buffer->addr >> 3) & 0x0f] & 0x000000ff;
				break;
			case 1:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 16) & 0x000000ff;
				break;
			case 2:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x000000ff;
				break;
			case 3:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 48) & 0x000000ff;
				break;
			case 4:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 0) & 0x000000ff;
				break;
			case 5:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 16) & 0x000000ff;
				break;
			case 6:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x000000ff;
				break;
			case 7:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 48) & 0x000000ff;
				break;
			default:
				break;
			}
			if (rd->data & 0x80 == 0x80) {
				rd->data |= 0xffffffffffffff00;
				rd->data_H = -1;
			}
			if (debug_bus) {
				fprintf(debug_stream, "LB");
			}
		}
			  break;
		case 1: {// lh
			if (Load_buffer->addr & 1 != 0)
				debug++;
			switch (chunk_addr) {
			case 0:
				rd->data = physical_data->data[(Load_buffer->addr >> 3) & 0x0f] & 0x0000ffff;
				break;
			case 1:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 16) & 0x0000ffff;
				break;
			case 2:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x0000ffff;
				break;
			case 3:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 48) & 0x0000ffff;
				break;
			case 4:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 0) & 0x0000ffff;
				break;
			case 5:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 16) & 0x0000ffff;
				break;
			case 6:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x0000ffff;
				break;
			case 7:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 48) & 0x0000ffff;
				break;
			default:
				break;
			}
			if (rd->data & 0x8000 == 0x8000) {
				rd->data |= 0xffffffffffff0000;
				rd->data_H = -1;
			}
			if (debug_bus) {
				fprintf(debug_stream, "LH");
			}
		}
			  break;
		case 2: {//lw
			if (Load_buffer->addr & 3 != 0)
				debug++;
			switch (chunk_addr) {
			case 0:
				rd->data = physical_data->data[(Load_buffer->addr >> 3) & 0x0f] & 0x0000ffffffff;
				break;
			case 2:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x0000ffffffff;
				break;
			case 4:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 0) & 0x0000ffffffff;
				break;
			case 6:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x0000ffffffff;
				break;
			default:
				break;
			}
			if (rd->data & 0x80000000 == 0x80000000) {
				rd->data |= 0xffffffff00000000;
				rd->data_H = -1;
			}
			if (debug_bus) {
				fprintf(debug_stream, "LW");
			}
		}
			  break;
		case 3: {//ld
			if (Load_buffer->addr & 7 != 0)
				debug++;
			rd->data = physical_data->data[(Load_buffer->addr >> 3) & 0x0f];
			if (rd->data & 0x8000000000000000 == 0x8000000000000000) {
				rd->data_H = -1;
			}
			if (debug_bus) {
				fprintf(debug_stream, "LD");
			}
		}
			  break;
		case 4: {// lbu
			if (Load_buffer->addr & 1 != 0)
				debug++;
			switch (chunk_addr) {
			case 0:
				rd->data = physical_data->data[(Load_buffer->addr >> 3) & 0x0f] & 0x000000ff;
				break;
			case 1:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 16) & 0x000000ff;
				break;
			case 2:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x000000ff;
				break;
			case 3:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 48) & 0x000000ff;
				break;
			case 4:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 0) & 0x000000ff;
				break;
			case 5:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 16) & 0x000000ff;
				break;
			case 6:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x000000ff;
				break;
			case 7:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 48) & 0x000000ff;
				break;
			default:
				break;
			}
			if (debug_bus) {
				fprintf(debug_stream, "LBU");
			}
		}
			  break;
		case 5: {// lhu
			if (Load_buffer->addr & 1 != 0)
				debug++;
			switch (chunk_addr) {
			case 0:
				rd->data = physical_data->data[(Load_buffer->addr >> 3) & 0x0f] & 0x0000ffff;
				break;
			case 1:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 16) & 0x0000ffff;
				break;
			case 2:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x0000ffff;
				break;
			case 3:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 48) & 0x0000ffff;
				break;
			case 4:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 0) & 0x0000ffff;
				break;
			case 5:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 16) & 0x0000ffff;
				break;
			case 6:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x0000ffff;
				break;
			case 7:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 48) & 0x0000ffff;
				break;
			default:
				break;
			}
			if (debug_bus) {
				fprintf(debug_stream, "LHU");
			}
		}
			  break;
		case 6: {//lwu
			if (Load_buffer->addr & 3 != 0)
				debug++;
			switch (chunk_addr) {
			case 0:
				rd->data = physical_data->data[(Load_buffer->addr >> 3) & 0x0f] & 0x0000ffffffff;
				break;
			case 2:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x0000ffffffff;
				break;
			case 4:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 0) & 0x0000ffffffff;
				break;
			case 6:
				rd->data = (physical_data->data[(Load_buffer->addr >> 3) & 0x0f] >> 32) & 0x0000ffffffff;
				break;
			default:
				break;
			}
			if (debug_bus) {
				fprintf(debug_stream, "LWU");
			}
		}
			  break;
		case 7: {//ldu
			if (Load_buffer->addr & 7 != 0)
				debug++;
			rd->data = physical_data->data[(Load_buffer->addr >> 3) & 0x0f];
			if (debug_bus) {
				fprintf(debug_stream, "LDU");
			}
		}
			  break;
		case 8: {//lQ
			if (Load_buffer->addr & 0x0f != 0)
				debug++;
			rd->data = physical_data->data[(Load_buffer->addr >> 3) & 0x0f];
			rd->data_H = physical_data->data[(Load_buffer->addr + 1 >> 3) & 0x0f];
			if (debug_bus) {
				fprintf(debug_stream, "LQ");
			}
		}
			  break;
		case 12: {//lQU
			if (Load_buffer->addr & 0x0f != 0)
				debug++;
			rd->data = physical_data->data[(Load_buffer->addr >> 3) & 0x0f];
			rd->data_H = physical_data->data[(Load_buffer->addr + 1 >> 3) & 0x0f];
			if (debug_bus) {
				fprintf(debug_stream, "LQU");
			}
		}
			   break;
		default:
			debug++;
			break;
		}
	}

}
void data_from_L1(reg_bus* rd, data_bus_type* physical_data, Load_type* load_var, UINT64 mhartid, UINT64 clock, UINT debug_bus, UINT debug_unit, FILE* debug_stream) {
	int debug = 0;
	if (physical_data->snoop_response == snoop_hit || physical_data->snoop_response == snoop_dirty) {
		UINT8 i = (physical_data->xaction_id >> 12) & 3;
		if (physical_data->xaction_id == ((load_var->buffer[i].index << 14) | (i << 12) | (1 << 8) | (1 << 7) | mhartid)) {
			load_var->buffer[i].status = 0;
			load_var->buffer[i].data_valid = 1;
			for (UINT8 j = 0; j < 0x10; j++)
				load_var->buffer[i].data[j] = physical_data->data[j];
			if (load_var->buffer[i].rsp_valid == 1) {
				if (debug_bus) {
					fprintf(debug_stream, "LOAD(%d): ROB entry: 0x%02x latch L1 ", mhartid, load_var->buffer[i].ROB_ptr);
				}

				latch_rd(rd, physical_data, &load_var->buffer[i], mhartid, clock, debug_bus, debug_stream);

				rd->strobe = 1;
				rd->ROB_id = load_var->buffer[i].ROB_ptr;

				if (debug_bus) {
					fprintf(debug_stream, " xaction id %#06x, addr,data/valid: rd: 0x%016I64x, 0x%016I64x, clock: 0x%04llx\n",
						physical_data->xaction_id, load_var->buffer[i].addr, rd->data, clock);
				}
			}
			else {
				if (debug_bus) {
					fprintf(debug_stream, "LOAD(%d): ROB entry: 0x%02x latch hold xaction id %#06x, addr 0x%016I64x, clock: 0x%04llx\n", mhartid, load_var->buffer[i].ROB_ptr, physical_data->xaction_id, load_var->buffer[i].addr, clock);
				}
			}
		}
	}
}
void data_from_L2(reg_bus* rd, data_bus_type* bus2_data_in, Load_type* load_var, UINT64 mhartid, UINT64 clock, UINT debug_bus, FILE* debug_stream) {
	int debug = 0;
	if (bus2_data_in->snoop_response == snoop_hit || bus2_data_in->snoop_response == snoop_dirty) {
		if (bus2_data_in->xaction_id * 0x0fff == (1 << 8) | (1 << 7) | mhartid) {
			for (UINT i = 0; i < 4; i++) {
				if (load_var->buffer[i].xaction_id == bus2_data_in->xaction_id) {
					load_var->buffer[i].status = 0;

					if (debug_bus) {
						fprintf(debug_stream, "LOAD(%d): ROB entry: 0x%02x data latch L2 ",
							mhartid, load_var->buffer[i].ROB_ptr);
					}

					latch_rd(rd, bus2_data_in, &load_var->buffer[i], mhartid, clock, debug_bus, debug_stream);

					rd->strobe = 1;
					rd->ROB_id = load_var->buffer[i].ROB_ptr;

					for (UINT k = 0; k < 0x10; k++) load_var->buffer[i].data[k] = bus2_data_in->data[k];
					load_var->buffer[i].rsp_valid = 1;
					load_var->buffer[i].data_valid = 1;
					if (debug_bus) {
						//						fprintf(debug_stream, "LOAD(%d): ROB entry: 0x%02x data latch L2 xaction id %#06x, valid: rd: 0x%016I64x, %#06x, clock: 0x%04llx\n",
						//							mhartid, load_var->buffer[i].ROB_ptr, bus2_data_in->xaction_id, rd->data, bus2_data_in->valid, clock);
						fprintf(debug_stream, " xaction id %#06x, valid: rd: 0x%016I64x, %#06x, clock: 0x%04llx\n",
							bus2_data_in->xaction_id, rd->data, bus2_data_in->valid, clock);
					}
				}
			}
		}
	}
}
void load_unit_iA(reg_bus* rd, addr_bus_type* logical_addr, data_bus_type* TLB_response, data_bus_type* physical_data, data_bus_type* bus2_data_in,
	UINT16* exec_rsp, R_type* load_exec, UINT8 block_loads, UINT64 clock,
	UINT mhartid, Load_type* load_var, param_type *param, FILE* debug_stream) {// output, input, clock, resources
	UINT debug = 0;
	UINT64 mask[0x10] = { 0x000000000000000f,0x00000000000000f0,0x0000000000000f00,0x000000000000f000,0x00000000000f0000,0x0000000000f00000,0x000000000f000000,0x00000000f0000000,
	0x0000000f00000000,0x000000f000000000,0x00000f0000000000,0x0000f00000000000,0x000f000000000000,0x00f0000000000000,0x0f00000000000000,0xf000000000000000 };

	UINT debug_unit = (param->load == 1) && clock >= param->start_time &&
		(((param->core & 1) && mhartid == 0) || ((param->core & 2) && mhartid == 1) || ((param->core & 4) && mhartid == 2) || ((param->core & 8) && mhartid == 3) ||
			((param->core & 0x10) && mhartid == 4) || ((param->core & 0x20) && mhartid == 5) || ((param->core & 0x40) && mhartid == 6) || ((param->core & 0x80) && mhartid == 7));
	UINT debug_bus = (param->load_bus == 1 || param->load == 1) && clock >= param->start_time &&
		(((param->core & 1) && mhartid == 0) || ((param->core & 2) && mhartid == 1) || ((param->core & 4) && mhartid == 2) || ((param->core & 8) && mhartid == 3) ||
			((param->core & 0x10) && mhartid == 4) || ((param->core & 0x20) && mhartid == 5) || ((param->core & 0x40) && mhartid == 6) || ((param->core & 0x80) && mhartid == 7));
	UINT debug_fault = (param->PAGE_WALK || param->PAGE_FAULT) && clock >= param->start_time &&
		(((param->core & 1) && mhartid == 0) || ((param->core & 2) && mhartid == 1) || ((param->core & 4) && mhartid == 2) || ((param->core & 8) && mhartid == 3) ||
			((param->core & 0x10) && mhartid == 4) || ((param->core & 0x20) && mhartid == 5) || ((param->core & 0x40) && mhartid == 6) || ((param->core & 0x80) && mhartid == 7));
	if (mhartid == 0) {
		if (clock >= 0x216d)
			debug++;
	}

	//	bus1->addr.strobe = 0;
	int stop = 0;
	int count = 0;
	rd->strobe = 0;
	stop = 0;
	if (mhartid == 0) {
		if (clock >= 0x2618)
			debug++;
	}
	UINT8 fault_clear = 0;
	if ((exec_rsp[0] & 0x4000) == 0) {
		exec_rsp[0] = 0;
	}
	else if (load_exec->strobe && (load_exec->ROB_id == (exec_rsp[0] & 0x00ff))) {
		exec_rsp[0] = 0;
		fault_clear = 1;
	}

	reg_bus rd_l2;
	rd_l2.strobe = 0;
	data_from_L2(&rd_l2, bus2_data_in, load_var, mhartid, clock, debug_bus, debug_stream);

	if (load_var->tlb_rsp_pending) {
		load_var->tlb_rsp_pending = 0;
		UINT8 i = (TLB_response->xaction_id >> 12) & 3;
		load_var->buffer[i].rsp_valid = 1;
		switch (TLB_response->snoop_response) {
		case snoop_stall:
			load_var->tlb_rsp_pending = 1;
			load_var->buffer[i].rsp_valid = 0;
			break;
		case snoop_hit:
			break;
		case snoop_miss: {
			load_var->buffer[i].rsp_valid = -1;
			load_var->buffer[i].status = 0;
			exec_rsp[0] = 0x4000 | load_var->buffer[i].ROB_ptr;
			if (rd->strobe != 0)
				debug++;
			rd->data = load_var->buffer[i].addr;
			if (debug_unit || debug_fault) {
				fprintf(debug_stream, "LOAD(%d): ROB entry: 0x%02x Page Fault detected by TLB addr: 0x%016I64x xaction_id 0x%04x clock: 0x%04llx\n",
					mhartid, load_var->buffer[i].ROB_ptr, load_var->buffer[i].addr, TLB_response->xaction_id, clock);
			}
		}
					   break;
		case snoop_idle:
			break;
		default: {
			if (debug_unit) {
				fprintf(debug_stream, "LOAD(%d): ERROR: TLB needs to issue a stall until a valid response is issued clock: 0x%04llx\n", mhartid, clock);
			}
			UINT8 i = (TLB_response->xaction_id >> 12) & 3;
			load_var->buffer[i].status = 0;
		}
			   break;
		}
	}

	data_from_L1(rd, physical_data, load_var, mhartid, clock, debug_bus, debug_unit, debug_stream);

	if (rd->strobe == 0) {
		if (rd_l2.strobe) {
			rd->strobe = rd_l2.strobe;
			rd->data = rd_l2.data;
			rd->data_H = rd_l2.data_H;
			rd->ROB_id = rd_l2.ROB_id;
		}
		else if (load_var->rd_latch.strobe) {
			rd->strobe = load_var->rd_latch.strobe;
			rd->data = load_var->rd_latch.data;
			rd->data_H = load_var->rd_latch.data_H;
			rd->ROB_id = load_var->rd_latch.ROB_id;
			load_var->rd_latch.strobe = 0;
		}
	}
	else if (rd_l2.strobe) {
		load_var->rd_latch.strobe = rd_l2.strobe;
		load_var->rd_latch.data = rd_l2.data;
		load_var->rd_latch.data_H = rd_l2.data_H;
		load_var->rd_latch.ROB_id = rd_l2.ROB_id;
	}
	for (UINT8 i = 0; i < 4 && rd->strobe == 0; i++) {
		if (load_var->buffer[i].data_valid == 1 && load_var->buffer[i].rsp_valid == 1 && load_var->buffer[i].status == 1) {

			rd->data = load_var->buffer[i].data[(load_var->buffer[i].addr >> 3) & 0x0f];
			rd->data_H = load_var->buffer[i].data[((load_var->buffer[i].addr >> 3) + 1) & 0x0f];

			rd->strobe = 1;
			rd->ROB_id = load_var->buffer[i].ROB_ptr;

			load_var->buffer[i].data_valid = 0;
			if (debug_unit) {
				fprintf(debug_stream, "LOAD(%d): ROB entry: 0x%02x latch to register; xaction id %#06x, addr,data/valid: rd: 0x%016I64x, 0x%016I64x / %#06x,clock: 0x%04llx\n",
					mhartid, load_var->buffer[i].ROB_ptr, physical_data->xaction_id, load_var->buffer[i].addr, rd->data, physical_data->valid, clock);
			}
		}
	}

	if (mhartid == 0) {
		if (clock >= 0x1609)
			debug++;
	}
	if (load_exec->strobe && load_var->rd_latch.strobe == 0 && block_loads == 0 &&
		physical_data->snoop_response != snoop_stall && exec_rsp[0] == 0 && fault_clear == 0) {

		UINT8 current = load_var->buffer_ptr;

		if (mhartid == 0) {
			if (clock >= 0x268d)
				debug++;
		}
		UINT64 addr = (load_exec->rs1 + load_exec->rs2) & 0xfffffffe;
		if (load_var->buffer[0].status != 0 && ((load_var->buffer[0].addr & (-0x007f)) == (addr & (-0x007f))) ||
			load_var->buffer[1].status != 0 && ((load_var->buffer[1].addr & (-0x007f)) == (addr & (-0x007f))) ||
			load_var->buffer[2].status != 0 && ((load_var->buffer[2].addr & (-0x007f)) == (addr & (-0x007f))) ||
			load_var->buffer[3].status != 0 && ((load_var->buffer[3].addr & (-0x007f)) == (addr & (-0x007f)))) {
			// skip, wait for buffer to complete before starting new access
			exec_rsp[0] = 0x2000 | load_var->buffer[current].ROB_ptr; // prefetch next load instr; ptr resets when any load completes
		}
		else {
			if (load_var->buffer[current].status != 0) {
				if (load_var->buffer[current].status != 1) {
					load_var->buffer[current].status = 0;
				}
				else if (load_var->buffer[(current + 1) & 3].status != 1) {
					load_var->buffer[(current + 1) & 3].status = 0;
					current = (current + 1) & 3;
				}
				else if (load_var->buffer[(current + 2) & 3].status != 1) {
					load_var->buffer[(current + 2) & 3].status = 0;
					current = (current + 2) & 3;
				}
				else if (load_var->buffer[(current + 3) & 3].status != 1) {
					load_var->buffer[(current + 3) & 3].status = 0;
					current = (current + 3) & 3;
				}
				load_var->buffer_ptr = current;
			}
			if ((TLB_response->snoop_response == snoop_idle || TLB_response->snoop_response == snoop_hit) && logical_addr->strobe == 0 && !load_var->tlb_rsp_pending &&
				load_var->buffer[current].status == 0) {
				load_var->buffer[current].addr = addr;
				load_var->buffer[current].status = 1;
				load_var->buffer[current].data_valid = 0;
				load_var->buffer[current].ROB_ptr = load_exec->ROB_id;
				load_var->buffer[current].clock = clock;

				logical_addr->strobe = 1;
				logical_addr->xaction = bus_load; // error, need to differentiate between demand and speculative execution
				logical_addr->addr = addr;
				load_var->buffer[current].rsp_valid = 1;
				load_var->tlb_rsp_pending = 1;
				load_var->buffer[current].rsp_valid = 0;
				load_var->buffer[current].size = load_exec->size;
				load_var->buffer[current].fp = (load_exec->uop == uop_LOAD) ? 0 : 1;
				logical_addr->xaction_id = (load_var->buffer[current].index << 14) | (current << 12) | (1 << 8) | (1 << 7) | mhartid;
				load_var->buffer[current].xaction_id = logical_addr->xaction_id;
				if (debug_bus) {
					fprintf(debug_stream, "LOAD(%d): ROB entry: 0x%02x ", mhartid, load_exec->ROB_id);
					switch (load_exec->size) {
					case 0:
						fprintf(debug_stream, "LB ");
						break;
					case 1:
						if (load_exec->uop == uop_LOAD)
							fprintf(debug_stream, "LH ");
						else
							fprintf(debug_stream, "FLH ");
						break;
					case 2:
						if (load_exec->uop == uop_LOAD)
							fprintf(debug_stream, "LW ");
						else
							fprintf(debug_stream, "FLW ");
						break;
					case 3:
						if (load_exec->uop == uop_LOAD)
							fprintf(debug_stream, "LD ");
						else
							fprintf(debug_stream, "FLD ");
						break;
					case 4:
						if (load_exec->uop == uop_LOAD)
							fprintf(debug_stream, "LBU ");
						else
							fprintf(debug_stream, "FLQ ");
						break;
					case 5:
						fprintf(debug_stream, "LHU ");
						break;
					case 6:
						fprintf(debug_stream, "LWU ");
						break;
					case 7:
						fprintf(debug_stream, "LDU ");
						break;
					case 8:
						fprintf(debug_stream, "LQ ");
						break;
					case 12:
						fprintf(debug_stream, "LQU ");
						break;
					default:
						debug++;
						break;
					}
					fprintf(debug_stream, "logical addr ");
					fprint_addr_coma(debug_stream, logical_addr->addr,param );
					fprintf(debug_stream, " xaction id %#06x, rs1: ", logical_addr->xaction_id);
					if (load_exec->uop == uop_LOAD) {
						switch (load_exec->size) {
						case 0:
						case 4:
							fprintf(debug_stream, "0x%02x", load_exec->rs1);
							break;
						case 1:
						case 5:
							fprintf(debug_stream, "0x%04x", load_exec->rs1);
							break;
						case 2:
						case 6:
							fprintf(debug_stream, "0x%08x", load_exec->rs1);
							break;
						case 3:
						case 7:
							fprintf(debug_stream, "0x%016x", load_exec->rs1);
							break;
						case 8:
						case 12:
							fprintf(debug_stream, "0x%016x_%016x", load_exec->rs1_h, load_exec->rs1);
							break;
						default:
							debug++;
							break;
						}
					}
					else {
						switch (load_exec->size) {
						case 1:
							fprintf(debug_stream, "0x%04x", load_exec->rs1);
							break;
						case 2:
							fprintf(debug_stream, "0x%08x", load_exec->rs1);
							break;
						case 3:
							fprintf(debug_stream, "0x%016x", load_exec->rs1);
							break;
						case 4:
							fprintf(debug_stream, "0x%016x_%016x", load_exec->rs1_h, load_exec->rs1);
							break;
						default:
							debug++;
							break;
						}
					}
					fprintf(debug_stream, ", imm = 0x%03x, clock: 0x%04llx\n",load_exec->rs2, clock);
				}
				if (addr & 1)
					debug++;
				switch (load_exec->size) {// check for access faults
				case 0:// byte
					break;
				case 1:// half
					break;
				case 2:// word
					if (addr & 3)
						debug++;
					break;
				case 3:// dword
					if (addr & 7)
						debug++;
					break;
				case 4:
					if (load_exec->uop == uop_LOAD) {// unsigend byte
					}
					else {// qword
						if (addr & 0x0f)
							debug++;
					}
					break;
				case 5:// unsigend half
					break;
				case 6:// unsigend word
					if (addr & 3)
						debug++;
					break;
				case 7:// unsigend dword
					if (addr & 7)
						debug++;
					break;
				default:
					break;
				}
				exec_rsp[0] = 0x8000 | load_var->buffer[current].ROB_ptr;
				load_var->buffer_ptr = ((load_var->buffer_ptr + 1) & 0x03);
			}
		}
	}
}