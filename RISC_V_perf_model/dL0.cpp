#include "cache.h"

void dL0_flush(addr_bus_type* bus2_addr, L0_data_cache_type* L0_cache, UINT mhartid, UINT64 clock, UINT debug_unit_walk, FILE* debug_stream) {
	if (((L0_cache->reg & 6) == 2) && bus2_addr->strobe == 0) {
		L0_cache->reg &= ~2;
		for (UINT8 i = 0; i < 0x20; i++) {
			if (L0_cache->entry[i].dirty) {
				for (UINT8 way = 0; way < 8 && bus2_addr->strobe == 0; way++) {
					if (L0_cache->entry[i].way[way].state != modified_line) {
						L0_cache->entry[i].way[way].state = invalid_line;
					}
					else if (L0_cache->id_in_use[way] == 0 && (((L0_cache->delayed_bus2write_stop + 1) & 7) == L0_cache->delayed_bus2write_start)) {

						bus2_addr->strobe = 1;
						bus2_addr->cacheable = page_rw;
						bus2_addr->addr = (L0_cache->entry[i].way[way].tag << 12) | (i << 7);
						bus2_addr->xaction = bus_store_full;
						bus2_addr->xaction_id = (way << 12) | (3 << 8) | mhartid;

						L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].snoop_response = snoop_dirty;
						L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].cacheable = page_rw;
						L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].xaction_id = bus2_addr->xaction_id;
						L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].valid = -1;
						for (UINT8 j = 0; j < 0x10; j++)L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].data[j] = L0_cache->entry[i].way[way].line[j];
						if (debug_unit_walk) {
							fprintf(debug_stream, "dl0(%x) set.way 0x%02x.%x, xaction id %#06x, ctrl: FLUSH cache line; address: 0x%016I64x, clock: 0x%04llx\n",
								mhartid, i, way, bus2_addr->xaction_id, bus2_addr->addr, clock);
						}
						L0_cache->entry[i].way[way].state = invalid_line;
						L0_cache->id_in_use[way] = 1;
						L0_cache->delayed_bus2write_stop = ((L0_cache->delayed_bus2write_stop + 1) & 7);
					}
				}
				L0_cache->entry[i].dirty = (L0_cache->entry[i].way[0].state >> 2) || (L0_cache->entry[i].way[1].state >> 2) || (L0_cache->entry[i].way[2].state >> 2) || (L0_cache->entry[i].way[3].state >> 2) ||
					(L0_cache->entry[i].way[4].state >> 2) || (L0_cache->entry[i].way[5].state >> 2) || (L0_cache->entry[i].way[6].state >> 2) || (L0_cache->entry[i].way[7].state >> 2);
				L0_cache->reg |= (L0_cache->entry[i].dirty << 1);
			}
		}
	}
}
void dL0_snoop_addr_to_data_write(data_bus_type* bus2_data_write, addr_bus_type* ctrl_addr, addr_bus_type* snoop_addr, cache_8way_type* entry, UINT ctrl_reg, UINT mhartid, UINT64 clock, UINT debug_unit_ext_snoop, FILE* debug_stream) {
	int debug = 0;
	if (snoop_addr->strobe == 1) {
		switch (snoop_addr->addr) {
		case io_addr_L0D_ctrl:
			if ((snoop_addr->xaction & 0xfc) == bus_SC) {
				copy_addr_bus_info(ctrl_addr, snoop_addr, clock);
				if (debug_unit_ext_snoop) {
					fprintf(debug_stream, "dl0(%x) xaction id %#06x, ctrl: L0D ctrl update addr received; clock: 0x%04llx\n", mhartid, snoop_addr->xaction_id, clock);
				}
			}
			else {
				copy_addr_bus_info(bus2_data_write, snoop_addr);
				bus2_data_write->data[0] = ctrl_reg;
				bus2_data_write->valid = 1;
				bus2_data_write->snoop_response = snoop_hit;
				if (debug_unit_ext_snoop) {
					fprintf(debug_stream, "dl0(%x) xaction id %#06x, ctrl: L0D ctrl read received; clock: 0x%04llx\n", mhartid, snoop_addr->xaction_id, clock);
				}
			}
			break;
		case io_addr_TLB_4K_data:
		case io_addr_TLB_2M_data:
		case io_addr_TLB_data_vaddr:
			//		case io_addr_TLB_data_vaddr:
		case io_addr_TLB_data_ctrl:
		case io_addr_L2_ctrl:
			break;
		default: {
			cache_8way_type* cache_entry = &entry[(snoop_addr->addr >> 7) & 0x00001f];
			bus2_data_write->snoop_response = snoop_miss;// need to wait for code and data L0 response
			bus2_data_write->xaction_id = snoop_addr->xaction_id;
			bus2_data_write->cacheable = snoop_addr->cacheable;
			UINT8 hit = 0;
			for (UINT8 way = 0; way < 8; way++) {
				if (cache_entry->way[way].tag == (snoop_addr->addr >> 12)) {
					bus2_data_write->snoop_response = (cache_entry->way[way].state == modified_line) ? snoop_dirty : snoop_hit;
					cache_entry->way[way].state = (snoop_addr->xaction == bus_allocate || snoop_addr->xaction == bus_store_full || snoop_addr->xaction == bus_store_partial) ? invalid_line : shared_line;
					cache_entry->dirty |= (cache_entry->way[way].state >> 2);
					bus2_data_write->valid = -1;
					for (UINT8 i = 0; i < 0x10; i++)bus2_data_write->data[i] = cache_entry->way[way].line[i];
					hit = 1;
					if (debug_unit_ext_snoop) {
						UINT8 set = (snoop_addr->addr >> 7) & 0x00001f;
						char state = (cache_entry->way[way].state == shared_line) ? 'S' : 'I';
						fprintf(debug_stream, "dl0(%x) set.way 0x%02x.%d %c xaction id %#06x, ", mhartid, set, way, state, bus2_data_write->xaction_id);
						UINT64* data = bus2_data_write->data;
						print_xaction2(snoop_addr->xaction, debug_stream);
						if (bus2_data_write->snoop_response == snoop_hit)
							fprintf(debug_stream, " Snoop Cache Hit logical addr,data 0x%016I64x,0x%016I64x clock: 0x%04llx\n", snoop_addr->addr, bus2_data_write->data[(snoop_addr->addr >> 7) & 0x0f], clock);
						else
							fprintf(debug_stream, " Snoop Cache DIRTY logical addr,data 0x%016I64x,0x%016I64x clock: 0x%04llx\n", snoop_addr->addr, bus2_data_write->data[(snoop_addr->addr >> 7) & 0x0f], clock);

						fprintf(debug_stream, "dl0(%x) set.way 0x%02x.%d %c xaction id %#06x, ", mhartid, set, way, state, bus2_data_write->xaction_id);
						fprintf(debug_stream, "0x00 0x%016I64x 0x08 0x%016I64x 0x10 0x%016I64x 0x18 0x%016I64x clock: 0x%04llx\n", data[0], data[1], data[2], data[3], clock);
						fprintf(debug_stream, "dl0(%x) set.way 0x%02x.%d %c xaction id %#06x, ", mhartid, set, way, state, bus2_data_write->xaction_id);
						fprintf(debug_stream, "0x20 0x%016I64x 0x28 0x%016I64x 0x30 0x%016I64x 0x38 0x%016I64x clock: 0x%04llx\n", data[4], data[5], data[6], data[7], clock);
						fprintf(debug_stream, "dl0(%x) set.way 0x%02x.%d %c xaction id %#06x, ", mhartid, set, way, state, bus2_data_write->xaction_id);
						fprintf(debug_stream, "0x40 0x%016I64x 0x48 0x%016I64x 0x50 0x%016I64x 0x58 0x%016I64x clock: 0x%04llx\n", data[8], data[9], data[10], data[11], clock);
						fprintf(debug_stream, "dl0(%x) set.way 0x%02x.%d %c xaction id %#06x, ", mhartid, set, way, state, bus2_data_write->xaction_id);
						fprintf(debug_stream, "0x60 0x%016I64x 0x68 0x%016I64x 0x70 0x%016I64x 0x78 0x%016I64x clock: 0x%04llx\n", data[12], data[13], data[14], data[15], clock);

					}
				}
			}
			if (!hit) {
				if (debug_unit_ext_snoop) {
					fprintf(debug_stream, "dl0(%x) xaction id %#06x, ", mhartid, snoop_addr->xaction_id);
					print_xaction2(snoop_addr->xaction, debug_stream);
					fprintf(debug_stream, " Snoop Cache Miss logical address: 0x%016I64x, clock: 0x%04llx\n", snoop_addr->addr, clock);
				}
			}
		}
			   break;
		}
	}
}
void dL0_evict_data_to_L2(data_bus_type* bus2_data_write, L0_data_cache_type* L0_cache, data_bus_type* physical_bus_data, UINT mhartid, UINT64 clock, UINT debug_unit, FILE* debug_stream) {
	int debug = 0;
	if (mhartid == 0) {
		if (clock == 0x13fb)
			debug++;
	}

	for (UINT current = L0_cache->delayed_bus2write_start; current != L0_cache->delayed_bus2write_stop && bus2_data_write->snoop_response == snoop_idle; current = ((current + 1) & 7)) {
		if (current == L0_cache->delayed_bus2write_start && L0_cache->delayed_bus2write[current].snoop_response == snoop_idle) {
			L0_cache->delayed_bus2write_start = ((L0_cache->delayed_bus2write_start + 1) & 7);
		}
		else {
			copy_data_bus_info(bus2_data_write, &L0_cache->delayed_bus2write[current]);
			L0_cache->delayed_bus2write[current].snoop_response = snoop_idle;
			if (debug_unit) {
				UINT64* data = bus2_data_write->data;
				fprintf(debug_stream, "dL0(%lld) xaction id %#06x, eviction to L2 data(0)0x%016I64x (1)0x%016I64x (2)0x%016I64x (3)0x%016I64x (4)0x%016I64x (5)0x%016I64x (6)0x%016I64x (7)0x%016I64x (8)0x%016I64x \n\t\t(9)0x%016I64x (a)0x%016I64x (b)0x%016I64x (c)0x%016I64x (d)0x%016I64x (e)0x%016I64x (f)0x%016I64x, clock: 0x%04llx\n",
					mhartid, bus2_data_write->xaction_id, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8],
					data[9], data[10], data[11], data[12], data[13], data[14], data[15], clock);
			}
			L0_cache->id_in_use[(L0_cache->delayed_bus2write[current].xaction_id >> 12) & 0x0f] = 0;
		}
	}
}
void dL0_store_data_to_array_and_to_L2(bus_w_snoop_signal1* bus2, L0_data_cache_type* L0_cache, data_bus_type* store_data,
	data_bus_type* physical_bus_data, UINT mhartid, UINT64 clock, UINT debug_unit, FILE* debug_stream) {
	addr_bus_type* bus2_addr = &bus2->addr.out;
	int debug = 0;
	if (mhartid == 0 && clock == 0x23d3)
		debug++;
	if (store_data->snoop_response != snoop_idle) {
		UINT hit = 0;
		UINT8 current = ((store_data->xaction_id >> 12) & 3);
		if (mhartid == 0) {
			if (clock >= 0x00be)
				debug++;
		}
		if (store_data->cacheable == page_non_cache || store_data->cacheable == page_IO) {
			if (L0_cache->write_data_fifo[0].snoop_response == snoop_idle)
				copy_data_bus_info(&L0_cache->write_data_fifo[0], store_data);
			else
				debug++;
			if (debug_unit) {
				fprintf(debug_stream, "dL0(%lld) xaction id %#06x, WRITE forward non cacheable (or IO) data to L2, data:  0x%016I64x, valid: 0x%016I64x, clock: 0x%04llx\n",
					mhartid, store_data->xaction_id, store_data->data[0], store_data->valid, clock);
			}
			switch (((store_data->xaction_id >> 8) & 3)) {
			case 2:
				if (L0_cache->alloc_list[current].latch.xaction_id == store_data->xaction_id) {
					hit = 1;
					if (L0_cache->alloc_list[current].latch.xaction != bus_SC)
						L0_cache->alloc_list[current].status = link_free;
				}
				break;
			case 3:
				if (L0_cache->write_list[current].latch.xaction_id == store_data->xaction_id) {
					hit = 1;
					if (L0_cache->write_list[current].latch.xaction != bus_SC)
						L0_cache->write_list[current].status = link_free;
				}
				break;
			case 1:
			default:
				fprintf(debug_stream, "ERROR: shouldn't enter this point");
				break;
			}
		}
		else {
			if (L0_cache->write_list[current].latch.xaction_id == store_data->xaction_id && L0_cache->write_list[current].status != link_free &&
				L0_cache->write_list[current].status != link_issued_forward && L0_cache->write_list[current].status != link_issued_forward1 && L0_cache->write_list[current].status != link_issued_forward2) {
				UINT8 set = ((L0_cache->write_list[current].latch.addr >> 7) & 0x1f);
				if (L0_cache->write_list[current].latch.xaction != bus_store_partial) {
					for (UINT8 way = 0; way < 8 && !hit && store_data->valid == 0xffffffffffffffff; way++) {
						if ((L0_cache->write_list[current].latch.addr >> 12) == (L0_cache->entry[set].way[way].tag) && (L0_cache->write_list[current].latch.xaction == bus_store_full || L0_cache->entry[set].way[way].state != modified_line)) {
							L0_cache->entry[set].way[way].state = modified_line;
							L0_cache->entry[set].dirty = 1;
							L0_cache->entry[set].way_ptr = ((way + 1) & 7);
							hit = 1;
							for (UINT i = 0; i < 0x10; i++)	L0_cache->entry[set].way[way].line[i] = store_data->data[i];
							L0_cache->entry[set].in_use = 1;
							if (debug_unit) {
								fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%dM xaction id 0x%04x q_id 0x%02x, write cache hit/fill cycle; tag: 0x%08x, addr: 0x%016x,",
									mhartid, set, way, store_data->xaction_id, current, L0_cache->entry[set].way[way].tag, L0_cache->write_list[current].latch.addr);
								fprintf(debug_stream, " clock: 0x%04llx\n", clock);
								fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%dM 0x00 0x%016x, 0x08 0x%016x, 0x10 0x%016x, 0x18 0x%016x clock: 0x%04llx\n",
									mhartid, set, way, store_data->data[0], store_data->data[1], store_data->data[2], store_data->data[3], clock);
								fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%dM 0x20 0x%016x, 0x28 0x%016x, 0x30 0x%016x, 0x38 0x%016x clock: 0x%04llx\n",
									mhartid, set, way, store_data->data[4], store_data->data[5], store_data->data[6], store_data->data[7], clock);
								fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%dM 0x40 0x%016x, 0x48 0x%016x, 0x50 0x%016x, 0x58 0x%016x clock: 0x%04llx\n",
									mhartid, set, way, store_data->data[8], store_data->data[9], store_data->data[10], store_data->data[11], clock);
								fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%dM 0x60 0x%016x, 0x68 0x%016x, 0x70 0x%016x, 0x78 0x%016x clock: 0x%04llx\n",
									mhartid, set, way, store_data->data[12], store_data->data[13], store_data->data[14], store_data->data[15], clock);
							}
							L0_cache->write_list[current].status = link_free;
						}
					}
				}
				if (!hit) {
					UINT8 way = L0_cache->entry[set].way_ptr;
					if (store_data->valid == 0xffffffffffffffff) {
						L0_cache->entry[set].way_ptr = ((L0_cache->entry[set].way_ptr + 1) & 7);
						if (L0_cache->entry[set].way[way].state != invalid_line) {
							UINT way2 = 0;
							for (UINT count = 0; count < 8 && L0_cache->entry[set].way[way2].state != invalid_line; count++) {
								way2 = (way2 + 1) & 7;
							}
							if (L0_cache->entry[set].way[way2].state == invalid_line) {
								way = way2;
								L0_cache->entry[set].way_ptr = (way2 + 1) & 7;
							}
						}
						if (L0_cache->entry[set].way[way].state == modified_line) {
							if (bus2_addr->strobe == 0) {
								bus2_addr->strobe = 1;
								bus2_addr->addr = (L0_cache->entry[set].way[way].tag << 12) | (set << 7);
								bus2_addr->xaction_id = (way << 12) | (3 << 8) | mhartid;
								bus2_addr->xaction = bus_store_full;
								bus2_addr->cacheable = page_rx;

								L0_cache->write_list[L0_cache->store_data_latch_ptr].status = link_issued_forward;
								if (debug_unit) {
									fprintf(debug_stream, "dL0(%lld) set.way %#4x.%d, xaction id %#06x, modified line eviction from L0D to L2, addr: %#016x, valid: 0x%016I64x, clock: 0x%04llx\n",
										mhartid, set, way, bus2_addr->xaction_id, bus2_addr->addr, store_data->valid, clock);
								}
								if (((L0_cache->delayed_bus2write_stop + 1) & 7) == L0_cache->delayed_bus2write_start)
									debug++;
								if (physical_bus_data[3].snoop_response != snoop_idle)
									debug++;
								physical_bus_data[3].snoop_response = snoop_stall;
								L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].snoop_response = snoop_dirty;
								L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].cacheable = page_rx;
								L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].xaction_id = bus2_addr->xaction_id;
								L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].valid = -1;
								for (UINT8 i = 0; i < 0x10; i++) L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].data[i] = L0_cache->entry[set].way[way].line[i];
								L0_cache->delayed_bus2write_stop = ((L0_cache->delayed_bus2write_stop + 1) & 7);

								for (UINT8 i = 0; i < 0x10; i++)  L0_cache->entry[set].way[way].line[i] = store_data->data[i];
								L0_cache->entry[set].way[way].state = modified_line;
								L0_cache->entry[set].way[way].tag = (L0_cache->write_list[current].latch.addr >> 12);
								if (L0_cache->write_list[current].latch.xaction_id != store_data->xaction_id)
									debug++;
								if (debug_unit) {
									fprintf(debug_stream, "dL0(%lld) set.way %#4x.%d, xaction id %#06x, store modified line from core, addr: %#016x, valid: 0x%016I64x, clock: 0x%04llx\n",
										mhartid, set, way, store_data->xaction_id, L0_cache->write_list[current].latch.addr, store_data->valid, clock);
								}
							}
							else {
								L0_cache->write_list[L0_cache->store_data_latch_ptr].status = link_hold_w;
								if (debug_unit) {
									fprintf(debug_stream, "dL0(%lld) set.way %#4x.%d, xaction id %#06x, cacheable WRITE L0 miss; bus busy; addr to L2, addr: %#016x, valid: %#016x, clock: 0x%04llx\n",
										mhartid, set, way, bus2_addr->xaction_id, bus2_addr->addr, store_data->valid, clock);
								}
								L0_cache->store_data_latch[L0_cache->store_data_latch_ptr].cacheable = page_rx;
								L0_cache->store_data_latch[L0_cache->store_data_latch_ptr].snoop_response = snoop_dirty;
								L0_cache->store_data_latch[L0_cache->store_data_latch_ptr].valid = -1;
								L0_cache->store_data_latch[L0_cache->store_data_latch_ptr].xaction_id = bus2_addr->xaction_id;
								for (UINT8 i = 0; i < 0x10; i++)	L0_cache->store_data_latch[L0_cache->store_data_latch_ptr].data[i] = L0_cache->entry[set].way[way].line[i];

								L0_cache->store_data_latch_ptr = (L0_cache->store_data_latch_ptr + 1) & 3;
							}
						}
						else {
							if (debug_unit) {
								fprintf(debug_stream, "dL0(%lld) set.way %#4x.%d, q_id 0x%02x, xaction id %#06x, write, cache miss/fill cycle;",
									mhartid, set, way, current, store_data->xaction_id);
								print_xaction(L0_cache->write_list[current].latch.xaction, debug_stream);
								fprintf(debug_stream, " clock: 0x%04llx\n", clock);
							}
							L0_cache->entry[set].way[way].state = modified_line;
							L0_cache->entry[set].dirty = 1;
							hit = 1;
							for (UINT i = 0; i < 0x10; i++)	L0_cache->entry[set].way[way].line[i] = store_data->data[i];
							L0_cache->entry[set].in_use = 1;
							L0_cache->entry[set].way[way].tag = (L0_cache->write_list[current].latch.addr >> 12);
							L0_cache->write_list[current].status = link_free;
						}
					}
					else if (bus2_addr->strobe == 0) {
						copy_addr_info(bus2_addr, &L0_cache->write_list[current].latch);
						L0_cache->write_list[current].status = link_issued_forward;
						copy_data_bus_info(&L0_cache->store_data_latch[L0_cache->store_data_latch_stop], store_data);
						L0_cache->store_data_latch_stop = (L0_cache->store_data_latch_stop + 1) & 3;
						L0_cache->store_data_latch_ptr = current;
						if (debug_unit) {
							fprintf(debug_stream, "dL0(%lld) set.way %#4x.%d, xaction id %#06x, cacheable WRITE addr to L2, addr: %#016x, valid: %#016x, clock: 0x%04llx\n",
								mhartid, set, way, bus2_addr->xaction_id, bus2_addr->addr, store_data->valid, clock);
						}
					}
					else {
						L0_cache->write_list[current].status = link_hold_w;
						copy_data_bus_info(&L0_cache->store_data_latch[L0_cache->store_data_latch_stop], store_data);
						L0_cache->store_data_latch_stop = (L0_cache->store_data_latch_stop + 1) & 3;
						L0_cache->store_data_latch_ptr = current;
						if (debug_unit) {
							fprintf(debug_stream, "dL0(%lld) set.way %#4x.%d, xaction id %#06x, cacheable WRITE addr to L2, addr: %#016x, valid: %#016x, clock: 0x%04llx\n",
								mhartid, set, way, bus2_addr->xaction_id, bus2_addr->addr, store_data->valid, clock);
						}
					}
					hit = 1;
				}
			}
		}
		if (debug_unit) {
			if (!hit && bus2_addr->strobe == 0) {
				fprintf(debug_stream, "dL0(%lld) ERROR: write with no matching addr, xaction id %#06x, cacheable WRITE data to L2, valid: %#010x, clock: 0x%04llx\n", mhartid, store_data->xaction_id, store_data->valid, clock);
			}
		}
	}
}
void dL0_incoming_read_data(addr_bus_type* bus2_addr, data_bus_type* bus2_data_write, L0_data_cache_type* L0_cache, data_bus_type* bus2_data_read, data_bus_type* physical_bus_data, UINT mhartid, UINT64 clock, UINT debug_unit, UINT debug_unit_walk, L2_cache_type* L2_cache_var, param_type* param, FILE* debug_stream) {
	int debug = 0;
	if (L0_cache->ctrl_addr.strobe && (bus2_data_read->snoop_response == snoop_dirty)) {
		if (bus2_data_read->xaction_id == L0_cache->ctrl_addr.xaction_id) {
			if (L0_cache->ctrl_addr.addr == io_addr_L0D_ctrl) {
				if (bus2_data_write->snoop_response == snoop_idle) {
					if (debug_unit_walk) {
						fprintf(debug_stream, "dl0(%x) xaction id %#06x, ctrl: L0D ctrl reg command received; address/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n",
							mhartid, L0_cache->ctrl_addr.xaction_id, L0_cache->ctrl_addr.addr, bus2_data_read->data[0], clock);
					}
					L0_cache->reg = bus2_data_read->data[0];
					if (bus2_data_read->data[0] == 0) {
						for (UINT8 i = 0; i < 0x20; i++) {
							L0_cache->reg |= (L0_cache->entry[i].dirty << 1);
							if (L0_cache->reg & 2 == 0) {
								for (UINT8 way = 0; way < 8; way++) L0_cache->entry[i].way[way].state = invalid_line;
							}
						}
					}
					else {
						debug++;
					}
					L0_cache->ctrl_addr.strobe = 0;
					bus2_data_write->snoop_response = snoop_dirty;
					bus2_data_write->xaction_id = bus2_data_read->xaction_id;
					bus2_data_write->cacheable = page_IO_error_rsp;
					bus2_data_write->valid = 1;
					bus2_data_write->data[0] = 0;// error response, no errors

				}
				else if (((L0_cache->delayed_bus2write_stop + 1) & 7) != L0_cache->delayed_bus2write_start) {
					if (debug_unit_walk) {
						fprintf(debug_stream, "dl0(%x) xaction id %#06x, ctrl: L0D ctrl reg command received, delayed error response; address/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n",
							mhartid, L0_cache->ctrl_addr.xaction_id, L0_cache->ctrl_addr.addr, bus2_data_read->data[0], clock);
					}
					L0_cache->reg = bus2_data_read->data[0];
					if (bus2_data_read->data[0] == 0) {
						for (UINT8 i = 0; i < 0x20; i++) {
							L0_cache->reg |= (L0_cache->entry[i].dirty << 1);
							if (L0_cache->reg & 2 == 0) {
								for (UINT8 way = 0; way < 8; way++) L0_cache->entry[i].way[way].state = invalid_line;
							}
						}
					}
					else {
						debug++;
					}
					L0_cache->ctrl_addr.strobe = 0;
					L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].snoop_response = snoop_dirty;
					L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].xaction_id = bus2_data_read->xaction_id;
					L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].cacheable = bus2_data_read->cacheable;
					L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].valid = 1;
					L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].data[0] = L0_cache->reg;
					L0_cache->delayed_bus2write_stop = ((L0_cache->delayed_bus2write_stop + 1) & 7);
				}
				else {
					debug++;// need to add a read in buffer
				}
			}
		}
	}
	else if ((bus2_data_read->snoop_response == snoop_hit || bus2_data_read->snoop_response == snoop_dirty) && ((bus2_data_read->xaction_id & 0x0300) != 0)) {
		UINT hit = 0;
		UINT8 current = ((bus2_data_read->xaction_id >> 12) & 3);
		switch (((bus2_data_read->xaction_id >> 8) & 3)) {
		case 1:
			if (L0_cache->read_list[current].latch.xaction_id == bus2_data_read->xaction_id && L0_cache->read_list[current].status != link_free) {
				hit = 0;
				if (L0_cache->read_list[current].latch.cacheable != page_non_cache) {
					int set = (L0_cache->read_list[current].latch.addr >> 7) & 0x0000001f;
					cache_8way_type* cache_entry = &L0_cache->entry[set];
					for (UINT way = 0; (way < 8) && !hit; way++) {
						if (cache_entry->way[way].tag == (L0_cache->read_list[current].latch.addr >> 12)) {
							hit = 1;
							if (cache_entry->way[way].state == modified_line) {
								if (debug_unit) {
									fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d, q_id 0x%02x, xaction id %#06x, Load unit Incomming data hit modified line;",
										mhartid, set, way, current, bus2_data_read->xaction_id);
									fprintf(debug_stream, ", list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
								}
							}
							else if (bus2_data_read->snoop_response == snoop_dirty) {
								cache_entry->way[way].state = modified_line;
								for (UINT i = 0; i < 0x10; i++)L0_cache->entry[set].way[way].line[i] = bus2_data_read->data[i];
								if (debug_unit) {
									fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d, q_id 0x%02x, xaction id %#06x, Load unit Incomming data hit modified line;",
										mhartid, set, way, current, bus2_data_read->xaction_id);
									fprintf(debug_stream, ", list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
								}
							}
							else {
								cache_entry->way[way].state = shared_line;
								for (UINT i = 0; i < 0x10; i++)L0_cache->entry[set].way[way].line[i] = bus2_data_read->data[i];
								if (debug_unit) {
									fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d S q_id 0x%02x, xaction id %#06x, Load unit Incomming data hit;",
										mhartid, set, way, current, bus2_data_read->xaction_id);
									fprintf(debug_stream, ", list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
								}
							}
							L0_cache->read_list[current].status = link_free;
							cache_entry->way_ptr = ((way + 1) & 7);
						}
					}
					for (UINT way = 0; (way < 8) && cache_entry->way[cache_entry->way_ptr].state != invalid_line; way++) {
						if (cache_entry->way[way].state == invalid_line) {
							cache_entry->way_ptr = way;
							if (debug_unit) {
								fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d S q_id 0x%02x, xaction id %#06x, Load unit Incomming data hit;",
									mhartid, set, way, current, bus2_data_read->xaction_id);
								fprintf(debug_stream, ", list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
							}
							L0_cache->read_list[current].status = link_free;
							cache_entry->way_ptr = ((way + 1) & 7);
						}
					}
					if (!hit) {
						if (cache_entry->way[cache_entry->way_ptr].state == modified_line) {
							if (bus2_addr->strobe == 0) {
								bus2_addr->strobe = 1;
								bus2_addr->addr = (cache_entry->way[cache_entry->way_ptr].tag << 12) | (set << 7);;
								bus2_addr->xaction_id = (L0_cache->alloc_list[current].latch.xaction_id & 0xf000) | (3 << 8) | mhartid;
								bus2_addr->xaction = bus_store_full;
								bus2_addr->cacheable = L0_cache->alloc_list[current].latch.cacheable;

								L0_cache->write_list[L0_cache->store_data_latch_ptr].status = link_issued_forward;
								if (debug_unit) {
									fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d addr: %#016x xaction id %#06x, modified line eviction, cause xaction_id: 0x%04x, clock: 0x%04llx\n",
										mhartid, set, cache_entry->way_ptr, bus2_addr->addr, bus2_addr->xaction_id, bus2_data_read->xaction_id, clock);
								}
								if (((L0_cache->delayed_bus2write_stop + 1) & 7) == L0_cache->delayed_bus2write_start)
									debug++;
								physical_bus_data[3].snoop_response = snoop_stall;
								L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].snoop_response = snoop_dirty;
								L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].cacheable = bus2_addr->cacheable;
								L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].xaction_id = bus2_addr->xaction_id;
								L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].valid = -1;
								for (UINT8 i = 0; i < 0x10; i++) L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].data[i] = L0_cache->entry[set].way[cache_entry->way_ptr].line[i];
								L0_cache->delayed_bus2write_stop = ((L0_cache->delayed_bus2write_stop + 1) & 7);
							}
							else {
								debug++;
							}
						}
						cache_entry->way[cache_entry->way_ptr].tag = (L0_cache->read_list[current].latch.addr >> 12);
						for (UINT8 i = 0; i < 0x10; i++) cache_entry->way[cache_entry->way_ptr].line[i] = bus2_data_read->data[i];
						cache_entry->in_use = 0;
						cache_entry->way[cache_entry->way_ptr].state = (bus2_data_read->snoop_response == snoop_dirty) ? modified_line : shared_line;
						hit = 1;
						if (debug_unit) {
							fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d %s addr,data 0x%016I64x,0x%016I64x xaction id %#06x, q_id 0x%02x, Cache Fill / ",
								mhartid, set, cache_entry->way_ptr, (bus2_data_read->snoop_response == snoop_dirty) ? "M" : "S",
								L0_cache->read_list[current].latch.addr, bus2_data_read->data[(L0_cache->read_list[current].latch.addr >> 3) & 0x0f], bus2_data_read->xaction_id, current);
							if (L0_cache->read_list[current].latch.xaction == bus_load)
								fprintf(debug_stream, "bus_load");
							else if (L0_cache->read_list[current].latch.xaction == bus_allocate)
								fprintf(debug_stream, "bus_allocate");
							else
								fprintf(debug_stream, "ERROR: unknown cycle");// needto fix
							fprintf(debug_stream, ", list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
						}
						L0_cache->read_list[current].status = link_free;
						cache_entry->way_ptr = ((cache_entry->way_ptr + 1) & 7);
						hit = 1;
					}
				}// parallel access - do not forward
				if (!hit) {
					if (debug_unit) {
						fprintf(debug_stream, "dL0(%lld) q_id, xaction id %#06x, non-cacheable / ", mhartid, current, bus2_data_read->xaction_id);

						fprintf(debug_stream, "external SNOOP cycle");
						fprintf(debug_stream, ", list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
					}
					L0_cache->read_list[current].status = link_free;
					hit = 1;
				}
			}
			break;
		case 2:
			if (L0_cache->alloc_list[current].latch.xaction_id == bus2_data_read->xaction_id && L0_cache->alloc_list[current].status != link_free) {
				hit = 0;
				if (bus2_data_read->cacheable != page_non_cache) {
					int set = (L0_cache->alloc_list[current].latch.addr >> 7) & 0x0000001f;
					cache_8way_type* cache_entry = &L0_cache->entry[set];
					for (UINT way = 0; (way < 8) && !hit; way++) {
						if (cache_entry->way[way].tag == (L0_cache->alloc_list[current].latch.addr >> 12)) {
							hit = 1;
							if (cache_entry->way[way].state == modified_line) {
								if (debug_unit) {
									fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d M addr: %#016x, q_id 0x%02x, xaction id %#06x, Allocate incoming data hit;",
										mhartid, set, way, L0_cache->alloc_list[current].latch.addr, current, bus2_data_read->xaction_id);
									fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
								}
							}
							else  if (bus2_data_read->snoop_response == snoop_dirty) {
								cache_entry->way[way].state == modified_line;
								if (cache_entry->way[way].state == invalid_line) 	cache_entry->way_ptr = way;
								if (debug_unit) {
									fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d M addr: %#016x, q_id 0x%02x, xaction id %#06x, Allocate incoming data hit;",
										mhartid, set, way, L0_cache->alloc_list[current].latch.addr, current, bus2_data_read->xaction_id);
									fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
								}
							}
							else {
								cache_entry->way[way].state = shared_line;
								for (UINT i = 0; i < 0x10; i++)L0_cache->entry[set].way[way].line[i] = bus2_data_read->data[i];
								if (debug_unit) {
									fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d S addr: %#016x, q_id 0x%02x, xaction id %#06x, Allocate incoming data hit;",
										mhartid, set, way, L0_cache->alloc_list[current].latch.addr, current, bus2_data_read->xaction_id);
									fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
								}
							}
							L0_cache->alloc_list[current].status = link_free;
							cache_entry->way_ptr = ((way + 1) & 7);
						}
					}
					if (!hit) {
						for (UINT way = 0; (way < 8) && !hit; way++) {
							if (cache_entry->way[way].state == invalid_line) {
								if (debug_unit) {
									fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d (E) addr: %#016x, q_id 0x%02x, xaction id %#06x, Allocate incoming data; ",
										mhartid, set, way, L0_cache->alloc_list[current].latch.addr, current, bus2_data_read->xaction_id);
									fprintf(debug_stream, ", list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
								}
								hit = 1;
								L0_cache->alloc_list[current].status = link_free;
								cache_entry->way[way].state = exclusive_line;
								cache_entry->way[way].tag = (L0_cache->alloc_list[current].latch.addr >> 12);
								for (UINT i = 0; i < 0x10; i++)L0_cache->entry[set].way[way].line[i] = bus2_data_read->data[i];
								cache_entry->way_ptr = ((way + 1) & 7);
							}
						}
					}
					if (!hit) {
						if (cache_entry->way[cache_entry->way_ptr].state == modified_line) {
							if (bus2_addr->strobe == 0) {
								bus2_addr->strobe = 1;
								bus2_addr->addr = (cache_entry->way[cache_entry->way_ptr].tag << 12) | (set << 7);;
								bus2_addr->xaction_id = (L0_cache->alloc_list[current].latch.xaction_id & 0xf000) | (3 << 8) | mhartid;
								bus2_addr->xaction = bus_store_full;
								bus2_addr->cacheable = L0_cache->alloc_list[current].latch.cacheable;

								L0_cache->alloc_list[current].status = link_issued_forward;
								if (debug_unit) {
									fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d M addr: %#016x, xaction id %#06x, eviction to L2, ref xaction_id: 0x%04x,clock: 0x%04llx\n",
										mhartid, set, cache_entry->way_ptr, bus2_addr->addr, bus2_addr->xaction_id, L0_cache->alloc_list[current].latch.xaction_id, clock);
								}
								if (((L0_cache->delayed_bus2write_stop + 1) & 7) == L0_cache->delayed_bus2write_start)
									debug++;
								physical_bus_data[3].snoop_response = snoop_stall;
								L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].snoop_response = snoop_dirty;
								L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].cacheable = bus2_addr->cacheable;
								L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].xaction_id = bus2_addr->xaction_id;
								L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].valid = -1;
								for (UINT8 i = 0; i < 0x10; i++) L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].data[i] = L0_cache->entry[set].way[cache_entry->way_ptr].line[i];
								L0_cache->delayed_bus2write_stop = ((L0_cache->delayed_bus2write_stop + 1) & 7);
							}
							else {
								debug++;
							}
						}
						cache_entry->way[cache_entry->way_ptr].tag = (L0_cache->alloc_list[current].latch.addr >> 12);
						for (UINT8 i = 0; i < 0x10; i++) cache_entry->way[cache_entry->way_ptr].line[i] = bus2_data_read->data[i];
						cache_entry->in_use = 0;
						cache_entry->way[cache_entry->way_ptr].state = exclusive_line;
						hit = 1;
						if (debug_unit) {
		//					fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d E addr, data: 0x%016x, 0x%016x; xaction id %#06x, q_id 0x%02x, ",
		//						mhartid, set, cache_entry->way_ptr, L0_cache->alloc_list[current].latch.addr,
		//						bus2_data_read->data[(L0_cache->alloc_list[current].latch.addr >> 3) & 0x0f], bus2_data_read->xaction_id, cache_entry->way_ptr, current);
							fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d E addr, data: ",
								mhartid, set, cache_entry->way_ptr);
							fprint_addr_coma(debug_stream, L0_cache->alloc_list[current].latch.addr, param);
							fprintf(debug_stream, " 0x%016x; xaction id %#06x, q_id 0x%02x, ",
								bus2_data_read->data[(L0_cache->alloc_list[current].latch.addr >> 3) & 0x0f], bus2_data_read->xaction_id, cache_entry->way_ptr, current);
							switch (L0_cache->alloc_list[current].latch.xaction) {
							case bus_prefetch:
								fprintf(debug_stream, "PREFETCH");
								break;
							case bus_fetch:
								fprintf(debug_stream, "FETCH");
								break;
							case bus_load:
								fprintf(debug_stream, "LOAD");
								break;
							case bus_allocate:
								fprintf(debug_stream, "ALLOCATE");
								break;
							case bus_LR_aq:
							case bus_LR_aq_rl:
								fprintf(debug_stream, "LOCKED READ");
								break;
							case bus_SC_aq:
							case bus_SC_aq_rl:
								fprintf(debug_stream, "LOCKED WRITE");
								break;
							default:
								fprintf(debug_stream, "UNKNOWN");
								break;
							}
							fprintf(debug_stream, " Cache Fill list_ptr: 0x%02x, clock: 0x%04llx\n",
								current, clock);
						}
						L0_cache->alloc_list[current].status = link_free;
						cache_entry->way_ptr = ((cache_entry->way_ptr + 1) & 7);
						hit = 1;
					}
				}// parallel access - do not forward
				if (!hit) {
					if (debug_unit) {
						fprintf(debug_stream, "dL0(%lld) q_id 0x%02x, xaction id %#06x, non-cacheable / Allocate store unit",
							mhartid, current, bus2_data_read->xaction_id);
						fprintf(debug_stream, ", list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
					}
					L0_cache->alloc_list[current].status = link_free;
					hit = 1;
				}
				if ((L0_cache->alloc_list[current].latch.xaction & 0xfc) == bus_LR) {
					L0_cache->reg &= (~0x4);// clear lock bit
				}
			}
			break;
		case 3:
			if (L0_cache->write_list[current].latch.xaction_id == bus2_data_read->xaction_id && L0_cache->write_list[current].status != link_free) {
				if (bus2_data_read->cacheable != page_non_cache && (bus2_data_read->cacheable != page_IO)) {
					fprintf(debug_stream, "dL0(%lld)  q_id 0x%02x, xaction id %#06x, Cache Fill / ", mhartid, current, bus2_data_read->xaction_id);
					fprintf(debug_stream, "ERROR: locked write error code should be non-cacheable");
					fprintf(debug_stream, ", list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
				}// parallel access - do not forward
				else {
					if (debug_unit) {
						fprintf(debug_stream, "dL0(%lld) q_id 0x%02x, xaction id %#06x, ", mhartid, current, bus2_data_read->xaction_id);
						fprintf(debug_stream, "locked write error code");
						fprintf(debug_stream, ", list_ptr: 0x%02x, data: 0x%016x, clock: 0x%04llx\n", current, bus2_data_read->data[0], clock);
					}
				}
				L0_cache->store_data_latch[current].snoop_response = snoop_idle;
				L0_cache->store_data_latch[current].valid = 0;
				if (L0_cache->store_data_latch[current].xaction_id != bus2_data_read->xaction_id)
					debug++;
				L0_cache->write_list[current].status = link_free;
			}
			break;
		default:
			fprintf(debug_stream, "dL0(%lld) ERROR: shouldn't enter this point, xaction id %#06x, clock: 0x%04llx\n", mhartid, bus2_data_read->xaction_id, clock);
			break;
		}
	}
}
// output: physical_bus_data (data to load/store), bus2_addr(request to L2) , bus2 data write/snoop_response
// input: 
//			core:	logical addr, store_data, 
//			TLB:	physical addr, logical data (data out of TLB), bus2_data_in (data from L2)
//	clock
// variables: L0_cache
// debug:|debug_stream
//
// need to narrow port to L2 to 1 addr/data path, not 4 
void L0_data_cache_unit(data_bus_type* physical_bus_data, bus_w_snoop_signal1* bus2, addr_bus_type* logical_addr, data_bus_type* store_data, addr_bus_type* physical_addr, data_bus_type* TLB_response, UINT mhartid,
	UINT64 clock, L0_data_cache_type* L0_cache, L2_cache_type* L2_cache_var, UINT debug_core, param_type *param, FILE* debug_stream) {
	int debug = 0;
	UINT debug_unit = (param->L0D || param->caches) && debug_core;
	UINT debug_unit_walk = (param->PAGE_WALK || param->L0D || param->caches) && debug_core;
	UINT debug_unit_ext_snoop = (param->L0D || param->caches || param->EXT_SNOOP) && debug_core;

	if (mhartid == 3) {
		if (clock == 0x00b4)
			debug++;
		if (clock == 0x1822)
			debug++;
	}
	for (UINT set = 0; set < 0x20; set++)		L0_cache->entry[set].in_use = 0;

	if (((L0_cache->delayed_bus2write_stop + 1) & 7) == L0_cache->delayed_bus2write_start || ((L0_cache->delayed_bus2write_stop + 2) & 7) == L0_cache->delayed_bus2write_start) {
		physical_bus_data[3].snoop_response = snoop_stall;
		if (debug_unit) {
			fprintf(debug_stream, "dL0(%lld) data write snoop stall issued clock: 0x%04llx\n", mhartid, clock);
		}
	}

	if (L0_cache->store_data_latch_start != L0_cache->store_data_latch_stop && bus2->data_read.in.snoop_response != snoop_stall && bus2->data_write.out.snoop_response == snoop_idle) {
		if (L0_cache->store_data_latch[L0_cache->store_data_latch_start].cacheable == page_non_cache) {
			copy_data_bus_info(&bus2->data_write.out, &L0_cache->store_data_latch[L0_cache->store_data_latch_start]);
			L0_cache->store_data_latch[L0_cache->store_data_latch_start].snoop_response = snoop_idle;
			L0_cache->store_data_latch_start = ((L0_cache->store_data_latch_start + 1) & 3);
			if (debug_unit) {
				fprintf(debug_stream, "dL0(%lld) xaction id 0x%04x forward data to L2 clock: 0x%04llx\n", mhartid, bus2->data_write.out.xaction_id, clock);
			}
		}
	}
	for (UINT8 port = 0; port < 4; port++) {
		if (L0_cache->l_addr_latch_stall[port] == 1) {
			UINT8 set = ((L0_cache->l_addr_latch[port].addr >> 7) & 0x1f);
			for (UINT8 way = 0; way < 8 && physical_bus_data[port].snoop_response != snoop_hit && physical_bus_data[port].snoop_response != snoop_dirty && physical_bus_data[port].snoop_response != snoop_stall; way++) {
				if ((L0_cache->l_addr_latch[port].addr >> 12) == (L0_cache->entry[set].way[way].tag) && L0_cache->entry[set].way[way].state != invalid_line) {
					physical_bus_data[port].xaction_id = L0_cache->l_addr_latch[port].xaction_id;

					L0_cache->l_addr_latch_stall[port] = 0;
					physical_bus_data[port].snoop_response = (L0_cache->entry[set].way[way].state == modified_line) ? snoop_dirty : snoop_hit;
					for (UINT8 i = 0; i < 0x10; i++) 	 physical_bus_data[port].data[i] = L0_cache->entry[set].way[way].line[i];
					if (debug_unit) {
						fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d, xaction id %#06x, cache hit(2) cycle: addr, data: 0x%016I64x, 0x%016I64x",
							mhartid, set, way, L0_cache->l_addr_latch[port].xaction_id, L0_cache->l_addr_latch[port].addr, physical_bus_data[port].data[(L0_cache->l_addr_latch[port].addr >> 3) & 0x0f]);
						print_xaction(L0_cache->l_addr_latch[port].xaction, debug_stream);
						fprintf(debug_stream, " clock: 0x%04llx\n", clock);
					}
					L0_cache->l_addr_latch[port].strobe = 0;
					L0_cache->entry[set].in_use = 1;
				}
			}
		}
	}

	// cache flush logic - NOTE: TLB must issue snoop stalls until cache flush is complete - hit or miss response would be invalid
	dL0_flush(&bus2->addr.out, L0_cache, mhartid, clock, debug_unit_walk, debug_stream);
	// external snoop block
	dL0_snoop_addr_to_data_write(&bus2->data_write.out, &L0_cache->ctrl_addr, &bus2->snoop_addr.in, L0_cache->entry, L0_cache->reg, mhartid, clock, debug_unit_ext_snoop, debug_stream);

	dL0_evict_data_to_L2(&bus2->data_write.out, L0_cache, physical_bus_data, mhartid, clock, debug_unit, debug_stream);

	dL0_store_data_to_array_and_to_L2(bus2, L0_cache, store_data, physical_bus_data, mhartid, clock, debug_unit, debug_stream);
	// data read block
	dL0_incoming_read_data(&bus2->addr.out, &bus2->data_write.out, L0_cache, &bus2->data_read.in, physical_bus_data, mhartid, clock, debug_unit, debug_unit_walk, L2_cache_var, param, debug_stream);

	for (UINT8 i = 0; i < 4; i++) {
		if (L0_cache->write_list[i].status == link_issued_forward1)
			L0_cache->write_list[i].status = link_issued_forward2;
		if (L0_cache->write_list[i].status == link_issued_forward)
			L0_cache->write_list[i].status = link_issued_forward1;
	}
	UINT mask = 0x23;
	if (bus2->addr.out.strobe == 0) {
		for (UINT8 j = 0; j < 4; j++) {
			if (L0_cache->alloc_list[j].status == link_hold) {
				copy_addr_info(&bus2->addr.out, &L0_cache->alloc_list[j].latch);
				L0_cache->alloc_list[j].status = link_issued_forward;
				L0_cache->l_addr_latch[j].strobe = 0;
				if (bus2->addr.out.xaction == bus_LR_aq_rl)
					L0_cache->reg |= 4;
				if (debug_unit) {
					fprintf(debug_stream, "dL0(%lld) xaction id %#06x, ", mhartid, bus2->addr.out.xaction_id);
					if (bus2->addr.out.xaction == bus_allocate)
						fprintf(debug_stream, "ALLOC from hold to L2 addr: 0x%016I64x, ", bus2->addr.out.addr);
					else if (bus2->addr.out.xaction == bus_LR_aq_rl)
						fprintf(debug_stream, "locked READ from hold to L2 addr: 0x%016I64x, ", bus2->addr.out.addr);
					else
						fprintf(debug_stream, "ERROR: unknown transaction type from hold to L2 addr: 0x%016I64x, ", bus2->addr.out.addr);
					fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", j, clock);
				}
			}
		}
	}
	// snoop addr, load, alloc, write
	for (UINT8 port = 0; port < 4; port++) {
		if (logical_addr[port].strobe) {
			copy_addr_bus_info(&L0_cache->l_addr_latch[port], &logical_addr[port], clock);
			if (physical_addr[port].strobe != 1)
				debug++;
		}
		if (physical_addr[port].strobe) {
			copy_addr_bus_info(&L0_cache->p_addr_latch[port], &physical_addr[port], clock);
			L0_cache->l_data_latch[port].snoop_response = TLB_response[port].snoop_response;
			L0_cache->l_data_latch[port].cacheable = TLB_response[port].cacheable;
		}
	}
	if (L0_cache->delayed_addr_latch.strobe == 1 && bus2->addr.out.strobe == 0) {
		copy_addr_bus_info(&bus2->addr.out, &L0_cache->delayed_addr_latch, clock);
		UINT8 current = ((L0_cache->delayed_addr_latch.xaction_id >> 12) & 3);
		if (debug_unit) {
			fprintf(debug_stream, "dL0(%lld) q_id 0x%02x xaction id %#06x, ", mhartid, current, bus2->addr.out.xaction_id);
			print_xaction2(bus2->addr.out.xaction, debug_stream);
			fprintf(debug_stream, " cacheable forwarded to L2 (delayed); physical addr ");
			fprint_addr_coma(debug_stream, bus2->addr.out.addr, param);
			fprintf(debug_stream, " logical addr");
			fprint_addr_coma(debug_stream, L0_cache->delayed_addr_latch.addr, param);
			fprintf(debug_stream, "  list_ptr: 0x%02x,clock: 0x%04llx\n", current, clock);
		}
		switch (((L0_cache->delayed_addr_latch.xaction_id >> 8) & 3)) {
		case 1:
			L0_cache->read_list[current].status = link_issued_forward;
			copy_addr_bus_info(&L0_cache->read_list[current].latch, &L0_cache->delayed_addr_latch, clock);
			break;
		case 2:
			L0_cache->alloc_list[current].status = link_issued_forward;
			copy_addr_bus_info(&L0_cache->alloc_list[current].latch, &L0_cache->delayed_addr_latch, clock);
			break;
		case 3:
			L0_cache->write_list[current].status = link_issued_forward;
			copy_addr_bus_info(&L0_cache->write_list[current].latch, &L0_cache->delayed_addr_latch, clock);
			break;
		default:
			break;
		}
		L0_cache->delayed_addr_latch.strobe = 0;
	}
	if (mhartid == 0) {
		if (clock == 0x0b4b)
			debug++;
	}
	for (UINT8 port = 1; port < 4; port++) {
		if (L0_cache->l_addr_latch[port].strobe && TLB_response[port].snoop_response != snoop_stall && physical_bus_data[port].snoop_response == snoop_idle) {
			UINT8 current = ((L0_cache->l_addr_latch[port].xaction_id >> 12) & 3);
			if (TLB_response[port].snoop_response == snoop_miss) {
				if (debug_unit) {
					fprintf(debug_stream, "dL0(%lld) xaction id %#06x, Page Miss detected, abort; clock: 0x%04llx\n", mhartid, TLB_response[port].xaction_id, clock);
				}
				L0_cache->l_addr_latch[port].strobe = 0;
			}
			else if (TLB_response[port].cacheable == page_non_cache || TLB_response[port].cacheable == page_IO ||
				L0_cache->l_addr_latch[port].xaction == bus_LR_aq_rl || L0_cache->l_addr_latch[port].xaction == bus_LR_rl || L0_cache->l_addr_latch[port].xaction == bus_SC_aq) {
				if (L0_cache->l_addr_latch[port].xaction == bus_SC_aq || L0_cache->l_addr_latch[port].xaction == bus_SC_aq_rl) {
					copy_addr_bus_info(&bus2->addr.out, &L0_cache->l_addr_latch[port], clock);
					bus2->addr.out.cacheable = L0_cache->p_addr_latch[port].cacheable;
					if (debug_unit) {
						fprintf(debug_stream, "dL0(%lld) xaction id %#06x, ", mhartid, bus2->addr.out.xaction_id);
					}
					if (debug_unit) {
						fprintf(debug_stream, "locked write addr: 0x%016I64x, ", bus2->addr.out.addr);
						print_xaction2(L0_cache->l_addr_latch[port].xaction, debug_stream);
						fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
					}
					L0_cache->write_list[current].status = link_issued_forward;
					copy_addr_bus_info(&L0_cache->write_list[current].latch, &L0_cache->l_addr_latch[port], clock);
				}
				else if (L0_cache->l_addr_latch[port].xaction == bus_LR_aq_rl || L0_cache->l_addr_latch[port].xaction == bus_LR_rl) {
					if (L0_cache->l_addr_latch[port].cacheable != page_non_cache) { // locked cycles cannot be cached as modified, by definition
						int set = (L0_cache->l_addr_latch[port].addr >> 7) & 0x0000001f;
						cache_8way_type* cache_entry = &L0_cache->entry[set];
						UINT8 hit = 0;
						for (UINT8 way = 0; way < 8; way++) {
							if (cache_entry->way[way].tag == (L0_cache->l_addr_latch[port].addr >> 12)) {
								hit = 1;
								switch (cache_entry->way[way].state) {
								case shared_line:
									physical_bus_data[port].snoop_response = snoop_hit;
									physical_bus_data[port].xaction_id = L0_cache->l_addr_latch[port].xaction_id;
									//				physical_bus_data[port].cacheable = L0_cache->l_addr_latch[port].cacheable;
									physical_bus_data[port].cacheable = TLB_response[port].cacheable;
									physical_bus_data[port].valid = -1;
									for (UINT8 i = 0; i < 0x10; i++) 	 physical_bus_data[port].data[i] = cache_entry->way[way].line[i];
									if (debug_unit) {
										fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d S addr, data: 0x%016I64x, 0x%016I64x xaction id %#06x, ",
											mhartid, set, way, L0_cache->l_addr_latch[port].addr, physical_bus_data[port].data[(L0_cache->l_addr_latch[port].addr >> 3) & 0x0f], L0_cache->l_addr_latch[port].xaction_id);
										fprintf(debug_stream, "locked read hold, cache hit ");
										print_xaction2(L0_cache->l_addr_latch[port].xaction, debug_stream);
										fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
									}
									break;
								case modified_line:
									copy_addr_bus_info(&bus2->addr.out, &L0_cache->l_addr_latch[port], clock);
									bus2->addr.out.cacheable = L0_cache->p_addr_latch[port].cacheable;
									if (debug_unit) {
										fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d M addr: 0x%016I64x, xaction id %#06x, ",
											mhartid, set, way, L0_cache->l_addr_latch[port].addr, L0_cache->l_addr_latch[port].xaction_id);
										fprintf(debug_stream, "locked read hold, cache hit, go to L2 ");
										print_xaction2(L0_cache->l_addr_latch[port].xaction, debug_stream);
										fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
									}
									break;
								case exclusive_line:
									copy_addr_bus_info(&bus2->addr.out, &L0_cache->l_addr_latch[port], clock);
									bus2->addr.out.cacheable = L0_cache->p_addr_latch[port].cacheable;
									if (debug_unit) {
										fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d E addr: 0x%016I64x, xaction id %#06x, ",
											mhartid, set, way, L0_cache->l_addr_latch[port].addr, L0_cache->l_addr_latch[port].xaction_id);
										fprintf(debug_stream, "locked read hold, cache hit, go to L2 ");
										print_xaction2(L0_cache->l_addr_latch[port].xaction, debug_stream);
										fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
									}
									break;
								case invalid_line:
									copy_addr_bus_info(&bus2->addr.out, &L0_cache->l_addr_latch[port], clock);
									bus2->addr.out.cacheable = L0_cache->p_addr_latch[port].cacheable;
									if (debug_unit) {
										fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d I addr: 0x%016I64x, xaction id %#06x, ",
											mhartid, set, way, L0_cache->l_addr_latch[port].addr, L0_cache->l_addr_latch[port].xaction_id);
										fprintf(debug_stream, "locked read hold, cache hit, go to L2 ");
										print_xaction2(L0_cache->l_addr_latch[port].xaction, debug_stream);
										fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
									}
									break;
								default:
									copy_addr_bus_info(&bus2->addr.out, &L0_cache->l_addr_latch[port], clock);
									bus2->addr.out.cacheable = L0_cache->p_addr_latch[port].cacheable;
									if (debug_unit) {
										fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d (ERROR) addr: 0x%016I64x, xaction id %#06x, ",
											mhartid, set, way, L0_cache->l_addr_latch[port].addr, L0_cache->l_addr_latch[port].xaction_id);
										fprintf(debug_stream, "locked read hold, cache hit, go to L2 ");
										print_xaction2(L0_cache->l_addr_latch[port].xaction, debug_stream);
										fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
									}
									break;
								}
							}
						}
						// check for cache hit
						if (!hit) {
							cache_entry->way_ptr = ((cache_entry->way_ptr + 1) & 7);
							if (cache_entry->way[cache_entry->way_ptr].state == modified_line) {
								// ERROR: need to evict to L2 before overwriting
								if (debug_unit) {
									fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d addr: 0x%016I64x, xaction id %#06x, ",
										mhartid, set, cache_entry->way_ptr, L0_cache->l_addr_latch[port].addr, L0_cache->l_addr_latch[port].xaction_id);
									fprintf(debug_stream, "locked read hold, evict modified line to L2, ");
									print_xaction2(L0_cache->l_addr_latch[port].xaction, debug_stream);
									fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
								}
								if (bus2->addr.out.strobe == 0) {
									bus2->addr.out.strobe = 1;
									bus2->addr.out.addr = cache_entry->way[cache_entry->way_ptr].tag << 12;
									bus2->addr.out.xaction_id = (cache_entry->way_ptr << 12) | (3 << 8) | mhartid;
									bus2->addr.out.xaction = bus_store_full;
									bus2->addr.out.cacheable = page_rx;

									if (debug_unit) {
										fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d addr: %#016x, xaction id %#06x, cacheable bus_LR modified line eviction from L0D to L2, clock: 0x%04llx\n",
											mhartid, set, cache_entry->way_ptr, bus2->addr.out.addr, bus2->addr.out.xaction_id, clock);
									}
									if (((L0_cache->delayed_bus2write_stop + 1) & 7) == L0_cache->delayed_bus2write_start)
										debug++;
									if (physical_bus_data[3].snoop_response != snoop_idle && physical_bus_data[3].snoop_response != snoop_stall)
										debug++;
									physical_bus_data[3].snoop_response = snoop_stall;
									physical_bus_data[port].cacheable = TLB_response[port].cacheable;
									L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].snoop_response = snoop_dirty;
									L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].cacheable = bus2->addr.out.cacheable;
									L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].xaction_id = bus2->addr.out.xaction_id;
									L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].valid = -1;
									for (UINT8 i = 0; i < 0x10; i++) L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].data[i] = cache_entry->way[cache_entry->way_ptr].line[i];
									L0_cache->delayed_bus2write_stop = ((L0_cache->delayed_bus2write_stop + 1) & 7);
								}
								else {
									if (L0_cache->delayed_addr_latch.strobe == 1)
										debug++;
									L0_cache->delayed_addr_latch.strobe = 1;
									L0_cache->delayed_addr_latch.addr = cache_entry->way[cache_entry->way_ptr].tag << 12;
									L0_cache->delayed_addr_latch.xaction_id = L0_cache->l_addr_latch[port].xaction_id | (3 << 8);
									L0_cache->delayed_addr_latch.xaction = bus_store_full;
									L0_cache->delayed_addr_latch.cacheable = L0_cache->l_addr_latch[port].cacheable;
									if (debug_unit) {
										fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d addr: %#016x, xaction id %#06x, cacheable bus_LR modified line eviction from L0D to hold, clock: 0x%04llx\n",
											mhartid, set, cache_entry->way_ptr, L0_cache->delayed_addr_latch.addr, L0_cache->delayed_addr_latch.xaction_id, clock);
									}
									if (((L0_cache->delayed_bus2write_stop + 1) & 7) == L0_cache->delayed_bus2write_start)
										debug++;
									if (physical_bus_data[3].snoop_response != snoop_idle)
										debug++;
									physical_bus_data[3].snoop_response = snoop_stall;
									physical_bus_data[port].cacheable = TLB_response[port].cacheable;
									L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].snoop_response = snoop_dirty;
									L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].cacheable = L0_cache->delayed_addr_latch.cacheable;
									L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].xaction_id = L0_cache->delayed_addr_latch.xaction_id;
									L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].valid = -1;
									for (UINT8 i = 0; i < 0x10; i++) L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].data[i] = cache_entry->way[cache_entry->way_ptr].line[i];
									L0_cache->delayed_bus2write_stop = ((L0_cache->delayed_bus2write_stop + 1) & 7);
								}

								L0_cache->alloc_list[current].status = link_hold;
								copy_addr_bus_info(&L0_cache->alloc_list[current].latch, &L0_cache->l_addr_latch[port], clock);
							}
							else {
								if (bus2->addr.out.strobe == 1 || bus2->data_read.in.snoop_response == snoop_stall)
									debug++;
								copy_addr_bus_info(&bus2->addr.out, &L0_cache->l_addr_latch[port], clock);
								bus2->addr.out.cacheable = L0_cache->p_addr_latch[port].cacheable;
								if (debug_unit) {
									fprintf(debug_stream, "dL0(%lld) addr: 0x%016I64x, xaction id %#06x, locked read cacheable forward to L2  ", mhartid, bus2->addr.out.addr, L0_cache->l_addr_latch[port].xaction_id);
									print_xaction2(L0_cache->l_addr_latch[port].xaction, debug_stream);
									fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
								}
								L0_cache->alloc_list[current].status = link_issued_forward;
								copy_addr_bus_info(&L0_cache->alloc_list[current].latch, &L0_cache->l_addr_latch[port], clock);
							}
						}
					}
					else {
						if (bus2->addr.out.strobe == 1 || bus2->data_read.in.snoop_response == snoop_stall)
							debug++;
						copy_addr_bus_info(&bus2->addr.out, &L0_cache->l_addr_latch[port], clock);
						bus2->addr.out.cacheable = L0_cache->p_addr_latch[port].cacheable;
						if (debug_unit) {
							fprintf(debug_stream, "dL0(%lld) xaction id %#06x,locked read noncacheable forward to L2 addr: 0x%016I64x, ", mhartid, L0_cache->l_addr_latch[port].xaction_id, bus2->addr.out.addr);
							print_xaction2(L0_cache->l_addr_latch[port].xaction, debug_stream);
							fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
						}
						L0_cache->alloc_list[current].status = link_issued_forward;
						copy_addr_bus_info(&L0_cache->alloc_list[current].latch, &L0_cache->l_addr_latch[port], clock);
					}
				}
				else {
					if (bus2->addr.out.strobe == 0 && bus2->data_read.in.snoop_response != snoop_stall) {
						copy_addr_bus_info(&bus2->addr.out, &L0_cache->l_addr_latch[port], clock);
						bus2->addr.out.cacheable = L0_cache->p_addr_latch[port].cacheable;
						if (debug_unit) {
							fprintf(debug_stream, "dL0(%lld) xaction id %#06x, ", mhartid, bus2->addr.out.xaction_id);
							fprintf(debug_stream, "addr: 0x%016I64x, ", bus2->addr.out.addr);
							print_xaction2(L0_cache->l_addr_latch[port].xaction, debug_stream);
							fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
						}
					}
					else {
						if (L0_cache->delayed_addr_latch.strobe == 1)
							debug++;
						copy_addr_bus_info(&L0_cache->delayed_addr_latch, &L0_cache->l_addr_latch[port], clock);
						if (debug_unit) {
							fprintf(debug_stream, "dL0(%lld) xaction id %#06x, ", mhartid, L0_cache->delayed_addr_latch.xaction_id);
							fprintf(debug_stream, "addr: 0x%016I64x, delayed addr latch: ", L0_cache->delayed_addr_latch.addr);
							print_xaction2(L0_cache->l_addr_latch[port].xaction, debug_stream);
							fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
						}
					}
					L0_cache->read_list[current].status = link_issued_forward;
					copy_addr_bus_info(&L0_cache->read_list[current].latch, &L0_cache->l_addr_latch[port], clock);
				}
				L0_cache->l_addr_latch[port].strobe = 0;
			}
			else if (L0_cache->l_addr_latch[port].xaction == bus_store_full || L0_cache->l_addr_latch[port].xaction == bus_store_partial) {// non cacheable already filtered out
				L0_cache->write_list[current].status = link_hold;
				copy_addr_bus_info(&L0_cache->write_list[current].latch, &L0_cache->l_addr_latch[port], clock);
				L0_cache->l_addr_latch[port].strobe = 0;

				if (L0_cache->l_addr_latch[port].xaction == bus_store_full) {
					int set = (L0_cache->write_list[current].latch.addr >> 7) & 0x0000001f;
					cache_8way_type* cache_entry = &L0_cache->entry[set];
					UINT hit = 0;
					for (UINT8 way = 0; way < 8 && !hit; way++) {
						if (cache_entry->way[way].tag == (L0_cache->write_list[current].latch.addr >> 12)) {
							cache_entry->way_ptr = way;
							hit = 1;
						}
					}
					if (!hit && cache_entry->way[cache_entry->way_ptr].state != invalid_line) {
						for (UINT8 way = 0; way < 8 && cache_entry->way[cache_entry->way_ptr].state != invalid_line; way++) {
							if (cache_entry->way[way].state == invalid_line) {
								cache_entry->way_ptr = way;
							}
						}
					}
					if (cache_entry->way[cache_entry->way_ptr].state == modified_line && !hit) {
						if (bus2->addr.out.strobe == 1 || bus2->data_read.in.snoop_response == snoop_stall)
							debug++;
						else {
							bus2->addr.out.strobe = 1;
							bus2->addr.out.addr = (cache_entry->way[cache_entry->way_ptr].tag << 12);
							bus2->addr.out.xaction_id = 0x0300 | (cache_entry->way_ptr << 12);
							bus2->addr.out.xaction = bus_store_full;
							bus2->addr.out.cacheable = page_rx;
							cache_entry->way[cache_entry->way_ptr].state = shared_line;
							if (((L0_cache->delayed_bus2write_stop + 1) & 7) == L0_cache->delayed_bus2write_start)
								debug++;
							if (physical_bus_data[port].snoop_response != snoop_idle)
								debug++;
							physical_bus_data[port].snoop_response = snoop_stall;
							physical_bus_data[port].cacheable = TLB_response[port].cacheable;
							L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].snoop_response = snoop_dirty;
							L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].xaction_id = bus2->addr.out.xaction_id;
							L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].valid = -1;
							L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].cacheable = page_rx;
							for (UINT8 i = 0; i < 0x10; i++)L0_cache->delayed_bus2write[L0_cache->delayed_bus2write_stop].data[i] = cache_entry->way[cache_entry->way_ptr].line[i];
							L0_cache->delayed_bus2write_stop = ((L0_cache->delayed_bus2write_stop + 1) & 7);
						}
					}
					cache_entry->in_use = 1;
					if (debug_unit) {
						fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d %s xaction id 0x%04x q_id 0x%02x, ",
							mhartid, set, L0_cache->entry[set].way_ptr,
							(cache_entry->way[cache_entry->way_ptr].state == modified_line) ? "M" :
							(cache_entry->way[cache_entry->way_ptr].state == exclusive_line) ? "E" :
							(cache_entry->way[cache_entry->way_ptr].state == shared_line) ? "S" : "I",
							L0_cache->l_addr_latch[port].xaction_id, current);
						fprintf(debug_stream, " write addr arrive to hold (lock); physical address: 0x%016I64x, logical address: 0x%016I64x, clock: 0x%04llx\n",
							L0_cache->l_addr_latch[port].addr, L0_cache->l_addr_latch[port].addr, clock);
						if (bus2->addr.out.strobe && bus2->addr.out.cacheable != page_non_cache) {
							fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d, q_id 0x%02x, xaction id %#06x, ", mhartid, set, L0_cache->entry[set].way_ptr, current, bus2->addr.out.xaction_id);
							fprintf(debug_stream, " line eviction; p-addr 0x%016I64x, clock: 0x%04llx\n", bus2->addr.out.addr, clock);
						}
					}
				}
				else {
					if (debug_unit) {
						fprintf(debug_stream, "dL0(%lld) q_id 0x%02x, xaction id %#06x, ", mhartid, current, L0_cache->l_addr_latch[port].xaction_id);
						fprintf(debug_stream, " write addr arrive to hold; physical address: 0x%016I64x, logical address: 0x%016I64x, list_ptr: 0x%02x,clock: 0x%04llx\n", L0_cache->l_addr_latch[port].addr, L0_cache->l_addr_latch[port].addr, current, clock);
					}
				}
			}
			else if (L0_cache->l_data_latch[port].snoop_response == snoop_hit || L0_cache->l_addr_latch[port].cacheable != page_non_cache) {
				UINT8 set = ((L0_cache->l_addr_latch[port].addr >> 7) & 0x1f);
				physical_bus_data[port].snoop_response = snoop_miss;
				physical_bus_data[port].cacheable = TLB_response[port].cacheable;
				if (clock >= 0x0764)
					debug++;
				for (UINT8 way = 0; way < 8 && physical_bus_data[port].snoop_response == snoop_miss; way++) {
					if ((L0_cache->l_addr_latch[port].addr >> 12) == (L0_cache->entry[set].way[way].tag) && L0_cache->entry[set].way[way].state != invalid_line) {
						physical_bus_data[port].xaction_id = L0_cache->l_addr_latch[port].xaction_id;
						if (L0_cache->entry[set].in_use == 1) {
							L0_cache->l_addr_latch_stall[port] = 1;
							physical_bus_data[port].snoop_response = snoop_stall;
							if (debug_unit) {
								fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d addr: 0x%016I64x", mhartid, set, way, L0_cache->l_addr_latch[port].addr);
								if (L0_cache->entry[set].way[way].state == modified_line)		fprintf(debug_stream, " M ");
								else if (L0_cache->entry[set].way[way].state == shared_line)	fprintf(debug_stream, " S ");
								else															fprintf(debug_stream, " Unk (ERROR) ");
								fprintf(debug_stream, " xaction id %#06x, cache set locked; stall cycle:",
									L0_cache->l_addr_latch[port].xaction_id);
								print_xaction(L0_cache->l_addr_latch[port].xaction, debug_stream);
								fprintf(debug_stream, " clock: 0x%04llx\n", clock);
							}
						}
						else {
							L0_cache->l_addr_latch_stall[port] = 0;
							if (L0_cache->l_addr_latch[port].xaction == bus_allocate) {
								physical_bus_data[port].snoop_response = (L0_cache->entry[set].way[way].state == modified_line) ? snoop_dirty :
									(L0_cache->entry[set].way[way].state == exclusive_line) ? snoop_hit : snoop_miss;
							}
							else {
								physical_bus_data[port].snoop_response = (L0_cache->entry[set].way[way].state == modified_line) ? snoop_dirty : snoop_hit;
								L0_cache->l_addr_latch[port].strobe = 0;
								L0_cache->entry[set].in_use = 1;
							}
							for (UINT8 i = 0; i < 0x10; i++) 	 physical_bus_data[port].data[i] = L0_cache->entry[set].way[way].line[i];
							physical_bus_data[port].valid = -1;
							if (debug_unit) {
								fprintf(debug_stream, "dL0(%lld) set.way 0x%02x.%d", mhartid, set, way);
								if (L0_cache->entry[set].way[way].state == modified_line)		fprintf(debug_stream, "M ");
								else if (L0_cache->entry[set].way[way].state == exclusive_line)	fprintf(debug_stream, "E ");
								else if (L0_cache->entry[set].way[way].state == shared_line)	fprintf(debug_stream, "S ");
								else															fprintf(debug_stream, "Unk (ERROR) ");
								fprintf(debug_stream, "addr, data: 0x%016I64x, 0x%016I64x xaction id %#06x, ",
									L0_cache->l_addr_latch[port].addr, physical_bus_data[port].data[(L0_cache->l_addr_latch[port].addr >> 3) & 0x0f], L0_cache->l_addr_latch[port].xaction_id);
								print_xaction(L0_cache->l_addr_latch[port].xaction, debug_stream);
								fprintf(debug_stream, "cache hit cycle; clock: 0x%04llx\n", clock);
							}
						}
					}
				}
				if (physical_bus_data[port].snoop_response == snoop_miss) {
					if (bus2->addr.out.strobe == 1 || bus2->data_read.in.snoop_response == snoop_stall) {
						copy_addr_bus_info(&L0_cache->delayed_addr_latch, &L0_cache->l_addr_latch[port], clock);
						L0_cache->l_addr_latch[port].strobe = 0;
						physical_bus_data[port].snoop_response = snoop_stall;
						L0_cache->delayed_addr_latch.addr = L0_cache->p_addr_latch[port].addr;
					}
					else {
						copy_addr_bus_info(&bus2->addr.out, &L0_cache->l_addr_latch[port], clock);
						bus2->addr.out.addr = L0_cache->p_addr_latch[port].addr;
						bus2->addr.out.cacheable = L0_cache->p_addr_latch[port].cacheable;
						if (debug_unit) {
							if (bus2->addr.out.cacheable == page_non_cache) {
								fprintf(debug_stream, "dL0(%lld) q_id 0x%02x xaction id %#06x, ", mhartid, current, bus2->addr.out.xaction_id);
								print_xaction2(bus2->addr.out.xaction, debug_stream);
								fprintf(debug_stream, " none cacheable forwarded to L2; physical addr: 0x%016I64x, logical addr: 0x%016I64x, list_ptr: 0x%02x,clock: 0x%04llx\n", bus2->addr.out.addr, L0_cache->l_addr_latch[port].addr, current, clock);
							}
							else if (bus2->addr.out.cacheable == page_IO) {
								fprintf(debug_stream, "dL0(%lld) q_id 0x%02x xaction id %#06x, ", mhartid, current, bus2->addr.out.xaction_id);
								print_xaction2(bus2->addr.out.xaction, debug_stream);
								fprintf(debug_stream, " IO access forwarded to L2; physical addr: 0x%016I64x, logical addr: 0x%016I64x, list_ptr: 0x%02x,clock: 0x%04llx\n", bus2->addr.out.addr, L0_cache->l_addr_latch[port].addr, current, clock);
							}
							else {
								fprintf(debug_stream, "dL0(%lld) q_id 0x%02x xaction id %#06x, ", mhartid, current, bus2->addr.out.xaction_id);
								print_xaction2(bus2->addr.out.xaction, debug_stream);
								fprintf(debug_stream, " cacheable forwarded to L2 (miss); physical addr: ");
								fprint_addr_coma(debug_stream, bus2->addr.out.addr,param);
								fprintf(debug_stream, "  logical addr ");
								fprint_addr_coma(debug_stream, L0_cache->l_addr_latch[port].addr, param);
								fprintf(debug_stream, "  list_ptr: 0x%02x,clock: 0x%04llx\n", current, clock);
							}
						}
						switch (((L0_cache->l_addr_latch[port].xaction_id >> 8) & 3)) {
						case 1:
							L0_cache->read_list[current].status = link_issued_forward;
							copy_addr_bus_info(&L0_cache->read_list[current].latch, &L0_cache->l_addr_latch[port], clock);
							break;
						case 2:
							L0_cache->alloc_list[current].status = link_issued_forward;
							copy_addr_bus_info(&L0_cache->alloc_list[current].latch, &L0_cache->l_addr_latch[port], clock);
							break;
						case 3:
							L0_cache->write_list[current].status = link_issued_forward;
							copy_addr_bus_info(&L0_cache->write_list[current].latch, &L0_cache->l_addr_latch[port], clock);
							break;
						default:
							break;
						}
						L0_cache->l_addr_latch[port].strobe = 0;
					}
				}
			}
			else if (bus2->addr.out.strobe == 1 || bus2->data_read.in.snoop_response == snoop_stall) {
				if (debug_unit) {
					fprintf(debug_stream, "dL0(%lld) xaction id %#06x, to Hold, bus2 addr is busy; ", mhartid, L0_cache->l_addr_latch[port].xaction_id);
					if (L0_cache->l_addr_latch[port].cacheable == page_non_cache) {
						fprintf(debug_stream, "non-cacheable ");
					}
					else {
						fprintf(debug_stream, "cacheable ");
					}
				}
				switch (L0_cache->l_addr_latch[port].xaction) {
				case bus_store_full:
				case bus_store_partial:
				case bus_SC_aq:
				case bus_SC_aq_rl:
					if (debug_unit) {
						fprintf(debug_stream, "write addr: 0x%016I64x, ", L0_cache->l_addr_latch[port].addr);
						fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
					}
					L0_cache->write_list[current].status = link_hold;
					copy_addr_bus_info(&L0_cache->write_list[current].latch, &L0_cache->l_addr_latch[port], clock);
					break;
				case bus_LR_aq_rl:
				case bus_LR_rl:
					if (debug_unit) {
						fprintf(debug_stream, "locked addr: 0x%016I64x, ", L0_cache->l_addr_latch[port].addr);
						fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
					}
					L0_cache->alloc_list[current].status = link_hold;
					copy_addr_bus_info(&L0_cache->alloc_list[current].latch, &L0_cache->l_addr_latch[port], clock);
					break;
				case bus_allocate:
					if (debug_unit) {
						fprintf(debug_stream, "allocate addr: 0x%016I64x, ", L0_cache->l_addr_latch[port].addr);
						fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
					}
					L0_cache->alloc_list[current].status = link_hold;
					copy_addr_bus_info(&L0_cache->alloc_list[current].latch, &L0_cache->l_addr_latch[port], clock);
					break;
				case bus_load:
					if (debug_unit) {
						fprintf(debug_stream, "read addr: 0x%016I64x, ", L0_cache->l_addr_latch[port].addr);
						fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
					}
					L0_cache->read_list[current].status = link_hold;
					copy_addr_bus_info(&L0_cache->read_list[current].latch, &L0_cache->l_addr_latch[port], clock);
					break;
				default:
					debug++;
					if (debug_unit) {
						fprintf(debug_stream, "addr: 0x%016I64x, ", L0_cache->l_addr_latch[port].addr);
						print_xaction2(L0_cache->l_addr_latch[port].xaction, debug_stream);
						fprintf(debug_stream, " list_ptr: 0x%02x, clock: 0x%04llx\n", current, clock);
					}
					L0_cache->read_list[current].status = link_hold;
					copy_addr_bus_info(&L0_cache->read_list[current].latch, &L0_cache->l_addr_latch[port], clock);
					break;
				}
				L0_cache->l_addr_latch[port].strobe = 0;
			}
			else if (L0_cache->l_addr_latch[port].xaction == bus_store_partial && bus2->addr.out.strobe == 0) {// non cacheable already filtered out
				if (bus2->addr.out.strobe == 1 || bus2->data_read.in.snoop_response == snoop_stall)
					debug++;
				copy_addr_bus_info(&bus2->addr.out, &L0_cache->l_addr_latch[port], clock);
				bus2->addr.out.addr = L0_cache->p_addr_latch[port].addr;

				if (debug_unit) {
					fprintf(debug_stream, "dL0(%lld) xaction id %#06x, ", mhartid, L0_cache->l_addr_latch[port].xaction_id);
					fprintf(debug_stream, " write partial to L2; physical address: 0x%016I64x, logical address: 0x%016I64x, list_ptr: 0x%02x,clock: 0x%04llx\n", L0_cache->l_addr_latch[port].addr, L0_cache->l_addr_latch[port].addr, current, clock);
				}
				L0_cache->write_list[current].status = link_issued_forward;
				copy_addr_bus_info(&L0_cache->write_list[current].latch, &L0_cache->l_addr_latch[port], clock);
				L0_cache->l_addr_latch[port].strobe = 0;
			}
			else {
				debug++;
			}
		}
	}
	// END: data read block
	if (bus2->addr.out.strobe == 0) {
		UINT hit = 0;
		for (UINT current = 0; current < 4 && !hit; current++) {
			if (L0_cache->write_list[current].latch.cacheable == page_non_cache && (L0_cache->write_list[current].status == link_hold_w || L0_cache->write_list[current].status == link_issued_forward) &&
				(L0_cache->write_list[current].latch.xaction == bus_store_full || L0_cache->write_list[current].latch.xaction == bus_store_partial)) {
				bus2->addr.out.strobe = 1;
				bus2->addr.out.addr = L0_cache->write_list[current].latch.addr;
				bus2->addr.out.xaction_id = L0_cache->write_list[current].latch.xaction_id;
				bus2->addr.out.xaction = L0_cache->write_list[current].latch.xaction;
				bus2->addr.out.cacheable = L0_cache->write_list[current].latch.cacheable;
				L0_cache->write_list[current].status = link_issued_forward;
				if (debug_unit) {
					fprintf(debug_stream, "dL0(%lld) xaction id %#06x, UC WRITE addr to L2, valid: 0x%016I64x, clock: 0x%04llx\n", mhartid, bus2->addr.out.xaction_id, store_data->valid, clock);
				}
				hit = 1;
			}
		}
		for (UINT current = 0; current < 4 && !hit; current++) {
			if (L0_cache->write_list[current].status == link_hold && !(L0_cache->write_list[current].latch.xaction == bus_store_full || L0_cache->write_list[current].latch.xaction == bus_store_partial)) {
				bus2->addr.out.strobe = 1;
				bus2->addr.out.addr = L0_cache->write_list[current].latch.addr;
				bus2->addr.out.xaction_id = L0_cache->write_list[current].latch.xaction_id;
				bus2->addr.out.xaction = L0_cache->write_list[current].latch.xaction;
				bus2->addr.out.cacheable = L0_cache->write_list[current].latch.cacheable;
				L0_cache->write_list[current].status = link_issued_forward;
				if (debug_unit) {
					fprintf(debug_stream, "dL0(%lld) xaction id %#06x, cacheable addr to L2, valid: 0x%016I64x, clock: 0x%04llx\n", mhartid, bus2->addr.out.xaction_id, store_data->valid, clock);
				}
				hit = 1;
			}
		}
	}
	if (L0_cache->write_data_fifo[0].snoop_response != snoop_idle) {
		if (bus2->data_write.out.snoop_response == snoop_idle) {
			copy_data_bus_info(&bus2->data_write.out, &L0_cache->write_data_fifo[0]);
			for (UINT i = 0; i < 3; i++) copy_data_bus_info(&L0_cache->write_data_fifo[i], &L0_cache->write_data_fifo[i + 1]);
			L0_cache->write_data_fifo[3].snoop_response = snoop_idle;
			if (debug_unit) {
				if (bus2->data_write.out.cacheable == page_IO)
					fprintf(debug_stream, "dL0(%lld) xaction id %#06x, IO data forwarded to L2:  0x%016I64x clock: 0x%04llx\n",
						mhartid, bus2->data_write.out.xaction_id, bus2->data_write.out.data[0], clock);
				else if (bus2->data_write.out.cacheable == page_non_cache)
					fprintf(debug_stream, "dL0(%lld) xaction id %#06x, uncacheable data forwarded to L2:  0x%016I64x clock: 0x%04llx\n",
						mhartid, bus2->data_write.out.xaction_id, bus2->data_write.out.data[0], clock);
				else
					fprintf(debug_stream, "dL0(%lld) xaction id %#06x, data forwarded to L2 clock: 0x%04llx\n",
						mhartid, bus2->data_write.out.xaction_id, clock);
			}
		}
		else {
			debug++;
		}
	}
}