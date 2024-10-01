// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "front_end.h"

void fprint_prefetch_latch_data(FILE* debug_stream,UINT64*data, UINT xaction_id, UINT64 tag, UINT64 clock, UINT8 mhartid, param_type* param) {
	fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, tag: ", mhartid, xaction_id);
	fprint_addr_coma(debug_stream, tag, param);
	fprintf(debug_stream, " 0x00 0x%08x %08x, 0x08 0x%08x %08x, 0x10 0x%08x %08x, 0x18 0x%08x %08x, clock: 0x%04llx\n",
		 data[0] >> 32, data[0], data[1] >> 32, data[1], data[2] >> 32, data[2], data[3] >> 32, data[3], clock);
	fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, tag: ", mhartid, xaction_id);
	fprint_addr_coma(debug_stream, tag, param);
	fprintf(debug_stream, " 0x20 0x%08x %08x, 0x28 0x%08x %08x, 0x30 0x%08x %08x, 0x38 0x%08x %08x, clock: 0x%04llx\n",
		 data[4] >> 32, data[4], data[5] >> 32, data[5], data[6] >> 32, data[6], data[7] >> 32, data[7], clock);
	fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, tag: ", mhartid, xaction_id);
	fprint_addr_coma(debug_stream, tag, param);
	fprintf(debug_stream, " 0x40 0x%08x %08x, 0x48 0x%08x %08x, 0x50 0x%08x %08x, 0x58 0x%08x %08x, clock: 0x%04llx\n",
		 data[8] >> 32, data[8], data[9] >> 32, data[9], data[10] >> 32, data[10], data[11] >> 32, data[11], clock);
	fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, tag: ", mhartid, xaction_id);
	fprint_addr_coma(debug_stream, tag, param);
	fprintf(debug_stream, " 0x60 0x%08x %08x, 0x68 0x%08x %08x, 0x70 0x%08x %08x, 0x78 0x%08x %08x, clock: 0x%04llx\n",
		 data[12] >> 32, data[12], data[13] >> 32, data[13], data[14] >> 32, data[14], data[15] >> 32, data[15], clock);
}
void L2_latch_aux_buffer(prefetcher_type* prefetcher, data_bus_type* bus2_data_in, UINT64 clock, UINT mhartid, UINT debug_unit, param_type* param, FILE* debug_stream) {
	int debug = 0;
	UINT8 buffer_ptr = (bus2_data_in->xaction_id >> 8) & 3;
	UINT8 line = (bus2_data_in->xaction_id >> 12) & 3;
	UINT hit = 0;
	for (UINT8 j = 0; j < 4; j++) {
		if (prefetcher->buffer[j].entry[line].xaction_id == bus2_data_in->xaction_id &&
			prefetcher->buffer[j].entry[line].status != fetch_invalid &&
			prefetcher->buffer[j].entry[line].status != fetch_available) {
			buffer_ptr = j;
			for (UINT8 i = 0; i < 0x10; i++) {
				prefetcher->buffer[buffer_ptr].data[(line << 5) | (2 * i)] = bus2_data_in->data[i];
				prefetcher->buffer[buffer_ptr].data[(line << 5) | (2 * i) | 1] = bus2_data_in->data[i] >> 32;
			}
			prefetcher->buffer[buffer_ptr].entry[line].status = fetch_available;
			prefetcher->buffer[buffer_ptr].entry[line].time = clock;
			prefetcher->buffer[buffer_ptr].data_count += 0x10;
			if (debug_unit) {
				fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, tag: ", mhartid, bus2_data_in->xaction_id);
				fprint_addr_coma(debug_stream, prefetcher->buffer[buffer_ptr].entry[line].tag, param);
				fprintf(debug_stream, " bus latch aux PC(%d.%d): ", prefetcher->demand_ptr, line);
				fprint_addr_coma(debug_stream, prefetcher->buffer[prefetcher->demand_ptr].PC, param);
				fprintf(debug_stream, " clock: 0x%04llx\n", clock);
				fprint_prefetch_latch_data(debug_stream, bus2_data_in->data, bus2_data_in->xaction_id, prefetcher->buffer[prefetcher->demand_ptr].entry[line].tag, clock, mhartid, param);
			}
			hit = 1;
		}
	}
	if (debug_unit) {
		if (hit == 0) {
			fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, Dropped Bus return data; current core id: %#06x, clock: 0x%04llx\n",
				mhartid, bus2_data_in->xaction_id, (prefetcher->buffer[buffer_ptr].unique << 10) | (line << 12) | (buffer_ptr << 8) | (0 << 7) | mhartid, clock);
		}
	}
}

