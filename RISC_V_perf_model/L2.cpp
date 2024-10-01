// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "cache.h"

void uL2_internal_variable_cleanup(L2_cache_type* cache_var, data_bus_type* bus2_data_read, UINT64 clock) {
	int debug = 0;
	if (bus2_data_read->snoop_response != snoop_idle) {
		UINT hit = 0;

		UINT8 current = ((bus2_data_read->xaction_id >> 12) & (cache_var->write_count - 1));
		switch ((bus2_data_read->xaction_id >> 8) & 3) {
		case 1:
			if (cache_var->addr_load_list[current].latch.xaction_id == bus2_data_read->xaction_id && cache_var->addr_load_list[current].latch.xaction != bus_SC_aq_rl) {
				if (cache_var->addr_load_list[current].status != link_retire)
					cache_var->addr_load_list[current].status = link_retire;
			}
			break;
		case 2:
			if (cache_var->addr_alloc_list[current].latch.xaction_id == bus2_data_read->xaction_id && cache_var->addr_alloc_list[current].latch.xaction != bus_SC_aq_rl) {
				if (cache_var->addr_alloc_list[current].status != link_retire)
					cache_var->addr_alloc_list[current].status = link_retire;
			}
			break;
		case 3:
			for (UINT8 current = 0; current < cache_var->write_count; current++) {
				if (cache_var->addr_write_list[current].latch.xaction_id == bus2_data_read->xaction_id && cache_var->addr_write_list[current].latch.xaction != bus_SC_aq_rl) {
					if (cache_var->addr_write_list[current].status != link_retire)
						cache_var->addr_write_list[current].status = link_retire;
				}
			}
			break;
		default:
			debug++;
			break;
		}
	}
	for (UINT8 current = 0; current < cache_var->write_count; current++) {
		if (cache_var->addr_load_list[current].status == link_retire)
			cache_var->addr_load_list[current].status = link_free;
		if (cache_var->addr_alloc_list[current].status == link_retire)
			cache_var->addr_alloc_list[current].status = link_free;
		if (cache_var->addr_write_list[current].status == link_retire) {
			cache_var->addr_write_list[current].status = link_free;
			cache_var->addr_write_list[current].latch.clock = clock;
		}
	}
	for (UINT8 current = 0; current < 0x40; current++) {
		if (cache_var->addr_fetch_list[current].status == link_retire)
			cache_var->addr_fetch_list[current].status = link_free;
	}
}

UINT8 clock_bank_array_path(L2_block_type* bank, UINT64 clock, char* header, UINT8 debug_unit, FILE* debug_stream) {
	int debug = 0;
	//cache array update
	UINT8 data_ready = 0;
	for (UINT8 i = 0; i < 7; i++) {//read data advance
		if (bank->array.data[i + 1].snoop_response != snoop_idle) {
			if ((bank->array.select[i + 1] & 0x0800) == 0) { // read cycle
				copy_data_bus_info(&bank->array.data[i], &bank->array.data[i + 1]);
				bank->array.data[i + 1].snoop_response = snoop_idle;
			}
		}
	}
	for (INT8 i = 6; i >= 0; i--) {
		if (bank->array.select[i] & 0x0800) { // artifact, detect write data. write-read transitionn with 0 clocks
			copy_data_bus_info(&bank->array.data[i + 1], &bank->array.data[i]);
			bank->array.data[i].snoop_response = snoop_idle;
		}
		bank->array.select[i + 1] = bank->array.select[i];
		bank->array.xaction_id_in[i + 1] = bank->array.xaction_id_in[i];
		bank->array.snoop_response[i + 1] = bank->array.snoop_response[i];
	}
	bank->array.select[0] = 0;
	bank->array.xaction_id_in[0] = 0;
	bank->array.snoop_response[0] = snoop_idle;
	bank->array.data[7].xaction_id = bank->array.xaction_id_in[7];
	if (bank->array.select[1] & 0x0800) {
		UINT set = bank->array.select[1] & 0x07ff;
		UINT way = (bank->array.select[7] >> 12) & 0x007;
		bank->tags[set].in_use = 0;
		if (debug_unit) {
			UINT8 hit = 0;
			for (UINT8 j = 0; j < 7 && !hit; j++)
				if (bank->array.select[j] != 0) hit = 1;
			fprintf(debug_stream, "%s set.way 0x%03x.%d, xaction id 0x%04x set lock released %s; clock: 0x%04llx \n",
				header, set, way, bank->array.xaction_id_in[7], (hit) ? "busy" : "free", clock);
		}
	}
	if (bank->array.select[7] & 0x8000) {// write data array update
		UINT set = bank->array.select[7] & 0x07ff;
		UINT way = (bank->array.select[7] >> 12) & 0x007;
		if (bank->array.select[7] & 0x0800) {
			for (UINT8 j = 0; j < 0x10; j++) {
				bank->array.line[way][set][j] = bank->array.data[7].data[j];
			}
			if (debug_unit) {
				UINT64* data = bank->array.line[way][set];
				UINT8 hit = 0;
				for (UINT8 j = 0; j < 7 && !hit; j++)
					if (bank->array.select[j] != 0) hit = 1;
				fprintf(debug_stream, "%s set.way 0x%03x.%d, xaction id 0x%04x array updated, write queue %s;data(0)0x%016I64x (1)0x%016I64x (2)0x%016I64x (3)0x%016I64x (4)0x%016I64x (5)0x%016I64x (6)0x%016I64x (7)0x%016I64x (8)0x%016I64x \n\t\t(9)0x%016I64x (a)0x%016I64x (b)0x%016I64x (c)0x%016I64x (d)0x%016I64x (e)0x%016I64x (f)0x%016I64x, clock: 0x%04llx \n",
					header, set, way, bank->array.xaction_id_in[7], (hit) ? "busy" : "free", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8],
					data[9], data[10], data[11], data[12], data[13], data[14], data[15], clock);
			}
		}
		else {
			bank->array.data[7].snoop_response = bank->array.snoop_response[7];
			for (UINT8 j = 0; j < 0x10; j++) {
				bank->array.data[7].data[j] = bank->array.line[way][set][j];
			}
		}
	}
	else {
		bank->array.data[7].snoop_response = snoop_idle;
	}
	if (bank->array.data[0].snoop_response) {// read data available for consumption
		data_ready = 1;
		if (debug_unit)
			fprintf(debug_stream, "%s xaction id 0x%04x, cache array data available, read cycle; clock: 0x%04llx \n", header, bank->array.data[0].xaction_id, clock);
	}
	return data_ready;
}
snoop_response_type cache_lookup(L2_block_type* L2, INT64 addr, UINT64 clock, char* header, UINT16 xaction_id, bus_xaction_type xaction, UINT debug_unit, FILE* debug_stream) {
	int debug = 0;
	snoop_response_type snoop_response = snoop_miss;
	UINT set = (addr >> L2->set_shift) & 0x000007ff;
	INT64 tag = addr >> L2->tag_shift;
	for (UINT8 way = 0; way < 8 && snoop_response == snoop_miss; way++) {
		if (L2->tags[set].tag[way] == tag && L2->tags[set].state[way] != invalid_line) {
			L2->tags[set].way_ptr = way;
			switch (L2->tags[set].state[way]) {
			case shared_line:		 snoop_response = (xaction == bus_allocate) ? snoop_miss : snoop_hit;		break;
			case modified_line:		snoop_response = snoop_dirty;		break;
			case exclusive_line:	snoop_response = snoop_hit;		break;
			default:
				debug++;
				break;
			}
		}
	}

	return snoop_response;
}
snoop_response_type cache_lookup_update(L2_block_type* L2, INT64 addr, UINT64 clock, char* header, UINT16 xaction_id, bus_xaction_type xaction, UINT debug_unit, param_type* param, FILE* debug_stream) {
	int debug = 0;
	snoop_response_type snoop_response = snoop_miss;
	UINT set = (addr >> L2->set_shift) & 0x000007ff;
	INT64 tag = addr >> L2->tag_shift;
	for (UINT8 way = 0; way < 8 && snoop_response == snoop_miss; way++) {
		if (L2->tags[set].tag[way] == tag && L2->tags[set].state[way] != invalid_line) {
			L2->tags[set].way_ptr = way;
			switch (L2->tags[set].state[way]) {
			case shared_line:		 snoop_response = (xaction == bus_allocate) ? snoop_miss : snoop_hit;		break;
			case modified_line:		snoop_response = snoop_dirty;		break;
			case exclusive_line:	snoop_response = snoop_hit;		break;
			default:
				debug++;
				break;
			}
			if (snoop_response != snoop_miss) {
				if (L2->array.read_write) {// write mode
					UINT hit = 0;
					if (L2->array.select[0] != 0)
						hit = 1;
					if (L2->array.select[1] != 0)
						hit = 1;
					if (L2->array.data[0].snoop_response != snoop_idle)// active clock
						hit = 1;
					if (L2->array.data[1].snoop_response != snoop_idle)// buffer turn around clock
						hit = 1;
					if (hit) {
						snoop_response = snoop_stall;
					}
					else {
						L2->array.read_write = 0;
						L2->tags[set].way_ptr = way;
						L2->array.select[0] = 0x8000 | (way << 12) | set; // initiate data fetch
						L2->array.xaction_id_in[0] = xaction_id;
						L2->array.snoop_response[0] = snoop_response;
					}
				}
				else {// read mode
					if (L2->array.select[0] != 0)
						snoop_response = snoop_stall;
					else {
						L2->tags[set].way_ptr = way;
						L2->array.select[0] = 0x8000 | (way << 12) | set; // initiate data fetch
						L2->array.xaction_id_in[0] = xaction_id;
						L2->array.snoop_response[0] = snoop_response;
					}
				}
				if (debug_unit) {
					fprintf(debug_stream, "%s set.way 0x%03x.%d ", header, set, L2->tags[set].way_ptr);
					switch (snoop_response) {
					case snoop_hit:
						fprintf(debug_stream, "S");			break;
					case snoop_dirty:
						fprintf(debug_stream, "M");			break;
					case snoop_miss:
						switch (L2->tags[set].state[L2->tags[set].way_ptr]) {
						case shared_line:		fprintf(debug_stream, "S");		break;
						case modified_line:		fprintf(debug_stream, "M");		break;
						case exclusive_line:	fprintf(debug_stream, "E");		break;
						default:				fprintf(debug_stream, "I");		break;
						}
						break;
					case snoop_stall:		fprintf(debug_stream, "stall");		break;
					default:
						debug++;
						break;
					}
					fprintf(debug_stream, " addr: 0x%016I64x xaction id 0x%04x, tag: 0x%08x, Cache read start clock: 0x%04llx\n", addr, xaction_id, tag, clock);
				}
			}
		}
	}
	if (snoop_response == snoop_miss && debug_unit) {
		fprintf(debug_stream, "%s set.way 0x%03x.%d ", header, set, L2->tags[set].way_ptr);
		switch (L2->tags[set].state[L2->tags[set].way_ptr]) {
		case shared_line:		fprintf(debug_stream, "S");		break;
		case modified_line:		fprintf(debug_stream, "M");		break;
		case exclusive_line:	fprintf(debug_stream, "E");		break;
		default:				fprintf(debug_stream, "I");		break;
		}
		fprintf(debug_stream, " addr ");
		fprint_addr_coma(debug_stream, addr, param);
		fprintf(debug_stream, "xaction id 0x%04x, tag: 0x%08x, Cache Miss ", xaction_id, tag);
		switch (xaction) {
		case bus_allocate:	fprintf(debug_stream, "allocate");		break;
		case bus_fetch:	fprintf(debug_stream, "fetch");		break;
		case bus_load:	fprintf(debug_stream, "load");		break;
		default:break;
		}
		fprintf(debug_stream, " clock: 0x%04llx\n", clock);
	}

	return snoop_response;
}
void add_write(L2_block_type* L2, UINT64 addr, UINT8 alloc, data_bus_type* data_in, UINT64 clock, UINT mhartid, UINT debug_unit, char* header, FILE* debug_stream) {
	int debug = 0;
	int set = (addr >> L2->set_shift) & 0x00007ff;
	L2->tags[set].way_ptr = ((L2->tags[set].way_ptr + 1) & 0x07);// incr way ptr to new, hopefully unused way
	UINT8 hit = 0;
	for (UINT way = 0; way < 8; way++) {
		if (L2->tags[set].tag[way] == (addr >> L2->tag_shift)) { // check for address matches
			L2->tags[set].way_ptr = way;
			hit = 1;
		}
	}
	if (!hit) {
		for (UINT way = 0; way < 8 && !hit; way++) {
			if (L2->tags[set].state[way] == invalid_line) {
				L2->tags[set].way_ptr = way;
				hit = 1;
			}
		}
		hit = 0;
	}
	UINT8 way = L2->tags[set].way_ptr;
	if (clock == 0x2074)
		debug++;
	if (L2->array.select[0] != 0)
		fprintf(debug_stream, "%s xaction id 0x%04x, ERROR: address overwrite (old id 0x%04x)clock: 0x%04llx\n", header, data_in->xaction_id, L2->array.xaction_id_in[0], clock);
	else {
		L2->tags[set].in_use = 0;
		if (debug_unit) {
			fprintf(debug_stream, "%s set.way 0x%03x.%01x %s->%s addr 0x%016I64x xaction id %#06x, writing data to bank: clock: 0x%04llx\n",
				header, set, way,
				(L2->tags[set].state[way] == modified_line) ? "M" : (L2->tags[set].state[way] == invalid_line) ? "I" : (L2->tags[set].state[way] == exclusive_line) ? "E" : "S",
				(data_in->snoop_response == snoop_dirty) ? "M" : (alloc) ? "E" : "S", addr, data_in->xaction_id, clock);
			fprintf(debug_stream, "%s set.way 0x%03x.%01x xaction id %#06x, 0x00 0x%016I64x 0x08 0x%016I64x 0x10 0x%016I64x 0x18 0x%016I64x clock: 0x%04llx\n",
				header, set, way, data_in->xaction_id, data_in->data[0], data_in->data[1], data_in->data[2], data_in->data[3], clock);
			fprintf(debug_stream, "%s set.way 0x%03x.%01x xaction id %#06x, 0x20 0x%016I64x 0x28 0x%016I64x 0x30 0x%016I64x 0x38 0x%016I64x clock: 0x%04llx\n",
				header, set, way, data_in->xaction_id, data_in->data[4], data_in->data[5], data_in->data[6], data_in->data[7], clock);
			fprintf(debug_stream, "%s set.way 0x%03x.%01x xaction id %#06x, 0x40 0x%016I64x 0x48 0x%016I64x 0x50 0x%016I64x 0x58 0x%016I64x clock: 0x%04llx\n",
				header, set, way, data_in->xaction_id, data_in->data[8], data_in->data[9], data_in->data[10], data_in->data[11], clock);
			fprintf(debug_stream, "%s set.way 0x%03x.%01x xaction id %#06x, 0x60 0x%016I64x 0x68 0x%016I64x 0x70 0x%016I64x 0x78 0x%016I64x clock: 0x%04llx\n",
				header, set, way, data_in->xaction_id, data_in->data[12], data_in->data[13], data_in->data[14], data_in->data[15], clock);
		}
	}
	if (L2->tags[set].state[way] == modified_line && !hit) // need to add cache line flush (if not tag match)
		debug++;
	L2->array.select[0] = 0x8000 | (way << 12) | set | 0x0800;//strobe, way, set, direction (0x0800 write, 0x0000 read)
	L2->array.xaction_id_in[0] = data_in->xaction_id;
	copy_data_bus_info(&L2->array.data[0], data_in);

	L2->tags[set].tag[way] = (addr >> L2->tag_shift);
	L2->tags[set].state[way] = (data_in->snoop_response == snoop_dirty) ? modified_line : (alloc) ? exclusive_line : shared_line;
}
// clock is only for debug purposes
UINT8 write_to_array(L2_block_type* L2, UINT64 addr, UINT8 alloc, data_bus_type* data_in, UINT64 clock, UINT mhartid, UINT debug_unit, char* header, FILE* debug_stream) {
	UINT8 success = 1;
	if (L2->array.read_write) {
		if (L2->array.select[0] == 0)
			add_write(L2, addr, alloc, data_in, clock, mhartid, debug_unit, header, debug_stream);
		else
			success = 0;
	}
	else {
		UINT8 bus_busy = 0;
		for (UINT i = 0; i < 8; i++) {
			bus_busy |= L2->array.data[i].snoop_response;
			bus_busy |= (L2->array.select[i] != 0) ? 1 : 0;
		}
		if (!bus_busy) {
			L2->array.read_write = 1;
			add_write(L2, addr, alloc, data_in, clock, mhartid, debug_unit, header, debug_stream);
		}
		else {
			success = 0;
		}
	}
	return success;
}


