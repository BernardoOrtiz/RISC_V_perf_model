// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "TLB.h"
// set all TLB's to return logical address on external snoops
void reg_update_addr_sm(addr_bus_type* addr_out0, addr_bus_type* addr_out1, addr_bus_type* addr_in, sTLB_type* sTLB, UINT mhartid, UINT64 clock, UINT debug_unit, FILE* debug_stream) {
	UINT8 debug = 0;
	switch (addr_in->addr) {
	case io_addr_TLB_2M_code:
	case io_addr_TLB_4K_code: {
		copy_addr_bus_info(&sTLB->walk.latch_addr, addr_in, clock);
		copy_addr_bus_info(addr_out0, addr_in, clock);
		sTLB->walk.reg_access_out = 1;
		sTLB->walk.latch_data.valid = 0;
		if (debug_unit) {
			fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: cTLB update v_addr received; address: 0x%016I64x, clock: 0x%04llx\n", mhartid, addr_in->xaction_id, addr_in->addr, clock);
		}
	}
							break;
							//	case io_addr_TLB_code_vaddr:
	case io_addr_TLB_code_vaddr: {
		copy_addr_bus_info(&sTLB->walk.latch_addr, addr_in, clock);
		copy_addr_bus_info(addr_out0, addr_in, clock);
		sTLB->walk.reg_access_out = 1;
		sTLB->walk.latch_data.valid = 0;
		if (debug_unit) {
			fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: cTLB update p_addr received; address: 0x%016I64x, clock: 0x%04llx\n", mhartid, addr_in->xaction_id, addr_in->addr, clock);
		}
	}
							   break;
	case io_addr_TLB_code_ctrl: {
		copy_addr_bus_info(&sTLB->walk.latch_addr, addr_in, clock);
		copy_addr_bus_info(addr_out0, addr_in, clock);
		sTLB->walk.reg_access_out = 1;
		sTLB->walk.latch_data.valid = 0;
		if (debug_unit) {
			fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: cTLB update address received; address: 0x%016I64x, clock: 0x%04llx\n", mhartid, addr_in->xaction_id, addr_in->addr, clock);
		}
	}
							  break;
	case io_addr_TLB_2M_data:
	case io_addr_TLB_4K_data: {
		// ERROR: 
		//		* need to delay if external snoop is ongoing
		//		* need to snoop stall L3 bus, not allow external snoops while registers are updating
		//			-> need to seperate virtual and physical addresses, external snoops are blocked after virtual address written, waiting for physical address
		//		? consider different mechanism - single IO location, 1st access is v_addr (lock set), second access is p_addr(lock release) - lock release resets state machine
		//
		copy_addr_bus_info(&sTLB->walk.latch_addr, addr_in, clock);
		copy_addr_bus_info(addr_out1, addr_in, clock);
		sTLB->walk.reg_access_out = 1;
		sTLB->walk.latch_data.valid = 0;
		if (debug_unit) {
			fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: dTLB update v_addr received; address: 0x%016I64x, clock: 0x%04llx\n", mhartid, addr_in->xaction_id, addr_in->addr, clock);
		}
	}
							break;
							//	case io_addr_TLB_data_vaddr:
	case io_addr_TLB_data_vaddr: {
		// ERROR: 
		//		* need to delay if external snoop is ongoing
		//		* need to snoop stall L3 bus, not allow external snoops while registers are updating
		//			-> need to seperate virtual and physical addresses, external snoops are blocked after virtual address written, waiting for physical address
		//		? consider different mechanism - single IO location, 1st access is v_addr (lock set), second access is p_addr(lock release) - lock release resets state machine
		//
		copy_addr_bus_info(&sTLB->walk.latch_addr, addr_in, clock);
		copy_addr_bus_info(addr_out1, addr_in, clock);
		sTLB->walk.reg_access_out = 1;
		sTLB->walk.latch_data.valid = 0;
		if (debug_unit) {
			fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: dTLB update p_addr received; address: 0x%016I64x, clock: 0x%04llx\n", mhartid, addr_in->xaction_id, addr_in->addr, clock);
		}
	}
							   break;
	case io_addr_TLB_data_ctrl: {
		// ERROR: 
		//		* need to delay if external snoop is ongoing
		//		* need to snoop stall L3 bus, not allow external snoops while registers are updating
		//			-> need to seperate virtual and physical addresses, external snoops are blocked after virtual address written, waiting for physical address
		//		? consider different mechanism - single IO location, 1st access is v_addr (lock set), second access is p_addr(lock release) - lock release resets state machine
		//
		copy_addr_bus_info(&sTLB->walk.latch_addr, addr_in, clock);
		copy_addr_bus_info(addr_out1, addr_in, clock);
		sTLB->walk.reg_access_out = 1;
		sTLB->walk.latch_data.valid = 0;
		if (debug_unit) {
			fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: dTLB update ctrl received; address: 0x%016I64x, clock: 0x%04llx\n", mhartid, addr_in->xaction_id, addr_in->addr, clock);
		}
	}
							  break;
	case io_addr_L0C_ctrl: {
		copy_addr_bus_info(&sTLB->walk.latch_addr, addr_in, clock);
		copy_addr_bus_info(addr_out0, addr_in, clock);
		sTLB->walk.reg_access_out = 1;
		sTLB->walk.latch_data.valid = 0;
		if (debug_unit) {
			fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: L0C ctrl update address received; address: 0x%016I64x, clock: 0x%04llx\n", mhartid, addr_in->xaction_id, addr_in->addr, clock);
		}
	}
						 break;
	case io_addr_L0D_ctrl: {
		copy_addr_bus_info(&sTLB->walk.latch_addr, addr_in, clock);
		copy_addr_bus_info(addr_out1, addr_in, clock);
		sTLB->walk.reg_access_out = 1;
		sTLB->walk.latch_data.valid = 0;
		if (debug_unit) {
			fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: L0D ctrl update ctrl received; address: 0x%016I64x, clock: 0x%04llx\n", mhartid, addr_in->xaction_id, addr_in->addr, clock);
		}
	}
						 break;
	case io_addr_L2_ctrl: {
		copy_addr_bus_info(&sTLB->walk.latch_addr, addr_in, clock);
		//		copy_addr_bus_info(addr_out1, addr_in, clock);
		sTLB->walk.reg_access_out = 1;
		sTLB->walk.latch_data.valid = 0;
		if (debug_unit) {
			fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: L2 ctrl update lock_reg_access received; address: 0x%016I64x, clock: 0x%04llx\n", mhartid, addr_in->xaction_id, addr_in->addr, clock);
		}
	}
						break;
	default:
		debug++;
		break;
	}

}
void lock_reg_access(addr_bus_type* snoop_addr_out0, addr_bus_type* snoop_addr_out1, snoop_response_type* snoop_response0, snoop_response_type* snoop_response1, addr_bus_type* addr_in, UINT64 clock, sTLB_type* sTLB, UINT mhartid, UINT debug_unit, FILE* debug_stream) {
	int debug = 0;
	if ((addr_in->xaction & 0xfc) == bus_SC) {
		reg_update_addr_sm(snoop_addr_out0, snoop_addr_out1, addr_in, sTLB, mhartid, clock, debug_unit, debug_stream);
	}
	else if ((addr_in->xaction & 0xfc) == bus_LR) {
		// error, needs signal from L2 to validate that fencing is complete before proceeding, 
		copy_addr_bus_info(&sTLB->walk.latch_addr, addr_in, clock);
		sTLB->walk.latch_data.snoop_response = snoop_idle;
		switch (addr_in->addr) {
		case io_addr_TLB_code_ctrl: {
			sTLB->walk.reg_access_out = 1;
			if (debug_unit) {
				fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: cTLB CTRL read; clock: 0x%04llx\n", mhartid, addr_in->xaction_id, clock);
			}
			copy_addr_bus_info(snoop_addr_out0, addr_in, clock);
		}
								  break;
		case io_addr_TLB_data_ctrl: {
			sTLB->walk.reg_access_out = 1;
			if (debug_unit) {
				fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: dTLB CTRL read; clock: 0x%04llx\n", mhartid, addr_in->xaction_id, clock);
			}
			copy_addr_bus_info(snoop_addr_out1, addr_in, clock);
		}
								  break;
		case io_addr_TLB_4K_code: {
			sTLB->walk.reg_access_out = 1;
			if (debug_unit) {
				fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: satp verification address forwarded; clock: 0x%04llx\n", mhartid, addr_in->xaction_id, clock);
			}
			copy_addr_bus_info(snoop_addr_out0, addr_in, clock);
		}
								break;
		case io_addr_TLB_code_vaddr: {
			sTLB->walk.reg_access_out = 1;
			if (debug_unit) {
				fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: satp verification address forwarded; clock: 0x%04llx\n", mhartid, addr_in->xaction_id, clock);
			}
			copy_addr_bus_info(snoop_addr_out0, addr_in, clock);
		}
								   break;
		case io_addr_TLB_4K_data: {
			sTLB->walk.reg_access_out = 1;
			if (debug_unit) {
				fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: satp verification address forwarded; clock: 0x%04llx\n", mhartid, addr_in->xaction_id, clock);
			}
			copy_addr_bus_info(snoop_addr_out1, addr_in, clock);
		}
								break;
		case io_addr_TLB_data_vaddr: {
			sTLB->walk.reg_access_out = 1;
			if (debug_unit) {
				fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: satp verification address forwarded; clock: 0x%04llx\n", mhartid, addr_in->xaction_id, clock);
			}
			copy_addr_bus_info(snoop_addr_out1, addr_in, clock);
		}
								   break;
		case io_addr_TLB_2M_code: {
			sTLB->walk.reg_access_out = 1;
			if (debug_unit) {
				fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: cTLB L2 reg access forwarded; clock: 0x%04llx\n", mhartid, addr_in->xaction_id, clock);
			}
			copy_addr_bus_info(snoop_addr_out0, addr_in, clock);
		}
								break;
								/*
   case io_addr_TLB_code_vaddr: {
	   sTLB->walk.reg_access_out = 1;
	   if (debug_unit) {
		   fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: cTLB L2 reg access forwarded; clock: 0x%04llx\n", mhartid, addr_in->xaction_id, clock);
	   }
	   copy_addr_bus_info(snoop_addr_out0, addr_in, clock);
   }
								break;/**/
		case io_addr_TLB_2M_data: {
			sTLB->walk.reg_access_out = 1;
			if (debug_unit) {
				fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: dTLB L2 reg access forwarded; clock: 0x%04llx\n", mhartid, addr_in->xaction_id, clock);
			}
			copy_addr_bus_info(snoop_addr_out1, addr_in, clock);
		}
								break;
								/*
   case io_addr_TLB_data_vaddr: {
	   sTLB->walk.reg_access_out = 1;
	   if (debug_unit) {
		   fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: dTLB L2 reg access forwarded; clock: 0x%04llx\n", mhartid, addr_in->xaction_id, clock);
	   }
	   copy_addr_bus_info(snoop_addr_out1, addr_in, clock);
   }
								break;
								/**/
		case io_addr_L0C_ctrl: {
			sTLB->walk.reg_access_out = 1;
			if (debug_unit) {
				fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: L0C CTRL read, forward to L0C; clock: 0x%04llx\n", mhartid, addr_in->xaction_id, clock);
			}
			copy_addr_bus_info(snoop_addr_out0, addr_in, clock);
		}
							 break;
		case io_addr_L0D_ctrl: {
			sTLB->walk.reg_access_out = 1;
			if (debug_unit) {
				fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: L0D CTRL read, forward to L0D; clock: 0x%04llx\n", mhartid, addr_in->xaction_id, clock);
			}
			copy_addr_bus_info(snoop_addr_out1, addr_in, clock);
		}
							 break;
		case io_addr_L2_ctrl: {
			sTLB->walk.reg_access_out = 1;
			debug++; // do nothing, handled by L2 directly
		}
							break;
		default: { // error, dont write to L0, just use snoop stalls until data return and unlock bit is set
			if (addr_in->xaction & 0x02) {// aquire lock
				sTLB->walk.lock = 1;
				snoop_response1[0] = snoop_stall;
				snoop_response0[0] = snoop_stall;
				if (debug_unit) {
					fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: aquire lock; clock: 0x%04llx\n", mhartid, addr_in->xaction_id, clock);// need to block prefetcher from issuing bus transactions
				}
			}
			else if (addr_in->xaction & 0x01) {// release lock
				// error, need to release after data cycle
				sTLB->walk.lock = 0;
				if (clock >= 0x045f)
					debug++;
				if (debug_unit) {
					fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: release lock address; clock: 0x%04llx\n", mhartid, addr_in->xaction_id, clock);
				}
			}
		}
			   break;
		}
	}

}
void shadow_tlb_walk_unit(bus_w_snoop_signal1* bus2, data_bus_type bus3_read_in, UINT64 clock, sTLB_type* sTLB, UINT mhartid, param_type *param, FILE* debug_stream) {
	int debug = 0;
	UINT debug_unit = (param->PAGE_WALK || param->TLB) && clock >= param->start_time && (((param->core & 1) && mhartid == 0) || ((param->core & 2) && mhartid == 1) || ((param->core & 4) && mhartid == 2) || ((param->core & 8) && mhartid == 3) ||
		((param->core & 0x10) && mhartid == 4) || ((param->core & 0x20) && mhartid == 5) || ((param->core & 0x40) && mhartid == 6) || ((param->core & 0x80) && mhartid == 7));
	if (mhartid == 0) {
		if (clock >= 0x272f)// 0x3281
			debug++;
	}
	if (sTLB->walk.est_lockout == 1 && sTLB->walk.lockout == 0)
		bus2[1].data_read.out.snoop_response = snoop_stall;
	if (bus2[1].addr.in.strobe && (((bus2[1].addr.in.xaction & 0xfc) == bus_LR) || ((bus2[1].addr.in.xaction & 0xfc) == bus_SC))) {
		// IO space, set IO flag 
		//		* look for errror response syscle with IOs
		if (bus2[1].addr.in.addr >= 0x10000000 && bus2[1].addr.in.addr < 0x20000000) {
			sTLB->walk.io_cycle = 1;
			switch (bus2[1].addr.in.addr) {
			case io_addr_TLB_code_ctrl:
			case io_addr_TLB_data_ctrl:
			case io_addr_TLB_4K_code:
			case io_addr_TLB_4K_data:
			case io_addr_TLB_2M_code:
			case io_addr_TLB_2M_data:
			case io_addr_TLB_code_vaddr:
			case io_addr_TLB_data_vaddr:
			case io_addr_L0C_ctrl:
			case io_addr_L0D_ctrl:
			case io_addr_L2_ctrl:
				bus2[0].data_read.out.snoop_response = snoop_stall;
				copy_addr_bus_info(&sTLB->walk.latch_addr, &bus2[1].addr.in, clock);
				sTLB->walk.est_lockout = 1;
				sTLB->walk.addr_sent = 0;
				sTLB->walk.data_sent = 0;
				sTLB->walk.latch_data.snoop_response = snoop_idle;
				if (debug_unit) {
					fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: ctrl reg access latched, aquiring lock; clock: 0x%04llx\n", mhartid, bus2[1].addr.in.xaction_id, clock);// need to block prefetcher from issuing bus transactions
				}
				break;
			default: { // error, dont write to L0, just use snoop stalls until data return and unlock bit is set
				if (bus2[1].addr.in.xaction & 0x02) {// aquire lock
					sTLB->walk.lock = 1;
					copy_addr_bus_info(&sTLB->walk.latch_addr, &bus2[1].addr.in, clock);
					sTLB->walk.latch_data.snoop_response = snoop_idle;
					if (debug_unit) {
						fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: aquire lock; clock: 0x%04llx\n", mhartid, bus2[1].addr.in.xaction_id, clock);// need to block prefetcher from issuing bus transactions
					}
				}
				else if (bus2[1].addr.in.xaction & 0x01) {// release lock
					// error, need to release after data cycle
					sTLB->walk.lock = 0;
					copy_addr_bus_info(&sTLB->walk.latch_addr, &bus2[1].addr.in, clock);
					sTLB->walk.latch_data.snoop_response = snoop_idle;
					if (clock >= 0x045f)
						debug++;
					if (debug_unit) {
						fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: release lock address; clock: 0x%04llx\n", mhartid, bus2[1].addr.in.xaction_id, clock);
					}
				}
			}
				   break;
			}

		}
		else {
			sTLB->walk.io_cycle = 0;
			if (bus2[1].addr.in.xaction & 0x02) {// aquire lock
				sTLB->walk.lock = 1;
				copy_addr_bus_info(&sTLB->walk.latch_addr, &bus2[1].addr.in, clock);
				sTLB->walk.latch_data.snoop_response = snoop_idle;
				if (debug_unit) {
					fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: aquire lock; clock: 0x%04llx\n", mhartid, bus2[1].addr.in.xaction_id, clock);// need to block prefetcher from issuing bus transactions
				}
			}
			else if (bus2[1].addr.in.xaction & 0x01) {// release lock
				// error, need to release after data cycle
				sTLB->walk.lock = 0;
				copy_addr_bus_info(&sTLB->walk.latch_addr, &bus2[1].addr.in, clock);
				sTLB->walk.latch_data.snoop_response = snoop_idle;
				if (clock >= 0x045f)
					debug++;
				if (debug_unit) {
					fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: release lock address; clock: 0x%04llx\n", mhartid, bus2[1].addr.in.xaction_id, clock);
				}
			}
		}
	}
	if (sTLB->walk.lockout) {
		if (sTLB->walk.latch_data.snoop_response != snoop_idle && (sTLB->walk.addr_sent == 1) && (sTLB->walk.data_sent == 0)) {
			switch (sTLB->walk.latch_addr.addr) {
			case io_addr_L0C_ctrl:
			case io_addr_TLB_code_ctrl:
			case io_addr_TLB_4K_code:
			case io_addr_TLB_code_vaddr:
			case io_addr_TLB_2M_code:
				//			case io_addr_TLB_code_vaddr:
				copy_data_bus_info(&bus2[0].data_read.out, &sTLB->walk.latch_data);
				sTLB->walk.data_sent = 1;
				if (debug_unit) {
					fprintf(debug_stream, "sTLB(%d) xaction id 0x%04x, walk: data to cTLB addr,data 0x%016I64x,0x%016I64x clock: 0x%04llx\n",
						mhartid, sTLB->walk.latch_data.xaction_id, sTLB->walk.latch_addr.addr, sTLB->walk.latch_data.data[0], clock);
				}
				break;
			case io_addr_L0D_ctrl:
			case io_addr_TLB_data_ctrl:
			case io_addr_TLB_4K_data:
			case io_addr_TLB_data_vaddr:
			case io_addr_TLB_2M_data:
				//			case io_addr_TLB_data_vaddr:
				copy_data_bus_info(&bus2[1].data_read.out, &sTLB->walk.latch_data);
				sTLB->walk.data_sent = 1;
				if (debug_unit) {
					fprintf(debug_stream, "sTLB(%d) xaction id 0x%04x, walk: data to dTLB addr,data 0x%016I64x,0x%016I64x clock: 0x%04llx\n",
						mhartid, sTLB->walk.latch_data.xaction_id, sTLB->walk.latch_addr.addr, sTLB->walk.latch_data.data[0], clock);
				}
				break;
			case io_addr_L2_ctrl:
				debug++; // direct write from core???
				break;
			default:
				debug++;
				break;
			}
		}
		if (sTLB->walk.latch_addr.strobe && (sTLB->walk.addr_sent == 0)) {
			lock_reg_access(&bus2[0].snoop_addr.out, &bus2[1].snoop_addr.out, &bus2[0].data_read.out.snoop_response, &bus2[1].data_read.out.snoop_response,
				&sTLB->walk.latch_addr, clock, sTLB, mhartid, debug_unit, debug_stream);
			sTLB->walk.addr_sent = 1;
		}
	}
	if (sTLB->walk.lock == 1) {
		bus2[0].data_read.out.xaction_id = sTLB->walk.latch_addr.xaction_id;
	}
	if (sTLB->walk.reg_access_out != 0 && bus2[0].data_write.in.snoop_response == snoop_dirty && bus2[0].data_write.in.xaction_id == sTLB->walk.latch_addr.xaction_id) {
		sTLB->walk.lock = 0;
		sTLB->walk.reg_access_out = 0;
		sTLB->walk.est_lockout = 0;
		sTLB->walk.lockout = 0;
		if (debug_unit) {
			fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: read response from L0C ctrl reg (1); clock: 0x%04llx\n", mhartid, bus2[0].data_write.in.xaction_id, clock);
		}
		copy_data_bus_info(&bus2[1].data_read.out, &bus2[0].data_write.in);
	}
	if (sTLB->walk.latch_addr.strobe) {
		if (bus3_read_in.snoop_response == snoop_hit || bus3_read_in.snoop_response == snoop_dirty) {
			if (bus3_read_in.xaction_id == sTLB->walk.latch_addr.xaction_id) {
				if ((sTLB->walk.latch_addr.xaction & 0xfc) == bus_LR && (bus2[1].addr.in.xaction & 0x01)) {
					sTLB->walk.lock = 0;
					if (clock >= 0x045f)
						debug++;
					if (debug_unit) {
						fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: release lock address; clock: 0x%04llx\n", mhartid, bus2[1].addr.in.xaction_id, clock);
					}
				}
			}
		}
		if ((sTLB->walk.latch_addr.xaction & 0xfc) == bus_SC) {
			if ((bus2[1].data_write.in.snoop_response == snoop_dirty || bus2[1].data_write.in.snoop_response == snoop_hit)) {
				if (bus2[1].data_write.in.cacheable == page_IO_error_rsp) {
					if (bus2[1].data_write.in.xaction_id == sTLB->walk.latch_addr.xaction_id) {
						copy_data_bus_info(&bus2[1].data_read.out, &bus2[1].data_write.in);
						sTLB->walk.latch_addr.strobe = 0;
						sTLB->walk.reg_access_out = 0;
						sTLB->walk.est_lockout = 0;
						sTLB->walk.lockout = 0;
						if (debug_unit) {
							if (bus2[1].data_read.in.xaction_id == sTLB->walk.latch_addr.xaction_id && sTLB->walk.latch_addr.addr == io_addr_L2_ctrl) {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: error response from uL2 to core; 0x%016x, clock: 0x%04llx\n",
									mhartid, sTLB->walk.latch_addr.xaction_id, bus2[1].data_read.in.data[0], clock);
							}
							else {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: error response from dTLB to core; 0x%016x, clock: 0x%04llx\n",
									mhartid, sTLB->walk.latch_addr.xaction_id, bus2[1].data_write.in.data[0], clock);
							}
							sTLB->walk.latch_data.data[1] = 0;
						}
					}
				}
				else {
					if (bus2[1].data_write.in.xaction_id == sTLB->walk.latch_addr.xaction_id) {
						switch (sTLB->walk.latch_addr.addr) {
						case io_addr_TLB_4K_code: {
							if (debug_unit) {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: vaddr value (data) received (sTLB); address/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n",
									mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], clock);
							}
							sTLB->cL1.way[sTLB->cL1.way_ptr].l_addr = bus2[1].data_write.in.data[0];
							// initiate writes through snoop port to sTLB's
							copy_data_bus_info(&sTLB->walk.latch_data, &bus2[1].data_write.in);
							copy_data_bus_info(&bus2[0].data_read.out, &bus2[1].data_write.in);
							break;
						}
						case io_addr_TLB_code_vaddr: {
							if (debug_unit) {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: paddr value (data) received (sTLB); address/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], clock);
							}
							sTLB->cL1.way[sTLB->cL1.way_ptr].directory = bus2->data_read.in.data[0];
							sTLB->cL1.way[sTLB->cL1.way_ptr].p_addr = ((bus2->data_read.in.data[0] << 2) & (~0x0fff));
							sTLB->cL1.way_ptr++;
							if (sTLB->cL1.way_ptr == sTLB->cL1.way_count)
								debug++; // sTLB overflow
							copy_data_bus_info(&sTLB->walk.latch_data, &bus2[1].data_write.in);
							copy_data_bus_info(&bus2[0].data_read.out, &bus2[1].data_write.in);
						}
												   break;
						case io_addr_TLB_2M_code: {
							copy_data_bus_info(&sTLB->walk.latch_data, &bus2[1].data_write.in);
							copy_data_bus_info(&bus2[0].data_read.out, &bus2[1].data_write.in);
							if (debug_unit) {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: vaddr value (data) received 0; addr/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n",
									mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], clock);
							}
						}
												break;
												/*
				  case io_addr_TLB_code_vaddr: {
					  if (debug_unit) {
						  fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: paddr value (data) received; address/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], clock);
					  }
					  // initiate writes through snoop port to sTLB's
					  copy_data_bus_info(&sTLB->walk.latch_data, &bus2[1].data_write.in);
					  copy_data_bus_info(&bus2[0].data_read.out, &bus2[1].data_write.in);
					  sTLB->walk.reg_access_out = 0;
				  }
												break;
												/**/
						case io_addr_TLB_4K_data: {
							if (debug_unit) {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: vaddr value (data) received 1; addr/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n",
									mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], clock);
							}
							sTLB->dL1.way[sTLB->dL1.way_ptr].l_addr = bus2[1].data_write.in.data[0];
							// initiate writes through snoop port to sTLB's
							copy_data_bus_info(&bus2[1].data_read.out, &bus2[1].data_write.in);
							copy_data_bus_info(&sTLB->walk.latch_data, &bus2[1].data_write.in);
							sTLB->walk.latch_data.data[1] = 0;
						}
												break;
						case io_addr_TLB_data_vaddr: {
							if (debug_unit) {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: paddr value (data) received; address: 0x%016I64x, data: 0x%016I64x, entry count: %d, clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], sTLB->dL1.way_ptr, clock);
							}
							sTLB->dL1.way[sTLB->dL1.way_ptr].directory = bus2->data_read.in.data[0];
							sTLB->dL1.way[sTLB->dL1.way_ptr].p_addr = ((bus2->data_read.in.data[0] << 2) & (~0x0fff));
							sTLB->dL1.way_ptr++;
							if (sTLB->dL1.way_ptr == sTLB->dL1.way_count) {
								debug++; // sTLB overflow
								if (debug_unit) {
									fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: sTLB L1 full, need to initiate cache flush; address: 0x%016I64x, data: 0x%016I64x, entry count: %d, clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], sTLB->dL1.way_ptr, clock);
								}
							}
							// initiate writes through snoop port to sTLB's
							copy_data_bus_info(&bus2[1].data_read.out, &bus2[1].data_write.in);
							copy_data_bus_info(&sTLB->walk.latch_data, &bus2[1].data_write.in);
							sTLB->walk.latch_data.data[1] = 0;
						}
												   break;
						case io_addr_TLB_2M_data: {
							if (debug_unit) {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: vaddr value (data) received 3; addr/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n",
									mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], clock);
							}
							// initiate writes through snoop port to sTLB's
							copy_data_bus_info(&bus2[1].data_read.out, &bus2[1].data_write.in);
							copy_data_bus_info(&sTLB->walk.latch_data, &bus2[1].data_write.in);
							sTLB->walk.latch_data.data[1] = 0;
						}
												break;
						case (io_addr_TLB_2M_code | 8):
						case (io_addr_TLB_2M_data | 8):
						case (io_addr_TLB_4K_code | 8):
						case (io_addr_TLB_4K_data | 8):
						case (io_addr_TLB_code_vaddr | 8):
						case (io_addr_TLB_data_vaddr | 8):
						{
							if (debug_unit) {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: vaddr verification (read data) to core; clock: 0x%04llx\n", mhartid, bus2[1].data_read.out.xaction_id, clock);
							}
							copy_data_bus_info(&bus2[1].data_read.out, &bus2[1].data_write.in);
							sTLB->walk.latch_addr.strobe = 0;
						}
						break;
						case io_addr_TLB_data_ctrl: {
							sTLB->walk.reg = (0xff00 & (bus2[1].data_write.in.data[0] << 8)) | (0x00ff & sTLB->walk.reg);
							if (debug_unit) {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: FLUSH TLB data command received; address/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], clock);
							}
							sTLB->dL1.way_ptr = 0;
							if (sTLB->walk.addr_sent) {
								copy_data_bus_info(&bus2[1].data_read.out, &bus2[1].data_write.in); // forward message to dTLB
								sTLB->walk.data_sent = 1;
							}
							else
								sTLB->walk.data_sent = 0;
							copy_data_bus_info(&sTLB->walk.latch_data, &bus2[1].data_write.in);
							sTLB->walk.latch_data.data[1] = 0;
						}
												  break;
						case io_addr_TLB_code_ctrl: {
							sTLB->walk.reg = (0xff00 & sTLB->walk.reg) | (0x00ff & bus2[1].data_write.in.data[0]);
							if (debug_unit) {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: FLUSH TLB code command received; address/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], clock);
							}
							sTLB->cL1.way_ptr = 0;
							if (sTLB->walk.addr_sent) {
								copy_data_bus_info(&bus2[0].data_read.out, &bus2[1].data_write.in); // forward message to dTLB
								sTLB->walk.data_sent = 1;
							}
							else
								sTLB->walk.data_sent = 0;
							copy_data_bus_info(&sTLB->walk.latch_data, &bus2[1].data_write.in);
							sTLB->walk.latch_data.data[1] = 0;
						}
												  break;
						case io_addr_L0C_ctrl: {
							if (bus2[1].data_write.in.data[0] == 1) {
								if (debug_unit) {
									fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: enable L0C command received; address/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], clock);
								}
								sTLB->cL1.way_ptr = 0;

								copy_data_bus_info(&bus2[0].data_read.out, &bus2[1].data_write.in); // forward message to dTLB
								copy_data_bus_info(&sTLB->walk.latch_data, &bus2[1].data_write.in);

								sTLB->walk.latch_data.data[1] = 0;
							}
							else if (bus2[1].data_write.in.data[0] == 2) {
								if (debug_unit) {
									fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: flush L0C command received; address/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], clock);
								}
								sTLB->cL1.way_ptr = 0;

								copy_data_bus_info(&bus2[0].data_read.out, &bus2[1].data_write.in); // forward message to dTLB
								copy_data_bus_info(&sTLB->walk.latch_data, &bus2[1].data_write.in);

								sTLB->walk.latch_data.data[1] = 0;
							}
							else {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: ERROR; address/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], clock);
							}
						}
											 break;
						case io_addr_L0D_ctrl: {
							if (debug_unit) {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk:  L0D ctrl reg command received; address/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], clock);
							}
							sTLB->dL1.way_ptr = 0;
							copy_data_bus_info(&bus2[1].data_read.out, &bus2[1].data_write.in); // forward message to dTLB
							copy_data_bus_info(&sTLB->walk.latch_data, &bus2[1].data_write.in);
							sTLB->walk.latch_data.data[1] = 0;
						}
											 break;
						case io_addr_L2_ctrl: {
							if (debug_unit) {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk:  uL2 ctrl reg command received, ReLease lock; address/data: 0x%016I64x/0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, bus2[1].data_write.in.data[0], clock);
							}
							sTLB->dL1.way_ptr = 0;
							copy_data_bus_info(&sTLB->walk.latch_data, &bus2[1].data_write.in);
							sTLB->walk.latch_data.data[1] = 0;
							sTLB->walk.lock = 0;
							sTLB->walk.reg_access_out = 0;
							sTLB->walk.est_lockout = 0;
							sTLB->walk.lockout = 0;
						}
											break;
						default:
							debug++;
							break;
						}
					}
				}
			}
			if (bus2[0].data_write.in.snoop_response == snoop_dirty) {
				if (bus2[0].data_write.in.cacheable == page_IO_error_rsp) {// error response cycle
					if (bus2[0].data_write.in.xaction_id == sTLB->walk.latch_addr.xaction_id) {
						sTLB->walk.latch_data.data[0] = bus2[0].data_write.in.data[0];

						copy_data_bus_info(&bus2[1].data_read.out, &bus2[0].data_write.in);
						sTLB->walk.latch_addr.strobe = 0;
						sTLB->walk.reg_access_out = 0;
						sTLB->walk.est_lockout = 0;
						sTLB->walk.lockout = 0;
						if (debug_unit) {
							fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: error response from cTLB to core; data: 0x%016x; clock: 0x%04llx\n",
								mhartid, bus2[0].data_write.in.xaction_id, bus2[0].data_write.in.data[0], clock);
						}
						sTLB->walk.latch_data.data[1] = 0;
					}
				}
				else {
					debug++;
				}
			}
			if (sTLB->walk.latch_data.cacheable != page_IO_error_rsp && sTLB->walk.latch_data.cacheable != page_IO) {// error response cycle
				if (bus2[1].data_write.in.snoop_response == snoop_hit || bus2[1].data_write.in.snoop_response == snoop_dirty) {
					if ((bus2[1].data_read.in.xaction_id == sTLB->walk.latch_addr.xaction_id) && (sTLB->walk.latch_addr.xaction & 0x01)) {
						sTLB->walk.lock = 0;
						sTLB->walk.latch_addr.strobe = 0;
						if (debug_unit) {
							fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: release lock data from bus2 b; clock: 0x%04llx\n", mhartid, bus2[1].data_read.in.xaction_id, clock);
						}
					}
					if (bus2[1].data_write.in.xaction_id == sTLB->walk.latch_addr.xaction_id && ((sTLB->walk.latch_addr.xaction & 0xfc) == bus_LR)) {
						sTLB->walk.lock = 0;
						sTLB->walk.latch_addr.strobe = 0;
						sTLB->walk.reg_access_out = 0;
						sTLB->walk.est_lockout = 0;
						sTLB->walk.lockout = 0;
						if (sTLB->walk.latch_addr.addr == io_addr_TLB_data_ctrl) {
							if (debug_unit) {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: read response from dTLB ctrl reg; data: 0x%08llx,clock: 0x%04llx\n", mhartid, bus2[1].data_write.in.xaction_id, bus2[1].data_write.in.data[0], clock);
							}
						}
						else {
							if (debug_unit) {
								fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: read response from L0D ctrl reg; data: 0x%08llx,clock: 0x%04llx\n", mhartid, bus2[1].data_write.in.xaction_id, bus2[1].data_write.in.data[0], clock);
							}
						}
						copy_data_bus_info(&bus2[1].data_read.out, &bus2[1].data_write.in);
					}
				}
				else if (bus2[0].data_write.in.snoop_response == snoop_hit || bus2[0].data_write.in.snoop_response == snoop_dirty) {
					if (bus2[0].data_write.in.xaction_id == sTLB->walk.latch_addr.xaction_id) {
						sTLB->walk.lock = 0;
						sTLB->walk.est_lockout = 0;

						sTLB->walk.lockout = 0;
						if (debug_unit) {
							fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: read response from L0C ctrl reg; clock: 0x%04llx\n", mhartid, bus2[0].data_write.in.xaction_id, clock);
						}
						copy_data_bus_info(&bus2[1].data_read.out, &bus2[0].data_write.in);
					}
				}
			}
			if (sTLB->walk.latch_data.data[1] == 3) { // ERROR, need to put in code for executable & data (WR) pages
				copy_data_bus_info(&bus2[1].data_read.out, &sTLB->walk.latch_data);
				bus2[1].data_read.out.xaction_id |= 0x0080;// match id to store_amo queue. read path == 2, write path == 3
				sTLB->walk.latch_addr.strobe = 0;
				sTLB->walk.est_lockout = 0;
				if (debug_unit) {
					fprintf(debug_stream, "sTLB(%d) xaction id %#06x, walk: error response to core; clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, clock);
				}
				sTLB->walk.latch_data.data[1] = 0;
			}
		}
	}
}
void snoop_sTLB_cache(addr_bus_type* snoop_L2, addr_bus_type* snoop_L0C, addr_bus_type* snoop_L0D, UINT8* hit, fully_assoc_TLB_cache_type* L1, addr_bus_type* snoop_latch, UINT64 clock) {
	for (UINT16 i = 0; i < L1->way_ptr && hit[0] == 0; i++) {
		if ((snoop_latch->addr & (~0x0fff)) == (L1->way[i].p_addr & (~0x0fff))) { // swappable memory
			hit[0] = 1;
			copy_addr_bus_info(snoop_L2, snoop_latch, clock);// snoop L2
			switch ((L1->way[i].directory >> 1) & 7) {
			case 1:
			case 2:
			case 3:
				copy_addr_bus_info(snoop_L0D, snoop_latch, clock);// snoop L2
				snoop_L0D->addr = (L1->way[i].l_addr & (~0x0fff)) | (snoop_latch->addr & 0x0fff);
				break;
			case 4:
				copy_addr_bus_info(snoop_L0C, snoop_latch, clock);// snoop L2
				snoop_L0C->addr = (L1->way[i].l_addr & (~0x0fff)) | (snoop_latch->addr & 0x0fff);
				break;
			default:
				copy_addr_bus_info(snoop_L0D, snoop_latch, clock);// snoop L2
				snoop_L0D->addr = (L1->way[i].l_addr & (~0x0fff)) | (snoop_latch->addr & 0x0fff);
				copy_addr_bus_info(snoop_L0C, snoop_latch, clock);// snoop L2
				snoop_L0C->addr = (L1->way[i].l_addr & (~0x0fff)) | (snoop_latch->addr & 0x0fff);
				break;
			}
		}
	}
}
void sTLB_L1_cache(addr_bus_type* snoop_L0C, addr_bus_type* snoop_L0D, addr_bus_type* snoop_L2, bus_w_snoop_signal1* bus3, UINT64 clock, sTLB_type* sTLB, UINT mhartid, UINT debug_core, param_type *param, FILE* debug_stream) {
	UINT debug = 0;
	UINT debug_common = (param->PAGE_WALK || param->PAGE_WALK || param->TLB || param->EXT_SNOOP) && clock >= param->start_time;
	UINT debug_unit = debug_common && debug_core;
	if (mhartid == 8) {
		if (clock >= 0x0115)// 0xa381
			debug++;
	}
	if ((clock & 1) == 0) {
		if (sTLB->walk.lockout == 0) {
			if (sTLB->walk.est_lockout) {
				bus3->data_write.out.snoop_response = snoop_stall;
				if (debug_unit) {
					fprintf(debug_stream, "sTLB(%d) xaction id 0x%04x, locked cycle request snoop stall; logical addr: 0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, clock);
				}
				if (bus3->data_write.in.snoop_response == snoop_stall) {
					UINT ext_snoop = sTLB->ext_snoop_track[0].strobe | sTLB->ext_snoop_track[1].strobe | sTLB->ext_snoop_track[2].strobe | sTLB->ext_snoop_track[3].strobe;
					if (ext_snoop == 0 && bus3->snoop_addr.in.strobe == 0) {
						sTLB->walk.lockout = 1;
						if (debug_unit) {
							fprintf(debug_stream, "sTLB(%d) xaction id 0x%04x, lock-out established; clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, clock);
						}
					}
					else {
						if (debug_unit) {
							if (sTLB->ext_snoop_track[0].strobe)
								fprintf(debug_stream, "sTLB(%d) xaction id 0x%04x, locked cycle request snoop stall(0); snoop xaction id 0x%04x, snoop clock: 0x%04llx; clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->ext_snoop_track[0].xaction_id, sTLB->ext_snoop_track[0].clock, clock);
							if (sTLB->ext_snoop_track[1].strobe)
								fprintf(debug_stream, "sTLB(%d) xaction id 0x%04x, locked cycle request snoop stall(1); snoop xaction id 0x%04x, snoop clock: 0x%04llx; clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->ext_snoop_track[1].xaction_id, sTLB->ext_snoop_track[1].clock, clock);
							if (sTLB->ext_snoop_track[2].strobe)
								fprintf(debug_stream, "sTLB(%d) xaction id 0x%04x, locked cycle request snoop stall(2); snoop xaction id 0x%04x, snoop clock: 0x%04llx; clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->ext_snoop_track[2].xaction_id, sTLB->ext_snoop_track[2].clock, clock);
							if (sTLB->ext_snoop_track[3].strobe)
								fprintf(debug_stream, "sTLB(%d) xaction id 0x%04x, locked cycle request snoop stall(3); snoop xaction id 0x%04x, snoop clock: 0x%04llx; clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->ext_snoop_track[3].xaction_id, sTLB->ext_snoop_track[3].clock, clock);
						}
					}
				}
			}
		}
		else {
			bus3->data_write.out.snoop_response = snoop_stall;
			if (debug_unit) {
				fprintf(debug_stream, "sTLB(%d) xaction id %#06x, locked cycle snoop stall; logical address: 0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, clock);
			}
		}
		if (sTLB->walk.reg_access_in != 0) {
			bus3->data_write.out.snoop_response = snoop_stall;
			if (debug_unit) {
				fprintf(debug_stream, "sTLB(%d) xaction id %#06x, stall bus3 caches; reg_access_in; logical address: 0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->walk.latch_addr.xaction_id, sTLB->walk.latch_addr.addr, clock);
			}
		}
		if (bus3->data_write.in.snoop_response != snoop_idle && bus3->data_write.in.snoop_response != snoop_stall) {
			if (sTLB->ext_snoop_track[0].strobe == 1) {
				if (bus3->data_write.in.xaction_id == sTLB->ext_snoop_track[0].xaction_id) {
					sTLB->ext_snoop_track[0].strobe = 0;
				}
			}
			if (sTLB->ext_snoop_track[1].strobe == 1) {
				if (bus3->data_write.in.xaction_id == sTLB->ext_snoop_track[1].xaction_id) {
					sTLB->ext_snoop_track[1].strobe = 0;
				}
			}
			if (sTLB->ext_snoop_track[2].strobe == 1) {
				if (bus3->data_write.in.xaction_id == sTLB->ext_snoop_track[2].xaction_id) {
					sTLB->ext_snoop_track[2].strobe = 0;
				}
			}
			if (sTLB->ext_snoop_track[3].strobe == 1) {
				if (bus3->data_write.in.xaction_id == sTLB->ext_snoop_track[3].xaction_id) {
					sTLB->ext_snoop_track[3].strobe = 0;
				}
			}
		}
		UINT snoop_latch_strobe = 0;
		for (UINT8 ptr = 0; ptr < 4 && (bus3->data_write.out.snoop_response == snoop_idle || bus3->data_write.out.snoop_response == snoop_stall) && snoop_L2->strobe == 0; ptr++) {
			if (sTLB->snoop_latch[ptr].strobe == 1 && !sTLB->walk.reg_access_in && sTLB->snoop_latch[sTLB->snoop_latch_ptr].strobe == 0) {
				snoop_latch_strobe = 1;
				UINT debug_extern = (debug_common && (param->core & (1 << (sTLB->snoop_latch[ptr].xaction_id & 0x0f))));
				if (sTLB->snoop_latch[ptr].cacheable != page_non_cache) {// logical == physical address
					copy_addr_bus_info(snoop_L2, &sTLB->snoop_latch[ptr], clock);// snoop L2
					copy_addr_bus_info(snoop_L0C, &sTLB->snoop_latch[ptr], clock);// snoop L2
					copy_addr_bus_info(snoop_L0D, &sTLB->snoop_latch[ptr], clock);// snoop L2
					if (sTLB->ext_snoop_track[sTLB->ext_snoop_ptr].strobe == 1)
						debug++;
					copy_addr_bus_info(&sTLB->ext_snoop_track[sTLB->ext_snoop_ptr], &sTLB->snoop_latch[ptr], clock);
					sTLB->ext_snoop_ptr = ((sTLB->ext_snoop_ptr + 1) & 3);
					bus3->data_write.out.snoop_response = snoop_stall;
					if (debug_unit || debug_extern) {
						fprintf(debug_stream, "sTLB(%d) xaction id %#06x, cache_non_swap, snoop core caches (ptr: %d); logical address: 0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->snoop_latch[ptr].xaction_id, ptr, sTLB->snoop_latch[ptr].addr, clock);
					}
				}
				else {
					UINT8 hit = 0;
					snoop_sTLB_cache(snoop_L2, snoop_L0C, snoop_L0D, &hit, &sTLB->cL1, &sTLB->snoop_latch[ptr], clock);
					snoop_sTLB_cache(snoop_L2, snoop_L0C, snoop_L0D, &hit, &sTLB->dL1, &sTLB->snoop_latch[ptr], clock);
					if (hit) {
						if (sTLB->ext_snoop_track[sTLB->ext_snoop_ptr].strobe == 1)
							debug++;
						copy_addr_bus_info(&sTLB->ext_snoop_track[sTLB->ext_snoop_ptr], &sTLB->snoop_latch[ptr], clock);
						sTLB->ext_snoop_ptr = ((sTLB->ext_snoop_ptr + 1) & 3);
						bus3->data_write.out.snoop_response = snoop_stall;
						if (debug_unit || debug_extern)
							fprintf(debug_stream, "sTLB(%d) xaction id %#06x, cache_swap HIT address: 0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->snoop_latch[ptr].xaction_id, sTLB->snoop_latch[ptr].addr, clock);
					}
					else {
						copy_addr_bus_info(&bus3->data_write.out, &sTLB->snoop_latch[ptr]);// snoop L2
						bus3->data_write.out.snoop_response = snoop_miss;
						if (debug_unit || debug_extern)
							fprintf(debug_stream, "sTLB(%d) xaction id %#06x, cache_swap MISS address: 0x%016I64x, clock: 0x%04llx\n", mhartid, sTLB->snoop_latch[ptr].xaction_id, sTLB->snoop_latch[ptr].addr, clock);
					}
				}
				sTLB->snoop_latch[ptr].strobe = 0;
			}
		}
		if (bus3->snoop_addr.in.strobe == 1 ) {
			if (sTLB->walk.reg_access_in) {
				if (sTLB->snoop_latch[sTLB->snoop_latch_ptr].strobe == 1)
					debug++;
				copy_addr_bus_info(&sTLB->snoop_latch[sTLB->snoop_latch_ptr], &bus3->snoop_addr.in, clock);// snoop L2
				sTLB->snoop_latch_ptr = ((sTLB->snoop_latch_ptr + 1) & 3);
				if (debug_unit) {
					fprintf(debug_stream, "sTLB(%d) xaction id %#06x, active reg update, external snoop latched and delayed (ptr %d) address: 0x%016I64x, clock: 0x%04llx\n", mhartid, bus3->snoop_addr.in.xaction_id, sTLB->snoop_latch_ptr, bus3->snoop_addr.in.addr, clock);
				}
			}
			else if (snoop_L2->strobe == 1 || bus3->data_write.out.snoop_response == snoop_miss) {
				if (sTLB->snoop_latch[sTLB->snoop_latch_ptr].strobe == 1)
					debug++;
				copy_addr_bus_info(&sTLB->snoop_latch[sTLB->snoop_latch_ptr], &bus3->snoop_addr.in, clock);// snoop L2
				sTLB->snoop_latch_ptr = ((sTLB->snoop_latch_ptr + 1) & 3);
				if (debug_unit) {
					fprintf(debug_stream, "sTLB(%d) xaction id %#06x, L2 snoop bus busy, external snoop latched and delayed (ptr %d) address: 0x%016I64x, clock: 0x%04llx\n", mhartid, bus3->snoop_addr.in.xaction_id, sTLB->snoop_latch_ptr, bus3->snoop_addr.in.addr, clock);
				}
			}
			else {
				UINT8 hit = 0;
				if (bus3->snoop_addr.in.cacheable != page_non_cache) {// logical == physical address (OS space)
					hit = 1;
					copy_addr_bus_info(snoop_L2, &bus3->snoop_addr.in, clock);// snoop L2
					copy_addr_bus_info(snoop_L0C, &bus3->snoop_addr.in, clock);// snoop L2
					copy_addr_bus_info(snoop_L0D, &bus3->snoop_addr.in, clock);// snoop L2
				}
				snoop_sTLB_cache(snoop_L2, snoop_L0C, snoop_L0D, &hit, &sTLB->cL1, &bus3->snoop_addr.in, clock);
				snoop_sTLB_cache(snoop_L2, snoop_L0C, snoop_L0D, &hit, &sTLB->dL1, &bus3->snoop_addr.in, clock);
				if (hit) {
					if (sTLB->ext_snoop_track[sTLB->ext_snoop_ptr].strobe == 1) {
						if (!(sTLB->ext_snoop_track[0].strobe && sTLB->ext_snoop_track[1].strobe && sTLB->ext_snoop_track[2].strobe && sTLB->ext_snoop_track[3].strobe)) {
							while (sTLB->ext_snoop_track[sTLB->ext_snoop_ptr].strobe == 1) {
								sTLB->ext_snoop_ptr = ((sTLB->ext_snoop_ptr + 1) & 3);
							}
						}
						else {
							fprintf(debug_stream, "sTLB(%d) xaction id %#06x, ERROR: bus snoop overwrite (ptr: %d); logical address: 0x%016I64x, clock: 0x%04llx\n", mhartid, bus3->snoop_addr.in.xaction_id, sTLB->ext_snoop_ptr, bus3->snoop_addr.in.addr, clock);
						}
					}
					copy_addr_bus_info(&sTLB->ext_snoop_track[sTLB->ext_snoop_ptr], &bus3->snoop_addr.in, clock);
					sTLB->ext_snoop_ptr = ((sTLB->ext_snoop_ptr + 1) & 3);
					if (debug_unit || (debug_common && ((param->core == 1 && (bus3->snoop_addr.in.xaction_id & 7) == 0) || (param->core == 2 && (bus3->snoop_addr.in.xaction_id & 7) == 1) || (param->core == 4 && (bus3->snoop_addr.in.xaction_id & 7) == 2) || (param->core == 8 && (bus3->snoop_addr.in.xaction_id & 7) == 3) ||
						(param->core == 0X10 && (bus3->snoop_addr.in.xaction_id & 7) == 4) || (param->core == 0x20 && (bus3->snoop_addr.in.xaction_id & 7) == 5) || (param->core == 0x40 && (bus3->snoop_addr.in.xaction_id & 7) == 6) || (param->core == 0x80 && (bus3->snoop_addr.in.xaction_id & 7) == 7)))) {
						fprintf(debug_stream, "sTLB(%d) xaction id %#06x, TLB HIT, snoop core 1 caches (ptr: %d); l addr: 0x%016I64x,p addr: 0x%016I64x, clock: 0x%04llx\n", mhartid, bus3->snoop_addr.in.xaction_id, sTLB->ext_snoop_ptr, snoop_L0C->addr, bus3->snoop_addr.in.addr, clock);
					}
				}
				else {
					copy_addr_bus_info(&bus3->data_write.out, &bus3->snoop_addr.in);// snoop L2
					bus3->data_write.out.snoop_response = snoop_miss;
					if (debug_unit || (debug_common && ((param->core == 1 && (bus3->snoop_addr.in.xaction_id & 7) == 0) || (param->core == 2 && (bus3->snoop_addr.in.xaction_id & 7) == 1) || (param->core == 4 && (bus3->snoop_addr.in.xaction_id & 7) == 2) || (param->core == 8 && (bus3->snoop_addr.in.xaction_id & 7) == 3) ||
						(param->core == 0X10 && (bus3->snoop_addr.in.xaction_id & 7) == 4) || (param->core == 0x20 && (bus3->snoop_addr.in.xaction_id & 7) == 5) || (param->core == 0x40 && (bus3->snoop_addr.in.xaction_id & 7) == 6) || (param->core == 0x80 && (bus3->snoop_addr.in.xaction_id & 7) == 7)))) {
						fprintf(debug_stream, "sTLB(%d) xaction id %#06x, L1 Miss logical address: 0x%016I64x, clock: 0x%04llx\n", mhartid, bus3->snoop_addr.in.xaction_id, bus3->snoop_addr.in.addr, clock);
					}
				}
			}
		}
	}
}
void shadow_tlb_unit(bus_w_snoop_signal1* bus2, addr_bus_type* snoop_L2, bus_w_snoop_signal1* bus3, UINT64 clock, sTLB_type* sTLB, UINT mhartid, UINT debug_core, param_type *param, FILE* debug_stream) {
	sTLB->walk.reg_access_in = sTLB->walk.reg_access_out;
	shadow_tlb_walk_unit(bus2, bus3->data_read.in, clock, sTLB, mhartid, param, debug_stream);// need to fix csr data bus to input and output
	sTLB_L1_cache(&bus2[0].snoop_addr.out, &bus2[1].snoop_addr.out, snoop_L2, bus3, clock, sTLB, mhartid, debug_core, param, debug_stream);
}