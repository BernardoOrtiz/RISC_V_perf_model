// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#include "ROB.h"

void print_uop(FILE* debug_stream, ROB_entry_Type entry, Reg_Table_Type* reg_table, UINT clock) {
	int debug = 0;
	switch (entry.uop) {
	case uop_JAL:
		fprintf(debug_stream, "JAL xr%02d.%x, offset = %#010x", entry.rd, entry.rd_ptr, ((entry.imm >> 11) & 0xfff00000) | (entry.rd_ptr, entry.imm & 0x000ff000) | ((entry.imm >> 20) & 0x7fe) | ((entry.imm >> 20) & 0x800));
		break;
	case uop_JALR:
		fprintf(debug_stream, "JALR xr%02d.%x, xr%02d.%x, %#010x ", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.imm);
		break;
	case uop_LUI:
		fprintf(debug_stream, "LUI xr%02d.%x, 0x%016I64x ", entry.rd, entry.rd_ptr, (INT64)entry.imm);
		break;
	case uop_AUIPC:
		fprintf(debug_stream, "AUIPC xr%02d.%x,  %#010x ", entry.rd, entry.rd_ptr, entry.imm);
		break;
	case uop_BEQ:
		fprintf(debug_stream, "BEQ xr%02d, xr%02d, 0x%03x ", entry.rs1, entry.rs2, entry.imm);
		break;
	case uop_BNE:
		fprintf(debug_stream, "BNE xr%02d.%x, xr%02d.%x, 0x%03x ", entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr, entry.imm);
		break;
	case uop_BGE:
		fprintf(debug_stream, "BGE xr%02d, xr%02d, 0x%03x ", entry.rs1, entry.rs2, entry.imm);
		break;
	case uop_BLT:
		fprintf(debug_stream, "BLT xr%02d, xr%02d, 0x%03x ", entry.rs1, entry.rs2, entry.imm);
		break;
	case uop_STORE:// need to work on funct3
		fprintf(debug_stream, "STORE xr%02d.%x, ", entry.rs2, entry.rs2_ptr);
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(reg%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_LOAD:// need to work on funct3
		//		fprintf(debug_stream, "LOAD xr%02d.%01d,  ", entry.rd, entry.rd_ptr);
		switch (entry.funct3) {
		case 0:
			fprintf(debug_stream, "LB xr%02d.%01d,  ", entry.rd, entry.rd_ptr);
			break;
		case 1:
			fprintf(debug_stream, "LH xr%02d.%01d,  ", entry.rd, entry.rd_ptr);
			break;
		case 2:
			fprintf(debug_stream, "LW xr%02d.%01d,  ", entry.rd, entry.rd_ptr);
			break;
		case 3:
			fprintf(debug_stream, "LD xr%02d.%01d,  ", entry.rd, entry.rd_ptr);
			break;
		case 8:
			fprintf(debug_stream, "LQ xr%02d.%01d,  ", entry.rd, entry.rd_ptr);
			break;
		case 4:
			fprintf(debug_stream, "ULB xr%02d.%01d,  ", entry.rd, entry.rd_ptr);
			break;
		case 5:
			fprintf(debug_stream, "ULH xr%02d.%01d,  ", entry.rd, entry.rd_ptr);
			break;
		case 6:
			fprintf(debug_stream, "ULW xr%02d.%01d,  ", entry.rd, entry.rd_ptr);
			break;
		case 7:
			fprintf(debug_stream, "ULD xr%02d.%01d,  ", entry.rd, entry.rd_ptr);
			break;
		case 12:
			fprintf(debug_stream, "ULQ xr%02d.%01d,  ", entry.rd, entry.rd_ptr);
			break;
		default:
			debug++;
			break;
		}
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(xr%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_LR:
		fprintf(debug_stream, "LR xr%02d.%x,  ", entry.rd, entry.rd_ptr);
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(reg%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_SC:
		fprintf(debug_stream, "SC xr%02d.%x, ", entry.rd, entry.rd_ptr);		//	error response
		fprintf(debug_stream, "xr%02d.%x, ", entry.rs1, entry.rs1_ptr);		//	addr
		fprintf(debug_stream, "xr%02d.%x ", entry.rs2, entry.rs2_ptr);		//	data
	break;	case uop_F_STORE:// need to work on funct3
		fprintf(debug_stream, "FSTORE fp%02d, ", entry.rs2);
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(reg%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_F_LOAD:// need to work on funct3
		fprintf(debug_stream, "FLOAD fp%02d,  ", entry.rd);
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(reg%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_ADDI:
		fprintf(debug_stream, "ADDI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, %d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.imm);
		break;
	case uop_ORI:
		fprintf(debug_stream, "ORI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, %d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.imm);
		break;
	case uop_XORI:
		fprintf(debug_stream, "XORI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, %d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.imm);
		break;
	case uop_ANDI:
		fprintf(debug_stream, "ANDI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, %d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.imm);
		break;
	case uop_SLLI:
		fprintf(debug_stream, "SLLI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "%s xr%02d.%x, %s xr%02d.%x, %d", reg_table->x_reg[entry.rd].name, entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rs1].name, entry.rs1, entry.rs1_ptr, entry.imm);
		break;
	case uop_SRLI:
		fprintf(debug_stream, "SRLI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, %d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.imm);
		break;
	case uop_SRAI:
		fprintf(debug_stream, "SRAI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, %d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.imm);
		break;
	case uop_ADD:
		fprintf(debug_stream, "ADD ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		else if (entry.funct3 & 8)
			fprintf(debug_stream, "_128 ");
		fprintf(debug_stream, "xr%02d.%02d, xr%02d.%02d, xr%02d.%02d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr);
		break;
	case uop_SUB:
		fprintf(debug_stream, "SUB ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		else if (entry.funct3 & 8)
			fprintf(debug_stream, "_128 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, xr%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr);
	break;	case uop_OR:
		fprintf(debug_stream, "OR ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, xr%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr);
		break;
	break;	case uop_XOR:
		fprintf(debug_stream, "XOR ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, xr%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr);
		break;
	case uop_AND:
		fprintf(debug_stream, "AND ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, xr%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr);
		break;
	case uop_SLL:
		fprintf(debug_stream, "SLL ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, xr%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr);
		break;
	case uop_SRL:
		fprintf(debug_stream, "SRL ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, xr%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr);
		break;
	case uop_MUL:
		fprintf(debug_stream, "MUL ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%02d, xr%02d.%02d, xr%02d.%02d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr);
		break;
	case uop_FCVTf2i:
		fprintf(debug_stream, "FCVT ");
		fprintf(debug_stream, "f%02d, xr%02d", entry.rd, entry.rs1);
		break;
	case uop_FCVTi2f:
		fprintf(debug_stream, "FCVT ");
		fprintf(debug_stream, "xr%02d, f%02d", entry.rd, entry.rs1);
		break;
	case uop_FMVf2i:
		fprintf(debug_stream, "FMV f2i ");
		fprintf(debug_stream, "xr%02d.%x, fp%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr);
		break;
	case uop_FMVi2f:
		fprintf(debug_stream, "FMV i2f ");
		fprintf(debug_stream, "fp%02d.%x, xr%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr);
		break;
	case uop_CSRRW:
		fprintf(debug_stream, "CSRRW xr%02d.%x, ", entry.rd, entry.rd_ptr);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "xr%02d.%x ", entry.rs1, entry.rs1_ptr);
		break;
	case uop_CSRRC:
		fprintf(debug_stream, "CSRRC xr%02d.%x, ", entry.rd, entry.rd_ptr);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "xr%02d.%x ", entry.rs1, entry.rs1_ptr);
		break;
	case uop_CSRRS:
		fprintf(debug_stream, "CSRRS xr%02d.%x, ", entry.rd, entry.rd_ptr);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "xr%02d.%x ", entry.rs1, entry.rs1_ptr);
		break;
	case uop_CSRRWI:
		if (entry.rd == 2)
			fprintf(debug_stream, "CSRRWI sp xr%02d.%x, ", entry.rd, entry.rd_ptr);
		else
			fprintf(debug_stream, "CSRRWI xr%02d.%x, ", entry.rd, entry.rd_ptr);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "0x%02x ", entry.imm);
		break;
	case uop_CSRRCI:
		if (entry.rd == 2)
			fprintf(debug_stream, "CSRRCI sp xr%02d.%x, ", entry.rd, entry.rd_ptr);
		else
			fprintf(debug_stream, "CSRRCI xr%02d.%x, ", entry.rd, entry.rd_ptr);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "0x%02x ", entry.imm);
		break;
	case uop_CSRRSI:
		fprintf(debug_stream, "CSRRSI xr%02d.%x, ", entry.rd, entry.rd_ptr);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "0x%02x ", entry.imm);
		break;
	case uop_FENCE:
		fprintf(debug_stream, "FENCE.%d ", entry.funct3);
		break;
	case uop_uret:
		fprintf(debug_stream, "uret ");
		break;
	case uop_MRET:
		fprintf(debug_stream, "MRET ");
		break;
	case uop_SRET:
		fprintf(debug_stream, "SRET ");
		break;
	case uop_ECALL:
		fprintf(debug_stream, "ECALL ");
		break;
	case uop_WFI:
		fprintf(debug_stream, "WFI ");
		break;
	case uop_HALT:
		fprintf(debug_stream, "HALT ");
		break;
	case uop_NOP:
		fprintf(debug_stream, "NOP ");
		break;
	case uop_shifti_add:
		fprintf(debug_stream, "SHIFTI ADD");
		break;
	case uop_or_add:
		fprintf(debug_stream, "OR ADD");
		break;
	case uop_or_addi:
		fprintf(debug_stream, "OR ADDI xr%02d, xr%02d, xr%02d, 0x%03x", entry.rd, entry.rs1, entry.rs2, entry.imm);
		break;
	case uop_imadd:
		fprintf(debug_stream, "integer MADD xr%02d, xr%02d, xr%02d, xr%02d", entry.rd, entry.rs1, entry.rs2, entry.rs3);
		break;
	case uop_inmadd:
		fprintf(debug_stream, "integer NMADD xr%02d, xr%02d, xr%02d, xr%02d", entry.rd, entry.rs1, entry.rs2, entry.rs3);
		break;
	default:
		debug++;
		break;
	}
	fprintf(debug_stream, "   // ");

}
void print_uop2(FILE* debug_stream, ROB_entry_Type entry, Reg_Table_Type* reg_table, param_type *param, UINT clock) {
	int debug = 0;
	switch (entry.uop) {
	case uop_JAL:
		fprintf(debug_stream, "JAL xr%02d.%x, ", entry.rd, entry.rd_ptr, entry.imm);
		break;
	case uop_JALR:
		fprintf(debug_stream, "JALR xr%02d.%x, xr%02d.%x, %#010x ", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.imm);
		break;
	case uop_LUI:
		fprintf(debug_stream, "LUI xr%02d.%x ", entry.rd, entry.rd_ptr);
		break;
	case uop_AUIPC:
		fprintf(debug_stream, "AUIPC xr%02d.%x,  %#010x ", entry.rd, entry.rd_ptr, entry.imm);
		break;
	case uop_BEQ:
		fprintf(debug_stream, "BEQ xr%02d, xr%02d, 0x%03x ", entry.rs1, entry.rs2, entry.imm);
		break;
	case uop_BNE:
		fprintf(debug_stream, "BNE xr%02d.%x, xr%02d.%x, 0x%03x ", entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr, entry.imm);
		break;
	case uop_BGE:
		fprintf(debug_stream, "BGE xr%02d, xr%02d, 0x%03x ", entry.rs1, entry.rs2, entry.imm);
		break;
	case uop_BLT:
		fprintf(debug_stream, "BLT xr%02d, xr%02d, 0x%03x ", entry.rs1, entry.rs2, entry.imm);
		break;
	case uop_STORE:// need to work on funct3
		fprintf(debug_stream, "STORE xr%02d.%x, ", entry.rs2, entry.rs2_ptr);
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(reg%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_LOAD:// need to work on funct3
		fprintf(debug_stream, "LOAD xr%02d.%01d,  ", entry.rd, entry.rd_ptr);
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(xr%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_LR:
		fprintf(debug_stream, "LR xr%02d.%x,  ", entry.rd, entry.rd_ptr);
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(reg%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_SC:
		fprintf(debug_stream, "SC xr%02d.%x,  ", entry.rd, entry.rd_ptr);
		fprintf(debug_stream, "xr%02d.%x,  ", entry.rs2, entry.rs2_ptr);
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(reg%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_F_STORE:// need to work on funct3
		fprintf(debug_stream, "FSTORE fp%02d, ", entry.rs2);
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(reg%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_F_LOAD:// need to work on funct3
		fprintf(debug_stream, "FLOAD fp%02d,  ", entry.rd);
		if (entry.rs1 == 2) {
			fprintf(debug_stream, "%#05x(sp) ", entry.imm);
		}
		else if (entry.rs1 == 3) {
			fprintf(debug_stream, "%#05x(gp) ", entry.imm);
		}
		else if (entry.rs1 == 4) {
			fprintf(debug_stream, "%#05x(tp) ", entry.imm);
		}
		else {
			fprintf(debug_stream, "%#05x(reg%02d) ", entry.imm, entry.rs1);
		}
		break;
	case uop_shifti_add:
		fprintf(debug_stream, "SHIFT ADD ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x,  xr%02d.%x, %d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr, entry.imm);

		fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs2, entry.rs2_ptr, reg_table->x_reg[entry.rs2].data[entry.rs1_ptr]);
		fprintf(debug_stream, " imm = 0x%04x,", entry.imm);
		break;
	case uop_or_addi:
		fprintf(debug_stream, "OR ADDI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x,  xr%02d.%x, %d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr, entry.imm);

		fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs2, entry.rs2_ptr, reg_table->x_reg[entry.rs2].data[entry.rs1_ptr]);
		fprintf(debug_stream, " imm = 0x%04x,", entry.imm);
		break;
	case uop_or_add:
		fprintf(debug_stream, "OR ADD ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, xr%02d.%x, xr%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr, entry.rs3, entry.rs3_ptr);

		fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " rs2 (xr%02d.%x)= 0x%016I64x,", entry.rs2, entry.rs2_ptr, reg_table->x_reg[entry.rs2].data[entry.rs3_ptr]);
		fprintf(debug_stream, " rs3 (xr%02d.%x)= 0x%016I64x,", entry.rs3, entry.rs3_ptr, reg_table->x_reg[entry.rs3].data[entry.rs3_ptr]);
		break;
	case uop_ADDI:
		fprintf(debug_stream, "ADDI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "%s xr%02d.%x, %s xr%02d.%x, %d", reg_table->x_reg[entry.rd].name, entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rs1].name, entry.rs1, entry.rs1_ptr, entry.imm);

		fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " imm = 0x%04x,", entry.imm);
		break;
	case uop_ORI:
		fprintf(debug_stream, "ORI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, %d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.imm);

		fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " imm = 0x%04x,", entry.imm);
		break;
	case uop_XORI:
		fprintf(debug_stream, "XORI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, %d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.imm);

		fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " imm = 0x%04x,", entry.imm);
		break;
	case uop_ANDI:
		fprintf(debug_stream, "ANDI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, %d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.imm);

		fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " imm = 0x%04x,", entry.imm);
		break;
	case uop_SLLI:
		fprintf(debug_stream, "SLLI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		else if (entry.funct3 & 8)
			fprintf(debug_stream, "_128 ");
		fprintf(debug_stream, "%s xr%02d.%x, %s xr%02d.%x, %d", reg_table->x_reg[entry.rd].name, entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rs1].name, entry.rs1, entry.rs1_ptr, entry.imm);
		if (param->mxl == 3) {
			fprintf(debug_stream, " (xr%02d.%x)= 0x%016I64x_0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data_H[entry.rd_ptr], reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		}
		else {
			fprintf(debug_stream, " (xr%02d.%x)= 0x%016I64x_0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data_H[entry.rd_ptr], reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		}
		fprintf(debug_stream, " (xr%02d.%x)= 0x%016I64x_0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data_H[entry.rs1_ptr], reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " imm = 0x%04x,", entry.imm);
		break;
	case uop_SRLI:
		fprintf(debug_stream, "SRLI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		else if (entry.funct3 & 8)
			fprintf(debug_stream, "_128 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, %d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.imm);
		if (param->mxl == 3) {
			fprintf(debug_stream, " (xr%02d.%x)= 0x%016I64x_0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data_H[entry.rd_ptr], reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		}
		else {
			fprintf(debug_stream, " (xr%02d.%x)= 0x%016I64x_0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data_H[entry.rd_ptr], reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		}
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x_0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data_H[entry.rs1_ptr], reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " imm = 0x%04x,", entry.imm);
		break;
	case uop_SRAI:
		fprintf(debug_stream, "SRAI ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, %d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.imm);

		fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x_0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data_H[entry.rd_ptr], reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x_0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data_H[entry.rs1_ptr], reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " imm = 0x%04x,", entry.imm);
		break;
	case uop_ADD:
		fprintf(debug_stream, "ADD ");
		fprintf(debug_stream, "%s xr%02d.%x, %s xr%02d.%x, %s xr%02d.%x", reg_table->x_reg[entry.rd].name, entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rs1].name, entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs2].name, entry.rs2, entry.rs2_ptr);
		if (param->mxl == 3) {
			fprintf(debug_stream, " (xr%02d.%x)= 0x%016I64x_%016llx,",
				entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data_H[entry.rd_ptr], reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		}
		else {
			fprintf(debug_stream, " (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		}
		fprintf(debug_stream, " (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " (xr%02d.%x)= 0x%016I64x,", entry.rs2, entry.rs2_ptr, reg_table->x_reg[entry.rs2].data[entry.rs2_ptr]);
		break;
	case uop_SUB:
		fprintf(debug_stream, "SUB ");
		fprintf(debug_stream, "%s xr%02d.%x, %s xr%02d.%x, %s xr%02d.%x",
			reg_table->x_reg[entry.rd].name, entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rs1].name, entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs2].name, entry.rs2, entry.rs2_ptr);
		if (param->mxl == 3) {
			fprintf(debug_stream, " (xr%02d.%x)= 0x%016I64x_%016llx,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data_H[entry.rd_ptr], reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		}
		else {
			fprintf(debug_stream, " (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		}
		fprintf(debug_stream, " (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " (xr%02d.%x)= 0x%016I64x,", entry.rs2, entry.rs2_ptr, reg_table->x_reg[entry.rs2].data[entry.rs2_ptr]);
		break;
	case uop_OR:
		fprintf(debug_stream, "OR ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, xr%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr);

		fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " rs2 (xr%02d.%x)= 0x%016I64x,", entry.rs2, entry.rs2_ptr, reg_table->x_reg[entry.rs2].data[entry.rs2_ptr]);
		break;
	break;	case uop_XOR:
		fprintf(debug_stream, "XOR ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, xr%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr);

		fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " rs2 (xr%02d.%x)= 0x%016I64x,", entry.rs2, entry.rs2_ptr, reg_table->x_reg[entry.rs2].data[entry.rs2_ptr]);
		break;
	case uop_AND:
		fprintf(debug_stream, "AND ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, xr%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr);

		fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " rs2 (xr%02d.%x)= 0x%016I64x,", entry.rs2, entry.rs2_ptr, reg_table->x_reg[entry.rs2].data[entry.rs2_ptr]);
		break;
	case uop_SLL:
		fprintf(debug_stream, "SLL ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, xr%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr);

		fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " rs2 (xr%02d.%x)= 0x%016I64x,", entry.rs2, entry.rs2_ptr, reg_table->x_reg[entry.rs2].data[entry.rs2_ptr]);
		break;
	case uop_SRL:
		fprintf(debug_stream, "SRL ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%x, xr%02d.%x, xr%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr);

		fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " rs2 (xr%02d.%x)= 0x%016I64x,", entry.rs2, entry.rs2_ptr, reg_table->x_reg[entry.rs2].data[entry.rs2_ptr]);
		break;
	case uop_MUL:
		fprintf(debug_stream, "MUL ");
		if ((entry.funct3 & 3) == 3)
			fprintf(debug_stream, "_64 ");
		fprintf(debug_stream, "xr%02d.%02d, xr%02d.%02d, xr%02d.%02d", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr, entry.rs2, entry.rs2_ptr);

		fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x,", entry.rd, entry.rd_ptr, reg_table->x_reg[entry.rd].data[entry.rd_ptr]);
		fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", entry.rs1, entry.rs1_ptr, reg_table->x_reg[entry.rs1].data[entry.rs1_ptr]);
		fprintf(debug_stream, " rs2 (xr%02d.%x)= 0x%016I64x,", entry.rs2, entry.rs2_ptr, reg_table->x_reg[entry.rs2].data[entry.rs2_ptr]);
		break;
	case uop_FCVTf2i:
		fprintf(debug_stream, "FCVT ");
		fprintf(debug_stream, "f%02d, xr%02d", entry.rd, entry.rs1);
		break;
	case uop_FCVTi2f:
		fprintf(debug_stream, "FCVT ");
		fprintf(debug_stream, "xr%02d, f%02d", entry.rd, entry.rs1);
		break;
	case uop_FMVf2i:
		fprintf(debug_stream, "FMV f2i ");
		fprintf(debug_stream, "xr%02d.%x, fp%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr);
		break;
	case uop_FMVi2f:
		fprintf(debug_stream, "FMV i2f ");
		fprintf(debug_stream, "fp%02d.%x, xr%02d.%x", entry.rd, entry.rd_ptr, entry.rs1, entry.rs1_ptr);
		break;
	case uop_CSRRW:
		fprintf(debug_stream, "CSRRW xr%02d.%x, ", entry.rd, entry.rd_ptr);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "xr%02d.%x ", entry.rs1, entry.rs1_ptr);
		break;
	case uop_CSRRC:
		fprintf(debug_stream, "CSRRC xr%02d.%x, ", entry.rd, entry.rd_ptr);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "xr%02d.%x ", entry.rs1, entry.rs1_ptr);
		break;
	case uop_CSRRS:
		fprintf(debug_stream, "CSRRS rd = xr%02d.%x, ", entry.rd, entry.rd_ptr);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "xr%02d.%x ", entry.rs1, entry.rs1_ptr);
		break;
	case uop_CSRRWI:
		fprintf(debug_stream, "CSRRWI xr%02d.%x, ", entry.rd, entry.rd_ptr);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "0x%02x ", entry.imm);
		break;
	case uop_CSRRCI:
		fprintf(debug_stream, "CSRRCI xr%02d.%x, ", entry.rd, entry.rd_ptr);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "0x%02x ", entry.imm);
		break;
	case uop_CSRRSI:
		fprintf(debug_stream, "CSRRSI xr%02d.%x, ", entry.rd, entry.rd_ptr);
		fprint_csr(debug_stream,entry.csr);
		fprintf(debug_stream, "0x%02x ", entry.imm);
		break;
	case uop_FENCE:
		fprintf(debug_stream, "FENCE.%d ", entry.funct3);
		break;
	case uop_uret:
		fprintf(debug_stream, "uret ");
		break;
	case uop_MRET:
		fprintf(debug_stream, "MRET ");
		break;
	case uop_SRET:
		fprintf(debug_stream, "SRET ");
		break;
	case uop_ECALL:
		fprintf(debug_stream, "ECALL ");
		break;
	case uop_WFI:
		fprintf(debug_stream, "WFI ");
		break;
	case uop_HALT:
		fprintf(debug_stream, "HALT ");
		break;
	case uop_NOP:
		fprintf(debug_stream, "NOP ");
		break;
	default:
		debug++;
		break;
	}
	fprintf(debug_stream, "   // ");

}
void fprint_addr_asm(FILE* debug_stream, ROB_entry_Type* ROB_q) {
	if (ROB_q->rs1 == 2) {
		fprintf(debug_stream, "%#05x(sp) ", ROB_q->imm);
	}
	else if (ROB_q->rs1 == 3) {
		fprintf(debug_stream, "%#05x(gp) ", ROB_q->imm);
	}
	else if (ROB_q->rs1 == 4) {
		fprintf(debug_stream, "%#05x(tp) ", ROB_q->imm);
	}
	else {
		fprintf(debug_stream, "%#05x(xr%02d.%01x) ", ROB_q->imm, ROB_q->rs1, ROB_q->rs1_ptr);
	}

}
void fprint_addr_coma(FILE* debug_stream, UINT64 addr, param_type* param) {
	UINT8 debug = 0;
	switch (param->satp >> 60 & 0x0f) {
	case 8://39
		fprintf(debug_stream, "0x%010I64x, ", addr);
		break;
	case 9://48
//		fprintf(debug_stream, "0x%012I64x, ", addr);
		fprintf(debug_stream, "0x%04x_%08x, ", addr>>32, addr&0x0000ffffffff);
		break;
	case 0x0a://57
		fprintf(debug_stream, "0x%015I64x, ", addr);
		break;
	case 0x0b://64
		fprintf(debug_stream, "0x%016I64x, ", addr);
		break;
	default:
		debug++;
		break;
	}
}
void fprint_addr(FILE* debug_stream, UINT64 addr, param_type* param) {
	UINT8 debug = 0;
	switch (param->satp >> 60 & 0x0f) {
	case 8://39
		fprintf(debug_stream, "0x%010I64x ", addr);
		break;
	case 9://48
		//		fprintf(debug_stream, "0x%012I64x, ", addr);
		fprintf(debug_stream, "0x%04x_%08x ", addr >> 32, addr & 0x0000ffffffff);
		break;
	case 0x0a://57
		fprintf(debug_stream, "0x%015I64x ", addr);
		break;
	case 0x0b://64
		fprintf(debug_stream, "0x%016I64x ", addr);
		break;
	default:
		debug++;
		break;
	}
}
void print_fp_data(FILE* debug_stream,UINT64 data, ROB_entry_Type* ROB_ptr) {
	UINT8 debug = 0;
	switch (ROB_ptr->funct3) {
	case 1:
		fprintf(debug_stream, " 0x%03I64x,", data);
		break;
	case 2:
		fprintf(debug_stream, " 0x%04I64x,", data);
		break;
	case 3:
		fprintf(debug_stream, " 0x%08I64x,", data);
		break;
	case 4:
		fprintf(debug_stream, " 0x%016I64x,", data);
		break;
	default:
		debug++;
		break;
	}
}
void retire_unit(ROB_Type* ROB, Reg_Table_Type* reg_table, csr_type* csr, retire_type* retire, UINT8 reset, reg_bus* rd_decode, UINT64 clock, UINT mhartid, UINT debug_core, param_type *param, FILE* debug_stream) {
	// need a unique static index to PC 
	int debug = 0;
	if (retire->load_PC == 4)
		retire->load_PC = 0;

	if (mhartid == 5) {
		if (clock >= 0x0178)
			debug++;
	}
//	UINT mhartid = mhartid;
	if (reset) {
		for (UINT i = 0; i < 0x100; i++) {
			ROB->q[i].state = ROB_free;
			ROB->q[i].addr = 0;
			ROB->q[i].imm = 0;
			ROB->q[i].csr = csr_mhartid;
			ROB->q[i].rd = 0;
			ROB->q[i].rs1 = 0;
			ROB->q[i].rs2 = 0;
			ROB->q[i].rd_ptr = 0;
			ROB->q[i].rs1_ptr = 0;
			ROB->q[i].rs2_ptr = 0;
			ROB->q[i].rd_retire_ptr = 0;
			ROB->q[i].rs_count = 0;

			ROB->q[i].q_ptr = 0;
			ROB->q[i].q_ptr_valid = 0;
		}
		retire->priviledge = 3;
		retire->next_priviledge = 3;
		retire->PC = 0x00001000;

		retire->load_PC = 8; // RESET hack, need signals. No way to assure that target is seeing the signal increase, then decrease
		ROB->decode_ptr = ROB->allocate_ptr = ROB->retire_ptr_out = 0;
		for (UINT8 j = 0; j < 0x20; j++) {
			reg_table->x_reg[j].current_ptr = reg_table->x_reg[j].retire_ptr = 0;
			for (UINT8 i = 0; i < reg_rename_size; i++)
				reg_table->x_reg[j].valid[i] = reg_free;
			reg_table->x_reg[j].valid[reg_table->x_reg[j].retire_ptr] = reg_valid_out;

			reg_table->f_reg[j].current_ptr = reg_table->f_reg[j].retire_ptr = 0;
			for (UINT8 i = 0; i < reg_rename_size; i++)
				reg_table->f_reg[j].valid[i] = reg_free;
			reg_table->f_reg[j].valid[reg_table->f_reg[j].retire_ptr] = reg_valid_out;
		}
		ROB->fault_halt = 0;
	}
	else if (rd_decode->strobe==2) {
		retire->PC = rd_decode->data;
	}
	else {
		for (UINT i = ROB->retire_ptr_out, count = 0; i != ROB->decode_ptr && count < param->decode_width; i = ((i + 1) & 0x00ff), count++) {
			if (ROB->q[i].state == ROB_retire_in || ROB->q[i].state == ROB_branch_miss) { // ROB_retire needs to be valid for at least 1 clock before retiring, code ordering required
				if (mhartid == 0)
					debug++;
				if (ROB->q[i].addr != retire->PC && ROB->q[i].addr != 0 && ROB->q[i].uop != uop_SRET && retire->PC != 0) { // reorder buffer is overflowing, need to introduce throttle
					if (ROB->q[(i - 1) & 0xff].map == BRANCH_map) {
						count = param->decode_width;
					}
					else {
						debug++;
						fprintf(debug_stream, "RETIRE(%lld): ERROR address missmatch; retire addr 0x%016I64x, PC addr 0x%016I64x, clock 0x%08llx\n", 
							mhartid, ROB->q[i].addr, retire->PC, clock);
					}
				}
				else {
					csr[csr_instret].value++;
					switch (retire->priviledge) {
					case 0:
						break;
					case 1:
						csr[csr_sinstret].value++;
						break;
					case 2:
						csr[csr_hinstret].value++;
						break;
					case 3:
						csr[csr_minstret].value++;
						break;
					default:
						debug++;
						break;
					}
					switch (ROB->q[i].map) {
					case SYSTEM_map: {
						ROB->fence = 0;
						switch (ROB->q[i].uop) {
						case uop_MRET: {
							csr[csr_mhpmcounter12].value++;
							if (debug_core) {
								fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ",
									mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr, retire->PC);
								fprintf(debug_stream, "MRET ");
								fprintf(debug_stream, " clock 0x%08llx\n", clock);
							}
							retire->PC += ROB->q[i].bytes;
							if (ROB->allocate_ptr == ROB->retire_ptr_out)
								ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
							ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

							reg_table->x_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
							reg_table->x_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;
							retire->priviledge = 0;
			
							retire->PC = csr[csr_mepc].value;
							retire->load_PC = 8; // MRET hack, need signals. No way to assure that target is seeing the signal increase, then decrease
							// clear ROB and associated queues
							for (int j = 0; j < 0x100; j++)ROB->q[j].state = ROB_free;
						}
									 break;
						case uop_SRET: {
							csr[csr_mhpmcounter12].value++;
							if (debug_core) {
								fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", 
									mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
								fprint_addr_coma(debug_stream, retire->PC, param);
								fprintf(debug_stream, "SRET return addr: ");
								fprint_addr_coma(debug_stream, csr[csr_sepc].value, param);
								fprintf(debug_stream, " clock 0x%08llx\n", clock);
							}
							if (ROB->allocate_ptr == ROB->retire_ptr_out)
								ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
							ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);
							retire->priviledge = 0;
							retire->PC = csr[csr_sepc].value;
							retire->load_PC = 4; // SRET hack, need signals. No way to assure that target is seeing the signal increase, then decrease
							// clear ROB and associated queues
							for (int j = 0; j < 0x100; j++)ROB->q[j].state = ROB_free;

							ROB->fence = 0;
							ROB->branch_stop = ROB->branch_start = 0; // SRET is a synchronizing event
						}
									 break;
						case uop_ECALL: {
							csr[csr_mhpmcounter12].value++;
							if (debug_core) {
								fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", 
									mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr, retire->PC);
								fprintf(debug_stream, "ECALL ");
								fprintf(debug_stream, " clock 0x%08llx\n", clock);
							}
							retire->PC += ROB->q[i].bytes;
							if (ROB->allocate_ptr == ROB->retire_ptr_out)
								ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
							ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

							reg_table->x_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
							reg_table->x_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;

	//						if (debug_core) {
								fprintf(debug_stream, "RETIRE(%lld): ROB entry: % #04x, ERROR- ECALL is not retired; clock 0x%08llx\n", mhartid, ROB->retire_ptr_out, clock);
	//						}

							retire->load_PC = 0x18; // ECALL hack, need signals. No way to assure that target is seeing the signal increase, then decrease
							retire->PC = csr[csr_stvec].value;;
							retire->load_PC = 4; // SRET hack, need signals. No way to assure that target is seeing the signal increase, then decrease
							for (int j = 0; j < 0x100; j++)ROB->q[j].state = ROB_free;

							ROB->fence = 0;
							ROB->branch_stop = ROB->branch_start = 0; // SRET is a synchronizing event
						}
									  break;
						case uop_WFI: {
							if (debug_core) {
								fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ",
									mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr, retire->PC);
								fprintf(debug_stream, "WFI ");
								fprintf(debug_stream, " clock 0x%08llx\n", clock);
							}
							retire->PC += ROB->q[i].bytes;
							if (ROB->allocate_ptr == ROB->retire_ptr_out)
								ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
							ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

							reg_table->x_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
							reg_table->x_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;

							for (UINT index = ((reg_table->x_reg[ROB->q[i].rd].current_ptr + 1) & (reg_rename_size - 1)); index != reg_table->x_reg[ROB->q[i].rd].retire_ptr; index = ((index + 1) & (reg_rename_size - 1)))
								reg_table->x_reg[ROB->q[i].rd].valid[index] = reg_free;

							retire->load_PC = 0x18; //WFI hack, need signals. No way to assure that target is seeing the signal increase, then decrease

							// clear ROB and associated queues
							for (int j = 0; j < 0x100; j++)ROB->q[j].state = ROB_free;
						}
									break;
						case uop_HALT: {
							if (debug_core) {
								fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, 0x%016I64x ", 
									mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr, retire->PC);
								fprintf(debug_stream, "HALT ");
								fprintf(debug_stream, " clock 0x%08llx\n", clock);
							}
							retire->PC += ROB->q[i].bytes;
							if (ROB->allocate_ptr == ROB->retire_ptr_out)
								ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
							ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

							reg_table->x_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
							reg_table->x_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;

							for (UINT index = ((reg_table->x_reg[ROB->q[i].rd].current_ptr + 1) & (reg_rename_size - 1)); index != reg_table->x_reg[ROB->q[i].rd].retire_ptr; index = ((index + 1) & (reg_rename_size - 1)))
								reg_table->x_reg[ROB->q[i].rd].valid[index] = reg_free;
							retire->load_PC = 0x18; //HALT hack, need signals. No way to assure that target is seeing the signal increase, then decrease
						}
									 break;
						default: {
							if (debug_core) {
								fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", 
									mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
								fprint_addr_coma(debug_stream, retire->PC, param);
								switch (ROB->q[i].uop) {
								case uop_CSRRW:
									fprintf(debug_stream, "CSRRW xr%02d.%x, ", ROB->q[i].rd, ROB->q[i].rd_ptr);
									fprint_csr(debug_stream, ROB->q[i].csr);
									fprintf(debug_stream, "xr%02d.%x ", ROB->q[i].rs1, ROB->q[i].rs1_ptr);
									break;
								case uop_CSRRC:
									fprintf(debug_stream, "CSRRC xr%02d.%x, ", ROB->q[i].rd, ROB->q[i].rd_ptr);
									fprint_csr(debug_stream, ROB->q[i].csr);
									fprintf(debug_stream, "xr%02d.%x ", ROB->q[i].rs1, ROB->q[i].rs1_ptr);
									break;
								case uop_CSRRS:
									fprintf(debug_stream, "CSRRS xr%02d.%x, ", ROB->q[i].rd, ROB->q[i].rd_ptr);
									fprint_csr(debug_stream, ROB->q[i].csr);
									fprintf(debug_stream, "xr%02d.%x ", ROB->q[i].rs1, ROB->q[i].rs1_ptr);
									break;
								case uop_CSRRWI:
									if (ROB->q[i].rd == 2)
										fprintf(debug_stream, "CSRRWI sp xr%02d.%x, ", ROB->q[i].rd, ROB->q[i].rd_ptr);
									else
										fprintf(debug_stream, "CSRRWI xr%02d.%x, ", ROB->q[i].rd, ROB->q[i].rd_ptr);
									fprint_csr(debug_stream, ROB->q[i].csr);
									fprintf(debug_stream, "0x%02x ", ROB->q[i].imm);
									break;
								case uop_CSRRCI:
									if (ROB->q[i].rd == 2)
										fprintf(debug_stream, "CSRRCI sp xr%02d.%x, ", ROB->q[i].rd, ROB->q[i].rd_ptr);
									else
										fprintf(debug_stream, "CSRRCI xr%02d.%x, ", ROB->q[i].rd, ROB->q[i].rd_ptr);
									fprint_csr(debug_stream, ROB->q[i].csr);
									fprintf(debug_stream, "0x%02x ", ROB->q[i].imm);
									break;
								case uop_CSRRSI:
									fprintf(debug_stream, "CSRRSI xr%02d.%x, ", ROB->q[i].rd, ROB->q[i].rd_ptr);
									fprint_csr(debug_stream, ROB->q[i].csr);
									fprintf(debug_stream, "0x%02x ", ROB->q[i].imm);
									break;
								default:
									debug++;
									break;
								}
								if (param->mxl == 3) {
									fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x_%016llx, csr: 0x%016I64x,", 
										ROB->q[i].rd, ROB->q[i].rd_ptr, reg_table->x_reg[ROB->q[i].rd].data_H[ROB->q[i].rd_ptr], reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr], csr[ROB->q[i].csr].value);
								}
								else {
									fprintf(debug_stream, " rd (xr%02d.%x)= 0x%016I64x, csr: 0x%016I64x,", 
										ROB->q[i].rd, ROB->q[i].rd_ptr, reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr], csr[ROB->q[i].csr].value);
								}
								fprintf(debug_stream, " clock 0x%08llx\n", clock);
							}
							retire->PC += ROB->q[i].bytes;
							if (ROB->allocate_ptr == ROB->retire_ptr_out)
								ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
							ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

							reg_table->x_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
							reg_table->x_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;
						}
						}
						break;
					}
								   break;
					case LUI_map: {
						if (debug_core) {
							fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", 
								mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
							fprint_addr_coma(debug_stream, retire->PC, param);
							fprintf(debug_stream, "LUI xr%02d.%x  0x%08x", ROB->q[i].rd, ROB->q[i].rd_ptr, reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr]);
							fprintf(debug_stream, " clock 0x%08llx\n", clock);
						}
						retire->PC += ROB->q[i].bytes;
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

						reg_table->x_reg[ROB->q[i].rd].valid[(ROB->q[i].rd_ptr - 1) & (reg_rename_size - 1)] = reg_free;
						reg_table->x_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;

						if (ROB->q[i].q_select == OP_q_select0 || ROB->q[i].q_select == OP_q_select1 || ROB->q[i].q_select == OP_q_select2 || ROB->q[i].q_select == OP_q_select3 || ROB->q[i].map == AMO_map) {// AMO fence bus transactions only
							debug++;
						}
					}
								break;
					case JAL_map: {
						csr[csr_mhpmcounter6].value++;
						UINT64 addr = retire->PC + ROB->q[i].imm; // make sure PC value is correct, wheather branch taken or not taken
						for (UINT index = ((reg_table->x_reg[ROB->q[i].rd].current_ptr + 1) & (reg_rename_size - 1)); index != reg_table->x_reg[ROB->q[i].rd].retire_ptr; index = ((index + 1) & (reg_rename_size - 1)))
							reg_table->x_reg[ROB->q[i].rd].valid[index] = reg_free;

						reg_table->x_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
						reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr] = retire->PC + 4;
						reg_table->x_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;

						if (debug_core) {
							fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
							fprint_addr_coma(debug_stream, retire->PC, param);
							fprintf(debug_stream, "JAL rd xr%d.%d = 0x%016I64x, addr ", ROB->q[i].rd, ROB->q[i].rd_ptr, reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr]);
							fprint_addr_coma(debug_stream, addr, param);
							fprintf(debug_stream, " clock 0x%08llx\n", clock);
						}
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

						if (addr >= csr[csr_sbound].value) {//0x86400000
							retire->priviledge = 0;
						}
						else if (addr >= csr[csr_mbound].value) {//0x80000000
							retire->priviledge = 1;
						}
						else {
							retire->priviledge = 3;
						}
						retire->PC = addr;
						retire->load_PC = 7;// JAL
					}
								break;
					case JALR_map: {
						csr[csr_mhpmcounter6].value++;
						if (debug_core) {
							fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
							fprint_addr_coma(debug_stream, retire->PC, param);
							fprintf(debug_stream, "JALR xr%02d.%x, xr%02d.%x ", ROB->q[i].rd, ROB->q[i].rd_ptr, ROB->q[i].rs1, ROB->q[i].rs1_ptr);
							if (ROB->q[i].bytes == 8)
								fprintf(debug_stream, " rd = 0x%08x, addr 0x%08x,", reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr], ROB->q[i].imm & 0x00000000ffffffff);
							else {
								fprintf(debug_stream, " rd = 0x%016I64x, addr ", reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr]);
								fprint_addr_coma(debug_stream, ROB->q[i].imm & 0x00000000ffffffff, param);
							}
							fprintf(debug_stream, " clock 0x%08llx\n", clock);
						}
						retire->PC += ROB->q[i].bytes;
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

						reg_table->x_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
						reg_table->x_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;

						for (UINT index = ((reg_table->x_reg[ROB->q[i].rd].current_ptr + 1) & (reg_rename_size - 1)); index != reg_table->x_reg[ROB->q[i].rd].retire_ptr; index = ((index + 1) & (reg_rename_size - 1)))
							reg_table->x_reg[ROB->q[i].rd].valid[index] = reg_free;
						UINT64 addr = ROB->q[i].imm & 0x00000000ffffffff; // make sure PC value is correct, wheather branch taken or not taken
						/**/
						if (addr >= csr[csr_sbound].value) {//0x86400000
							retire->priviledge = 0;
						}
						else if (addr >= csr[csr_mbound].value) {//0x80000000
							retire->priviledge = 1;
						}
						else {
							retire->priviledge = 3;
						}
						/**/
						retire->PC = addr;
						count = param->decode_width;
					}
								 break;
					case BRANCH_map: {
						csr[csr_hpmcounter4].value++;
						switch (retire->priviledge) {
						case 0:
							break;
						case 1:
							csr[csr_shpmcounter4].value++;
							break;
						case 2:
							csr[csr_hhpmcounter4].value++;
							break;
						case 0x0f:
						case 3:
							csr[csr_mhpmcounter4].value++;
							break;
						default:
							debug++;
							break;
						}
						UINT index = ROB->branch_start;
						if (debug_core) {
							fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ",
								mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
							fprint_addr_coma(debug_stream, retire->PC, param);
							ROB_entry_Type* entry = &ROB->q[i];
							switch (ROB->q[i].uop) {
							case uop_BEQ:
								fprintf(debug_stream, "BEQ");
								break;
							case uop_BNE:
								fprintf(debug_stream, "BNE");
								break;
							case uop_BGE:
								fprintf(debug_stream, "BGE");
								break;
							case uop_BLT:
								fprintf(debug_stream, "BLT");
								break;
							default:
								break;
							}
							fprintf(debug_stream, " %s xr%02d.%x, %s xr%02d.%x, 0x%03x",
								reg_table->x_reg[entry->rs1].name, entry->rs1, entry->rs1_ptr,
								reg_table->x_reg[entry->rs2].name, entry->rs2, entry->rs2_ptr,
								entry->imm - ROB->q[i].addr);
							if (ROB->q[i].state == ROB_branch_miss) {
								fprintf(debug_stream, " BRANCH MISS ");
							}
							else {
								fprintf(debug_stream, " BRANCH ");
							}
							fprintf(debug_stream, "addr = ");
	//						fprint_addr_coma(debug_stream, ROB->q[i].addr + ROB->q[i].imm, param);
							fprint_addr_coma(debug_stream, ROB->q[i].imm, param);
							fprintf(debug_stream, " taken=%d", ROB->q[i].rs3);
							fprintf(debug_stream, " rs1 (xr%02d.%x)= 0x%016I64x,", ROB->q[i].rs1, ROB->q[i].rs1_ptr, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr]);
							fprintf(debug_stream, " rs2 (xr%02d.%x)= 0x%016I64x,", ROB->q[i].rs2, ROB->q[i].rs2_ptr, reg_table->x_reg[ROB->q[i].rs2].data[ROB->q[i].rs2_ptr]);
							fprintf(debug_stream, " branch_num %d %d/%d,", ROB->q[i].branch_num, ROB->branch_start, ROB->branch_stop);
							fprintf(debug_stream, " clock 0x%08llx\n", clock);
						}
						retire->PC += ROB->q[i].bytes;
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);
						retire->PC = ((UINT64)ROB->q[i].imm)&0x0000000ffffffff; // make sure PC value is correct, wheather branch taken or not taken
						ROB->fault_halt = 0;
						if (ROB->q[i].state == ROB_branch_miss) {
							count = param->decode_width;
							ROB->decode_ptr = ROB->allocate_ptr = ROB->retire_ptr_out;
							for (UINT16 j = 0; j < 0x100; j++)ROB->q[j].state = ROB_free;
						}
					}
								   break;
					case STORE_map: {
						csr[csr_hpmcounter10].value++;
						switch (retire->priviledge) {
						case 0:
							break;
						case 1:
							csr[csr_shpmcounter10].value++;
							break;
						case 2:
							csr[csr_hhpmcounter10].value++;
							break;
						case 3:
							csr[csr_mhpmcounter10].value++;
							break;
						default:
							debug++;
							break;
						}
						if (debug_core) {
							fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", 
								mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
							fprint_addr_coma(debug_stream, retire->PC,param);
							switch (ROB->q[i].funct3) {
							case 0:
								fprintf(debug_stream, "SB xr%02d.%x (%s), ", ROB->q[i].rs2, ROB->q[i].rs2_ptr, reg_table->x_reg[ROB->q[i].rs2].name);
								break;
							case 1:
								fprintf(debug_stream, "SH xr%02d.%x (%s), ", ROB->q[i].rs2, ROB->q[i].rs2_ptr, reg_table->x_reg[ROB->q[i].rs2].name);
								break;
							case 2:
								fprintf(debug_stream, "SW xr%02d.%x (%s), ", ROB->q[i].rs2, ROB->q[i].rs2_ptr, reg_table->x_reg[ROB->q[i].rs2].name);
								break;
							case 3:
								fprintf(debug_stream, "SD xr%02d.%x (%s), ", ROB->q[i].rs2, ROB->q[i].rs2_ptr, reg_table->x_reg[ROB->q[i].rs2].name);
								break;
							case 8:
								fprintf(debug_stream, "SQ xr%02d.%x (%s), ", ROB->q[i].rs2, ROB->q[i].rs2_ptr, reg_table->x_reg[ROB->q[i].rs2].name);
								break;
							case 4:
								fprintf(debug_stream, "SQ xr%02d.%x (%s), ", ROB->q[i].rs2, ROB->q[i].rs2_ptr, reg_table->x_reg[ROB->q[i].rs2].name);
								break;
							default:
								debug++;
								break;
							}
							if (ROB->q[i].rs1 == 2) {
								fprintf(debug_stream, "%#05x(sp xr02.%x) ", ROB->q[i].imm, ROB->q[i].rs1_ptr);
							}
							else if (ROB->q[i].rs1 == 3) {
								fprintf(debug_stream, "%#05x(gp) ", ROB->q[i].imm);
							}
							else if (ROB->q[i].rs1 == 4) {
								fprintf(debug_stream, "%#05x(tp) ", ROB->q[i].imm);
							}
							else {
								fprintf(debug_stream, "%#05x(reg%02d.%x) ", ROB->q[i].imm, ROB->q[i].rs1, ROB->q[i].rs1_ptr);
							}
							fprintf(debug_stream, " addr, data = ");
							fprint_addr_coma(debug_stream, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr] + ROB->q[i].imm, param);
							switch (ROB->q[i].funct3) {
							case 0:
								fprintf(debug_stream, " 0x%02x,", reg_table->x_reg[ROB->q[i].rs2].data[ROB->q[i].rs2_ptr]);
								break;
							case 1:
								fprintf(debug_stream, "  0x%04x,", reg_table->x_reg[ROB->q[i].rs2].data[ROB->q[i].rs2_ptr]);
								break;
							case 2:
								fprintf(debug_stream, " 0x%08x,", reg_table->x_reg[ROB->q[i].rs2].data[ROB->q[i].rs2_ptr]);
								break;
							case 3:
								fprintf(debug_stream, " 0x%016I64x,", reg_table->x_reg[ROB->q[i].rs2].data[ROB->q[i].rs2_ptr]);
								break;
							case 4:
								fprintf(debug_stream, " 0x%016I64x_%016llx,", reg_table->x_reg[ROB->q[i].rs2].data_H[ROB->q[i].rs2_ptr], reg_table->x_reg[ROB->q[i].rs2].data[ROB->q[i].rs2_ptr]);
								break;
							default:
								debug++;
								break;
							}
							fprintf(debug_stream, " clock 0x%08llx\n", clock);
						}
						retire->PC += ROB->q[i].bytes;
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

						for (UINT index = ((reg_table->x_reg[ROB->q[i].rd].current_ptr + 1) & (reg_rename_size - 1)); index != reg_table->x_reg[ROB->q[i].rd].retire_ptr; index = ((index + 1) & (reg_rename_size - 1)))
							reg_table->x_reg[ROB->q[i].rd].valid[index] = reg_free;
					}
								  break;
					case LOAD_map: {
						csr[csr_hpmcounter8].value++;
						switch (retire->priviledge) {
						case 0:
							break;
						case 1:
							csr[csr_shpmcounter8].value++;
							break;
						case 2:
							csr[csr_hhpmcounter8].value++;
							break;
						case 3:
							csr[csr_mhpmcounter8].value++;
							break;
						default:
							debug++;
							break;
						}
						if (debug_core) {
							fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
							fprint_addr_coma(debug_stream, retire->PC, param);
							switch (ROB->q[i].funct3) {
							case 0:
								fprintf(debug_stream, "LB xr%02d.%01x (%s),  ", ROB->q[i].rd, ROB->q[i].rd_ptr, reg_table->x_reg[ROB->q[i].rd].name);
								fprint_addr_asm(debug_stream, &ROB->q[i]);
								fprintf(debug_stream, " addr, data = ");
								fprint_addr_coma(debug_stream, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr] + ROB->q[i].imm, param);
								fprintf(debug_stream, " %02llx, clock 0x%08llx\n", reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr], clock);
								break;
							case 1:
								fprintf(debug_stream, "LH xr%02d.%01x (%s),  ", ROB->q[i].rd, ROB->q[i].rd_ptr, reg_table->x_reg[ROB->q[i].rd].name);
								fprint_addr_asm(debug_stream, &ROB->q[i]);
								fprintf(debug_stream, " addr, data = ");
								fprint_addr_coma(debug_stream, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr] + ROB->q[i].imm, param);
								fprintf(debug_stream, " %04llx, clock 0x%08llx\n", reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr], clock);
								break;
							case 2:
								fprintf(debug_stream, "LW xr%02d.%01x (%s),  ", ROB->q[i].rd, ROB->q[i].rd_ptr, reg_table->x_reg[ROB->q[i].rd].name);
								fprint_addr_asm(debug_stream, &ROB->q[i]);
								fprintf(debug_stream, " addr, data = ");
								fprint_addr_coma(debug_stream, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr] + ROB->q[i].imm, param);
								fprintf(debug_stream, "  %08llx, clock 0x%08llx\n", reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr], clock);
								break;
							case 3:
								fprintf(debug_stream, "LD xr%02d.%01x (%s),  ", ROB->q[i].rd, ROB->q[i].rd_ptr, reg_table->x_reg[ROB->q[i].rd].name);
								fprint_addr_asm(debug_stream, &ROB->q[i]);
								fprintf(debug_stream, " addr, data = ");
								fprint_addr_coma(debug_stream, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr] + ROB->q[i].imm, param);
								fprintf(debug_stream, " %016llx, clock 0x%08llx\n", reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr], clock);
								break;
							case 8:
								fprintf(debug_stream, "LQ xr%02d.%01x (%s),  ", ROB->q[i].rd, ROB->q[i].rd_ptr, reg_table->x_reg[ROB->q[i].rd].name);
								fprint_addr_asm(debug_stream, &ROB->q[i]);
								fprintf(debug_stream, " addr, data = ");
								fprint_addr_coma(debug_stream, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr] + ROB->q[i].imm, param);
								fprintf(debug_stream, " 0x%016I64x_%016llx, clock 0x%08llx\n", 
									reg_table->x_reg[ROB->q[i].rd].data_H[ROB->q[i].rd_ptr], reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr], clock);
								break;
							case 4:
								fprintf(debug_stream, "LBU xr%02d.%01x (%s),  ", ROB->q[i].rd, ROB->q[i].rd_ptr, reg_table->x_reg[ROB->q[i].rd].name);
								fprint_addr_asm(debug_stream, &ROB->q[i]);
								fprintf(debug_stream, " addr, data = ");
								fprint_addr_coma(debug_stream, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr] + ROB->q[i].imm, param);
								fprintf(debug_stream, "  %02llx, clock 0x%08llx\n", reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr], clock);
								break;
							case 5:
								fprintf(debug_stream, "LHU xr%02d.%01x (%s),  ", ROB->q[i].rd, ROB->q[i].rd_ptr, reg_table->x_reg[ROB->q[i].rd].name);
								fprint_addr_asm(debug_stream, &ROB->q[i]);
								fprintf(debug_stream, " addr, data = ");
								fprint_addr_coma(debug_stream, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr] + ROB->q[i].imm, param);
								fprintf(debug_stream, " %04llx, clock 0x%08llx\n", reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr], clock);
								break;
							case 6:
								fprintf(debug_stream, "LWU xr%02d.%01x (%s),  ", ROB->q[i].rd, ROB->q[i].rd_ptr, reg_table->x_reg[ROB->q[i].rd].name);
								fprint_addr_asm(debug_stream, &ROB->q[i]);
								fprintf(debug_stream, " addr, data = ");
								fprint_addr_coma(debug_stream, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr] + ROB->q[i].imm, param);
								fprintf(debug_stream, " %08llx, clock 0x%08llx\n", reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr], clock);
								break;
							case 7:
								fprintf(debug_stream, "LDU xr%02d.%01x (%s),  ", ROB->q[i].rd, ROB->q[i].rd_ptr, reg_table->x_reg[ROB->q[i].rd].name);
								fprint_addr_asm(debug_stream, &ROB->q[i]);
								fprintf(debug_stream, " addr, data = ");
								fprint_addr_coma(debug_stream, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr] + ROB->q[i].imm, param);
								fprintf(debug_stream, "  %016llx, clock 0x%08llx\n", reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr], clock);
								break;
							case 12:
								fprintf(debug_stream, "LQU xr%02d.%01x (%s),  ", ROB->q[i].rd, ROB->q[i].rd_ptr, reg_table->x_reg[ROB->q[i].rd].name);
								fprint_addr_asm(debug_stream, &ROB->q[i]);
								fprintf(debug_stream, " addr, data = ");
								fprint_addr_coma(debug_stream, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr] + ROB->q[i].imm, param);
								fprintf(debug_stream, " 0x%016I64x_%016llx, clock 0x%08llx\n", 
									reg_table->x_reg[ROB->q[i].rd].data_H[ROB->q[i].rd_ptr], reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr], clock);
								break;
							default:
								debug++;
								break;
							}
						}
						retire->PC += ROB->q[i].bytes;
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

						reg_table->x_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;

						for (UINT index = ((reg_table->x_reg[ROB->q[i].rd].current_ptr + 1) & (reg_rename_size - 1)); index != reg_table->x_reg[ROB->q[i].rd].retire_ptr; index = ((index + 1) & (reg_rename_size - 1)))
							reg_table->x_reg[ROB->q[i].rd].valid[index] = reg_free;
					}
								 break;
					case STORE_FP_map: {
						csr[csr_mstatus].value |= (3 << 13);
						csr[csr_hpmcounter10].value++;
						switch (retire->priviledge) {
						case 0:
							break;
						case 1:
							csr[csr_shpmcounter10].value++;
							break;
						case 2:
							csr[csr_hhpmcounter10].value++;
							break;
						case 3:
							csr[csr_mhpmcounter10].value++;
							break;
						default:
							debug++;
							break;
						}
						if (debug_core) {
							fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", 
								mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
							fprint_addr_coma(debug_stream, retire->PC, param);
							switch (ROB->q[i].funct3) {
							case 1:
								fprintf(debug_stream, "FSH fp%02d.%x,  ", ROB->q[i].rs2, ROB->q[i].rs2_ptr);
								break;
							case 2:
								fprintf(debug_stream, "FSW fp%02d.%x,  ", ROB->q[i].rs2, ROB->q[i].rs2_ptr);
								break;
							case 3:
								fprintf(debug_stream, "FSD fp%02d.%x,  ", ROB->q[i].rs2, ROB->q[i].rs2_ptr);
								break;
							case 4:
								fprintf(debug_stream, "FSQ fp%02d.%x,  ", ROB->q[i].rs2, ROB->q[i].rs2_ptr);
								break;
							default:
								debug++;
								break;
							}
							if (ROB->q[i].rs1 == 2) {
								fprintf(debug_stream, "%#05x(sp xr02.%x) ", ROB->q[i].imm, ROB->q[i].rs1_ptr);
							}
							else if (ROB->q[i].rs1 == 3) {
								fprintf(debug_stream, "%#05x(gp) ", ROB->q[i].imm);
							}
							else if (ROB->q[i].rs1 == 4) {
								fprintf(debug_stream, "%#05x(tp) ", ROB->q[i].imm);
							}
							else {
								fprintf(debug_stream, "%#05x(xr%02d.%x) ", ROB->q[i].imm, ROB->q[i].rs1, ROB->q[i].rs1_ptr);
							}
							fprintf(debug_stream, " stored addr, data = ", reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr] + ROB->q[i].imm, reg_table->f_reg[ROB->q[i].rs2].data[ROB->q[i].rs2_ptr]);
							fprint_addr_coma(debug_stream, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr] + ROB->q[i].imm, param);
							print_fp_data(debug_stream, reg_table->f_reg[ROB->q[i].rs2].data[ROB->q[i].rs2_ptr], &ROB->q[i]);

							fprintf(debug_stream, " clock 0x%08llx\n", clock);
						}
						retire->PC += ROB->q[i].bytes;
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

						for (UINT index = ((reg_table->f_reg[ROB->q[i].rd].current_ptr + 1) & (reg_rename_size - 1)); index != reg_table->f_reg[ROB->q[i].rd].retire_ptr; index = ((index + 1) & (reg_rename_size - 1)))
							reg_table->f_reg[ROB->q[i].rd].valid[index] = reg_free;
					}
									 break;
					case LOAD_FP_map: {
						csr[csr_mstatus].value |= (3 << 13);
						csr[csr_hpmcounter8].value++;
						switch (retire->priviledge) {
						case 0:
							break;
						case 1:
							csr[csr_shpmcounter8].value++;
							break;
						case 2:
							csr[csr_hhpmcounter8].value++;
							break;
						case 3:
							csr[csr_mhpmcounter8].value++;
							break;
						default:
							debug++;
							break;
						}
						if (debug_core) {
							fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
							fprint_addr_coma(debug_stream, retire->PC, param);
							switch (ROB->q[i].funct3) {
							case 1:
								fprintf(debug_stream, "FLH fp%02d.%x,  ", ROB->q[i].rd, ROB->q[i].rd_ptr);
								break;
							case 2:
								fprintf(debug_stream, "FLW fp%02d.%x,  ", ROB->q[i].rd, ROB->q[i].rd_ptr);
								break;
							case 3:
								fprintf(debug_stream, "FLD fp%02d.%x,  ", ROB->q[i].rd, ROB->q[i].rd_ptr);
								break;
							case 4:
								fprintf(debug_stream, "FLQ fp%02d.%x,  ", ROB->q[i].rd, ROB->q[i].rd_ptr);
								break;
							default:
								debug++;
								break;
							}
							if (ROB->q[i].rs1 == 2) {
								fprintf(debug_stream, "%#05x(sp) ", ROB->q[i].imm);
							}
							else if (ROB->q[i].rs1 == 3) {
								fprintf(debug_stream, "%#05x(gp) ", ROB->q[i].imm);
							}
							else if (ROB->q[i].rs1 == 4) {
								fprintf(debug_stream, "%#05x(tp) ", ROB->q[i].imm);
							}
							else {
								fprintf(debug_stream, "%#05x(xr%02d.%x) ", ROB->q[i].imm, ROB->q[i].rs1, ROB->q[i].rs1_ptr);
							}
							fprintf(debug_stream, " load addr, data = ");
							fprint_addr_coma(debug_stream, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr] + ROB->q[i].imm,  param);
							print_fp_data(debug_stream, reg_table->f_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr], &ROB->q[i]);
							fprintf(debug_stream, " clock 0x%08llx\n", clock);
						}
						retire->PC += ROB->q[i].bytes;
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

						reg_table->f_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;

						for (UINT index = ((reg_table->f_reg[ROB->q[i].rd].current_ptr + 1) & (reg_rename_size - 1)); index != reg_table->f_reg[ROB->q[i].rd].retire_ptr; index = ((index + 1) & (reg_rename_size - 1)))
							reg_table->f_reg[ROB->q[i].rd].valid[index] = reg_free;
					}
									break;
					case AMO_map: {
						if (ROB->q[i].uop == uop_SC) {
							if (debug_core) {
								fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ",
									mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
								fprint_addr_coma(debug_stream, retire->PC, param);
								if (ROB->q[i].funct3 == 2)
									fprintf(debug_stream, "SC.w rd = xr%02d.%x, rs2 = xr%02d.%x, ", ROB->q[i].rd, ROB->q[i].rd_ptr, ROB->q[i].rs2, ROB->q[i].rs2_ptr);
								else if (ROB->q[i].funct3 == 3)
									fprintf(debug_stream, "SC.d rd = xr%02d.%x, rs2 = xr%02d.%x, ", ROB->q[i].rd, ROB->q[i].rd_ptr, ROB->q[i].rs2, ROB->q[i].rs2_ptr);
								else
									debug++;
								if (ROB->q[i].rs1 == 2) 			fprintf(debug_stream, "%#05x(sp) ", ROB->q[i].imm);
								else if (ROB->q[i].rs1 == 3) 		fprintf(debug_stream, "%#05x(gp) ", ROB->q[i].imm);
								else if (ROB->q[i].rs1 == 4) 		fprintf(debug_stream, "%#05x(tp) ", ROB->q[i].imm);
								else 								fprintf(debug_stream, "%#05x(reg%02d) ", ROB->q[i].imm, ROB->q[i].rs1);
								if (ROB->q[i].funct3 == 2)
									fprintf(debug_stream, "rd (error code) = 0x%04I64x, addr/data = ", reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr]);
								else if (ROB->q[i].funct3 == 3)
									fprintf(debug_stream, "rd (error code) = 0x%08I64x, addr/data = ", reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr]);
								fprint_addr_coma(debug_stream, reg_table->x_reg[ROB->q[i].rs1].data[ROB->q[i].rs1_ptr], param);
								if (ROB->q[i].funct3 == 2)
									fprintf(debug_stream, " 0x%04I64x, clock 0x%08llx\n",  reg_table->x_reg[ROB->q[i].rs2].data[ROB->q[i].rs2_ptr], clock);
								else if (ROB->q[i].funct3 == 3)
									fprintf(debug_stream, " 0x%08I64x, clock 0x%08llx\n", reg_table->x_reg[ROB->q[i].rs2].data[ROB->q[i].rs2_ptr], clock);
							}
						}
						else {
							if (debug_core) {
								fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", 
									mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
								fprint_addr_coma(debug_stream, retire->PC, param);
								if (ROB->q[i].funct3 == 2)
									fprintf(debug_stream, "LR.w xr%02d.%x, ", ROB->q[i].rd, ROB->q[i].rd_ptr);
								else if (ROB->q[i].funct3 == 3)
									fprintf(debug_stream, "LR.d xr%02d.%x, ", ROB->q[i].rd, ROB->q[i].rd_ptr);
								else
									debug++;
								if (ROB->q[i].rs1 == 2) {
									fprintf(debug_stream, "%#05x(sp) ", ROB->q[i].imm);
								}
								else if (ROB->q[i].rs1 == 3) {
									fprintf(debug_stream, "%#05x(gp) ", ROB->q[i].imm);
								}
								else if (ROB->q[i].rs1 == 4) {
									fprintf(debug_stream, "%#05x(tp) ", ROB->q[i].imm);
								}
								else {
									fprintf(debug_stream, "%#05x(reg%02d) ", ROB->q[i].imm, ROB->q[i].rs1);
								}		
								if (ROB->q[i].funct3 == 2)
									fprintf(debug_stream, "rd = 0x%08I64x, ", reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr]);
								else if (ROB->q[i].funct3 == 3)
									fprintf(debug_stream, "rd = 0x%016I64x, ", reg_table->x_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr]);
								else
									debug++;
								fprintf(debug_stream, "clock 0x%08llx\n", clock);
							}
						}
						if (ROB->branch_start != ROB->q[i].branch_num)
							debug++;
						retire->PC += ROB->q[i].bytes;
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

						reg_table->x_reg[ROB->q[i].rd].valid[(ROB->q[i].rd_ptr - 1) & (reg_rename_size - 1)] = reg_free;
						reg_table->x_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;

						if (ROB->q[i].q_select == OP_q_select0 || ROB->q[i].q_select == OP_q_select1 || ROB->q[i].q_select == OP_q_select2 || ROB->q[i].q_select == OP_q_select3 || ROB->q[i].map == AMO_map) {// AMO fence bus transactions only
							debug++;
						}
					}
								break;
					case MISC_MEM_map: {
						if (debug_core) {
							fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
							fprint_addr_coma(debug_stream, retire->PC, param);
							fprintf(debug_stream, "FENCE.%d	// clock 0x%08llx\n", ROB->q[i].funct3, clock);
						}
						retire->PC = ROB->q[i].addr + ROB->q[i].bytes; // use address in case of fault handler
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);
						for (UINT8 k = 0; k < 0x20; k++) {
							for (UINT8 j = 0; j < 0x10; j++) {
								if (j != reg_table->x_reg[k].current_ptr)reg_table->x_reg[k].valid[j] = reg_free;
								if (j != reg_table->f_reg[k].current_ptr)reg_table->f_reg[k].valid[j] = reg_free;
							}
						}

						if (ROB->q[i].q_select == OP_q_select0 || ROB->q[i].q_select == OP_q_select1 || ROB->q[i].q_select == OP_q_select2 || ROB->q[i].q_select == OP_q_select3 || ROB->q[i].map == AMO_map) {// AMO fence bus transactions only
							debug++;
						}
					}
									 break;
					case OP_FP_map:
						switch (ROB->q[i].uop) {
						case uop_FCVTf2i:
							if (debug_core) {
								fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ",
									mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
								fprint_addr_coma(debug_stream, retire->PC, param);
								fprintf(debug_stream, "FCVTf2i xr%02d.%x, fp%02d.%x,", ROB->q[i].rd, ROB->q[i].rd_ptr, ROB->q[i].rs1, ROB->q[i].rs1_ptr);
								fprintf(debug_stream, " clock 0x%08llx\n", clock);
							}
							retire->PC += ROB->q[i].bytes;
							if (ROB->allocate_ptr == ROB->retire_ptr_out)
								ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
							ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

							reg_table->x_reg[ROB->q[i].rd].valid[(ROB->q[i].rd_ptr - 1) & (reg_rename_size - 1)] = reg_free;
							reg_table->x_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
							//							reg_table->x_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;

							if (ROB->q[i].q_select == OP_q_select0 || ROB->q[i].q_select == OP_q_select1 || ROB->q[i].q_select == OP_q_select2 || ROB->q[i].q_select == OP_q_select3 || ROB->q[i].map == AMO_map) {// AMO fence bus transactions only
								debug++;
							}
							break;
						case uop_FCVTi2f:
							if (debug_core) {
								fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", 
									mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
								fprint_addr_coma(debug_stream, retire->PC, param);
								fprintf(debug_stream, "FCVTi2f fp%02d.%x, xr%02d.%x,", ROB->q[i].rd, ROB->q[i].rd_ptr, ROB->q[i].rs1, ROB->q[i].rs1_ptr);
								fprintf(debug_stream, " clock 0x%08llx\n", clock);
							}
							retire->PC += ROB->q[i].bytes;
							if (ROB->allocate_ptr == ROB->retire_ptr_out)
								ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
							ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

							reg_table->f_reg[ROB->q[i].rd].valid[(ROB->q[i].rd_ptr - 1) & (reg_rename_size - 1)] = reg_free;
							reg_table->f_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
							//							reg_table->f_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;

							if (ROB->q[i].q_select == OP_q_select0 || ROB->q[i].q_select == OP_q_select1 || ROB->q[i].q_select == OP_q_select2 || ROB->q[i].q_select == OP_q_select3 || ROB->q[i].map == AMO_map) {// AMO fence bus transactions only
								debug++;
							}
							break;
						case uop_FMVf2i:
							if (debug_core) {
								fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", 
									mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
								fprint_addr_coma(debug_stream, retire->PC, param);
								fprintf(debug_stream, "FMVf2i xr%02d.%x, fp%02d.%x,", ROB->q[i].rd, ROB->q[i].rd_ptr, ROB->q[i].rs1);
								fprintf(debug_stream, " clock 0x%08llx\n", clock);
							}
							retire->PC += ROB->q[i].bytes;
							if (ROB->allocate_ptr == ROB->retire_ptr_out)
								ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
							ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

							reg_table->x_reg[ROB->q[i].rd].valid[(ROB->q[i].rd_ptr - 1) & (reg_rename_size - 1)] = reg_free;
							reg_table->x_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
							//							reg_table->x_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;

							if (ROB->q[i].q_select == OP_q_select0 || ROB->q[i].q_select == OP_q_select1 || ROB->q[i].q_select == OP_q_select2 || ROB->q[i].q_select == OP_q_select3 || ROB->q[i].map == AMO_map) {// AMO fence bus transactions only
								debug++;
							}
							break;
						case uop_FMVi2f:
							if (debug_core) {
								fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ",
									mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
								fprint_addr_coma(debug_stream, retire->PC, param);
								fprintf(debug_stream, "FMVi2f fp%02d.%x, xr%02d.%x,", ROB->q[i].rd, ROB->q[i].rd_ptr, ROB->q[i].rs1, ROB->q[i].rs1_ptr);
								fprintf(debug_stream, " clock 0x%08llx\n", clock);
							}
							retire->PC += ROB->q[i].bytes;
							if (ROB->allocate_ptr == ROB->retire_ptr_out)
								ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
							ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

							reg_table->f_reg[ROB->q[i].rd].valid[(ROB->q[i].rd_ptr - 1) & (reg_rename_size - 1)] = reg_free;
							reg_table->f_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
							//							reg_table->f_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;

							if (ROB->q[i].q_select == OP_q_select0 || ROB->q[i].q_select == OP_q_select1 || ROB->q[i].q_select == OP_q_select2 || ROB->q[i].q_select == OP_q_select3 || ROB->q[i].map == AMO_map) {// AMO fence bus transactions only
								debug++;
							}
							break;
						default:
							if (debug_core) {
								fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
								fprint_addr_coma(debug_stream, retire->PC, param);
								fprintf(debug_stream, "OP_FP fp%02d.%x, fp%02d.%x, fp%02d.%x,", ROB->q[i].rd, ROB->q[i].rd_ptr, ROB->q[i].rs1, ROB->q[i].rs1_ptr, ROB->q[i].rs2, ROB->q[i].rs2_ptr);
								switch (ROB->q[i].funct7&3) {
								case 0://32b
									fprintf(debug_stream, " rd = 0x%08x", reg_table->f_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr]);
									break;
								case 1://64b
									fprintf(debug_stream, " rd = 0x%016llx", reg_table->f_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr]);
									break;
								case 2://16b
									fprintf(debug_stream, " rd = 0x%04x", reg_table->f_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr]);
									break;
								case 3://128b
									fprintf(debug_stream, " rd = 0x%016llx_%016llx", reg_table->f_reg[ROB->q[i].rd].data_H[ROB->q[i].rd_ptr], reg_table->f_reg[ROB->q[i].rd].data[ROB->q[i].rd_ptr]);
									break;
								default:
									debug++;
									break;
								}
								fprintf(debug_stream, " clock 0x%08llx\n", clock);
							}
							retire->PC += ROB->q[i].bytes;
							if (ROB->allocate_ptr == ROB->retire_ptr_out)
								ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
							ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

							reg_table->f_reg[ROB->q[i].rd].valid[(ROB->q[i].rd_ptr - 1) & (reg_rename_size - 1)] = reg_free;
							reg_table->f_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
							//							reg_table->f_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;

							if (ROB->q[i].q_select == OP_q_select0 || ROB->q[i].q_select == OP_q_select1 || ROB->q[i].q_select == OP_q_select2 || ROB->q[i].q_select == OP_q_select3 || ROB->q[i].map == AMO_map) {// AMO fence bus transactions only
								debug++;
							}
							break;
						}
						break;
					case MADD_map: {
						if (debug_core) {
							fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", 
								mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
							fprint_addr_coma(debug_stream, retire->PC, param);
							fprintf(debug_stream, "FMADD fp%02d.%x, fp%02d.%x, fp%02d.%x, fp%02d.%x,", ROB->q[i].rd, ROB->q[i].rd_ptr, ROB->q[i].rs1, ROB->q[i].rs1_ptr, ROB->q[i].rs2, ROB->q[i].rs2_ptr, ROB->q[i].rs3, ROB->q[i].rs3_ptr);
							fprintf(debug_stream, " clock 0x%08llx\n", clock);
						}
						retire->PC += ROB->q[i].bytes;
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

						reg_table->f_reg[ROB->q[i].rd].valid[(ROB->q[i].rd_ptr - 1) & (reg_rename_size - 1)] = reg_free;
						reg_table->f_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
						//						reg_table->f_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;

						if (ROB->q[i].q_select == OP_q_select0 || ROB->q[i].q_select == OP_q_select1 || ROB->q[i].q_select == OP_q_select2 || ROB->q[i].q_select == OP_q_select3 || ROB->q[i].map == AMO_map) {// AMO fence bus transactions only
							debug++;
						}
					}
								 break;
					case MSUB_map: {
						if (debug_core) {
							fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ",
								mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
							fprint_addr_coma(debug_stream, retire->PC, param);
							fprintf(debug_stream, "FMSUB fp%02d.%x, fp%02d.%x, fp%02d.%x, fp%02d.%x,", ROB->q[i].rd, ROB->q[i].rd_ptr, ROB->q[i].rs1, ROB->q[i].rs1_ptr, ROB->q[i].rs2, ROB->q[i].rs2_ptr, ROB->q[i].rs3, ROB->q[i].rs3_ptr);
							fprintf(debug_stream, " clock 0x%08llx\n", clock);
						}
						retire->PC += ROB->q[i].bytes;
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

						reg_table->f_reg[ROB->q[i].rd].valid[(ROB->q[i].rd_ptr - 1) & (reg_rename_size - 1)] = reg_free;
						reg_table->f_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
						//						reg_table->f_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;

						if (ROB->q[i].q_select == OP_q_select0 || ROB->q[i].q_select == OP_q_select1 || ROB->q[i].q_select == OP_q_select2 || ROB->q[i].q_select == OP_q_select3 || ROB->q[i].map == AMO_map) {// AMO fence bus transactions only
							debug++;
						}
					}
								 break;
					case NMADD_map: {
						if (debug_core) {
							fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ",
								mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
							fprint_addr_coma(debug_stream, retire->PC, param);
							fprintf(debug_stream, "FNMADD fp%02d.%x, fp%02d.%x, fp%02d.%x, fp%02d.%x,", ROB->q[i].rd, ROB->q[i].rd_ptr, ROB->q[i].rs1, ROB->q[i].rs1_ptr, ROB->q[i].rs2, ROB->q[i].rs2_ptr, ROB->q[i].rs3, ROB->q[i].rs3_ptr);
							fprintf(debug_stream, " clock 0x%08llx\n", clock);
						}
						retire->PC += ROB->q[i].bytes;
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

						reg_table->f_reg[ROB->q[i].rd].valid[(ROB->q[i].rd_ptr - 1) & (reg_rename_size - 1)] = reg_free;
						reg_table->f_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;

						if (ROB->q[i].q_select == OP_q_select0 || ROB->q[i].q_select == OP_q_select1 || ROB->q[i].q_select == OP_q_select2 || ROB->q[i].q_select == OP_q_select3 || ROB->q[i].map == AMO_map) {// AMO fence bus transactions only
							debug++;
						}
					}
								  break;
					case NMSUB_map: {
						if (debug_core) {
							fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", 
								mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
							fprint_addr_coma(debug_stream, retire->PC, param);
							fprintf(debug_stream, "FNMSUB fp%02d.%x, fp%02d.%x, fp%02d.%x, fp%02d.%x,", ROB->q[i].rd, ROB->q[i].rd_ptr, ROB->q[i].rs1, ROB->q[i].rs1_ptr, ROB->q[i].rs2, ROB->q[i].rs2_ptr, ROB->q[i].rs3, ROB->q[i].rs3_ptr);
							fprintf(debug_stream, " clock 0x%08llx\n", clock);
						}
						retire->PC += ROB->q[i].bytes;
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

						reg_table->f_reg[ROB->q[i].rd].valid[(ROB->q[i].rd_ptr - 1) & (reg_rename_size - 1)] = reg_free;
						reg_table->f_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;
						//						reg_table->f_reg[ROB->q[i].rd].valid[ROB->q[i].rd_ptr] = reg_valid_out;

						if (ROB->q[i].q_select == OP_q_select0 || ROB->q[i].q_select == OP_q_select1 || ROB->q[i].q_select == OP_q_select2 || ROB->q[i].q_select == OP_q_select3 || ROB->q[i].map == AMO_map) {// AMO fence bus transactions only
							debug++;
						}
					}
								  break;
					default: {
						if (debug_core) {
							fprintf(debug_stream, "RETIRE(%lld): ROB entry: 0x%02x 0x%02x-0x%02x, ", 
								mhartid, ROB->retire_ptr_out, ROB->retire_ptr_out, ROB->decode_ptr);
							fprint_addr_coma(debug_stream, retire->PC, param);
							print_uop2(debug_stream, ROB->q[i], reg_table, param, clock);
							fprintf(debug_stream, " clock 0x%08llx\n", clock);
						}
						retire->PC += ROB->q[i].bytes;
						if (ROB->allocate_ptr == ROB->retire_ptr_out)
							ROB->allocate_ptr = ((ROB->allocate_ptr + 1) & 0x00ff);
						ROB->retire_ptr_out = ((ROB->retire_ptr_out + 1) & 0x00ff);

						reg_table->x_reg[ROB->q[i].rd].valid[(ROB->q[i].rd_ptr - 1) & (reg_rename_size - 1)] = reg_free;
						reg_table->x_reg[ROB->q[i].rd].retire_ptr = ROB->q[i].rd_ptr;

						if (ROB->q[i].q_select == OP_q_select0 || ROB->q[i].q_select == OP_q_select1 || ROB->q[i].q_select == OP_q_select2 || ROB->q[i].q_select == OP_q_select3 || ROB->q[i].map == AMO_map) {// AMO fence bus transactions only
							debug++;
						}
					}
						   break;
					}
					for (UINT8 j = 0; j < 0x20; j++) {
						if (reg_table->x_reg[j].current_ptr >= 0x20)
							debug++;
						if (reg_table->x_reg[j].retire_ptr >= 0x20)
							debug++;
					}
				}
				ROB->q[i].state = ROB_free;
				ROB->q[i].q_ptr = 0;
				ROB->q[i].q_ptr_valid = 0;
			}
			else
				count = param->decode_width; // used to enforce in order retirement
		}
		// zero out reg 0
		reg_table->x_reg[0].current_ptr = reg_table->x_reg[0].retire_ptr = 0;
		for (UINT16 j = 0; j < (reg_rename_size - 1); j++) {
			reg_table->x_reg[0].data[j] = 0;
			reg_table->x_reg[0].valid[j] = reg_valid_in;
		}
		for (UINT8 q_index = ROB->decode_ptr; q_index != ROB->retire_ptr_in; q_index = ((q_index + 1) & 0x00ff))
			ROB->q[q_index].state = ROB_free;
	}
}