// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "ROB.h"

void write_poster_data(store_buffer_type* buffer, UINT64 clock, UINT mhartid, int debug_unit, FILE* debug_stream) {
	if (debug_unit) {
		UINT64* data_out = buffer->data_out;
		fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x 0x00 0x%016x, 0x08 0x%016x, 0x10 0x%016x, 0x18 0x%016x",
			mhartid, buffer->ROB_ptr, data_out[0], data_out[1], data_out[2], data_out[3]);
		fprintf(debug_stream, " clock: 0x%04llx\n", clock);
		fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x 0x20 0x%016x, 0x28 0x%016x, 0x30 0x%016x, 0x38 0x%016x",
			mhartid, buffer->ROB_ptr, data_out[4], data_out[5], data_out[6], data_out[7]);
		fprintf(debug_stream, " clock: 0x%04llx\n", clock);
		fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x 0x40 0x%016x, 0x48 0x%016x, 0x50 0x%016x, 0x58 0x%016x",
			mhartid, buffer->ROB_ptr, data_out[8], data_out[9], data_out[10], data_out[11]);
		fprintf(debug_stream, " clock: 0x%04llx\n", clock);
		fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x 0x60 0x%016x, 0x68 0x%016x, 0x70 0x%016x, 0x78 0x%016x",
			mhartid, buffer->ROB_ptr, data_out[12], data_out[13], data_out[14], data_out[15]);
		fprintf(debug_stream, " clock: 0x%04llx\n", clock);
	}
}
void merge_data_out(store_buffer_type* buffer, UINT64* data) {

	for (UINT8 j = 0; j < 8; j++) {
		if ((buffer->valid_out_l >> (j + 0) & 0x1) == 0) {
			buffer->data_out[0] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[0] |= data[0] & ((INT64)0xff << (8 * j));
		}
		if ((buffer->valid_out_l >> (j + 8) & 0x1) == 0) {
			buffer->data_out[1] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[1] |= data[1] & ((INT64)0xff << (8 * j));
		}
		if ((buffer->valid_out_l >> (j + 16) & 0x1) == 0) {
			buffer->data_out[2] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[2] |= data[2] & ((INT64)0xff << (8 * j));
		}
		if ((buffer->valid_out_l >> (j + 24) & 0x1) == 0) {
			buffer->data_out[3] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[3] |= data[3] & ((INT64)0xff << (8 * j));
		}
		if ((buffer->valid_out_l >> (j + 32) & 0x1) == 0) {
			buffer->data_out[4] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[4] |= data[4] & ((INT64)0xff << (8 * j));
		}
		if ((buffer->valid_out_l >> (j + 40) & 0x1) == 0) {
			buffer->data_out[5] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[5] |= data[5] & ((INT64)0xff << (8 * j));
		}
		if ((buffer->valid_out_l >> (j + 48) & 0x1) == 0) {
			buffer->data_out[6] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[6] |= data[6] & ((INT64)0xff << (8 * j));
		}
		if ((buffer->valid_out_l >> (j + 56) & 0x1) == 0) {
			buffer->data_out[7] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[7] |= data[7] & ((INT64)0xff << (8 * j));
		}

		if ((buffer->valid_out_h >> (j + 0) & 0x1) == 0) {
			buffer->data_out[8] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[8] |= data[8] & ((INT64)0xff << (8 * j));
		}
		if ((buffer->valid_out_h >> (j + 8) & 0x1) == 0) {
			buffer->data_out[9] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[9] |= data[9] & ((INT64)0xff << (8 * j));
		}
		if ((buffer->valid_out_h >> (j + 16) & 0x1) == 0) {
			buffer->data_out[10] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[10] |= data[10] & ((INT64)0xff << (8 * j));
		}
		if ((buffer->valid_out_h >> (j + 24) & 0x1) == 0) {
			buffer->data_out[11] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[11] |= data[11] & ((INT64)0xff << (8 * j));
		}
		if ((buffer->valid_out_h >> (j + 32) & 0x1) == 0) {
			buffer->data_out[12] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[12] |= data[12] & ((INT64)0xff << (8 * j));
		}
		if ((buffer->valid_out_h >> (j + 40) & 0x1) == 0) {
			buffer->data_out[13] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[13] |= data[13] & ((INT64)0xff << (8 * j));
		}
		if ((buffer->valid_out_h >> (j + 48) & 0x1) == 0) {
			buffer->data_out[14] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[14] |= data[14] & ((INT64)0xff << (8 * j));
		}
		if ((buffer->valid_out_h >> (j + 56) & 0x1) == 0) {
			buffer->data_out[15] &= (~((INT64)0xff << (8 * j)));
			buffer->data_out[15] |= data[15] & ((INT64)0xff << (8 * j));
		}
	}
	buffer->valid_out_l = 0xffffffffffffffff;
	buffer->valid_out_h = 0xffffffffffffffff;
}
void write_data_out(UINT16* exec_rsp2, data_bus_type* logical_data_out, Store_type* store_var, 
	UINT64 clock, int debug_unit, int debug_bus, UINT mhartid, param_type* param, FILE* debug_stream) {
	int debug = 0;
	if (mhartid == 0)
		if (clock == 0x1922)
			debug++;
	for (UINT8 i = 0; i < 4; i++) {
		if ((store_var->buffer[i].status == store_w2_addr_valid) && logical_data_out->snoop_response != snoop_dirty) {
			logical_data_out->snoop_response = snoop_dirty;//modified data
			logical_data_out->xaction_id = store_var->buffer[i].xaction_id;
			logical_data_out->cacheable = store_var->buffer[i].cacheable;
			logical_data_out->valid = 0xffffffffffffffff;
			for (UINT8 j = 0; j < 0x10; j++) logical_data_out->data[j] = store_var->buffer[i].data_out[j];
			if (debug_unit || debug_bus) {
				fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, write data0 sWC(%d) xaction id %#06x, addr ",
					mhartid, store_var->buffer[i].ROB_ptr, i, logical_data_out->xaction_id);
				fprint_addr_coma(debug_stream, store_var->buffer[i].addr, param);
				fprintf(debug_stream, " clock %#04llx\n", clock);
			}
			store_var->buffer[i].valid_out_h = 0;
			store_var->buffer[i].valid_out_l = 0;
			store_var->buffer[i].xaction = bus_idle;
			store_var->buffer[i].status = store_retire;
			store_var->buffer[i].index = ((store_var->buffer[i].index + 1) & 0x03);
			store_var->buffer[i].index = ((store_var->buffer[i].index + 1) & 0x03);
		}
		if (store_var->buffer[i].status == store_w1_addr_valid) {
			store_var->buffer[i].status = store_w2_addr_valid;
			if (debug_unit) {
				fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, store_w2_addr_valid sWC(%d) xaction id %#06x, addr ",
					mhartid, store_var->buffer[i].ROB_ptr, i, store_var->buffer[i].xaction_id);
				fprint_addr_coma(debug_stream, store_var->buffer[i].addr, param);
				fprintf(debug_stream, " clock %#04llx\n", clock);
			}
		}
		if (store_var->buffer[i].status == store_w_addr_valid) {
			store_var->buffer[i].status = store_w1_addr_valid;
			if (debug_unit) {
				fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, store_w1_addr_valid sWC(%d) xaction id %#06x, addr ",
					mhartid, store_var->buffer[i].ROB_ptr, i, store_var->buffer[i].xaction_id);
				fprint_addr_coma(debug_stream, store_var->buffer[i].addr, param);
				fprintf(debug_stream, " clock %#04llx\n", clock);
			}
		}
	}
}
void data_from_l0(reg_bus* rd, UINT16* exec_rsp2, addr_bus_type* logical_addr, data_bus_type* physical_data_in, Store_type* store_var, UINT16 retire_num, UINT8 block_loads,UINT8 priviledge,
	UINT64 clock, csr_type* csr, int debug_unit, int debug_bus, UINT mhartid, param_type* param, FILE* debug_stream) {
	int debug = 0;
	if (physical_data_in->snoop_response == snoop_hit || physical_data_in->snoop_response == snoop_dirty) {
		if (mhartid == 0)
			if (clock == 0x35d5)
				debug++;
		UINT8 i = (physical_data_in->xaction_id >> 12) & 3;
		if (physical_data_in->xaction_id == store_var->alloc[i].xaction_id) {
			if (store_var->alloc[i].status != store_inactive) {
				if (store_var->alloc[i].xaction == bus_allocate) {
					merge_data_out(&store_var->buffer[i], physical_data_in->data);
					store_var->buffer[i].xaction_id = ((store_var->buffer[i].index << 14) | (i << 12) | (3 << 8) | (1 << 7) | mhartid);
					store_var->buffer[i].cacheable = physical_data_in->cacheable;
					if (logical_addr->strobe == 0 && (retire_num == store_var->buffer[i].ROB_ptr || store_var->buffer[i].status == store_TLB_and_retire_valid)) {
						store_var->alloc[i].status = store_inactive;
						store_var->buffer[i].status = store_w_addr_valid;
						logical_addr->strobe = 1;
						logical_addr->addr = store_var->buffer[i].addr;
						logical_addr->cacheable = physical_data_in->cacheable;
						logical_addr->xaction = bus_store_full;
						csr[csr_store_issued].value++;
						switch (priviledge) {
						case 0:
							break;
						case 1:
							csr[csr_sstore_issued].value++;
							break;
						case 2:
							csr[csr_hstore_issued].value++;
							break;
						case 3:
						case 0x0f:
							csr[csr_mstore_issued].value++;
							break;
						default:
							debug++;
							break;
						}
						logical_addr->xaction_id = ((store_var->buffer[i].index << 14) | (i << 12) | (3 << 8) | (1 << 7) | mhartid);
						store_var->buffer[i].xaction_id = logical_addr->xaction_id;
						exec_rsp2[0] = 0x8000 | store_var->buffer[i].ROB_ptr;
						if (debug_unit || debug_bus) {
							fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, ALLOC received, write merged sWC(%d), writeback addr ",
								mhartid, store_var->buffer[i].ROB_ptr, physical_data_in->xaction_id, i);
							fprint_addr_coma(debug_stream, logical_addr->addr, param);
							fprintf(debug_stream, " write xaction id %#06x,  clock %#04llx\n", logical_addr->xaction_id, clock);
						}
					}
					else if (block_loads) {
						store_var->alloc[i].status = store_inactive;
						store_var->buffer[i].status = store_inactive;
						exec_rsp2[0] = 0x8000 | store_var->buffer[i].ROB_ptr;
						if (debug_unit || debug_bus) {
							fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, ALLOC received sWC(%d), speculative, wrong branch, abort ",
								mhartid, store_var->buffer[i].ROB_ptr, physical_data_in->xaction_id, i);
							fprint_addr_coma(debug_stream, logical_addr->addr, param);
							fprintf(debug_stream, " write xaction id %#06x,  clock %#04llx\n", logical_addr->xaction_id, clock);
						}
					}
					else {
						store_var->buffer[i].status = store_data_merged;
						store_var->alloc[i].status = store_inactive;
						store_var->buffer[i].ROB_ptr = store_var->alloc[i].ROB_ptr;
						if (debug_unit || debug_bus) {
							fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, ALLOC received, write merged sWC(%d), no speculative write, addr ",
								mhartid, store_var->buffer[i].ROB_ptr, physical_data_in->xaction_id, i, store_var->buffer[i].addr, store_var->buffer[i].xaction_id, clock);
							fprint_addr_coma(debug_stream, store_var->buffer[i].addr, param);
							fprintf(debug_stream, " write xaction id %#06x, clock %#04llx\n", store_var->buffer[i].xaction_id, clock);
						}
					}
				}
				else {
					exec_rsp2[0] = 0x8000 | store_var->buffer[i].ROB_ptr;
					UINT64 addr = store_var->buffer[i].addr & 0xfffffffe;
					if (debug_unit || debug_bus) {
						fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, AMO received addr,data ",
							mhartid, store_var->alloc[i].ROB_ptr, physical_data_in->xaction_id);
						fprint_addr_coma(debug_stream, addr, param);
						fprintf(debug_stream, "0x%016I64x clock %#04llx\n", physical_data_in->data[(addr >> 3) & 0x0f], clock);
					}
					rd->data = physical_data_in->data[(addr >> 3) & 0x0f];
					rd->strobe = 1;
					rd->ROB_id = store_var->alloc[i].ROB_ptr;
					store_var->alloc[i].status = store_inactive;
				}
			}
		}
		else {
			fprintf(debug_stream, "STORE(%lld): ERROR: invalid physical bus xaction id %#06x, clock %#04llx\n", mhartid, physical_data_in->xaction_id, clock);
		}
	}
}
void data_from_l2(reg_bus* rd, UINT16* exec_rsp2, addr_bus_type* logical_addr, data_bus_type* bus2_data_in, data_bus_type* physical_data_in, Store_type* store_var, UINT16 retire_num,UINT8 priviledge,
	UINT64 clock, int debug_unit, int debug_bus, int debug_unit_walk, csr_type* csr, UINT mhartid, param_type* param, FILE* debug_stream) {
	int debug = 0;
	if ((bus2_data_in->snoop_response == snoop_dirty || bus2_data_in->snoop_response == snoop_hit) && bus2_data_in->snoop_response != snoop_stall && ((bus2_data_in->xaction_id & (2 << 8)) != 0) && ((bus2_data_in->xaction_id & (3 << 6)) != 0)) {// last cmp: not an external snoop
		UINT8 i = (bus2_data_in->xaction_id >> 12) & 3;
		if (store_var->alloc[i].status != store_inactive && bus2_data_in->xaction_id == bus2_data_in->xaction_id) {
			if (store_var->alloc[i].xaction == bus_LR_aq_rl) {// locked read
				exec_rsp2[0] = 0x8000 | store_var->alloc[i].ROB_ptr;
				UINT64 addr = store_var->alloc[i].addr & 0xfffffffe;
				if (addr < csr[csr_mbound].value)// UC is I/O access, aligned
					rd->data = bus2_data_in->data[0];
				else
					rd->data = bus2_data_in->data[((addr >> 3) & 0x0f) | 0];
				if (debug_unit || debug_unit_walk || debug_bus) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x xaction id %#06x, LR address/data: 0x%016I64x / 0x%016I64x,clock %#04llx\n",
						mhartid, store_var->alloc[i].ROB_ptr, bus2_data_in->xaction_id, addr, rd->data, clock);
				}
				rd->strobe = 1;
				rd->ROB_id = store_var->alloc[i].ROB_ptr;
				store_var->alloc[i].status = store_inactive;
			}
			else if (store_var->alloc[i].xaction == bus_SC_aq_rl) {//locked store
				if (store_var->IO_track.strobe && store_var->IO_track.xaction_id == bus2_data_in->xaction_id && bus2_data_in->cacheable != page_IO_error_rsp) {
					if (debug_unit || debug_unit_walk || debug_bus) {
						fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, IO reg access to dTLB or dL0; addr,data: 0x%016x,0x%016x; clock %#04llx\n",
							mhartid, store_var->buffer[i].ROB_ptr, bus2_data_in->xaction_id, store_var->IO_track.addr, bus2_data_in->data[0], clock);
					}
				}
				else if (bus2_data_in->cacheable == page_IO_error_rsp) {
					if (debug_unit || debug_unit_walk || debug_bus) {
						fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, SC error response received: 0x%016x; clock %#04llx\n",
							mhartid, store_var->buffer[i].ROB_ptr, bus2_data_in->xaction_id, bus2_data_in->data[0], clock);
					}
					exec_rsp2[0] = 0x8000 | store_var->buffer[i].ROB_ptr;
					UINT64 addr = store_var->buffer[i].addr & 0xfffffffe;
					addr >>= 3;
					addr &= 0x0f;
					if (addr < csr[csr_mbound].value)// UC is I/O access, aligned
						rd->data = bus2_data_in->data[0];
					else
						rd->data = bus2_data_in->data[addr | 0];
					rd->strobe = 1;
					rd->ROB_id = store_var->buffer[i].ROB_ptr;

					store_var->buffer[i].status = store_inactive;
					store_var->lock = 0;
					store_var->alloc[i].status = store_inactive;
				}
				else {
					debug++;
				}
				store_var->IO_track.strobe = 0;
			}
			else {
				merge_data_out(&store_var->buffer[i], bus2_data_in->data);

				store_var->buffer[i].cacheable = bus2_data_in->cacheable;
				store_var->buffer[i].xaction_id = ((store_var->buffer[i].index << 14) | (i << 12) | (3 << 8) | (1 << 7) | csr[csr_mhartid].value);

				if (store_var->buffer[i].status == store_inactive) {
					store_var->buffer[i].status = store_inactive;
					store_var->alloc[i].status = store_inactive;
					if (debug_unit || debug_bus) {
						fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, alloc xaction id %#06x, alloc dropped on branch missprediction addr: 0x%016I64x, write xaction id %#06x, clock %#04llx\n",
							mhartid, store_var->buffer[i].ROB_ptr, bus2_data_in->xaction_id, logical_addr->addr, logical_addr->xaction_id, clock);
					}
				}
				else if (physical_data_in->snoop_response != snoop_stall && logical_addr->strobe == 0 && (retire_num == store_var->buffer[i].ROB_ptr || store_var->buffer[i].status == store_TLB_and_retire_valid)) {
					store_var->buffer[i].status = store_w_addr_valid;
					store_var->alloc[i].status = store_inactive;
					logical_addr->strobe = 1;
					logical_addr->addr = store_var->buffer[i].addr;
					logical_addr->xaction = bus_store_full;
					csr[csr_store_issued].value++;
					switch (priviledge) {
					case 0:
						break;
					case 1:
						csr[csr_sstore_issued].value++;
						break;
					case 2:
						csr[csr_hstore_issued].value++;
						break;
					case 3:
					case 0x0f:
						csr[csr_mstore_issued].value++;
						break;
					default:
						debug++;
						break;
					}
					logical_addr->xaction_id = ((store_var->buffer[i].index << 14) | (i << 12) | (3 << 8) | (1 << 7) | csr[csr_mhartid].value);
					exec_rsp2[0] = (0x8000 | store_var->buffer[i].ROB_ptr);
					if (debug_unit || debug_bus) {
						UINT64* data_out = store_var->buffer[i].data_out;
						fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, alloc xaction id 0x%04x, alloc write merged sWC(%d) addr ",
							mhartid, store_var->buffer[i].ROB_ptr, bus2_data_in->xaction_id, i);
						fprint_addr_coma(debug_stream, logical_addr->addr, param);
						fprintf(debug_stream, " write xaction id %#06x, clock %#04llx\n", logical_addr->xaction_id, clock);
					}
				}
				else {
					store_var->buffer[i].ROB_ptr = store_var->alloc[i].ROB_ptr;
					store_var->buffer[i].xaction = bus_store_full;
					if (retire_num == store_var->buffer[i].ROB_ptr || store_var->buffer[i].status == store_TLB_and_retire_valid || store_var->buffer[i].status == store_flush) {
						store_var->buffer[i].status = store_branch_match;
						exec_rsp2[0] = (0x8000 | store_var->buffer[i].ROB_ptr);
						if (debug_unit || debug_bus) {
							fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, alloc xaction id 0x%04x, alloc write merged sWC(%d) delayed write branch match addr ",
								mhartid, store_var->buffer[i].ROB_ptr, bus2_data_in->xaction_id, i);
							fprint_addr_coma(debug_stream, store_var->buffer[i].addr, param);
							fprintf(debug_stream, " write xaction id %#06x, clock %#04llx\n", store_var->buffer[i].xaction_id, clock);
						}
					}
					else {
						store_var->buffer[i].status = store_data_merged;
						if (debug_unit || debug_bus) {
							fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, alloc xaction id 0x%04x, alloc write merged sWC(%d) delayed write TLB valid addr ",
								mhartid, store_var->buffer[i].ROB_ptr, bus2_data_in->xaction_id, i);
							fprint_addr_coma(debug_stream, store_var->buffer[i].addr, param);
							fprintf(debug_stream, " write xaction id %#06x, clock %#04llx\n", store_var->buffer[i].xaction_id, clock);
						}
					}
				}
				store_var->alloc[i].status = store_inactive;
				write_poster_data(&store_var->buffer[i], clock, mhartid, debug_unit, debug_stream);
			}
		}
	}

}
UINT8 delayed_alloc_addr_out(addr_bus_type* logical_addr, UINT8* count, data_bus_type* bus2_data_in, data_bus_type* logical_data_in, Store_type* store_var,UINT8 priviledge, UINT64 clock, int debug_unit, int debug_bus, csr_type* csr, FILE* debug_stream) {
	UINT8 stop = 0;
	UINT debug = 0;
	if (store_var->alloc[store_var->alloc_ptr].status == store_inactive && store_var->fault == 0) {
		for (UINT i = 0; i < 4; i++) {
			if (store_var->buffer[i].status == store_wait_TLB_issue) {
				if (logical_addr->strobe == 0 && bus2_data_in->snoop_response != snoop_stall && store_var->buffer[store_var->alloc_ptr].status == store_inactive && logical_data_in->snoop_response != snoop_stall) {
					logical_addr->strobe = 1;
					logical_addr->xaction = bus_allocate; // error, need to differentiate between demand and speculative execution
					csr[csr_alloc_issued].value++;
					switch (priviledge) {
					case 0:
						break;
					case 1:
						csr[csr_salloc_issued].value++;
						break;
					case 2:
						csr[csr_halloc_issued].value++;
						break;
					case 3:
					case 0x0f:
						csr[csr_malloc_issued].value++;
						break;
					default:
						debug++;
						break;
					}
					store_var->alloc[store_var->alloc_ptr].xaction = bus_allocate;
					logical_addr->addr = store_var->buffer[i].addr;
					store_var->alloc[store_var->alloc_ptr].ROB_ptr = store_var->buffer[i].ROB_ptr;

					logical_addr->xaction_id = ((store_var->alloc_ptr << 12) | (2 << 8) | (2 << 6) | csr[csr_mhartid].value);
					store_var->alloc[store_var->alloc_ptr].xaction_id = logical_addr->xaction_id;
					store_var->alloc[store_var->alloc_ptr].clock = clock;
					stop = 1;// only need to stop on address issued on bus
					count += 8;
					if (debug_unit || debug_bus) {
						fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, ALLOCATE issued_0;",
							csr[csr_mhartid].value, store_var->buffer[i].ROB_ptr, logical_addr->xaction_id);
						fprintf(debug_stream, " addr: 0x%016I64x // sWC(%d),", logical_addr->addr, i);
					}
					store_var->alloc[store_var->alloc_ptr].status = store_wait_TLB;
					store_var->alloc_ptr = ((store_var->alloc_ptr + 1) & 0x03);
				}
			}
		}
	}
	return stop;
}

