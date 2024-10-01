// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#pragma once

#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

#define io_addr_TLB_code_ctrl		0x10002000
#define io_addr_TLB_data_ctrl		0x10002010
#define io_addr_TLB_code_vaddr		0x10002020
#define io_addr_TLB_data_vaddr		0x10002030
#define io_addr_TLB_4K_code			0x10002040
#define io_addr_TLB_4K_data			0x10002050
#define io_addr_TLB_2M_code			0x10002060
#define io_addr_TLB_2M_data			0x10002070

#define io_addr_L0C_ctrl		0x100020b0
#define io_addr_L0D_ctrl		0x100020c0
#define io_addr_L2_ctrl			0x100020d0

enum snoop_response_type : UINT8 {
	snoop_idle = 0x00,
	snoop_miss = 0x01,			// no data (bit 0 = strobe)
	snoop_hit = 0x03,			// data - exclusive if alloc, shared otherwise (bit 1 = clean data)
	snoop_dirty = 0x05,			// data - modified state (bit 2 = modified data)
	snoop_stall = 0x07			// all active = stall
};// format 1/0
enum bus_xaction_type : UINT8 {// 0 - strobe, 1-write/read, 2-data/code, 3 demand/speculate, 4-Exclusive/Shared, 5-I/O, 6-lock
	bus_idle = 0x00,
	bus_prefetch = 0x01,
	bus_preload = 0x05,
	//	bus_write_chunk = 0x07,// wt v wb handled by cache flags from TLB; early write from write posters??
	bus_fetch = 0x09,
	bus_load = 0x0d, // load data
	bus_store_full = 0x1f,// wt v wb handled by cache flags from TLB; early write from write posters??
	bus_store_partial = 0x0f,// wt v wb handled by cache flags from TLB; early write from write posters??
	bus_allocate = 0x1d,
	//	bus_write_modified = 0x0f,
	// following are all fenced operations, aka forced sync in core

	bus_LR = 0x20, // read IO port, read control table
	bus_LR_rl = 0x21,
	bus_LR_aq = 0x22,
	bus_LR_aq_rl = 0x23,
	bus_SC = 0x24,
	bus_SC_rl = 0x25,
	bus_SC_aq = 0x26,
	bus_SC_aq_rl = 0x27

	// flush cycle??
};
enum control_status_reg_type :UINT16 {
	csr_ustatus = 0x000, // user status register
	csr_fflags = 0x001, // floating point flags
	csr_frm = 0x002, // floating point dynamic rounding
	csr_fcsr = 0x003, // fp control and status register
	csr_uie = 0x004, // user interrupt enable register
	csr_utvec = 0x005, // trap vector

	csr_uscratch = 0x0040,
	csr_uepc = 0x0041, // user exception program counter
	csr_ucause = 0x042,
	csr_utval = 0x043,
	csr_uip = 0x044, // interrupt pending

	// user counters
	csr_cycle = 0xc00, //  cycle counter (RDCYCLE)
	csr_time = 0xc01, //  wall clock counter (RDTIME)
	csr_instret = 0xc02, //  instructions retired counter (RDINSTRET)
	csr_hpmcounter3 = 0xc03, //  preformance monitor counter instr decoded
	csr_hpmcounter4 = 0xc04, // branches retired counter
	csr_hpmcounter5 = 0xc05, // branches decoded counter
	csr_hpmcounter8 = 0xc08, // load retired counter
	csr_hpmcounter9 = 0xc09, // load decoded counter
	csr_hpmcounter10 = 0xc0a, // store retired counter
	csr_hpmcounter11 = 0xc0b, // store decoded counter
	csr_load_issued = 0xc0c,
	csr_alloc_issued = 0xc0d,
	csr_store_issued = 0xc0e,
	csr_amo_issued = 0xc0f,
	// ..
	csr_hpmcounter31 = 0xc1f, //   preformance monitor counter
	csr_cycleh = 0xc80, //  cycle counter
	csr_timeh = 0xc81, //  wall clock counter
	csr_instreth = 0xc82, //  instructions retired counter

	csr_sstatus = 0x100, // supervisor status register
	csr_sedeleg = 0x102,// supervisor delegate exceptions
	csr_sideleg = 0x103,// supervisor delegate interrupts
	csr_sie = 0x104, // supervisor interrupt enable register
	csr_stvec = 0x105, // supervisor trap vector

