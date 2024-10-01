// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "TLB.h"
#include "ROB.h"

// need to move the control registers to be written through locked data bus transaction
// mirror of tlb needs to be duplicated at L2 cache level 
//	* filter out L2 and poximal strutures using TLB table first
//	* filter uses reverse TLB table (phy address to logical; rather than logical to physical)
//	* data bus used to write to control registers (atomic access)
// cr_port_addr is a boot strapped io address, must be unique
// cr_port_data is cr_port_addr + 1 by convention

void code_tlb_walk_unit(bus_w_snoop_signal1* bus2, UINT64 clock, TLB_type* TLB, UINT mhartid, param_type *param, FILE* debug_stream) {
	int debug = 0;

	UINT debug_unit = (1 << mhartid & param->core) && (param->PAGE_WALK || param->TLB) && clock >= param->start_time;

	if (mhartid == 0)
		if (clock >= 0x1947)
			debug++;

	if (TLB->walk.latch_addr.strobe && (bus2->data_read.in.snoop_response == snoop_dirty)) {
		if (bus2->data_read.in.xaction_id == TLB->walk.latch_addr.xaction_id) {
			if (TLB->walk.latch_addr.addr == io_addr_TLB_4K_code || TLB->walk.latch_addr.addr == io_addr_TLB_code_vaddr ||
				TLB->walk.latch_addr.addr == io_addr_TLB_2M_code || TLB->walk.latch_addr.addr == io_addr_TLB_code_ctrl) {
				copy_data_bus_info(&bus2->data_write.out, &bus2->data_read.in);
				bus2->data_write.out.data[0] = 0;// error code: 0 = no errors
				bus2->data_write.out.cacheable = page_IO_error_rsp;
				INT64 paddr, paddr_h;
				switch (TLB->walk.latch_addr.addr) {
				case io_addr_TLB_code_ctrl:
					if (debug_unit) {
						fprintf(debug_stream, "cTLB (%d): xaction id %#06x, walk: code TLB control reg value updated (data) received; address/data: 0x%016I64x/ 0x%016I64x, clock: 0x%04llx\n", mhartid, TLB->walk.latch_addr.xaction_id, TLB->walk.latch_addr.addr, bus2->data_read.in.data[0], clock);
					}
					TLB->walk.reg = bus2->data_read.in.data[0];
					break;
				case io_addr_TLB_4K_code:
					if (debug_unit) {
						fprintf(debug_stream, "cTLB (%d): xaction id %#06x, walk: L1 vaddr value (data) received; address/data: 0x%016I64x/ 0x%016I64x, clock: 0x%04llx\n", mhartid, TLB->walk.latch_addr.xaction_id, TLB->walk.latch_addr.addr, bus2->data_read.in.data[0], clock);
					}
					TLB->L1[TLB->way_ptr1].l_addr = bus2->data_read.in.data[0];
					break;
				case io_addr_TLB_2M_code:
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
					if (debug_unit) {
						fprintf(debug_stream, "cTLB (%d): xaction id %#06x, walk: 2MB page entry value(data) received; address/data: 0x%016I64x/ 0x%016I64x error:  0x%016I64x, clock: 0x%04llx\n",
							mhartid, TLB->walk.latch_addr.xaction_id, TLB->walk.latch_addr.addr, bus2->data_read.in.data[0], bus2->data_write.out.data[0], clock);
					}
					break;
				case io_addr_TLB_code_vaddr:// logical address
					if (debug_unit) {
						fprintf(debug_stream, "cTLB(%d): xaction id %#06x, walk: vaddr value (data) received; address/data: 0x%016I64x/ 0x%016I64x, clock: 0x%04llx\n", mhartid, TLB->walk.latch_addr.xaction_id, TLB->walk.latch_addr.addr, bus2->data_read.in.data[0], clock);
					}
					if (param->mxl == 3) {// 128b addr
					}
					TLB->L1[TLB->way_ptr1].l_addr = bus2->data_read.in.data[0];
					TLB->L1[TLB->way_ptr1].l_addr_h = bus2->data_read.in.data[1];
					TLB->way_ptr1++;
					TLB->L2[TLB->L2_set].way[TLB->L2[TLB->L2_set].way_ptr].l_addr = bus2->data_read.in.data[0];
					TLB->L2[TLB->L2_set].way[TLB->L2[TLB->L2_set].way_ptr].l_addr_h = bus2->data_read.in.data[1];
					if (TLB->way_ptr1 == 0x40) {
						debug++; // TLB overflow
						fprintf(debug_stream, "cTLB(%d): xaction id %#06x, walk: ERROR: code TLB overflow, need to implement flush; address/data: 0x%016I64x/ 0x%016I64x clock: 0x%04llx\n", mhartid, TLB->walk.latch_addr.xaction_id, TLB->walk.latch_addr.addr, bus2->data_read.in.data[0], clock);
						exit(0);
					}
					break;
				default:
					break;
				}
			}
			else {
				if (debug_unit) {
					fprintf(debug_stream, "cTLB(%lld): xaction id %#06x, walk: LOCK ReLease complete; clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
				}
			}
			TLB->lock = 0;
			TLB->walk.latch_addr.strobe = 0;
		}
	}

	if (bus2->snoop_addr.in.strobe) {
		copy_addr_bus_info(&TLB->walk.latch_addr, &bus2->snoop_addr.in, clock);
		TLB->walk.latch_addr.cacheable = page_IO_error_rsp;
		switch (bus2->snoop_addr.in.xaction) {
		case bus_LR:
		case bus_LR_aq:
		case bus_LR_rl:
		case bus_LR_aq_rl:
		{
			copy_addr_bus_info(&bus2->data_write.out, &bus2->snoop_addr.in);
			for (UINT i = 0; i < 0x10; i++) bus2->data_write.out.data[i] = 0;
			if (bus2->data_write.in.snoop_response == snoop_stall)
				debug++;
			bus2->data_write.out.snoop_response = snoop_dirty;
			if (bus2->snoop_addr.in.addr == io_addr_TLB_code_ctrl) {
				bus2->data_write.out.data[0] = TLB->walk.reg;
				if (debug_unit) {
					fprintf(debug_stream, "cTLB(%lld): xaction id %#06x, walk: cTLB ctrl reg read; clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
				}
			}
			else {
				if (debug_unit) {
					fprintf(debug_stream, "cTLB(%lld): xaction id %#06x, walk: cTLB targeted read; clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
				}
			}
			if (bus2->snoop_addr.in.xaction & 0x02 == 0x02) {
				if (debug_unit) {
					fprintf(debug_stream, "cTLB(%lld): xaction id %#06x, walk: LOCK AQuire; clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
				}
				TLB->lock = 1;
			}
			if (bus2->snoop_addr.in.xaction & 0x01 == 0x01) {
				if (debug_unit) {
					fprintf(debug_stream, "cTLB(%lld): xaction id %#06x, walk: LOCK ReLease; clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
				}
				TLB->lock = 0;
			}
		}
		break;
		case bus_SC:
		case bus_SC_aq:
		case bus_SC_rl:
		case bus_SC_aq_rl: {
			if (TLB->walk.latch_addr.addr == io_addr_TLB_4K_code || TLB->walk.latch_addr.addr == io_addr_TLB_code_vaddr ||
				TLB->walk.latch_addr.addr == io_addr_TLB_2M_code || TLB->walk.latch_addr.addr == io_addr_TLB_code_vaddr ||
				TLB->walk.latch_addr.addr == io_addr_TLB_code_ctrl) {
				if (debug_unit) {
					fprintf(debug_stream, "cTLB(%d): xaction id %#06x, walk: cTLB update address received; address: 0x%016I64x, clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, bus2->snoop_addr.in.addr, clock);
				}
			}
			if (bus2->snoop_addr.in.xaction & 0x02 == 0x02) {
				if (debug_unit) {
					fprintf(debug_stream, "cTLB(%lld): xaction id %#06x, walk: LOCK AQuire; clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
				}
				TLB->lock = 1;
				if (mhartid == 0)
					debug++;
			}
			if (bus2->snoop_addr.in.xaction & 0x01 == 0x01) {
				if (debug_unit) {
					fprintf(debug_stream, "cTLB(%lld): xaction id %#06x, walk: write addr with lock ReLease received; clock: 0x%04llx\n", mhartid, bus2->snoop_addr.in.xaction_id, clock);
				}
			}
		}
						 break;
		default:
			break;
		}
	}
}
//
// output: physical address, hit/fault response to core, L2 request
// input: logical address, L2 response, clock (synchronous)
// variables: TLB
// debug: debug_stream
//
void code_tlb_L1_cache(data_bus_type* TLB_response, addr_bus_type* p_addr_bus, addr_bus_type* l_addr_bus, UINT64 clock, TLB_type* TLB, UINT mhartid, param_type *param, FILE* debug_stream) {
	int debug = 0;
	UINT debug_unit = (param->caches || param->TLB) && clock >= param->start_time && (((param->core & 1) && mhartid == 0) || ((param->core & 2) && mhartid == 1) || ((param->core & 4) && mhartid == 2) || ((param->core & 8) && mhartid == 3) ||
		((param->core & 0x10) && mhartid == 4) || ((param->core & 0x20) && mhartid == 5) || ((param->core & 0x40) && mhartid == 6) || ((param->core & 0x80) && mhartid == 7));
	UINT debug_fault = (param->caches || param->TLB || param->PAGE_FAULT) && clock >= param->start_time && (((param->core & 1) && mhartid == 0) || ((param->core & 2) && mhartid == 1) || ((param->core & 4) && mhartid == 2) || ((param->core & 8) && mhartid == 3) ||
		((param->core & 0x10) && mhartid == 4) || ((param->core & 0x20) && mhartid == 5) || ((param->core & 0x40) && mhartid == 6) || ((param->core & 0x80) && mhartid == 7));
	if (mhartid == 0)
		if (clock == 0x18cd)
			debug++;

	if (l_addr_bus->strobe) {
		copy_addr_bus_info(p_addr_bus, l_addr_bus, clock);
		if (l_addr_bus->addr < 0x80000000) {
			p_addr_bus->cacheable = page_non_cache;
			if (l_addr_bus->addr > 0x10000000) {
				p_addr_bus->cacheable = page_IO;
			}
		}
		else if (l_addr_bus->addr < 0x86400000) {
			p_addr_bus->cacheable = page_cache_no_swap;
		}
		else {
			if (TLB->lock) {
				TLB_response->snoop_response = snoop_stall;
			}
			else {
				TLB_response->snoop_response = snoop_stall; // frees up the bus for the next request
				copy_addr_bus_info(TLB_response, l_addr_bus);
				p_addr_bus->cacheable = page_invalid;
				for (UINT way = 0; way < TLB->way_ptr1 && (TLB_response->snoop_response == snoop_stall); way++) {
					if (TLB->L1[way].directory & 1) {// entry must be valid
						if (TLB->L1[way].type == 0) {
							if ((TLB->L1[way].l_addr >> 12) == (l_addr_bus->addr >> 12)) {
								copy_addr_bus_info(p_addr_bus, l_addr_bus, clock);
								p_addr_bus->cacheable = (page_cache_type)(TLB->L1[way].directory & 0x0f);
								p_addr_bus->addr = ((TLB->L1[way].p_addr >> 12) << 12) | (l_addr_bus->addr & 0x0fff);
								TLB_response->snoop_response = snoop_hit;
								if (debug_unit) {
									fprintf(debug_stream, "cTLB(%lld): xaction id %#06x, L1 snoop hit; physical address: 0x%016I64x, logical address: 0x%016I64x, clock: 0x%04llx\n", mhartid, l_addr_bus->xaction_id, l_addr_bus->addr, p_addr_bus->addr, clock);
								}
							}
						}
						else if (TLB->L1[way].type == 1) {
							if ((TLB->L1[way].l_addr >> 21) == (l_addr_bus->addr >> 21)) {
								copy_addr_bus_info(p_addr_bus, l_addr_bus, clock);
								p_addr_bus->cacheable = (page_cache_type)(TLB->L1[way].directory & 0x0f);
								p_addr_bus->addr = ((TLB->L1[way].p_addr >> 21) << 21) | (l_addr_bus->addr & 0x01fffff);
								TLB_response->snoop_response = snoop_hit;
								if (debug_unit) {
									fprintf(debug_stream, "cTLB(%lld): xaction id %#06x, L1 snoop hit; physical address: 0x%016I64x, logical address: 0x%016I64x, clock: 0x%04llx\n", mhartid, l_addr_bus->xaction_id, l_addr_bus->addr, p_addr_bus->addr, clock);
								}
							}
						}
						else {
							debug++;
						}
					}
				}
				// need to add L2 check

				if (TLB_response->snoop_response == snoop_stall) {
					TLB_response->snoop_response = snoop_miss;
					if (debug_unit || debug_fault) {
						fprintf(debug_stream, "cTLB(%lld): xaction id %#06x, L1 snoop miss (Page Fault); physical address: 0x%016I64x, clock: 0x%04llx\n", mhartid, l_addr_bus->xaction_id, l_addr_bus->addr, clock);
					}
				}
			}
		}

	}
}

//
// output : physical address, TLB hit/fault to core
// input : logical address, clock (synchronous)
// saved unit variables: TLB, csr (control status registers)
// debug: debug_stream for file output
//
void code_tlb_unit(data_bus_type* TLB_response, addr_bus_type* p_addr_bus, addr_bus_type* l_addr_bus, bus_w_snoop_signal1* bus2, UINT64 clock, TLB_type* TLB, UINT mhartid, param_type *param, FILE* debug_stream) {
	code_tlb_walk_unit(bus2, clock, TLB, mhartid, param, debug_stream);// need to fix csr data bus to input and output
	code_tlb_L1_cache(TLB_response, p_addr_bus, l_addr_bus, clock, TLB, mhartid, param, debug_stream);
}