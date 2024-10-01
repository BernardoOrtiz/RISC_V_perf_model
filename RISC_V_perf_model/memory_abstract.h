// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

#pragma once

#include "internal_bus.h"
#include "compile_to_RISC_V.h"

void memory_abstract(UINT64* data, UINT64 address, UINT8 write_enable, memory_space_type* memory_space, UINT16 xaction_id, UINT64 Bclock);
