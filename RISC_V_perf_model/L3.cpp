// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "cache.h"

void fifo_input(addr_bus_type* fifo, addr_bus_type* data, UINT64 clock, UINT bank, FILE* debug_stream) {
	int hit = 0;
	for (UINT i = 0; i < (4*core_count) && hit == 0; i++) {
		if (fifo[i].strobe == 0) {
			copy_addr_bus_info(&fifo[i], data, clock);
			hit = 1;
		}
	}
	if (hit == 0) {
		fprintf(debug_stream, "L3 bank(%d) xaction id 0x%04x, ERROR: address fifo over-run //  clock: 0x%04llx\n",
			bank, data->xaction_id, clock);
	}
}
void fifo_input(data_bus_type* fifo, data_bus_type* data,  UINT64 clock, char* message, FILE* debug_stream) {
	int hit = 0;
	for (UINT i = 0; i < core_count && hit == 0; i++) {
		if (fifo[i].snoop_response == snoop_idle) {
			copy_data_bus_info(&fifo[i], data);
			hit = 1;
		}
	}
	if (hit == 0) {
		fprintf(debug_stream, "L3 xaction id 0x%04x, ERROR: data fifo over-run; %s//  clock: 0x%04llx\n",
			data->xaction_id, message, clock);
	}
}
void L3_bank_addr_path(banked_cache_type* L3_cache, UINT current, UINT64 clock, UINT mhartid, UINT debug_unit, param_type* param, FILE* debug_stream) {
	int debug = 0;
	if (clock == 0x00a8)
		debug++;

	cache_addr_linked_list_type* addr_tracker = L3_cache->bus3_tracker[mhartid].list;

	if (addr_tracker[current].latch.strobe == 1) {// data strobe
		UINT8 hit = 0;
		int bank = (addr_tracker[current].latch.addr >> (L3_cache->bank[0].set_shift + 11)) & (core_count - 1);
		L2_block_type* L2_block = &L3_cache->bank[bank];
		UINT set = (addr_tracker[current].latch.addr >> L2_block->set_shift) & 0x000007ff;
		INT64 tag = addr_tracker[current].latch.addr >> L2_block->tag_shift;
		L2_block->in_use = 1;
		if (addr_tracker[current].latch.cacheable == page_non_cache) {
			fifo_input(L3_cache->mem_addr_out, &addr_tracker[current].latch, clock, bank, debug_stream);
			addr_tracker[current].addr_sent = 1;
			addr_tracker[current].status = link_issued_forward;
			if (debug_unit) {
				fprintf(debug_stream, "L3 addr 0x%016I64x xaction id %#06x q_id:%#4x, ", addr_tracker[current].latch.addr, addr_tracker[current].latch.xaction_id, current);
				fprintf(debug_stream, "non_cacheable sent to mem bus fifo: clock: 0x%04llx\n", clock);
			}
			addr_tracker[current].latch.clock = clock;
		}
		else {
			snoop_response_type snoop_response = snoop_miss;
			if (L2_block->tags[set].in_use) {
				copy_addr_bus_info(&addr_tracker[current].latch, &addr_tracker[current].latch, clock);
				addr_tracker[current].addr_sent = 0;
				if (debug_unit && addr_tracker[current].status != link_hold) {
					fprintf(debug_stream, "L3 bank(%d) set.way 0x%03x.%d addr 0x%016I64x xaction id 0x%04x q_id(%d): 0x%02x, ",
						bank, set, L2_block->tags[set].way_ptr, addr_tracker[current].latch.addr, addr_tracker[current].latch.xaction_id, mhartid, current);
					fprintf(debug_stream, "address from L2 to hold, set in use; clock: 0x%04llx\n", clock);
				}
				addr_tracker[current].status = link_hold;
			}
			else {
				for (UINT8 way = 0; way < 8 && snoop_response == snoop_miss; way++) {
					if (L2_block->tags[set].tag[way] == tag && L2_block->tags[set].state[way] != invalid_line) {
						switch (L2_block->tags[set].state[way]) {
						case shared_line: // return data - no snooping req
							if (addr_tracker[current].latch.xaction == bus_allocate) { // need to invalidate internal caches
								L2_block->tags[set].state[way] = exclusive_line;
								addr_tracker[current].latch.clock = clock;
								addr_tracker[current].addr_sent = 0;
								addr_tracker[current].status = link_waiting_snoop_response; // state for outgoing bus free
								addr_tracker[current].snoop_complete = 0;
								if (debug_unit) {
									fprintf(debug_stream, "L3 bank(%d) set.way 0x%03x.%d addr 0x%016I64x xaction id %#06x q_id(%d): %#4x, EXCLUSIVE queued to snoop other cores// clock: 0x%04llx\n",
										bank, set, L2_block->tags[set].way_ptr, addr_tracker[current].latch.addr, addr_tracker[current].latch.xaction_id, mhartid, current, clock);
								}
								snoop_response = snoop_hit;
								L2_block->tags[set].in_use = 1; // out of use once full snoop is complete
							}
							else {
								if (L2_block->array.read_write) {// write mode
									UINT hit = 0;
									if (L2_block->array.select[0] != 0)
										hit = 1;
									if (L2_block->array.select[1] != 0)
										hit = 1;
									if (L2_block->array.data[0].snoop_response != snoop_idle)// active clock
										hit = 1;
									if (L2_block->array.data[1].snoop_response != snoop_idle)// buffer turn around clock
										hit = 1;
									if (hit) {
										snoop_response = snoop_stall;
									}
									else {
										L2_block->array.read_write = 0;
										L2_block->tags[set].way_ptr = way;
										L2_block->array.select[0] = 0x8000 | (way << 12) | set; // initiate data fetch
										L2_block->array.xaction_id_in[0] = addr_tracker[current].latch.xaction_id;
										L2_block->array.snoop_response[0] = snoop_hit;
										snoop_response = snoop_hit;
										if (debug_unit) {
											fprintf(debug_stream, "L3 bank(%d) set.way 0x%03x.%d addr ", bank, set, L2_block->tags[set].way_ptr);
											fprint_addr_coma(debug_stream, addr_tracker[current].latch.addr, param);
											fprintf(debug_stream, "xaction id %#06x q_id(%d): %#4x, shared line, return data to core, do not snoop other cores 0x%016I64x clock: 0x%04llx\n",
												addr_tracker[current].latch.xaction_id, mhartid, current, L2_block->array.line[way][set][(addr_tracker[current].latch.addr >> 3) & 0x0f], clock);
										}
										addr_tracker[current].status = link_retire;
									}
								}
								else {// read mode
									if (L2_block->array.select[0] != 0)
										snoop_response = snoop_stall;
									else {
										L2_block->tags[set].way_ptr = way;
										// initiate data fetch
										L2_block->array.select[0] = 0x8000 | (way << 12) | set; // strobe, way, set, direction(0x0800 write, 0x0000 read)
										L2_block->array.xaction_id_in[0] = addr_tracker[current].latch.xaction_id;
										L2_block->array.snoop_response[0] = snoop_hit;
										snoop_response = snoop_hit;
										if (debug_unit) {
											fprintf(debug_stream, "L3 bank(%d) set.way 0x%03x.%d xaction id %#06x addr ", 
												bank, set, L2_block->tags[set].way_ptr, addr_tracker[current].latch.xaction_id);
											fprint_addr_coma(debug_stream, addr_tracker[current].latch.addr, param);
											fprintf(debug_stream, "q_id(%d): %#4x, shared line, return data to core, do not snoop other cores// clock: 0x%04llx\n",
												 mhartid, current, clock);
											fprintf(debug_stream, "L3 bank(%d) set.way 0x%03x.%d xaction id %#06x ", 
												bank, set, L2_block->tags[set].way_ptr, addr_tracker[current].latch.xaction_id);
											fprintf(debug_stream, "0x00 0x%016I64x 0x08 0x%016I64x 0x10 0x%016I64x 0x18 0x%016I64x clock: 0x%04llx\n",
												L2_block->array.line[way][set][0], L2_block->array.line[way][set][1], L2_block->array.line[way][set][2], L2_block->array.line[way][set][3], clock);
											fprintf(debug_stream, "L3 bank(%d) set.way 0x%03x.%d xaction id %#06x ", 
												bank, set, L2_block->tags[set].way_ptr, addr_tracker[current].latch.xaction_id);
											fprintf(debug_stream, "0x20 0x%016I64x 0x28 0x%016I64x 0x30 0x%016I64x 0x38 0x%016I64x clock: 0x%04llx\n",
												L2_block->array.line[way][set][4], L2_block->array.line[way][set][5], L2_block->array.line[way][set][6], L2_block->array.line[way][set][6], clock);
											fprintf(debug_stream, "L3 bank(%d) set.way 0x%03x.%d xaction id %#06x ", 
												bank, set, L2_block->tags[set].way_ptr, addr_tracker[current].latch.xaction_id);
											fprintf(debug_stream, "0x40 0x%016I64x 0x48 0x%016I64x 0x50 0x%016I64x 0x58 0x%016I64x clock: 0x%04llx\n",
												L2_block->array.line[way][set][8], L2_block->array.line[way][set][9], L2_block->array.line[way][set][10], L2_block->array.line[way][set][11], clock);
											fprintf(debug_stream, "L3 bank(%d) set.way 0x%03x.%d xaction id %#06x ", 
												bank, set, L2_block->tags[set].way_ptr, addr_tracker[current].latch.xaction_id);
											fprintf(debug_stream, "0x60 0x%016I64x 0x68 0x%016I64x 0x70 0x%016I64x 0x78 0x%016I64x clock: 0x%04llx\n",
												L2_block->array.line[way][set][12], L2_block->array.line[way][set][13], L2_block->array.line[way][set][14], L2_block->array.line[way][set][15], clock);
										}
										addr_tracker[current].status = link_retire;
									}
								}
							}
							break;
						case modified_line: // snooping required - invalidate caches
						case exclusive_line:
							snoop_response = (L2_block->tags[set].state[way] == modified_line) ? snoop_dirty : snoop_hit;
							addr_tracker[current].latch.clock = clock;
							addr_tracker[current].addr_sent = 0;
							addr_tracker[current].status = link_waiting_snoop_response; // state for outgoing bus free
							addr_tracker[current].snoop_complete = 0;
							if (debug_unit) {
								fprintf(debug_stream, "L3 bank(%d) set.way 0x%03x.%d addr 0x%016I64x xaction id %#06x q_id(%d): 0x%02x 0x%02x-0x%02x, EXCLUSIVE/MODIFIED queued to snoop other cores// clock: 0x%04llx\n",
									bank, set, L2_block->tags[set].way_ptr, addr_tracker[current].latch.addr, addr_tracker[current].latch.xaction_id, mhartid,
									current, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, clock);
							}
							L2_block->tags[set].in_use = 1; // out of use once full snoop is complete
							break;
						default:
							debug++; // logical error
							break;
						}
					}
				}
				if (snoop_response == snoop_miss) { // not inclusive cache, must snoop cores
					addr_tracker[current].latch.clock = clock;
					addr_tracker[current].addr_sent = 0;
					addr_tracker[current].status = link_waiting_snoop_response; // state for outgoing bus free
					addr_tracker[current].snoop_complete = 0;
					if (debug_unit) {
						fprintf(debug_stream, "L3 bank(%d) set.way 0x%03x.%d addr 0x%016I64x xaction id %#06x q_id(%d): 0x%02x 0x%02x-0x%02x, MISS queued to snoop other cores// clock: 0x%04llx\n",
							bank, set, L2_block->tags[set].way_ptr, addr_tracker[current].latch.addr, addr_tracker[current].latch.xaction_id, mhartid,
							current, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, clock);
					}
					L2_block->tags[set].in_use = 1; // out of use once full snoop is complete
				}
			}
		}
	}
}