UINT8 tlb_response_unit(UINT16* exec_rsp2, reg_bus* rd, data_bus_type* logical_data_out, data_bus_type* tlb_response, addr_bus_type* logical_addr, data_bus_type* bus2_data_in, Store_type* store_var, UINT16 retire_num, UINT8 block_loads, UINT64 clock, int debug_unit, int debug_bus, int debug_unit_walk, UINT mhartid, param_type* param, FILE* debug_stream) {
	int debug = 0;
	UINT8 stop = 0;
	if (tlb_response->snoop_response == snoop_miss) {
		UINT8 i = (tlb_response->xaction_id >> 12) & 3;
		if (((tlb_response->xaction_id >> 8) & 3) == 3) {
			store_var->buffer[i].cacheable = tlb_response->cacheable;
			if (store_var->buffer[i].status == store_inactive) {
				fprintf(debug_stream, "STORE(%lld): xaction id %#06x, ERROR accessed buffer is inactive (not valid) addr: ",
					mhartid, tlb_response->xaction_id);
				fprint_addr_coma(debug_stream, logical_addr->addr, param);
				fprintf(debug_stream, " clock: 0x%04llx\n", clock);
			}
		}
		else if (((tlb_response->xaction_id >> 8) & 3) == 2) {
			if (store_var->alloc[i].status == store_inactive) {
				fprintf(debug_stream, "STORE(%lld): xaction id %#06x, ERROR accessed buffer is inactive (not valid) addr: ",
					mhartid, tlb_response->xaction_id);
				fprint_addr_coma(debug_stream, logical_addr->addr, param);
				fprintf(debug_stream, " clock: 0x%04llx\n", clock);
			}
		}
		else {
			debug++;
		}
		if (tlb_response->xaction_id == store_var->alloc[i].xaction_id) {
			store_var->alloc[i].status = store_inactive;
			store_var->fault_addr = logical_addr->addr;
			store_var->fault_ROB_ptr = store_var->alloc[i].ROB_ptr;
			// need to block decoder
			if (store_var->alloc[i].ROB_ptr == retire_num) {
				if (exec_rsp2[0] != 0)
					debug++;
				exec_rsp2[0] = 0x4000 | store_var->alloc[i].ROB_ptr;
				rd->data = logical_addr->addr;
				store_var->fault = 2;
				if (debug_unit_walk || debug_bus) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, retire 0x%02x,  xaction id %#06x, Page Fault0 loading addr: ",
						mhartid, store_var->alloc[i].ROB_ptr, retire_num, tlb_response->xaction_id);
					fprint_addr_coma(debug_stream, logical_addr->addr, param);
					fprintf(debug_stream, " clock: 0x%04llx\n", clock);
				}
			}
			else {
				store_var->fault = 1;
				if (debug_unit_walk || debug_bus) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, retire 0x%02x,  xaction id %#06x, Page Fault0 Waiting for Retire Queue, loading addr: ",
						mhartid, store_var->alloc[i].ROB_ptr, retire_num, tlb_response->xaction_id);
					fprint_addr_coma(debug_stream, logical_addr->addr, param);
					fprintf(debug_stream, " clock: 0x%04llx\n", clock);
				}
			}

			if (store_var->buffer[i].status == store_wait_TLB)
				store_var->buffer[i].status = store_inactive;
			else
				store_var->buffer[i].status = store_flush;
			for (UINT8 j = 0; j < 4; j++) {
				if (store_var->buffer[j].status == store_wait_TLB)
					store_var->buffer[j].status = store_inactive;
				else if (store_var->buffer[j].status != store_inactive && !(store_var->buffer[j].valid_out_l == 0 && store_var->buffer[j].valid_out_h == 0) && store_var->buffer[j].status != store_w2_addr_valid) {
					if (store_var->buffer[j].status != store_w_addr_valid && store_var->buffer[j].status != store_w1_addr_valid)
						store_var->buffer[j].status = store_flush;
					if (debug_unit || debug_bus) {
						fprintf(debug_stream, "STORE(%lld) ROB entry 0x%02x,xaction id %#06x,  FLUSH sWB's, LOCK incoming address: ",
							mhartid, store_var->buffer[j].ROB_ptr, store_var->buffer[j].xaction_id);
						fprint_addr_coma(debug_stream, store_var->buffer[j].addr, param);
						fprintf(debug_stream, " clock: 0x%04llx\n", clock);
					}
				}
			}
		}
		else if (tlb_response->xaction_id == store_var->buffer[i].xaction_id) {
			if (debug_unit_walk || debug_bus) {
				fprintf(debug_stream, "STORE(%lld): xaction id %#06x, Page Fault1 loading addr ",
					mhartid, tlb_response->xaction_id);
				fprint_addr_coma(debug_stream, logical_addr->addr, param);
				fprintf(debug_stream, " clock: 0x%04llx\n", clock);
			}
			store_var->buffer[i].status = store_inactive;
			store_var->buffer[i].cacheable = tlb_response->cacheable;
			store_var->fault = 1;
			store_var->fault_ROB_ptr = store_var->alloc[i].ROB_ptr;

			for (UINT8 j = 0; j < 4; j++) {
				if (store_var->buffer[j].status == store_wait_TLB)
					store_var->buffer[j].status = store_inactive;
				else if (store_var->buffer[j].status != store_inactive && !(store_var->buffer[j].valid_out_l == 0 && store_var->buffer[j].valid_out_h == 0) && store_var->buffer[j].status != store_w2_addr_valid) {
					if (store_var->buffer[j].status != store_w_addr_valid && store_var->buffer[j].status != store_w1_addr_valid)
						store_var->buffer[j].status = store_flush;
					if (debug_unit || debug_bus) {
						fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, FLUSH sWB(%d), LOCK incoming",
							mhartid, store_var->buffer[j].ROB_ptr, store_var->buffer[j].xaction_id, j);
						fprintf(debug_stream, " address: 0x%016I64x // clock: 0x%04llx\n", store_var->buffer[j].addr, clock);
					}
				}
			}
			stop = 1;
		}
		else {
			UINT8 hit = 0;
			for (UINT i = 0; i < 4 && !hit; i++) {
				if (tlb_response->xaction_id == store_var->buffer[i].xaction_id) {
					if (debug_unit_walk || debug_bus) {
						fprintf(debug_stream, "STORE(%lld): xaction id %#06x, Page Fault2 loading addr: ",
							mhartid, tlb_response->xaction_id);
						fprint_addr_coma(debug_stream, logical_addr->addr, param);
						fprintf(debug_stream, " clock: 0x%04llx\n", clock);
					}
					store_var->buffer[i].status = store_inactive;
					store_var->buffer[i].cacheable = tlb_response->cacheable;
					store_var->fault = 1;
					store_var->fault_ROB_ptr = store_var->alloc[i].ROB_ptr;

					for (UINT8 j = 0; j < 4; j++) {
						if (store_var->buffer[j].status == store_wait_TLB)
							store_var->buffer[j].status = store_inactive;
						else if (store_var->buffer[j].status != store_inactive && !(store_var->buffer[j].valid_out_l == 0 && store_var->buffer[j].valid_out_h == 0) && store_var->buffer[j].status != store_w2_addr_valid) {
							if (store_var->buffer[j].status != store_w_addr_valid && store_var->buffer[j].status != store_w1_addr_valid)
								store_var->buffer[j].status = store_flush;
							if (debug_unit || debug_bus) {
								fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x , xaction id %#06x, FLUSH sWB(%d), LOCK incoming",
									mhartid, store_var->buffer[j].ROB_ptr, store_var->buffer[j].xaction_id, j);
								fprintf(debug_stream, " address: 0x%016I64x // clock: 0x%04llx\n", store_var->buffer[j].addr, clock);
							}
						}
					}
					hit = 1;
				}
			}
			if (!hit) {
				debug++;
				fprintf(debug_stream, "STORE(%lld): xaction id %#06x, ERROR xaction ID missmatch (core xaction id = %#06x) addr: 0x%016I64x  clock: 0x%04llx\n",
					mhartid, tlb_response->xaction_id, store_var->buffer[i].xaction_id, logical_addr->addr, clock);
			}
		}
	}
	else if (tlb_response->snoop_response == snoop_hit) {
		UINT8 i = (tlb_response->xaction_id >> 12) & 3;
		switch (store_var->alloc[i].xaction) {
		case bus_SC:
		case bus_SC_aq:
		case bus_SC_aq_rl:
			if (tlb_response->xaction_id == store_var->buffer[i].xaction_id && logical_data_out->cacheable != page_IO) {
				logical_data_out->snoop_response = snoop_dirty; // snoop dirty means full cache line, otherwise snoop_hit for 1 chunk valid (look at valid bits)
				logical_data_out->xaction_id = tlb_response->xaction_id; // snoop dirty means full cache line, otherwise snoop_hit for 1 chunk valid (look at valid bits)
				logical_data_out->cacheable = tlb_response->cacheable;
				logical_data_out->valid = store_var->buffer[i].valid_out_l;
				for (UINT8 j = 0; j < 0x10; j++)  logical_data_out->data[j] = store_var->buffer[i].data_out[j];
				if (debug_unit || debug_bus) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, bus_write data valid: %#06x // sWC(%d), clock: 0x%04llx\n",
						mhartid, store_var->buffer[i].ROB_ptr, logical_data_out->xaction_id, logical_data_out->valid, i, clock);
				}
				if (tlb_response->cacheable != page_IO) {
					store_var->buffer[i].index = ((store_var->buffer[i].index + 1) & 0x03);
					store_var->buffer[i].status = store_inactive;
				}
			}
			break;
		case bus_allocate:
			if (tlb_response->xaction_id == store_var->alloc[i].xaction_id && store_var->alloc[i].status != store_inactive && store_var->fault == 0) {
				store_var->alloc[i].cacheable = tlb_response->cacheable;
				store_var->buffer[i].cacheable = tlb_response->cacheable;
				if (tlb_response->cacheable != page_non_cache) {
					if (store_var->buffer[i].ROB_ptr == retire_num) {
						exec_rsp2[0] = 0x8000 | store_var->buffer[i].ROB_ptr;
						store_var->buffer[i].status = store_TLB_and_retire_valid;
						store_var->alloc[i].status = store_TLB_and_retire_valid;
						if (debug_unit || debug_bus) {
							fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, ALLOCATE rcved TLB HIT demand valid: %#06x // sWC(%d), clock: 0x%04llx\n",
								mhartid, store_var->buffer[i].ROB_ptr, tlb_response->xaction_id, tlb_response->valid, i, clock);
						}
					}
					else if (block_loads) {
						exec_rsp2[0] = 0x8000 | store_var->buffer[i].ROB_ptr;
						store_var->buffer[i].status = store_inactive;
						store_var->alloc[i].status = store_inactive;
						if (debug_unit || debug_bus) {
							fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, ALLOCATE rcved TLB HIT speculative branch invalid, flush transaction: %#06x // sWC(%d), clock: 0x%04llx\n",
								mhartid, store_var->buffer[i].ROB_ptr, tlb_response->xaction_id, tlb_response->valid, i, clock);
						}
					}
					else {
						store_var->buffer[i].status = store_TLB_valid;
						store_var->alloc[i].status = store_TLB_valid;
						if (debug_unit || debug_bus) {
							fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, ALLOCATE rcved TLB HIT speculative valid: %#06x // sWC(%d), clock: 0x%04llx\n",
								mhartid, store_var->buffer[i].ROB_ptr, tlb_response->xaction_id, tlb_response->valid, i, clock);
						}
					}
					write_poster_data(&store_var->buffer[i], clock, mhartid, debug_unit, debug_stream);
				}
				else if (store_var->buffer[i].valid_out_l == 0xffffffffffffffff && store_var->buffer[i].valid_out_h == 0xffffffffffffffff && store_var->buffer[i].ROB_ptr == retire_num) {
					if (bus2_data_in->snoop_response != snoop_idle) {
						fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, ALLOCATE to UC space aborted valid: %#06x // sWC(%d), clock: 0x%04llx\n",
							mhartid, store_var->buffer[i].ROB_ptr, tlb_response->xaction_id, tlb_response->valid, i, clock);

						store_var->buffer[i].status = store_wait_TLB;
						store_var->buffer[i].xaction = bus_store_full; // error, need to differentiate between demand and speculative execution
						logical_addr->strobe = 1;
						logical_addr->xaction = bus_store_full; // error, need to differentiate between demand and speculative execution
						logical_addr->addr = store_var->buffer[i].addr;
						logical_addr->xaction_id = store_var->buffer[i].xaction_id | 0x0300;
						store_var->buffer[i].xaction_id = logical_addr->xaction_id;
						store_var->buffer[i].time = clock;
						stop = 1;// only need to stop on address issued on bus
						if (debug_unit || debug_bus) {
							fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, bus_write ",
								mhartid, store_var->buffer[i].ROB_ptr, logical_addr->xaction_id);
							fprintf(debug_stream, " address: 0x%016I64x // clock: 0x%04llx\n", logical_addr->addr, clock);
						}
						exec_rsp2[0] = 0x8000 | store_var->buffer[i].ROB_ptr;
					}
					else {
						debug++;
					}
				}
				else {
					debug++;
				}
			}
			break;
		case bus_store_full:
		case bus_store_partial:
			if ((tlb_response->xaction_id & (~0x0100)) == store_var->buffer[i].xaction_id) {
				logical_data_out->snoop_response = snoop_dirty; // snoop dirty means full cache line, otherwise snoop_hit for 1 chunk valid (look at valid bits)
				logical_data_out->xaction_id = tlb_response->xaction_id; // snoop dirty means full cache line, otherwise snoop_hit for 1 chunk valid (look at valid bits)
				logical_data_out->cacheable = tlb_response->cacheable;
				logical_data_out->valid = store_var->buffer[i].valid_out_l;
				for (UINT8 j = 0; j < 0x10; j++)  logical_data_out->data[j] = store_var->buffer[i].data_out[j];
				if (debug_unit || debug_bus) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, bus_write data valid: %#06x // sWC(%d), clock: 0x%04llx\n",
						mhartid, store_var->buffer[i].ROB_ptr, logical_data_out->xaction_id, logical_data_out->valid, i, clock);
				}
				store_var->buffer[i].index = ((store_var->buffer[i].index + 1) & 0x03);
				store_var->buffer[i].status = store_inactive;
			}
			break;
		case bus_LR_aq:
		case bus_LR_aq_rl:
			break;
		default:
			debug++;
			break;
		}
	}
	return stop;
}
void IO_lock_data_out(data_bus_type* logical_data_out, addr_bus_type* logical_addr, Store_type* store_var, data_bus_type* tlb_response, snoop_response_type bus2_snoop_response, UINT64 clock, int debug_unit, int debug_bus, int debug_unit_walk, UINT mhartid, param_type* param, FILE* debug_stream) {
	int debug = 0;
	for (UINT8 i = 0; i < 4; i++) {
		if (((store_var->buffer[i].xaction & 0xfc) == bus_SC) && store_var->buffer[i].status != store_inactive) {
			store_var->alloc_ptr = i;
			if (store_var->alloc[store_var->alloc_ptr].status == store_w_addr_valid && bus2_snoop_response != snoop_stall) {
				logical_data_out->snoop_response = snoop_dirty;
				logical_data_out->xaction_id = store_var->buffer[i].xaction_id;
				logical_data_out->cacheable = store_var->buffer[i].cacheable;
				logical_data_out->valid = 0x000000ff;// 64b
				logical_data_out->data[0] = store_var->buffer[i].data_out[0];
				store_var->alloc[store_var->alloc_ptr].status = store_alloc_addr_valid;
				store_var->alloc[store_var->alloc_ptr].ROB_ptr = store_var->buffer[i].ROB_ptr;
				store_var->alloc[store_var->alloc_ptr].xaction = logical_addr->xaction;
				store_var->alloc[store_var->alloc_ptr].xaction_id = logical_addr->xaction_id;

				if (debug_unit || debug_unit_walk || debug_bus) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, bus_SC_aq_rl (data out) data: 0x%016I64x, valid: %#06x // sWC(%d), clock: 0x%04llx\n",
						mhartid, store_var->buffer[i].ROB_ptr, logical_data_out->xaction_id, store_var->buffer[i].data_out[0], logical_data_out->valid, i, clock);
				}
			}
		}
		if (((store_var->buffer[i].xaction & 0xfc) == bus_SC) && store_var->buffer[i].status != store_inactive) {
			store_var->alloc_ptr = i;
			if (store_var->alloc[store_var->alloc_ptr].status == store_w1_addr_valid) {
				store_var->alloc[store_var->alloc_ptr].status = store_w_addr_valid;
				if (debug_unit || debug_unit_walk || debug_bus) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, bus_SC_aq_rl (data wait 2) data: 0x%016I64x, valid: %#06x // sWC(%d), clock: 0x%04llx\n",
						mhartid, store_var->buffer[i].ROB_ptr, store_var->buffer[i].xaction_id, store_var->buffer[i].data_out[0], logical_data_out->valid, i, clock);
				}
			}
			else if (store_var->alloc[store_var->alloc_ptr].status == store_w2_addr_valid) {
				store_var->alloc[store_var->alloc_ptr].status = store_w1_addr_valid;
				if (debug_unit || debug_unit_walk || debug_bus) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, bus_SC_aq_rl (data wait 1) data: 0x%016I64x, valid: %#06x // sWC(%d), clock: 0x%04llx\n",
						mhartid, store_var->buffer[i].ROB_ptr, store_var->buffer[i].xaction_id, store_var->buffer[i].data_out[0], logical_data_out->valid, i, clock);
				}
			}
		}
	}
	if (tlb_response->snoop_response == snoop_hit) {
		UINT i = (tlb_response->xaction_id >> 12) & 0x03;
		if (((store_var->buffer[i].xaction & 0xfc) == bus_SC) && store_var->buffer[i].status != store_inactive) {
			store_var->alloc_ptr = i;
			store_var->buffer[i].cacheable = tlb_response->cacheable;
			if (store_var->alloc[store_var->alloc_ptr].status == store_inactive) {
				store_var->alloc[store_var->alloc_ptr].status = store_w2_addr_valid;
				store_var->alloc[store_var->alloc_ptr].ROB_ptr = store_var->buffer[i].ROB_ptr;
				store_var->alloc[store_var->alloc_ptr].xaction = logical_addr->xaction;
				store_var->alloc[store_var->alloc_ptr].xaction_id = logical_addr->xaction_id;

				if (debug_unit || debug_unit_walk || debug_bus) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, bus_SC_aq_rl (data wait 0) addr,data: ",
						mhartid, store_var->buffer[i].ROB_ptr, logical_addr->xaction_id);
					fprint_addr_coma(debug_stream, store_var->buffer[i].addr,param);
					fprintf(debug_stream, "0x%016I64x, valid: %#06x // sWC(%d), clock: 0x%04llx\n", store_var->buffer[i].data_out[0], store_var->buffer[i].valid_out_l, i, clock);
				}
			}
		}
	}
}
void update_write_posters(store_buffer_type* buffer, R3_type* store_exec, UINT64 clock, UINT8 current, UINT mhartid, int debug_unit, param_type* param, FILE* debug_stream) {
	int debug = 0;
	INT64 addr = store_exec->rs1 + store_exec->rs3;
	UINT16 index = addr & 0x7f;// 8*16
	switch (store_exec->size) {
	case 12:
		if ((addr & 0x0f) != 0)
			debug++;
		buffer->data_out[((addr >> 3) + 1) & 0x0f] = store_exec->rs2_h;
		buffer->data_out[(addr >> 3) & 0x0f] = store_exec->rs2;
		if (index < 64)
			buffer->valid_out_l |= ((UINT64)0x0000ffff << index);//128b - error: wraps around, doesn't roll off.
		else
			buffer->valid_out_h |= ((UINT64)0x0000ffff << (index - 64));//128b
		break;
	case 4: // 128b - error, 128b registers not modeled, not a primative in x86
		buffer->data_out[(addr >> 3) & 0x0f] = store_exec->rs2;
		if (index < 64)
			buffer->valid_out_l |= ((UINT64)0x0000ffff << index);//128b - error: wraps around, doesn't roll off.
		else
			buffer->valid_out_h |= ((UINT64)0x0000ffff << (index - 64));//128b
	case 3: // 64b
		buffer->data_out[(addr >> 3) & 0x0f] = store_exec->rs2;
		if (index < 64)
			buffer->valid_out_l |= ((UINT64)0x00ff << index);//128b - error: wraps around, doesn't roll off.
		else
			buffer->valid_out_h |= ((UINT64)0x00ff << (index - 64));//128b
		break;
	case 2: // 32b
		if (store_exec->uop == uop_F_STORE) {
			buffer->data_out[(addr >> 3) & 0x0f] &= ~0x00000000ffffffff;
			buffer->data_out[(addr >> 3) & 0x0f] |= store_exec->rs2 & 0x00000000ffffffff;
		}
		else
			buffer->data_out[(addr >> 3) & 0x0f] = store_exec->rs2;
		if (index < 64)
			buffer->valid_out_l |= ((UINT64)0x000f << index);//128b - error: wraps around, doesn't roll off.
		else
			buffer->valid_out_h |= ((UINT64)0x000f << (index - 64));//128b
		break;
	case 1: // 16b
		if (store_exec->uop == uop_F_STORE) {
			buffer->data_out[(addr >> 3) & 0x0f] &= ~0x0000ffff;
			buffer->data_out[(addr >> 3) & 0x0f] |= store_exec->rs2 & 0x0000ffff;
		}
		else
			buffer->data_out[(addr >> 3) & 0x0f] = store_exec->rs2;
		if (index < 64)
			buffer->valid_out_l |= ((UINT64)0x0003 << index);//128b - error: wraps around, doesn't roll off.
		else
			buffer->valid_out_h |= ((UINT64)0x0003 << (index - 64));//128b
		break;
	case 0: // 8b
		if (store_exec->uop == uop_F_STORE) {
			buffer->data_out[(addr >> 3) & 0x0f] &= ~0x000000ff;
			buffer->data_out[(addr >> 3) & 0x0f] |= store_exec->rs2 & 0x000000ff;
		}
		else
			buffer->data_out[(addr >> 3) & 0x0f] = store_exec->rs2;
		if (index < 64)
			buffer->valid_out_l |= ((UINT64)0x0001 << index);//128b - error: wraps around, doesn't roll off.
		else
			buffer->valid_out_h |= ((UINT64)0x0001 << (index - 64));//128b
		break;
	default:
		debug++;
		break;
	}
	if (debug_unit) {
		fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x sWC(%d) HIT buffer addr ", mhartid, store_exec->ROB_id, current);
		fprint_addr_coma(debug_stream, buffer->addr & ((UINT64)~0x003f),param);
		fprintf(debug_stream, "block(0x%02x) STORE // value: 0x%016I64x, store addr ", addr & 0x78, store_exec->rs2);
		fprint_addr_coma(debug_stream, addr, param);
		fprintf(debug_stream, "xaction id 0x%04x clock: 0x%04llx\n", buffer->xaction_id, clock);
	}
	write_poster_data(buffer, clock, mhartid, debug_unit, debug_stream);
}
// need float store unit
void store_amo_unit(reg_bus* rd, UINT16* exec_rsp, UINT16* exec_rsp2, R3_type* store_exec, UINT8 block_loads, addr_bus_type* logical_addr, 
	data_bus_type* logical_data_out, data_bus_type* tlb_response, data_bus_type* physical_data2_in, data_bus_type* physical_data3_in, 
	bus_w_snoop_signal1* bus2, UINT8 fault_release, UINT16 retire_num, UINT8 branch_clear, UINT8 *active_IO, UINT8 prefetcher_busy, UINT8 priviledge,
	UINT64 clock, csr_type* csr, Store_type* store_var, UINT debug_core, param_type *param, FILE* debug_stream) {// output, input, clock, resources
	UINT debug = 0;
	UINT mhartid = csr[csr_mhartid].value;
	int debug_unit = (param->store == 1 || param->store_bus == 1) && debug_core;
	int debug_bus = param->store_bus == 1 && debug_core;
	int debug_unit_walk = (param->store_bus == 1  || param->PAGE_WALK) && debug_core;
	int debug_unit_fault = (param->store_bus == 1 || param->PAGE_WALK || param->PAGE_FAULT) && debug_core;

	UINT64 mask[0x10] = { 0x000000000000000f,0x00000000000000f0,0x0000000000000f00,0x000000000000f000,0x00000000000f0000,0x0000000000f00000,0x000000000f000000,0x00000000f0000000,
	0x0000000f00000000,0x000000f000000000,0x00000f0000000000,0x0000f00000000000,0x000f000000000000,0x00f0000000000000,0x0f00000000000000,0xf000000000000000 };

	addr_bus_type* bus2_addr_in = &bus2->snoop_addr.in;
	data_bus_type* bus2_data_in = &bus2->data_read.in;
	logical_data_out->snoop_response = snoop_idle;

	rd->strobe = 0;
	exec_rsp[0] = 0;
	exec_rsp2[0] = 0;

	if (mhartid == 0) {
		if (clock >= 0x3cf5) // need to latch snoop addr and track, not latch data  due to IO reg writes to sTLB dL0
			debug++;
		if (clock >= 0x0236) // need to latch snoop addr and track, not latch data  due to IO reg writes to sTLB dL0
			debug++;
	}
	for (UINT8 current = 0; current < 4; current++) {
		if (store_var->buffer[current].status == store_retire)
			store_var->buffer[current].status = store_inactive;
		if (block_loads) {
			if (store_var->buffer[current].status == store_data_merged)
				store_var->buffer[current].status = store_inactive;
		}
	}
	int stop = 0;
	UINT8 count = 0;
	if (fault_release) store_var->fault = 0;
	else if(store_var->fault == 1 && retire_num == store_var->fault_ROB_ptr) {
		if (exec_rsp2[0] != 0)
			debug++;
		exec_rsp2[0] = 0x4000 | store_var->fault_ROB_ptr;
		rd->data = store_var->fault_addr;
		store_var->fault = 2;
	}

	// error: 
	//		ROB entry free at allocate past TLB
	//		physical write needs to wait to occur on same branch
	//		retire before physical write occur allows for branch to progress ahead of write - causing branch missmatch
	//	solutions:
	//		1)delay ROB retire from alloc TLB hit to physical write
	//		2)
	//			a) new phase: detect branch synch after TLB hit
	//			b) check branch synch flag before issuing physical write
	//			? how well do reads match up with writes (no poster snooping)  - handle with compiler
	//
	for (UINT i = 0; i < 4; i++) {
		if (branch_clear && store_var->buffer[i].status != store_inactive && store_var->buffer[i].status != store_TLB_and_retire_valid &&
			store_var->buffer[i].status != store_w2_addr_valid && store_var->buffer[i].status != store_w1_addr_valid && store_var->buffer[i].status != store_w_addr_valid) {
			if (store_var->buffer[i].status != store_data_merged)
				debug++;
			store_var->buffer[i].status = store_inactive;
		}
		if (store_var->buffer[i].status == store_data_merged && retire_num == store_var->buffer[i].ROB_ptr) {
			store_var->buffer[i].status = store_branch_match;
			exec_rsp2[0] = 0x8000 | store_var->buffer[i].ROB_ptr;
		}
		if (store_var->buffer[i].status == store_TLB_valid && retire_num == store_var->buffer[i].ROB_ptr) {
			store_var->buffer[i].status = store_alloc_addr_valid;
		}
		if ((physical_data3_in->snoop_response != snoop_stall) && (physical_data3_in->snoop_response != snoop_idle) && ((physical_data3_in->xaction_id & 0x0f) != mhartid) && (clock > 0x100))
			debug++;
		if (store_var->buffer[i].status == store_branch_match && store_var->buffer[i].xaction != bus_SC_aq_rl && logical_addr->strobe == 0 && physical_data3_in->snoop_response != snoop_stall) {
			store_var->buffer[i].status = store_w_addr_valid;
			logical_addr->strobe = 1;
			logical_addr->addr = store_var->buffer[i].addr;
			logical_addr->cacheable = store_var->buffer[i].cacheable;
			store_var->buffer[i].xaction = logical_addr->xaction = bus_store_full;
			csr[csr_store_issued].value++;
			switch (priviledge) {
			case 0:
				break;
			case 1:
				csr[csr_sstore_issued].value++;
				break;
			case 2:
				csr[csr_hstore_issued].value++;
				break;
			case 3:
			case 0x0f:
				csr[csr_mstore_issued].value++;
				break;
			default:
				debug++;
				break;
			}

			store_var->buffer[i].xaction_id |= 0x0300;
			logical_addr->xaction_id = store_var->buffer[i].xaction_id;
			if (debug_unit || debug_bus) {
				fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x alloc writeback sWC(%d) issued addr ", mhartid, store_var->buffer[i].ROB_ptr, i);
				fprint_addr_coma(debug_stream, logical_addr->addr,param );
				fprintf(debug_stream, " write xaction id %#06x, clock %#04llx\n", logical_addr->xaction_id, clock);
			}
		}
	}
	write_data_out(exec_rsp2, logical_data_out, store_var, clock, debug_unit, debug_bus, csr[csr_mhartid].value,param,  debug_stream);
	data_from_l0(rd, exec_rsp2, logical_addr, physical_data2_in, store_var, retire_num, block_loads,priviledge, clock, csr, debug_unit, debug_bus, csr[csr_mhartid].value, param, debug_stream);
	data_from_l2(rd, exec_rsp2, logical_addr, bus2_data_in, physical_data3_in, store_var, retire_num, priviledge, clock, debug_unit, debug_bus, debug_unit_walk, csr, mhartid,param, debug_stream);
	stop |= delayed_alloc_addr_out(logical_addr, &count, bus2_data_in, tlb_response, store_var, priviledge, clock, debug_unit, debug_bus, csr, debug_stream);
	stop |= tlb_response_unit(exec_rsp2, rd, logical_data_out, tlb_response, logical_addr, bus2_data_in, store_var, retire_num, block_loads, clock, debug_unit, debug_bus, debug_unit_walk, mhartid, param, debug_stream);
	IO_lock_data_out(logical_data_out, logical_addr, store_var, tlb_response, bus2->data_read.in.snoop_response, clock, debug_unit, debug_bus, debug_unit_walk, csr[csr_mhartid].value, param, debug_stream);
	if (bus2_addr_in->strobe && ((bus2_addr_in->xaction & 0xfc) == bus_SC)) {
		copy_addr_bus_info(&store_var->IO_track, bus2_addr_in, clock);
		if (debug_unit || debug_bus || debug_unit_walk) {
			fprintf(debug_stream, "STORE(%lld) xaction id %#06x, addr: 0x%016I64x, IO write to dTLB or dL0 detected (addr) clock %#04llx\n",
				mhartid, bus2_addr_in->xaction_id, bus2_addr_in->addr, clock);
		}
	}
	// addr portion
	UINT reg_busy = 0;
	for (UINT i = 0; i < 4; i++)
		if (store_var->buffer[0].status != store_inactive)
			reg_busy++;

	if (mhartid == 0) {
		if (clock >= 0x1f5b)// x_id 1280
			debug++;
	}
	UINT8 hit = 0;
	if (store_exec->strobe && !stop) {
		if (mhartid == 0)
			if (clock >= 0x0200)
				debug++;
		switch (store_exec->uop) {
		case uop_LR:// locked read
			if (bus2_data_in->snoop_response != snoop_stall && tlb_response->snoop_response != snoop_stall &&
				store_var->lock == 0 && retire_num == store_exec->ROB_id &&
				store_var->buffer[0].status == store_inactive && store_var->buffer[1].status == store_inactive && store_var->buffer[2].status == store_inactive && store_var->buffer[3].status == store_inactive &&
				store_var->alloc[0].status == store_inactive && store_var->alloc[1].status == store_inactive && store_var->alloc[2].status == store_inactive && store_var->alloc[3].status == store_inactive) {
				store_var->alloc[store_var->alloc_ptr].status = store_alloc_addr_valid;
				store_var->alloc[store_var->alloc_ptr].ROB_ptr = rd->ROB_id;
				logical_addr->strobe = 1;
				exec_rsp[0] = 0x8000 | store_exec->ROB_id;

				csr[csr_amo_issued].value++;
				switch (priviledge) {
				case 0:
					break;
				case 1:
					csr[csr_samo_issued].value++;
					break;
				case 2:
					csr[csr_hamo_issued].value++;
					break;
				case 3:
				case 0x0f:
					csr[csr_mamo_issued].value++;
					break;
				default:
					debug++;
					break;
				}
				logical_addr->xaction = (bus_xaction_type)(bus_LR | (store_exec->rs3 & 3)); // error, need to differentiate between demand and speculative execution
				store_var->alloc[store_var->alloc_ptr].xaction = logical_addr->xaction;
				logical_addr->addr = store_exec->rs1 & 0xfffffffe;
				logical_addr->xaction_id = ((store_var->alloc_ptr << 12) | (2 << 8) | (2 << 6) | mhartid);
				store_var->alloc[store_var->alloc_ptr].xaction_id = logical_addr->xaction_id;
				store_var->alloc[store_var->alloc_ptr].ROB_ptr = store_exec->ROB_id;
				store_var->alloc[store_var->alloc_ptr].addr = logical_addr->addr;
				count += 8;
				if (debug_unit || debug_bus || debug_unit_walk) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, xaction id %#06x, ", mhartid, store_exec->ROB_id, logical_addr->xaction_id);
					fprintf(debug_stream, "bus_LR_aq_rl address: 0x%016I64x, // clock: 0x%04llx\n", logical_addr->addr, clock);
				}
				store_var->alloc_ptr = ((store_var->alloc_ptr + 1) & 3);
			}
			break;
		case uop_SC: {
			active_IO[0] = 1;
			if (bus2_data_in->snoop_response != snoop_stall && tlb_response->snoop_response != snoop_stall &&  prefetcher_busy == 0 &&
				store_var->lock == 0 && retire_num == store_exec->ROB_id &&
				store_var->buffer[0].status == store_inactive && store_var->buffer[1].status == store_inactive && store_var->buffer[2].status == store_inactive && store_var->buffer[3].status == store_inactive &&
				store_var->alloc[0].status == store_inactive && store_var->alloc[1].status == store_inactive && store_var->alloc[2].status == store_inactive && store_var->alloc[3].status == store_inactive) {

				UINT8 current = store_var->alloc_ptr;
				store_var->buffer[current].ROB_ptr = rd->ROB_id;

				logical_addr->strobe = 1;
				exec_rsp[0] = 0x8000 | store_exec->ROB_id;

				csr[csr_amo_issued].value++;
				switch (priviledge) {
				case 0:
					break;
				case 1:
					csr[csr_samo_issued].value++;
					break;
				case 2:
					csr[csr_hamo_issued].value++;
					break;
				case 3:
				case 0x0f:
					csr[csr_mamo_issued].value++;
					break;
				default:
					debug++;
					break;
				}
				logical_addr->xaction = store_var->buffer[current].xaction = (bus_xaction_type)(bus_SC | (store_exec->rs3 & 3)); // when data is issued, read to status register is issued (addr+8)
				logical_addr->addr = store_exec->rs1;
				store_var->buffer[current].ROB_ptr = store_exec->ROB_id;
				store_var->buffer[current].addr = logical_addr->addr;
				logical_addr->xaction_id = ((store_var->buffer[current].index << 14) | (current << 12) | (3 << 8) | (2 << 6) | mhartid);
				store_var->buffer[current].xaction_id = ((store_var->buffer[current].index << 14) | (current << 12) | (3 << 8) | (2 << 6) | mhartid);
				store_var->buffer[current].cacheable = logical_addr->cacheable;

				store_var->buffer[current].data_out[0] = store_exec->rs2;
				store_var->buffer[current].valid_out_l = 0x00ff; //64b register
				store_var->buffer[current].status = store_wait_TLB;
				store_var->buffer[current].amo = 1;
				store_var->buffer[current].time = clock;
				store_var->lock = 1;

				count += 8;
				if (debug_unit || debug_bus || debug_unit_walk) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, bus_SC_aq_rl sWC(%d) addr ", mhartid, store_exec->ROB_id, current);
					fprint_addr_coma(debug_stream, logical_addr->addr, param);
					fprintf(debug_stream, " xaction id %#06x, clock: 0x%04llx\n", logical_addr->xaction_id, clock);
				}
			}
		}
				   break;
		case uop_FENCE: {
			if (store_var->buffer[0].status == store_inactive && store_var->buffer[1].status == store_inactive && store_var->buffer[2].status == store_inactive && store_var->buffer[3].status == store_inactive) {
				exec_rsp[0] = 0x2000 | store_exec->ROB_id;

				if (debug_unit) {
					fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, FENCE , clock: 0x%04llx\n",
						mhartid, store_exec->ROB_id, clock);
				}
			}
		}
					  break;
		case uop_STORE:
		case uop_F_STORE:
		{
			UINT64 addr = store_exec->rs1 + store_exec->rs3; // 1 clock delay
			if (mhartid == 1) {
				if (clock >= 0x2048)
					debug++;
			}

			UINT8 current = 0;
			for (; current < 4 && !hit && !stop; current++) { // only one adder in write unit, can only have 1 write
				if ((store_var->buffer[current].addr & 0xffffffffffffff80) == (addr & 0xffffffffffffff80) && store_var->buffer[current].status != store_inactive) {
					if (store_var->buffer[current].status == store_retire) {
						stop = 1;
					}
					else if (store_var->buffer[current].status == store_wait_TLB || store_var->buffer[current].status == store_wait_TLB_issue || 
						store_var->buffer[current].status == store_TLB_valid || store_var->buffer[current].status == store_alloc_addr_valid ||
						store_var->buffer[current].status == store_w_addr_valid || store_var->buffer[current].status == store_w1_addr_valid ||
						store_var->buffer[current].status == store_w2_addr_valid || store_var->buffer[current].status == store_data_merged ||
						store_var->buffer[current].status == store_TLB_and_retire_valid || store_var->buffer[current].status == store_branch_match) {
						hit = 1;
						exec_rsp[0] = 0x2000 | store_exec->ROB_id;

						if (store_var->buffer[current].status == store_w2_addr_valid)
							store_var->buffer[current].status = store_w_addr_valid;// delay one clock to allow write for future write buffer hits, max 1 hit per clock
						update_write_posters(&store_var->buffer[current], store_exec, clock, current, mhartid, debug_unit, param, debug_stream);
					}
					else {
						debug++;
					}
				}
			}
			if ((store_var->alloc[0].status == store_inactive && store_var->buffer[0].status == store_inactive) ||
				(store_var->alloc[1].status == store_inactive && store_var->buffer[1].status == store_inactive) ||
				(store_var->alloc[2].status == store_inactive && store_var->buffer[2].status == store_inactive) ||
				(store_var->alloc[3].status == store_inactive && store_var->buffer[3].status == store_inactive)) {

				if (store_var->alloc[store_var->alloc_ptr].status != store_inactive || store_var->buffer[store_var->alloc_ptr].status != store_inactive)
					store_var->alloc_ptr = ((store_var->alloc_ptr + 1) & 3);
				if (store_var->alloc[store_var->alloc_ptr].status != store_inactive || store_var->buffer[store_var->alloc_ptr].status != store_inactive)
					store_var->alloc_ptr = ((store_var->alloc_ptr + 1) & 3);
				if (store_var->alloc[store_var->alloc_ptr].status != store_inactive || store_var->buffer[store_var->alloc_ptr].status != store_inactive)
					store_var->alloc_ptr = ((store_var->alloc_ptr + 1) & 3);
			}
			if (!hit && !stop && store_var->alloc[store_var->alloc_ptr].status == store_inactive && store_var->fault == 0 ) {
				if (logical_addr->strobe == 0 && bus2_data_in->snoop_response != snoop_stall && store_var->buffer[store_var->alloc_ptr].status == store_inactive &&
					tlb_response->snoop_response != snoop_stall && physical_data2_in->snoop_response != snoop_stall) {// throttle back speculation to avoid lock-up
					if (clock == 0x21e6 && mhartid == 0)
						debug++;
					current = store_var->alloc_ptr;
					store_var->buffer[current].amo = 0;
					store_var->buffer[current].addr = addr & (~0x007f);
					store_var->buffer[current].xaction_id = ((store_var->buffer[current].index << 14) | (current << 12) | (2 << 8) | (2 << 6) | mhartid);
					store_var->buffer[current].valid_out_h = 0;
					store_var->buffer[current].valid_out_l = 0;
					store_var->buffer[current].ROB_ptr = store_exec->ROB_id;
					store_var->buffer[current].status = store_wait_TLB;
					store_var->buffer[current].time = clock;
					if (store_exec->uop != uop_STORE && store_exec->uop != uop_F_STORE)
						debug++;
					logical_addr->strobe = 1;
					exec_rsp[0] = 0x8000 | store_exec->ROB_id;

					logical_addr->xaction = bus_allocate; // error, need to differentiate between demand and speculative execution
					csr[csr_alloc_issued].value++;
					switch (priviledge) {
					case 0:
						break;
					case 1:
						csr[csr_salloc_issued].value++;
						break;
					case 2:
						csr[csr_halloc_issued].value++;
						break;
					case 3:
					case 0x0f:
						csr[csr_malloc_issued].value++;
						break;
					default:
						debug++;
						break;
					}
					store_var->buffer[current].xaction = bus_allocate; // error, need to differentiate between demand and speculative execution
					store_var->alloc[store_var->alloc_ptr].xaction = bus_allocate;
					for (UINT8 j = 0; j < 0x10; j++) store_var->buffer[current].data_out[j] = 0;
					logical_addr->addr = addr;
					store_var->alloc[store_var->alloc_ptr].ROB_ptr = store_exec->ROB_id;

					logical_addr->xaction_id = ((store_var->alloc_ptr << 12) | (2 << 8) | (2 << 6) | mhartid);
					store_var->alloc[store_var->alloc_ptr].xaction_id = logical_addr->xaction_id;
					store_var->buffer[current].xaction_id = logical_addr->xaction_id;
					store_var->alloc[store_var->alloc_ptr].clock = clock;
					//					store_var->buffer[current].branch_num = store_exec->rs3_h;
					stop = 1;// only need to stop on address issued on bus
					count += 8;
					if (logical_addr->cacheable == page_non_cache) {// cache swap, we need to wait for snoop response
						if (debug_unit || debug_bus) {
							fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x, ALLOCATE issued_1a sWC(%d);",
								mhartid, store_exec->ROB_id, logical_addr->xaction_id, current);
							fprintf(debug_stream, " l_addr/data: 0x%016I64x / 0x%016I64x, STORE // ", logical_addr->addr, store_exec->rs2);
							fprintf(debug_stream, "  clock: 0x%04llx\n", clock);
						}
						store_var->alloc[store_var->alloc_ptr].status = store_alloc_addr_valid;
						//						ROB_ptr->state = ROB_retire_out; // no TLB access needed because non-swapable
						exec_rsp2[0] = 0x8000 | store_var->buffer[current].ROB_ptr;
						store_var->buffer[current].status = store_TLB_and_retire_valid;
					}
					else if (logical_addr->cacheable == page_non_cache) {
						if (debug_unit || debug_bus) {
							fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x xaction id %#06x, ALLOCATE issued_1b;",
								mhartid, store_exec->ROB_id, logical_addr->xaction_id);
							fprintf(debug_stream, " l_addr/data: 0x%016I64x / 0x%016I64x // sWC(%d),", logical_addr->addr, store_exec->rs2, current);
							fprintf(debug_stream, "  clock: 0x%04llx\n", clock);
						}
						store_var->buffer[current].status = store_TLB_valid;
						store_var->buffer[current].xaction_id = logical_addr->xaction_id;
						store_var->buffer[current].time = clock;
						store_var->alloc[store_var->alloc_ptr].status = store_TLB_valid;
					}
					else {
						if (debug_unit || debug_bus) {
							fprintf(debug_stream, "STORE(%lld): ROB entry: 0x%02x xaction id %#06x, ALLOCATE issued_1c;",
								mhartid, store_exec->ROB_id, logical_addr->xaction_id);
							fprintf(debug_stream, " l_addr,data: ");
							fprint_addr_coma(debug_stream, logical_addr->addr,param);
							fprintf(debug_stream, " 0x%016I64x // sWC(%d),", store_exec->rs2, current);
							fprintf(debug_stream, " clock: 0x%04llx\n", clock);
						}
						store_var->buffer[current].status = store_wait_TLB;
						store_var->alloc[store_var->alloc_ptr].status = store_wait_TLB;
						store_var->buffer[current].xaction_id = logical_addr->xaction_id;
						store_var->buffer[current].time = clock;
					}
					hit = 1;
					store_var->buffer[current].ROB_ptr = store_exec->ROB_id;
					store_var->alloc_ptr = ((store_var->alloc_ptr + 1) & 0x03);
					store_var->alloc_ptr = ((store_var->alloc_ptr + 1) & 0x03);
					store_var->buffer[current].addr = addr;
					update_write_posters(&store_var->buffer[current], store_exec, clock, current, mhartid, debug_unit,param, debug_stream);
				}
			}
			else {
				if (!hit && !stop) {
					if (store_var->buffer[store_var->alloc_ptr].status == store_inactive) {
						debug++;
					}
				}
				else
					current--;
			}
		}
		break;
		default:
			debug++;
			break;
		}
	}
}