void L0_latch_aux_buffer(prefetcher_type* prefetcher, data_bus_type* physical_data, UINT64 clock, UINT mhartid, UINT debug_unit, param_type* param, FILE* debug_stream) {
	UINT8 buffer_ptr = (physical_data->xaction_id >> 8) & 3;
	UINT8 line = (physical_data->xaction_id >> 12) & 3;
	for (UINT8 i = 0; i < 0x10; i++) {
		prefetcher->buffer[buffer_ptr].data[(line << 5) | (2 * i)] = physical_data->data[i];
		prefetcher->buffer[buffer_ptr].data[(line << 5) | (2 * i) | 1] = physical_data->data[i] >> 32;
	}
	prefetcher->buffer[buffer_ptr].entry[line].status = fetch_available;
	prefetcher->buffer[buffer_ptr].entry[line].time = clock;
	prefetcher->buffer[buffer_ptr].data_count += 0x10;
	if (debug_unit) {
		fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, tag: ", mhartid, physical_data->xaction_id);
		fprint_addr_coma(debug_stream, prefetcher->buffer[buffer_ptr].entry[line].tag, param);
		fprintf(debug_stream, " prefetch bus latch PC(%d.%d): ", prefetcher->demand_ptr, line);
		fprint_addr_coma(debug_stream, prefetcher->buffer[prefetcher->demand_ptr].PC, param);
		fprintf(debug_stream, " clock: 0x%04llx\n", clock);

		fprint_prefetch_latch_data(debug_stream, physical_data->data, physical_data->xaction_id, prefetcher->buffer[buffer_ptr].entry[line].tag, clock, mhartid, param);
	}
}
// ISSUE - need to limit reg access to CSR regs 
// Output: logical_addr
// Input: logical_data (fault code), 
void prefetch_unit(decode_shifter_struct*shifter, reg_bus* rd, addr_bus_type* logical_addr, data_bus_type* TLB_response, data_bus_type* physical_data, data_bus_type* bus2_data_in, retire_type *retire, reg_bus* rd_branch,
	UINT8 ROB_stall, UINT8 on_branch, UINT mhartid, char interrupt, prefetcher_type* prefetcher, UINT8 reset, reg_bus* rd_JALR,UINT8 *prefetcher_busy, UINT8 active_IO, UINT64 clock, UINT debug_core, param_type *param, FILE* debug_stream) {
	int debug = 0;

	UINT debug_unit = (param->prefetcher == 1) && debug_core;
	UINT debug_unit_walk = ((param->prefetcher == 1) || param->PAGE_WALK || param->PAGE_FAULT) && debug_core;
	UINT debug_unit_branch = ((param->prefetcher == 1) || param->branch) && debug_core;

	prefetcher->idle_flag = 1;
	rd->strobe = 0;
	for (UINT8 i = 0; i < 4; i++) {
		for (UINT j = 0; j < 4; j++) {
			if (prefetcher->buffer[i].entry[j].status == fetch_incoming || prefetcher->buffer[i].entry[j].status == fetch_q_hit) {
				prefetcher->idle_flag = 0;
			}
		}
	}
	if (mhartid == 0) {
		if (clock >= 0x2681)
			debug++;

		if (clock >= 0x00db)
			debug++;
	}
	prefetcher_busy[0] = 1;
	if (active_IO)
		prefetcher_busy[0] = 0;
	for (UINT8 i = 0; i < 4; i++) {
		for (UINT8 j = 0; j < 4; j++) {
			if (prefetcher->buffer[i].entry[j].status != fetch_invalid && prefetcher->buffer[i].entry[j].status != fetch_valid && prefetcher->buffer[i].entry[j].status != fetch_available)
				prefetcher_busy[0] = 2;
		}
	}
	if (reset) {
		for (UINT8 i = 0; i < 4; i++) {
			prefetcher->buffer[i].entry[0].status = fetch_invalid; // halt all prefetchers
			prefetcher->buffer[i].entry[1].status = fetch_invalid;
			prefetcher->buffer[i].entry[2].status = fetch_invalid;
			prefetcher->buffer[i].entry[3].status = fetch_invalid;

			prefetcher->buffer[i].PC = 0;
			prefetcher->buffer[i].unique = 0;
			prefetcher->buffer[i].data_count = 0;
			prefetcher->buffer[i].sm = 0;

		}
		prefetcher->demand_ptr = 0;
		prefetcher->aux_ptr = 0;
		shifter->valid = 0;
		prefetcher->halt = 1;
	}
	else if (prefetcher->reset_latch == 1) {
		prefetcher->buffer[0].PC = retire->PC;
		if (debug_unit) {
			fprintf(debug_stream, "PREFETCHER(%lld): ", mhartid);
			fprintf(debug_stream, "exiting reset, reset vector: 0x%016I64x, clock: 0x%04llx\n", prefetcher->buffer[0].PC, clock);
		}
		prefetcher->buffer[0].entry[0].status = fetch_valid;
		prefetcher->buffer[0].entry[1].status = fetch_valid;
		prefetcher->buffer[0].entry[2].status = fetch_valid;
		prefetcher->buffer[0].entry[3].status = fetch_valid;
		prefetcher->halt = 0;
	}
	else {
		if (retire->load_PC & 0x10 || shifter->response.msg == halt) {// wfi
			for (UINT8 i = 0; i < 4; i++) {
				for (UINT8 j = 0; j < 4; j++)
					if (prefetcher->buffer[i].entry[j].status != fetch_incoming)
						prefetcher->buffer[i].entry[j].status = fetch_invalid; // halt all prefetchers

				prefetcher->buffer[i].PC = 0;
				prefetcher->buffer[i].unique = 0;
				prefetcher->buffer[i].data_count = 0;
				prefetcher->buffer[i].sm = 0;

				prefetcher->victim.entry[i].status = fetch_invalid;
			}
			prefetcher->demand_ptr = -1;
			prefetcher->aux_ptr = 0;
	//		prefetcher->shift_buf_valid = 0;
			shifter->valid = 0;
			prefetcher->flush = 0;
			prefetcher->halt = 1;
			if (debug_unit) {
				fprintf(debug_stream, "PREFETCHER(%lld): Preftecher halted; clock: 0x%04llx\n", mhartid, clock);
			}
		}
		else if (retire->load_PC & 0x10 || (shifter->response.msg == service_fault)) {
			for (UINT8 i = 0; i < 4; i++) {
				for (UINT8 j = 0; j < 4; j++) {
					if (prefetcher->buffer[i].entry[j].status != fetch_incoming) {
						prefetcher->buffer[i].entry[j].status = fetch_invalid; // halt all prefetchers
					}
				}
				prefetcher->buffer[i].PC = 0;
				prefetcher->buffer[i].unique = 0;
				prefetcher->buffer[i].data_count = 0;
				prefetcher->buffer[i].sm = 0;

				prefetcher->victim.entry[i].status = fetch_invalid;
			}
			prefetcher->demand_ptr = (prefetcher->demand_ptr + 1) & 3;
			if ((shifter->response.msg == service_fault))
				prefetcher->buffer[prefetcher->demand_ptr].PC = shifter->response.addr;
			else
				prefetcher->buffer[prefetcher->demand_ptr].PC = retire->PC;
			if (prefetcher->buffer[prefetcher->demand_ptr].entry[0].status != fetch_invalid ||
				prefetcher->buffer[prefetcher->demand_ptr].entry[1].status != fetch_invalid ||
				prefetcher->buffer[prefetcher->demand_ptr].entry[2].status != fetch_invalid ||
				prefetcher->buffer[prefetcher->demand_ptr].entry[3].status != fetch_invalid)
				debug++;
			prefetcher->buffer[prefetcher->demand_ptr].entry[0].status = fetch_valid;
			prefetcher->buffer[prefetcher->demand_ptr].entry[1].status = fetch_valid;
			prefetcher->buffer[prefetcher->demand_ptr].entry[2].status = fetch_valid;
			prefetcher->buffer[prefetcher->demand_ptr].entry[3].status = fetch_valid;

			prefetcher->aux_ptr = 0;
			shifter->valid = 0;
			prefetcher->flush = 0;
			prefetcher->halt = 0;
			if (debug_unit) {
				fprintf(debug_stream, "PREFETCHER(%lld): Fault being serviced; PC(%d) ", 
					mhartid, prefetcher->demand_ptr);
				fprint_addr_coma(debug_stream, prefetcher->buffer[prefetcher->demand_ptr].PC, param);
				fprintf(debug_stream, " clock: 0x%04llx\n", clock);
			}
		}
		else if (((retire->load_PC && retire->load_PC != 7 ) || rd_branch->strobe)) {// need a true reset - stop all prefetchers, then load demand 
			prefetcher->flush = 0;
			prefetcher->halt = 0;
			prefetcher->demand_ptr = (prefetcher->demand_ptr + 1) & 3;
			if (rd_branch->strobe)
				prefetcher->buffer[prefetcher->demand_ptr].PC = rd_branch->data;
			else if (retire->load_PC == 4) {
				prefetcher->buffer[prefetcher->demand_ptr].PC = retire->PC;
			}
			else {
				prefetcher->buffer[prefetcher->demand_ptr].PC = rd_branch->data;
			}
			prefetcher->buffer[prefetcher->demand_ptr].entry[0].status = fetch_valid;
			prefetcher->buffer[prefetcher->demand_ptr].entry[1].status = fetch_valid;
			prefetcher->buffer[prefetcher->demand_ptr].entry[2].status = fetch_valid;
			prefetcher->buffer[prefetcher->demand_ptr].entry[3].status = fetch_valid;
			UINT index = (prefetcher->buffer[prefetcher->demand_ptr].PC >> 7) & 3;
			switch (index) {
			case 0:
				prefetcher->buffer[prefetcher->demand_ptr].entry[0].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x000;
				prefetcher->buffer[prefetcher->demand_ptr].entry[1].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x080;
				prefetcher->buffer[prefetcher->demand_ptr].entry[2].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x100;
				prefetcher->buffer[prefetcher->demand_ptr].entry[3].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x180;
				break;
			case 1:
				prefetcher->buffer[prefetcher->demand_ptr].entry[0].tag = ((prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x000) + 0x200;
				prefetcher->buffer[prefetcher->demand_ptr].entry[1].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x080;
				prefetcher->buffer[prefetcher->demand_ptr].entry[2].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x100;
				prefetcher->buffer[prefetcher->demand_ptr].entry[3].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x180;
				break;
			case 2:
				prefetcher->buffer[prefetcher->demand_ptr].entry[0].tag = ((prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x000) + 0x200;
				prefetcher->buffer[prefetcher->demand_ptr].entry[1].tag = ((prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x080) + 0x200;
				prefetcher->buffer[prefetcher->demand_ptr].entry[2].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x100;
				prefetcher->buffer[prefetcher->demand_ptr].entry[3].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x180;
				break;
			case 3:
				prefetcher->buffer[prefetcher->demand_ptr].entry[0].tag = ((prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x000) + 0x200;
				prefetcher->buffer[prefetcher->demand_ptr].entry[1].tag = ((prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x080) + 0x200;
				prefetcher->buffer[prefetcher->demand_ptr].entry[2].tag = ((prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x100) + 0x200;
				prefetcher->buffer[prefetcher->demand_ptr].entry[3].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x180;
				break;
			default:
				break;
			}
			prefetcher->buffer[prefetcher->demand_ptr].unique = ((prefetcher->buffer[prefetcher->demand_ptr].unique + 1) & 3);
		//	prefetcher->shift_buf_valid = 0;
			shifter->valid = 0;
			if (debug_unit_branch) {
				fprintf(debug_stream, "PREFETCHER(%lld): Misspredicted Branch (hit 0); next addr ", mhartid);
				fprint_addr_coma(debug_stream, prefetcher->buffer[prefetcher->demand_ptr].PC, param);
				fprintf(debug_stream, " clock: 0x%04llx\n", clock);
			}
		}
		else if (on_branch && prefetcher->demand_ptr != 0xff) {
			if (prefetcher->buffer[prefetcher->demand_ptr].entry[0].status == fetch_invalid) {
				prefetcher->buffer[prefetcher->demand_ptr].entry[0].status = fetch_valid;
			}
			if (prefetcher->buffer[prefetcher->demand_ptr].entry[1].status == fetch_invalid) {
				prefetcher->buffer[prefetcher->demand_ptr].entry[1].status = fetch_valid;
			}
			if (prefetcher->buffer[prefetcher->demand_ptr].entry[2].status == fetch_invalid) {
				prefetcher->buffer[prefetcher->demand_ptr].entry[2].status = fetch_valid;
			}
			if (prefetcher->buffer[prefetcher->demand_ptr].entry[3].status == fetch_invalid) {
				prefetcher->buffer[prefetcher->demand_ptr].entry[3].status = fetch_valid;
			}
		}
		if (prefetcher->halt == 0) {
			if (prefetcher->flush == 1) {// check for buffer hit
				UINT8 hit = 0;
				for (UINT8 i = 0; i < 4 && !hit; i++) {
					UINT8 index = (retire->PC >> 7) & 3;
					if (((retire->PC >> 6) >= (prefetcher->buffer[prefetcher->demand_ptr].PC >> 6)) &&
						((retire->PC >> 6) < ((prefetcher->buffer[prefetcher->demand_ptr].PC >> 6) + 4)) &&
						(prefetcher->buffer[prefetcher->demand_ptr].entry[index].status == fetch_available)) {

						if (prefetcher->demand_ptr == i) {
							prefetcher->buffer[prefetcher->demand_ptr].PC = retire->PC;
							if (debug_unit_branch) {
								fprintf(debug_stream, "PREFETCHER(%lld): Misspredicted Branch, fetcher hit; next addr ", mhartid);
								fprint_addr_coma(debug_stream, retire->PC, param);
								fprintf(debug_stream, " clock: 0x%04llx\n", clock);
							}
							hit = 1;
						}
					}
					else if (prefetcher->buffer[i].entry[0].status == fetch_invalid && prefetcher->buffer[i].entry[1].status == fetch_invalid && prefetcher->buffer[i].entry[2].status == fetch_invalid && prefetcher->buffer[i].entry[3].status == fetch_invalid) {
						prefetcher->demand_ptr = i;

						hit = 1;

						prefetcher->buffer[prefetcher->demand_ptr].PC = retire->PC;
						prefetcher->buffer[prefetcher->demand_ptr].entry[0].status = fetch_valid;
						prefetcher->buffer[prefetcher->demand_ptr].entry[1].status = fetch_valid;
						prefetcher->buffer[prefetcher->demand_ptr].entry[2].status = fetch_valid;
						prefetcher->buffer[prefetcher->demand_ptr].entry[3].status = fetch_valid;
						UINT index = (prefetcher->buffer[prefetcher->demand_ptr].PC >> 7) & 3;
						switch (index) {
						case 0:
							prefetcher->buffer[prefetcher->demand_ptr].entry[0].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x000;
							prefetcher->buffer[prefetcher->demand_ptr].entry[1].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x080;
							prefetcher->buffer[prefetcher->demand_ptr].entry[2].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x100;
							prefetcher->buffer[prefetcher->demand_ptr].entry[3].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x180;
							break;
						case 1:
							prefetcher->buffer[prefetcher->demand_ptr].entry[0].tag = ((prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x000) + 0x200;
							prefetcher->buffer[prefetcher->demand_ptr].entry[1].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x080;
							prefetcher->buffer[prefetcher->demand_ptr].entry[2].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x100;
							prefetcher->buffer[prefetcher->demand_ptr].entry[3].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x180;
							break;
						case 2:
							prefetcher->buffer[prefetcher->demand_ptr].entry[0].tag = ((prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x000) + 0x200;
							prefetcher->buffer[prefetcher->demand_ptr].entry[1].tag = ((prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x080) + 0x200;
							prefetcher->buffer[prefetcher->demand_ptr].entry[2].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x100;
							prefetcher->buffer[prefetcher->demand_ptr].entry[3].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x180;
							break;
						case 3:
							prefetcher->buffer[prefetcher->demand_ptr].entry[0].tag = ((prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x000) + 0x200;
							prefetcher->buffer[prefetcher->demand_ptr].entry[1].tag = ((prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x080) + 0x200;
							prefetcher->buffer[prefetcher->demand_ptr].entry[2].tag = ((prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x100) + 0x200;
							prefetcher->buffer[prefetcher->demand_ptr].entry[3].tag = (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x1ff)) | 0x180;
							break;
						default:
							break;
						}
						prefetcher->buffer[prefetcher->demand_ptr].unique = ((prefetcher->buffer[prefetcher->demand_ptr].unique + 1) & 3);
						if (debug_unit_branch) {
							fprintf(debug_stream, "PREFETCHER(%lld): Misspredicted Branch (hit); next addr ", mhartid);
							fprint_addr_coma(debug_stream, retire->PC, param);
							fprintf(debug_stream, " clock: 0x%04llx\n", clock);
						}
						if (((retire->PC >> 6) == (prefetcher->victim.entry[index].tag >> 6)) &&
							(prefetcher->victim.entry[index].status == fetch_available)) {

							prefetcher->buffer[prefetcher->demand_ptr].entry[index].status = fetch_available;
							prefetcher->buffer[prefetcher->demand_ptr].entry[index].tag = prefetcher->victim.entry[index].tag;
							prefetcher->buffer[prefetcher->demand_ptr].entry[index].xaction_id = prefetcher->victim.entry[index].xaction_id;
							for (UINT8 i = 0; i < 0x20; i++)prefetcher->buffer[prefetcher->demand_ptr].data[(index << 5) | i] = prefetcher->victim.data[(index << 5) | i];
						}

					}
				}
				prefetcher->flush = 0;
				if (!hit) {// need to figure out clear prefetchers for long jumps, context switching: csr_priviledge change
					prefetcher->demand_ptr = (prefetcher->demand_ptr + 1) & 3;
					prefetcher->buffer[prefetcher->demand_ptr].PC = retire->PC;
					prefetcher->buffer[prefetcher->demand_ptr].entry[0].status = fetch_valid;
					prefetcher->buffer[prefetcher->demand_ptr].entry[1].status = fetch_valid;
					prefetcher->buffer[prefetcher->demand_ptr].entry[2].status = fetch_valid;
					prefetcher->buffer[prefetcher->demand_ptr].entry[3].status = fetch_valid;
					prefetcher->buffer[prefetcher->demand_ptr].unique = ((prefetcher->buffer[prefetcher->demand_ptr].unique + 1) & 3);
					if (debug_unit_branch) {
						fprintf(debug_stream, "PREFETCHER(%lld): Misspredicted Branch (miss); next addr ",   clock);
						fprint_addr_coma(debug_stream, retire->PC, param);
						fprintf(debug_stream, " clock: 0x%04llx\n",  clock);
					}
				}
				shifter->valid = 0;
			}
			else if (prefetcher->buffer[prefetcher->demand_ptr].entry[0].status == fetch_fault) {
				if ((retire->PC & (~0x0fff)) == (prefetcher->buffer[prefetcher->demand_ptr].PC & (~0x0fff))) {
					rd->strobe = 1;
					rd->data = prefetcher->fault_addr;
					/*
					csr[csr_scause].value |= 0x00001000;										// set bit 12, instruction page fault
					csr[csr_mcause].value |= 0x00001000;										// set bit 12, instruction page fault
					csr[csr_mtval].value = csr[csr_stval].value = prefetcher->fault_addr;// save offending address - ISSUE, need to drop addr from data
					csr[csr_sepc].value = csr[csr_mepc].value = retire->PC;// save offending address - ISSUE, need to drop addr from data
					/**/
					if (debug_unit_walk) {
//						fprintf(debug_stream, "PREFETCHER(%lld): Instr Page Fault Handler Triggered; addr: 0x%016I64x, resturn addr: 0x%016I64x, clock: 0x%04llx\n",
//							mhartid, prefetcher->fault_addr, csr[csr_mepc].value, clock);
						fprintf(debug_stream, "PREFETCHER(%lld): Instr Page Fault Handler Triggered; addr: 0x%016I64x, clock: 0x%04llx\n",
							mhartid, prefetcher->fault_addr,  clock);
					}
				}
			}
			else if (rd_JALR->strobe) {
				prefetcher->demand_ptr = ((prefetcher->demand_ptr + 1) & 3);
				UINT8 next1_ptr = ((prefetcher->demand_ptr + 1) & 3);
				UINT8 next2_ptr = ((prefetcher->demand_ptr + 2) & 3);
				UINT8 next3_ptr = ((prefetcher->demand_ptr + 3) & 3);
				if (prefetcher->buffer[next1_ptr].entry[0].status != fetch_incoming &&
					prefetcher->buffer[next1_ptr].entry[1].status != fetch_incoming &&
					prefetcher->buffer[next1_ptr].entry[2].status != fetch_incoming &&
					prefetcher->buffer[next1_ptr].entry[3].status != fetch_incoming)
					prefetcher->demand_ptr = next1_ptr;
				else if (prefetcher->buffer[next2_ptr].entry[0].status != fetch_incoming &&
					prefetcher->buffer[next2_ptr].entry[1].status != fetch_incoming &&
					prefetcher->buffer[next2_ptr].entry[2].status != fetch_incoming &&
					prefetcher->buffer[next2_ptr].entry[3].status != fetch_incoming)
					prefetcher->demand_ptr = next2_ptr;
				else if (prefetcher->buffer[next3_ptr].entry[0].status != fetch_incoming &&
					prefetcher->buffer[next3_ptr].entry[1].status != fetch_incoming &&
					prefetcher->buffer[next3_ptr].entry[2].status != fetch_incoming &&
					prefetcher->buffer[next3_ptr].entry[3].status != fetch_incoming)
					prefetcher->demand_ptr = next3_ptr;
				else
					debug++;
				prefetcher->buffer[prefetcher->demand_ptr].entry[0].status = fetch_valid;
				prefetcher->buffer[prefetcher->demand_ptr].entry[1].status = fetch_valid;
				prefetcher->buffer[prefetcher->demand_ptr].entry[2].status = fetch_valid;
				prefetcher->buffer[prefetcher->demand_ptr].entry[3].status = fetch_valid;
				prefetcher->buffer[prefetcher->demand_ptr].unique = ((prefetcher->buffer[prefetcher->demand_ptr].unique + 1) & 3);
				prefetcher->buffer[prefetcher->demand_ptr].PC = rd_JALR->data;
//				prefetcher->buffer[prefetcher->demand_ptr].PC = rd->data;

				if (debug_unit_branch) {
					fprintf(debug_stream, "PREFETCHER(%lld): Branch detected in branch unit(forward); buffer: %d, next address: 0x%016I64x, clock: 0x%04llx\n", mhartid, prefetcher->demand_ptr, rd_JALR->data, clock);
				}
		//		prefetcher->shift_buf_valid = 0;
				shifter->valid = 0;
				/**/
			}
			else if (shifter->response.msg != inactive) {
				if (shifter->response.msg == taken || shifter->response.msg == unconditional) {
					if (clock== 0x1e98)
						debug++;
					UINT8 next1_ptr = ((prefetcher->demand_ptr + 1) & 3);
					UINT8 next2_ptr = ((prefetcher->demand_ptr + 2) & 3);
					UINT8 next3_ptr = ((prefetcher->demand_ptr + 3) & 3);
					if (prefetcher->buffer[next1_ptr].entry[0].status != fetch_incoming &&
						prefetcher->buffer[next1_ptr].entry[1].status != fetch_incoming &&
						prefetcher->buffer[next1_ptr].entry[2].status != fetch_incoming &&
						prefetcher->buffer[next1_ptr].entry[3].status != fetch_incoming)
						prefetcher->demand_ptr = next1_ptr;
					else if (prefetcher->buffer[next2_ptr].entry[0].status != fetch_incoming &&
						prefetcher->buffer[next2_ptr].entry[1].status != fetch_incoming &&
						prefetcher->buffer[next2_ptr].entry[2].status != fetch_incoming &&
						prefetcher->buffer[next2_ptr].entry[3].status != fetch_incoming)
						prefetcher->demand_ptr = next2_ptr;
					else if (prefetcher->buffer[next3_ptr].entry[0].status != fetch_incoming &&
						prefetcher->buffer[next3_ptr].entry[1].status != fetch_incoming &&
						prefetcher->buffer[next3_ptr].entry[2].status != fetch_incoming &&
						prefetcher->buffer[next3_ptr].entry[3].status != fetch_incoming)
						prefetcher->demand_ptr = next3_ptr;
					else
						debug++;
					prefetcher->buffer[prefetcher->demand_ptr].entry[0].status = fetch_valid;
					prefetcher->buffer[prefetcher->demand_ptr].entry[1].status = fetch_valid;
					prefetcher->buffer[prefetcher->demand_ptr].entry[2].status = fetch_valid;
					prefetcher->buffer[prefetcher->demand_ptr].entry[3].status = fetch_valid;
					prefetcher->buffer[prefetcher->demand_ptr].unique = ((prefetcher->buffer[prefetcher->demand_ptr].unique + 1) & 3);
					prefetcher->buffer[prefetcher->demand_ptr].PC = shifter->response.addr;
					if (shifter->response.msg == unconditional) {
						for (UINT i = 0; i < 4; i++) {
							if (i != prefetcher->demand_ptr) {
								for (UINT j = 0; j < 4; j++) {
									if (prefetcher->buffer[i].entry[j].status == fetch_valid)
										prefetcher->buffer[i].entry[j].status = fetch_invalid;
								}
							}
						}
					}
					if (debug_unit_branch) {
						if (shifter->response.msg == unconditional)
							fprintf(debug_stream, "PREFETCHER(%lld): Branch detected in decoder(unconditional); buffer: %d, next address: 0x%016I64x, clock: 0x%04llx\n", mhartid, prefetcher->demand_ptr, shifter->response.addr, clock);
						else
							fprintf(debug_stream, "PREFETCHER(%lld): Branch detected in decoder(forward); buffer: %d, next address: 0x%016I64x, clock: 0x%04llx\n", mhartid, prefetcher->demand_ptr, shifter->response.addr, clock);
					}
					shifter->valid = 0;
				}
			}
		}
		else {
			if (shifter->response.msg == unconditional) {
				UINT8 next1_ptr = ((prefetcher->demand_ptr + 1) & 3);
				UINT8 next2_ptr = ((prefetcher->demand_ptr + 2) & 3);
				UINT8 next3_ptr = ((prefetcher->demand_ptr + 3) & 3);
				if (prefetcher->buffer[next1_ptr].entry[0].status != fetch_incoming &&
					prefetcher->buffer[next1_ptr].entry[1].status != fetch_incoming &&
					prefetcher->buffer[next1_ptr].entry[2].status != fetch_incoming &&
					prefetcher->buffer[next1_ptr].entry[3].status != fetch_incoming)
					prefetcher->demand_ptr = next1_ptr;
				else if (prefetcher->buffer[next2_ptr].entry[0].status != fetch_incoming &&
					prefetcher->buffer[next2_ptr].entry[1].status != fetch_incoming &&
					prefetcher->buffer[next2_ptr].entry[2].status != fetch_incoming &&
					prefetcher->buffer[next2_ptr].entry[3].status != fetch_incoming)
					prefetcher->demand_ptr = next2_ptr;
				else if (prefetcher->buffer[next3_ptr].entry[0].status != fetch_incoming &&
					prefetcher->buffer[next3_ptr].entry[1].status != fetch_incoming &&
					prefetcher->buffer[next3_ptr].entry[2].status != fetch_incoming &&
					prefetcher->buffer[next3_ptr].entry[3].status != fetch_incoming)
					prefetcher->demand_ptr = next3_ptr;
				else
					debug++;
				prefetcher->buffer[prefetcher->demand_ptr].entry[0].status = fetch_valid;
				prefetcher->buffer[prefetcher->demand_ptr].entry[1].status = fetch_valid;
				prefetcher->buffer[prefetcher->demand_ptr].entry[2].status = fetch_valid;
				prefetcher->buffer[prefetcher->demand_ptr].entry[3].status = fetch_valid;
				prefetcher->buffer[prefetcher->demand_ptr].unique = ((prefetcher->buffer[prefetcher->demand_ptr].unique + 1) & 3);
				prefetcher->buffer[prefetcher->demand_ptr].PC = shifter->response.addr;
				if (debug_unit_branch) {
					fprintf(debug_stream, "PREFETCHER(%lld): Branch detected in decoder(unconditional); buffer: %d, next address: 0x%016I64x, clock: 0x%04llx\n", mhartid, prefetcher->demand_ptr, shifter->response.addr, clock);
				}
	//			prefetcher->shift_buf_valid = 0;
				shifter->valid = 0;
				prefetcher->halt = 0;
			}
			else if (rd_JALR->strobe) {
				prefetcher->flush = 0;
				prefetcher->halt = 0;
				prefetcher->demand_ptr = ((prefetcher->demand_ptr + 1) & 3);
				prefetcher->buffer[prefetcher->demand_ptr].PC = rd_JALR->data;
				prefetcher->buffer[prefetcher->demand_ptr].entry[0].status = fetch_valid;
				prefetcher->buffer[prefetcher->demand_ptr].entry[1].status = fetch_valid;
				prefetcher->buffer[prefetcher->demand_ptr].entry[2].status = fetch_valid;
				prefetcher->buffer[prefetcher->demand_ptr].entry[3].status = fetch_valid;
				if (debug_unit_branch) {
					fprintf(debug_stream, "PREFETCHER(%lld): unconditional Branch detected in excution; buffer: %d, next address: 0x%016I64x, clock: 0x%04llx\n",
						mhartid, prefetcher->demand_ptr, rd_JALR->data, clock);
				}
			}
		}
		//		if (prefetcher->buffer[prefetcher->demand_ptr].entry[0].status != fetch_fault) {
		if (prefetcher->halt == 0) {
			UINT8 index = (prefetcher->buffer[prefetcher->demand_ptr].PC >> 7) & 3;
			int base_addr = (prefetcher->buffer[prefetcher->demand_ptr].PC >> 2);// 32b blocks, address is 8b
			if ((prefetcher->flush == 0 || shifter->response.msg == inactive) && prefetcher->halt == 0) {

				if (shifter->valid == 1 && shifter->index > 0) {
					if (mhartid == 0)
						debug++;
					if ((((prefetcher->buffer[prefetcher->demand_ptr].PC + (2 * shifter->index)) >> 7) & 3) != index) {
						if (mhartid == 0)
							debug++;
						if ((prefetcher->buffer[prefetcher->demand_ptr].PC - prefetcher->victim.PC > 0x40) || (prefetcher->buffer[prefetcher->demand_ptr].PC - prefetcher->victim.PC < 0)) {
							prefetcher->victim.entry[0].status = prefetcher->victim.entry[1].status = prefetcher->victim.entry[2].status = prefetcher->victim.entry[3].status = fetch_valid;
						}
						prefetcher->victim.PC = prefetcher->buffer[prefetcher->demand_ptr].entry[index].tag;
						for (UINT8 i = 0; i < 0x20; i++)  prefetcher->victim.data[(index << 5) | i] = prefetcher->buffer[prefetcher->demand_ptr].data[(index << 5) | i];
						prefetcher->victim.entry[index].status = fetch_available;
						prefetcher->victim.entry[index].tag = prefetcher->buffer[prefetcher->demand_ptr].entry[index].tag;
						prefetcher->victim.entry[index].xaction_id = prefetcher->buffer[prefetcher->demand_ptr].entry[index].xaction_id;;
						prefetcher->buffer[prefetcher->demand_ptr].entry[index].status = fetch_valid; // invalidate cache line, keep prefetching

					}
					prefetcher->buffer[prefetcher->demand_ptr].PC += (2 * shifter->index);// 4-32b blocks; cache line is 32
					index = (prefetcher->buffer[prefetcher->demand_ptr].PC >> 7) & 3;
					base_addr = (prefetcher->buffer[prefetcher->demand_ptr].PC >> 2);// 32b blocks, address is 8b
				}
			}
			shifter->valid = 0;
			if (prefetcher->buffer[prefetcher->demand_ptr].entry[(index + 1) & 3].tag != (prefetcher->buffer[prefetcher->demand_ptr].entry[index].tag + 0x80)) {
				prefetcher->buffer[prefetcher->demand_ptr].entry[(index + 1) & 3].tag = (prefetcher->buffer[prefetcher->demand_ptr].entry[index].tag + 0x80 && prefetcher->halt == 0);
				prefetcher->buffer[prefetcher->demand_ptr].entry[(index + 1) & 3].status = fetch_valid;
			}
			if (shifter->response.msg == inactive && prefetcher->halt == 0 &&
				(prefetcher->buffer[prefetcher->demand_ptr].entry[0].status != fetch_fault || prefetcher->buffer[prefetcher->demand_ptr].PC != prefetcher->fault_addr)) {
				base_addr = (prefetcher->buffer[prefetcher->demand_ptr].PC >> 2);// 32b blocks, address is 8b
				if (!ROB_stall && ((prefetcher->buffer[prefetcher->demand_ptr].entry[index].status == fetch_available && (base_addr & 0x1f) <= 0x18) ||
					(prefetcher->buffer[prefetcher->demand_ptr].entry[index].status == fetch_available && prefetcher->buffer[prefetcher->demand_ptr].entry[(index + 1) & 3].status == fetch_available))) {// assumes 128b aligned

					if (mhartid == 1)
						if (clock == 0x22ef)
							debug++;
					for (UINT i = 0; i < 8; i++) {
						shifter->buffer[(2 * i) + 0] = prefetcher->buffer[prefetcher->demand_ptr].data[(base_addr + i) & (UINT64)(0x07f)] & 0x0000ffff;
						shifter->buffer[(2 * i) + 1] = (prefetcher->buffer[prefetcher->demand_ptr].data[(base_addr + i) & (UINT64)(0x07f)] >> 16) & 0x0000ffff;
						shifter->tag[(2 * i) + 0] = prefetcher->buffer[prefetcher->demand_ptr].PC + (4 * i);
						shifter->tag[(2 * i) + 1] = prefetcher->buffer[prefetcher->demand_ptr].PC + (4 * i) + 2;
					}
					shifter->valid = 1;
					shifter->index = 0;
					if (debug_unit) {
						fprintf(debug_stream, "PREFETCHER(%lld): data to decoder PC[%d.%d] ", mhartid, prefetcher->demand_ptr, (base_addr >> 5) & 3);
						fprint_addr_coma(debug_stream, prefetcher->buffer[prefetcher->demand_ptr].PC, param);
						for (UINT i = 0; i < 2*param->decode_width; i++) {// optimized for 32b instr
							fprintf(debug_stream, " 0x%02x 0x%04x", i * 4, shifter->buffer[i]);
						}
						fprintf(debug_stream, " clock: 0x%04llx\n", clock);
					}
				}
			}
			UINT8 hit = 0;
			// code bus issue
			if (prefetcher->victim.entry[index].status == fetch_available && prefetcher->buffer[prefetcher->demand_ptr].entry[index].status == fetch_valid) { // issue, only do for demand
				if ((prefetcher->buffer[prefetcher->demand_ptr].PC >> 7) == (prefetcher->victim.entry[index].tag >> 7)) {
					hit = 1;
					if (debug_unit) {
						fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, ", mhartid, prefetcher->victim.entry[index].xaction_id);
						fprintf(debug_stream, "victim cache hit ");
						fprintf(debug_stream, "PC[%d]:  0x%016I64x, tag[%d]:  0x%016I64x, clock: 0x%04llx\n", prefetcher->demand_ptr, prefetcher->buffer[prefetcher->demand_ptr].PC, index, prefetcher->victim.entry[index].tag, clock);
					}
					prefetcher->victim.entry[index].status = fetch_invalid; // steal transaction from issuer to demand fetch
					prefetcher->buffer[prefetcher->demand_ptr].entry[index].status = fetch_available;
					prefetcher->buffer[prefetcher->demand_ptr].entry[index].tag = prefetcher->victim.entry[index].tag;
					for (UINT8 i = 0; i < 0x20; i++)prefetcher->buffer[prefetcher->demand_ptr].data[(index << 5) | i] = prefetcher->victim.data[(index << 5) | i];
				}
			}
			for (UINT8 j = 0; j < 4; j++) {
				if (j == prefetcher->demand_ptr) {
					for (UINT8 index = 0; index < 4; index++) {
						UINT64 tag = (prefetcher->buffer[j].PC & (~0x01ff)) | (index << 7);
						tag = ((prefetcher->buffer[j].PC >> 7) <= (tag >> 7)) ? tag : (tag + 0x0200);
						hit = 0;
						if (prefetcher->buffer[j].entry[index].tag != tag) {

							if (prefetcher->buffer[j].entry[index].status == fetch_incoming && mhartid == 0)
								debug++;
							prefetcher->buffer[j].entry[index].tag = tag;
							prefetcher->buffer[j].entry[index].status = fetch_valid;
						}
						for (UINT8 i = 0; i < 4 && !hit; i++) {
							if ((prefetcher->buffer[i].entry[index].status == fetch_incoming || prefetcher->buffer[i].entry[index].status == fetch_available) && prefetcher->buffer[j].entry[index].status == fetch_valid) { // issue, only do for demand
								if ((tag >> 7) == (prefetcher->buffer[i].entry[index].tag >> 7)) {
									hit = 1;
									if (debug_unit) {
										if (clock == 0x1e94)
											debug++;
										fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, ", mhartid, prefetcher->buffer[i].entry[index].xaction_id);
										if (prefetcher->buffer[i].entry[index].status == fetch_available)	fprintf(debug_stream, "tag hit available ");
										else																fprintf(debug_stream, "tag hit incoming ");
										fprintf(debug_stream, "PC[%d.%d]:  0x%016I64x:  0x%016I64x, clock: 0x%04llx\n", j, index, prefetcher->buffer[j].PC, prefetcher->buffer[i].entry[index].tag, clock);
									}
									prefetcher->buffer[j].entry[index].status = prefetcher->buffer[i].entry[index].status;
									prefetcher->buffer[j].entry[index].tag = prefetcher->buffer[i].entry[index].tag;
									prefetcher->buffer[j].entry[index].xaction_id = prefetcher->buffer[i].entry[index].xaction_id;
									for (UINT8 k = 0; k < 0x20; k++)prefetcher->buffer[j].data[(index << 5) | k] = prefetcher->buffer[i].data[(index << 5) | k];

									prefetcher->buffer[i].entry[index].status = fetch_invalid; // steal transaction from issuer to demand fetch
								}
							}
							if (hit == 1)
								debug++;
						}
						debug++;
					}
				}
				else {
					for (UINT8 index = 0; index < 4; index++) {
						UINT64 tag = (prefetcher->buffer[j].PC & (~0x01ff)) | (index << 7);
						tag = ((prefetcher->buffer[j].PC >> 7) <= (tag >> 7)) ? tag : (tag + 0x0200);
						for (UINT8 i = 0; i < 4 && !hit; i++) {
							if (j != i) {
								if ((prefetcher->buffer[i].entry[index].status == fetch_incoming || prefetcher->buffer[i].entry[index].status == fetch_available) && prefetcher->buffer[j].entry[index].status == fetch_valid) { // issue, only do for demand
									if ((tag >> 7) == (prefetcher->buffer[i].entry[index].tag >> 7)) {
										hit = 1;
										if (debug_unit) {
											fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, ", mhartid, prefetcher->buffer[i].entry[index].xaction_id);
											fprintf(debug_stream, "prefetch tag hit: invalidate bus req; ");
											fprintf(debug_stream, "PC(%d.%d): 0x%016I64x, 0x%016I64x, clock: 0x%04llx\n", j, index, prefetcher->buffer[j].PC, prefetcher->buffer[i].entry[index].tag, clock);
										}
										prefetcher->buffer[j].entry[index].status = fetch_invalid;
									}
								}
							}
						}
					}
				}
			}
			index = (prefetcher->buffer[prefetcher->demand_ptr].PC >> 7) & 3;

			for (UINT8 j = prefetcher->demand_ptr, count = 0; count < 4; count++, j = ((j + 1) & 3)) {
				switch (prefetcher->buffer[j].sm) {
				case 0:// idle
					if (TLB_response->snoop_response == snoop_idle && !hit && bus2_data_in->snoop_response != snoop_stall && active_IO == 0) {
						for (UINT8 index = (prefetcher->buffer[j].PC >> 7) & 3, i = 0; i < 4 && logical_addr->strobe == 0 && !hit; i++, index = ((index + 1) & 3)) {// 8b aligned
							UINT addr = prefetcher->buffer[j].entry[index].tag = ((prefetcher->buffer[j].PC + (i * 0x80)) & 0xffffffffffffff80);
							if (prefetcher->buffer[j].entry[index].status == fetch_valid && ((j == prefetcher->demand_ptr) || (addr > 0x01000))) {

								UINT id = (index << 12) | (prefetcher->buffer[j].unique << 10) | (j << 8) | (1 << 6) | mhartid;
					//			if ((csr[csr_mcause].value != 0) && (((csr[csr_mstatus].value & 0x08) == 0x08) || ((csr[csr_mstatus].value & 0x02) == 0x02))  && shifter->response.fault_in_service == 0) {
								if (interrupt) {
									// do not initiate fetch/prefetch if interrupt present
								}
								else if (!prefetcher->id_in_use[(id >> 6) & 0x00ff]) {
									logical_addr->strobe = 1;
									logical_addr->addr = addr;// error  - need to roll over on page boundary - don't prefetch new pages
									logical_addr->xaction_id = (index << 12) | (prefetcher->buffer[j].unique << 10) | (j << 8) | (1 << 6) | mhartid;

									prefetcher->id_in_use[logical_addr->xaction_id >> 6] = 1;

									prefetcher->buffer[j].entry[index].xaction_id = (index << 12) | (prefetcher->buffer[j].unique << 10) | (j << 8) | (1 << 6) | mhartid;

									if (addr < 0x01000)
										debug++;
									if (debug_unit) {
										fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, ", mhartid, logical_addr->xaction_id);
										if (j == prefetcher->demand_ptr) 			fprintf(debug_stream, "bus_fetch ");
										else 										fprintf(debug_stream, "bus_prefetch ");
										fprintf(debug_stream, "addr ");
										fprint_addr_coma(debug_stream, logical_addr->addr, param);
										fprintf(debug_stream, " PC(%d.%d): ", j, index);
										fprint_addr_coma(debug_stream, prefetcher->buffer[j].PC, param);
										fprintf(debug_stream, " clock: 0x%04llx\n", clock);
									}
									if (j == prefetcher->demand_ptr) {
										logical_addr->xaction = bus_fetch;
									}
									else {
										logical_addr->xaction = bus_prefetch;
									}
									prefetcher->buffer[j].entry[index].status = fetch_incoming;
								}
								else {
									debug++;
								}
							}
						}
					}
					break;
				case 1:
					if (TLB_response->snoop_response == snoop_stall) {
					}
					else if (TLB_response->snoop_response == snoop_miss && ((TLB_response->xaction_id & 0x0f) == mhartid)) {
						prefetcher->buffer[j].data_count = 0;
						prefetcher->buffer[j].sm = 0;
						prefetcher->id_in_use[TLB_response->xaction_id >> 6] = 0;
						if (prefetcher->demand_ptr == j && on_branch && TLB_response->xaction_id == prefetcher->buffer[j].entry[TLB_response->xaction_id >> 12].xaction_id) {// on branch or jump, no flag for direct jump though (needed??)
							prefetcher->buffer[j].entry[0].status = fetch_fault;// invalidate address on page miss
							prefetcher->buffer[j].entry[1].status = fetch_fault;// invalidate address on page miss
							prefetcher->buffer[j].entry[2].status = fetch_fault;// invalidate address on page miss
							prefetcher->buffer[j].entry[3].status = fetch_fault;// invalidate address on page miss
							if (debug_unit_walk) {
								fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, Instr Page Fault latched; addr: 0x%016I64x, PC: 0x%016I64x,clock: 0x%04llx\n",
									mhartid, TLB_response->xaction_id, prefetcher->buffer[j].entry[TLB_response->xaction_id >> 12].tag, prefetcher->buffer[j].PC, clock);
							}
							prefetcher->fault_addr = (prefetcher->buffer[j].PC > prefetcher->buffer[j].entry[TLB_response->xaction_id >> 12].tag) ?
								prefetcher->buffer[j].PC : prefetcher->buffer[j].entry[TLB_response->xaction_id >> 12].tag;
						}
						else {
							if (debug_unit_walk) {
								if (prefetcher->buffer[j].entry[0].status != fetch_invalid || prefetcher->buffer[j].entry[1].status != fetch_invalid || prefetcher->buffer[j].entry[2].status != fetch_invalid || prefetcher->buffer[j].entry[3].status != fetch_invalid) {
									fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, Prefetch to invalid space halted; addr: 0x%016I64x, clock: 0x%04llx\n",
										mhartid, TLB_response->xaction_id, prefetcher->buffer[j].entry[TLB_response->xaction_id >> 12].tag, clock);
								}
							}
							prefetcher->buffer[j].entry[0].status = fetch_invalid;// invalidate address on page miss
							prefetcher->buffer[j].entry[1].status = fetch_invalid;// invalidate address on page miss
							prefetcher->buffer[j].entry[2].status = fetch_invalid;// invalidate address on page miss
							prefetcher->buffer[j].entry[3].status = fetch_invalid;// invalidate address on page miss
						}
					}
					else if (TLB_response->snoop_response == snoop_hit) {
						prefetcher->buffer[j].sm = 0;
					}
					else {
						debug++;
					}
					break;
				default:
					debug++;
					break;
				}
			}
		}
		else {
			if (TLB_response->snoop_response == snoop_miss && ((TLB_response->xaction_id & 0x0f) == mhartid)) {
				prefetcher->id_in_use[TLB_response->xaction_id >> 6] = 0;
			}
		}
		if (mhartid == 1) {// 0x0000000086531d80
			if (clock == 0x3010) // valid
				debug++;
			if (clock == 0x32b3)// valid
				debug++;
			if (clock == 0x3baa)// 
				debug++;
		}
		if (TLB_response->snoop_response != snoop_idle) {
			if ((TLB_response->xaction_id & 0x0f) != mhartid)
				debug++;
			if (TLB_response->snoop_response == snoop_miss && ((TLB_response->xaction_id & 0x0f) == mhartid)) {
				for (UINT8 j = 0; j < 4; j++) {
					prefetcher->buffer[j].data_count = 0;
					prefetcher->buffer[j].sm = 0;
					prefetcher->id_in_use[TLB_response->xaction_id >> 6] = 0;
					if (prefetcher->demand_ptr == j && TLB_response->xaction_id == prefetcher->buffer[j].entry[TLB_response->xaction_id >> 12].xaction_id) {// on branch or jump, no flag for direct jump though (needed??)
						prefetcher->buffer[j].entry[0].status = fetch_fault;// invalidate address on page miss
						prefetcher->buffer[j].entry[1].status = fetch_fault;// invalidate address on page miss
						prefetcher->buffer[j].entry[2].status = fetch_fault;// invalidate address on page miss
						prefetcher->buffer[j].entry[3].status = fetch_fault;// invalidate address on page miss
						if (debug_unit_walk) {
							fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, Instr Page Fault latched; addr: 0x%016I64x, PC: 0x%016I64x,clock: 0x%04llx\n",
								mhartid, TLB_response->xaction_id, prefetcher->buffer[j].entry[TLB_response->xaction_id >> 12].tag, prefetcher->buffer[j].PC, clock);
						}
						prefetcher->fault_addr = (prefetcher->buffer[j].PC > prefetcher->buffer[j].entry[TLB_response->xaction_id >> 12].tag) ?
							prefetcher->buffer[j].PC : prefetcher->buffer[j].entry[TLB_response->xaction_id >> 12].tag;
					}
					else {
						if (debug_unit_walk) {
							if (prefetcher->buffer[j].entry[0].status != fetch_invalid || prefetcher->buffer[j].entry[1].status != fetch_invalid || prefetcher->buffer[j].entry[2].status != fetch_invalid || prefetcher->buffer[j].entry[3].status != fetch_invalid) {
								fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, Prefetch to page not in TLB, halt prefetches; addr: 0x%016I64x, clock: 0x%04llx\n",
									mhartid, TLB_response->xaction_id, prefetcher->buffer[j].entry[TLB_response->xaction_id >> 12].tag, clock);
							}
						}
						prefetcher->buffer[j].entry[0].status = fetch_invalid;// invalidate address on page miss
						prefetcher->buffer[j].entry[1].status = fetch_invalid;// invalidate address on page miss
						prefetcher->buffer[j].entry[2].status = fetch_invalid;// invalidate address on page miss
						prefetcher->buffer[j].entry[3].status = fetch_invalid;// invalidate address on page miss
					}
				}
			}
		}
		// code bus latch
		if (physical_data->snoop_response == snoop_hit && ((physical_data->xaction_id & 0x0f) == mhartid)) {
			UINT8 buffer_ptr = (physical_data->xaction_id >> 8) & 3;
			UINT8 line = (physical_data->xaction_id >> 12) & 3;
			UINT8 hit = 0;
			prefetcher->id_in_use[physical_data->xaction_id >> 6] = 0;
			if (prefetcher->demand_ptr == 0xff) {
				L0_latch_aux_buffer(prefetcher, physical_data, clock, mhartid, debug_unit, param, debug_stream);
			}
			else if (prefetcher->buffer[prefetcher->demand_ptr].entry[line].xaction_id == physical_data->xaction_id && prefetcher->buffer[prefetcher->demand_ptr].entry[line].status == fetch_incoming) {
				for (UINT8 i = 0; i < 0x10; i++) {
					prefetcher->buffer[prefetcher->demand_ptr].data[(line << 5) | (2 * i)] = physical_data->data[i];
					prefetcher->buffer[prefetcher->demand_ptr].data[(line << 5) | (2 * i) | 1] = physical_data->data[i] >> 32; // 64b ->32b
				}
				prefetcher->buffer[prefetcher->demand_ptr].data_count += 0x10;
				if (debug_unit) {
					fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, tag: ", mhartid, physical_data->xaction_id);
					fprint_addr_coma(debug_stream, prefetcher->buffer[buffer_ptr].entry[line].tag, param);
					fprintf(debug_stream, " demand bus latch PC(%d.%d): ", prefetcher->demand_ptr, line);
					fprint_addr_coma(debug_stream, prefetcher->buffer[prefetcher->demand_ptr].PC, param);
					fprintf(debug_stream, "  clock: 0x%04llx\n", clock);
					fprint_prefetch_latch_data(debug_stream, physical_data->data, physical_data->xaction_id, prefetcher->buffer[buffer_ptr].entry[line].tag, clock, mhartid,param);
				}
				for (UINT8 i = 0; i < 4; i++)
					if (prefetcher->buffer[i].entry[line].xaction_id == physical_data->xaction_id &&
						i != prefetcher->demand_ptr &&
						prefetcher->buffer[i].entry[line].status != fetch_invalid) {
						if (prefetcher->buffer[i].entry[line].status != fetch_available)
							debug++;
						prefetcher->buffer[i].entry[line].status = fetch_invalid;
					}
				prefetcher->buffer[prefetcher->demand_ptr].entry[line].status = fetch_available;
				prefetcher->buffer[prefetcher->demand_ptr].entry[line].time = clock;
			}
			else if (prefetcher->buffer[0].entry[line].xaction_id == physical_data->xaction_id && prefetcher->buffer[0].entry[line].status == fetch_incoming) {
				for (UINT8 i = 0; i < 0x10; i++) {
					prefetcher->buffer[0].data[(line << 5) | (2 * i)] = physical_data->data[i];
					prefetcher->buffer[0].data[(line << 5) | (2 * i) | 1] = physical_data->data[i] >> 32; // 64b ->32b
				}
				prefetcher->buffer[0].data_count += 0x10;
				if (debug_unit) {
					fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, tag: ", mhartid, physical_data->xaction_id);
					fprint_addr_coma(debug_stream, prefetcher->buffer[buffer_ptr].entry[line].tag, param);
					fprintf(debug_stream, " aux bus latch PC(%d.%d): ", 0, line);
					fprint_addr_coma(debug_stream, prefetcher->buffer[0].PC, param);
					fprintf(debug_stream, "  clock: 0x%04llx\n", clock);
					fprint_prefetch_latch_data(debug_stream, physical_data->data, physical_data->xaction_id, prefetcher->buffer[buffer_ptr].entry[line].tag, clock, mhartid, param);
				}
				for (UINT8 i = 0; i < 4; i++)
					if (prefetcher->buffer[i].entry[line].xaction_id == physical_data->xaction_id &&
						i != 0 &&
						prefetcher->buffer[i].entry[line].status != fetch_invalid) {
						if (prefetcher->buffer[i].entry[line].status != fetch_available)
							debug++;
						prefetcher->buffer[i].entry[line].status = fetch_invalid;
					}
				prefetcher->buffer[0].entry[line].status = fetch_available;
				prefetcher->buffer[0].entry[line].time = clock;
			}
			else if (prefetcher->buffer[1].entry[line].xaction_id == physical_data->xaction_id && prefetcher->buffer[1].entry[line].status == fetch_incoming) {
				for (UINT8 i = 0; i < 0x10; i++) {
					prefetcher->buffer[1].data[(line << 5) | (2 * i)] = physical_data->data[i];
					prefetcher->buffer[1].data[(line << 5) | (2 * i) | 1] = physical_data->data[i] >> 32; // 64b ->32b
				}
				prefetcher->buffer[1].data_count += 0x10;
				if (debug_unit) {
					fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, tag: ", mhartid, physical_data->xaction_id);
					fprint_addr_coma(debug_stream, prefetcher->buffer[buffer_ptr].entry[line].tag, param);
					fprintf(debug_stream, " aux bus latch PC(%d.%d): ", 1, line);
					fprint_addr_coma(debug_stream, prefetcher->buffer[1].PC, param);
					fprintf(debug_stream, "  clock: 0x%04llx\n", clock);
					fprint_prefetch_latch_data(debug_stream, physical_data->data, physical_data->xaction_id, prefetcher->buffer[buffer_ptr].entry[line].tag, clock, mhartid, param);
				}
				for (UINT8 i = 0; i < 4; i++)
					if (prefetcher->buffer[i].entry[line].xaction_id == physical_data->xaction_id &&
						i != 1 &&
						prefetcher->buffer[i].entry[line].status != fetch_invalid) {
						if (prefetcher->buffer[i].entry[line].status != fetch_available)
							debug++;
						prefetcher->buffer[i].entry[line].status = fetch_invalid;
					}
				prefetcher->buffer[1].entry[line].status = fetch_available;
				prefetcher->buffer[1].entry[line].time = clock;
			}
			else if (prefetcher->buffer[2].entry[line].xaction_id == physical_data->xaction_id && prefetcher->buffer[2].entry[line].status == fetch_incoming) {
				for (UINT8 i = 0; i < 0x10; i++) {
					prefetcher->buffer[2].data[(line << 5) | (2 * i)] = physical_data->data[i];
					prefetcher->buffer[2].data[(line << 5) | (2 * i) | 1] = physical_data->data[i] >> 32; // 64b ->32b
				}
				prefetcher->buffer[2].data_count += 0x10;
				if (debug_unit) {
					fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, tag: ", mhartid, physical_data->xaction_id);
					fprint_addr_coma(debug_stream, prefetcher->buffer[buffer_ptr].entry[line].tag, param);
					fprintf(debug_stream, " aux bus latch PC(%d.%d): ", 2, line);
					fprint_addr_coma(debug_stream, prefetcher->buffer[2].PC, param);
					fprintf(debug_stream, "  clock: 0x%04llx\n", clock);
					fprint_prefetch_latch_data(debug_stream, physical_data->data, physical_data->xaction_id, prefetcher->buffer[buffer_ptr].entry[line].tag, clock, mhartid, param);
				}
				for (UINT8 i = 0; i < 4; i++)
					if (prefetcher->buffer[i].entry[line].xaction_id == physical_data->xaction_id &&
						i != 2 &&
						prefetcher->buffer[i].entry[line].status != fetch_invalid) {
						if (prefetcher->buffer[i].entry[line].status != fetch_available)
							debug++;
						prefetcher->buffer[i].entry[line].status = fetch_invalid;
					}
				prefetcher->buffer[2].entry[line].status = fetch_available;
				prefetcher->buffer[2].entry[line].time = clock;
			}
			else if (prefetcher->buffer[3].entry[line].xaction_id == physical_data->xaction_id && prefetcher->buffer[3].entry[line].status == fetch_incoming) {
				for (UINT8 i = 0; i < 0x10; i++) {
					prefetcher->buffer[3].data[(line << 5) | (2 * i)] = physical_data->data[i];
					prefetcher->buffer[3].data[(line << 5) | (2 * i) | 1] = physical_data->data[i] >> 32; // 64b ->32b
				}
				prefetcher->buffer[3].data_count += 0x10;
				if (debug_unit) {
					fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, tag: ", mhartid, physical_data->xaction_id);
					fprint_addr_coma(debug_stream, prefetcher->buffer[buffer_ptr].entry[line].tag, param);
					fprintf(debug_stream, " aux bus latch PC(%d.%d): ", 3, line);
					fprint_addr_coma(debug_stream, prefetcher->buffer[3].PC, param);
					fprintf(debug_stream, "  clock: 0x%04llx\n", clock);
					fprint_prefetch_latch_data(debug_stream, physical_data->data, physical_data->xaction_id, prefetcher->buffer[buffer_ptr].entry[line].tag, clock, mhartid, param);
				}
				for (UINT8 i = 0; i < 4; i++)
					if (prefetcher->buffer[i].entry[line].xaction_id == physical_data->xaction_id &&
						i != 3 &&
						prefetcher->buffer[i].entry[line].status != fetch_invalid) {
						if (prefetcher->buffer[i].entry[line].status != fetch_available)
							debug++;
						prefetcher->buffer[i].entry[line].status = fetch_invalid;
					}
				prefetcher->buffer[3].entry[line].status = fetch_available;
				prefetcher->buffer[3].entry[line].time = clock;
				}
			else if ((prefetcher->buffer[buffer_ptr].unique != ((physical_data->xaction_id >> 10) & 3)) || prefetcher->buffer[buffer_ptr].entry[line].status == 0 ||
				(prefetcher->buffer[buffer_ptr].entry[line].tag >> 7) < (prefetcher->buffer[buffer_ptr].PC >> 7)) {
//				if (debug_unit) {
					fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, ERROR: Dropped Bus return data; current core id: %#06x, clock: 0x%04llx\n",
						mhartid, physical_data->xaction_id, (prefetcher->buffer[buffer_ptr].unique << 10) | (line << 12) | (buffer_ptr << 8) | (0 << 7) | mhartid, clock);
//				}
			}
			else {
				L0_latch_aux_buffer(prefetcher, physical_data, clock, mhartid, debug_unit, param, debug_stream);
			}
		}
		if ((bus2_data_in->snoop_response == snoop_hit || bus2_data_in->snoop_response == snoop_dirty) && ((bus2_data_in->xaction_id & 0x0f) == mhartid)) {
			if (mhartid == 1)
				if (clock == 0x22ce)
					debug++;
			UINT8 buffer_ptr = (bus2_data_in->xaction_id >> 8) & 3;
			UINT8 line = (bus2_data_in->xaction_id >> 12) & 3;
			UINT8 hit = 0;
			if (bus2_data_in->xaction_id >> 6 < 0x100)
				prefetcher->id_in_use[bus2_data_in->xaction_id >> 6] = 0;
			if (prefetcher->demand_ptr == 0xff) {
				L2_latch_aux_buffer(prefetcher, bus2_data_in, clock, mhartid, debug_unit, param, debug_stream);
			}
			else if (prefetcher->buffer[prefetcher->demand_ptr].entry[line].xaction_id == bus2_data_in->xaction_id && prefetcher->buffer[prefetcher->demand_ptr].entry[line].status == fetch_incoming) {
				for (UINT8 i = 0; i < 0x10; i++) {
					prefetcher->buffer[prefetcher->demand_ptr].data[(line << 5) | (2 * i)] = bus2_data_in->data[i];
					prefetcher->buffer[prefetcher->demand_ptr].data[(line << 5) | (2 * i) | 1] = bus2_data_in->data[i] >> 32;
				}
				prefetcher->buffer[prefetcher->demand_ptr].data_count += 0x10;
				if (debug_unit) {
			//		UINT64* data = bus2_data_in->data;
					fprintf(debug_stream, "PREFETCHER(%lld): xaction id %#06x, tag:", mhartid, bus2_data_in->xaction_id);
					fprint_addr_coma(debug_stream, prefetcher->buffer[buffer_ptr].entry[line].tag, param);
					fprintf(debug_stream, " bus latch demand PC(%d.%d): ", prefetcher->demand_ptr, line);
					fprint_addr_coma(debug_stream, prefetcher->buffer[prefetcher->demand_ptr].PC, param);
					fprintf(debug_stream, " clock: 0x%04llx\n", clock);

					fprint_prefetch_latch_data(debug_stream, bus2_data_in->data, bus2_data_in->xaction_id, prefetcher->buffer[buffer_ptr].entry[line].tag, clock, mhartid,param);
				}
				for (UINT8 i = 0; i < 4; i++)
					if (prefetcher->buffer[i].entry[line].xaction_id == bus2_data_in->xaction_id &&
						i != prefetcher->demand_ptr &&
						prefetcher->buffer[i].entry[line].status != fetch_invalid)
						prefetcher->buffer[i].entry[line].status = fetch_invalid;
				prefetcher->buffer[prefetcher->demand_ptr].entry[line].status = fetch_available;
				prefetcher->buffer[prefetcher->demand_ptr].entry[line].time = clock;
			}
			else {
				L2_latch_aux_buffer(prefetcher, bus2_data_in, clock, mhartid, debug_unit, param, debug_stream);
			}
		}
	}
	//	}
	prefetcher->reset_latch = reset;
}