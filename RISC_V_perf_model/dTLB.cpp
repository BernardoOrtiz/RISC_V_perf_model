// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "TLB.h"

void data_tlb_walk_unit(bus_w_snoop_signal1* bus2, UINT64 clock, TLB_type* TLB, UINT mhartid, param_type *param, FILE* debug_stream) {
	int debug = 0;
	UINT debug_unit = (param->PAGE_WALK || param->TLB) && clock >= param->start_time && (((param->core & 1) && mhartid == 0) || ((param->core & 2) && mhartid == 1) || ((param->core & 4) && mhartid == 2) || ((param->core & 8) && mhartid == 3) ||
		((param->core & 0x10) && mhartid == 4) || ((param->core & 0x20) && mhartid == 5) || ((param->core & 0x40) && mhartid == 6) || ((param->core & 0x80) && mhartid == 7));

	if (TLB->delay.snoop_response != snoop_idle) {
		copy_data_bus_info(&bus2->data_write.out, &TLB->delay);
		if (debug_unit) {
			fprintf(debug_stream, "dTLB unit(%lld): walk: delayed output return to sTLB; xaction id %#06x, data: 0x%08x, clock: 0x%04llx\n", mhartid, TLB->delay.xaction_id, TLB->delay.data[0], clock);
		}
		TLB->delay.snoop_response = snoop_idle;
	}
	if (mhartid == 0) {
		if (clock >= 0x2420)
			debug++;
	}
	if (bus2->snoop_addr.in.strobe && bus2->snoop_addr.in.addr != io_addr_L0D_ctrl && (bus2->snoop_addr.in.xaction_id & 0x0f) == mhartid) {
		copy_addr_bus_info(&TLB->walk.latch_addr, &bus2->snoop_addr.in, clock);
		switch (bus2->snoop_addr.in.addr) {
		case io_addr_TLB_data_ctrl:
			if ((bus2->snoop_addr.in.xaction & 0xfc) == bus_LR) {
				copy_addr_bus_info(&TLB->delay, &bus2->snoop_addr.in);
				for (UINT i = 0; i < 0x10; i++) TLB->delay.data[i] = 0;
				TLB->delay.data[0] = TLB->walk.reg;
				TLB->delay.snoop_response = snoop_dirty;
				if (debug_unit) {
					fprintf(debug_stream, "dTLB unit(%lld): walk: TLB ctrl reg read0; xaction id %#06x, data: 0x%08x, clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, TLB->walk.reg, clock);
				}
			}
			else if ((bus2->snoop_addr.in.xaction & 0xfc) == bus_SC) {
				if (debug_unit) {
					fprintf(debug_stream, "dTLB unit(%d): xaction id %#06x, walk: TLB ctrl update address received; clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
				}
			}
			else {
				fprintf(debug_stream, "dTLB unit(%lld): walk: ERROR, improper TLB ctrl reg access; xaction id %#06x, clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
			}
			break;
		case io_addr_TLB_2M_data:
			if ((bus2->snoop_addr.in.xaction & 0xfc) == bus_LR) {
				copy_addr_bus_info(&TLB->delay, &bus2->snoop_addr.in);
				for (UINT i = 0; i < 0x10; i++) TLB->delay.data[i] = 0;
				TLB->delay.data[0] = TLB->L2[TLB->L2_set].way[TLB->L2[TLB->L2_set].way_ptr].directory;
				TLB->delay.snoop_response = snoop_dirty;
				if (debug_unit) {
					fprintf(debug_stream, "dTLB unit(%lld): walk: TLB ctrl reg read1; xaction id %#06x, data: 0x%08x, clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, TLB->walk.reg, clock);
				}
			}
			else if ((bus2->snoop_addr.in.xaction & 0xfc) == bus_SC) {
				if (debug_unit) {
					fprintf(debug_stream, "dTLB unit(%d): xaction id %#06x, walk: TLB l2 update address received; clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
				}
			}
			else {
				fprintf(debug_stream, "dTLB unit(%lld): walk: ERROR, improper TLB ctrl reg access; xaction id %#06x, clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
			}
			break;
		case io_addr_TLB_data_vaddr:
			if ((bus2->snoop_addr.in.xaction & 0xfc) == bus_LR) {
				copy_addr_bus_info(&TLB->delay, &bus2->snoop_addr.in);
				for (UINT i = 0; i < 0x10; i++) TLB->delay.data[i] = 0;
				TLB->delay.snoop_response = snoop_dirty;
				if (debug_unit) {
					fprintf(debug_stream, "dTLB unit(%lld): walk: TLB ctrl reg read2; xaction id %#06x, data: 0x%08x, clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, TLB->walk.reg, clock);
				}
			}
			else if ((bus2->snoop_addr.in.xaction & 0xfc) == bus_SC) {
				if (debug_unit) {
					fprintf(debug_stream, "dTLB unit(%d): xaction id %#06x, walk: TLB L1 update address received; clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
				}
			}
			else {
				fprintf(debug_stream, "dTLB unit(%lld): walk: ERROR, improper TLB ctrl reg access; xaction id %#06x, clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
			}
			break;
		default: {
			if ((((bus2->snoop_addr.in.xaction & 0xfc) == bus_SC) || ((bus2->snoop_addr.in.xaction & 0xfc) == bus_LR)) && bus2->snoop_addr.in.xaction & 0x02) {// aquire lock
				if (debug_unit) {
					fprintf(debug_stream, "dTLB unit(%lld): walk: LOCK AQuire; xaction id %#06x, clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
				}
				TLB->lock = 1;
				if ((bus2->snoop_addr.in.xaction & 0xfc) == bus_LR) {
					copy_addr_bus_info(&TLB->delay, &bus2->snoop_addr.in);
					for (UINT i = 0; i < 0x10; i++) TLB->delay.data[i] = 0;
					TLB->delay.snoop_response = snoop_dirty;
				}
			}
			else if ((((bus2->snoop_addr.in.xaction & 0xfc) == bus_SC) || ((bus2->snoop_addr.in.xaction & 0xfc) == bus_LR)) && bus2->snoop_addr.in.xaction & 0x01) {// release lock
				if (debug_unit) {
					fprintf(debug_stream, "dTLB unit(%lld): walk: LOCK ReLease; xaction id %#06x, clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
				}
				TLB->lock = 0;
				if ((bus2->snoop_addr.in.xaction & 0xfc) == bus_LR) {
					copy_addr_bus_info(&TLB->delay, &bus2->snoop_addr.in);
					for (UINT i = 0; i < 0x10; i++) TLB->delay.data[i] = 0;
					TLB->delay.snoop_response = snoop_dirty;
				}
			}
		}
			   break;
		}
	}

	if (TLB->walk.latch_addr.strobe && (bus2->data_read.in.snoop_response == snoop_dirty)) {
		if (bus2->data_read.in.xaction_id == TLB->walk.latch_addr.xaction_id) {
			INT64 paddr, paddr_h;
			switch (TLB->walk.latch_addr.addr) {
			case io_addr_TLB_4K_data:
				copy_data_bus_info(&bus2->data_write.out, &bus2->data_read.in);
				bus2->data_write.out.data[0] = 0;// ERROR code: no error
				bus2->data_write.out.cacheable = page_IO_error_rsp;// ERROR code: no error
				if (debug_unit) {
					fprintf(debug_stream, "dTLB unit(%d): xaction id %#06x, walk: vaddr value (data) received; address: 0x%016I64x, data: 0x%016I64x, way: 0x%04llx, clock: 0x%04x\n",
						mhartid, TLB->walk.latch_addr.xaction_id, TLB->walk.latch_addr.addr, bus2->data_read.in.data[0], TLB->way_ptr1, clock);
				}
				TLB->L1[TLB->way_ptr1].l_addr = bus2->data_read.in.data[0];
				TLB->L1[TLB->way_ptr1].time = clock;

				break;
			case io_addr_TLB_data_vaddr: {
				copy_data_bus_info(&bus2->data_write.out, &bus2->data_read.in);
				bus2->data_write.out.data[0] = 0;// ERROR code: no error
				bus2->data_write.out.cacheable = page_IO_error_rsp;// ERROR code: no error
				if (mhartid == 0)
					debug++;
				TLB->L1[TLB->way_ptr1].l_addr = bus2->data_read.in.data[0];
				TLB->L1[TLB->way_ptr1].l_addr_h = bus2->data_read.in.data[1];
				TLB->L2[TLB->L2_set].way[TLB->L2[TLB->L2_set].way_ptr].l_addr = bus2->data_read.in.data[0] & (~0x01fffff);
				TLB->L2[TLB->L2_set].way[TLB->L2[TLB->L2_set].way_ptr].l_addr_h = bus2->data_read.in.data[1];
				if (debug_unit) {
					fprintf(debug_stream, "dTLB unit(%d): xaction id %#06x, walk: paddr value (data) received; address/data: 0x%016I64x/ 0x%016I64x, error response = %d, way: 0x%04x, clock: 0x%04llx\n",
						mhartid, TLB->walk.latch_addr.xaction_id, TLB->walk.latch_addr.addr, bus2->data_read.in.data[0], bus2->data_write.out.data[0], TLB->way_ptr1, clock);
				}
				TLB->way_ptr1++;
				if (TLB->way_ptr1 >= 0x40) {
					if (debug_unit) {
						fprintf(debug_stream, "dTLB unit(%d): xaction id %#06x, walk: dTLB overflow, need to issue a flush command; address/data: 0x%016I64x/ 0x%016I64x clock: 0x%04llx\n", mhartid, TLB->walk.latch_addr.xaction_id, TLB->walk.latch_addr.addr, bus2->data_read.in.data[0], clock);
					}
					bus2->data_write.out.data[0] = 1;// ERROR code: TLB full, need to flush
				}
			}
									   break;
			case io_addr_TLB_2M_data:
				if (param->mxl == 3) {// 128b addr
					paddr = (bus2->data_read.in.data[0] << 2) & 0xffffffffffe00000;
					paddr_h = (bus2->data_read.in.data[1] & 0x003fffffffffffff) << 2;
				}
				else {
					paddr = (bus2->data_read.in.data[0] << 2) & 0xffffffffffe00000;
					paddr_h = 0;
				}
				TLB->L1[TLB->way_ptr1].directory = bus2->data_read.in.data[0];
				TLB->L1[TLB->way_ptr1].directory_h = bus2->data_read.in.data[1];
				TLB->L1[TLB->way_ptr1].p_addr = paddr;
				TLB->L1[TLB->way_ptr1].p_addr_h = paddr_h;
				TLB->L1[TLB->way_ptr1].type = 1;// 2M page
				TLB->L1[TLB->way_ptr1].time = clock;
				{
					TLB->L2_set = (bus2->data_read.in.data[0] >> 10) & 0x007f;
					if (TLB->L2[TLB->L2_set].way[TLB->L2[TLB->L2_set].way_ptr].directory & 1)
						TLB->L2[TLB->L2_set].way_ptr = (TLB->L2[TLB->L2_set].way_ptr + 1) & 7;
					UINT8 way = TLB->L2[TLB->L2_set].way_ptr;

					TLB->L2[TLB->L2_set].way[way].directory = bus2->data_read.in.data[0];
					TLB->L2[TLB->L2_set].way[way].directory_h = bus2->data_read.in.data[1];
					TLB->L2[TLB->L2_set].way[way].p_addr = paddr;
					TLB->L2[TLB->L2_set].way[way].p_addr_h = paddr_h;
					TLB->L2[TLB->L2_set].way[way].type = 1;// 2M page
					TLB->L2[TLB->L2_set].way[way].time = clock;
				}

				copy_data_bus_info(&bus2->data_write.out, &bus2->data_read.in);
				bus2->data_write.out.data[0] = 0;// ERROR code: no error
				bus2->data_write.out.cacheable = page_IO_error_rsp;// ERROR code: no error
				if (debug_unit) {
					fprintf(debug_stream, "dTLB unit(%d): xaction id %#06x, walk: 2M page entry value (data) received (dTLB); address: 0x%016I64x, data: 0x%016I64x, clock: 0x%04llx\n", mhartid, TLB->walk.latch_addr.xaction_id, TLB->walk.latch_addr.addr, bus2->data_read.in.data[0], clock);
				}
				break;

			case io_addr_TLB_data_ctrl:
				copy_data_bus_info(&bus2->data_write.out, &bus2->data_read.in);
				bus2->data_write.out.data[0] = 0;// ERROR code: no error
				bus2->data_write.out.cacheable = page_IO_error_rsp;// ERROR code: no error
				if (bus2->data_read.in.data[0] == 1) {
					if (debug_unit) {
						fprintf(debug_stream, "dTLB unit(%d): xaction id %#06x, walk: FLUSH dTLB command received; address/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n", mhartid, TLB->walk.latch_addr.xaction_id, TLB->walk.latch_addr.addr, bus2->data_read.in.data[0], clock);
					}
					TLB->way_ptr1 = 0;
				}
				else {
					debug++;
				}
				break;
			default:
				break;
			}
			TLB->walk.latch_addr.strobe = 0;
		}
	}
}
// output: p_addr_bus, next_addr
// input: 
void data_tlb_L1_cache(addr_bus_type* p_addr_bus, data_bus_type* l_data_bus, addr_bus_type* l_addr_bus, UINT64 clock, TLB_type* TLB, UINT mhartid, param_type *param, FILE* debug_stream) {
	int debug = 0;
	UINT debug_unit = param->TLB && clock >= param->start_time && (((param->core & 1) && mhartid == 0) || ((param->core & 2) && mhartid == 1) || ((param->core & 4) && mhartid == 2) || ((param->core & 8) && mhartid == 3));
	UINT debug_fault = (param->TLB || param->PAGE_WALK || param->PAGE_FAULT) && clock >= param->start_time && (((param->core & 1) && mhartid == 0) || ((param->core & 2) && mhartid == 1) || ((param->core & 4) && mhartid == 2) || ((param->core & 8) && mhartid == 3));
	if (mhartid == 0) {
		if (clock >= 0x16b9)
			debug++;
	}
	for (UINT8 port_id = 0; port_id < 4; port_id++) {
		if (l_addr_bus[port_id].strobe) {
			copy_addr_bus_info(&p_addr_bus[port_id], &l_addr_bus[port_id], clock);
			copy_addr_bus_info(&l_data_bus[port_id], &l_addr_bus[port_id]);
			if (l_addr_bus[port_id].addr < 0x80000000) {
				l_data_bus[port_id].snoop_response = snoop_hit;
				if (l_addr_bus[port_id].addr > 0x10000000) {
					p_addr_bus[port_id].cacheable = page_IO;
					l_data_bus[port_id].cacheable = page_IO;
					if (debug_unit) {
						fprintf(debug_stream, "dTLB unit(%lld): IO space address: 0x%016I64x, xaction id %#06x, clock: 0x%04llx\n", mhartid, p_addr_bus[port_id].addr, p_addr_bus[port_id].xaction_id, clock);
					}
				}
				else {
					p_addr_bus[port_id].cacheable = page_non_cache;
					l_data_bus[port_id].cacheable = page_non_cache;
					if (debug_unit) {
						fprintf(debug_stream, "dTLB unit(%lld): uncacheable memory space address: 0x%016I64x, xaction id %#06x, clock: 0x%04llx\n", mhartid, p_addr_bus[port_id].addr, p_addr_bus[port_id].xaction_id, clock);
					}
				}
			}
			else if (l_addr_bus[port_id].addr < 0x86400000) {
				p_addr_bus[port_id].cacheable = page_cache_no_swap;
				l_data_bus[port_id].cacheable = page_cache_no_swap;
				l_data_bus[port_id].snoop_response = snoop_hit;
				if (debug_unit) {
					fprintf(debug_stream, "dTLB unit(%lld): direct map cached memory space address: 0x%016I64x, xaction id %#06x, clock: 0x%04llx\n", mhartid, p_addr_bus[port_id].addr, p_addr_bus[port_id].xaction_id, clock);
				}
			}
			else {
				if ((l_addr_bus[port_id].xaction & 0xfc) == bus_LR) {// always directly mapped, not in page tables, cacheable
					copy_addr_bus_info(&p_addr_bus[port_id], &l_addr_bus[port_id], clock);
					if (debug_unit) {
						fprintf(debug_stream, "dTLB unit(%lld): LOCK cycle physical address: 0x%016I64x, xaction id %#06x, clock: 0x%04llx\n", mhartid, p_addr_bus[port_id].addr, p_addr_bus[port_id].xaction_id, clock);
					}
					l_data_bus[port_id].snoop_response = snoop_hit; // hand shake, ready for next address
				}
				else {
					l_data_bus[port_id].snoop_response = snoop_stall; // frees up the bus for the next request
					for (UINT way = 0; (way < TLB->way_ptr1) && (l_data_bus[port_id].snoop_response == snoop_stall); way++) {
						if (TLB->L1[way].directory & 1) {// entry must be valid
							if (TLB->L1[way].type == 0) {
								if ((TLB->L1[way].l_addr >> 12) == (l_addr_bus[port_id].addr >> 12)) {
									copy_addr_bus_info(&p_addr_bus[port_id], &l_addr_bus[port_id], clock);
									p_addr_bus[port_id].addr = ((TLB->L1[way].p_addr >> 12) << 12) | (l_addr_bus[port_id].addr & 0x0fff);
									p_addr_bus[port_id].cacheable = (page_cache_type)(TLB->L1[way].directory & 0x0f);
									l_data_bus[port_id].snoop_response = snoop_hit;
									if (debug_unit) {
										fprintf(debug_stream, "dTLB unit(%lld): L1 Hit physical address: 0x%016I64x, logical address: 0x%016I64x, xaction id %#06x, clock: 0x%04llx\n", mhartid, p_addr_bus[port_id].addr, l_addr_bus[port_id].addr, p_addr_bus[port_id].xaction_id, clock);
									}
								}
							}
							else if (TLB->L1[way].type == 1) {
								if ((TLB->L1[way].l_addr >> 21) == (l_addr_bus[port_id].addr >> 21)) {
									copy_addr_bus_info(&p_addr_bus[port_id], &l_addr_bus[port_id], clock);
									p_addr_bus[port_id].addr = ((TLB->L1[way].p_addr >> 21) << 21) | (l_addr_bus[port_id].addr & 0x1fffff);
									p_addr_bus[port_id].cacheable = (page_cache_type)(TLB->L1[way].directory & 0x0f);
									l_data_bus[port_id].snoop_response = snoop_hit;
									if (debug_unit) {
										fprintf(debug_stream, "dTLB unit(%lld): L1 Hit physical address: 0x%016I64x, logical address: 0x%016I64x, xaction id %#06x, clock: 0x%04llx\n", mhartid, p_addr_bus[port_id].addr, l_addr_bus[port_id].addr, p_addr_bus[port_id].xaction_id, clock);
									}
								}
							}
							else {
								debug++;
							}
						}
					}
					if (l_data_bus[port_id].snoop_response == snoop_stall) {
						l_data_bus[port_id].snoop_response = snoop_miss;
						if (mhartid == 0)
							debug++;
						TLB->L2_set = (l_addr_bus[port_id].addr >> 21) & 0x007f;
						UINT hit = 0;
						TLB->walk.reg &= ~(1 << 4);
						for (UINT8 way = 0; (way < 0x10) && (~hit); way++) {
							if ((TLB->L2[TLB->L2_set].way[way].l_addr & ~0x0001ffff) == (l_addr_bus[port_id].addr & ~0x0001ffff)) {
								TLB->walk.reg = (1 << 4);
								hit = 1;
								TLB->L2[TLB->L2_set].way_ptr = way;
							}
						}
						l_data_bus[port_id].cacheable = page_invalid;
						if (debug_fault) {
							if (hit) {
								fprintf(debug_stream, "dTLB unit(%lld): xaction id %#06x, Page Fault, L2 hit; dTLB_2 set/way: 0x%02x, physical address: 0x%016I64x, clock: 0x%04llx\n", mhartid, l_addr_bus[port_id].xaction_id, TLB->L2_set, l_addr_bus[port_id].addr, clock);
							}
							else {
								fprintf(debug_stream, "dTLB unit(%lld): xaction id %#06x, Page Fault, L2 miss; logical address: ",
									mhartid, l_addr_bus[port_id].xaction_id);
								fprint_addr_coma(debug_stream, l_addr_bus[port_id].addr, param);
								fprintf(debug_stream, " clock: 0x%04llx\n", clock);
							}
						}
					}
					UINT8 set = (l_addr_bus[port_id].addr >> 21) & 0x7f;
					for (UINT8 way = 0; way < 0x10; way++) {
						if (TLB->L2[set].way[way].l_addr & (~0x0001fffff) == l_addr_bus[port_id].addr & (~0x0001fffff)) {
							debug++;
						}
					}
					p_addr_bus[port_id].cacheable = page_rwx;
					l_data_bus[port_id].cacheable = page_rwx;
				}
			}
		}
	}
}
void data_tlb_unit(addr_bus_type* p_addr_bus, data_bus_type* l_data_bus, addr_bus_type* l_addr_bus, bus_w_snoop_signal1* bus2, UINT64 clock, TLB_type* TLB, UINT mhartid, param_type *param, FILE* debug_stream) {
	data_tlb_walk_unit(bus2, clock, TLB, mhartid, param, debug_stream);// need to fix csr data bus to input and output
	data_tlb_L1_cache(p_addr_bus, l_data_bus, l_addr_bus, clock, TLB, mhartid, param, debug_stream);//l_add:P_add cache
}