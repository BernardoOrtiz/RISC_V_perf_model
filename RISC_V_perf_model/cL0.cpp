// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "cache.h"

// output: physical_bus (data to prefetcher), &bus2->addr.out(request to L2) , snoop_response
// input: logical addr, physical addr (addr out of TLB), logical data (data out of TLB), bus1_data_in (data from L2), clock
// variables: L0_cache
// debug:|debug_stream
//
void L0_code_cache_unit(data_bus_type* physical_bus, bus_w_snoop_signal1* bus2, addr_bus_type* logical_addr, addr_bus_type* physical_addr, data_bus_type* TLB_response, L0_code_cache_type* L0_cache, UINT64 clock,
	UINT debug_core, UINT mhartid, param_type *param, FILE* debug_stream) {
	int debug = 0;

	UINT debug_unit = (param->L0C || param->caches) && debug_core;
	UINT debug_unit_ext_snoop = (param->L0C || param->caches || param->EXT_SNOOP) && debug_core;

	while (L0_cache->list[L0_cache->list_start_ptr].status == link_free && L0_cache->list_start_ptr != L0_cache->list_stop_ptr)  L0_cache->list_start_ptr = ((L0_cache->list_start_ptr + 1) & 0x1f);

	if (logical_addr->strobe) {
		copy_addr_bus_info(&L0_cache->l_addr_latch, logical_addr, clock);
		copy_addr_bus_info(&L0_cache->p_addr_latch, physical_addr, clock);
		if (physical_addr->strobe != 1)
			debug++;
	}
	if (mhartid == 1) {
		if (clock >= 0x2507)//86418280 x-action 0x1440
			debug++;
	}

	if (bus2->snoop_addr.in.strobe == 1 && bus2->snoop_addr.in.addr != io_addr_TLB_code_ctrl) {
		switch (bus2->snoop_addr.in.addr) {
		case io_addr_L0C_ctrl:
			switch (bus2->snoop_addr.in.xaction) {
			case bus_load:
				copy_addr_bus_info(&bus2->data_write.out, &bus2->snoop_addr.in);
				bus2->data_write.out.data[0] = L0_cache->reg;
				break;
			case bus_SC:
			case bus_SC_rl:
			case bus_SC_aq:
			case bus_SC_aq_rl:
				debug++;
				copy_addr_bus_info(&L0_cache->ctrl_addr, &bus2->snoop_addr.in, clock);
				break;
			default:
				debug++;
				break;
			}
			if (debug_unit) {
				fprintf(debug_stream, "cL0(%d) xaction id %#06x, L0C ctrl address rcvd, clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
			}
			break;
		case io_addr_TLB_code_ctrl:
		case io_addr_TLB_2M_code:
		case io_addr_TLB_code_vaddr:
			break;
		default: {
			cache_8way_type* cache_entry = &L0_cache->entry[(bus2->snoop_addr.in.addr >> 7) & 0x00001f];
			copy_addr_bus_info(&bus2->data_write.out, &bus2->snoop_addr.in);
			bus2->data_write.out.snoop_response = snoop_miss;// need to wait for code and data L0 response
			UINT8 hit = 0;
			for (UINT8 way = 0; way < 8; way++) {
				if (cache_entry->way[way].tag == (bus2->snoop_addr.in.addr >> 12)) {
					bus2->data_write.out.snoop_response = snoop_hit;
					cache_entry->way[way].state = (bus2->snoop_addr.in.xaction == bus_allocate) ? invalid_line : shared_line;
					bus2->data_write.out.valid = -1;
					for (UINT8 i = 0; i < 0x10; i++)bus2->data_write.out.data[i] = cache_entry->way[way].line[i];
					hit = 1;
					if (debug_unit_ext_snoop) {
						fprintf(debug_stream, "cL0(%d) set.way 0x%02x.%d xaction id %#06x, ", mhartid, (bus2->snoop_addr.in.addr >> 7) & 0x00001f, way, bus2->snoop_addr.in.xaction_id);
						fprintf(debug_stream, " SNOOP Cache Hit logical addr: 0x%016I64x, d0 0x%016I64x, clock: 0x%04llx\n",
							bus2->snoop_addr.in.addr, bus2->data_write.out.data[0], clock);
					}
				}
			}
			if (debug_unit_ext_snoop && !hit) {
				fprintf(debug_stream, "cL0(%d) xaction id %#06x, ", mhartid, bus2->snoop_addr.in.xaction_id);
				fprintf(debug_stream, " SNOOP Cache Miss logical addr ");
				fprint_addr_coma(debug_stream, bus2->snoop_addr.in.addr, param);
				fprintf(debug_stream, " clock: 0x%04llx\n", clock);
			}
		}
			   break;
		}
	}

	if (L0_cache->l_addr_latch.strobe) {
		switch (TLB_response->snoop_response) {
		case snoop_hit:
			for (UINT i = 0; i < 0x20; i++) {
				if (L0_cache->list[i].latch.xaction_id == L0_cache->l_addr_latch.xaction_id && L0_cache->list[i].status != link_free) {
					fprintf(debug_stream, "cL0(%d) xaction id %#06x ERROR: duplicate issue from time 0x%04llx,clock: 0x%04llx\n",
						mhartid, L0_cache->list[i].latch.xaction_id, L0_cache->list[i].latch.clock, clock);
				}
			}
			if (L0_cache->p_addr_latch.cacheable == page_non_cache) {// UC
				copy_addr_bus_info(&bus2->addr.out, &L0_cache->l_addr_latch, clock);
				bus2->addr.out.cacheable = L0_cache->p_addr_latch.cacheable;
				if (debug_unit) {
					fprintf(debug_stream, "cL0(%d) xaction id %#06x, ", mhartid, bus2->addr.out.xaction_id);
					print_xaction2(bus2->addr.out.xaction, debug_stream);
					fprintf(debug_stream, " UC physical address: 0x%016I64x, logical address: 0x%016I64x,clock: 0x%04llx\n", bus2->addr.out.addr, L0_cache->l_addr_latch.addr, clock);
				}
				L0_cache->list[L0_cache->list_stop_ptr].status = link_issued_forward;
				copy_addr_bus_info(&L0_cache->list[L0_cache->list_stop_ptr].latch, &L0_cache->l_addr_latch, clock);
				L0_cache->list[L0_cache->list_stop_ptr].latch.cacheable = L0_cache->p_addr_latch.cacheable;
				L0_cache->list_stop_ptr = ((L0_cache->list_stop_ptr + 1) & 0x1f);
			}
			else {
				UINT8 set = ((L0_cache->l_addr_latch.addr >> 7) & 0x1f);
				physical_bus->snoop_response = snoop_miss;
				for (UINT8 way = 0; way < 8; way++) {
					if ((L0_cache->l_addr_latch.addr >> 12) == (L0_cache->entry[set].way[way].tag)) {
						L0_cache->entry[set].way_ptr = ((way + 1) & 7);
						physical_bus->snoop_response = snoop_hit;
						physical_bus->xaction_id = L0_cache->l_addr_latch.xaction_id;
						for (UINT8 i = 0; i < 0x10; i++) 	 physical_bus->data[i] = L0_cache->entry[set].way[way].line[i];
						L0_cache->list[L0_cache->list_stop_ptr].status = link_free;

						if (debug_unit) {
							fprintf(debug_stream, "cL0(%d) set.way 0x%02x.%d xaction id 0x%04x, ", mhartid, set, way, L0_cache->l_addr_latch.xaction_id);
							print_xaction2(L0_cache->l_addr_latch.xaction, debug_stream);
							fprintf(debug_stream, " Cache Hit1 physical address: 0x%016I64x, logical address: 0x%016I64x, clock: 0x%04llx\n",
								L0_cache->l_addr_latch.addr, L0_cache->l_addr_latch.addr, clock);
							fprintf(debug_stream, "cL0(%d) xaction id 0x%04x, ", mhartid, L0_cache->l_addr_latch.xaction_id);
							fprintf(debug_stream, "0x00 0x%016I64x, 0x08 0x%016I64x, 0x10 0x%016I64x, 0x18 0x%016I64x, clock: 0x%04llx\n",
								physical_bus->data[0], physical_bus->data[1], physical_bus->data[2], physical_bus->data[3], clock);
							fprintf(debug_stream, "cL0(%d) xaction id 0x%04x, ", mhartid, L0_cache->l_addr_latch.xaction_id);
							fprintf(debug_stream, "0x20 0x%016I64x, 0x28 0x%016I64x, 0x30 0x%016I64x, 0x38 0x%016I64x, clock: 0x%04llx\n",
								physical_bus->data[4], physical_bus->data[5], physical_bus->data[6], physical_bus->data[7], clock);
							fprintf(debug_stream, "cL0(%d) xaction id 0x%04x, ", mhartid, L0_cache->l_addr_latch.xaction_id);
							fprintf(debug_stream, "0x40 0x%016I64x, 0x48 0x%016I64x, 0x50 0x%016I64x, 0x58 0x%016I64x, clock: 0x%04llx\n",
								physical_bus->data[8], physical_bus->data[9], physical_bus->data[10], physical_bus->data[11], clock);
							fprintf(debug_stream, "cL0(%d) xaction id 0x%04x, ", mhartid, L0_cache->l_addr_latch.xaction_id);
							fprintf(debug_stream, "0x60 0x%016I64x, 0x68 0x%016I64x, 0x70 0x%016I64x, 0x78 0x%016I64x, clock: 0x%04llx\n",
								physical_bus->data[12], physical_bus->data[13], physical_bus->data[14], physical_bus->data[15], clock);
						}
					}
				}
				if (physical_bus->snoop_response == snoop_miss) {
					copy_addr_bus_info(&bus2->addr.out, &L0_cache->p_addr_latch, clock);

					if (debug_unit) {
						fprintf(debug_stream, "cL0(%d) xaction id %#06x, ", mhartid, bus2->addr.out.xaction_id);
						print_xaction2(bus2->addr.out.xaction, debug_stream);
						fprintf(debug_stream, " cacheable physical address: 0x%016I64x, logical address: 0x%016I64x, clock: 0x%04llx\n", bus2->addr.out.addr, L0_cache->l_addr_latch.addr, clock);
					}
					L0_cache->list[L0_cache->list_stop_ptr].status = link_issued_forward;
					copy_addr_bus_info(&L0_cache->list[L0_cache->list_stop_ptr].latch, physical_addr, clock);
					L0_cache->list_stop_ptr = ((L0_cache->list_stop_ptr + 1) & 0x1f);
				}
			}
			bus2->addr.out.cacheable = L0_cache->p_addr_latch.cacheable;
			L0_cache->l_addr_latch.strobe = 0;
			break;
		case snoop_stall:// hold state machine
			break;
		case snoop_miss: // page fault; drop
			L0_cache->l_addr_latch.strobe = 0;
			break;
		case snoop_idle:// illegal state
			if (L0_cache->p_addr_latch.cacheable != page_invalid && L0_cache->p_addr_latch.cacheable != page_non_cache) {
				UINT8 set = ((L0_cache->l_addr_latch.addr >> 7) & 0x1f);
				physical_bus->snoop_response = snoop_miss;
				for (UINT i = 0; i < 0x20; i++) {
					if (L0_cache->list[i].latch.xaction_id == L0_cache->l_addr_latch.xaction_id && L0_cache->list[i].status != link_free) {
						fprintf(debug_stream, "cL0(%d) xaction id %#06x, ", mhartid, L0_cache->l_addr_latch.xaction_id);
						print_xaction2(L0_cache->l_addr_latch.xaction, debug_stream);
						fprintf(debug_stream, " ERROR: transaction ID not free, last used on clock 0x%04llx; current clock: 0x%04llx\n", L0_cache->list[i].latch.clock, clock);
					}
				}
				for (UINT8 way = 0; way < 8; way++) {
					if ((L0_cache->l_addr_latch.addr >> 12) == (L0_cache->entry[set].way[way].tag)) {
						L0_cache->entry[set].way_ptr = ((way + 1) & 7);
						physical_bus->snoop_response = snoop_hit;
						physical_bus->xaction_id = L0_cache->l_addr_latch.xaction_id;
						for (UINT8 i = 0; i < 0x10; i++) 	 physical_bus->data[i] = L0_cache->entry[set].way[way].line[i];
						L0_cache->list[L0_cache->list_stop_ptr].status = link_free;
						if (debug_unit) {
							fprintf(debug_stream, "cL0(%d) set.way 0x%02x.%d xaction id %#06x, ", mhartid, set, way, L0_cache->l_addr_latch.xaction_id);
							print_xaction2(L0_cache->l_addr_latch.xaction, debug_stream);
							fprintf(debug_stream, " Cache Hit2 physical addr ");
							fprint_addr_coma(debug_stream, L0_cache->p_addr_latch.addr, param);
							fprintf(debug_stream, " logical addr ");
							fprint_addr_coma(debug_stream, L0_cache->l_addr_latch.addr, param);
							fprintf(debug_stream, " clock: 0x%04llx\n", clock);
							fprintf(debug_stream, "cL0(%d) set.way 0x%02x.%d 0x00 0x%016llx 0x08 0x%016llx 0x10 0x%016llx 0x18 0x%016llx clock: 0x%04llx\n",
								mhartid, set, way, physical_bus->data[0], physical_bus->data[1], physical_bus->data[2], physical_bus->data[3], clock);
							fprintf(debug_stream, "cL0(%d) set.way 0x%02x.%d 0x20 0x%016llx 0x28 0x%016llx 0x30 0x%016llx 0x38 0x%016llx clock: 0x%04llx\n",
								mhartid, set, way, physical_bus->data[0], physical_bus->data[1], physical_bus->data[2], physical_bus->data[3], clock);
							fprintf(debug_stream, "cL0(%d) set.way 0x%02x.%d 0x40 0x%016llx 0x48 0x%016llx 0x50 0x%016llx 0x58 0x%016llx clock: 0x%04llx\n",
								mhartid, set, way, physical_bus->data[0], physical_bus->data[1], physical_bus->data[2], physical_bus->data[3], clock);
							fprintf(debug_stream, "cL0(%d) set.way 0x%02x.%d 0x60 0x%016llx 0x68 0x%016llx 0x70 0x%016llx 0x78 0x%016llx clock: 0x%04llx\n",
								mhartid, set, way, physical_bus->data[0], physical_bus->data[1], physical_bus->data[2], physical_bus->data[3], clock);
						}
					}
				}
				if (physical_bus->snoop_response == snoop_miss) {
					copy_addr_bus_info(&bus2->addr.out, &L0_cache->l_addr_latch, clock);
					bus2->addr.out.cacheable = L0_cache->p_addr_latch.cacheable;

					if (debug_unit) {
						fprintf(debug_stream, "cL0(%d) xaction id %#06x, ", mhartid, bus2->addr.out.xaction_id);
						print_xaction2(bus2->addr.out.xaction, debug_stream);
						fprintf(debug_stream, " cache_non_swap physical addr ");
						fprint_addr_coma(debug_stream, bus2->addr.out.addr, param);
						fprintf(debug_stream, " logical addr ");
						fprint_addr_coma(debug_stream, L0_cache->l_addr_latch.addr, param);
						fprintf(debug_stream, " clock: 0x%04llx\n", clock);
					}
					L0_cache->list[L0_cache->list_stop_ptr].status = link_issued_forward;
					copy_addr_bus_info(&L0_cache->list[L0_cache->list_stop_ptr].latch, &L0_cache->l_addr_latch, clock);

					L0_cache->list_stop_ptr = ((L0_cache->list_stop_ptr + 1) & 0x1f);
				}
				L0_cache->l_addr_latch.strobe = 0;
			}
			else {
				copy_addr_bus_info(&bus2->addr.out, &L0_cache->l_addr_latch, clock);
				bus2->addr.out.cacheable = L0_cache->p_addr_latch.cacheable;

				if (debug_unit) {
					fprintf(debug_stream, "cL0(%d) xaction id %#06x, ", mhartid, bus2->addr.out.xaction_id);
					print_xaction2(bus2->addr.out.xaction, debug_stream);
					fprintf(debug_stream, " UC physical address: 0x%016I64x, logical address: 0x%016I64x,clock: 0x%04llx\n", bus2->addr.out.addr, L0_cache->l_addr_latch.addr, clock);
				}
				L0_cache->l_addr_latch.strobe = 0;
			}
			debug++;
			break;
		default:
			debug++;
			break;
		}
	}
	if (bus2->data_read.in.snoop_response == snoop_hit || bus2->data_read.in.snoop_response == snoop_dirty) {
		if (bus2->data_read.in.xaction_id == L0_cache->ctrl_addr.xaction_id && L0_cache->ctrl_addr.strobe) {
			L0_cache->reg = bus2->data_read.in.data[0];
			copy_data_bus_info(&bus2->data_write.out, &bus2->data_read.in);
			bus2->data_write.out.data[0] = 0;
			bus2->data_write.out.cacheable = page_IO_error_rsp;
			L0_cache->ctrl_addr.strobe = 0;
		}
		else if (bus2->data_read.in.cacheable != page_non_cache) {
			copy_data_bus_info(&L0_cache->bus2_data_read, &bus2->data_read.in);
			for (UINT i = 0; i < 0x20; i++) {
				if (L0_cache->list[i].latch.xaction_id == bus2->data_read.in.xaction_id && L0_cache->list[i].status != link_free) {
					L0_cache->list[i].status = link_free;
					UINT8 set = ((L0_cache->list[i].latch.addr >> 7) & 0x1f);
					UINT8 way = L0_cache->entry[set].way_ptr;
					L0_cache->entry[set].way[way].tag = (L0_cache->list[i].latch.addr >> 12);
					L0_cache->entry[set].way[way].state = (bus2->data_read.in.snoop_response == snoop_hit) ? shared_line : modified_line;
					for (UINT8 j = 0; j < 0x10; j++) 	 L0_cache->entry[set].way[way].line[j] = bus2->data_read.in.data[j];
					if (debug_unit) {
						fprintf(debug_stream, "cL0(%d) set.way 0x%02x.%d xaction id %#06x, addr: 0x%016I64x, cache fill clock: 0x%04llx\n",
							mhartid, set, way, bus2->data_read.in.xaction_id, L0_cache->list[i].latch.addr, clock);
						UINT64* data = bus2->data_read.in.data;
						fprintf(debug_stream, "cL0(%d) xaction id 0x%04x, ", mhartid, bus2->data_read.in.xaction_id);
						fprintf(debug_stream, "0x00 0x%08x %08x, 0x08 0x%08x %08x, 0x10 0x%08x %08x, 0x18 0x%08x %08x, clock: 0x%04llx\n",
							data[0]>>32, data[0], data[1] >> 32, data[1], data[2] >> 32, data[2], data[3] >> 32, data[3], clock);
						fprintf(debug_stream, "cL0(%d) xaction id 0x%04x, ", mhartid, bus2->data_read.in.xaction_id);
						fprintf(debug_stream, "0x20 0x%08x %08x, 0x28 0x%08x %08x, 0x30 0x%08x %08x, 0x38 0x%08x %08x, clock: 0x%04llx\n",
							data[4] >> 32, data[4], data[5] >> 32, data[5], data[6] >> 32, data[6], data[7] >> 32, data[7], clock);
						fprintf(debug_stream, "cL0(%d) xaction id 0x%04x, ", mhartid, bus2->data_read.in.xaction_id);
						fprintf(debug_stream, "0x40 0x%08x %08x, 0x48 0x%08x %08x, 0x50 0x%08x %08x, 0x58 0x%08x %08x, clock: 0x%04llx\n",
							data[8] >> 32, data[8], data[9] >> 32, data[9], data[10] >> 32, data[10], data[11] >> 32, data[11], clock);
						fprintf(debug_stream, "cL0(%d) xaction id 0x%04x, ", mhartid, bus2->data_read.in.xaction_id);
						fprintf(debug_stream, "0x60 0x%08x %08x, 0x68 0x%08x %08x, 0x70 0x%08x %08x, 0x78 0x%08x %08x, clock: 0x%04llx\n",
							data[12] >> 32, data[12], data[13] >> 32, data[13], data[14] >> 32, data[14], data[15] >> 32, data[15], clock);
					}
					L0_cache->entry[set].way_ptr = ((L0_cache->entry[set].way_ptr + 1) & 7);
				}
			}
		}
	}
	if (physical_bus->snoop_response == snoop_idle) {
		if (L0_cache->bus2_data_read.snoop_response == snoop_hit) {
			UINT hit = 0;
			if (L0_cache->bus2_data_read.cacheable != page_non_cache && (L0_cache->reg & 1 == 1)) {
				for (UINT current = L0_cache->list_start_ptr; current != L0_cache->list_stop_ptr && !hit; current = ((current + 1) & 0x1f)) {
					if (L0_cache->list[current].latch.xaction_id == L0_cache->bus2_data_read.xaction_id && L0_cache->list[current].status != link_free) {
						L0_cache->list[current].status = link_free;
						if (debug_unit) {
							fprintf(debug_stream, "cL0(%d) xaction id %#06x, entry dequeued clock: 0x%04llx\n", mhartid, L0_cache->bus2_data_read.xaction_id, clock);
						}
						hit = 1;
						if (L0_cache->list[current].latch.cacheable != page_non_cache) {
							if (mhartid == 1)
								debug++;
							UINT8 set = ((L0_cache->list[current].latch.addr >> 7) & 0x1f);
							hit = 0;
							for (UINT8 way = 0; way < 8 && !hit; way++) {
								if (L0_cache->entry[set].way[way].tag == L0_cache->list[current].latch.addr >> 12 && L0_cache->entry[set].way[way].state != invalid_line) {
									hit = 1;
									for (UINT8 i = 0; i < 0x10; i++) 	 L0_cache->entry[set].way[way].line[i] = L0_cache->bus2_data_read.data[i];
									if (debug_unit)
										fprintf(debug_stream, "cL0(%d) set.way 0x%02x.%x xaction id %#06x, L0C  hit; addr: 0x%016I64x, tag: 0x%016I64x, clock: 0x%04llx\n",
											mhartid, set, way, L0_cache->bus2_data_read.xaction_id, L0_cache->list[current].latch.addr, L0_cache->entry[set].way[way].tag, clock);
									L0_cache->entry[set].way_ptr = ((way + 1) & 7);
								}
							}
							if (!hit) {
								if (mhartid == 0)
									debug++;
								hit = 1;
								L0_cache->entry[set].way[L0_cache->entry[set].way_ptr].state = shared_line;
								L0_cache->entry[set].way[L0_cache->entry[set].way_ptr].tag = L0_cache->list[current].latch.addr >> 12;
								for (UINT8 i = 0; i < 0x10; i++) 	 L0_cache->entry[set].way[L0_cache->entry[set].way_ptr].line[i] = L0_cache->bus2_data_read.data[i];
								if (debug_unit)
									fprintf(debug_stream, "cL0(%d) set.way 0x%02x.%x xaction id 0x%04x, L0C fill; addr: 0x%016I64x, tag: 0x%016I64x, clock: 0x%04llx\n",
										mhartid, set, L0_cache->entry[set].way_ptr, L0_cache->bus2_data_read.xaction_id, L0_cache->list[current].latch.addr, L0_cache->entry[set].way[L0_cache->entry[set].way_ptr].tag, clock);
								L0_cache->entry[set].way_ptr = ((L0_cache->entry[set].way_ptr + 1) & 7);
							}
						}
					}
				}
			}
			else {
				copy_data_bus_info(physical_bus, &L0_cache->bus2_data_read);
			}
			L0_cache->bus2_data_read.snoop_response = snoop_idle;
		}
	}
}