void copy_addr_info(addr_bus_type* dest, addr_bus_type* src) {
	dest->strobe = src->strobe;
	dest->addr = src->addr;
	dest->cacheable = src->cacheable;
	dest->xaction = src->xaction;
	dest->xaction_id = src->xaction_id;
	dest->clock = src->clock;
}
void write_data_to_L3(banked_cache_type* L3_cache, UINT bus_id, UINT64 clock, UINT debug_unit, FILE* debug_stream) {
	int debug = 0;
	if (L3_cache->data_write_fifo[bus_id][0].snoop_response != snoop_idle && L3_cache->data_write_fifo[bus_id][0].snoop_response != snoop_stall) {
		UINT hit = 0;
		UINT mhartid = L3_cache->data_write_fifo[bus_id][0].xaction_id & 0x0f;
		if (clock == 0x00fc)
			debug++;
		for (UINT current_addr = L3_cache->bus3_tracker[mhartid].start; current_addr != L3_cache->bus3_tracker[mhartid].stop && !hit; current_addr =((current_addr +1)& L3_cache->bus3_tracker[mhartid].round_count)) {
			if (L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction_id == L3_cache->data_write_fifo[bus_id][0].xaction_id &&
				L3_cache->bus3_tracker[mhartid].list[current_addr].status != link_free && L3_cache->bus3_tracker[mhartid].list[current_addr].status != link_retire) {

				if ((L3_cache->data_write_fifo[bus_id][0].xaction_id & (core_count - 1)) != bus_id) {
					L3_cache->bus3_tracker[mhartid].list[current_addr].snoop_complete |= (1 << bus_id);
					L3_cache->bus3_tracker[mhartid].list[current_addr].snoop_complete |= (1 << (L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction_id&0x0f));
					hit = 1;
					int bank = (L3_cache->bus3_tracker[mhartid].list[current_addr].latch.addr >> (L3_cache->bank[0].set_shift + 11)) & (core_count - 1);
					UINT set = (L3_cache->bus3_tracker[mhartid].list[current_addr].latch.addr >> L3_cache->bank[0].set_shift) & 0x000007ff;
					if (L3_cache->data_write_fifo[bus_id][0].snoop_response == snoop_hit || L3_cache->data_write_fifo[bus_id][0].snoop_response == snoop_dirty) {
						if (L3_cache->data_write_fifo[bus_id][0].cacheable == page_non_cache)
							debug++;
						char header[0x100];
						sprintf_s(header, "L3 bank(%d)", bank);
						if (write_to_array(&L3_cache->bank[bank], L3_cache->bus3_tracker[mhartid].list[current_addr].latch.addr,
							(L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction == bus_allocate) ? 1 : 0, &L3_cache->data_write_fifo[bus_id][0], clock, mhartid, debug_unit, header, debug_stream)) {
							L3_cache->bank[bank].tags[set].in_use = 1; // writing to tag array
						}
						else {
							debug++;
						}
						if (L3_cache->data_write_fifo[bus_id][0].snoop_response == snoop_dirty || L3_cache->bus3_tracker[mhartid].list[current_addr].data.snoop_response != snoop_dirty)
							copy_data_bus_info(&L3_cache->bus3_tracker[mhartid].list[current_addr].data, &L3_cache->data_write_fifo[bus_id][0]);
						if (L3_cache->data_write_fifo[bus_id][0].snoop_response == snoop_dirty && L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction != bus_allocate &&
							L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction != bus_LR_aq_rl && L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction != bus_SC_aq_rl)
							debug++; // need to flush internal caches
					}
					if (L3_cache->bus3_tracker[mhartid].list[current_addr].snoop_complete == ((1<<core_count)-1)) {
						if (L3_cache->bus3_tracker[mhartid].list[current_addr].status == link_fill_waiting_snoop_response) {
							L3_cache->bus3_tracker[mhartid].list[current_addr].status = link_free; // continue to array lookup and DRAM
							if (debug_unit) {
								fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, External Snoop Complete, end cycle; xaction id %#06x, addr: 0x%016I64x, clock: 0x%04llx\n",
									mhartid, current_addr, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, L3_cache->data_write_fifo[bus_id][0].xaction_id, L3_cache->bus3_tracker[mhartid].list[current_addr].latch.addr, clock);
							}
						}
						else if (L3_cache->bus3_tracker[mhartid].list[current_addr].data.snoop_response != snoop_idle) {
							char message[0x100];
							sprintf_s(message, "bus3(%d) read data return", mhartid);
							fifo_input(L3_cache->data_read_fifo[mhartid], &L3_cache->bus3_tracker[mhartid].list[current_addr].data, clock, message, debug_stream);
							L3_cache->bus3_tracker[mhartid].list[current_addr].status = link_free;
							L3_cache->bus3_tracker[mhartid].list[current_addr].data.snoop_response = snoop_idle;
							L3_cache->bank[bank].tags[set].in_use = 0;
							if (debug_unit) {
								fprintf(debug_stream, "L3 bank(%d) set.way 0x%03x.%d xaction id %#06x, q_id(%d): 0x%02x 0x%02x-0x%02x, External Snoop Complete (HIT), return data, set released; addr,data 0x%016I64x,0x%016I64x clock: 0x%04llx\n",
									bank, set, L3_cache->bank[bank].tags[set].way_ptr, L3_cache->data_write_fifo[bus_id][0].xaction_id, mhartid, current_addr, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop,
									L3_cache->bus3_tracker[mhartid].list[current_addr].latch.addr,
									L3_cache->bus3_tracker[mhartid].list[current_addr].data.data[(L3_cache->bus3_tracker[mhartid].list[current_addr].latch.addr >> 7) & 0x0f], clock);
							}
						}
						else {
							fifo_input(L3_cache->mem_addr_out, &L3_cache->bus3_tracker[mhartid].list[current_addr].latch, clock, bank, debug_stream);
							L3_cache->bus3_tracker[mhartid].list[current_addr].status = link_issued_forward;
							L3_cache->bus3_tracker[mhartid].list[current_addr].addr_sent = 1;
							if (debug_unit) {
								fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, External Snoop Complete; forward to memory xaction id %#06x, addr: 0x%016I64x, clock: 0x%04llx\n",
									mhartid, current_addr, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, L3_cache->data_write_fifo[bus_id][0].xaction_id, L3_cache->bus3_tracker[mhartid].list[current_addr].latch.addr, clock);
							}
						}
						L3_cache->bus3_tracker[mhartid].list[current_addr].snoop_complete = 0;
						for (UINT i = 0; i < core_count; i++)	L3_cache->snoop_active[i] = 0;
					}
					else {
						switch (L3_cache->data_write_fifo[bus_id][0].snoop_response) {
						case snoop_hit:
							copy_data_bus_info(&L3_cache->bus3_tracker[mhartid].list[current_addr].data, &L3_cache->data_write_fifo[bus_id][0]);
							if (debug_unit) {
								fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, External Snoop HIT Incomplete: %#06x; xaction id %#06x, addr: 0x%016I64x, clock: 0x%04llx\n",
									mhartid, current_addr, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, L3_cache->bus3_tracker[mhartid].list[current_addr].snoop_complete, L3_cache->data_write_fifo[bus_id][0].xaction_id, L3_cache->bus3_tracker[mhartid].list[current_addr].latch.addr, clock);
							}
							break;
						case snoop_dirty:
							copy_data_bus_info(&L3_cache->bus3_tracker[mhartid].list[current_addr].data, &L3_cache->data_write_fifo[bus_id][0]);
							if (debug_unit) {
								fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, External Snoop DIRTY Incomplete: %#06x; xaction id %#06x, addr: 0x%016I64x, clock: 0x%04llx\n",
									mhartid, current_addr, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, L3_cache->bus3_tracker[mhartid].list[current_addr].snoop_complete, L3_cache->data_write_fifo[bus_id][0].xaction_id, L3_cache->bus3_tracker[mhartid].list[current_addr].latch.addr, clock);
							}
							break;
						case snoop_miss:
							if (debug_unit) {
								fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, External Snoop MISS Incomplete: %#06x; bus 0x%01x, xaction id %#06x, addr: 0x%016I64x, clock: 0x%04llx\n",
									mhartid, current_addr, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, L3_cache->bus3_tracker[mhartid].list[current_addr].snoop_complete, bus_id, L3_cache->data_write_fifo[bus_id][0].xaction_id, L3_cache->bus3_tracker[mhartid].list[current_addr].latch.addr, clock);
							}
							break;
						default:
							if (debug_unit) {
								fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, External Snoop Incomplete: %#06x; xaction id %#06x, addr: 0x%016I64x, clock: 0x%04llx\n",
									mhartid, current_addr, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, L3_cache->bus3_tracker[mhartid].list[current_addr].snoop_complete, L3_cache->data_write_fifo[bus_id][0].xaction_id, L3_cache->bus3_tracker[mhartid].list[current_addr].latch.addr, clock);
							}
							break;
						}
						if (L3_cache->data_write_fifo[bus_id][0].snoop_response == snoop_dirty && L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction != bus_allocate) {
							L3_cache->data_write_fifo[bus_id][0].snoop_response == snoop_hit;// do not make exclusive on lower caches without an allocate cycle
						}
					}
				}
				else if (L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction == bus_fetch) {
					hit = 1;
				}
				else if (L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction == bus_store_full || L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction == bus_store_partial) {

					for (UINT current = L3_cache->bus3_tracker[mhartid].start; current != L3_cache->bus3_tracker[mhartid].stop && !hit && L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction == bus_store_full; current = ((current + 1) & (L3_cache->bus3_tracker[mhartid].round_count))) {
						cache_addr_linked_list_type* alloc_addr = &L3_cache->bus3_tracker[mhartid].list[current];
						int bank = (alloc_addr->latch.addr >> (L3_cache->bank[0].set_shift + 11)) & (core_count - 1);
						int set = (alloc_addr->latch.addr >> L3_cache->bank[0].set_shift) & 0x000007ff;
						char header[0x100];
						sprintf_s(header, "L3 bank(%d)", bank);
						if (alloc_addr->latch.xaction_id == (L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction_id & 0xfeff) && (alloc_addr->latch.xaction == bus_allocate)) {
							if (alloc_addr->status == link_hold_C) {
								if (write_to_array(&L3_cache->bank[bank], alloc_addr->latch.addr, (alloc_addr->latch.xaction == bus_allocate) ? 1 : 0, &L3_cache->data_write_fifo[bus_id][0], clock, mhartid, debug_unit, header, debug_stream)) {
									alloc_addr->status = link_free;
								}
								else {
									debug++;
								}
							}
							else if (alloc_addr->status == link_waiting_snoop_response) {
								// need l2/l3 switch for debug stream
								// need bank for debug stream
								if (write_to_array(&L3_cache->bank[bank], alloc_addr->latch.addr, (alloc_addr->latch.xaction == bus_allocate) ? 1 : 0, &L3_cache->data_write_fifo[bus_id][0], clock, mhartid, debug_unit, header, debug_stream)) {
									alloc_addr->status = link_free;
								}
								else {
									alloc_addr->status = link_fill_waiting_snoop_response;
								}
								L3_cache->bus3_tracker[mhartid].list[current_addr].status = link_free;
							}
							else if ((alloc_addr->status == link_issued_forward)) {
								for (UINT8 way = 0; way < 8 && !hit; way++) {
									if (write_to_array(&L3_cache->bank[bank], alloc_addr->latch.addr, (alloc_addr->latch.xaction == bus_allocate) ? 1 : 0, &L3_cache->data_write_fifo[bus_id][0], clock, mhartid, debug_unit, header, debug_stream)) {
										alloc_addr->status = link_issued_forward_stop;
									}
									else {
										debug++;
									}
								}
							}
							else if (alloc_addr->status == link_free) {
								// ignore;
							}
							else {
								debug++;
							}
						}
						if (!hit && L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction == bus_store_full) {
							UINT8 way = L3_cache->bank[bank].tags[set].way_ptr;
							if (L3_cache->bank[bank].tags[set].state[way] == modified_line)
								debug++;
							hit = 1;
							if (write_to_array(&L3_cache->bank[bank], alloc_addr->latch.addr, (alloc_addr->latch.xaction == bus_allocate) ? 1 : 0, &L3_cache->data_write_fifo[bus_id][0], clock, mhartid, debug_unit, header, debug_stream)) {
								L3_cache->bus3_tracker[mhartid].list[current_addr].status = link_free;
								cache_addr_linked_list_type* alloc_addr = &L3_cache->bus3_tracker[mhartid].list[current];
								alloc_addr->status = link_issued_forward_stop;
							}
							else {
								debug++;
							}
						}
					}
					if (!hit) {
						if (L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction == bus_store_partial) {
							if (L3_cache->bus3_tracker[mhartid].list[current_addr].status == link_hold) {
								copy_data_bus_info(&L3_cache->data_write_list[L3_cache->data_write_end_ptr].latch, &L3_cache->data_write_fifo[bus_id][0]);
								L3_cache->data_write_list[L3_cache->data_write_end_ptr].status = link_hold;

								L3_cache->data_write_end_ptr = (L3_cache->data_write_end_ptr + 1) & (L3_cache->data_write_count - 1);
								if (debug_unit) {
									fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, bus data write to HOLD; xaction id %#06x, valid: 0x%016I64x, clock: 0x%04llx\n",
										mhartid, current_addr, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, L3_cache->data_write_fifo[bus_id][0].xaction_id, L3_cache->data_write_fifo[bus_id][0].valid, clock);
								}
								L3_cache->bus3_tracker[mhartid].list[current_addr].status = link_hold_w;
							}
							else {
								L3_cache->bus3_tracker[mhartid].list[current_addr].status = link_free;
								char message[0x100];
								sprintf_s(message, "mem_bus write data forward");
								fifo_input(L3_cache->mem_data_write_fifo, &L3_cache->data_write_fifo[bus_id][0], clock, message, debug_stream);
								if (debug_unit) {
									fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, bus data write to DRAM ctrl; xaction id %#06x, valid: 0x%016I64x, clock: 0x%04llx\n",
										mhartid, current_addr, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, L3_cache->data_write_fifo[bus_id][0].xaction_id, L3_cache->data_write_fifo[bus_id][0].valid, clock);
								}
							}
						}
						else {
							debug++; // coding error, shouldn't reach this state
						}
					}
					hit = 1;
				}
			}
		}
		if (debug_unit) {
			if (!hit && bus_id == (L3_cache->data_write_fifo[bus_id][0].xaction_id & 0x0f))
				fprintf(debug_stream, "L3: ERROR: data response without tracked address issued, xaction id %#06x, valid: 0x%016I64x, clock: 0x%04llx\n", L3_cache->data_write_fifo[bus_id][0].xaction_id, L3_cache->data_write_fifo[bus_id][0].valid, clock);
		}
		for (UINT i = 0; i < (core_count - 1); i++) {
			copy_data_bus_info(&L3_cache->data_write_fifo[bus_id][i], &L3_cache->data_write_fifo[bus_id][i + 1]);
		}
		L3_cache->data_write_fifo[bus_id][(core_count - 1)].snoop_response = snoop_idle;
	}
}
void L3_queue_maintainence(banked_cache_type* L3_cache) {

	for (UINT bank = 0; bank < core_count; bank++) {
		L3_cache->bank[bank].in_use = 0;
		UINT8 stop = 0;
		while (!stop && L3_cache->data_read_start_ptr != L3_cache->data_read_end_ptr) {
			if (L3_cache->data_read_list[L3_cache->data_read_start_ptr].status == link_free)
				L3_cache->data_read_start_ptr = ((L3_cache->data_read_start_ptr + 1) & 0x7f);
			else
				stop = 1;
		}
	}

	for (UINT mhartid = 0; mhartid < core_count; mhartid++) {
		while ((L3_cache->bus3_tracker[mhartid].list[L3_cache->bus3_tracker[mhartid].start].status == link_free ||
			L3_cache->bus3_tracker[mhartid].list[L3_cache->bus3_tracker[mhartid].start].status == link_retire) &&
			L3_cache->bus3_tracker[mhartid].start != L3_cache->bus3_tracker[mhartid].stop)
			L3_cache->bus3_tracker[mhartid].start = ((L3_cache->bus3_tracker[mhartid].start + 1) & (L3_cache->bus3_tracker[mhartid].round_count));
	}
	while (L3_cache->data_write_list[L3_cache->data_write_start_ptr].status == link_free && L3_cache->data_write_start_ptr != L3_cache->data_write_end_ptr)  L3_cache->data_write_start_ptr = ((L3_cache->data_write_start_ptr + 1) & (L3_cache->data_write_count - 1));
}
void snoop_cores(bus_w_snoop_signal1* bus3, banked_cache_type* L3_cache, UINT64 Bclock, int debug_unit, FILE* debug_stream) {
	int debug = 0;
	int snoop_addr_issued = 0;
	UINT16 Ext_snoop_stall = 0;
	for (UINT i = 0; i < core_count; i++) {
		if (bus3[i].data_write.in.snoop_response == snoop_stall) {
			Ext_snoop_stall |= (1 << i);
		}
	}
	while (L3_cache->data_read_list[L3_cache->data_read_start_ptr].status == link_free && L3_cache->data_read_start_ptr != L3_cache->data_read_end_ptr) L3_cache->data_read_start_ptr = ((L3_cache->data_read_start_ptr + 1) & (L3_cache->data_read_count - 1));
	if (Bclock == 0x00d6)
		debug++;
	// need to rotate bank for priority
	for (UINT mhartid = ((L3_cache->active_core + 1) & (core_count-1)), count = 0; count < core_count && !snoop_addr_issued; mhartid = ((mhartid + 1) & (core_count - 1)), count++) {
		UINT fence = 0;
		for (UINT current = L3_cache->bus3_tracker[mhartid].start; current != L3_cache->bus3_tracker[mhartid].stop && !snoop_addr_issued && !fence; current = ((current + 1) & (L3_cache->bus3_tracker[mhartid].round_count))) {
			if (current != L3_cache->bus3_tracker[mhartid].start && (L3_cache->bus3_tracker[mhartid].list[current].latch.xaction == bus_LR_rl || L3_cache->bus3_tracker[mhartid].list[current].latch.xaction == bus_LR_aq || L3_cache->bus3_tracker[mhartid].list[current].latch.xaction == bus_LR_aq_rl)) {
				fence = 1; // lock has to wait for previous transaction to dequeue and not allow any other to pass (evaluated individualy per core)
			}
			else if (L3_cache->bus3_tracker[mhartid].list[current].status == link_waiting_snoop_response && L3_cache->bus3_tracker[mhartid].list[current].snoop_complete != ((1 << core_count) - 1)) {
				if (L3_cache->snoop_active[L3_cache->bus3_tracker[mhartid].list[current].latch.xaction_id & (core_count - 1)] == 0 && L3_cache->bus3_tracker[mhartid].list[current].snoop_complete == 0) {
					if (L3_cache->snoop_active[mhartid] == 0) {
						if (!Ext_snoop_stall != 0) {
							snoop_addr_issued = 1; // 1 snoop per clock
							L3_cache->active_core = mhartid;
							for (UINT j = 0; j < core_count; j++) {
								if (j != mhartid) {
									copy_addr_bus_info(&bus3[j].snoop_addr.out, &L3_cache->bus3_tracker[mhartid].list[current].latch, 2 * Bclock);
								}
							}
							L3_cache->bus3_tracker[mhartid].list[current].snoop_complete = (1 << mhartid);
							L3_cache->snoop_active[mhartid] = 1;
							if (debug_unit) {
								fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, ", mhartid, current, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop);
								print_xaction2(L3_cache->bus3_tracker[mhartid].list[current].latch.xaction, debug_stream);
								fprintf(debug_stream, " latch: External Snoop started address: 0x%016I64x, xaction id %#06x, Bclock: 0x%04llx, clock: 0x%04llx\n", L3_cache->bus3_tracker[mhartid].list[current].latch.addr, L3_cache->bus3_tracker[mhartid].list[current].latch.xaction_id, Bclock, 2 * Bclock);
							}
						}
						else {
							if (debug_unit) {
								fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, ", mhartid, current, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop);
								print_xaction2(L3_cache->bus3_tracker[mhartid].list[current].latch.xaction, debug_stream);
								fprintf(debug_stream, " latch: External Snoop stalled (0x%04x) address: 0x%016I64x, xaction id %#06x, Bclock: 0x%04llx, clock: 0x%04llx\n",
									Ext_snoop_stall, L3_cache->bus3_tracker[mhartid].list[current].latch.addr, L3_cache->bus3_tracker[mhartid].list[current].latch.xaction_id, Bclock, 2 * Bclock);
							}
						}
					}
				}
				fence = 1; // only issue 1 snoop per core at 1 time, block later issues
			}
		}
	}// data read portion below
}
void snoop_response_to_cores(bus_w_snoop_signal1* bus3, banked_cache_type* L3_cache, UINT64 Bclock, int debug_unit, FILE* debug_stream) {
	for (UINT current = L3_cache->data_read_start_ptr; current != L3_cache->data_read_end_ptr; current = ((current + 1) & (L3_cache->data_read_count - 1))) {
		int mhartid = L3_cache->data_read_list[current].latch.xaction_id & (core_count - 1);
		if ((bus3[mhartid].data_read.out.snoop_response == snoop_idle || bus3[mhartid].data_read.out.snoop_response == snoop_stall) && L3_cache->data_read_list[current].status != link_free) { // fifo, need to prioritize order (fetch, read, alloc, prefetch)
			copy_data_bus_info(&bus3[mhartid].data_read.out, &L3_cache->data_read_list[current].latch);
			L3_cache->data_read_list[current].status = link_free;
			if (debug_unit) {
				fprintf(debug_stream, "L3: rq_id(%d): 0x%02x 0x%02x-0x%02x, data read to L2(%d) xaction id %#06x, Bclock: 0x%04llx clock: 0x%04llx\n", mhartid, current, L3_cache->data_read_start_ptr, L3_cache->data_read_end_ptr, mhartid, L3_cache->data_read_list[current].latch.xaction_id, Bclock, 2 * Bclock);
			}
		}
	}
}
void latch_data_from_memory(banked_cache_type* L3_cache, bus_w_snoop_signal1* bus3, data_bus_type* mem_data_read, UINT64 clock, UINT64 Bclock, int debug_unit, FILE* debug_stream) {
	int debug = 0;
	if (mem_data_read->snoop_response == snoop_hit) {
		UINT hit = 0;
		UINT mhartid = mem_data_read->xaction_id & 0x0f;
		for (UINT current = L3_cache->bus3_tracker[mhartid].start; current != L3_cache->bus3_tracker[mhartid].stop && !hit; current = ((current + 1) & (L3_cache->bus3_tracker[mhartid].round_count))) {
			if (L3_cache->bus3_tracker[mhartid].list[current].latch.xaction_id == mem_data_read->xaction_id &&
				L3_cache->bus3_tracker[mhartid].list[current].status != link_free && L3_cache->bus3_tracker[mhartid].list[current].status != link_retire) {
				if (L3_cache->bus3_tracker[mhartid].list[current].status == link_issued_forward_stop) {
					L3_cache->bus3_tracker[mhartid].list[current].status = link_free;
					hit = 1;
					if (debug_unit) {
						fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, ", mhartid, current, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop);
					}
				}
				else {
					// add non cache - direct to L2, & q hit transfers
					if (L3_cache->bus3_tracker[mhartid].list[current].latch.cacheable == page_non_cache) {
						if (debug_unit) {
							fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, ", mhartid, current, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop);
						}
						int mhartid = mem_data_read->xaction_id & 0x0f;
						if (bus3[mhartid].data_read.out.snoop_response == snoop_idle) {
							char message[0x100];
							sprintf_s(message, "mem_bus read data to bus3(%d)", mhartid);
							fifo_input(L3_cache->data_read_fifo[mhartid], mem_data_read, clock, message, debug_stream);
							
							if (debug_unit) {
								fprintf(debug_stream, "UC, forward to core data read latch xaction id %#06x, Bclock: 0x%04llx clock: 0x%04llx\n",
									mem_data_read->xaction_id, Bclock, clock);
							}
							L3_cache->bus3_tracker[mhartid].list[current].status = link_free;
						}
						else {
							copy_data_bus_info(&L3_cache->data_read_list[L3_cache->data_read_end_ptr].latch, mem_data_read);
							L3_cache->data_read_list[L3_cache->data_read_end_ptr].xaction = L3_cache->bus3_tracker[mhartid].list[current].latch.xaction;// needed to sort returns (fetch, read, alloc, prefetch)
							L3_cache->data_read_list[L3_cache->data_read_end_ptr].status = link_hold;
							L3_cache->bus3_tracker[mhartid].list[current].status = link_free;
							L3_cache->data_read_end_ptr++;
							L3_cache->data_read_end_ptr &= (L3_cache->data_read_count - 1);
							if (debug_unit) {
								fprintf(debug_stream, "UC, bus busy, hold ");
								fprintf(debug_stream, "data read latch xaction id %#06x, Bclock: 0x%04llx clock: 0x%04llx\n", mem_data_read->xaction_id, Bclock, clock);
							}
						}
						hit = 1;
						for (UINT index = L3_cache->bus3_tracker[mhartid].start; index != L3_cache->bus3_tracker[mhartid].stop; index = ((index + 1) & (L3_cache->bus3_tracker[mhartid].round_count))) {
							if (L3_cache->bus3_tracker[mhartid].list[index].status == link_q_hit) {
								if ((L3_cache->bus3_tracker[mhartid].list[index].latch.addr & (~0x7f)) == (L3_cache->bus3_tracker[mhartid].list[current].latch.addr & (~0x7f))) {
									if (bus3[mhartid].data_read.out.snoop_response == snoop_idle) {
										copy_data_bus_info(&bus3[mhartid].data_read.out, mem_data_read);
										bus3[mhartid].data_read.out.xaction_id = L3_cache->bus3_tracker[mhartid].list[index].latch.xaction_id;
										for (UINT8 i = 0; i < 0x10; i++) bus3[mhartid].data_read.out.data[i] = mem_data_read->data[i];

										L3_cache->bus3_tracker[mhartid].list[index].status = link_free;
										if (debug_unit) {
											fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, data read q hit xaction id %#06x, Bclock: 0x%04llx clock: 0x%04llx\n",
												mhartid, current, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, bus3[mhartid].data_read.out.xaction_id, Bclock, clock);
										}
									}
									else {
										copy_data_bus_info(&L3_cache->data_read_list[L3_cache->data_read_end_ptr].latch, mem_data_read);
										L3_cache->data_read_list[L3_cache->data_read_end_ptr].xaction = L3_cache->bus3_tracker[mhartid].list[index].latch.xaction;// needed to sort returns (fetch, read, alloc, prefetch)
										L3_cache->data_read_list[L3_cache->data_read_end_ptr].status = link_hold;
										L3_cache->bus3_tracker[mhartid].list[index].status = link_free;
										L3_cache->data_read_end_ptr++;
										L3_cache->data_read_end_ptr &= (L3_cache->data_read_count - 1);
										if (debug_unit) {
											fprintf(debug_stream, "UC, bus busy, q hit hold ");
											fprintf(debug_stream, "data read latch xaction id %#06x, Bclock: 0x%04llx clock: 0x%04llx\n", L3_cache->bus3_tracker[mhartid].list[index].latch.xaction_id, Bclock, clock);
										}
									}
								}
							}
						}
					}
					else {
						int bank = (L3_cache->bus3_tracker[mhartid].list[current].latch.addr >> (L3_cache->bank[0].set_shift+11)) & (core_count - 1);// only 8 bank entries initialized
						L3_cache->bank[bank].in_use = 1; // writing to tag array
						int set = (L3_cache->bus3_tracker[mhartid].list[current].latch.addr >> L3_cache->bank[0].set_shift) & 0x000007ff;
						hit = 0;
						UINT mhartid = mem_data_read->xaction_id & (core_count - 1);
						char header[0x100];
						sprintf_s(header, "L3 bank(%d)", bank);
						UINT8 write_success = write_to_array(&L3_cache->bank[bank], L3_cache->bus3_tracker[mhartid].list[current].latch.addr, (L3_cache->bus3_tracker[mhartid].list[current].latch.xaction == bus_allocate) ? 1 : 0, mem_data_read, clock, mhartid, debug_unit, header, debug_stream);
						if (write_success == 0)
							debug++;
						if ((bus3[mhartid].data_read.out.snoop_response == snoop_idle || bus3[mhartid].data_read.out.snoop_response == snoop_stall) && write_success) {
							copy_data_bus_info(&bus3[mhartid].data_read.out, mem_data_read);

							// if bus3 is free, issue immediately, also check q_hit list and deal with each appropriately
							L3_cache->bus3_tracker[mhartid].list[current].status = link_free;
							L3_cache->snoop_active[mhartid] = 0;
							if (debug_unit) {
								fprintf(debug_stream, "L3 bank(%d) addr 0x%016x xaction id 0x%04x q_id(%d): 0x%02x 0x%02x-0x%02x; data read forward to L2 Bclock: 0x%04llx clock: 0x%04llx\n",
									bank, L3_cache->bus3_tracker[mhartid].list[current].latch.addr, L3_cache->bus3_tracker[mhartid].list[current].latch.xaction_id,
									mhartid, current, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, Bclock, clock);
							}
							for (UINT index = L3_cache->bus3_tracker[mhartid].start; index != L3_cache->bus3_tracker[mhartid].stop; index = ((index + 1) & (L3_cache->bus3_tracker[mhartid].round_count))) {
								if (L3_cache->bus3_tracker[mhartid].list[index].status == link_q_hit) {
									if ((L3_cache->bus3_tracker[mhartid].list[index].latch.addr & (~0x7f)) == (L3_cache->bus3_tracker[mhartid].list[current].latch.addr & (~0x7f))) {
										if (bus3[mhartid].data_read.out.snoop_response == snoop_idle) {
											copy_data_bus_info(&bus3[mhartid].data_read.out, mem_data_read);
											bus3[mhartid].data_read.out.xaction_id = L3_cache->bus3_tracker[mhartid].list[index].latch.xaction_id;
											for (UINT8 i = 0; i < 0x10; i++) bus3[mhartid].data_read.out.data[i] = mem_data_read->data[i];
											L3_cache->bus3_tracker[mhartid].list[index].status = link_free;
											if (debug_unit) {
												fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, data read q hit2 xaction id %#06x, Bclock: 0x%04llx clock: 0x%04llx\n",
													mhartid, index, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, bus3[mhartid].data_read.out.xaction_id, Bclock, clock);
											}
										}
										else {
											copy_data_bus_info(&L3_cache->data_read_list[L3_cache->data_read_end_ptr].latch, mem_data_read);
											L3_cache->data_read_list[L3_cache->data_read_end_ptr].xaction = L3_cache->bus3_tracker[mhartid].list[index].latch.xaction;// needed to sort returns (fetch, read, alloc, prefetch)
											L3_cache->data_read_list[L3_cache->data_read_end_ptr].status = link_hold;
											L3_cache->bus3_tracker[mhartid].list[index].status = link_free;
											L3_cache->data_read_end_ptr++;
											L3_cache->data_read_end_ptr &= (L3_cache->data_read_count - 1);
											if (debug_unit) {
												fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, ", mhartid, index, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop);
												fprintf(debug_stream, "bus busy, q hit hold2 data read latch xaction id %#06x, Bclock: 0x%04llx clock: 0x%04llx\n",
													L3_cache->bus3_tracker[mhartid].list[index].latch.xaction_id, Bclock, clock);
											}
										}
									}
								}
							}
						}
						else {
							copy_data_bus_info(&L3_cache->data_read_list[L3_cache->data_read_end_ptr].latch, mem_data_read);
							L3_cache->data_read_list[L3_cache->data_read_end_ptr].xaction = L3_cache->bus3_tracker[mhartid].list[current].latch.xaction;// needed to sort returns (fetch, read, alloc, prefetch)
							L3_cache->data_read_list[L3_cache->data_read_end_ptr].status = link_hold;
							L3_cache->bank[bank].tags[set].in_use = 0;// performance bug, need to place data in cache before unblocking set
							L3_cache->bus3_tracker[mhartid].list[current].status = link_free;
							L3_cache->data_read_end_ptr++;
							L3_cache->data_read_end_ptr &= (L3_cache->data_read_count - 1);
							if (debug_unit) {
								fprintf(debug_stream, "data read latch Bclock: 0x%04llx clock: 0x%04llx\n", Bclock, 2 * Bclock);
							}
							for (UINT index = L3_cache->bus3_tracker[mhartid].start; index != L3_cache->bus3_tracker[mhartid].stop; index = ((index + 1) & (L3_cache->bus3_tracker[mhartid].round_count))) {
								if (L3_cache->bus3_tracker[mhartid].list[index].status == link_q_hit) {
									if ((L3_cache->bus3_tracker[mhartid].list[index].latch.addr & (~0x7f)) == (L3_cache->bus3_tracker[mhartid].list[current].latch.addr & (~0x7f))) {
										if (debug_unit) {
											if (!(bus3[mhartid].data_read.out.snoop_response == snoop_idle || bus3[mhartid].data_read.out.snoop_response == snoop_stall))
												fprintf(debug_stream, "L3: rq_id(%d): 0x%02x 0x%02x-0x%02x, ERROR: bus collision, needs code fix(%d)  xaction id %#06x, Bclock: 0x%04llx clock: 0x%04llx\n", mhartid, index, L3_cache->data_read_start_ptr, L3_cache->data_read_end_ptr, mhartid, bus3[mhartid].data_read.out.xaction_id, Bclock, 2 * Bclock);
										}
										copy_data_bus_info(&bus3[mhartid].data_read.out, mem_data_read);
										L3_cache->bus3_tracker[mhartid].list[index].status = link_free;
										if (debug_unit) {
											fprintf(debug_stream, "L3: rq_id(%d): 0x%02x 0x%02x-0x%02x, q hit data read to L2(%d)  xaction id %#06x, Bclock: 0x%04llx clock: 0x%04llx\n", mhartid, index, L3_cache->data_read_start_ptr, L3_cache->data_read_end_ptr, mhartid, bus3[mhartid].data_read.out.xaction_id, Bclock, 2 * Bclock);
										}
									}
								}
							}
						}
						for (UINT q_index = L3_cache->bus3_tracker[mhartid].start; q_index != L3_cache->bus3_tracker[mhartid].stop && !hit; q_index = ((q_index + 1) & (L3_cache->bus3_tracker[mhartid].round_count))) {
							if ((L3_cache->bus3_tracker[mhartid].list[current].latch.addr & (~0x7f)) == (L3_cache->bus3_tracker[mhartid].list[q_index].latch.addr & (~0x7f)) && L3_cache->bus3_tracker[mhartid].list[q_index].status == link_q_hit) {
								if (bus3[mhartid].data_read.out.snoop_response == snoop_idle) {
									copy_data_bus_info(&bus3[mhartid].data_read.out, mem_data_read);
									bus3[mhartid].data_read.out.xaction_id = L3_cache->bus3_tracker[mhartid].list[q_index].latch.xaction_id;
									L3_cache->bus3_tracker[mhartid].list[q_index].status = link_free;
									if (debug_unit) {
										fprintf(debug_stream, "L3 unit: q_id(%d): 0x%02x, 0x%02x-0x%02x q hit forward to L2 xaction id %#06x, Bclock: 0x%04llx clock: 0x%04llx\n",
											mhartid, q_index, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, L3_cache->bus3_tracker[mhartid].list[q_index].latch.xaction_id, Bclock, 2 * Bclock);
									}
								}
								else {
									copy_data_bus_info(&L3_cache->data_read_list[L3_cache->data_read_end_ptr].latch, mem_data_read);
									L3_cache->data_read_list[L3_cache->data_read_end_ptr].xaction = L3_cache->bus3_tracker[mhartid].list[q_index].latch.xaction;// needed to sort returns (fetch, read, alloc, prefetch)
									L3_cache->data_read_list[L3_cache->data_read_end_ptr].status = link_hold;
									L3_cache->bus3_tracker[mhartid].list[q_index].status = link_free;
									L3_cache->data_read_end_ptr++;
									L3_cache->data_read_end_ptr &= (L3_cache->data_read_count - 1);
									if (debug_unit) {
										fprintf(debug_stream, "L3 unit: q_id(%d): 0x%02x, 0x%02x-0x%02x q hit bus busy, prep to L2 xaction id %#06x, Bclock: 0x%04llx clock: 0x%04llx\n",
											mhartid, q_index, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, L3_cache->bus3_tracker[mhartid].list[q_index].latch.xaction_id, Bclock, 2 * Bclock);
									}
								}
							}
						}
						hit = 1;
					}
				}
			} // need to throttle at L3 addr issue, or at DRAM end
		}
		if (!hit) {
			fprintf(debug_stream, "L3: ERROR: data read xaction id %#06x, Bclock: 0x%04llx clock: 0x%04llx\n", mem_data_read->xaction_id, Bclock, 2 * Bclock);
		}
	}
}
void data_write_to_memory(banked_cache_type* L3_cache, UINT64 clock, int debug_unit, FILE* debug_stream) {
	int debug = 0;
	for (UINT current_data = L3_cache->data_write_start_ptr; current_data != L3_cache->data_write_end_ptr; current_data = ((current_data + 1) & (L3_cache->data_write_count - 1))) {
		UINT mhartid = L3_cache->data_write_list[current_data].latch.xaction_id & 0x0f;
		if (L3_cache->data_write_list[current_data].status == link_hold) {
			for (UINT current_addr = L3_cache->bus3_tracker[mhartid].start; current_addr != L3_cache->bus3_tracker[mhartid].stop; current_addr++) {
				if (L3_cache->data_write_list[current_data].latch.xaction_id == L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction_id && L3_cache->bus3_tracker[mhartid].list[current_addr].status == link_issued_forward) {
					L3_cache->bus3_tracker[mhartid].list[current_addr].status = link_free;
					L3_cache->data_write_list[current_data].status = link_free;
					char message[0x100];
					sprintf_s(message, "mem_bus data write fifo");
					fifo_input(L3_cache->mem_data_write_fifo, &L3_cache->data_write_list[current_data].latch, clock, message, debug_stream);
					if (debug_unit) {
						fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, bus data write from hold to DRAM ctrl; xaction id %#06x, valid: %#010x, clock: 0x%04llx\n",
							mhartid, current_addr, L3_cache->bus3_tracker[mhartid].start, L3_cache->bus3_tracker[mhartid].stop, L3_cache->data_write_list[current_data].latch.xaction_id, L3_cache->data_write_list[current_data].latch.valid, clock);
					}
					UINT hit = 0;
					for (UINT current = L3_cache->bus3_tracker[mhartid].start; current != L3_cache->bus3_tracker[mhartid].stop && !hit; current = ((current + 1) & (L3_cache->bus3_tracker[mhartid].round_count))) {
						if ((L3_cache->bus3_tracker[mhartid].list[current].latch.xaction_id & 0x000f) == (L3_cache->bus3_tracker[mhartid].list[current_addr].latch.xaction_id & 0x000f) && (L3_cache->bus3_tracker[mhartid].list[current].latch.addr & ~0x007f) == (L3_cache->bus3_tracker[mhartid].list[current_addr].latch.addr & ~0x007f)) {
							// fill cache, if we can, kill memory transactions
							if (L3_cache->bus3_tracker[mhartid].list[current].latch.xaction == bus_allocate) {
								int bank = (L3_cache->bus3_tracker[mhartid].list[current].latch.addr >> (L3_cache->bank[0].set_shift+11)) & (core_count - 1);
								int set = (L3_cache->bus3_tracker[mhartid].list[current].latch.addr >> L3_cache->bank[0].set_shift) & 0x000007ff;
								char header[0x100];
								sprintf_s(header, "L3 bank(%d)", bank);
								if (write_to_array(&L3_cache->bank[bank], L3_cache->bus3_tracker[mhartid].list[current].latch.addr, (L3_cache->bus3_tracker[mhartid].list[current].latch.xaction == bus_allocate) ? 1 : 0, &L3_cache->data_write_list[current_data].latch, clock, mhartid, debug_unit, header, debug_stream)) {
									switch (L3_cache->bus3_tracker[mhartid].list[current].status) {
									case link_hold_C: {
										L3_cache->bus3_tracker[mhartid].list[current].status = link_free;
									}
													break;
									case link_waiting_snoop_response: {
										if (L3_cache->bus3_tracker[mhartid].list[current].snoop_complete == 0)
											L3_cache->bus3_tracker[mhartid].list[current].status = link_free;
										else
											L3_cache->bus3_tracker[mhartid].list[current].status = link_fill_waiting_snoop_response;
									}
																	break;
									case link_issued_forward: 	L3_cache->bus3_tracker[mhartid].list[current].status = link_free; break;
									case link_issued_forward_stop:
									case link_free:				break; // ignore
									default:
										debug++;
										break;
									}

								}
								else {
									debug++;
								}
							}
						}
					}
				}
			}
		}
	}
}
void bus3_address_latch(banked_cache_type* L3_cache, addr_bus_type* bus3_addr_in, UINT64 Bclock, UINT mhartid, int debug_unit, FILE* debug_stream) {
	int debug = 0;
	bus3_tracker_type* bus3_tracker = &L3_cache->bus3_tracker[mhartid];
	if (bus3_addr_in->strobe == 1) {// latch request - place in queue
		if (Bclock == 0x0444)
			debug++;
		cache_addr_linked_list_type* addr_tracker = bus3_tracker->list;
		UINT current = bus3_tracker->stop;
		addr_tracker[current].snoop_complete = 0;
		addr_tracker[current].latch.clock = Bclock;
		copy_addr_info(&addr_tracker[current].latch, bus3_addr_in);
		bus3_tracker->stop = ((bus3_tracker->stop + 1) & (bus3_tracker->round_count));
		if (((bus3_tracker->stop + 1) & (bus3_tracker->round_count)) == bus3_tracker->start)
			debug++;
		int bank = (bus3_addr_in->addr >> (L3_cache->bank[0].set_shift+11)) & (core_count - 1); // 1 clock look-up, 1 clock banck switch = 1 Bclock
		UINT set = (bus3_addr_in->addr >> L3_cache->bank[0].set_shift) & 0x000007ff;//11b
		if (bus3_tracker->stop == bus3_tracker->start) {
			fprintf(debug_stream, "L3 bank(%d) set.way 0x%03x.%01x addr 0x%016I64x, xaction id %#06x q_id(%d):%#4x %#4x-%#4x, ",
				bank, set, L3_cache->bank[bank].tags[set].way_ptr, bus3_addr_in->addr, bus3_addr_in->xaction_id, mhartid, current,
				bus3_tracker->start, bus3_tracker->stop);
			fprintf(debug_stream, "ERROR: tracker over-run, data lost; clock: 0x%04llx\n", Bclock);
		}
		if (bus3_addr_in->cacheable != page_non_cache && bus3_addr_in->xaction != bus_store_full && bus3_addr_in->xaction != bus_store_partial) {
			addr_tracker[current].addr_sent = 0;
			addr_tracker[current].status = link_hold;
			if (debug_unit) {
				fprintf(debug_stream, "L3 bank(%d) set.way 0x%03x.%01x addr 0x%016I64x, xaction id %#06x q_id(%d):0x%02x 0x%02x-0x%02x, ",
					bank, set, L3_cache->bank[bank].tags[set].way_ptr, bus3_addr_in->addr, bus3_addr_in->xaction_id, mhartid, current,
					bus3_tracker->start, bus3_tracker->stop);
				fprintf(debug_stream, "address from L2 latched to hold; Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
			}
		}
		else if (bus3_addr_in->xaction != bus_store_full && bus3_addr_in->xaction != bus_store_partial) {// writes need data before forwarding, and/or cache look-up
			fifo_input(L3_cache->mem_addr_out, bus3_addr_in, 2 * Bclock, bank, debug_stream);
			if (debug_unit) {
				fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, ", mhartid, current, bus3_tracker->start, bus3_tracker->stop);
				print_xaction2(bus3_addr_in->xaction, debug_stream);
				fprintf(debug_stream, " addr issued to memory addr out fifo: address: 0x%016I64x, xaction id %#06x, Bclock: 0x%04llx, clock: 0x%04llx\n",
					bus3_addr_in->addr, bus3_addr_in->xaction_id, Bclock, 2 * Bclock);
			}
			bus3_tracker->list[current].status = link_issued_forward1;
		}
		else {
			bus3_tracker->list[bus3_tracker->stop].status = link_hold;
			if (debug_unit) {
				fprintf(debug_stream, "L3: q_id(%d): 0x%02x 0x%02x-0x%02x, ", mhartid, current, bus3_tracker->start, bus3_tracker->stop);
				print_xaction2(bus3_addr_in->xaction, debug_stream);
				fprintf(debug_stream, " latch: to hold address: 0x%016I64x, xaction id %#06x, Bclock: 0x%04llx, clock: 0x%04llx\n", 
					bus3_addr_in->addr, bus3_addr_in->xaction_id, Bclock, 2 * Bclock);
			}
		}
	}
}
void delayed_data_read_to_memory(bus3_tracker_type* bus3_tracker, banked_cache_type* L3_cache, UINT64 clock, UINT mhartid, int debug_unit, FILE* debug_stream) {
	UINT debug = 0;
	for (UINT current_addr = bus3_tracker->start; current_addr != bus3_tracker->stop; current_addr= ((current_addr+1)& bus3_tracker->round_count)) {
		if (bus3_tracker->list[current_addr].status == link_hold_w) {
			if (bus3_tracker->list[current_addr].latch.xaction != bus_store_partial && bus3_tracker->list[current_addr].latch.xaction != bus_store_full)
				debug++;
			if (bus3_tracker->list[current_addr].latch.cacheable != page_non_cache) {
				for (UINT current_data = L3_cache->data_write_start_ptr; current_data != L3_cache->data_write_end_ptr; current_data++) {
					if (L3_cache->data_write_list[current_data].latch.xaction_id == bus3_tracker->list[current_addr].latch.xaction_id) {

						int bank = (bus3_tracker->list[current_addr].latch.addr >> (L3_cache->bank[0].set_shift+11)) & (core_count - 1);// only 8 bank entries initialized
						char header[0x100];
						sprintf_s(header, "L3 bank(%d)", bank);
						if (write_to_array(&L3_cache->bank[bank], bus3_tracker->list[current_addr].latch.addr, (bus3_tracker->list[current_addr].latch.xaction == bus_allocate) ? 1 : 0, &L3_cache->data_write_list[current_data].latch, clock, mhartid, debug_unit, header, debug_stream)) {
							bus3_tracker->list[current_addr].status = link_free;
						}
						else {
							debug++;
						}
					}
				}
			}
		}
	}
}

void L3_internals(UINT64 clock, banked_cache_type* L3_cache, int debug_unit, param_type* param, FILE* debug_stream) {
	int debug = 0;
	for (UINT bank = 0; bank < core_count; bank++) {
		char header[0x100];
		sprintf_s(header, "L3 bank(%d) ", bank);
		if (clock_bank_array_path(&L3_cache->bank[bank], clock, header, debug_unit, debug_stream)) {
			UINT mhartid = L3_cache->bank[bank].array.data[0].xaction_id & 0x0f;
			if ((clock & 1) != 0)
				debug++;
			char message[0x100];
			sprintf_s(message, "bank(%d) cache hit data return to bus3(%d)", bank, mhartid);
			fifo_input(L3_cache->data_read_fifo[mhartid], &L3_cache->bank[bank].array.data[0], clock, message, debug_stream);

			if (debug_unit) {
				fprintf(debug_stream, "L3 bank(%d) xaction id 0x%04x, data return bus3(%01x) //  clock: 0x%04llx\n", bank, L3_cache->bank[bank].array.data[0].xaction_id, mhartid, clock);
			}
			L3_cache->bank[bank].array.data[0].snoop_response = snoop_idle;
		}
	}

	L3_queue_maintainence(L3_cache);
	for (UINT mhartid = 0; mhartid < core_count; mhartid++) {
		delayed_data_read_to_memory(&L3_cache->bus3_tracker[mhartid], L3_cache, clock, mhartid, debug_unit, debug_stream);
	}

	data_write_to_memory(L3_cache, clock, debug_unit, debug_stream);

	for (UINT mhartid = ((L3_cache->active_core + 1) & (core_count - 1)), count = 0; count < core_count; count++, mhartid = ((mhartid + 1) & (core_count - 1))) {
		for (UINT current = L3_cache->bus3_tracker[mhartid].start; current != L3_cache->bus3_tracker[mhartid].stop; current = ((current + 1) & (L3_cache->bus3_tracker[mhartid].round_count))) {
			if (L3_cache->bus3_tracker[mhartid].list[current].status == link_hold_C) {
				int bank = (L3_cache->bus3_tracker[mhartid].list[current].latch.addr >> (L3_cache->bank[0].set_shift+11)) & (core_count - 1);
				UINT set_addr = (L3_cache->bus3_tracker[mhartid].list[current].latch.addr >> L3_cache->bank[0].set_shift) & 0x000007ff;
				if (L3_cache->bank[bank].in_use == 0 && L3_cache->bank[bank].tags[set_addr].in_use == 0 && L3_cache->bus3_tracker[mhartid].list[current].addr_sent == 0) {
					L3_bank_addr_path(L3_cache, current, clock, mhartid, debug_unit, param, debug_stream);
				}
			}
		}
	}
	for (UINT mhartid = ((L3_cache->active_core + 1) & (core_count - 1)), count = 0; count < core_count; count++, mhartid = ((mhartid + 1) & (core_count - 1))) {
		UINT8 stop = 0;
		for (UINT current = L3_cache->bus3_tracker[mhartid].start; current != L3_cache->bus3_tracker[mhartid].stop && !stop; current = ((current + 1) & (L3_cache->bus3_tracker[mhartid].round_count))) {
			if (L3_cache->bus3_tracker[mhartid].list[current].latch.xaction == bus_LR_aq_rl && (current != L3_cache->bus3_tracker[mhartid].start)) { // serialize upon locked operation
				stop = 1;
			}
			else {
				int bank = (L3_cache->bus3_tracker[mhartid].list[current].latch.addr >> (L3_cache->bank[0].set_shift+11)) & (core_count - 1);
				if (L3_cache->bus3_tracker[mhartid].list[current].status == link_issued_forward && L3_cache->bus3_tracker[mhartid].list[current].addr_sent == 0) {
					fifo_input(L3_cache->mem_addr_out, &L3_cache->bus3_tracker[mhartid].list[current].latch, clock, bank, debug_stream);
					L3_cache->bus3_tracker[mhartid].list[current].addr_sent = 1;
					if (debug_unit) {
						fprintf(debug_stream, "L3 addr 0x%016I64x xaction id %#06x q_id:%#4x, ",
							L3_cache->bus3_tracker[mhartid].list[current].latch.addr, L3_cache->bus3_tracker[mhartid].list[current].latch.xaction_id, current);
						fprintf(debug_stream, "non_cacheable sent to mem ctrl; clock 0x%04llx\n", clock);
					}
				}
				else if (L3_cache->bus3_tracker[mhartid].list[current].status == link_hold_C) {
					fifo_input(L3_cache->mem_addr_out, &L3_cache->bus3_tracker[mhartid].list[current].latch, clock, bank, debug_stream);
					L3_cache->bus3_tracker[mhartid].list[current].status = link_issued_forward1;
				}
				else if (L3_cache->bus3_tracker[mhartid].list[current].status == link_hold) {
					if (L3_cache->bus3_tracker[mhartid].list[current].latch.cacheable == page_non_cache) {
						fifo_input(L3_cache->mem_addr_out, &L3_cache->bus3_tracker[mhartid].list[current].latch, clock, bank, debug_stream);
						L3_cache->bus3_tracker[mhartid].list[current].status = link_issued_forward;
					}
					else {
//						int bank = (L3_cache->bus3_tracker[mhartid].list[current].latch.addr >> (core_count - 1)) & (core_count - 1);
						UINT set_addr = (L3_cache->bus3_tracker[mhartid].list[current].latch.addr >> L3_cache->bank[0].set_shift) & 0x000007ff;
						if (L3_cache->bank[bank].in_use == 0 && L3_cache->bus3_tracker[mhartid].list[current].addr_sent == 0) {
							L3_bank_addr_path(L3_cache, current, clock, mhartid, debug_unit, param, debug_stream);
							if (L3_cache->bus3_tracker[mhartid].list[current].status != link_hold && L3_cache->bus3_tracker[mhartid].list[current].status != link_free) {
								L3_cache->bank[bank].in_use = 1;
								L3_cache->active_core = mhartid;
							}
						}
					}
				}
			}
		}
	}

	for (UINT mhartid = ((L3_cache->active_core + 1) & (core_count - 1)), count = 0; count < core_count; count++, mhartid = ((mhartid + 1) & (core_count - 1))) {
		for (UINT current = L3_cache->bus3_tracker[mhartid].start; current != L3_cache->bus3_tracker[mhartid].stop; current = ((current + 1) & (L3_cache->bus3_tracker[mhartid].round_count))) {
			if (L3_cache->bus3_tracker[mhartid].list[current].status == link_hold) {
				int bank = (L3_cache->bus3_tracker[mhartid].list[current].latch.addr >> (L3_cache->bank[0].set_shift+11)) & (core_count - 1);
				UINT set_addr = (L3_cache->bus3_tracker[mhartid].list[current].latch.addr >> L3_cache->bank[0].set_shift) & 0x000007ff;
				if (L3_cache->bank[bank].in_use == 0 && L3_cache->bank[bank].tags[set_addr].in_use == 0 && L3_cache->bus3_tracker[mhartid].list[current].addr_sent == 0) {
					L3_bank_addr_path(L3_cache, current, clock, mhartid, debug_unit, param, debug_stream);
					L3_cache->active_core = mhartid;
				}
			}
		}
	}
	for (UINT mhartid = ((L3_cache->active_core + 1) & (core_count - 1)), count = 0; count < core_count; count++, mhartid = ((mhartid + 1) & (core_count - 1))) {
		write_data_to_L3(L3_cache, mhartid, clock, debug_unit, debug_stream);
	}

}
void L3_cache_unit(addr_bus_type* mem_addr_out, bus_w_snoop_signal1* bus3, data_bus_type* mem_data_write, data_bus_type* mem_data_read, UINT8 reset, UINT64 clock, banked_cache_type* L3_cache, UINT8* exit_flag, param_type *param, FILE* debug_stream) {
	int debug = 0;
	int debug_unit = (param->caches || param->L3) && clock >= param->start_time;
	if (clock >= 0x0178)
		debug++;
	if ((clock & 1) == 0) {
		for (UINT mhartid = 0; mhartid < core_count; mhartid++) {
			if (bus3[mhartid].data_write.in.snoop_response != snoop_idle && bus3[mhartid].data_write.in.snoop_response != snoop_stall) {
				char message[0x100];
				sprintf_s(message, "mem bus data write forward from bus3(%d)", mhartid);
				fifo_input(L3_cache->data_write_fifo[mhartid], &bus3[mhartid].data_write.in,  clock, message, debug_stream);
				if (debug_unit) {
					switch (bus3[mhartid].data_write.in.snoop_response) {
					case snoop_miss:
						fprintf(debug_stream, "L3 xaction id %#06x, data write latch from bus3(%d) MISS clock 0x%04llx\n",
							bus3[mhartid].data_write.in.xaction_id, mhartid, clock);
						break;
					case snoop_hit:
						fprintf(debug_stream, "L3 xaction id %#06x, data write latch from bus3(%d) HIT clock 0x%04llx data0: 0x%016I64x, 1: 0x%016I64x, 2: 0x%016I64x, 3: 0x%016I64x, t4: 0x%016I64x, 5: 0x%016I64x, 6: 0x%016I64x, 7: 0x%016I64x,\n",
							bus3[mhartid].data_write.in.xaction_id, mhartid, clock, bus3[mhartid].data_write.in.data[0], bus3[mhartid].data_write.in.data[1], bus3[mhartid].data_write.in.data[2], bus3[mhartid].data_write.in.data[3],
							bus3[mhartid].data_write.in.data[4], bus3[mhartid].data_write.in.data[5], bus3[mhartid].data_write.in.data[6], bus3[mhartid].data_write.in.data[7]);
						fprintf(debug_stream, "\ 8: 0x%016I64x, 9: 0x%016I64x, 10: 0x%016I64x, 11: 0x%016I64x, 12: 0x%016I64x, 13: 0x%016I64x, 14: 0x%016I64x, 15: 0x%016I64x\n",
							bus3[mhartid].data_write.in.data[8], bus3[mhartid].data_write.in.data[9], bus3[mhartid].data_write.in.data[10], bus3[mhartid].data_write.in.data[11],
							bus3[mhartid].data_write.in.data[12], bus3[mhartid].data_write.in.data[13], bus3[mhartid].data_write.in.data[14], bus3[mhartid].data_write.in.data[15]);
						break;
					case snoop_dirty:
						fprintf(debug_stream, "L3 xaction id %#06x, data write latch from bus3(%d) DIRTY clock 0x%04llx\n",
							bus3[mhartid].data_write.in.xaction_id, mhartid, clock);
						break;
					case snoop_idle:
					case snoop_stall:
					default:
						debug++;
						break;
					}
				}
			}
		}
	}

	L3_internals(clock, L3_cache, debug_unit, param, debug_stream);
	// data returning from cache arrays
	// need bank ptr - round robin priority - make returns from array more fair
	// buffer out needs to be cleared before array queues
	if ((clock & 1) == 0) {
		const long mask = 0x00007fff;
		const UINT64 mask_b = 0xffffffffff400000;

		if (reset) {
			for (UINT mhartid = 0; mhartid < core_count; mhartid++)L3_cache->mem_data_write_fifo[mhartid].snoop_response = snoop_idle;
		}
		else {
			if (L3_cache->bus3_tracker[0].stop != 0)
				debug++;

			for (UINT mhartid = 0; mhartid < core_count; mhartid++) {// latched read data return
				if (bus3[mhartid].data_read.out.snoop_response != snoop_idle)
					debug++;
				copy_data_bus_info(&bus3[mhartid].data_read.out, &L3_cache->data_read_fifo[mhartid][0]);
				for (UINT i = 0; i < (core_count - 1); i++)
					copy_data_bus_info(&L3_cache->data_read_fifo[mhartid][i], &L3_cache->data_read_fifo[mhartid][i + 1]);
				L3_cache->data_read_fifo[mhartid][(core_count - 1)].snoop_response = snoop_idle;
			}
			// end data returning from cache arrays
			if (mem_addr_out->strobe != 0)
				debug++;

			// NOTE: 
			// 1st come first serve - no particular ordering
			// alternate is round robin per core, no one core can clog up the network, limit to 4 address per core queuing???
			int depth = 0;
			copy_addr_bus_info(mem_addr_out, &L3_cache->mem_addr_out[0], clock);
			for (UINT i = 0; i < ((4 * core_count) - 1); i++) {
				copy_addr_bus_info(&L3_cache->mem_addr_out[i], &L3_cache->mem_addr_out[i + 1], clock);
				if (L3_cache->mem_addr_out[i].strobe)
					depth = i;
			}
			L3_cache->mem_addr_out[((4 * core_count) - 1)].strobe = 0;
			if (debug_unit && mem_addr_out->strobe) {
				fprintf(debug_stream, "L3 xaction id %#06x, memory address fifo to bus; depth 0x%04x clock 0x%04llx\n", 
					mem_addr_out->xaction_id, depth, clock);
			}
			// end addr issue to mem controller


			copy_data_bus_info(mem_data_write, &L3_cache->mem_data_write_fifo[0]);
			for (UINT mhartid = 0; mhartid < (core_count - 1); mhartid++)
				copy_data_bus_info(&L3_cache->mem_data_write_fifo[mhartid], &L3_cache->mem_data_write_fifo[mhartid + 1]);
			L3_cache->mem_data_write_fifo[(core_count - 1)].snoop_response = snoop_idle;
			// end data issue to mem controller

			// snoop handlers first
			snoop_cores(bus3, L3_cache, (clock >> 1), debug_unit, debug_stream);
			snoop_response_to_cores(bus3, L3_cache, (clock >> 1), debug_unit, debug_stream);
			// data write 1st priority

			latch_data_from_memory(L3_cache, bus3, mem_data_read, clock, (clock >> 1), debug_unit, debug_stream);
			// bus 3 interface
			for (UINT mhartid = ((L3_cache->active_core + 1) & (core_count - 1)), count = 0; count < core_count; count++, mhartid = ((mhartid + 1) & (core_count - 1))) {
				bus3_address_latch(L3_cache, &bus3[mhartid].addr.in, (clock >> 1), mhartid, debug_unit, debug_stream);
			}
			L3_cache->active_core = ((L3_cache->active_core + 1) & (core_count - 1));
			if (L3_cache->data_write_start_ptr == ((L3_cache->data_write_end_ptr + 1) & (L3_cache->data_write_count - 1)))
				debug++;
		}
	}
}