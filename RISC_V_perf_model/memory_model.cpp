// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "memory_model.h"

#include "memory_abstract.h"

// need to move latencies to control registers
//	* allow to run performance tests in one run 
//		- yielding immediate results rather than post process
//		- same code would work on model and actual silicon for model validation
// 
// need at least 4 channels for DDR4, 8 channels (2 per DIMM column) for DDR5
//#define CAS_latency 13		// need pipelining - cas latency can be as high as 19; minimum pipelining of 5 active entries for back to back reads to same row
//#define RAS_latency 13
#define tRCD 13
//#define bank_latency 13
//				DDR4		DIMM (8 parts)
// CAS bits		A0-A9		A10-A12
//#define CAS_bits	14		// 10 bits per die, +3 bits per DIMM (assuming 8 die per DIM), +1 bit since by 16
//#define RAS_bits	28		// 14 bits + CAS count
// 4 bits x16 * 8 die per DIMM
// 10 bits for CAS
// 14 bits for RAS
// 2 bits for bank address
// 1 bit for bank group

void print_xaction3(DDR_status_type xaction, FILE* debug_stream) {
	switch (xaction) {
	case ddr_idle:
		fprintf(debug_stream, "ddr_idle");
		break;
	case ddr_read:
		fprintf(debug_stream, "READ");
		break;
	case ddr_read2:
		fprintf(debug_stream, "READ(2)");
		break;
	case ddr_write:
		fprintf(debug_stream, "WRITE");
		break;
	case ddr_allocate0:
	case ddr_allocate1:
		fprintf(debug_stream, "ALLOCATE");
		break;
	case ddr_allocate2:
		fprintf(debug_stream, "ALLOCATE(2)");
		break;
	case ddr_allocate_c:
		fprintf(debug_stream, "ALLOCATE cacheable");
		break;
	case ddr_allocate_c2:
		fprintf(debug_stream, "ALLOCATE(2) cacheable");
		break;
	default:
		fprintf(debug_stream, "unknown xaction");
		break;
	}
}
void memory_controller(addr_bus_type* mem_addr, data_bus_type* mem_data_read, data_bus_type* mem_data_write, UINT8 reset, UINT64 Bclock, DDR_control_type* DDR_ctrl, memory_space_type* memory_space, param_type *param, FILE* debug_stream) {
	int debug = 0;
	const long mask = 0x00fffff;

	UINT64 valid_mask[0x10] = { 0x000000000000000f,0x00000000000000f0,0x0000000000000f00,0x000000000000f000,0x00000000000f0000,0x0000000000f00000,0x000000000f000000,0x00000000f0000000,
0x0000000f00000000,0x000000f000000000,0x00000f0000000000,0x0000f00000000000,0x000f000000000000,0x00f0000000000000,0x0f00000000000000,0xf000000000000000 };

	UINT debug_unit = (param->caches || param->mem) && (2 * Bclock) >= param->start_time;

	DDR_ctrl->data_bus_ptr = ((DDR_ctrl->data_bus_ptr + 1) & 0x1f);
	while (DDR_ctrl->list[DDR_ctrl->list_start_ptr].status == ddr_idle && DDR_ctrl->list_start_ptr != DDR_ctrl->list_stop_ptr) DDR_ctrl->list_start_ptr = ((DDR_ctrl->list_start_ptr + 1) & (DDR_ctrl->list_count - 1));

	if (Bclock >= 0x12cd)
		debug++;

	if (!((DDR_ctrl->data_bus[1].valid == 0) || (DDR_ctrl->data_bus[1].valid == 1)))
		debug++;
	if (DDR_ctrl->data_bus[DDR_ctrl->data_bus_ptr].valid) {
		DDR_ctrl->data_valid_count++;
		UINT current_data = DDR_ctrl->data_bus[DDR_ctrl->data_bus_ptr].current_data;
		INT8 current_addr = DDR_ctrl->data_bus[DDR_ctrl->data_bus_ptr].list_ptr;
		if (DDR_ctrl->data_valid_count == 4) {
			if (DDR_ctrl->list[current_data].status == ddr_idle && DDR_ctrl->data_bus[DDR_ctrl->data_bus_ptr].write) {
				if (debug_unit) {
					fprintf(debug_stream, "DRAM bus: XFER: write cycle complete ");
					fprintf(debug_stream, "mWC(%d) ", DDR_ctrl->list[current_data].wc_ptr);
					fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
				}
			}
			else if (DDR_ctrl->list[current_data].status == ddr_allocate2) {
				if (debug_unit) {
					fprintf(debug_stream, "DRAM bus: XFER:");
					fprintf(debug_stream, "merge DRAM data with mWC(%d) valid: 0x%016I64x, xaction CAS issued:: %#06x, ", DDR_ctrl->list[current_data].wc_ptr, DDR_ctrl->wc[DDR_ctrl->list[current_data].wc_ptr].dirty, DDR_ctrl->list[current_data].xaction_id);
					fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
				}
				for (UINT8 i = 0; i < 0x10; i++)
					if (!(DDR_ctrl->wc[DDR_ctrl->list[current_data].wc_ptr].dirty & ((UINT64)3 << (i * 2))))
						DDR_ctrl->wc[DDR_ctrl->list[current_data].wc_ptr].data[i] = DDR_ctrl->list[current_data].data[i];
				DDR_ctrl->wc[DDR_ctrl->list[current_data].wc_ptr].dirty = 0xffffffff;
				DDR_ctrl->list[current_data].status = ddr_write;
			}
			else if (DDR_ctrl->list[current_data].status == ddr_allocate3) {
				if (debug_unit) {
					fprintf(debug_stream, "DRAM bus: XFER:");
					fprintf(debug_stream, "merge DRAM data with mWC(%d) valid: 0x%016I64x, xaction id %#06x, ", DDR_ctrl->list[current_data].wc_ptr, DDR_ctrl->wc[DDR_ctrl->list[current_data].wc_ptr].dirty, DDR_ctrl->list[current_data].xaction_id);
					fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
				}
				for (UINT8 i = 0; i < 0x10; i++)
					if (!(DDR_ctrl->wc[DDR_ctrl->list[current_data].wc_ptr].dirty & ((UINT64)3 << (i * 2))))
						DDR_ctrl->wc[DDR_ctrl->list[current_data].wc_ptr].data[i] = DDR_ctrl->list[current_data].data[i];
				DDR_ctrl->wc[DDR_ctrl->list[current_data].wc_ptr].dirty = 0xffffffff;
				DDR_ctrl->list[current_data].status = ddr_idle;
			}
			else {
				if (current_data != -1) {
					if (DDR_ctrl->list[current_data].xaction != bus_store_full && DDR_ctrl->list[current_data].xaction != bus_store_partial && DDR_ctrl->list[current_data].status != ddr_allocate3) {
						mem_data_read->snoop_response = snoop_hit;
						mem_data_read->xaction_id = DDR_ctrl->list[current_data].xaction_id;
						mem_data_read->cacheable = DDR_ctrl->list[current_data].cacheable;
						mem_data_read->valid = 0x0f;
						for (UINT8 i = 0; i < 0x10; i++) mem_data_read->data[i] = DDR_ctrl->list[current_data].data[i];
						if (debug_unit) {
							fprintf(debug_stream, "DRAM xaction id %#06x q_id: %#04x %#04x-%#04x, XFER: ", mem_data_read->xaction_id, current_data, DDR_ctrl->list_start_ptr, DDR_ctrl->list_stop_ptr);
							fprintf(debug_stream, "Memory Read return (2) Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
							fprintf(debug_stream, "DRAM xaction id %#06x 0x00 0x%016I64x 0x08 0x%016I64x 0x10 0x%016I64x 0x18 0x%016I64x \n",
								mem_data_read->xaction_id, mem_data_read->data[0], mem_data_read->data[1], mem_data_read->data[2], mem_data_read->data[3]);
							fprintf(debug_stream, "DRAM xaction id %#06x 0x20 0x%016I64x 0x28 0x%016I64x 0x30 0x%016I64x 0x38 0x%016I64x \n",
								mem_data_read->xaction_id, mem_data_read->data[4], mem_data_read->data[5], mem_data_read->data[6], mem_data_read->data[7]);
							fprintf(debug_stream, "DRAM xaction id %#06x 0x40 0x%016I64x 0x48 0x%016I64x 0x50 0x%016I64x 0x58 0x%016I64x\n",
								mem_data_read->xaction_id, mem_data_read->data[8], mem_data_read->data[9], mem_data_read->data[10], mem_data_read->data[11]);
							fprintf(debug_stream, "DRAM xaction id %#06x 0x60 0x%016I64x 0x68 0x%016I64x 0x70 0x%016I64x 0x78 0x%016I64x \n",
								mem_data_read->xaction_id, mem_data_read->data[12], mem_data_read->data[13], mem_data_read->data[14], mem_data_read->data[15]);
						}
					}
					DDR_ctrl->list[current_data].status = ddr_idle;
					DDR_ctrl->victim[DDR_ctrl->victim_ptr].tag = DDR_ctrl->data_bus[DDR_ctrl->data_bus_ptr].tag_addr;
					DDR_ctrl->victim[DDR_ctrl->victim_ptr].xaction_id = DDR_ctrl->list[current_data].xaction_id;
					for (UINT8 i = 0; i < 0x10; i++) DDR_ctrl->victim[DDR_ctrl->victim_ptr].data[i] = DDR_ctrl->list[current_data].data[i];
					DDR_ctrl->victim_ptr = ((DDR_ctrl->victim_ptr + 1) & 3);
				}
			}
			DDR_ctrl->data_valid_count = 0;
			if (DDR_ctrl->list[current_data].status != ddr_write)
				DDR_ctrl->list[current_data].status = ddr_idle;
		}
		DDR_ctrl->data_bus[DDR_ctrl->data_bus_ptr].valid = 0;
	}
	else {
		if (DDR_ctrl->data_valid_count != 0) {
			fprintf(debug_stream, "DRAM bus: ERROR: valid count ");
			fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);

			DDR_ctrl->data_valid_count = 0;
		}
	}
	for (UINT8 current = DDR_ctrl->list_start_ptr; current != DDR_ctrl->list_stop_ptr && mem_data_read->snoop_response == snoop_idle; current = ((current + 1) & (DDR_ctrl->list_count - 1))) {
		for (UINT8 index = 0; index < 4 && mem_data_read->snoop_response == snoop_idle; index++) {
			if ((DDR_ctrl->list[current].status == ddr_read || DDR_ctrl->list[current].status == ddr_allocate_c) && ((DDR_ctrl->list[current].addr & (UINT64)(~0x007f)) == (DDR_ctrl->victim[index].tag & (UINT64)(~0x007f))) && DDR_ctrl->victim[DDR_ctrl->victim_ptr].xaction_id != DDR_ctrl->list[current].xaction_id) {
				if (Bclock == 0x3a)
					debug++;
				mem_data_read->snoop_response = snoop_hit;
				mem_data_read->xaction_id = DDR_ctrl->list[current].xaction_id;
				mem_data_read->cacheable = DDR_ctrl->list[current].cacheable;
				mem_data_read->valid = 0x0f;
				for (UINT8 i = 0; i < 0x10; i++) mem_data_read->data[i] = DDR_ctrl->victim[index].data[i];
				DDR_ctrl->list[current].status = ddr_idle;
				if (debug_unit) {
					fprintf(debug_stream, "DRAM  q_id: %#04x %#04x-%#04x, xaction id %#06x, bus: Return data from victim cache; ", current, DDR_ctrl->list_start_ptr, DDR_ctrl->list_stop_ptr, mem_data_read->xaction_id);
					fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
					fprintf(debug_stream, "DRAM  d[0] 0x%16llx,  d[1] 0x%16llx, d[2] 0x%16llx, d[3] 0x%16llx, d[4] 0x%16llx, d[5] 0x%16llx, d[6] 0x%16llx, d[7] 0x%16llx,\n",
						DDR_ctrl->victim[index].data[0], DDR_ctrl->victim[index].data[1], DDR_ctrl->victim[index].data[2], DDR_ctrl->victim[index].data[3], DDR_ctrl->victim[index].data[4], DDR_ctrl->victim[index].data[5], DDR_ctrl->victim[index].data[6], DDR_ctrl->victim[index].data[7]);
					fprintf(debug_stream, "DRAM  d[8] 0x%16llx, d[9] 0x%16llx, d[a] 0x%16llx, d[b] 0x%16llx, d[c] 0x%16llx, d[d] 0x%16llx, d[e] 0x%16llx, d[f] 0x%16llx\n",
						DDR_ctrl->victim[index].data[9], DDR_ctrl->victim[index].data[9], DDR_ctrl->victim[index].data[10], DDR_ctrl->victim[index].data[11], DDR_ctrl->victim[index].data[12], DDR_ctrl->victim[index].data[13], DDR_ctrl->victim[index].data[14], DDR_ctrl->victim[index].data[15]);
				}
			}
		}
	}
	//
	// core
	//
	// following is an accurate loop per DDR channel
	// each channel can run independently, but work on different physical address space
	int hit = 0;

	UINT8 cas_active = 0;
	for (UINT8 i = 0; i < 0x10; i++) cas_active |= DDR_ctrl->data_bus[i].valid;
	if (cas_active == 0) {
		DDR_ctrl->bank[0].CA = DDR_ctrl->bank[1].CA = DDR_ctrl->bank[2].CA = DDR_ctrl->bank[3].CA = -1;// not active
	}
	for (UINT8 current = DDR_ctrl->list_start_ptr; current != DDR_ctrl->list_stop_ptr && !hit; current = ((current + 1) & (DDR_ctrl->list_count - 1))) {
		if (DDR_ctrl->list[current].status == ddr_allocate1) {
			if (DDR_ctrl->wc[DDR_ctrl->list[current].wc_ptr].dirty == 0xffffffff)
				DDR_ctrl->list[current].status = ddr_write;
		}
	}

	if (mem_data_read->snoop_response == snoop_idle) {
		for (UINT8 current = DDR_ctrl->list_start_ptr; current != DDR_ctrl->list_stop_ptr && !hit; current = ((current + 1) & (DDR_ctrl->list_count - 1))) {
			if (DDR_ctrl->list[current].status == ddr_allocate_c && DDR_ctrl->wc[DDR_ctrl->list[current].wc_ptr].dirty == 0xffffffff) {
				mem_data_read->snoop_response = snoop_hit;
				mem_data_read->xaction_id = DDR_ctrl->list[current].xaction_id;
				mem_data_read->cacheable = DDR_ctrl->list[current].cacheable;
				mem_data_read->valid = 0x0f;
				for (UINT8 i = 0; i < 0x10; i++) mem_data_read->data[i] = DDR_ctrl->wc[DDR_ctrl->list[current].wc_ptr].data[i];
				DDR_ctrl->list[current].status = ddr_idle;
				if (debug_unit) {
					fprintf(debug_stream, "DRAM bus: Return data from mWC(%d) buffers; xaction id %#06x, ", DDR_ctrl->list[current].wc_ptr, mem_data_read->xaction_id);
					fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
				}
			}
		}
	}
	// bus driven cycles, first make sure there is a spot open
	if (DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].valid == 0 && DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].valid == 0 &&
		DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].valid == 0 && DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].valid == 0) {

		// locked cycles must be in order
		if (DDR_ctrl->list[DDR_ctrl->list_start_ptr].status == ddr_read_fence) {
			UINT8 bank = ((DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr >> 21) & 3);
			if ((DDR_ctrl->bank[bank].RA == ((DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr >> 23) & 0x3fff))) {
				if (DDR_ctrl->bank[bank].CA != ((DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr >> 7) & 0x3fff)) {
					DDR_ctrl->bank[bank].CA = ((DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr >> 7) & 0x3fff);

					if (2 * Bclock >= 0x17d4)
						debug++;
					if (2 * Bclock >= 0x164a0)
						debug++;
					memory_abstract(DDR_ctrl->list[DDR_ctrl->list_start_ptr].data, DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr, 0, memory_space, DDR_ctrl->list[DDR_ctrl->list_start_ptr].xaction_id, Bclock);
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].valid = 1;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].valid = 1;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].valid = 1;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].valid = 1;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].write = 0;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].write = 0;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].write = 0;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].write = 0;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].current_data = DDR_ctrl->list_start_ptr;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].current_data = DDR_ctrl->list_start_ptr;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].current_data = DDR_ctrl->list_start_ptr;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].current_data = DDR_ctrl->list_start_ptr;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].list_ptr = -1;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].list_ptr = -1;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].list_ptr = -1;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].list_ptr = -1;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].tag_addr = DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].tag_addr = DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].tag_addr = DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr;
					DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].tag_addr = DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr;
					DDR_ctrl->bank[bank].CA_issue_ptr = DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency;
					if (debug_unit) {
						fprintf(debug_stream, "DRAM bank(%d): RA = %#05x: CA = %#05x: CAS issued: Locked Read;", bank, DDR_ctrl->bank[bank].RA, DDR_ctrl->bank[bank].CA);
					}
					if (DDR_ctrl->list[DDR_ctrl->list_start_ptr].status == ddr_allocate1) {
						if (debug_unit) {
							fprintf(debug_stream, " mWC(%d) valid: 0x%08llx", DDR_ctrl->list[DDR_ctrl->list_start_ptr].wc_ptr, DDR_ctrl->wc[DDR_ctrl->list[DDR_ctrl->list_start_ptr].wc_ptr].dirty);
						}
						DDR_ctrl->list[DDR_ctrl->list_start_ptr].status = ddr_allocate2;
					}
					else if (DDR_ctrl->list[DDR_ctrl->list_start_ptr].status == ddr_read_fence) {
						DDR_ctrl->list[DDR_ctrl->list_start_ptr].status = ddr_read_fence2;
					}
					else {
						debug++;
					}
					if (debug_unit) {
						fprintf(debug_stream, " addr ");
						fprint_addr_coma(debug_stream, DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr, param);
						fprintf(debug_stream, " xaction id %#06x, Bclock: 0x%04llx, clock: 0x%04llx\n",
							DDR_ctrl->list[DDR_ctrl->list_start_ptr].xaction_id, Bclock, 2 * Bclock);
					}
					DDR_ctrl->bank[bank].CA_strobe = 1;
					DDR_ctrl->bank_ptr = ((bank + 1) & 3);
				}
				/**/
				if (DDR_ctrl->bank[bank].CA == ((DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr >> 7) & 0x3fff)) {
					DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 0) & 0x1f].list_ptr = DDR_ctrl->list_start_ptr;
					DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 1) & 0x1f].list_ptr = DDR_ctrl->list_start_ptr;
					DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 2) & 0x1f].list_ptr = DDR_ctrl->list_start_ptr;
					DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 3) & 0x1f].list_ptr = DDR_ctrl->list_start_ptr;
					if (debug_unit) {
						fprintf(debug_stream, "DRAM bank(%d): Locked Read;", bank);
						fprintf(debug_stream, " moved to data cycle addr, data: 0x%016I64x, 0x%016I64x xaction id %#06x, Bclock: 0x%04llx, clock: 0x%04llx\n",
							DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr, DDR_ctrl->list[DDR_ctrl->list_start_ptr].data[(DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr >> 2) & 0x1f], DDR_ctrl->list[DDR_ctrl->list_start_ptr].xaction_id, Bclock, 2 * Bclock);
					}
					hit = 1;
				}
				/**/
			}
			else {
				if (DDR_ctrl->bank[bank].delay == 0) { // will implement cross bank blocks later - likely a queue, similar to data
					DDR_ctrl->bank[bank].RA = ((DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr >> 23) & 0x3fff);
					DDR_ctrl->bank[bank].row_valid = 1;
					DDR_ctrl->bank[bank].delay = tRCD;
					if (debug_unit) {
						fprintf(debug_stream, "DRAM bank(%d): RA = %#05x: RAS issued: ", bank, DDR_ctrl->bank[bank].RA);
						print_xaction3(DDR_ctrl->list[DDR_ctrl->list_start_ptr].status, debug_stream);
						fprintf(debug_stream, " addr ");
						fprint_addr_coma(debug_stream, DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr, param);
						fprintf(debug_stream, " xaction id %#06x, Bclock: 0x%04llx, clock: 0x%04llx\n", 
							DDR_ctrl->list[DDR_ctrl->list_start_ptr].xaction_id, Bclock, 2 * Bclock);
					}
					hit = 1;
					DDR_ctrl->bank_ptr = ((bank + 1) & 3);
				}
				else {
					DDR_ctrl->bank[bank].delay--;
				}
			}
		}
		else if (Bclock - DDR_ctrl->list[DDR_ctrl->list_start_ptr].clock > 0x200 && DDR_ctrl->list[DDR_ctrl->list_start_ptr].status != ddr_idle) {
			UINT8 bank = ((DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr >> 21) & 3);

			DDR_ctrl->bank[bank].CA_strobe_latch = DDR_ctrl->bank[bank].CA_strobe;
			DDR_ctrl->bank[bank].CA_strobe = 0;
			if (DDR_ctrl->bank[bank].delay == 0) { // will implement cross bank blocks later - likely a queue, similar to data
				UINT stop = 0;
				if ((DDR_ctrl->bank[bank].RA == ((DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr >> 23) & 0x3fff))) {
					if ((DDR_ctrl->list[DDR_ctrl->list_start_ptr].status == ddr_write)) {
						UINT8 skip = 0;
						for (UINT8 i = 0; i < DDR_ctrl->CAS_W_latency; i++)
							skip |= (DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + i + 0) & 0x1f].valid & !DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + i + 0) & 0x1f].write);
						for (UINT8 i = DDR_ctrl->CAS_W_latency; i < DDR_ctrl->CAS_W_latency + 4; i++)
							skip |= DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + i + 0) & 0x1f].valid;
						if ((DDR_ctrl->wc[DDR_ctrl->list[DDR_ctrl->list_start_ptr].wc_ptr].data_pending == 0) && !skip) {
							//							UINT skip = 0;
							hit = 1;
							memory_abstract(DDR_ctrl->wc[DDR_ctrl->list[DDR_ctrl->list_start_ptr].wc_ptr].data, DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr, 1, memory_space, DDR_ctrl->wc[DDR_ctrl->list[DDR_ctrl->list_start_ptr].wc_ptr].xaction_id, Bclock);

							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 0) & 0x1f].valid = 1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 1) & 0x1f].valid = 1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 2) & 0x1f].valid = 1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 3) & 0x1f].valid = 1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 0) & 0x1f].write = 1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 1) & 0x1f].write = 1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 2) & 0x1f].write = 1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 3) & 0x1f].write = 1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 0) & 0x1f].current_data = DDR_ctrl->list_start_ptr;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 1) & 0x1f].current_data = DDR_ctrl->list_start_ptr;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 2) & 0x1f].current_data = DDR_ctrl->list_start_ptr;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 3) & 0x1f].current_data = DDR_ctrl->list_start_ptr;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 0) & 0x1f].list_ptr = -1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 1) & 0x1f].list_ptr = -1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 2) & 0x1f].list_ptr = -1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 3) & 0x1f].list_ptr = -1;
							if (debug_unit) {
								fprintf(debug_stream, "DRAM bank(%d): RA = %#05x: CA = %#05x: CAS issued: synch DRAM with mWC(%d)", bank, DDR_ctrl->bank[bank].RA, DDR_ctrl->bank[bank].CA, DDR_ctrl->list[DDR_ctrl->list_start_ptr].wc_ptr);
								fprintf(debug_stream, " address: 0x%016I64x, xaction id %#06x, ", DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr, DDR_ctrl->list[DDR_ctrl->list_start_ptr].xaction_id);
								fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
							}
							DDR_ctrl->list[DDR_ctrl->list_start_ptr].status = ddr_idle;
							DDR_ctrl->wc[DDR_ctrl->list[DDR_ctrl->list_start_ptr].wc_ptr].addr_valid = 0;
							DDR_ctrl->list[DDR_ctrl->list_start_ptr].status = ddr_idle;

							DDR_ctrl->bank[bank].CA_strobe = 1;
							DDR_ctrl->bank_ptr = ((bank + 1) & 3);
						}
					}
					else if ((DDR_ctrl->list[DDR_ctrl->list_start_ptr].status != ddr_allocate2) && (DDR_ctrl->list[DDR_ctrl->list_start_ptr].status != ddr_allocate0) && (DDR_ctrl->list[DDR_ctrl->list_start_ptr].status != ddr_read2) && (DDR_ctrl->list[DDR_ctrl->list_start_ptr].status != ddr_read_fence2) && (DDR_ctrl->list[DDR_ctrl->list_start_ptr].status != ddr_allocate_c2)) {
						UINT8 victim_hit = 0;
						for (UINT8 index = 0; index < 4; index++)
							if ((DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr & (UINT64)(~0x007f)) == (DDR_ctrl->victim[index].tag & (UINT64)(~0x007f)))
								victim_hit = 1;
						if (DDR_ctrl->bank[bank].CA != ((DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr >> 7) & 0x3fff) && !victim_hit &&
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].valid == 0 && DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].valid == 0 &&
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].valid == 0 && DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].valid == 0) {

							DDR_ctrl->bank[bank].CA = ((DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr >> 7) & 0x3fff);

							memory_abstract(DDR_ctrl->list[DDR_ctrl->list_start_ptr].data, DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr, 0, memory_space, DDR_ctrl->list[DDR_ctrl->list_start_ptr].xaction_id, Bclock);
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].valid = 1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].valid = 1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].valid = 1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].valid = 1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].write = 0;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].write = 0;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].write = 0;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].write = 0;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].current_data = DDR_ctrl->list_start_ptr;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].current_data = DDR_ctrl->list_start_ptr;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].current_data = DDR_ctrl->list_start_ptr;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].current_data = DDR_ctrl->list_start_ptr;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].list_ptr = -1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].list_ptr = -1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].list_ptr = -1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].list_ptr = -1;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].tag_addr = DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].tag_addr = DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].tag_addr = DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr;
							DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].tag_addr = DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr;
							DDR_ctrl->bank[bank].CA_issue_ptr = DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency;
							if (debug_unit) {
								fprintf(debug_stream, "DRAM bank(%d): RA = %#05x: CA = %#05x: CAS issued: ", bank, DDR_ctrl->bank[bank].RA, DDR_ctrl->bank[bank].CA);
								print_xaction3(DDR_ctrl->list[DDR_ctrl->list_start_ptr].status, debug_stream);
							}
							if (DDR_ctrl->list[DDR_ctrl->list_start_ptr].status == ddr_allocate1) {
								if (debug_unit) {
									fprintf(debug_stream, " mWC(%d) valid: 0x%08llx", DDR_ctrl->list[DDR_ctrl->list_start_ptr].wc_ptr, DDR_ctrl->wc[DDR_ctrl->list[DDR_ctrl->list_start_ptr].wc_ptr].dirty);
								}
								DDR_ctrl->list[DDR_ctrl->list_start_ptr].status = ddr_allocate2;
							}
							else if (DDR_ctrl->list[DDR_ctrl->list_start_ptr].status == ddr_read) {
								DDR_ctrl->list[DDR_ctrl->list_start_ptr].status = ddr_read2;
							}
							else if (DDR_ctrl->list[DDR_ctrl->list_start_ptr].status == ddr_allocate_c) {
								DDR_ctrl->list[DDR_ctrl->list_start_ptr].status = ddr_allocate_c2;
							}
							else {
								debug++;
							}
							if (debug_unit) {
								fprintf(debug_stream, " address: 0x%016I64x, xaction id %#06x, ", DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr, DDR_ctrl->list[DDR_ctrl->list_start_ptr].xaction_id);
								fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
							}
							DDR_ctrl->bank[bank].CA_strobe = 1;
							DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 0) & 0x1f].list_ptr = DDR_ctrl->list_start_ptr;
							DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 1) & 0x1f].list_ptr = DDR_ctrl->list_start_ptr;
							DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 2) & 0x1f].list_ptr = DDR_ctrl->list_start_ptr;
							DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 3) & 0x1f].list_ptr = DDR_ctrl->list_start_ptr;
							hit = 1;
							DDR_ctrl->bank_ptr = ((bank + 1) & 3);
						}
					}
				}
				else {
					if (DDR_ctrl->bank[bank].delay == 0) { // will implement cross bank blocks later - likely a queue, similar to data
						DDR_ctrl->bank[bank].RA = ((DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr >> 23) & 0x3fff);
						DDR_ctrl->bank[bank].row_valid = 1;
						DDR_ctrl->bank[bank].delay = tRCD;
						if (debug_unit) {
							fprintf(debug_stream, "DRAM bank(%d): RA = %#05x: RAS issued: ", bank, DDR_ctrl->bank[bank].RA);
							print_xaction3(DDR_ctrl->list[DDR_ctrl->list_start_ptr].status, debug_stream);
							fprintf(debug_stream, " xaction id %#06x, addr ", DDR_ctrl->list[DDR_ctrl->list_start_ptr].xaction_id);
							fprint_addr_coma(debug_stream, DDR_ctrl->list[DDR_ctrl->list_start_ptr].addr, param);
							fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
						}
						hit = 1;
						DDR_ctrl->bank_ptr = ((bank + 1) & 3);
					}
					else {
						DDR_ctrl->bank[bank].delay--;
					}
				}
			}
		}
		else {
			for (UINT8 bank_count = 0, bank = DDR_ctrl->bank_ptr; bank_count < 4; bank_count++, bank = ((bank + 1) & 3)) {// do bank miss first

				DDR_ctrl->bank[bank].CA_strobe_latch = DDR_ctrl->bank[bank].CA_strobe;
				DDR_ctrl->bank[bank].CA_strobe = 0;
				if (DDR_ctrl->bank[bank].delay == 0) { // will implement cross bank blocks later - likely a queue, similar to data
					UINT stop = 0;
					for (UINT8 current = DDR_ctrl->list_start_ptr; current != DDR_ctrl->list_stop_ptr && !hit && !stop; current = ((current + 1) & (DDR_ctrl->list_count - 1))) {
						if (DDR_ctrl->list[current].status == ddr_read_fence) {
//							if (current == DDR_ctrl->list_start_ptr && (bank == ((DDR_ctrl->list[current].addr >> 21) & 3)) && (DDR_ctrl->bank[bank].RA == ((DDR_ctrl->list[current].addr >> 23) & 0x3fff))) {
							if ((bank == ((DDR_ctrl->list[current].addr >> 21) & 3)) && (DDR_ctrl->bank[bank].RA == ((DDR_ctrl->list[current].addr >> 23) & 0x3fff))) {
								stop = 1;
								if (DDR_ctrl->bank[bank].CA != ((DDR_ctrl->list[current].addr >> 7) & 0x3fff)) {
									DDR_ctrl->bank[bank].CA = ((DDR_ctrl->list[current].addr >> 7) & 0x3fff);
									if (2 * Bclock == 0x17d4)
										debug++;

									memory_abstract(DDR_ctrl->list[current].data, DDR_ctrl->list[current].addr, 0, memory_space, DDR_ctrl->list[current].xaction_id, Bclock);
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].valid = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].valid = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].valid = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].valid = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].write = 0;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].write = 0;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].write = 0;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].write = 0;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].current_data = current;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].current_data = current;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].current_data = current;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].current_data = current;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].list_ptr = -1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].list_ptr = -1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].list_ptr = -1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].list_ptr = -1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].tag_addr = DDR_ctrl->list[current].addr;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].tag_addr = DDR_ctrl->list[current].addr;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].tag_addr = DDR_ctrl->list[current].addr;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].tag_addr = DDR_ctrl->list[current].addr;
									DDR_ctrl->bank[bank].CA_issue_ptr = DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency;
									if (debug_unit) {
										fprintf(debug_stream, "DRAM bank(%d): RA = %#05x: CA = %#05x: CAS issued: Locked Read;", bank, DDR_ctrl->bank[bank].RA, DDR_ctrl->bank[bank].CA);
									}
									if (DDR_ctrl->list[current].status == ddr_allocate1) {
										if (debug_unit) {
											fprintf(debug_stream, " mWC(%d) valid: 0x%08llx", DDR_ctrl->list[current].wc_ptr, DDR_ctrl->wc[DDR_ctrl->list[current].wc_ptr].dirty);
										}
										DDR_ctrl->list[current].status = ddr_allocate2;
									}
									else if (DDR_ctrl->list[current].status == ddr_read_fence) {
										DDR_ctrl->list[current].status = ddr_read_fence2;
									}
									else {
										debug++;
									}
									if (debug_unit) {
										fprintf(debug_stream, " address: 0x%016I64x, xaction id %#06x, Bclock: 0x%04llx, clock: 0x%04llx\n", DDR_ctrl->list[current].addr, DDR_ctrl->list[current].xaction_id, Bclock, 2 * Bclock);
									}
									DDR_ctrl->bank[bank].CA_strobe = 1;
									DDR_ctrl->bank_ptr = ((bank + 1) & 3);
								}
								if (DDR_ctrl->bank[bank].CA == ((DDR_ctrl->list[current].addr >> 7) & 0x3fff)) {
									DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 0) & 0x1f].list_ptr = current;
									DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 1) & 0x1f].list_ptr = current;
									DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 2) & 0x1f].list_ptr = current;
									DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 3) & 0x1f].list_ptr = current;
									if (debug_unit) {
										fprintf(debug_stream, "DRAM bank(%d): Locked Read;", bank);
										fprintf(debug_stream, " moved to data cycle addr,data: 0x%016I64x, 0x%016I64x xaction id %#06x, Bclock: 0x%04llx, clock: 0x%04llx\n",
											DDR_ctrl->list[current].addr, DDR_ctrl->list[current].data[(DDR_ctrl->list[current].addr >> 2) & 0x1f], DDR_ctrl->list[current].xaction_id, Bclock, 2 * Bclock);
									}
									hit = 1;
								}
							}
						}
						else if ((DDR_ctrl->list[current].status != ddr_idle) && (bank == ((DDR_ctrl->list[current].addr >> 21) & 3)) && (DDR_ctrl->bank[bank].RA == ((DDR_ctrl->list[current].addr >> 23) & 0x3fff))) {
							if ((DDR_ctrl->list[current].status == ddr_write)) {
								UINT8 skip = 0;
								for (UINT8 i = 0; i < DDR_ctrl->CAS_W_latency; i++)
									skip |= (DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + i + 0) & 0x1f].valid & !DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + i + 0) & 0x1f].write);
								for (UINT8 i = DDR_ctrl->CAS_W_latency; i < DDR_ctrl->CAS_W_latency + 4; i++)
									skip |= DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + i + 0) & 0x1f].valid;
								if ((DDR_ctrl->wc[DDR_ctrl->list[current].wc_ptr].data_pending == 0) && !skip) {
									//							UINT skip = 0;
									hit = 1;
									memory_abstract(DDR_ctrl->wc[DDR_ctrl->list[current].wc_ptr].data, DDR_ctrl->list[current].addr, 1, memory_space, DDR_ctrl->wc[DDR_ctrl->list[current].wc_ptr].xaction_id, Bclock);

									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 0) & 0x1f].valid = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 1) & 0x1f].valid = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 2) & 0x1f].valid = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 3) & 0x1f].valid = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 0) & 0x1f].write = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 1) & 0x1f].write = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 2) & 0x1f].write = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 3) & 0x1f].write = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 0) & 0x1f].current_data = current;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 1) & 0x1f].current_data = current;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 2) & 0x1f].current_data = current;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 3) & 0x1f].current_data = current;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 0) & 0x1f].list_ptr = -1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 1) & 0x1f].list_ptr = -1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 2) & 0x1f].list_ptr = -1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_W_latency + 3) & 0x1f].list_ptr = -1;
									if (debug_unit) {
										fprintf(debug_stream, "DRAM bank(%d): RA = %#05x: CA = %#05x: CAS issued: synch DRAM with mWC(%d)", bank, DDR_ctrl->bank[bank].RA, DDR_ctrl->bank[bank].CA, DDR_ctrl->list[current].wc_ptr);
										fprintf(debug_stream, " address: 0x%016I64x, xaction id %#06x, ", DDR_ctrl->list[current].addr, DDR_ctrl->list[current].xaction_id);
										fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
									}
									DDR_ctrl->list[current].status = ddr_idle;
									DDR_ctrl->wc[DDR_ctrl->list[current].wc_ptr].addr_valid = 0;
									DDR_ctrl->list[current].status = ddr_idle;

									DDR_ctrl->bank[bank].CA_strobe = 1;
									DDR_ctrl->bank_ptr = ((bank + 1) & 3);
								}
							}
							else if ((DDR_ctrl->list[current].status != ddr_allocate2) && (DDR_ctrl->list[current].status != ddr_allocate0) && (DDR_ctrl->list[current].status != ddr_read2) && (DDR_ctrl->list[current].status != ddr_read_fence2) && (DDR_ctrl->list[current].status != ddr_allocate_c2)) {
								UINT8 victim_hit = 0;
								for (UINT8 index = 0; index < 4; index++)
									if ((DDR_ctrl->list[current].addr & (UINT64)(~0x007f)) == (DDR_ctrl->victim[index].tag & (UINT64)(~0x007f)) && DDR_ctrl->victim[DDR_ctrl->victim_ptr].xaction_id != DDR_ctrl->list[current].xaction_id)
										victim_hit = 1;
								if (DDR_ctrl->bank[bank].CA != ((DDR_ctrl->list[current].addr >> 7) & 0x3fff) && !victim_hit &&
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].valid == 0 && DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].valid == 0 &&
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].valid == 0 && DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].valid == 0) {

									DDR_ctrl->bank[bank].CA = ((DDR_ctrl->list[current].addr >> 7) & 0x3fff);

									if (Bclock == 0x10e6)
										debug++;
									if (2 * Bclock >= 0x164a0)
										debug++;
									memory_abstract(DDR_ctrl->list[current].data, DDR_ctrl->list[current].addr, 0, memory_space, DDR_ctrl->list[current].xaction_id, Bclock);
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].valid = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].valid = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].valid = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].valid = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].write = 0;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].write = 0;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].write = 0;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].write = 0;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].current_data = current;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].current_data = current;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].current_data = current;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].current_data = current;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].list_ptr = -1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].list_ptr = -1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].list_ptr = -1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].list_ptr = -1;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 0) & 0x1f].tag_addr = DDR_ctrl->list[current].addr;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 1) & 0x1f].tag_addr = DDR_ctrl->list[current].addr;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 2) & 0x1f].tag_addr = DDR_ctrl->list[current].addr;
									DDR_ctrl->data_bus[(DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency + 3) & 0x1f].tag_addr = DDR_ctrl->list[current].addr;
									DDR_ctrl->bank[bank].CA_issue_ptr = DDR_ctrl->data_bus_ptr + DDR_ctrl->CAS_latency;
									if (debug_unit) {
										fprintf(debug_stream, "DRAM bank(%d): RA = %#05x: CA = %#05x: CAS issued: 0x%04x, ", bank, DDR_ctrl->bank[bank].RA, DDR_ctrl->bank[bank].CA);
										print_xaction3(DDR_ctrl->list[current].status, debug_stream);
									}
									if (DDR_ctrl->list[current].status == ddr_allocate1) {
										if (debug_unit) {
											fprintf(debug_stream, " mWC(%d) valid: 0x%08llx", DDR_ctrl->list[current].wc_ptr, DDR_ctrl->wc[DDR_ctrl->list[current].wc_ptr].dirty);
										}
										DDR_ctrl->list[current].status = ddr_allocate2;
									}
									else if (DDR_ctrl->list[current].status == ddr_read) {
										DDR_ctrl->list[current].status = ddr_read2;
									}
									else if (DDR_ctrl->list[current].status == ddr_allocate_c) {
										DDR_ctrl->list[current].status = ddr_allocate_c2;
									}
									else {
										debug++;
									}
									if (debug_unit) {
										fprintf(debug_stream, " address: 0x%016I64x, xaction id %#06x, eta(Bclock): 0x%04llx, ", DDR_ctrl->list[current].addr, DDR_ctrl->list[current].xaction_id, Bclock + DDR_ctrl->CAS_latency);
										fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
									}
									DDR_ctrl->bank[bank].CA_strobe = 1;
									DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 0) & 0x1f].list_ptr = current;
									DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 1) & 0x1f].list_ptr = current;
									DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 2) & 0x1f].list_ptr = current;
									DDR_ctrl->data_bus[(DDR_ctrl->bank[bank].CA_issue_ptr + 3) & 0x1f].list_ptr = current;
									hit = 1;
									DDR_ctrl->bank_ptr = ((bank + 1) & 3);
								}
							}
						}
					}
				}
			}
		}
	}
	for (UINT8 bank_count = 0, bank = DDR_ctrl->bank_ptr; bank_count < 4; bank_count++, bank = ((bank + 1) & 3)) {// do bank hits first

		// check row hits for all banks first
		// then check for row misses for all banks
		if (DDR_ctrl->bank[bank].delay == 0) { // will implement cross bank blocks later - likely a queue, similar to data

			for (UINT8 current = DDR_ctrl->list_start_ptr; current != DDR_ctrl->list_stop_ptr && !hit; current = ((current + 1) & (DDR_ctrl->list_count - 1))) {
				if ((DDR_ctrl->list[current].status != ddr_idle) && (bank == ((DDR_ctrl->list[current].addr >> 21) & 3))) {
					if ((DDR_ctrl->bank[bank].RA == ((DDR_ctrl->list[current].addr >> 23) & 0x3fff))) {
						hit = 1; // clear out page hits before switch pages
					}
					else {
						DDR_ctrl->bank[bank].RA = ((DDR_ctrl->list[current].addr >> 23) & 0x3fff);
						DDR_ctrl->bank[bank].row_valid = 1;
						DDR_ctrl->bank[bank].delay = tRCD;
						if (debug_unit) {
							fprintf(debug_stream, "DRAM bank(%d): RA = %#05x: RAS issued: ", bank, DDR_ctrl->bank[bank].RA);
							print_xaction3(DDR_ctrl->list[current].status, debug_stream);
							fprintf(debug_stream, " xaction id %#06x, addr ", DDR_ctrl->list[current].xaction_id);
							fprint_addr_coma(debug_stream, DDR_ctrl->list[current].addr, param);
							fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
						}
						hit = 1;
						DDR_ctrl->bank_ptr = ((bank + 1) & 3);
					}
				}
			}
		}
		else {
			DDR_ctrl->bank[bank].delay--;
		}
	}
	// write data latch
	if (mem_data_write->snoop_response != snoop_idle && reset == 0) {
		UINT8 hit = 0;
		for (UINT8 current = DDR_ctrl->list_start_ptr; current != DDR_ctrl->list_stop_ptr && !hit; current = ((current + 1) & (DDR_ctrl->list_count - 1))) {
			if (mem_data_write->xaction_id == DDR_ctrl->list[current].xaction_id &&
				(DDR_ctrl->list[current].status == ddr_allocate0 || (mem_data_write->valid == 0xffffffffffffffff && DDR_ctrl->list[current].xaction == bus_store_full))) {
				hit = 1;
				UINT8 wc = DDR_ctrl->list[current].wc_ptr;
				DDR_ctrl->list[current].status = ddr_allocate1;

				DDR_ctrl->wc[wc].dirty |= mem_data_write->valid;
				DDR_ctrl->wc[wc].xaction_id = mem_data_write->xaction_id;
				for (UINT8 word = 0; word < 0x10; word++) {
					if (mem_data_write->valid & (0x3 << (word * 2))) {
						DDR_ctrl->wc[wc].data[word] = DDR_ctrl->list[current].data[word] = mem_data_write->data[word];
					}
				}
				if (debug_unit) {
					fprintf(debug_stream, "DRAM ctrl: DATA latch: ");
					fprintf(debug_stream, "buffers updated; mWC(%d) addr: 0x%016I64x, valid: 0x%08llx, source valid: %#010x, xaction id %#06x ", wc, DDR_ctrl->list[current].addr, DDR_ctrl->wc[wc].dirty, mem_data_write->valid, mem_data_write->xaction_id);
					fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
				}
				if (DDR_ctrl->list[current].xaction_id != DDR_ctrl->wc[wc].xaction_id)
					DDR_ctrl->list[current].status = ddr_idle;
				DDR_ctrl->wc[DDR_ctrl->list[current].wc_ptr].data_pending = 0;
			}
		}
		//		if (debug_unit) {
		if (!hit) { // try to match wc posters before declaring error
			fprintf(debug_stream, "DRAM ctrl: DATA latch: ");
			fprintf(debug_stream, "ERROR - no allocate match: WRITE data xaction id %#06x, ", mem_data_write->xaction_id);
			fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
		}
		//		}
	}

	// address latch
	if (mem_addr->strobe) {

		UINT bank = (mem_addr->addr >> 21) & 3;
		UINT RA = (mem_addr->addr >> 23) & 0x3fff;

		hit = 0;
		UINT8 temp = DDR_ctrl->list_stop_ptr;
		for (UINT8 wc_ptr = 0; wc_ptr < 4; wc_ptr++) {
			if (DDR_ctrl->wc[wc_ptr].addr_valid == 1 && (DDR_ctrl->wc[wc_ptr].addr & (UINT64)(~0x007f)) == (mem_addr->addr & (UINT64)(~0x007f))) {
				hit = 1;
				if (debug_unit) {
					fprintf(debug_stream, "DRAM bank(%d): wc(%d) xaction id %#06x, ", bank, wc_ptr, mem_addr->xaction_id, mem_addr->addr);
					print_xaction2(mem_addr->xaction, debug_stream);
				}
				DDR_ctrl->list[temp].wc_ptr = wc_ptr;
				if (DDR_ctrl->bank[bank].RA == RA && DDR_ctrl->bank[bank].row_valid) {

					UINT8 temp = DDR_ctrl->list_stop_ptr;
					DDR_ctrl->list[temp].xaction = mem_addr->xaction;
					DDR_ctrl->list[temp].xaction_id = mem_addr->xaction_id;
					DDR_ctrl->list[temp].addr = mem_addr->addr;
					DDR_ctrl->list[temp].cacheable = mem_addr->cacheable;
					DDR_ctrl->list[temp].clock = Bclock;
					if (debug_unit) {
						fprintf(debug_stream, " mWC(%d)", DDR_ctrl->list[temp].wc_ptr);
					}
					if (mem_addr->cacheable != page_non_cache) {
						if (mem_addr->xaction == bus_allocate) {
							DDR_ctrl->list[temp].status = ddr_allocate_c;
							DDR_ctrl->list[temp].wc_ptr = wc_ptr;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].data_pending = 1;
						}
						else if (mem_addr->xaction == bus_LR_aq_rl) {
							DDR_ctrl->list[temp].status = ddr_read_fence;
						}
						else {// need to flush out write poster before completing read
							// optimization 1: perform fence operation
							// optimization 2: flush out only this WC buffer
							// optimization 3: create data path from write posters to cores
							DDR_ctrl->list[temp].status = ddr_read_fence;
						}
					}
					else {
						if (mem_addr->xaction == bus_store_full || mem_addr->xaction == bus_store_partial) {
							DDR_ctrl->list[temp].status = ddr_allocate0;
							DDR_ctrl->list[temp].wc_ptr = wc_ptr;
							DDR_ctrl->wc[DDR_ctrl->list[temp].wc_ptr].data_pending = 1;
						}
						else {
							DDR_ctrl->list[temp].status = ddr_read_fence;// need to flush WC buffers
						}
					}
					if (debug_unit) {
						fprintf(debug_stream, "q_id: %#04x %#04x-%#04x, RA hit, latch addr: 0x%016I64x, ", DDR_ctrl->list_stop_ptr, DDR_ctrl->list_start_ptr, DDR_ctrl->list_stop_ptr, mem_addr->addr);
					}
					DDR_ctrl->list_stop_ptr = (DDR_ctrl->list_stop_ptr + 1) & (DDR_ctrl->list_count - 1);
				}
				else {// should fix duplication with function call, make hit, miss a parameter that is passed
					if (debug_unit) {
						fprintf(debug_stream, " RA miss, latch addr ");
						fprint_addr_coma(debug_stream, mem_addr->addr, param);
					}
					DDR_ctrl->list[temp].xaction = mem_addr->xaction;
					DDR_ctrl->list[temp].xaction_id = mem_addr->xaction_id;
					DDR_ctrl->list[temp].addr = mem_addr->addr;
					DDR_ctrl->list[temp].cacheable = mem_addr->cacheable;
					DDR_ctrl->list[temp].clock = Bclock;
					if (mem_addr->cacheable != page_non_cache) {
						if (mem_addr->xaction == bus_allocate) {
							DDR_ctrl->list[temp].status = ddr_allocate_c;
							DDR_ctrl->list[temp].wc_ptr = wc_ptr;
							if (debug_unit) {
								fprintf(debug_stream, " mWC(%d), ", DDR_ctrl->list[temp].wc_ptr);
							}
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].data_pending = 1;
						}
						else if (mem_addr->xaction == bus_LR_aq_rl) {
							DDR_ctrl->list[temp].status = ddr_read_fence;
						}
						else {
							DDR_ctrl->list[temp].status = ddr_read;
						}
					}
					else {
						if (mem_addr->xaction == bus_store_full || mem_addr->xaction == bus_store_partial) {
							DDR_ctrl->list[temp].status = ddr_allocate0;
							DDR_ctrl->list[temp].wc_ptr = wc_ptr;
							DDR_ctrl->wc[DDR_ctrl->list[temp].wc_ptr].data_pending = 1;
							if (debug_unit) {
								fprintf(debug_stream, " mWC(%d)", DDR_ctrl->list[temp].wc_ptr);
							}
						}
						else {
							DDR_ctrl->list[temp].status = ddr_read;
						}
					}
					DDR_ctrl->list_stop_ptr = (DDR_ctrl->list_stop_ptr + 1) & (DDR_ctrl->list_count - 1);
				}
				if (debug_unit) {
					fprintf(debug_stream, "List Entry: %#04x, Bclock: 0x%04llx, clock: 0x%04llx\n", temp, Bclock, 2 * Bclock);
				}
			}
		}

		if (hit == 0) {
			if (debug_unit) {
				fprintf(debug_stream, "DRAM bank(%d) q_id: %#04x %#04x-%#04x,  ADDR latch: ", bank, temp, DDR_ctrl->list_start_ptr, DDR_ctrl->list_stop_ptr);
				print_xaction2(mem_addr->xaction, debug_stream);
			}
			if (DDR_ctrl->bank[bank].RA == RA && DDR_ctrl->bank[bank].row_valid) {

				DDR_ctrl->list[temp].xaction = mem_addr->xaction;
				DDR_ctrl->list[temp].xaction_id = mem_addr->xaction_id;
				DDR_ctrl->list[temp].addr = mem_addr->addr;
				DDR_ctrl->list[temp].cacheable = mem_addr->cacheable;
				DDR_ctrl->list[temp].clock = Bclock;
				if (mem_addr->cacheable != page_non_cache) {
					if (mem_addr->xaction == bus_allocate || (mem_addr->xaction == bus_store_full || mem_addr->xaction == bus_store_partial)) {
						DDR_ctrl->list[temp].status = ddr_allocate_c;

						if (DDR_ctrl->wc[0].addr_valid && (DDR_ctrl->wc[0].addr & (UINT64)(~0x03f)) == (mem_addr->addr & (UINT64)(~0x03f))) {
							DDR_ctrl->list[temp].wc_ptr = 0;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].data_pending = 1;
						}
						else if (DDR_ctrl->wc[0].addr_valid && (DDR_ctrl->wc[1].addr & (UINT64)(~0x03f)) == (mem_addr->addr & (UINT64)(~0x03f))) {
							DDR_ctrl->list[temp].wc_ptr = 1;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].data_pending = 1;
						}
						else if (DDR_ctrl->wc[0].addr_valid && (DDR_ctrl->wc[2].addr & (UINT64)(~0x03f)) == (mem_addr->addr & (UINT64)(~0x03f))) {
							DDR_ctrl->list[temp].wc_ptr = 2;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].data_pending = 1;
						}
						else if (DDR_ctrl->wc[0].addr_valid && (DDR_ctrl->wc[3].addr & (UINT64)(~0x03f)) == (mem_addr->addr & (UINT64)(~0x03f))) {
							DDR_ctrl->list[temp].wc_ptr = 3;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].data_pending = 1;
						}
						else {
							DDR_ctrl->list[temp].wc_ptr = DDR_ctrl->wc_ptr;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].addr_valid = 1;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].xaction_id = mem_addr->xaction_id;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].addr = mem_addr->addr;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].dirty = 0;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].data_pending = 1;
							DDR_ctrl->wc_ptr = ((DDR_ctrl->wc_ptr + 1) & 3);
						}
						if (debug_unit) {
							fprintf(debug_stream, " mWC(%d),", DDR_ctrl->list[temp].wc_ptr);
						}
					}
					else if (mem_addr->xaction == bus_LR_aq_rl) {
						DDR_ctrl->list[temp].status = ddr_read_fence;
					}
					else {
						DDR_ctrl->list[temp].status = ddr_read;
					}
				}
				else {
					if (mem_addr->xaction == bus_store_full || mem_addr->xaction == bus_store_partial) {
						DDR_ctrl->list[temp].status = ddr_allocate0;

						if (DDR_ctrl->wc[0].addr_valid && (DDR_ctrl->wc[0].addr & (UINT64)(~0x07f)) == (mem_addr->addr & (UINT64)(~0x07f))) {
							DDR_ctrl->list[temp].wc_ptr = 0;
							hit = 1;
						}
						else if (DDR_ctrl->wc[1].addr_valid && (DDR_ctrl->wc[1].addr & (UINT64)(~0x07f)) == (mem_addr->addr & (UINT64)(~0x07f))) {
							DDR_ctrl->list[temp].wc_ptr = 1;
							hit = 1;
						}
						else if (DDR_ctrl->wc[2].addr_valid && (DDR_ctrl->wc[2].addr & (UINT64)(~0x07f)) == (mem_addr->addr & (UINT64)(~0x07f))) {
							DDR_ctrl->list[temp].wc_ptr = 2;
							hit = 1;
						}
						else if (DDR_ctrl->wc[3].addr_valid && (DDR_ctrl->wc[3].addr & (UINT64)(~0x07f)) == (mem_addr->addr & (UINT64)(~0x07f))) {
							DDR_ctrl->list[temp].wc_ptr = 3;
							hit = 1;
						}
						else {
							DDR_ctrl->list[temp].wc_ptr = DDR_ctrl->wc_ptr;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].addr_valid = 1;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].xaction_id = mem_addr->xaction_id;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].addr = mem_addr->addr;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].dirty = 0;
							DDR_ctrl->wc_ptr = ((DDR_ctrl->wc_ptr + 1) & 3);
						}
						DDR_ctrl->wc[DDR_ctrl->list[temp].wc_ptr].data_pending = 1;
						if (debug_unit) {
							fprintf(debug_stream, " mWC(%d)", DDR_ctrl->list[temp].wc_ptr);
						}
						if (hit) {
							hit = 0;
							for (UINT8 current = DDR_ctrl->list_start_ptr; current != DDR_ctrl->list_stop_ptr && !hit; current = ((current + 1) & (DDR_ctrl->list_count - 1))) {
								if ((DDR_ctrl->list[current].xaction_id == (mem_addr->xaction_id & 0xfeff)) && DDR_ctrl->list[current].status == ddr_allocate2) {
									if ((DDR_ctrl->list[current].addr & (UINT64)(~0x007f)) == (mem_addr->addr & (UINT64)(~0x007f)))
										DDR_ctrl->list[current].status == ddr_allocate3;
								}
							}
						}
					}
					else {
						DDR_ctrl->list[temp].status = ddr_read;
					}
				}
				DDR_ctrl->list_stop_ptr = (DDR_ctrl->list_stop_ptr + 1) & (DDR_ctrl->list_count - 1);
				if (debug_unit) {
					fprintf(debug_stream, " RA hit, latch addr: 0x%016I64x, xaction id %#06x, ", mem_addr->addr, mem_addr->xaction_id);
				}
			}
			else {// should fix duplication with function call, make hit, miss a parameter that is passed
				if (debug_unit) {
					fprintf(debug_stream, " RA miss, latch addr ");
					fprint_addr_coma(debug_stream, mem_addr->addr, param);
					fprintf(debug_stream, " xaction id %#06x, list_ptr: %#04x, ", mem_addr->addr, mem_addr->xaction_id, DDR_ctrl->list_stop_ptr);
				}
				DDR_ctrl->list[temp].xaction = mem_addr->xaction;
				DDR_ctrl->list[temp].xaction_id = mem_addr->xaction_id;
				DDR_ctrl->list[temp].addr = mem_addr->addr;
				DDR_ctrl->list[temp].cacheable = mem_addr->cacheable;
				DDR_ctrl->list[temp].clock = Bclock;
				if (mem_addr->cacheable != page_non_cache) {
					if (mem_addr->xaction == bus_allocate) {
						DDR_ctrl->list[temp].status = ddr_allocate_c;
						if (DDR_ctrl->wc[0].addr_valid && (DDR_ctrl->wc[0].addr & (UINT64)(~0x007f)) == (mem_addr->addr & (UINT64)(~0x007f))) {
							DDR_ctrl->list[temp].wc_ptr = 0;
						}
						else if (DDR_ctrl->wc[0].addr_valid && (DDR_ctrl->wc[1].addr & (UINT64)(~0x007f)) == (mem_addr->addr & (UINT64)(~0x007f))) {
							DDR_ctrl->list[temp].wc_ptr = 1;
						}
						else if (DDR_ctrl->wc[0].addr_valid && (DDR_ctrl->wc[2].addr & (UINT64)(~0x007f)) == (mem_addr->addr & (UINT64)(~0x007f))) {
							DDR_ctrl->list[temp].wc_ptr = 2;
						}
						else if (DDR_ctrl->wc[0].addr_valid && (DDR_ctrl->wc[3].addr & (UINT64)(~0x007f)) == (mem_addr->addr & (UINT64)(~0x007f))) {
							DDR_ctrl->list[temp].wc_ptr = 3;
						}
						else {
							DDR_ctrl->list[temp].wc_ptr = DDR_ctrl->wc_ptr;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].addr_valid = 1;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].xaction_id = mem_addr->xaction_id;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].addr = mem_addr->addr;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].dirty = 0;
							DDR_ctrl->wc_ptr = ((DDR_ctrl->wc_ptr + 1) & 3);
						}
						if (debug_unit) {
							fprintf(debug_stream, "mWC(%d), ", DDR_ctrl->list[temp].wc_ptr);
						}
						DDR_ctrl->wc[DDR_ctrl->wc_ptr].data_pending = 1;
					}
					else if (mem_addr->xaction == bus_LR_aq_rl) {
						DDR_ctrl->list[temp].status = ddr_read_fence;
					}
					else {
						DDR_ctrl->list[temp].status = ddr_read;
					}
				}
				else {
					if (mem_addr->xaction == bus_store_full || mem_addr->xaction == bus_store_partial) {
						DDR_ctrl->list[temp].status = ddr_allocate0;

						if (DDR_ctrl->wc[0].addr_valid && (DDR_ctrl->wc[0].addr & (UINT64)(~0x007f)) == (mem_addr->addr & (UINT64)(~0x007f))) {
							DDR_ctrl->list[temp].wc_ptr = 0;
						}
						else if (DDR_ctrl->wc[1].addr_valid && (DDR_ctrl->wc[1].addr & (UINT64)(~0x007f)) == (mem_addr->addr & (UINT64)(~0x007f))) {
							DDR_ctrl->list[temp].wc_ptr = 1;
						}
						else if (DDR_ctrl->wc[2].addr_valid && (DDR_ctrl->wc[2].addr & (UINT64)(~0x007f)) == (mem_addr->addr & (UINT64)(~0x007f))) {
							DDR_ctrl->list[temp].wc_ptr = 2;
						}
						else if (DDR_ctrl->wc[3].addr_valid && (DDR_ctrl->wc[3].addr & (UINT64)(~0x007f)) == (mem_addr->addr & (UINT64)(~0x007f))) {
							DDR_ctrl->list[temp].wc_ptr = 3;
						}
						else {
							DDR_ctrl->list[temp].wc_ptr = DDR_ctrl->wc_ptr;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].addr_valid = 1;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].xaction_id = mem_addr->xaction_id;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].addr = mem_addr->addr;
							DDR_ctrl->wc[DDR_ctrl->wc_ptr].dirty = 0;
							DDR_ctrl->wc_ptr = ((DDR_ctrl->wc_ptr + 1) & 3);
						}
						DDR_ctrl->wc[DDR_ctrl->list[temp].wc_ptr].data_pending = 1;
						if (debug_unit) {
							fprintf(debug_stream, " mWC(%d)", DDR_ctrl->list[temp].wc_ptr);
						}
					}
					else {
						DDR_ctrl->list[temp].status = ddr_read;
					}
				}
				DDR_ctrl->list_stop_ptr = (DDR_ctrl->list_stop_ptr + 1) & (DDR_ctrl->list_count - 1);
			}
			if (debug_unit) {
				fprintf(debug_stream, "Bclock: 0x%04llx, clock: 0x%04llx\n", Bclock, 2 * Bclock);
			}
		}
	}
	if (DDR_ctrl->list_start_ptr == ((DDR_ctrl->list_stop_ptr + 1) & (DDR_ctrl->list_count - 1))) {
		debug++;
		exit(0);
	}
}