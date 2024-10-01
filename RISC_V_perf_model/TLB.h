// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#pragma once

#include "internal_bus.h"
typedef struct {
	addr_bus_type latch_addr;
	data_bus_type latch_data;
	UINT reg; // control register: bit 0 = enable; 4 = L2 hit
}TLB_walk_type;

typedef struct {
	UINT8 type; //0-4k; 1-2M; 2-1G
	UINT64 directory, directory_h; // page table content
	UINT64 l_addr, l_addr_h;	// contains logical (virtual) address >>12
	UINT64 p_addr, p_addr_h;	// contains physical address >>12
	UINT64 time;	// time when set (debugging purposes only
}TLB_cache_way_type;// table walk in TLB proper - TLB cache entries are translated for speed
typedef struct {
	UINT way_ptr;
	TLB_cache_way_type way[8];
}TLB_cache_entry_type;
typedef struct {
	UINT8 sm[4];
	UINT index_count;
	UINT index_mask;
	addr_bus_type L2_addr_latch;
	TLB_cache_entry_type* entry;
	UINT way_count;
}TLB_cache_type;// table walk in TLB proper - TLB cache entries are translated for speed
typedef struct {
	UINT8 sm[4];
	UINT way_ptr, way_count;
	TLB_cache_way_type* way;
}fully_assoc_TLB_cache_type;// table walk in TLB proper - TLB cache entries are translated for speed

typedef struct {
	TLB_cache_way_type L0[8];// 8 entry - 8 way
	TLB_cache_way_type L1[0x40];// 8 entry - 8 way
	UINT8 way_ptr0, way_ptr1;
	TLB_cache_entry_type L2[0x80];//512 entry - 8 way (64 sets)

	data_bus_type delay;
	UINT8 L2_set;
	TLB_walk_type walk;
	UINT8 enabled;
	UINT8 lock;

	addr_bus_type reg_update_addr_latch;// io reg access
	data_bus_type reg_update_data_latch;// io reg access

}TLB_type;
typedef struct {
	addr_bus_type latch_addr;
	data_bus_type latch_data;
	UINT io_cycle; // 0: memory mapped, 1: IO mapped
	UINT8 addr_sent, data_sent;

	UINT reg; // control register; bit 0 = enable code, 4 = cL2 hit, 8 = enable data, 0x0c = dL2 hit
	UINT8 reg_access_out, reg_access_in;// 0 = inactive, 1 = hold, 2 = active
	UINT8 est_lockout, lockout;
	UINT8 lock;
}sTLB_walk_type;

typedef struct {
	data_bus_type delay;
	fully_assoc_TLB_cache_type cL1;
	fully_assoc_TLB_cache_type dL1;
	sTLB_walk_type walk;

	addr_bus_type snoop_latch[4];// postponed (inactive) snoop
	addr_bus_type ext_snoop_track[4];// active snoop
	UINT8 ext_snoop_track_issued[4];
	UINT8 snoop_latch_ptr, ext_snoop_ptr;
}sTLB_type;

//void tlb_overhead(TLB_type* TLB);
void data_tlb_unit(addr_bus_type* p_addr_bus, data_bus_type* l_data_bus, addr_bus_type* l_addr_bus, bus_w_snoop_signal1* bus2, UINT64 clock, TLB_type* TLB, UINT mhartid, param_type *param, FILE* debug_stream);
void code_tlb_unit(data_bus_type* TLB_response, addr_bus_type* p_addr_bus, addr_bus_type* l_addr_bus, bus_w_snoop_signal1* bus2, UINT64 clock, TLB_type* TLB, UINT mhartid, param_type *param, FILE* debug_stream);
void shadow_tlb_unit(bus_w_snoop_signal1* bus2, addr_bus_type* snoop_L2, bus_w_snoop_signal1* bus3, UINT64 clock, sTLB_type* TLB, UINT mhartid, UINT debug_core, param_type *param, FILE* debug_stream);