	csr_sscratch = 0x0140,
	csr_sepc = 0x0141, // supervisor exception program counter
	csr_scause = 0x142,
	csr_stval = 0x143,
	csr_sip = 0x144,

	csr_satp = 0x180,// supervisor page table base address

	// supervisor counters
	csr_scycle = 0xd00, // supervisor cycle counter
	csr_stime = 0xd01, // supervisor wall clock counter
	csr_sinstret = 0xd02, // supervisor instructions retired counter
	csr_shpmcounter3 = 0xd03, //  supervisor  instr decoded
	csr_shpmcounter4 = 0xd04, // supervisor  branches retired counter
	csr_shpmcounter5 = 0xd05, // supervisor  branches decoded counter
	csr_shpmcounter8 = 0xd08, // supervisor  load retired counter
	csr_shpmcounter9 = 0xd09, // supervisor  load decoded counter
	csr_shpmcounter10 = 0xd0a, // supervisor  store retired counter
	csr_shpmcounter11 = 0xd0b, // supervisor  store decoded counter
	csr_sload_issued = 0xd0c,
	csr_salloc_issued = 0xd0d,
	csr_sstore_issued = 0xd0e,
	csr_samo_issued = 0xd0f,
	csr_scycleh = 0xd80, // supervisor cycle counter
	csr_stimeh = 0xd81, // supervisor wall clock counter
	csr_sinstreth = 0xd82, // supervisor enstructions retired counter

	csr_hstatus = 0x200, // hypervisor status register
	csr_hedeleg = 0x202,// hypervisor delegate exceptions
	csr_hideleg = 0x203,// hypervisor delegate interrupts
	csr_hie = 0x204, // hypervisor interrupt enable register
	csr_htvec = 0x205, // hypervisor trap vector

	csr_hscratch = 0x0240,
	csr_hepc = 0x0241, // hypervisor exception program counter 
	csr_hcause = 0x242,
	csr_htval = 0x243,
	csr_hip = 0x244,

	// hypervisor counters
	csr_hcycle = 0xe00, // hypervisor cycle counter
	csr_htime = 0xe01, // hypervisor wall clock counter
	csr_hinstret = 0xe02, // hypervisor instructions retired counter
	csr_hhpmcounter3 = 0xe03, //  hypervisor  instr decoded
	csr_hhpmcounter4 = 0xe04, // hypervisor branches retired counter
	csr_hhpmcounter5 = 0xe05, // hypervisor branches decoded counter
	csr_hhpmcounter8 = 0xe08, // hypervisor load retired counter
	csr_hhpmcounter9 = 0xe09, // hypervisor load decoded counter
	csr_hhpmcounter10 = 0xe0a, // hypervisor store retired counter
	csr_hhpmcounter11 = 0xe0b, // hypervisor store decoded counter
	csr_hload_issued = 0xe0c,
	csr_halloc_issued = 0xe0d,
	csr_hstore_issued = 0xe0e,
	csr_hamo_issued = 0xe0f,
	csr_hcycleh = 0xe80, // hypervisor cycle counter
	csr_htimeh = 0xe81, // hypervisor wall clock counter
	csr_hinstreth = 0xe82, // hypervisor enstructions retired counter

	// machine read/write
	csr_misa = 0xf10,  // ISA and extensions support
	csr_mvendorid = 0xf11,  // vendor id
	csr_marchid = 0xf12,  // architecture id
	csr_mimpid = 0xf13,  // machine implementation id
	csr_mhartid = 0xf14, // machine hardware thread ID. aka boot id

	csr_mstatus = 0x300, // machine status register
	csr_medeleg = 0x302,// machine delegate exceptions
	csr_mideleg = 0x303,// machine delegate interrupts
	csr_mie = 0x304, // machine interrupt enable register
	csr_mtvec = 0x305, // machine trap vector

	csr_mscratch = 0x0340,
	csr_mepc = 0x0341, // machine exception program counter 
	csr_mcause = 0x342,
	csr_mtval = 0x343,
	csr_mip = 0x344,

