// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "internal_bus.h"

void copy_addr_bus_info(data_bus_type* dest, addr_bus_type* src) {
	dest->xaction_id = src->xaction_id;
	dest->cacheable = src->cacheable;

	dest->valid = 0;
	for (UINT8 i = 0; i < 0x10; i++) dest->data[i] = 0;
}
void copy_addr_bus_info(addr_bus_type* dest, addr_bus_type* src, UINT64 clock) {
	dest->xaction_id = src->xaction_id;

	dest->strobe = src->strobe;
	dest->cacheable = src->cacheable;
	dest->addr = src->addr;
	dest->xaction = src->xaction;
	dest->clock = clock;
}

void copy_data_bus_info(data_bus_type* dest, data_bus_type* src) {
	dest->xaction_id = src->xaction_id;

	dest->cacheable = src->cacheable;
	dest->snoop_response = src->snoop_response;
	dest->valid = src->valid;

	dest->data[0] = src->data[0];
	dest->data[1] = src->data[1];
	dest->data[2] = src->data[2];
	dest->data[3] = src->data[3];
	dest->data[4] = src->data[4];
	dest->data[5] = src->data[5];
	dest->data[6] = src->data[6];
	dest->data[7] = src->data[7];
	dest->data[8] = src->data[8];
	dest->data[9] = src->data[9];
	dest->data[10] = src->data[10];
	dest->data[11] = src->data[11];
	dest->data[12] = src->data[12];
	dest->data[13] = src->data[13];
	dest->data[14] = src->data[14];
	dest->data[15] = src->data[15];
}

void print_xaction(bus_xaction_type xaction, FILE* debug_stream) {
	switch (xaction) {
	case bus_idle:
		fprintf(debug_stream, " xaction: bus_idle,");
		break;
	case bus_prefetch:
		fprintf(debug_stream, " xaction: bus_prefetch,");
		break;
	case bus_fetch:
		fprintf(debug_stream, " xaction: bus_fetch,");
		break;
	case bus_preload:
		fprintf(debug_stream, " xaction: bus_preload,");
		break;
	case bus_load:
		fprintf(debug_stream, " xaction: bus_load,");
		break;
	case bus_store_full:
		fprintf(debug_stream, " xaction: bus_store_full,");
		break;
	case bus_store_partial:
		fprintf(debug_stream, " xaction: bus_store_partial,");
		break;
	case bus_allocate:
		fprintf(debug_stream, " xaction: bus_allocate,");
		break;
	case bus_LR_rl:
		fprintf(debug_stream, " xaction: bus_LR_rl,");
		break;
	case bus_LR_aq:
		fprintf(debug_stream, " xaction: bus_LR_aq,");
		break;
	case bus_LR_aq_rl:
		fprintf(debug_stream, " xaction: bus_LR_aq_rl,");
		break;
	case bus_SC_rl:
		fprintf(debug_stream, " xaction: bus_SC_rl,");
		break;
	case bus_SC_aq:
		fprintf(debug_stream, " xaction: bus_SC_aq,");
		break;
	case bus_SC_aq_rl:
		fprintf(debug_stream, " xaction: bus_SC_aq_rl,");
		break;
	default:
		fprintf(debug_stream, " xaction: unknown xaction,");
		break;
	}
}

void print_xaction2(bus_xaction_type xaction, FILE* debug_stream) {
	switch (xaction) {
	case bus_idle:
		fprintf(debug_stream, "bus_idle");
		break;
	case bus_prefetch:
		fprintf(debug_stream, "PREFETCH");
		break;
	case bus_fetch:
		fprintf(debug_stream, "FETCH");
		break;
	case bus_preload:
		fprintf(debug_stream, "PRELOAD");
		break;
	case bus_load:
		fprintf(debug_stream, "READ");
		break;
	case bus_store_full:
		fprintf(debug_stream, "WRITE FULL");
		break;
	case bus_store_partial:
		fprintf(debug_stream, "WRITE PARTIAL");
		break;
	case bus_allocate:
		fprintf(debug_stream, "ALLOCATE");
		break;
	case bus_LR:
		fprintf(debug_stream, "bus_LR");
		break;
	case bus_LR_rl:
		fprintf(debug_stream, "bus_LR_rl");
		break;
	case bus_LR_aq:
		fprintf(debug_stream, "bus_LR_aq");
		break;
	case bus_LR_aq_rl:
		fprintf(debug_stream, "bus_LR_aq_rl");
		break;
	case bus_SC:
		fprintf(debug_stream, "bus_SC");
		break;
	case bus_SC_rl:
		fprintf(debug_stream, "bus_SC_rl");
		break;
	case bus_SC_aq:
		fprintf(debug_stream, "bus_SC_aq");
		break;
	case bus_SC_aq_rl:
		fprintf(debug_stream, "bus_SC_aq_rl");
		break;
	default:
		fprintf(debug_stream, "unknown xaction");
		break;
	}
}