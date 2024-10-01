// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "compile_to_RISC_V.h"
#include <Windows.h>
#include <Commdlg.h>
#include <WinBase.h>
#include <stdio.h>

void new_page_directory(memory_space_type* memory_space, UINT64 ppn, UINT flags, page_state type) {
	int debug = 0;
	UINT page_index = ppn >> (21 - 9);
	UINT memory_index = ((ppn << 9) & 0x1fffff) >> 2;// int = 2
	if (memory_space->page[page_index].type == page_free) {
		memory_space->page[page_index].type = page_in_use;
		memory_space->page[page_index].memory = (UINT64*)malloc(0x40000 * sizeof(UINT64));
		memory_space->page[page_index].subtype = (page_state*)malloc(0x80000 * sizeof(page_state));
		for (UINT i = 0; i < 0x200; i++) {
			memory_space->page[page_index].memory[memory_index | i] = flags;
			memory_space->page[page_index].subtype[memory_index | i] = type;
		}
		for (UINT i = 0x200; i < 0x40000; i++) {
			memory_space->page[page_index].memory[memory_index | i] = flags;
			memory_space->page[page_index].subtype[memory_index | i] = page_free;
		}
	}
	else if (memory_space->page[page_index].subtype[memory_index] == page_free) {
		for (UINT i = 0; i < 0x200; i++) {
			memory_space->page[page_index].memory[memory_index | i] = flags;
			memory_space->page[page_index].subtype[memory_index | i] = type;
		}
	}
	else {
		debug++;// trying to redefine existing page directory
	}
}

void allocate_page(memory_space_type* memory_space, UINT64 logical_address, UINT64 physical_address, UINT64 page, UINT guxwr, INT64 base, UINT8 RV64_mode, FILE* malloc_file) {
	UINT debug = 0;

	UINT L0 = (logical_address >> 12) & 0x1ff;
	UINT L1 = (logical_address >> 21) & 0x1ff;
	UINT L2 = (logical_address >> 30) & 0x1ff;
	UINT L3 = (logical_address >> 39) & 0x1ff;
	UINT L4 = (logical_address >> 48) & 0x1ff;

	UINT64 base_page = (base - 0x80000000) >> 12;
	UINT64 pte0_entry, pte1_entry, pte2_entry, pte3_entry;
	UINT64 pte3_page = base_page, pte2_page = base_page, pte1_page = base_page, pte0_page = base_page;
	switch (RV64_mode) {
	case 1:// sv32 - embedded (int 32b only)
		debug++;
		break;
	case 8: {// sv39 - desktop
		debug++;
	}
		  break;
	case 9: {// sv48 - server
		page = pte3_page;
		new_page_directory(memory_space, page, 0, page_pte3);

		while (!(memory_space->page[page].type == page_free || (memory_space->page[page].type == page_pte2 && (memory_space->page[pte3_page].memory[2 * L3] & 1)))) page++;
		new_page_directory(memory_space, page, 0, page_pte2);

		pte2_page = page;
		memory_space->page[pte2_page].type = page_pte2;
		pte3_entry = ((0x80000000 + (pte2_page << 12)) >> 2) | 1; // sv39
		memory_space->page[pte3_page].memory[2 * L3] = pte3_entry;
		memory_space->page[pte3_page].memory[2 * L3 + 1] = pte3_entry >> 32;

		while (!(memory_space->page[page].type == page_free || (memory_space->page[page].type == page_pte1 && (memory_space->page[pte2_page].memory[2 * L2] & 1)))) page++;

		if (memory_space->page[page].type == page_free) {
			pte1_page = page;
			new_page_directory(memory_space, page, 0, page_pte1);
			pte2_entry = ((0x80000000 + (pte1_page << 12)) >> 2) | 1; // sv39
			memory_space->page[pte2_page].memory[2 * L2] = pte2_entry;
			memory_space->page[pte2_page].memory[2 * L2 + 1] = pte2_entry >> 32;
		}
		else {
			pte2_entry = (memory_space->page[pte2_page].memory[2 * L2 + 1] << 32) | memory_space->page[pte2_page].memory[2 * L2];
			pte1_page = ((pte2_entry << 2) - 0x80000000) >> 12;
			new_page_directory(memory_space, page, 0, page_pte1);
		}

		while (!(memory_space->page[page].type == page_free)) page++;
		new_page_directory(memory_space, page, 0, page_in_use);

		pte0_page = page;
		pte1_entry = ((0x80000000 + (pte0_page << 12)) >> 2) | 1;
		memory_space->page[pte1_page].memory[2 * L1] = pte1_entry;
		memory_space->page[pte1_page].memory[2 * L1 + 1] = pte1_entry >> 32;

		pte0_entry = ((physical_address & 0xffffffffffe00000) >> 2) | (guxwr << 1) | 1;
		//		for (UINT i = 0; i < 0x200; i++) {
		memory_space->page[pte0_page].memory[0] = pte0_entry | (0 << 10);
		memory_space->page[pte0_page].memory[1] = pte0_entry >> 32;
		fprintf(malloc_file, "logical address: 0x%016I64x, physical address: 0x%016I64x, pte3: 0x%016I64x, 0x%016I64x; pte2: 0x%016I64x, 0x%016I64x; pte1: 0x%016I64x, 0x%016I64x; pte0: 0x%016I64x, 0x%016I64x;\n",
			logical_address + 0 * 0x1000, physical_address + 0 * 0x1000, 0x80000000 + (pte3_page << 12) + (L3 << 3), pte3_entry, 0x80000000 + (pte2_page << 12) + (L2 << 3), pte2_entry, 0x80000000 + (pte1_page << 12) + (L1 << 3), pte1_entry, 0x80000000 + (pte0_page << 12) + (0 << 3), pte0_entry | (0 << 10));
		//		}
	}
		  break;
	case 10:// sv57 - HPC
		break;
	default:
		break;
	}
}