	csr_mbase = 0x380,
	csr_mbound = 0x381,
	csr_mibase = 0x382,	// machine code base address
	csr_mibound = 0x383,
	csr_mdbase = 0x384,	// machine data base address
	csr_mdbound = 0x385,

	// machine counters
	csr_mcycle = 0xb00, // machine cycle counter
	csr_mtime = 0xb01, // machine wall clock counter
	csr_minstret = 0xb02, // machine instructions retired counter
	csr_mhpmcounter3 = 0xb03, // machine instructions decoded counter
	csr_mhpmcounter4 = 0xb04, // machine branches retired counter
	csr_mhpmcounter5 = 0xb05, // machine branches decoded counter
	csr_mhpmcounter6 = 0xb06, // machine jumps retired counter
	csr_mhpmcounter7 = 0xb07, // machine jumps decoded counter
	csr_mhpmcounter8 = 0xb08, // machine loads retired counter
	csr_mhpmcounter9 = 0xb09, // machine loads decoded counter
	csr_mhpmcounter10 = 0xb0a, // machine stores retired counter
	csr_mhpmcounter11 = 0xb0b, // machine stores decoded counter
	csr_mhpmcounter12 = 0xb0c, // machine faults retired counter
	csr_mhpmcounter13 = 0xb0d, // machine faults decoded counter
	csr_mload_issued = 0xb0c,
	csr_malloc_issued = 0xb0d,
	csr_mstore_issued = 0xb0e,
	csr_mamo_issued = 0xb0f,
	//..
	csr_mhpmcounter31 = 0xb1f, // machine instructions retired counter
	csr_mcycleh = 0xb80, // machine cycle counter
	csr_mtimeh = 0xb81, // machine wall clock counter
	csr_minstreth = 0xb82, // machine enstructions retired counter

	csr_mucounteren = 0x320, // machine user counter enable
	csr_mphmevent3 = 0x323, // machine user counter enable
	csr_mphmevent4 = 0x324, // machine user counter enable
	//..
	csr_mphmevent31 = 0x33f, // machine user counter enable

	// 0x7__ = debugger
	csr_mucycle_delta = 0x700, //cycle counter delta
	csr_mutime_delta = 0x701, //timer counter delta
	csr_muinstret_delta = 0x702, //instret counter delta
	csr_mscycle_delta = 0x704, //scycle counter delta
	csr_mstime_delta = 0x705, //stimer counter delta
	csr_msinstret_delta = 0x706, //sinstret counter delta
	csr_mhcycle_delta = 0x708, //hcycle counter delta
	csr_mhtime_delta = 0x709, //htimer counter delta
	csr_mhinstret_delta = 0x70a, //hinstret counter delta
	csr_mucycle_deltah = 0x780, //upper cycle counter delta
	csr_mutime_deltah = 0x781, //upper timer counter delta
	csr_muinstret_deltah = 0x782, //upper instret counter delta
	csr_mscycle_deltah = 0x784, //upper scycle counter delta
	csr_mstime_deltah = 0x785, //upper stimer counter delta
	csr_msinstret_deltah = 0x786, //upper sinstret counter delta
	csr_mhcycle_deltah = 0x788, //upper hcycle counter delta
	csr_mhtime_deltah = 0x789, //upper htimer counter delta
	csr_mhinstret_deltah = 0x78a, //upper hinstret counter delta

	csr_scounteren = 0x106,
	csr_hcounteren = 0x206,
	csr_mcounteren = 0x306,

	csr_dscratch0 = 0x07b2,
	csr_dscratch1 = 0x07b3,

	csr_dpc = 0x07bc, // machine exception program counter 
	csr_µpc = 0x0f41, // micro-code program counter	

	csr_iobase = 0x7c0,
	csr_sbound = 0x181,

	// machine custom read/write

