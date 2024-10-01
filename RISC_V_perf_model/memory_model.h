// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#pragma once

#include "internal_bus.h"

enum DDR_status_type :UINT8 {//bit 0 - valid, 1-write, 2-allocate
	ddr_idle = 0x00,
	// read state machine
	ddr_read = 0x01,			// return data to core only
	ddr_read2 = 0x21,			// data in flight
	ddr_read_fence = 0x81,			// data in flight
	ddr_read_fence2 = 0xa1,			// data in flight
	// write state machine (uncacheable)
	ddr_allocate0 = 0x05,		// address valid
	ddr_allocate1 = 0x15,		// data latched
	ddr_allocate2 = 0x25,		// data in flight, next step is write
	ddr_allocate3 = 0x35,		// data in flight, do not return data
	ddr_allocate0_lock = 0x85,		// address valid
	ddr_write = 0x045,		// next step is idle
	ddr_write_lock = 0x0c5,		// next step is idle
	// cacheable writes??
	ddr_allocate_c = 0x0d,	// return data to core and write combine buffers
	ddr_allocate_c2 = 0x2d	// return data to core and write combine buffers
	// locked cycles??
};

struct mem_linked_list_node {
	DDR_status_type status;
	UINT16 xaction_id;// 0-6 mhartid, 7=data_code, 8-9 port_id, 12-13 buffer_id
	bus_xaction_type xaction;
	UINT64 addr; // physical address
	page_cache_type cacheable;
	UINT64 data[0x10];

	UINT8 wc_ptr;
	UINT clock;
};
struct mem_wc_buffer {
	UINT16 xaction_id;
	UINT8 addr_valid;
	UINT8 data_pending;
	UINT64 addr;
	UINT dirty; // doing 1 valid bit per 32 bit, should be per 16 bit.
	UINT64 data[0x10];
};
struct DDR_bank_type {
	INT16 RA, CA;// row_address
	UINT8 CA_strobe, CA_strobe_latch;
	UINT8 CA_issue_ptr;
	INT8 row_valid;
	UINT8 delay;
};
struct ddr_data_entry {
	UINT8 valid, write;
	UINT8 current_data, list_ptr;
	UINT64 tag_addr;
};
struct mem_victim {
	UINT64 tag;
	UINT64 data[0x20];

	UINT16 xaction_id; // do not self snoop
};
struct DDR_control_type {
	INT8 bank_ptr;
	DDR_bank_type bank[4];

	mem_linked_list_node list[0x80];
	UINT8 list_start_ptr, list_stop_ptr, list_count;

	UINT8 wc_ptr;
	mem_wc_buffer wc[4];

	UINT8 victim_ptr;
	mem_victim victim[4];

	UINT8 data_valid_count;
	UINT8 data_bus_ptr;
	UINT8 CAS_latency;
	UINT8 CAS_W_latency;
	ddr_data_entry data_bus[0x20];

	//	UINT64 *memory;
};

void memory_controller(addr_bus_type* mem_addr, data_bus_type* mem_data_read, data_bus_type* mem_data_write, UINT8 reset, UINT64 Bclock, DDR_control_type* DDR_ctrl, memory_space_type* memory_space, param_type *param, FILE* debug_stream);