/*
 * Copyright 2011, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "StubLayout.h"

#include "utils/flush_cpu_cache.h"
#include "utils/raw_ostream.h"
#include "utils/rsl_assert.h"

#include <stdint.h>
#include <stdlib.h>

StubLayout::StubLayout() : table(NULL), count(0) {
}

void StubLayout::initStubTable(unsigned char *table_, size_t count_) {
  table = table_;
  count = count_;
}

void *StubLayout::allocateStub(void *addr) {
  // Check if we have created this stub or not.
  std::map<void *, void *>::iterator index_iter = stub_index.find(addr);

  if (index_iter != stub_index.end()) {
    return index_iter->second;
  }

  // We have to create a new stub
  if (count == 0) {
    // No free stub slot is available
    return NULL;
  }

  // Initialize the stub
  unsigned char *stub = table;
  setStubAddress(stub, addr);
  stub_index.insert(std::make_pair(addr, stub));

  // Increase the free stub slot pointer
  table += getUnitStubSize();
  count--;

  return stub;
}

size_t StubLayout::calcStubTableSize(size_t count) const {
  return count * getUnitStubSize();
}

size_t StubLayoutAARCH64::getUnitStubSize() const {
  return 16;
}

void StubLayoutAARCH64::setStubAddress(void *stub_, void *addr) {
  uint8_t *stub = (uint8_t *)stub_;

  // First instruction:
  // ldr x16,[pc,#8]        LDR literal (pc relative)
  // +--+---+-+--+-------------------+-----+
  // |01|011|0|00| (#8 >> 2) = 10    |10000|
  // +--+---+-+--+-------------------+-----+
  // 0x58000050
  // Little endian.
  stub[0] = 0x50;
  stub[1] = 0x00;
  stub[2] = 0x00;
  stub[3] = 0x58;

  // Next Instruction:
  // br x16
  // +-------+--+--+-----+------+-----+-----+
  // |1101011|00|00|11111|000000|10000|00000|
  // +-------+--+--+-----+------+-----+-----+
  // 0xd61f0200

  stub += 4;
  stub[0] = 0x00;
  stub[1] = 0x02;
  stub[2] = 0x1f;
  stub[3] = 0xd6;

  // Now the absolute address (64 bits).
  uint64_t *target = reinterpret_cast<uint64_t*>(stub + 4);
  *target = reinterpret_cast<uint64_t>(addr);
}

size_t StubLayoutARM::getUnitStubSize() const {
  return 8;
}

void StubLayoutARM::setStubAddress(void *stub_, void *addr) {
  uint8_t *stub = (uint8_t *)stub_;
  stub[0] = 0x04; // ldr pc, [pc, #-4]
  stub[1] = 0xf0; // ldr pc, [pc, #-4]
  stub[2] = 0x1f; // ldr pc, [pc, #-4]
  stub[3] = 0xe5; // ldr pc, [pc, #-4]

  void **target = (void **)(stub + 4);
  *target = addr;
}

size_t StubLayoutMIPS::getUnitStubSize() const {
  return 16;
}

void StubLayoutMIPS::setStubAddress(void *stub_, void *addr) {
  uint32_t addr32 = (uint32_t)(uintptr_t)addr;
  uint16_t addr_hi16 = (addr32 >> 16) &  0xffff;
  uint16_t addr_lo16 = addr32 & 0xffff;

  uint32_t *stub = (uint32_t *)stub_;
  stub[0] = 0x3c190000ul | addr_hi16; // lui
  stub[1] = 0x37390000ul | addr_lo16; // ori
  stub[2] = 0x03200008ul; // jr (jump register)
  stub[3] = 0x00000000ul; // nop
}

size_t StubLayoutX86::getUnitStubSize() const {
  return 8;
}

void StubLayoutX86::setStubAddress(void *stub_, void *addr) {
  uint8_t *stub = (uint8_t *)stub_;
  stub[0] = 0xE9; // 32-bit pc-relative jump.
  void **target = (void **)(stub + 1);
  *target = addr;
}

size_t StubLayoutX86_64::getUnitStubSize() const {
  return 16;
}

void StubLayoutX86_64::setStubAddress(void *stub_, void *addr) {
  // x86 doesn't have proper register/mem to store the jump destination
  // use below instructions to jump to the specified address

  // jmp *0x0(%rip);       jump to the location which is stored in next instruction
  // addr;                 this is not a real instruction, just an address
  uint8_t *stub = (uint8_t*)stub_;
  stub[0] = 0xff;
  stub[1] = 0x25;
  stub[2] = 0x0;
  stub[3] = 0x0;
  stub[4] = 0x0;
  stub[5] = 0x0;
  uint64_t *target = reinterpret_cast<uint64_t*>(stub + 6);
  *target = reinterpret_cast<uint64_t>(addr);
}