void allocate_pageB(memory_space_type* memory_space, UINT64 logical_address, UINT64 physical_address, UINT64 page, UINT guxwr, INT64 base, FILE* malloc_file) {
	UINT debug = 0;

	UINT8 mode = 9; // need to pull in satp -> compile reset before alloc

	UINT L0 = (logical_address >> 12) & 0x1ff;
	UINT L1 = (logical_address >> 21) & 0x1ff;
	UINT L2 = (logical_address >> 30) & 0x1ff;
	UINT L3 = (logical_address >> 39) & 0x1ff;
	UINT L4 = (logical_address >> 48) & 0x1ff;

	UINT64 base_page = (base - 0x80000000) >> 12;
	UINT64 pte0_entry, pte1_entry, pte2_entry, pte3_entry;
	UINT64 pte3_page = base_page, pte2_page = base_page, pte1_page = base_page, pte0_page = base_page;
	switch (mode) {
	case 1:// sv32 - embedded (int 32b only)
		debug++;
		break;
	case 8: {// sv39 - desktop
		debug++;
	}
		  break;
	case 9: {// sv48 - server
		page = pte3_page;
		new_page_directory(memory_space, page, 0, page_pte3);

		while (!(memory_space->page[page].type == page_free || (memory_space->page[page].type == page_pte2 && (memory_space->page[pte3_page].memory[2 * L3] & 1)))) page++;
		new_page_directory(memory_space, page, 0, page_pte2);

		pte2_page = page;
		memory_space->page[pte2_page].type = page_pte2;
		pte3_entry = ((0x80000000 + (pte2_page << 12)) >> 2) | 1; // sv39
		memory_space->page[pte3_page].memory[2 * L3] = pte3_entry;
		memory_space->page[pte3_page].memory[2 * L3 + 1] = pte3_entry >> 32;

		while (!(memory_space->page[page].type == page_free || (memory_space->page[page].type == page_pte1 && (memory_space->page[pte2_page].memory[2 * L2] & 1)))) page++;

		if (memory_space->page[page].type == page_free) {
			pte1_page = page;
			new_page_directory(memory_space, page, 0, page_pte1);
			pte2_entry = ((0x80000000 + (pte1_page << 12)) >> 2) | 1; // sv39
			memory_space->page[pte2_page].memory[2 * L2] = pte2_entry;
			memory_space->page[pte2_page].memory[2 * L2 + 1] = pte2_entry >> 32;
		}
		else {
			pte2_entry = (memory_space->page[pte2_page].memory[2 * L2 + 1] << 32) | memory_space->page[pte2_page].memory[2 * L2];
			pte1_page = ((pte2_entry << 2) - 0x80000000) >> 12;
			new_page_directory(memory_space, page, 0, page_pte1);
		}

		while (!(memory_space->page[page].type == page_free)) page++;
		new_page_directory(memory_space, page, 0, page_in_use);

		pte0_page = page;
		pte1_entry = ((0x80000000 + (pte0_page << 12)) >> 2) | 1;
		memory_space->page[pte1_page].memory[2 * L1] = pte1_entry;
		memory_space->page[pte1_page].memory[2 * L1 + 1] = pte1_entry >> 32;

		pte0_entry = ((physical_address & 0xfffffffffffff000) >> 2) | (guxwr << 1) | 1;
		memory_space->page[pte0_page].memory[2 * L0] = pte0_entry;// x, r, w??
		memory_space->page[pte0_page].memory[2 * L0 + 1] = pte0_entry >> 32;// x, r, w??
	}
		  break;
	case 10:// sv57 - HPC
		break;
	default:
		break;
	}
	fprintf(malloc_file, "logical address: 0x%016I64x, physical address: 0x%016I64x, pte3: 0x%016I64x, 0x%016I64x; pte2: 0x%016I64x, 0x%016I64x; pte1: 0x%016I64x, 0x%016I64x; pte0: 0x%016I64x, 0x%016I64x;\n",
		logical_address, physical_address, 0x80000000 + (pte3_page << 12) + (L3 << 3), pte3_entry, 0x80000000 + (pte2_page << 12) + (L2 << 3), pte2_entry, 0x80000000 + (pte1_page << 12) + (L1 << 3), pte1_entry, 0x80000000 + (pte0_page << 12) + (L0 << 3), pte0_entry);
}
// data page includes heap or stack space, increment downwards
UINT64 allocate_data_4K_page(memory_space_type* memory_space, UINT64 logical_address, UINT guxwr, INT64 base, UINT8 RV64_mode, FILE* malloc_file) {
	int debug = 0;
	UINT64 physical_address = logical_address;
	UINT64 page = ((logical_address - 0x80000000) >> 12);
	page &= ~0x01ff;
	while (memory_space->page[page].type != page_free && page >= 0) page -= 0x200;
	for (UINT i = 0; i < 0x200; i++) {
		memory_space->page[page | i].type = page_in_use;
		memory_space->page[page | i].memory = (UINT64*)malloc(0x40000 * sizeof(UINT64));
	}
	physical_address = 0x80000000 + (page << 12);
	allocate_page(memory_space, logical_address, physical_address, page, guxwr, base, RV64_mode, malloc_file);

	return physical_address;
}
UINT64 allocate_data_2M_page(memory_space_type* memory_space, UINT64 logical_address, UINT guxwr, INT64 base, UINT8 RV64_mode, FILE* malloc_file) {
	UINT64 physical_address;
	if (logical_address < 0x86400000) {// non-swapable - no page tables available
		physical_address = logical_address;
		UINT64 page_2M = ((logical_address - 0x80000000) >> 21);
		memory_space->page[page_2M].memory = (UINT64*)malloc(0x40000 * sizeof(UINT64));
		fprintf(malloc_file, "logical address: 0x%016I64x, physical address: 0x%016I64x	// 2M non swap memory page; \n",
			logical_address, physical_address);
	}
	else {
		int debug = 0;
		UINT64 ppn = ((logical_address - 0x80000000) >> 21);
		while (memory_space->page[ppn].type != page_free && ppn >= 0) ppn--;
		if (memory_space->page[ppn].type != page_in_use) {
			memory_space->page[ppn].type = page_in_use;
			memory_space->page[ppn].memory = (UINT64*)malloc(0x40000 * sizeof(UINT64));
			memory_space->page[ppn].subtype = (page_state*)malloc(0x40000 * sizeof(page_state));
			for (UINT i = 0; i < 0x40000; i++)
				memory_space->page[ppn].subtype[i] = page_free;
			for (UINT i = 0; i < 0x200; i++)
				memory_space->page[ppn].subtype[i] = page_in_use;
		}
		physical_address = 0x80000000 + (ppn << 21);

		UINT L0 = (logical_address >> 12) & 0x1ff;
		UINT L1 = (logical_address >> 21) & 0x1ff;
		UINT L2 = (logical_address >> 30) & 0x1ff;
		UINT L3 = (logical_address >> 39) & 0x1ff;
		UINT L4 = (logical_address >> 48) & 0x1ff;

		UINT64 base_page = (base - 0x80000000) >> 9;
		UINT64 pte1_entry, pte2_entry, pte3_entry;
		UINT64 pte3_page = base_page, pte2_page = base_page, pte1_page = base_page;
		switch (RV64_mode) {
		case 1:// sv32 - embedded (int 32b only)
			debug++;
			break;
		case 8: {// sv39 - desktop
			debug++;
		}
			  break;
		case 9: {// sv48 - server
			ppn = pte3_page;
			new_page_directory(memory_space, ppn, 0, page_pte3);
			UINT page3_index = ppn >> (21 - 9);
			UINT memory3_index = ((ppn << 9) & 0x1fffff) >> 2;
			UINT memory2_index = memory3_index;
			while (memory_space->page[page3_index].subtype[memory2_index] != page_free &&
				memory_space->page[page3_index].subtype[memory2_index] != page_pte2 && memory2_index < (0x40000 - 0x200))
				memory2_index += 0x200;
			ppn = page3_index << 21;
			ppn |= (memory2_index << 3);
			ppn >>= 9;
			new_page_directory(memory_space, ppn, 0, page_pte2);
			UINT page2_index = ppn >> (21 - 9);
			memory2_index = ((ppn << 9) >> 3) & 0x3ffff;
			memory_space->page[page2_index].subtype[memory2_index] = page_pte2;

			pte3_entry = ((0x80000000 + (ppn << 9)) >> 2) | 1; // sv39
			memory_space->page[page3_index].memory[memory3_index | L3] = pte3_entry;

			UINT memory1_index = memory2_index;
			while (memory_space->page[page2_index].subtype[memory1_index] != page_free &&
				memory_space->page[page2_index].subtype[memory1_index] != page_pte1 && memory1_index < (0x40000 - 0x200))memory1_index += 0x200;
			ppn = page2_index << 21;
			ppn |= (memory1_index << 3);
			ppn >>= 9;

			pte1_page = ppn;
			new_page_directory(memory_space, ppn, 0, page_pte1);
			UINT page1_index = ppn >> (21 - 9);
			memory1_index = ((ppn << 9) >> 3) & 0x3ffff;

			pte2_entry = ((0x80000000 + (ppn << 9)) >> 2) | 1; // sv39
			memory_space->page[page2_index].memory[memory2_index | L2] = pte2_entry;


			pte1_entry = ((physical_address & 0xffffffffffe00000) >> 2) | (guxwr << 1) | 1;
			memory_space->page[page1_index].memory[memory1_index | L1] = pte1_entry | (0 << 10);
			fprintf(malloc_file, "logical address: 0x%016I64x, physical address: 0x%016I64x, pte3: 0x%016I64x, 0x%016I64x; pte2: 0x%016I64x, 0x%016I64x; pte1: 0x%016I64x, 0x%016I64x; \n",
				logical_address, physical_address, 0x80000000 + (page3_index << 21) + ((memory3_index | L3) << 3), pte3_entry,
				(INT64)(0x80000000 + (page2_index << 21) + ((memory2_index | L2) << 3)), pte2_entry, (INT64)(0x80000000 + (page1_index << 21) + ((memory1_index | L1) << 3)), pte1_entry);
		}
			  break;
		case 10:// sv57 - HPC
			break;
		default:
			break;
		}
	}
	return physical_address;
}