void uL2_data_forward_to_L0(data_bus_type* bus2_data_read_0, data_bus_type* bus2_data_read_1, L2_cache_type* cache_var, UINT mhartid, UINT64 clock, UINT debug_unit, FILE* debug_stream) {
	UINT8 stop = 0;
	while (!stop && cache_var->data_r_start != cache_var->data_r_stop) {
		if (cache_var->data_read_list[cache_var->data_r_start].status == link_free)
			cache_var->data_r_start = ((cache_var->data_r_start + 1) & 0x3f);
		else
			stop = 1;
	}
	for (UINT8 current = cache_var->data_r_start; current != cache_var->data_r_stop; current = ((current + 1) & 0x1f)) {
		if ((cache_var->data_read_list[current].latch.xaction_id >> 7) & 1) { // data
			if (cache_var->data_read_list[current].status == link_hold && bus2_data_read_1->snoop_response == snoop_idle) {
				copy_data_bus_info(bus2_data_read_1, &cache_var->data_read_list[current].latch);
				cache_var->data_read_list[current].status = link_free;
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%x) xaction id %#06x, data from hold to dL0//  clock: 0x%04llx\n", mhartid, bus2_data_read_1->xaction_id, clock);
				}
			}
		}
		else {//code
			if (cache_var->data_read_list[current].status == link_hold && bus2_data_read_0->snoop_response == snoop_idle) {
				copy_data_bus_info(bus2_data_read_0, &cache_var->data_read_list[current].latch);
				cache_var->data_read_list[current].status = link_free;
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%x) xaction id %#06x, data from hold to cL0//  clock: 0x%04llx\n", mhartid, bus2_data_read_0->xaction_id, clock);
				}
			}
		}
	}

}
// 
// array - schedule xaction towards core if hit
// bus to memory - forward towards memory if miss
// tracker - handles current state of transaction until complete NOTE: current - seperate index format for load/allocate vs fetch/prefecth
void L2_addr_path(addr_bus_type* bus_to_memory, L2_cache_type* cache_var, cache_addr_linked_list_type* addr_tracker, UINT8 current, addr_bus_type* bus_from_core, UINT64 clock, UINT mhartid, UINT debug_unit, param_type* param, FILE* debug_stream) {
	int debug = 0;
	if (bus_from_core->strobe == 1) {// data strobe
		UINT8 hit = 0;
		UINT set = (bus_from_core->addr >> 7) & 0x000007ff;
		INT64 tag = bus_from_core->addr >> 18;
		if (bus_from_core->cacheable == page_non_cache) {
			copy_addr_info(&addr_tracker[current].latch, bus_from_core);
			if (bus_to_memory->strobe == 0) {
				copy_addr_info(bus_to_memory, bus_from_core);
				bus_to_memory->strobe = 1;
				addr_tracker[current].addr_sent = 1;
				addr_tracker[current].status = link_issued_forward;
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%x) addr ", mhartid);
					fprint_addr_coma(debug_stream, bus_from_core->addr, param);
					fprintf(debug_stream, "xaction id %#06x q_id:%#4x, non_cacheable sent to L3; clock: 0x%04llx\n", 
						bus_from_core->xaction_id, current, clock);
				}
			}
			else {
				addr_tracker[current].addr_sent = 0;
				addr_tracker[current].status = link_issued_forward; // state for outgoing bus free
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%x) addr ", mhartid);
					fprint_addr_coma(debug_stream, bus_from_core->addr,param);
					fprintf(debug_stream, "xaction id %#06x q_id:%#4x, non_cacheable queued to L3; clock: 0x%04llx\n", 
						bus_from_core->xaction_id, current, clock);
				}
			}
			addr_tracker[current].latch.clock = clock;
		}
		else if ((cache_var->bank.array.select[0] & 0x8000) == 0x0000) {
			if (cache_var->bank.array.read_write) {
				copy_addr_info(&addr_tracker[current].latch, bus_from_core);
				addr_tracker[current].addr_sent = 0;
				addr_tracker[current].status = link_hold;
				addr_tracker[current].latch.clock = clock;
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%x) addr ", mhartid);
					fprint_addr_coma(debug_stream, bus_from_core->addr, param);
					fprintf(debug_stream, "xaction id %#06x q_id:%#4x, address from data L0 to hold, set in use; clock: 0x%04llx\n", 
						bus_from_core->xaction_id, current, clock);
				}
			}
			else {
				if (clock == 0x18c9)
					debug++;
				char header[0x100];
				sprintf_s(header, "uL2(%d)", mhartid);
				switch (cache_lookup_update(&cache_var->bank, bus_from_core->addr, clock, header, bus_from_core->xaction_id, bus_from_core->xaction, debug_unit, param, debug_stream)) {
				case snoop_hit:
				case snoop_dirty:
					copy_addr_info(&addr_tracker[current].latch, bus_from_core);
					addr_tracker[current].status = link_issued_forward2;
					break;
				case snoop_miss: {
					copy_addr_info(&addr_tracker[current].latch, bus_from_core);// data out needs to be synched to bus3 clock
					addr_tracker[current].latch.clock = clock;
					addr_tracker[current].addr_sent = 0;
					addr_tracker[current].status = link_issued_forward; // state for outgoing bus free
					if (debug_unit) {
						fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d addr ",
							mhartid, set, cache_var->bank.tags[set].way_ptr);
						fprint_addr_coma(debug_stream, bus_from_core->addr, param);
						fprintf(debug_stream, "xaction id %#06x q_id:%#4x, MISS queued to L3(0); clock: 0x%04llx\n", 
							bus_from_core->xaction_id, current, clock);
					}
				}
							   break;
				default:
					debug++;
					break;
				}
			}
		}
		else {
			copy_addr_bus_info(&addr_tracker[current].latch, bus_from_core, clock);
			addr_tracker[current].addr_sent = 0;
			//			addr_tracker[current].latch.clock = clock;
			if (debug_unit) {
				fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d addr ", mhartid, set, cache_var->bank.tags[set].way_ptr);
				fprint_addr_coma(debug_stream, bus_from_core->addr, param);
				fprintf(debug_stream, " xaction id %#06x q_id:%#4x, queued to L3 (1); clock: 0x%04llx\n", bus_from_core->xaction_id, current, clock);
			}
			addr_tracker[current].status = link_issued_forward;
		}
	}
}
// array - schedule xaction towards core if hit
// bus to memory - forward towards memory if miss
// tracker - handles current state of transaction until complete
void L2_hold_path(L2_cache_type* cache_var, addr_bus_type* bus_to_memory, cache_addr_linked_list_type* addr_tracker, UINT8 current, UINT64 clock, UINT mhartid, UINT debug_unit, param_type* param, FILE* debug_stream) {
	int debug = 0;
	UINT set;
	INT64 tag;

	set = (addr_tracker[current].latch.addr >> 7) & 0x000007ff;
	tag = addr_tracker[current].latch.addr >> 18;
	if (addr_tracker[current].status == link_issued_forward) {
		if (bus_to_memory->strobe == 0) {
			copy_addr_info(bus_to_memory, &addr_tracker[current].latch);
			bus_to_memory->strobe = 1;
			addr_tracker[current].addr_sent = 1;
			addr_tracker[current].status = link_issued_forward;
			if (debug_unit) {

				fprintf(debug_stream, "uL2(%x) set.way %#05x.%d addr ", mhartid, set, cache_var->bank.tags[set].way_ptr);
				fprint_addr_coma(debug_stream, addr_tracker[current].latch.addr, param);
				fprintf(debug_stream, "xaction id %#06x q_id:%#4x, ", addr_tracker[current].latch.xaction_id, current);
				fprintf(debug_stream, "from hold sent to L3; clock: 0x%04llx\n", clock);
			}
		}
	}
	else if (addr_tracker[current].status == link_hold) {// data strobe
		if (clock == 0x18c9)
			debug++;

		if (cache_var->bank.array.select[0] == 0) {
			char header[0x100];
			sprintf_s(header, "uL2(%d)", mhartid);
			switch (cache_lookup_update(&cache_var->bank, addr_tracker[current].latch.addr, clock, header, addr_tracker[current].latch.xaction_id, addr_tracker[current].latch.xaction, debug_unit,param, debug_stream)) {
			case snoop_stall:
				debug++;
				break;
			case snoop_hit:
			case snoop_dirty:
				addr_tracker[current].status = link_issued_forward2;
				break;
			case snoop_miss:
				addr_tracker[current].latch.clock = clock;
				addr_tracker[current].addr_sent = 0;
				addr_tracker[current].status = link_issued_forward; // state for outgoing bus free
				if (debug_unit) {

					fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d addr 0x%016I64x xaction id %#06x q_id:%#4x, ",
						mhartid, set, cache_var->bank.tags[set].way_ptr, addr_tracker[current].latch.addr, addr_tracker[current].latch.xaction_id, current);
					fprintf(debug_stream, "queued to L3 (2); clock: 0x%04llx\n", clock);
				}
				break;
			case snoop_idle:
				debug++;
				break;
			default:
				debug++;
				break;
			}
		}
	}
}
void uL2_code_fetch_addr_path(L2_cache_type* cache_var, addr_bus_type* bus2_addr_read, UINT mhartid, UINT64 clock, UINT debug_unit, param_type* param, FILE* debug_stream) {
	int debug = 0;
	if (bus2_addr_read->strobe) {// data strobe
		if (bus2_addr_read->cacheable != page_non_cache && cache_var->bank.array.select[0] == 0) {
			if (clock == 0x18c9)
				debug++;

			char header[0x100];
			sprintf_s(header, "uL2(%d)", mhartid);
			switch (cache_lookup_update(&cache_var->bank, bus2_addr_read->addr, clock, header, bus2_addr_read->xaction_id, bus2_addr_read->xaction, debug_unit,param, debug_stream)) {
			case snoop_hit:
			case snoop_dirty:
				break;
			case snoop_miss: {
				UINT8 current = ((bus2_addr_read->xaction_id >> 8) & 0x3f);
				copy_addr_bus_info(&cache_var->addr_fetch_list[current].latch, bus2_addr_read, clock);
				cache_var->addr_fetch_list[current].addr_sent = 0;
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%x) xaction id %#06x q_id:%#4x, ", mhartid, bus2_addr_read->xaction_id, current);
					fprintf(debug_stream, "FETCH address from data L0 queued to go to L3: 0x%016I64x, clock: 0x%04llx\n", bus2_addr_read->addr, clock);
				}
				cache_var->addr_fetch_list[current].status = link_issued_forward;
			}
						   break;
			default:
				debug++;
				break;
			}
		}
		else {
			UINT8 current = ((bus2_addr_read->xaction_id >> 8) & 0x3f);
			copy_addr_bus_info(&cache_var->addr_fetch_list[current].latch, bus2_addr_read, clock);
			cache_var->addr_fetch_list[current].addr_sent = 0;
			if (debug_unit) {
				fprintf(debug_stream, "uL2(%x) xaction id %#06x q_id:%#4x, ", mhartid, bus2_addr_read->xaction_id, current);
				fprintf(debug_stream, "FETCH address from data L0 queued to go to L3: 0x%016I64x, clock: 0x%04llx\n", bus2_addr_read->addr, clock);
			}
			cache_var->addr_fetch_list[current].status = link_issued_forward;
		}
	}
}
void uL2_data_unit_addr_path(L2_cache_type* cache_var, data_bus_type* bus2_data_read, addr_bus_type* bus3_addr, addr_bus_type* bus2_addr, UINT mhartid, UINT64 clock, UINT debug_unit, UINT debug_unit_walk, param_type* param, FILE* debug_stream) {
	int debug = 0;
	if (bus2_addr->strobe) {// data strobe
		UINT8 port = ((bus2_addr->xaction_id >> 8) & 3);
		UINT8 hit = 0;
		if (clock == 0x2cdd)
			debug++;
		UINT8 current = ((bus2_addr->xaction_id >> 12) & (cache_var->write_count - 1));
		if (bus2_addr->addr == io_addr_L2_ctrl) {
			if (bus2_addr->xaction == bus_SC_aq_rl) {
				copy_addr_info(&cache_var->addr_write_list[current].latch, bus2_addr); // need to wait for data cycle to determine if FLUSH
				if (debug_unit_walk) {
					fprintf(debug_stream, "uL2(%x) xaction id %#06x,", mhartid, bus2_addr->xaction_id);
					fprintf(debug_stream, " ctrl reg write: address : 0x%016I64x, clock: 0x%04llx\n", cache_var->addr_write_list[current].latch.addr, clock);
				}
			}
			else if (bus2_addr->xaction == bus_LR_aq_rl) {// need to wait for data cycle to determine if FLUSH
				if (bus2_data_read->snoop_response != snoop_idle)
					debug++;
				bus2_data_read->snoop_response = snoop_hit;
				bus2_data_read->xaction_id = bus2_addr->xaction_id;
				bus2_data_read->cacheable = bus2_addr->cacheable;
				bus2_data_read->valid = 0x0f;
				bus2_data_read->data[0] = cache_var->flush;
				if (debug_unit_walk) {
					fprintf(debug_stream, "uL2(%x) xaction id %#06x, ", mhartid, bus2_addr->xaction_id);
					fprintf(debug_stream, " L2 cache control register read: address : %#x, clock: 0x%04llx\n", bus2_addr->addr, clock);
				}
			}
			else {
				debug++;
			}
		}
		else if (bus2_addr->xaction == bus_store_full || bus2_addr->xaction == bus_store_partial) {
			UINT set = (bus2_addr->addr >> 7) & 0x00007ff;
			if (cache_var->addr_write_list[current].latch.xaction_id == (bus2_addr->xaction_id & 0xfeff) && cache_var->addr_write_list[current].latch.xaction == bus_allocate) {
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%x) set.way: 0x%03x.%d address : 0x%016x, xaction id %#06x, ",
						mhartid, set, cache_var->bank.tags[set].way_ptr, bus2_addr->addr, bus2_addr->xaction_id);
					fprintf(debug_stream, " WRITE address hold, allocate hit: alloc xaction id %#06x, clock: 0x%04llx\n", cache_var->addr_write_list[current].latch.xaction_id, clock);
				}
				hit = 1;
				copy_addr_info(&cache_var->addr_write_list[current].latch, bus2_addr); // need to wait for data length, full cacheline status

				// error, need to check cache array, "hit" or way_ptr not dirty, else evict cache line here
				if (cache_var->addr_write_list[current].status != link_issued_forward)
					cache_var->addr_write_list[current].status = link_free;
				cache_var->addr_write_list[current].addr_sent = 0;
				if (cache_var->snoop_write_list[current].status == link_free) {
				}
				else {
					fprintf(debug_stream, "uL2(%x) set.way: 0x%03x.%d address : 0x%016x, xaction id %#06x, ",
						mhartid, set, cache_var->bank.tags[set].way_ptr, bus2_addr->addr, bus2_addr->xaction_id);
					fprintf(debug_stream, "ERROR: WRITE data list overflow on allocated data, data loss: clock: 0x%04llx\n", clock);
				}
			}
			else if (cache_var->addr_write_list[current].status == link_free) {
				hit = 1;
				copy_addr_bus_info(&cache_var->addr_write_list[current].latch, bus2_addr, clock); // need to wait for data length, full cacheline status
				cache_var->addr_write_list[current].data.snoop_response = snoop_idle;
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%x) set.way: 0x%03x.%d address : 0x%016x, xaction id %#06x, ",
						mhartid, set, cache_var->bank.tags[set].way_ptr, bus2_addr->addr, bus2_addr->xaction_id);
					fprintf(debug_stream, "WRITE address hold: clock: 0x%04llx\n", clock);
				}
				if (cache_var->snoop_write_list[current].status != link_free) {
					fprintf(debug_stream, "uL2(%x) set.way: 0x%03x.%d address : 0x%016x, xaction id %#06x, ",
						mhartid, set, cache_var->bank.tags[set].way_ptr, bus2_addr->addr, bus2_addr->xaction_id);
					fprintf(debug_stream, "ERROR: WRITE data list overflow, data loss: old time: 0x%04llx, old id: %#06x, clock: 0x%04llx\n",
						cache_var->snoop_write_list[current].time, cache_var->snoop_write_list[current].latch.xaction_id, clock);
				}
				cache_var->addr_write_list[current].addr_sent = 0;
				cache_var->addr_write_list[current].status = link_write_addr;
			}
			else {
				fprintf(debug_stream, "uL2(%x) set.way: 0x%03x.%d address : 0x%016x, xaction id %#06x, ",
					mhartid, set, cache_var->bank.tags[set].way_ptr, bus2_addr->addr, bus2_addr->xaction_id);
				fprintf(debug_stream, "ERROR: WRITE address list overflow, data loss:lock: 0x%04llx\n", clock);
			}
		}
		if (!hit) {
			switch ((bus2_addr->xaction_id >> 8) & 3) {
			case 1:
				L2_addr_path(bus3_addr, cache_var, cache_var->addr_load_list, current, bus2_addr, clock, mhartid, debug_unit, param, debug_stream);
				break;
			case 2:
				L2_addr_path(bus3_addr, cache_var, cache_var->addr_alloc_list, current, bus2_addr, clock, mhartid, debug_unit, param, debug_stream);
				break;
			case 3:
				if (bus2_addr->xaction == bus_store_full)
					debug++;
				if (cache_var->addr_write_list[current].status != link_free) {
					fprintf(debug_stream, "uL2(%x) xaction id %#06x, ", mhartid, bus2_addr->xaction_id);
					fprintf(debug_stream, "ERROR: active write bufer over write, data lost (xaction id %#06x, time 0x%08llx): addr: 0x%016I64x, clock: 0x%04llx\n",
						cache_var->addr_write_list[current].latch.xaction_id, cache_var->addr_write_list[current].latch.clock, cache_var->addr_write_list[current].latch.addr, clock);
					// question - should we delay buffer ready until address is out of overflow buffer??
					bus2_data_read->snoop_response = snoop_stall;
					copy_addr_bus_info(&cache_var->addr_write_list[current].latch, bus2_addr, clock);
					cache_var->addr_write_list[current].status = link_issued_forward;
				}
				else {
					cache_var->addr_write_list[current].status = link_issued_forward;
					cache_var->addr_write_list[current].latch.clock = clock;
				}
				break;
			default:
				debug++;
				break;
			}
			if (bus2_addr->xaction == bus_store_full && ((clock & 1) == 0)) {// writes need to snoop L2 first before forwarding to bus3
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%x) xaction id %#06x, ", mhartid, bus2_addr->xaction_id);
					fprintf(debug_stream, "WRITE FULL address received (to L2 cache array) : 0x%016I64x,  clock: 0x%04llx\n", bus2_addr->addr, clock);
				}
				if (cache_var->addr_write_list[current].status != link_free) {
					debug++;
				}
				copy_addr_bus_info(&cache_var->addr_write_list[current].latch, bus2_addr, clock);
				cache_var->addr_write_list[current].addr_sent = 0;
				cache_var->addr_write_list[current].status = link_write_addr;
			}
			else {
				// IO space
				switch (bus2_addr->addr) {
				case io_addr_L2_ctrl:
					break;
				case io_addr_L0D_ctrl:
				case io_addr_L0C_ctrl:
				case io_addr_TLB_data_ctrl:
				case io_addr_TLB_code_ctrl:
				case (io_addr_TLB_data_ctrl | 8):
				case (io_addr_TLB_code_ctrl | 8):
				case io_addr_TLB_4K_data:
				case io_addr_TLB_4K_code:
				case io_addr_TLB_2M_data:
				case io_addr_TLB_2M_code:
				case io_addr_TLB_data_vaddr:
				case io_addr_TLB_code_vaddr:
					//				case io_addr_TLB_data_vaddr:
					//				case io_addr_TLB_code_vaddr:
					for (UINT8 i = 0; i < 8 && cache_var->bank.array.array_busy_snoop_stall == 0; i++) {
						if (cache_var->bank.array.data[i].snoop_response != snoop_idle) {
							if (bus2_data_read->snoop_response != snoop_idle)
								debug++;
							bus2_data_read->snoop_response = snoop_stall;
							cache_var->bank.array.array_busy_snoop_stall = 1;
							if (debug_unit) {
								fprintf(debug_stream, "uL2(%d) xaction id %#06x, cache array busy on incoming IO cycle//  clock: 0x%04llx\n", mhartid, cache_var->bank.array.data[i].xaction_id, clock);
							}
						}
					}
					switch ((bus2_addr->xaction_id >> 8) & 3) {
					case 1:
						copy_addr_info(&cache_var->addr_load_list[current].latch, bus2_addr);
						cache_var->addr_load_list[current].addr_sent = 0;
						if (debug_unit) {
							fprintf(debug_stream, "uL2(%x) xaction id %#06x q_id:0x%02x, READ", mhartid, bus2_addr->xaction_id, current);
							fprintf(debug_stream, " ERROR; should be IO cycle: 0x%016I64x, clock: 0x%04llx\n", bus2_addr->addr, clock);
						}
						break;
					case 2:
						copy_addr_info(&cache_var->addr_alloc_list[current].latch, bus2_addr);
						cache_var->addr_alloc_list[current].addr_sent = 0;
						if (debug_unit) {
							fprintf(debug_stream, "uL2(%x) xaction id %#06x q_id:0x%02x, ", mhartid, bus2_addr->xaction_id, current);
							fprintf(debug_stream, "local IO LOAD address from data L0 to hold v2, do not forward: 0x%016I64x, clock: 0x%04llx\n", bus2_addr->addr, clock);
						}
						break;
					case 3:
						copy_addr_info(&cache_var->addr_write_list[current].latch, bus2_addr);
						cache_var->addr_write_list[current].addr_sent = 0;
						if (debug_unit) {
							fprintf(debug_stream, "uL2(%x) xaction id %#06x q_id:0x%02x, ", mhartid, bus2_addr->xaction_id, current);
							fprintf(debug_stream, "local IO STORE address from data L0 to hold v2, do not forward: 0x%016I64x, clock: 0x%04llx\n", bus2_addr->addr, clock);
						}
						break;
					default:
						debug++;
						break;
					}
					break;
				default:
					switch ((bus2_addr->xaction_id >> 8) & 3) {
					case 1:
						break;
					case 2:
						break;
					case 3:
						if (cache_var->addr_write_list[current].status != link_free)
							debug++;
						copy_addr_info(&cache_var->addr_write_list[current].latch, bus2_addr);
						cache_var->addr_write_list[current].addr_sent = 0;
						if (debug_unit) {
							int set = ((bus2_addr->addr >> 7) & 0x07ff);
							fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d addr 0x%016I64x xaction id %#06x q_id:0x%02x, ",
								mhartid, set, cache_var->bank.tags[set].way_ptr, bus2_addr->addr, bus2_addr->xaction_id, current);
							if (bus2_addr->addr >= 0x10002000 && bus2_addr->addr < 0x10003000)
								fprintf(debug_stream, "Locked STORE address from data L0 to hold v2; clock: 0x%04llx\n", clock);
							else
								fprintf(debug_stream, "STORE address from data L0 to hold v2, bus3 not ready; clock: 0x%04llx\n", clock);
						}
						break;
					default:
						debug++;
						break;
					}
					break;
				}
			}
		}
	}
}
// clock is only for debug purposes
void uL2_data_write_path_0(L2_cache_type* cache_var, data_bus_type* bus2_data_read, data_bus_type* bus2_data_write, UINT mhartid, UINT64 clock, UINT debug_unit, UINT debug_unit_walk, UINT debug_unit_ext_snoop, param_type* param, FILE* debug_stream) {
	UINT debug = 0;
	if (bus2_data_write->snoop_response != snoop_idle) {
		if ((bus2_data_write->xaction_id & 0x07) == mhartid) {
			for (UINT8 current = 0; current < 8; current++) {
				if (bus2_data_write->xaction_id == cache_var->addr_write_list[current].latch.xaction_id) {
					switch (cache_var->addr_write_list[current].latch.addr) {
					case io_addr_L2_ctrl:
						break;
					case io_addr_L0D_ctrl:
					case io_addr_L0C_ctrl:
					case io_addr_TLB_data_ctrl:
					case io_addr_TLB_code_ctrl:
					case (io_addr_TLB_data_ctrl | 8):
					case (io_addr_TLB_code_ctrl | 8):
					case io_addr_TLB_4K_data:
					case io_addr_TLB_4K_code:
					case io_addr_TLB_2M_data:
					case io_addr_TLB_2M_code:
					case io_addr_TLB_data_vaddr:
					case io_addr_TLB_code_vaddr:
						//					case io_addr_TLB_data_vaddr:
						//					case io_addr_TLB_code_vaddr:
						cache_var->addr_write_list[current].status = link_retire;
						break;
					default: {
						UINT8 hit = 0;
						if (cache_var->addr_write_list[current].latch.xaction_id == bus2_data_write->xaction_id) {
							if (cache_var->addr_write_list[current].latch.xaction == bus_SC_aq_rl && cache_var->addr_write_list[current].status != link_retire && cache_var->addr_write_list[current].status != link_free &&
								cache_var->addr_write_list[current].latch.addr >= 0x0000000010002000 && cache_var->addr_write_list[current].latch.addr < 0x0000000010003000) {
								hit = 1;
								cache_var->addr_write_list[current].status = link_retire;
								if (debug_unit) {
									fprintf(debug_stream, "uL2(%x) xaction id %#06x, IO write to TLBs clock: 0x%04llx\n", mhartid, bus2_data_write->xaction_id, clock);
								}
							}
						}
						if (!hit) {
							if (cache_var->addr_write_list[current].latch.xaction_id == bus2_data_write->xaction_id && (bus2_data_write->xaction_id & 0x0f == mhartid)) {
								if (cache_var->addr_write_list[current].latch.xaction != bus_store_full)
									debug++;
								char header[0x100];
								sprintf_s(header, "uL2(%d)", mhartid);
								UINT8 success = write_to_array(&cache_var->bank, cache_var->addr_write_list[current].latch.addr, (cache_var->addr_write_list[current].latch.xaction == bus_allocate) ? 1 : 0, bus2_data_write, clock, mhartid, debug_unit, header, debug_stream);
								if (!success)
									debug++;
								cache_var->addr_write_list[current].status = link_retire;
								cache_var->snoop_write_list[current].status = link_retire;
							}
						}
					}
						   break;
					}
				}
			}
		}
		else { // external snoop
			UINT8 current = bus2_data_write->xaction_id & 0x0f;
			cache_var->snoop_write_list[current].external_snoop = (ExternalSnoopResponse)(cache_var->snoop_write_list[current].external_snoop | 1);
			if (bus2_data_write->cacheable == page_non_cache)
				debug++;
			if (bus2_data_write->snoop_response == snoop_hit) {
				char header[0x100];
				sprintf_s(header, "uL2(%d)", mhartid);
				copy_data_bus_info(&cache_var->snoop_write_list[current].latch, bus2_data_write);
				cache_var->snoop_write_list[current].latch.valid = 0xffffffffffffffff;
				cache_var->snoop_write_list[current].status = link_hold;
				cache_var->snoop_write_list[current].latch.valid = 0;
				switch (cache_lookup(&cache_var->bank, cache_var->snoop_write_list[current].addr, clock, header, bus2_data_write->xaction_id, cache_var->snoop_write_list[current].xaction, debug_unit, debug_stream)) {
				case snoop_miss: {
					UINT8 success = write_to_array(&cache_var->bank, cache_var->snoop_write_list[current].addr, (cache_var->snoop_write_list[current].xaction == bus_allocate) ? 1 : 0, bus2_data_write, clock, mhartid, debug_unit, header, debug_stream);
					if (!success && debug_unit_ext_snoop) {
						fprintf(debug_stream, "uL2(%x) xaction id %#06x, ", mhartid, bus2_data_write->xaction_id);
						fprintf(debug_stream, "WARNING: external snoop response HIT, shared data not latched in L2 on way out. clock: 0x%04llx\n", clock);
					}
				}
							   break;
				case snoop_dirty:
					debug++; // coherency error
					break;
				case snoop_hit:
					break;// expected - keep going
				default:
					debug++; // ?? unexpected - check coding
					break;
				}
			}
			else if (bus2_data_write->snoop_response == snoop_dirty) {
				//				UINT8 current = bus2_data_write->xaction_id & 0x0f;
				copy_data_bus_info(&cache_var->snoop_write_list[current].latch, bus2_data_write);
				cache_var->snoop_write_list[current].latch.valid = 0xffffffffffffffff;
				cache_var->snoop_write_list[current].status = link_hold;
				cache_var->snoop_write_list[current].latch.valid = 0;
				char header[0x100];
				sprintf_s(header, "uL2(%d)", mhartid);
				UINT8 success = write_to_array(&cache_var->bank, cache_var->snoop_write_list[current].addr, (cache_var->snoop_write_list[current].xaction == bus_allocate) ? 1 : 0, bus2_data_write, clock, mhartid, debug_unit, header, debug_stream);
				if (!success && debug_unit_ext_snoop) {
					fprintf(debug_stream, "uL2(%x) xaction id %#06x, ", mhartid, bus2_data_write->xaction_id);
					fprintf(debug_stream, "WARNING: external snoop response dirty, shared data not latched in L2 on way out. clock: 0x%04llx\n", clock);
				}
			}
			else if (bus2_data_write->snoop_response == snoop_miss) {
				//				UINT8 current = bus2_data_write->xaction_id & 0x0f;
				int set = (cache_var->snoop_write_list[current].addr >> 7) & 0x000007ff;
				if ((cache_var->snoop_write_list[current].external_snoop & 3) == ExternalSnoop_ready) {
					copy_data_bus_info(&cache_var->snoop_write_list[current].latch, bus2_data_write);
					cache_var->snoop_write_list[current].latch.valid = 0;
					cache_var->snoop_write_list[current].status = link_hold;
					cache_var->snoop_write_list[current].time = clock;
					if (bus2_data_write->xaction_id == cache_var->snoop_write_list[current].latch.xaction_id) {
						for (UINT8 way = 0; way < 8; way++) {
							if (cache_var->bank.tags[set].tag[way] == (cache_var->snoop_write_list[current].addr >> 18)) {
								if (cache_var->bank.array.select[0] != 0)
									debug++;
								if (cache_var->bank.array.read_write)
									debug++;
								cache_var->bank.array.read_write = 0;
								cache_var->bank.tags[set].way_ptr = way;
								cache_var->bank.array.select[0] = 0x8000 | (way << 12) | set; // initiate data fetch
								cache_var->bank.array.xaction_id_in[0] = bus2_data_write->xaction_id;
								cache_var->bank.array.snoop_response[0] = (cache_var->bank.tags[set].state[way] == modified_line) ? snoop_dirty : snoop_hit;

								cache_var->bank.tags[set].state[way] = (cache_var->snoop_write_list[current].xaction == bus_allocate ||
									cache_var->snoop_write_list[current].xaction == bus_LR_aq_rl || cache_var->snoop_write_list[current].xaction == bus_SC_aq_rl) ? invalid_line : shared_line;
								// needs to be placed in array read q, to L3?? need to check mux
							}
						}
						if (debug_unit_ext_snoop) {
							fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d (%s) addr 0x%016x xaction id %#06x, ",
								mhartid, (cache_var->snoop_write_list[current].addr >> 7) & 0x000007ff, cache_var->bank.tags[set].way_ptr,
								(cache_var->bank.tags[set].way_ptr == modified_line) ? "M" : "S", cache_var->snoop_write_list[current].addr, bus2_data_write->xaction_id);
							fprintf(debug_stream, "L0C Snoop Response Valid delayed clock: 0x%04llx\n", clock);
						}
					}
					else {
						fprintf(debug_stream, "uL2(%x) xaction id %#06x, ", mhartid, bus2_data_write->xaction_id);
						fprintf(debug_stream, "ERROR: L0C snoop response to invalid delayed clock: 0x%04llx\n", clock);
					}
				}
				else {
					if (debug_unit_ext_snoop) {
						fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d %s addr ", mhartid, set, cache_var->bank.tags[set].way_ptr,
							(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == modified_line) ? "M" : "S");
						fprint_addr_coma(debug_stream, cache_var->snoop_write_list[current].addr, param);
						fprintf(debug_stream, "xaction id %#06x, L0C Snoop Response Received, waiting (no data); clock: 0x%04llx\n", 
							bus2_data_write->xaction_id, clock);
					}
				}
			}
			else {
				debug++; // need to fill cache, as well as above code
			}
		}
	}
}
void uL2_data_write_path_1(data_bus_type* bus2_data_read, data_bus_type* bus3_data_write, L2_cache_type* cache_var, data_bus_type* bus2_data_write, UINT mhartid, UINT64 clock, UINT debug_unit, UINT debug_unit_walk, UINT debug_unit_ext_snoop, param_type* param, FILE* debug_stream) {
	UINT debug = 1;
	if (mhartid == 0)
		if (clock == 0x2368)
			debug++;
	if (bus2_data_write->snoop_response != snoop_idle) {
		if ((bus2_data_write->xaction_id & 0x07) == mhartid) {
			for (UINT8 current = 0; current < 8; current++) {
				if (bus2_data_write->xaction_id == cache_var->addr_write_list[current].latch.xaction_id &&
					cache_var->addr_write_list[current].status != link_free && cache_var->addr_write_list[current].status != link_retire) {
					switch (cache_var->addr_write_list[current].latch.addr) {
					case io_addr_L2_ctrl: {
						copy_data_bus_info(bus2_data_read, bus2_data_write);
						bus2_data_read->data[0] = 0;
						bus2_data_read->cacheable = page_IO_error_rsp;
						switch (bus2_data_write->data[0]) {
						case 0:
							cache_var->flush = 1;
							for (UINT16 j = 0; j < 0x800; j++) {
								for (UINT16 k = 0; k < 0x10; k++) bus2_data_read->data[0] |= ((cache_var->bank.tags[j].state[k] == modified_line) ? 1 : 0);
								cache_var->bank.tags[j].state[0] = (cache_line_state_type)(cache_var->bank.tags[j].state[0] & 4);
							}
							if (debug_unit_walk) {
								fprintf(debug_stream, "uL2(%x) xaction id %#06x, FLUSH and dissable L2 clock: 0x%04llx\n", mhartid, bus2_data_write->xaction_id, clock);
							}
							break;
						case 1:
							if (debug_unit_walk) {
								fprintf(debug_stream, "uL2(%x) xaction id %#06x, enable L2 return error response clock: 0x%04llx\n", mhartid, bus2_data_write->xaction_id, clock);
							}
							break;
						default:
							debug++;
							break;
						}
						cache_var->enabled = bus2_data_write->data[0];
						cache_var->addr_write_list[current].status = link_retire;
					}
										break;
					case io_addr_L0D_ctrl:
					case io_addr_L0C_ctrl:
					case io_addr_TLB_data_ctrl:
					case io_addr_TLB_code_ctrl:
					case (io_addr_TLB_data_ctrl | 8):
					case (io_addr_TLB_code_ctrl | 8):
					case io_addr_TLB_4K_data:
					case io_addr_TLB_4K_code:
					case io_addr_TLB_2M_data:
					case io_addr_TLB_2M_code:
					case io_addr_TLB_data_vaddr:
					case io_addr_TLB_code_vaddr:
						//					case io_addr_TLB_data_vaddr:
						//					case io_addr_TLB_code_vaddr:
						cache_var->addr_write_list[current].status = link_retire;
						break;
					default: {
						if (cache_var->addr_write_list[current].latch.xaction == bus_SC_aq_rl && cache_var->addr_write_list[current].status != link_retire && cache_var->addr_write_list[current].status != link_free &&
							cache_var->addr_write_list[current].latch.addr >= 0x0000000010002000 && cache_var->addr_write_list[current].latch.addr < 0x0000000010003000) {
							cache_var->addr_write_list[current].status = link_retire;
							if (debug_unit) {
								fprintf(debug_stream, "uL2(%x) xaction id %#06x, IO write to TLBs clock: 0x%04llx\n", mhartid, bus2_data_write->xaction_id, clock);
							}
						}
						else if (cache_var->addr_write_list[current].latch.xaction == bus_store_full) {
							UINT set = (cache_var->addr_write_list[current].latch.addr >> 7) & 0x00007ff;
							UINT8 hit = 0;
							char header[0x100];
							sprintf_s(header, "uL2(%d)", mhartid);
							if (write_to_array(&cache_var->bank, cache_var->addr_write_list[current].latch.addr, (cache_var->addr_write_list[current].latch.xaction == bus_allocate) ? 1 : 0, bus2_data_write, clock, mhartid, debug_unit, header, debug_stream)) {
								cache_var->addr_write_list[current].status = link_retire;
								//			cache_var->snoop_write_list[current].status = link_free;
								//			cache_var->snoop_write_list[current].time = clock;
								if (debug_unit) {
									fprintf(debug_stream, "uL2(%x) xaction id %#06x, WRITE L0 eviction to L2 array clock: 0x%04llx\n", mhartid, bus2_data_write->xaction_id, clock);
								}
							}
							else {
								copy_data_bus_info(&cache_var->addr_write_list[current].data, bus2_data_write);
								cache_var->addr_write_list[current].status = link_cache_array_write_pend;
								if (debug_unit) {
									fprintf(debug_stream, "uL2(%x) xaction id %#06x, WRITE L0 eviction to hold clock: 0x%04llx\n", mhartid, bus2_data_write->xaction_id, clock);
								}
							}
						}
						else if (bus3_data_write->snoop_response == snoop_idle) {
							if (cache_var->addr_write_list[current].addr_sent == 1) {
								copy_data_bus_info(bus3_data_write, bus2_data_write);
								if (debug_unit) {
									fprintf(debug_stream, "uL2(%x) xaction id %#06x, WRITE data forward to L3 clock: 0x%04llx\n", mhartid, bus3_data_write->xaction_id, clock);
								}
								cache_var->addr_write_list[current].status = link_retire;
							}
							else {
								UINT8 current = (bus2_data_write->xaction_id >> 12) & 0x0f;
								copy_data_bus_info(&cache_var->snoop_write_list[current].latch, bus2_data_write);
								cache_var->snoop_write_list[current].status = link_hold;
								cache_var->snoop_write_list[current].time = clock;
								if (debug_unit) {
									fprintf(debug_stream, "uL2(%x) xaction id %#06x, WRITE data pending Address still Active clock: 0x%04llx\n", mhartid, cache_var->snoop_write_list[current].latch.xaction_id, clock);
								}
							}
						}
					}
						   break;
					}
				}
			}
		}
		else {
			UINT8 current = bus2_data_write->xaction_id & 0x0f;
			cache_var->snoop_write_list[current].external_snoop = (ExternalSnoopResponse)(cache_var->snoop_write_list[current].external_snoop | 2);
			if (bus2_data_write->cacheable == page_non_cache)
				debug++;
			switch (bus2_data_write->snoop_response) {
			case snoop_hit:
			case snoop_dirty: {
				copy_data_bus_info(&cache_var->snoop_write_list[current].latch, bus2_data_write);
				cache_var->snoop_write_list[current].latch.valid = 0xffffffffffffffff;
				cache_var->snoop_write_list[current].time = clock;
				cache_var->snoop_write_list[current].status = link_hold;
				if (bus2_data_write->xaction_id == cache_var->snoop_write_list[current].latch.xaction_id) {
					char header[0x100];
					sprintf_s(header, "uL2(%d)", mhartid);
					if (write_to_array(&cache_var->bank, cache_var->snoop_write_list[current].addr, (cache_var->snoop_write_list[current].xaction == bus_allocate) ? 1 : 0, bus2_data_write, clock, mhartid, debug_unit, header, debug_stream)) {
						cache_var->snoop_write_list[current].latch.valid = 0;
					}
					else {
						if (debug_unit) {
							fprintf(debug_stream, "uL2(%x) xaction id %#06x, ", mhartid, bus2_data_write->xaction_id);
							fprintf(debug_stream, "WARNING: maybe dropping write data, need to confirm array update from latch clock: 0x%04llx\n", clock);
						}
					}
				}
				else {
					fprintf(debug_stream, "uL2(%x) xaction id %#06x, ", mhartid, bus2_data_write->xaction_id);
					fprintf(debug_stream, "ERROR: L0D snoop response HIT to invalid delayed clock: 0x%04llx\n", clock);
				}
			}
							break;
			case snoop_miss: {
				if ((cache_var->snoop_write_list[current].external_snoop & 3) == ExternalSnoop_ready) {
					//					UINT8 current = bus2_data_write->xaction_id & 0x0f;
					int set = (cache_var->snoop_write_list[current].addr >> 7) & 0x000007ff;

					if (cache_var->snoop_write_list[current].latch.snoop_response != snoop_hit) {
						copy_data_bus_info(&cache_var->snoop_write_list[current].latch, bus2_data_write);
						cache_var->snoop_write_list[current].latch.snoop_response = snoop_miss;// need to wait for code and data L0 response
						cache_var->snoop_write_list[current].latch.valid = 0;
						cache_var->snoop_write_list[current].status = link_hold;
						cache_var->snoop_write_list[current].time = clock;

						if (clock == 0x3154)
							debug++;
						char header[0x100];
						sprintf_s(header, "uL2(%d)", mhartid);
						switch (cache_lookup_update(&cache_var->bank, cache_var->snoop_write_list[current].addr, clock, header, bus2_data_write->xaction_id, cache_var->snoop_write_list[current].xaction, debug_unit,param, debug_stream)) {
						case snoop_hit:
						case snoop_dirty:
							if (debug_unit_ext_snoop) {
								fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d (%s) addr 0x%016x xaction id %#06x, ",
									mhartid, (cache_var->snoop_write_list[current].addr >> 7) & 0x000007ff, cache_var->bank.tags[set].way_ptr,
									(cache_var->bank.tags[set].way_ptr == modified_line) ? "M" : "S", cache_var->snoop_write_list[current].addr, bus2_data_write->xaction_id);
								fprintf(debug_stream, "L0D Snoop Response Valid delayed 2 clock: 0x%04llx\n", clock);

								UINT64* data = cache_var->snoop_write_list[current].latch.data;
								fprintf(debug_stream, "uL2(%x) xaction id %#06x 0x00 0x%016I64x, 0x08 0x%016I64x, 0x10 0x%016I64x, 0x18 0x%016I64x, 0x20 0x%016I64x, 0x28 0x%016I64x, 0x30 0x%016I64x, 0x38 0x%016I64x clock: 0x%04llx\n",
									mhartid, cache_var->snoop_write_list[current].latch.xaction_id, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
								fprintf(debug_stream, "uL2(%x) xaction id %#06x 0x40 0x%016I64x, 0x48 0x%016I64x, 0x50 0x%016I64x, 0x58 0x%016I64x, 0x60 0x%016I64x, 0x68 0x%016I64x, 0x70 0x%016I64x, 0x78 0x%016I64x clock: 0x%04llx\n",
									mhartid, cache_var->snoop_write_list[current].latch.xaction_id, data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
							}
							break;
						case snoop_miss:
							if (debug_unit_ext_snoop) {
								fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d addr ",
									mhartid, (cache_var->snoop_write_list[current].addr >> 7) & 0x000007ff, cache_var->bank.tags[set].way_ptr);
								fprint_addr_coma(debug_stream, cache_var->snoop_write_list[current].addr, param);
								fprintf(debug_stream, "xaction id %#06x, ",bus2_data_write->xaction_id);
								fprintf(debug_stream, "L0D Snoop Response Received (no data) MISS(1) clock: 0x%04llx\n", clock);
							}
							break;
						case snoop_stall:
							cache_var->snoop_write_list[current].status = link_cache_array_read_pend;
							if (debug_unit_ext_snoop) {
								fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d (%s) addr 0x%016x xaction id %#06x, ",
									mhartid, (cache_var->snoop_write_list[current].addr >> 7) & 0x000007ff, cache_var->bank.tags[set].way_ptr,
									(cache_var->bank.tags[set].way_ptr == modified_line) ? "M" : "S", cache_var->snoop_write_list[current].addr, bus2_data_write->xaction_id);
								fprintf(debug_stream, "L2 array busy, update delayed clock: 0x%04llx\n", clock);
							}
							break;
						default:
							debug++;
							break;
						}
					}
					else {
						if (debug_unit_ext_snoop) {
							fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d addr 0x%016x xaction id %#06x, ",
								mhartid, (cache_var->snoop_write_list[current].addr >> 7) & 0x000007ff, cache_var->bank.tags[set].way_ptr,
								cache_var->snoop_write_list[current].addr, bus2_data_write->xaction_id);
							fprintf(debug_stream, "L0D MISS (no data) L0C HIT clock: 0x%04llx\n", clock);
						}
					}
				}
				else {
					if (debug_unit_ext_snoop) {
						fprintf(debug_stream, "uL2(%x) xaction id %#06x, ", mhartid, bus2_data_write->xaction_id);
						fprintf(debug_stream, "L0D Snoop Response Received (no data), waiting; clock: 0x%04llx\n", clock);
					}
				}
			}
						   break;
			case snoop_stall:// no valid data on the bus
				break;
			default:
				debug++;
				break;
			}
		}
	}
}
void uL2_external_snoop_latch(L2_cache_type* cache_var, addr_bus_type* snoop_L2, UINT mhartid, UINT64 clock, UINT debug_unit_ext_snoop, param_type *param, FILE* debug_stream) {
	if (snoop_L2->strobe == 1) {
		UINT8 ptr = snoop_L2->xaction_id & (core_count -1);
		cache_var->snoop_write_list[ptr].external_snoop = ExternalSnoop_WaitCodeData;// need to receive from sTLB: code, data, or both??
		cache_var->snoop_write_list[ptr].status = link_hold;
		cache_var->snoop_write_list[ptr].xaction = snoop_L2->xaction;
		cache_var->snoop_write_list[ptr].addr = snoop_L2->addr;
		cache_var->snoop_write_list[ptr].latch.snoop_response = snoop_idle;
		cache_var->snoop_write_list[ptr].latch.cacheable = snoop_L2->cacheable;
		cache_var->snoop_write_list[ptr].latch.xaction_id = snoop_L2->xaction_id;
		cache_var->snoop_write_list[ptr].time = clock;
		if (debug_unit_ext_snoop || ((1<<ptr)&param->core && clock > param->start_time && (param->store_bus||param->load_bus || 
			(param->prefetcher&&(snoop_L2->xaction == bus_prefetch || snoop_L2->xaction==bus_fetch))))) {
			int set = ((snoop_L2->addr >> 7) & 0x07ff);
			fprintf(debug_stream, "uL2(%x) set.way 0x%03x.x addr ", mhartid, set);
			fprint_addr_coma(debug_stream, snoop_L2->addr, param);
			fprintf(debug_stream, " xaction id %#06x, External Snoop Queued clock: 0x%04llx\n", snoop_L2->xaction_id, clock);
		}
	}
}
void uL2_bus3_addr_out_interface(addr_bus_type* bus3_addr, data_bus_type* bus3_data_write, L2_cache_type* cache_var, data_bus_type bus3_data_read, UINT mhartid, UINT64 clock, UINT debug_unit, UINT debug_unit_walk, param_type* param, FILE* debug_stream) {
	int debug = 0;
	for (UINT8 i = 0; i < cache_var->write_count && bus3_addr->strobe == 0 && bus3_data_read.snoop_response != snoop_stall; i++) {
		if (cache_var->addr_write_list[i].status != link_free && cache_var->addr_write_list[i].status != link_write_addr && cache_var->addr_write_list[i].addr_sent == 0 &&
			!(cache_var->addr_write_list[i].latch.addr >= 0x10002000 && cache_var->addr_write_list[i].latch.addr < 0x10003000) && cache_var->addr_write_list[i].latch.xaction != bus_store_full) {

			copy_addr_info(bus3_addr, &cache_var->addr_write_list[i].latch);

			cache_var->addr_write_list[i].status = link_issued_forward;
			cache_var->addr_write_list[i].addr_sent = 1;
			if (debug_unit) {
				fprintf(debug_stream, "uL2(%x) xaction id %#06x, ", mhartid, cache_var->addr_write_list[i].latch.xaction_id);
				fprintf(debug_stream, "data write poster to L3 addr: 0x%016I64x, clock: 0x%04llx\n", cache_var->addr_write_list[i].latch.addr, clock);
			}
		}
	}
	// L3 interface address out portion
	if (bus3_addr->strobe == 0 && bus3_data_read.snoop_response != snoop_stall) {// issue, need to prioritize holds in order of : code, data (read, write; demand, prefetch)
		if (cache_var->flush) {
			for (UINT16 set = 0; set < 0x800 && bus3_addr->strobe == 0; set++) {
				for (UINT8 way = 0; way < 8 && bus3_addr->strobe == 0; way++) {
					if (cache_var->bank.tags[set].state[way] == modified_line) {
						bus3_addr->strobe = 1;
						bus3_addr->addr = (cache_var->bank.tags[set].tag[way] << 18) | (set << 7);
						bus3_addr->cacheable = page_rx;
						bus3_addr->xaction = bus_store_full;
						bus3_addr->xaction_id = (way << 12) | (3 << 8) | mhartid;

						bus3_data_write->snoop_response = snoop_dirty;
						bus3_data_write->cacheable = bus3_addr->cacheable;
						bus3_data_write->valid = -1;
						bus3_data_write->xaction_id = bus3_addr->xaction_id;
						for (UINT8 j = 0; j < 8; j++) bus3_data_write->data[j] = cache_var->bank.array.data[set].data[j];

						if (debug_unit_walk) {
							fprintf(debug_stream, "uL2(%x) set.way 0x%04x.%d: xaction id %#06x, ", mhartid, set, way, bus3_addr->xaction_id);
							fprintf(debug_stream, "L2 line eviction: addr: 0x%016I64x, clock: 0x%04llx\n", bus3_addr->addr, clock);
						}
					}
					cache_var->bank.tags[set].state[way] = invalid_line;
				}
			}
			cache_var->flush = bus3_addr->strobe;
		}
		else {
			for (UINT current = 0; current < 0x40 && bus3_addr->strobe == 0; current++) {
				if (cache_var->addr_fetch_list[current].status != link_free && cache_var->addr_fetch_list[current].latch.xaction == bus_fetch && cache_var->addr_fetch_list[current].addr_sent == 0) {// demand fetch
					L2_hold_path(cache_var, bus3_addr, cache_var->addr_fetch_list, current, clock, mhartid, debug_unit, param, debug_stream);
				}
			}
			for (UINT current = 0; current < 4 && bus3_addr->strobe == 0; current++) {
				if (cache_var->addr_load_list[current].status != link_free && cache_var->addr_load_list[current].status != link_retire && cache_var->addr_load_list[current].addr_sent == 0) {
					if (cache_var->addr_load_list[current].latch.addr >= 0x10002000 && cache_var->addr_load_list[current].latch.addr < 0x10003000) {
						// cache control registers, do not forward to L3
					}
					else {
						L2_hold_path(cache_var, bus3_addr, cache_var->addr_load_list, current, clock, mhartid, debug_unit, param, debug_stream);
					}
				}
			}
			for (UINT current = 0; current < 4 && bus3_addr->strobe == 0; current++) {
				if (cache_var->addr_alloc_list[current].status != link_free && cache_var->addr_alloc_list[current].status != link_retire && cache_var->addr_alloc_list[current].addr_sent == 0) {
					if (cache_var->addr_alloc_list[current].latch.addr >= 0x10002000 && cache_var->addr_alloc_list[current].latch.addr < 0x10003000) {
						// cache control registers, do not forward to L3
					}
					else {
						L2_hold_path(cache_var, bus3_addr, cache_var->addr_alloc_list, current, clock, mhartid, debug_unit,param,  debug_stream);
					}
				}
			}
			for (UINT current = 0; current < 0x40 && bus3_addr->strobe == 0; current++) {//prefetch
				if (cache_var->addr_fetch_list[current].status != link_free && cache_var->addr_fetch_list[current].status != link_retire && cache_var->addr_fetch_list[current].addr_sent == 0) {
					L2_hold_path(cache_var, bus3_addr, cache_var->addr_fetch_list, current, clock, mhartid, debug_unit,param, debug_stream);
				}
			}
		}																			 // highest priority to data write ejections, free up write buffers fast
	}
}
void uL2_bus3_data_write_out_interface(data_bus_type* bus3_data_write, L2_cache_type* cache_var, UINT mhartid, UINT64 clock, UINT debug_unit, FILE* debug_stream) {
	// L3 interface data out portion
	int debug = 0;
	UINT8 hit = 0;
	if (mhartid == 0)
		if (clock >= 0x2369)
			debug++;
	for (UINT current_data = 0; current_data < 8 && !hit && (bus3_data_write->snoop_response == snoop_idle || bus3_data_write->snoop_response == snoop_stall); current_data++) {
		if ((cache_var->snoop_write_list[current_data].latch.xaction_id & 0x0f) != mhartid && cache_var->snoop_write_list[current_data].status == link_hold && cache_var->snoop_write_list[current_data].external_snoop == ExternalSnoop_ready) {
			hit = 1;
			if (mhartid == 0)
				if (clock >= 0x0101)
					debug++;

			switch (cache_var->snoop_write_list[current_data].latch.snoop_response) {
			case snoop_dirty: {
				copy_data_bus_info(bus3_data_write, &cache_var->snoop_write_list[current_data].latch);
				UINT8 ptr = bus3_data_write->xaction_id & 0x07;
				UINT set = (cache_var->snoop_write_list[ptr].addr >> 7) & 0x000007ff;
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d %s addr,data 0x%016x,0x%016x xaction id %#06x, ",
						mhartid, (cache_var->snoop_write_list[ptr].addr >> 7) & 0x000007ff, cache_var->bank.tags[set].way_ptr,
						(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == modified_line) ? "M" :
						(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == shared_line) ? "S" :
						(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == exclusive_line) ? "E" : "I",
						cache_var->snoop_write_list[ptr].addr, bus3_data_write->data[(cache_var->snoop_write_list[ptr].addr >> 7) & 0x0f],
						bus3_data_write->xaction_id);
					//					if (bus3_data_write->snoop_response == snoop_hit)
					fprintf(debug_stream, "External Snoop HIT, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
					//					else if (bus3_data_write->snoop_response == snoop_dirty)
					//						fprintf(debug_stream, "External Snoop DIRTY, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
					//					else
					//						fprintf(debug_stream, "External Snoop MISS, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
				}
				cache_var->snoop_write_list[current_data].status = link_free;
				cache_var->snoop_write_list[current_data].external_snoop == ExternalSnoop_WaitCodeData;
			}
							break;
			case snoop_miss: {
				data_bus_type* latch_ptr = &cache_var->snoop_write_list[current_data].latch;
				UINT8 ptr = latch_ptr->xaction_id & 0x07;
				UINT set = (cache_var->snoop_write_list[ptr].addr >> 7) & 0x000007ff;
				for (UINT way = 0; way < 8; way++) {
					if ((cache_var->bank.tags[set].tag[way] == (cache_var->snoop_write_list[ptr].addr >> 18)) && (cache_var->bank.tags[set].state[way] != invalid_line)) {
						cache_var->bank.tags[set].way_ptr = way;
						latch_ptr->snoop_response = (cache_var->bank.tags[set].state[way] == modified_line) ? snoop_dirty : snoop_hit;
						cache_var->bank.tags[set].state[way] = (cache_var->snoop_write_list[ptr].xaction == bus_allocate ||
							cache_var->snoop_write_list[ptr].xaction == bus_LR_aq_rl || cache_var->snoop_write_list[ptr].xaction == bus_SC_aq_rl) ? invalid_line : shared_line;
					}
				}
				if (latch_ptr->snoop_response == snoop_miss) {
					copy_data_bus_info(bus3_data_write, &cache_var->snoop_write_list[current_data].latch);
					if (debug_unit) {
						fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d %s addr,data 0x%016x,0x%016x xaction id %#06x, ",
							mhartid, (cache_var->snoop_write_list[ptr].addr >> 7) & 0x000007ff, cache_var->bank.tags[set].way_ptr,
							(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == modified_line) ? "M" :
							(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == shared_line) ? "S" :
							(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == exclusive_line) ? "E" : "I",
							cache_var->snoop_write_list[ptr].addr, bus3_data_write->data[(cache_var->snoop_write_list[ptr].addr >> 7) & 0x0f],
							bus3_data_write->xaction_id);
						//						if (bus3_data_write->snoop_response == snoop_hit)
						//							fprintf(debug_stream, "External Snoop HIT, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
						//						else if (bus3_data_write->snoop_response == snoop_dirty)
						//							fprintf(debug_stream, "External Snoop DIRTY, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
						//						else
						fprintf(debug_stream, "External Snoop MISS, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
					}
					cache_var->snoop_write_list[current_data].status = link_free;
					cache_var->snoop_write_list[current_data].external_snoop == ExternalSnoop_WaitCodeData;
				}// else wait for data out of dadhe array to arrive
			}
						   break;
			case snoop_hit: {
				data_bus_type* latch_ptr = &cache_var->snoop_write_list[current_data].latch;
				UINT8 ptr = latch_ptr->xaction_id & 0x07;
				UINT set = (cache_var->snoop_write_list[ptr].addr >> 7) & 0x000007ff;
				for (UINT way = 0; way < 8; way++) {
					if ((cache_var->bank.tags[set].tag[way] == (cache_var->snoop_write_list[ptr].addr >> 18)) && (cache_var->bank.tags[set].state[way] != invalid_line)) {
						cache_var->bank.tags[set].way_ptr = way;
						latch_ptr->snoop_response = (cache_var->bank.tags[set].state[way] == modified_line) ? snoop_dirty : snoop_hit;
						cache_var->bank.tags[set].state[way] = (cache_var->snoop_write_list[ptr].xaction == bus_allocate ||
							cache_var->snoop_write_list[ptr].xaction == bus_LR_aq_rl || cache_var->snoop_write_list[ptr].xaction == bus_SC_aq_rl) ? invalid_line : shared_line;
					}
				}
				if (latch_ptr->snoop_response == snoop_hit) {
					copy_data_bus_info(bus3_data_write, &cache_var->snoop_write_list[current_data].latch);
					if (debug_unit) {
						fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d %s addr,data 0x%016x,0x%016x xaction id %#06x, ",
							mhartid, (cache_var->snoop_write_list[ptr].addr >> 7) & 0x000007ff, cache_var->bank.tags[set].way_ptr,
							(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == modified_line) ? "M" :
							(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == shared_line) ? "S" :
							(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == exclusive_line) ? "E" : "I",
							cache_var->snoop_write_list[ptr].addr, bus3_data_write->data[(cache_var->snoop_write_list[ptr].addr >> 7) & 0x0f],
							bus3_data_write->xaction_id);
						//						if (bus3_data_write->snoop_response == snoop_hit)
						fprintf(debug_stream, "External Snoop HIT, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
						//						else if (bus3_data_write->snoop_response == snoop_dirty)
						//							fprintf(debug_stream, "External Snoop DIRTY, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
						//						else
						//	fprintf(debug_stream, "External Snoop MISS, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
					}
					cache_var->snoop_write_list[current_data].status = link_free;
					cache_var->snoop_write_list[current_data].external_snoop == ExternalSnoop_WaitCodeData;
				}// else wait for data out of dadhe array to arrive
			}
						  break;
			default:
				debug++;
			}
			if (cache_var->snoop_write_list[current_data].latch.snoop_response == snoop_dirty) {
			}
			else {
				data_bus_type* latch_ptr = &cache_var->snoop_write_list[current_data].latch;
				UINT8 ptr = latch_ptr->xaction_id & 0x07;
				UINT set = (cache_var->snoop_write_list[ptr].addr >> 7) & 0x000007ff;
				if (latch_ptr->snoop_response != snoop_dirty) {
					for (UINT way = 0; way < 8; way++) {
						if ((cache_var->bank.tags[set].tag[way] == (cache_var->snoop_write_list[ptr].addr >> 18)) && (cache_var->bank.tags[set].state[way] != invalid_line)) {
							cache_var->bank.tags[set].way_ptr = way;
							latch_ptr->snoop_response = (cache_var->bank.tags[set].state[way] == modified_line) ? snoop_dirty : snoop_hit;
							cache_var->bank.tags[set].state[way] = (cache_var->snoop_write_list[ptr].xaction == bus_allocate ||
								cache_var->snoop_write_list[ptr].xaction == bus_LR_aq_rl || cache_var->snoop_write_list[ptr].xaction == bus_SC_aq_rl) ? invalid_line : shared_line;
						}
					}
				}
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d %s addr,data 0x%016x,0x%016x xaction id %#06x, ",
						mhartid, (cache_var->snoop_write_list[ptr].addr >> 7) & 0x000007ff, cache_var->bank.tags[set].way_ptr,
						(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == modified_line) ? "M" :
						(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == shared_line) ? "S" :
						(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == exclusive_line) ? "E" : "I",
						cache_var->snoop_write_list[ptr].addr, bus3_data_write->data[(cache_var->snoop_write_list[ptr].addr >> 7) & 0x0f],
						bus3_data_write->xaction_id);
					if (bus3_data_write->snoop_response == snoop_hit)
						fprintf(debug_stream, "External Snoop HIT, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
					else if (bus3_data_write->snoop_response == snoop_dirty)
						fprintf(debug_stream, "External Snoop DIRTY, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
					else
						fprintf(debug_stream, "External Snoop MISS, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
				}
				cache_var->snoop_write_list[current_data].status = link_free;
				cache_var->snoop_write_list[current_data].external_snoop == ExternalSnoop_WaitCodeData;
			}
			/*
			copy_data_bus_info(bus3_data_write, &cache_var->snoop_write_list[current_data].latch);
			UINT8 ptr = bus3_data_write->xaction_id & 0x07;
			UINT set = (cache_var->snoop_write_list[ptr].addr >> 7) & 0x000007ff;
			if (bus3_data_write->snoop_response != snoop_dirty) {
				for (UINT way = 0; way < 8; way++) {
					if ((cache_var->bank.tags[set].tag[way] == (cache_var->snoop_write_list[ptr].addr >> 18))&& (cache_var->bank.tags[set].state[way] != invalid_line)) {
						cache_var->bank.tags[set].way_ptr = way;
						bus3_data_write->snoop_response = (cache_var->bank.tags[set].state[way] == modified_line) ? snoop_dirty : snoop_hit;
						cache_var->bank.tags[set].state[way] = (cache_var->snoop_write_list[ptr].xaction == bus_allocate ||
							cache_var->snoop_write_list[ptr].xaction == bus_LR_aq_rl || cache_var->snoop_write_list[ptr].xaction == bus_SC_aq_rl) ? invalid_line : shared_line;
					}
				}
			}
			if (debug_unit) {
				fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d %s addr,data 0x%016x,0x%016x xaction id %#06x, ",
					mhartid, (cache_var->snoop_write_list[ptr].addr >> 7) & 0x000007ff, cache_var->bank.tags[set].way_ptr,
					(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == modified_line) ? "M" :
					(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == shared_line) ? "S" :
					(cache_var->bank.tags[set].state[cache_var->bank.tags[set].way_ptr] == exclusive_line) ? "E" : "I",
					cache_var->snoop_write_list[ptr].addr, bus3_data_write->data[(cache_var->snoop_write_list[ptr].addr>>7)&0x0f],
					bus3_data_write->xaction_id);
				if (bus3_data_write->snoop_response == snoop_hit)
					fprintf(debug_stream, "External Snoop HIT, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
				else if (bus3_data_write->snoop_response == snoop_dirty)
					fprintf(debug_stream, "External Snoop DIRTY, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
				else
					fprintf(debug_stream, "External Snoop MISS, data from hold to L3; valid: %#010x, clock: 0x%04llx\n", bus3_data_write->valid, clock);
			}
			cache_var->snoop_write_list[current_data].status = link_free;
			cache_var->snoop_write_list[current_data].external_snoop == ExternalSnoop_WaitCodeData;
			/**/
		}
	}
	for (UINT current = 0; current < cache_var->write_count && !hit; current++) {
		if (cache_var->addr_write_list[current].addr_sent == 1 && ((!(cache_var->addr_write_list[current].status == link_retire || cache_var->addr_write_list[current].status == link_free) && cache_var->snoop_write_list[current].status != link_free && bus3_data_write->snoop_response == snoop_idle) ||
			(cache_var->snoop_write_list[current].latch.xaction_id != mhartid && cache_var->snoop_write_list[current].status != link_free && (bus3_data_write->snoop_response == snoop_idle || bus3_data_write->snoop_response == snoop_stall)))) {
			hit = 1;
			copy_data_bus_info(bus3_data_write, &cache_var->snoop_write_list[current].latch);
			if (debug_unit) {
				UINT set = (cache_var->addr_write_list[current].latch.addr >> 7) & 0x000007ff;
				fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d addr: 0x%016I64x, xaction id %#06x, WRITE data from hold; valid: %#010x, clock: 0x%04llx\n",
					mhartid, set, cache_var->bank.tags[set].way_ptr, cache_var->addr_write_list[current].latch.addr, bus3_data_write->xaction_id, bus3_data_write->valid, clock);
			}
			cache_var->snoop_write_list[current].status = link_free;
			cache_var->addr_write_list[current].status = link_retire;
		}
		else if (cache_var->addr_write_list[current].latch.addr >= 0x10002000 && cache_var->addr_write_list[current].latch.addr < 0x10003000 && cache_var->snoop_write_list[current].status != link_free && cache_var->addr_write_list[current].latch.xaction_id == cache_var->snoop_write_list[current].latch.xaction_id) {
			if (debug_unit) {
				fprintf(debug_stream, "uL2(%x) xaction id %#06x, WRITE data to cache csr, do not forward; valid: %#010x, clock: 0x%04llx\n", mhartid, cache_var->snoop_write_list[current].latch.xaction_id, cache_var->snoop_write_list[current].latch.valid, clock);
			}
			cache_var->snoop_write_list[current].status = link_free;
			cache_var->addr_write_list[current].status = link_retire;
			hit = 1;
		}
	}
}
void uL2_bus3_data_read_in_interface(data_bus_type* bus2_data_read_0, data_bus_type* bus2_data_read_1, L2_cache_type* cache_var, data_bus_type* bus3_data_read, UINT mhartid, UINT64 clock, UINT debug_unit, FILE* debug_stream) {
	UINT debug = 0;
	// L3 interface data in portion
	if (mhartid == 0) {
		if (clock == 0x13c1)
			debug++;
	}
	if ((bus3_data_read->snoop_response == snoop_hit || bus3_data_read->snoop_response == snoop_dirty) && (bus3_data_read->xaction_id & 7) == mhartid) {
		UINT8 hit = 0;
		if ((bus3_data_read->xaction_id >> 7) & 1) {
			UINT8 current = ((bus3_data_read->xaction_id >> 12) & (cache_var->write_count - 1));
			if (bus2_data_read_1->snoop_response == snoop_idle || bus2_data_read_1->snoop_response == snoop_stall) {
				copy_data_bus_info(bus2_data_read_1, bus3_data_read);
				for (UINT8 i = 0; i < 0x10; i++) bus2_data_read_1->data[i] = bus3_data_read->data[i];
				if (cache_var->addr_alloc_list[current].latch.xaction_id == bus3_data_read->xaction_id) {
					if (cache_var->bank.array.select[0] == 0) {
						char header[0x100];
						sprintf_s(header, "uL2(%d)", mhartid);
						if (write_to_array(&cache_var->bank, cache_var->addr_alloc_list[current].latch.addr, (cache_var->addr_alloc_list[current].latch.xaction == bus_allocate) ? 1 : 0, bus3_data_read, clock, mhartid, debug_unit, header, debug_stream)) {
							cache_var->addr_alloc_list[current].status = link_free;
							if (debug_unit) {
								UINT set = (cache_var->addr_alloc_list[current].latch.addr >> 7) & 0x00007ff;
								fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d (%s)addr 0x%016x xaction id %#06x, ALLOC return data to L0D, clock: 0x%04llx \n",
									mhartid, set, cache_var->bank.tags[set].way_ptr, (bus3_data_read->snoop_response == snoop_dirty) ? "M" : "E",
									cache_var->addr_alloc_list[current].latch.addr, bus3_data_read->xaction_id, clock);
							}
						}
						else {
							copy_data_bus_info(&cache_var->addr_alloc_list[current].data, bus3_data_read);
							cache_var->addr_alloc_list[current].status = link_cache_array_write_pend;
						}
					}
					else {
						copy_data_bus_info(&cache_var->addr_alloc_list[current].data, bus3_data_read);
						cache_var->addr_alloc_list[current].status = link_cache_array_write_pend;
					}
				}
				else if (cache_var->addr_load_list[current].latch.xaction_id == bus3_data_read->xaction_id) {
					if (cache_var->bank.array.select[0] == 0) {
						char header[0x100];
						sprintf_s(header, "uL2(%d)", mhartid);
						if (write_to_array(&cache_var->bank, cache_var->addr_load_list[current].latch.addr, (cache_var->addr_load_list[current].latch.xaction == bus_allocate) ? 1 : 0, bus3_data_read, clock, mhartid, debug_unit, header, debug_stream)) {
							cache_var->addr_load_list[current].status = link_free;
							if (debug_unit) {
								fprintf(debug_stream, "uL2(%x) xaction id %#06x, LOAD return data to L0D, addr,data 0x%016I64x,0x%016I64x  clock: 0x%04llx \n",
									mhartid, bus3_data_read->xaction_id, cache_var->addr_load_list[current].latch.addr, bus3_data_read->data[(cache_var->addr_load_list[current].latch.addr >> 3) & 0x0f], clock);
							}
						}
					}
					else {
						copy_data_bus_info(&cache_var->addr_load_list[current].data, bus3_data_read);
						cache_var->addr_load_list[current].status = link_cache_array_write_pend;
					}
				}
				else {
					if (debug_unit) {
						fprintf(debug_stream, "uL2(%x) xaction id %#06x, ERROR: data returned to L0D, clock: 0x%04llx \n",
							mhartid, bus3_data_read->xaction_id, clock);
					}
				}
			}
			else {
				copy_data_bus_info(&cache_var->data_read_list[cache_var->data_r_stop].latch, bus3_data_read);
				cache_var->data_read_list[cache_var->data_r_stop].status = link_hold;
				cache_var->data_r_stop = ((cache_var->data_r_stop + 1) & 0x1f);

				char header[0x100];
				sprintf_s(header, "uL2(%d)", mhartid);
				if (cache_var->addr_alloc_list[current].latch.xaction_id == bus3_data_read->xaction_id) {
					if (write_to_array(&cache_var->bank, cache_var->addr_alloc_list[current].latch.addr, (cache_var->addr_alloc_list[current].latch.xaction == bus_allocate) ? 1 : 0, bus3_data_read, clock, mhartid, debug_unit, header, debug_stream)) {
						cache_var->addr_alloc_list[current].status = link_free;
						if (debug_unit) {
							fprintf(debug_stream, "uL2(%x) xaction id %#06x, return alloc data to hold, clock: 0x%04llx \n",
								mhartid, bus3_data_read->xaction_id, clock);
						}
					}
					else {
						// hold
					}
				}
				else if (cache_var->addr_load_list[current].latch.xaction_id == bus3_data_read->xaction_id) {
					if (write_to_array(&cache_var->bank, cache_var->addr_load_list[current].latch.addr, (cache_var->addr_load_list[current].latch.xaction == bus_allocate) ? 1 : 0, bus3_data_read, clock, mhartid, debug_unit, header, debug_stream)) {
						cache_var->addr_load_list[current].status = link_free;
						if (debug_unit) {
							fprintf(debug_stream, "uL2(%x) xaction id %#06x, return load data to hold, clock: 0x%04llx \n",
								mhartid, bus3_data_read->xaction_id, clock);
						}
					}
					else {
						// hold
					}
				}
				else {
					fprintf(debug_stream, "uL2(%x) xaction id %#06x, ERROR: no ID match, data to hold, clock: 0x%04llx \n",
						mhartid, bus3_data_read->xaction_id, clock);
				}
			}
			hit = 1;
		}
		else {
			if (bus2_data_read_0->snoop_response == snoop_idle || bus2_data_read_0->snoop_response == snoop_stall) {
				copy_data_bus_info(bus2_data_read_0, bus3_data_read);
				for (UINT8 i = 0; i < 0x10; i++) bus2_data_read_0->data[i] = bus3_data_read->data[i];
			}
			else {
				copy_data_bus_info(&cache_var->data_read_list[cache_var->data_r_stop].latch, bus3_data_read);
				cache_var->data_read_list[cache_var->data_r_stop].status = link_hold;
				cache_var->data_r_stop = ((cache_var->data_r_stop + 1) & 0x1f);
			}
		}

		UINT8 current = ((bus3_data_read->xaction_id >> 8) & 0x3f);
		if (bus3_data_read->cacheable != page_non_cache) {
			if (cache_var->addr_fetch_list[current].latch.xaction_id == bus3_data_read->xaction_id && cache_var->addr_fetch_list[current].status == link_issued_forward) {
				if (cache_var->addr_fetch_list[current].latch.cacheable != page_non_cache && cache_var->enabled) {
					char header[0x100];
					sprintf_s(header, "uL2(%d)", mhartid);
					if (write_to_array(&cache_var->bank, cache_var->addr_fetch_list[current].latch.addr, (cache_var->addr_fetch_list[current].latch.xaction == bus_allocate) ? 1 : 0, bus3_data_read, clock, mhartid, debug_unit, header, debug_stream)) {
						cache_var->addr_fetch_list[current].status = link_retire;
						if (debug_unit) {
							fprintf(debug_stream, "uL2(%x) xaction id %#06x, issue to L0C clock: 0x%04llx\n", mhartid, bus3_data_read->xaction_id, clock);
						}
					}
					else {
						if (debug_unit) {
							fprintf(debug_stream, "uL2(%x) xaction id %#06x, WARNING: array access stall event occuring, not sure this path works? clock: 0x%04llx\n", mhartid, bus3_data_read->xaction_id, clock);
						}
					}
					hit = 1;
				}
			}
		}
		else {
			if (cache_var->addr_fetch_list[current].latch.xaction_id == bus3_data_read->xaction_id) {
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%x) xaction id %#06x, UC; return data to L0C clock: 0x%04llx \n", mhartid, bus3_data_read->xaction_id, clock);
				}
				hit = 1;
				cache_var->addr_fetch_list[current].status = link_retire;
			}
		}
		if (debug_unit) {
			if (hit == 0) {
				fprintf(debug_stream, "uL2(%x) xaction id %#06x, ERROR: data returned to core, no id match, assume UC:", mhartid, bus3_data_read->xaction_id);
				if ((bus3_data_read->xaction_id >> 7) & 1)
					fprintf(debug_stream, "L0D");
				else
					fprintf(debug_stream, "L0C");
				fprintf(debug_stream, " clock: 0x%04llx \n", clock);
			}
		}
	}
}
void L2_2MB_cache(bus_w_snoop_signal1* bus2, bus_w_snoop_signal1* bus3, addr_bus_type* snoop_L2, UINT mhartid, UINT64 clock, L2_cache_type* cache_var, UINT8 L0_present, UINT debug_core, param_type *param, FILE* debug_stream) {
	int debug = 0;
	UINT debug_unit = (param->L2 || param->caches) && debug_core;
	UINT debug_unit_walk = (param->L2 || param->caches || param->PAGE_WALK) && debug_core;

	UINT debug_unit_ext_snoop = (param->L2 || param->caches || param->EXT_SNOOP) && debug_core;

	if (mhartid == 8) {// id: 0x1300
		if (clock >= 0x0115)// 1e79
			debug++;
	}
	if (mhartid == 1)
		if (clock >= 0x3154) {
			debug++;
			if ((cache_var->bank.array.line[0][0x19][1] & 0x0ff) != 0x13)
				debug++;
		}
	char header[0x100];
	sprintf_s(header, "uL2(%d) ", mhartid);
	if (cache_var->bank.array.array_busy_snoop_stall == 1) {
		cache_var->bank.array.array_busy_snoop_stall = 0;
		for (UINT8 i = 0; i < 8 && cache_var->bank.array.array_busy_snoop_stall == 0; i++) {
			if (cache_var->bank.array.data[i].snoop_response != snoop_idle) {
				bus2[0].data_read.out.snoop_response = snoop_stall;
				bus2[1].data_read.out.snoop_response = snoop_stall;
				cache_var->bank.array.array_busy_snoop_stall = 1;
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%d) xaction id %#06x, cache array busy on incoming IO cycle//  clock: 0x%04llx\n", mhartid, cache_var->bank.array.data[i].xaction_id, clock);
				}
			}
		}
	}
	if (clock_bank_array_path(&cache_var->bank, clock, header, debug_unit, debug_stream)) {
		if ((cache_var->bank.array.data[0].xaction_id & 0x000f) == mhartid) {// local data
			if (cache_var->bank.array.data[0].xaction_id & 0x0040) {// code
				copy_data_bus_info(&bus2[0].data_read.out, &cache_var->bank.array.data[0]);
				bus2[0].data_read.out.cacheable = page_x;
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%d) xaction id %#06x, L2 hit data issued to cL0//  clock: 0x%04llx\n", mhartid, cache_var->bank.array.data[0].xaction_id, clock);
				}
			}
			else {// data
				copy_data_bus_info(&bus2[1].data_read.out, &cache_var->bank.array.data[0]);
				bus2[0].data_read.out.cacheable = page_rw;
				if (debug_unit) {
					fprintf(debug_stream, "uL2(%d) xaction id %#06x, L2 hit data issued to dL0//  clock: 0x%04llx\n", mhartid, cache_var->bank.array.data[0].xaction_id, clock);
				}
			}
		}
		else {// error: missing evictions
			if (bus3->data_write.out.snoop_response == snoop_dirty)
				debug++;
			if (bus3->data_write.out.snoop_response != snoop_idle)
				debug++;
			copy_data_bus_info(&bus3->data_write.out, &cache_var->bank.array.data[0]); // need to queue up bus3, Bclock issue only
		}
		cache_var->bank.array.data[0].snoop_response = snoop_idle;
	}
	for (int current = 0; current < 8; current++) {
		if (cache_var->snoop_write_list[current].status == link_cache_array_read_pend) {
			cache_var->snoop_write_list[current].status = link_free;
			int set = (cache_var->snoop_write_list[current].addr >> 7) & 0x000007ff;
			char header[0x100];
			sprintf_s(header, "uL2(%d)", mhartid);
			switch (cache_lookup_update(&cache_var->bank, cache_var->snoop_write_list[current].addr, clock, header, cache_var->snoop_write_list[current].latch.xaction_id, cache_var->snoop_write_list[current].xaction, debug_unit, param, debug_stream)) {
			case snoop_hit:
			case snoop_dirty:
				if (debug_unit_ext_snoop) {
					fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d (%s) addr 0x%016I64x xaction id %#06x, ",
						mhartid, (cache_var->snoop_write_list[current].addr >> 7) & 0x000007ff, cache_var->bank.tags[set].way_ptr,
						(cache_var->bank.tags[set].way_ptr == modified_line) ? "M" : "S", cache_var->snoop_write_list[current].addr, cache_var->snoop_write_list[current].latch.xaction_id);
					fprintf(debug_stream, "L0D Snoop Response Valid delayed clock: 0x%04llx\n", clock);
					UINT64* data = cache_var->snoop_write_list[current].latch.data;
					fprintf(debug_stream, "uL2(%x) xaction id %#06x 0x00 0x%016I64x, 0x08 0x%016I64x, 0x10 0x%016I64x, 0x18 0x%016I64x, 0x20 0x%016I64x, 0x28 0x%016I64x, 0x30 0x%016I64x, 0x38 0x%016I64x clock: 0x%04llx\n",
						mhartid, cache_var->snoop_write_list[current].latch.xaction_id, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
					fprintf(debug_stream, "uL2(%x) xaction id %#06x 0x40 0x%016I64x, 0x48 0x%016I64x, 0x50 0x%016I64x, 0x58 0x%016I64x, 0x60 0x%016I64x, 0x68 0x%016I64x, 0x70 0x%016I64x, 0x78 0x%016I64x clock: 0x%04llx\n",
						mhartid, cache_var->snoop_write_list[current].latch.xaction_id, data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
				}
				break;
			case snoop_miss:
				if (debug_unit_ext_snoop) {
					fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d addr 0x%016x xaction id %#06x, ",
						mhartid, (cache_var->snoop_write_list[current].addr >> 7) & 0x000007ff, cache_var->bank.tags[set].way_ptr,
						cache_var->snoop_write_list[current].addr, cache_var->snoop_write_list[current].latch.xaction_id);
					fprintf(debug_stream, "L0D Snoop Response Received (no data) MISS(2) clock: 0x%04llx\n", clock);
				}
				break;
			case snoop_stall:
				cache_var->snoop_write_list[current].status = link_cache_array_read_pend;
				if (debug_unit_ext_snoop) {
					fprintf(debug_stream, "uL2(%x) set.way 0x%03x.%d (%s) addr 0x%016x xaction id %#06x, ",
						mhartid, (cache_var->snoop_write_list[current].addr >> 7) & 0x000007ff, cache_var->bank.tags[set].way_ptr,
						(cache_var->bank.tags[set].way_ptr == modified_line) ? "M" : "S", cache_var->snoop_write_list[current].addr, cache_var->snoop_write_list[current].latch.xaction_id);
					fprintf(debug_stream, "L2 array busy, update delayed clock: 0x%04llx\n", clock);
				}
				break;
			default:
				debug++;
				break;
			}
		}
	}
	for (int current = 0; current < 8; current++) {
		if (cache_var->addr_load_list[current].status == link_cache_array_write_pend) {
			if (cache_var->bank.array.select[0] == 0) {
				char header[0x100];
				sprintf_s(header, "uL2(%d)", mhartid);
				if (write_to_array(&cache_var->bank, cache_var->addr_load_list[current].latch.addr, (cache_var->addr_load_list[current].latch.xaction == bus_allocate) ? 1 : 0, &cache_var->addr_load_list[current].data, clock, mhartid, debug_unit, header, debug_stream)) {
					cache_var->addr_load_list[current].status = link_free;
					if (debug_unit) {
						fprintf(debug_stream, "uL2(%x) xaction id %#06x, LOAD return data to L0D, addr,data 0x%016I64x,0x%016I64x  clock: 0x%04llx \n",
							mhartid, cache_var->addr_load_list[current].data.xaction_id, cache_var->addr_load_list[current].latch.addr, cache_var->addr_load_list[current].data.data[(cache_var->addr_load_list[current].latch.addr >> 3) & 0x0f], clock);
					}
				}
			}
		}
		else if (cache_var->addr_alloc_list[current].status == link_cache_array_write_pend) {
			if (cache_var->bank.array.select[0] == 0) {
				char header[0x100];
				sprintf_s(header, "uL2(%d)", mhartid);
				if (write_to_array(&cache_var->bank, cache_var->addr_alloc_list[current].latch.addr, (cache_var->addr_alloc_list[current].latch.xaction == bus_allocate) ? 1 : 0, &cache_var->addr_alloc_list[current].data, clock, mhartid, debug_unit, header, debug_stream)) {
					cache_var->addr_alloc_list[current].status = link_free;
					if (debug_unit) {
						fprintf(debug_stream, "uL2(%x) xaction id %#06x, LOAD return data to L0D, addr,data 0x%016I64x,0x%016I64x  clock: 0x%04llx \n",
							mhartid, cache_var->addr_alloc_list[current].data.xaction_id, cache_var->addr_alloc_list[current].latch.addr, cache_var->addr_alloc_list[current].data.data[(cache_var->addr_alloc_list[current].latch.addr >> 3) & 0x0f], clock);
					}
				}
			}
		}
	}
	const UINT64 cache_mask_b = 0xfffffffffffe0000;
	UINT hit = 0;
	uL2_internal_variable_cleanup(cache_var, &bus2[1].data_read.in, clock);
	uL2_data_forward_to_L0(&bus2[0].data_read.out, &bus2[1].data_read.out, cache_var, mhartid, clock, debug_unit, debug_stream);

	for (UINT current = 0; current < 8; current++) {
		if (cache_var->addr_write_list[current].status != link_free && cache_var->addr_write_list[current].data.snoop_response != snoop_idle) {
			UINT set = (cache_var->addr_write_list[current].latch.addr >> 7) & 0x00007ff;
			char header[0x100];
			sprintf_s(header, "uL2(%d)", mhartid);
			if (write_to_array(&cache_var->bank, cache_var->addr_write_list[current].latch.addr, (cache_var->addr_write_list[current].latch.xaction == bus_allocate) ? 1 : 0, &cache_var->addr_write_list[current].data, clock, mhartid, debug_unit, header, debug_stream)) {
				cache_var->addr_write_list[current].status = link_retire;
				cache_var->snoop_write_list[current].status = link_free;
				cache_var->snoop_write_list[current].time = clock;
			}
		}
	}
	// issue delayed transactions before incoming transactions
	uL2_bus3_addr_out_interface(&bus3->addr.out, &bus3->data_write.out, cache_var, bus3->data_read.in, mhartid, clock, debug_unit, debug_unit_walk,param, debug_stream);
	L2_addr_path(&bus3->addr.out, cache_var, cache_var->addr_fetch_list, ((bus2[0].addr.in.xaction_id >> 8) & 0x3f), &bus2[0].addr.in, clock, mhartid, debug_unit,param, debug_stream);
	uL2_data_unit_addr_path(cache_var, &bus2[1].data_read.out, &bus3->addr.out, &bus2[1].addr.in, mhartid, clock, debug_unit, debug_unit_walk, param, debug_stream);
	// error: data write out is 1 clock, need to be held for 2 clocks. need to work through internal variable
	uL2_data_write_path_0(cache_var, &bus2[0].data_read.out, &bus2[0].data_write.in, mhartid, clock, debug_unit, debug_unit_walk, debug_unit_ext_snoop, param, debug_stream);// code has priority over data
	uL2_data_write_path_1(&bus2[1].data_read.out, &bus3->data_write.out, cache_var, &bus2[1].data_write.in, mhartid, clock, debug_unit, debug_unit_walk, debug_unit_ext_snoop, param, debug_stream);
	// 
	// L3 interface portion
	if (mhartid == 1)
		if (clock >= 0x1bdc)
			debug++;
	uL2_external_snoop_latch(cache_var, snoop_L2, mhartid, clock, debug_unit_ext_snoop, param, debug_stream);//bus 3 clock sync handled by sTLB
	if ((clock & 1) == 1) {

		uL2_bus3_data_write_out_interface(&bus3->data_write.out, cache_var, mhartid, clock, debug_unit, debug_stream);
		// need to fix, bus 2 should be driven only by internal variables
		uL2_bus3_data_read_in_interface(&bus2[0].data_read.out, &bus2[1].data_read.out, cache_var, &bus3->data_read.in, mhartid, clock, debug_unit, debug_stream);
	}
}