	csr_pmpcfg0 = 0x3a0, // page memory configuration 
	csr_pmpcfg1 = 0x3a1, // page memory configuration // 32 bit mode only
		csr_pmpcfg2 = 0x3a2, // page memory configuration 
		csr_pmpcfg3 = 0x3a3, // page memory configuration // 32 bit mode only
		csr_pmpcfg4 = 0x3a4, // page memory configuration 
		csr_pmpcfg5 = 0x3a5, // page memory configuration // 32 bit mode only
		csr_pmpcfg6 = 0x3a6, // page memory configuration 
		csr_pmpcfg7 = 0x3a7, // page memory configuration // 32 bit mode only
		csr_pmpcfg8 = 0x3a8, // page memory configuration 
		csr_pmpcfg9 = 0x3a9, // page memory configuration // 32 bit mode only
		csr_pmpcfg10 = 0x3a10, // page memory configuration 
		csr_pmpcfg11 = 0x3a11, // page memory configuration // 32 bit mode only
		csr_pmpcfg12 = 0x3a12, // page memory configuration 
		csr_pmpcfg13 = 0x3a13, // page memory configuration // 32 bit mode only
		csr_pmpcfg14 = 0x3a14, // page memory configuration 
		csr_pmpcfg15 = 0x3a15, // page memory configuration // 32 bit mode only

	csr_pmpaddr0 = 0x3b0, // page memory protection address reg
	csr_pmpaddr1 = 0x3b1, // page memory protection address reg
	csr_pmpaddr2 = 0x3b2, // page memory protection address reg
	csr_pmpaddr3 = 0x3b3, // page memory protection address reg
	csr_pmpaddr4 = 0x3b4, // page memory protection address reg
	csr_pmpaddr5 = 0x3b5, // page memory protection address reg
	csr_pmpaddr6 = 0x3b6, // page memory protection address reg
	csr_pmpaddr7 = 0x3b7, // page memory protection address reg
	csr_pmpaddr8 = 0x3b8, // page memory protection address reg
	csr_pmpaddr9 = 0x3b9, // page memory protection address reg
	csr_pmpaddr10 = 0x3ba, // page memory protection address reg
	csr_pmpaddr11 = 0x3bb, // page memory protection address reg
	csr_pmpaddr12 = 0x3bc, // page memory protection address reg
	csr_pmpaddr13 = 0x3bd, // page memory protection address reg
	csr_pmpaddr14 = 0x3be, // page memory protection address reg
	csr_pmpaddr15 = 0x3bf, // page memory protection address reg
	csr_pmpaddr16 = 0x3c0, // page memory protection address reg

	csr_pmpaddr32 = 0x3d0, // page memory protection address reg

	csr_pmpaddr48 = 0x3e0, // page memory protection address reg

	csr_pmpaddr63 = 0x3ef, // page memory protection address reg
};
enum page_cache_type :UINT8 {// r- read, w - write, x - executable; omite user v global, etc
	page_invalid = 0,		//page fault
	page_pointer = 1,		//do not cache
	page_non_cache = 2,		//do not cache
	page_r = 3,				//dl0
	page_reserved_1 = 5,
	page_IO = 4,			// IO cycyle
	page_IO_error_rsp = 8,	// IO error response
	page_rw = 7,			//dl0
	page_x = 9,				//cl0
	page_rx = 11,			//dl0 & dl0
	page_reserved_2 = 0x0d,
	page_cache_no_swap = 0x0e,
	page_rwx = 0x0f			//dl0 & dl0
};

struct param_type {
	UINT misa;
	UINT8 mxl;//3-128b, 2-64b, 1-32b, 0-16b
	UINT64 satp;
	UINT64 start_time, stop_time;
	UINT core;
	UINT8 prefetcher, decoder, allocator, store, store_bus, load, load_bus, csr, intx, imul, fp, branch;
	UINT8 caches, L0C, L0D, L2, L3, mem;
	UINT8 TLB, PAGE_WALK, PAGE_FAULT, EXT_SNOOP;
	UINT align, cache_line, decode_width, fmac;
};
struct addr_bus_type {
	UINT16 xaction_id;// 0-5 mhartid, 6-7:0=snoop,1=code,2=alloc,3=store, 8-9 port_id, 12-13 buffer_id, 10-11 index(branch prediction/speculation assist), 14-15 write dissambiguity
	UINT8 strobe;
	page_cache_type cacheable;
	UINT64 addr;
	bus_xaction_type xaction;
	UINT64 clock;
};
struct data_queue_type {
	UINT16 xaction_id;// 0-5 mhartid, 6-7:0=snoop,1=code,2=alloc,3=store, 8-9 port_id, 12-13 buffer_id, 10-11 index(branch prediction/speculation assist), 14-15 write dissambiguity
	page_cache_type cacheable;
	snoop_response_type snoop_response;
	UINT valid;// snoop: 1 per byte, 5bit code || TLB : valid_bit values:  bit 0 = read, 1 = write, 2 = execute, 3 = user, 4 global, 5 = dirty
	UINT64 data[4];// represents 128b bus, double pumped
};
struct data_bus3_type {
	//	UINT64 data[0x10];
	UINT8 ptr;
	data_queue_type q[8];// cache line transfer in 4 control logic clocks (8 data clocks)
};

