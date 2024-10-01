// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#pragma once
#include "internal_bus.h"

enum cache_state_type : UINT8 {
	cache_idle = 0,
	cache_hit = 1,
	cache_miss = 2, // reserved for data
	cache_data = 3, // data available
	cache_tag_lookup = 4, // tag results pending
	cache_complete = 5
};

enum cache_line_state_type :UINT8 {
	invalid_line = 0,
	shared_line = 1,
	exclusive_line = 2,
	modified_line = 4
};
enum ExternalSnoopResponse :UINT8 {
	ExternalSnoop_WaitCodeData = 0,
	ExternalSnoop_WaitCode = 2,
	ExternalSnoop_WaitData = 1,
	ExternalSnoop_Code = 1,
	ExternalSnoop_Data = 2,
	ExternalSnoop_ready = 3,
};

struct cache_line_type {
	cache_line_state_type state;// bit0-valid, bit1-dirty		?? allocate bit to free up way while waiting for data
	UINT64 tag;
	UINT64 line[0x10];
};
struct delay_line_type {
	cache_line_state_type state;// bit0-valid, bit1-dirty		?? allocate bit to free up way while waiting for data
	UINT64 line[0x10];
	UINT xaction_id;
	UINT64 addr;
};

struct cache_8way_type {
	UINT8 way_ptr, dirty, in_use;
	cache_line_type way[8];
};
enum link_status_type :UINT8 {
	link_free = 0,
	link_waiting_snoop_response = 1,
	link_fill_waiting_snoop_response = 7,
	link_issued_forward = 2,
	link_issued_forward1 = 12,
	link_issued_forward2 = 22,
	link_issued_forward_stop = 9,
	link_write_addr = 10,
	link_hold_C = 3, // L3: awaiting tag look-up and external snoop
	link_hold = 4, // L3: external snoop complete
	link_hold_w = 8, // L3: write from L2 to L3, array delay
	link_cache_array_write_pend = 20, // cache array write pending, then retire
	link_cache_array_read_pend = 21, // cache array read pending, then retire
	link_q_hit = 5,
	link_retire = 6
};
struct cache_addr_linked_list_type {
	link_status_type status;
	addr_bus_type latch;
	data_bus_type data;
	UINT snoop_complete; // external snoop, not L3 lookup
	UINT8 addr_sent;
};
struct cache_data_linked_list_node {
	link_status_type status;
	INT64 addr;
	data_bus_type latch;
	bus_xaction_type xaction;
	UINT64 time;// time when set
	ExternalSnoopResponse external_snoop;
};

struct L0_code_cache_type {
	cache_8way_type* entry;
	addr_bus_type l_addr_latch, p_addr_latch, ctrl_addr;
	//	data_bus_type l_data_latch, bus2_data_read;
	data_bus_type  bus2_data_read;
	data_bus_type store_data_latch[4];
	UINT8 store_data_latch_start, store_data_latch_stop;
	UINT8 store_data_latch_ptr;

	UINT reg; // control reg

	cache_addr_linked_list_type list[0x20];// contains actual content
	UINT8 list_start_ptr, list_stop_ptr;
};
struct L0_data_cache_type {
	cache_8way_type* entry;

	data_bus_type write_data_fifo[4];

	addr_bus_type l_addr_latch[4], p_addr_latch[4], ctrl_addr, delayed_addr_latch;
	data_bus_type l_data_latch[4], bus2_data_read;
	data_bus_type store_data_latch[4], delayed_bus2write[8];
	UINT8 l_addr_latch_stall[4];

	UINT8 id_in_use[8];
	UINT8 store_data_latch_start, store_data_latch_stop;
	UINT8 delayed_bus2write_start, delayed_bus2write_stop;
	UINT8 store_data_latch_ptr;

	UINT reg;// control register: bit 0: enable, bit 1 flush; bit 2: lock

	cache_addr_linked_list_type read_list[4], alloc_list[4], write_list[4];// contains actual content
};
#define core_count 16

struct delay_line {
	UINT8 strobe;
	UINT8 ptr;// ptr to link list entry
};

struct cache_type {
	delay_line tag_delay[4];// array_delay[0x10];

	delay_line_type flight_delay;// one buffer per bank

	cache_8way_type* entry;//2M cache is too large, must use malloc

	data_bus_type data_bus_latch[2][4];// port, clock
	addr_bus_type addr_bus_latch[2][4];// port, clock

	UINT8 active_addr_queue[0x20], active_addr_start, active_addr_end;
	UINT8 hold_addr_queue[0x20], hold_addr_start, hold_addr_end;
	UINT8 data_queue[4], data_start, data_end;