struct data_bus_type {
	snoop_response_type snoop_response;
	UINT16 xaction_id;// 0-5 mhartid, 6-7:0=snoop,1=code,2=alloc,3=store, 8-9 port_id, 12-13 buffer_id, 10-11 index(branch prediction/speculation assist), 14-15 write dissambiguity
	page_cache_type cacheable;
	UINT64 valid;// snoop: 1 per byte, 5bit code || TLB : valid_bit values:  bit 0 = read, 1 = write, 2 = execute, 3 = user, 4 global, 5 = dirty
	UINT64 data[0x10];
};
struct addr_bus_signal {
	addr_bus_type* in, * out;
};
struct addr_bus_signal1 {
	addr_bus_type in, out;
};
struct data_bus_signal {
	data_bus_type* in, * out; // in and out are in reference to the unit, not the core
};
struct data_bus_signal1 {
	data_bus_type in, out; // in and out are in reference to the unit, not the core
};

struct internal_bus_signal {
	UINT8 size;
	addr_bus_signal addr; // wrong place to split out array
	data_bus_signal data;
};
struct internal_bus_signal1 {
	addr_bus_signal1 addr; // wrong place to split out array
	data_bus_signal1 data;
};

struct bus_w_snoop_signal1 {
	addr_bus_signal1 addr, snoop_addr; // wrong place to split out array
	data_bus_signal1 data_read, data_write;//  proximal = towards core, distal = away from core
};
// register structures
enum reg_state_type :UINT8 {
	reg_free = 0,
	reg_allocated = 1,
	reg_valid_in = 2,
	reg_valid_out = 3,
	reg_valid_out2 = 4
};
#define reg_rename_size 16
struct reg_bus {
	UINT64 data_H, data;
	UINT8 strobe, ROB_id;
};
struct x_reg_type {
	UINT64 data_H[reg_rename_size];
	UINT64 data[reg_rename_size];
	UINT16 ROB_ptr[reg_rename_size];
	reg_state_type valid[reg_rename_size];
	char name[0x10];
	UINT8 current_ptr, retire_ptr;
};
struct f_reg_type {
	UINT64 data[reg_rename_size];
	UINT64 data_H[reg_rename_size];// to support 128b fp
	reg_state_type valid[reg_rename_size];
	UINT8 current_ptr, retire_ptr;
};
struct Reg_Table_Type {	// control/performance registers
	x_reg_type x_reg[0x20];// x_reg[0] defined as always equal to 0; x_reg[1] = return address, x_reg[2] = stack pointer, x_reg[3] = global pointer, x_reg[5] = alternate return address
	f_reg_type f_reg[0x20];
};
struct csr_type {
	UINT64 value;
};
enum page_state :UINT8 {
	page_free = 0,
	page_pte1 = 1,
	page_pte2 = 2,
	page_pte3 = 3,
	page_in_use = 16
};
struct page_4K_type {
	page_state type;
	UINT64* memory;//indexed in
	page_state* subtype;
};
struct memory_space_type {
	page_4K_type* reset;
	page_4K_type* clic;
	page_4K_type* page;
};
/*
struct swap_address_entry {
	UINT64 logical, physical;
};
/**/
void copy_addr_bus_info(data_bus_type* dest, addr_bus_type* src);
void copy_addr_bus_info(addr_bus_type* dest, addr_bus_type* src, UINT64 clock);
void copy_data_bus_info(data_bus_type* dest, data_bus_type* src);

void print_xaction(bus_xaction_type xaction, FILE* debug_stream);
void print_xaction2(bus_xaction_type xaction, FILE* debug_stream);

void load_params(param_type* param, const char* src_name);

void fprint_addr_coma(FILE* debug_stream, UINT64 addr, param_type* param);
void fprint_addr(FILE* debug_stream, UINT64 addr, param_type* param);