	cache_data_linked_list_node data_list[0x22];
	cache_addr_linked_list_type addr_list[0x22];
	UINT8  active, reserve, d_active, d_reserve;
};
struct L2_2MB_type {
	UINT64*** line; //line[8][0x800][0x10];//way, set
	INT16 select[8];//strobe = 0x8000, way[8] = 0x7000,set[0x800] = 0x07fff, 0x0800 = write
	INT16 xaction_id_in[8];
	snoop_response_type snoop_response[8];
	data_bus_type data[8];
	UINT8 read_write; // read = 0, write = 1
	UINT8 array_busy_snoop_stall; // 1 = set; continue snoop stall until array is clear of updates, for IO cycles
};
struct L2_tag_type {//line[8][0x800][0x10];//way,set,line (in 64b / 8B size)
	cache_line_state_type state[8];
	UINT64 tag[8];
	UINT8 way_ptr;
	UINT8 in_use;
};
struct L2_array_type {
	cache_8way_type* set_list;//2M cache is too large, must use malloc
	UINT64 in_addr[4];
	data_bus_type read_q[0x10], write_q[0x10], in_latch[4];
	UINT8 read_q_ptr, read_q_delay, write_q_ptr, write_q_delay;
	UINT8 in_use; // for L3 banks only
};
struct L2_block_type {
	L2_2MB_type array;
	L2_tag_type* tags; // tags[0x800];set.way
	UINT8 set_shift, tag_shift, in_use;
};
struct L2_cache_type {
	L2_block_type bank;

	//	L2_array_type array;
	//	delay_line tag_delay[4], array_delay[0x10];
	UINT8 flush;
	UINT8 enabled;

	// data bus 2 trackers
	cache_data_linked_list_node data_write_list[8];
	cache_data_linked_list_node snoop_write_list[core_count];
	UINT8 write_count;

	cache_data_linked_list_node data_read_list[0x20];
	UINT8  data_r_start, data_r_stop;

	// addr bus 2 trackers
	cache_addr_linked_list_type addr_write_list[8];// bus2 write cycle
	cache_addr_linked_list_type addr_load_list[8];
	cache_addr_linked_list_type addr_alloc_list[8];
	cache_addr_linked_list_type addr_fetch_list[0x40];

	//	ExternalSnoopResponse external_snoop;
};
struct bus3_tracker_type {
	cache_addr_linked_list_type* list;
	UINT start, stop, round_count;
};

struct banked_cache_type {//line[8][x][0x800][0x10];//way,bank (x= # banks = core count), set,line (in 64b / 8B size)
	L2_block_type bank[core_count];
	data_bus_type data_read_fifo[core_count][core_count];// 8 core busses, 8 banks; for Bclock synch
	data_bus_type data_write_fifo[core_count][core_count];// 8 core busses, ??; data write to cache array
	addr_bus_type mem_addr_out[4 * core_count];// 8 cores, 4 fetches per prefetcher
	data_bus_type mem_data_write_fifo[core_count];

	bus3_tracker_type bus3_tracker[core_count];
	UINT8 active_core; /// for round robin core support - avoid deadlocks

	cache_data_linked_list_node data_write_list[0x80];
	cache_data_linked_list_node data_read_list[0x80];
	UINT8 snoop_active[core_count];
	UINT8 data_read_start_ptr, data_read_end_ptr, data_read_count = 0x80;
	UINT8 data_write_start_ptr, data_write_end_ptr, data_write_count = 0x80;
};
void L0_data_cache_unit(data_bus_type* physical_bus_data, bus_w_snoop_signal1* bus2, addr_bus_type* logical_addr, data_bus_type* store_data, addr_bus_type* physical_addr, data_bus_type* TLB_response, UINT mhartid,
	UINT64 clock, L0_data_cache_type* L0_cache, L2_cache_type* L2_cache_var, UINT debug_core, param_type* param, FILE* debug_stream);
void L0_code_cache_unit(data_bus_type* physical_bus, bus_w_snoop_signal1* bus2, addr_bus_type* logical_addr, addr_bus_type* physical_addr, data_bus_type* TLB_response, L0_code_cache_type* L0_cache, UINT64 clock,
	UINT debug_core, UINT mhartid, param_type *param, FILE* debug_stream);
void L2_2MB_cache(bus_w_snoop_signal1* bus2, bus_w_snoop_signal1* bus3, addr_bus_type* snoop_L2, UINT mhartid, UINT64 clock, L2_cache_type* cache_var, UINT8 L0_present, UINT debug_core, param_type *param, FILE* debug_stream);
void L3_cache_unit(addr_bus_type* mem_addr_out, bus_w_snoop_signal1* bus3, data_bus_type* mem_data_write, data_bus_type* mem_data_read, UINT8 reset, UINT64 clock, banked_cache_type* L3_cache, UINT8* exit_flag, param_type *param, FILE* debug_streams);

void copy_addr_info(addr_bus_type* dest, addr_bus_type* src);
UINT8 write_to_array(L2_block_type* L2, UINT64 addr, UINT8 alloc, data_bus_type* data_in, UINT64 clock, UINT mhartid, UINT debug_unit, char* header, FILE* debug_stream);
//snoop_response_type cache_lookup(L2_block_type* L2, INT64 addr, UINT64 clock, char* header, UINT16 xaction_id, UINT debug_unit, FILE* debug_stream);
UINT8 clock_bank_array_path(L2_block_type* bank, UINT64 clock, char* header, UINT8 debug_unit, FILE* debug_stream);