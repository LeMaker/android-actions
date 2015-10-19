/*
 * Copyright 2011-2012, The Android Open Source Project
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

#ifndef ELF_OBJECT_HXX
#define ELF_OBJECT_HXX

#include <android/log.h>

#include "ELFHeader.h"
#include "ELFReloc.h"
#include "ELFSection.h"
#include "ELFSectionHeaderTable.h"
#include "StubLayout.h"
#include "GOT.h"
#include "ELF.h"

#include <llvm/ADT/SmallVector.h>

#include "utils/rsl_assert.h"


template <unsigned Bitwidth>
template <typename Archiver>
inline ELFObject<Bitwidth> *
ELFObject<Bitwidth>::read(Archiver &AR) {
  std::unique_ptr<ELFObjectTy> object(new ELFObjectTy());

  // Read header
  object->header.reset(ELFHeaderTy::read(AR));
  if (!object->header) {
    return 0;
  }

  // Read section table
  object->shtab.reset(ELFSectionHeaderTableTy::read(AR, object.get()));
  if (!object->shtab) {
    return 0;
  }

  // Read each section
  llvm::SmallVector<size_t, 4> progbits_ndx;
  for (size_t i = 0; i < object->header->getSectionHeaderNum(); ++i) {
    if ((*object->shtab)[i]->getType() == SHT_PROGBITS) {
      object->stab.push_back(NULL);
      progbits_ndx.push_back(i);
    } else {
      std::unique_ptr<ELFSectionTy> sec(
        ELFSectionTy::read(AR, object.get(), (*object->shtab)[i]));
      object->stab.push_back(sec.release());
    }
  }

  object->shtab->buildNameMap();
  ELFSectionSymTabTy *symtab =
    static_cast<ELFSectionSymTabTy *>(object->getSectionByName(".symtab"));
  rsl_assert(symtab && "Symtab is required.");
  symtab->buildNameMap();

  for (size_t i = 0; i < progbits_ndx.size(); ++i) {
    size_t index = progbits_ndx[i];

    std::unique_ptr<ELFSectionTy> sec(
      ELFSectionTy::read(AR, object.get(), (*object->shtab)[index]));
    object->stab[index] = sec.release();
  }

  return object.release();
}

template <unsigned Bitwidth>
inline char const *ELFObject<Bitwidth>::getSectionName(size_t i) const {
  ELFSectionTy const *sec = stab[header->getStringSectionIndex()];

  if (sec) {
    ELFSectionStrTabTy const &st =
      static_cast<ELFSectionStrTabTy const &>(*sec);
    return st[i];
  }

  return NULL;
}

template <unsigned Bitwidth>
inline ELFSection<Bitwidth> const *
ELFObject<Bitwidth>::getSectionByIndex(size_t i) const {
  return stab[i];
}

template <unsigned Bitwidth>
inline ELFSection<Bitwidth> *
ELFObject<Bitwidth>::getSectionByIndex(size_t i) {
  return stab[i];
}

template <unsigned Bitwidth>
inline ELFSection<Bitwidth> const *
ELFObject<Bitwidth>::getSectionByName(std::string const &str) const {
  size_t idx = getSectionHeaderTable()->getByName(str)->getIndex();
  return stab[idx];
}

template <unsigned Bitwidth>
inline ELFSection<Bitwidth> *
ELFObject<Bitwidth>::getSectionByName(std::string const &str) {
  ELFObjectTy const *const_this = this;
  ELFSectionTy const *sptr = const_this->getSectionByName(str);
  // Const cast for the same API's const and non-const versions.
  return const_cast<ELFSectionTy *>(sptr);
}


template <unsigned Bitwidth>
inline void ELFObject<Bitwidth>::
relocateARM(void *(*find_sym)(void *context, char const *name),
            void *context,
            ELFSectionRelTableTy *reltab,
            ELFSectionProgBitsTy *text) {
  // FIXME: Should be implement in independent files.
  rsl_assert(Bitwidth == 32 && "ARM only have 32 bits.");

  ELFSectionSymTabTy *symtab =
    static_cast<ELFSectionSymTabTy *>(getSectionByName(".symtab"));
  rsl_assert(symtab && "Symtab is required.");

  for (size_t i = 0; i < reltab->size(); ++i) {
    // FIXME: Can not implement here, use Fixup!
    ELFRelocTy *rel = (*reltab)[i];
    ELFSymbolTy *sym = (*symtab)[rel->getSymTabIndex()];

    // FIXME: May be not uint32_t *.
    typedef int32_t Inst_t;
    Inst_t *inst = (Inst_t *)&(*text)[rel->getOffset()];
    Inst_t P = (Inst_t)(int64_t)inst;
    Inst_t A = 0;
    Inst_t S = (Inst_t)(int64_t)sym->getAddress(EM_ARM);
    Inst_t T = 0;

    if (sym->isConcreteFunc() && (sym->getValue() & 0x1)) {
      T = 1;
    }
    relinfo_t reltype = rel->getType();
    switch (reltype) {
    default:
      rsl_assert(0 && "Not implemented relocation type.");
      break;

    case R_ARM_ABS32:
      {
        if (S == 0 && sym->getType() == STT_NOTYPE) {
          void *ext_sym = find_sym(context, sym->getName());
          if (!ext_sym) {
            missingSymbols = true;
          }
          S = (Inst_t)(uintptr_t)ext_sym;
          sym->setAddress(ext_sym);
        }
        A = *inst;
        *inst = (S + A) | T;
      }
      break;

      // FIXME: Predefine relocation codes.
    case R_ARM_CALL:
    case R_ARM_THM_CALL:
    case R_ARM_JUMP24:
    case R_ARM_THM_JUMP24:
      {
#define SIGN_EXTEND(x, l) (((x)^(1<<((l)-1)))-(1<<(l-1)))
        if (reltype == R_ARM_CALL || reltype == R_ARM_JUMP24) {
          A = (Inst_t)(int64_t)SIGN_EXTEND(*inst & 0xFFFFFF, 24);
          A <<= 2;
        } else {
          // Hack for two 16bit.
          *inst = ((*inst >> 16) & 0xFFFF) | (*inst << 16);
          Inst_t s  = (*inst >> 26) & 0x1u,    // 26
                 u  = (*inst >> 16) & 0x3FFu,  // 25-16
                 l  =  *inst        & 0x7FFu, // 10-0
                 j1 = (*inst >> 13) & 0x1u,    // 13
                 j2 = (*inst >> 11) & 0x1u;    // 11
          Inst_t i1 = (~(j1 ^ s)) & 0x1u,
                 i2 = (~(j2 ^ s)) & 0x1u;
          // [31-25][24][23][22][21-12][11-1][0]
          //      0   s  i1  i2      u     l  0
          A = SIGN_EXTEND((s << 23) | (i1 << 22) | (i2 << 21) | (u << 11) | l, 24);
          A <<= 1;
        }
#undef SIGN_EXTEND

        void *callee_addr = sym->getAddress(EM_ARM);

        switch (sym->getType()) {
        default:
          rsl_assert(0 && "Wrong type for R_ARM_CALL relocation.");
          abort();
          break;

        case STT_FUNC:
          // NOTE: Callee function is in the object file, but it may be
          // in different PROGBITS section (which may be far call).

          if (callee_addr == 0) {
            rsl_assert(0 && "We should get function address at previous "
                   "sym->getAddress(EM_ARM) function call.");
            abort();
          }
          break;

        case STT_NOTYPE:
          // NOTE: Callee function is an external function.  Call find_sym
          // if it has not resolved yet.

          if (callee_addr == 0) {
            callee_addr = find_sym(context, sym->getName());
            if (!callee_addr) {
              missingSymbols = true;
            }
            sym->setAddress(callee_addr);
          }
          break;
        }

        // Get the stub for this function
        StubLayout *stub_layout = text->getStubLayout();

        if (!stub_layout) {
          llvm::errs() << "unable to get stub layout." << "\n";
          abort();
        }

        void *stub = stub_layout->allocateStub(callee_addr);

        if (!stub) {
          llvm::errs() << "unable to allocate stub." << "\n";
          abort();
        }

        //LOGI("Function %s: using stub %p\n", sym->getName(), stub);
        S = (uint32_t)(uintptr_t)stub;

        if (reltype == R_ARM_CALL || reltype == R_ARM_JUMP24) {
          // Relocate the R_ARM_CALL relocation type
          uint32_t result = (S + A - P) >> 2;

          if (result > 0x007FFFFF && result < 0xFF800000) {
            rsl_assert(0 && "Stub is still too far");
            abort();
          }

          *inst = ((result) & 0x00FFFFFF) | (*inst & 0xFF000000);
        } else {
          P &= ~0x3;  // Base address align to 4 bytes.  (For BLX.)

          // Relocate the R_ARM_THM_CALL relocation type
          uint32_t result = (S + A - P) >> 1;

          if (result > 0x007FFFFF && result < 0xFF800000) {
            rsl_assert(0 && "Stub is still too far");
            abort();
          }

          //*inst &= 0xF800D000u;
          // Rewrite instruction to BLX.  (Stub is always ARM.)
          *inst &= 0xF800C000u;
          // [31-25][24][23][22][21-12][11-1][0]
          //      0   s  i1  i2      u     l  0
          Inst_t s  = (result >> 23) & 0x1u,   // 26
                 u  = (result >> 11) & 0x3FFu, // 25-16
                 // For BLX, bit [0] is 0.
                 l  =  result        & 0x7FEu, // 10-0
                 i1 = (result >> 22) & 0x1u,
                 i2 = (result >> 21) & 0x1u;
          Inst_t j1 = ((~i1) ^ s) & 0x01u,       // 13
                 j2 = ((~i2) ^ s) & 0x01u;       // 11
          *inst |= s << 26;
          *inst |= u << 16;
          *inst |= l;
          *inst |= j1 << 13;
          *inst |= j2 << 11;
          // Hack for two 16bit.
          *inst = ((*inst >> 16) & 0xFFFF) | (*inst << 16);
        }
      }
      break;
    case R_ARM_MOVT_ABS:
    case R_ARM_MOVW_ABS_NC:
    case R_ARM_THM_MOVW_ABS_NC:
    case R_ARM_THM_MOVT_ABS:
      {
        if (S == 0 && sym->getType() == STT_NOTYPE) {
          void *ext_sym = find_sym(context, sym->getName());
          if (!ext_sym) {
            missingSymbols = true;
          }
          S = (Inst_t)(uintptr_t)ext_sym;
          sym->setAddress(ext_sym);
        }
        if (reltype == R_ARM_MOVT_ABS
            || reltype == R_ARM_THM_MOVT_ABS) {
          S >>= 16;
        }

        if (reltype == R_ARM_MOVT_ABS
            || reltype == R_ARM_MOVW_ABS_NC) {
          // No need sign extend.
          A = ((*inst & 0xF0000) >> 4) | (*inst & 0xFFF);
          uint32_t result = (S + A);
          *inst = (((result) & 0xF000) << 4) |
            ((result) & 0xFFF) |
            (*inst & 0xFFF0F000);
        } else {
          // Hack for two 16bit.
          *inst = ((*inst >> 16) & 0xFFFF) | (*inst << 16);
          // imm16: [19-16][26][14-12][7-0]
          A = (((*inst >>  4) & 0xF000u) |
               ((*inst >> 15) & 0x0800u) |
               ((*inst >>  4) & 0x0700u) |
               ( *inst        & 0x00FFu));
          uint32_t result;
          if (reltype == R_ARM_THM_MOVT_ABS) {
            result = (S + A);
          } else {
            result = (S + A) | T;
          }
          // imm16: [19-16][26][14-12][7-0]
          *inst &= 0xFBF08F00u;
          *inst |= (result & 0xF000u) << 4;
          *inst |= (result & 0x0800u) << 15;
          *inst |= (result & 0x0700u) << 4;
          *inst |= (result & 0x00FFu);
          // Hack for two 16bit.
          *inst = ((*inst >> 16) & 0xFFFF) | (*inst << 16);
        }
      }
      break;
    }
    //llvm::errs() << "S:     " << (void *)S << '\n';
    //llvm::errs() << "A:     " << (void *)A << '\n';
    //llvm::errs() << "P:     " << (void *)P << '\n';
    //llvm::errs() << "S+A:   " << (void *)(S+A) << '\n';
    //llvm::errs() << "S+A-P: " << (void *)(S+A-P) << '\n';
  }
}

template <unsigned Bitwidth>
inline void ELFObject<Bitwidth>::
relocateAARCH64(void *(*find_sym)(void *context, char const *name),
            void *context,
            ELFSectionRelTableTy *reltab,
            ELFSectionProgBitsTy *text) {
  // FIXME: Should be implement in independent files.
  rsl_assert(Bitwidth == 64 && "AARCH64 only have 64 bits.");

  // Change this to true to enable some debugging in the log.
  const bool kDebugSymbolPrint = false;

  ELFSectionSymTabTy *symtab =
    static_cast<ELFSectionSymTabTy *>(getSectionByName(".symtab"));
  rsl_assert(symtab && "Symtab is required.");

  for (size_t i = 0; i < reltab->size(); ++i) {
    ELFRelocTy *rel = (*reltab)[i];
    ELFSymbolTy *sym = (*symtab)[rel->getSymTabIndex()];

    typedef int64_t Inst_t;
    Inst_t *inst = (Inst_t *)&(*text)[rel->getOffset()];
    int32_t* inst32 = reinterpret_cast<int32_t*>(inst);
    int16_t* inst16 = reinterpret_cast<int16_t*>(inst);
    Inst_t P = (Inst_t)(int64_t)inst;
    Inst_t A = 0;
    Inst_t S = (Inst_t)(int64_t)sym->getAddress(EM_ARM);
    Inst_t Page_P = P & ~0xfff;         // Page address.

    if (S == 0 && sym->getType() == STT_NOTYPE) {
      void *ext_sym = find_sym(context, sym->getName());
      if (!ext_sym) {
        missingSymbols = true;
      }
      S = (Inst_t)(uintptr_t)ext_sym;
      sym->setAddress(ext_sym);
    }

    if (kDebugSymbolPrint) {
      __android_log_print(ANDROID_LOG_INFO, "rs", "AARCH64 relocation symbol %s value is %llx\n",
          sym->getName(), (unsigned long long)S);
    }

    // TODO: add other relocations when we know what ones are used.
    relinfo_t reltype = rel->getType();
    switch (reltype) {
    default:
      __android_log_print(ANDROID_LOG_ERROR, "rs",
        "Unimplemented AARCH64 relocation type %d(0x%x)\n", static_cast<uint32_t>(reltype),
        static_cast<uint32_t>(reltype));
      rsl_assert(0 && "Unimplemented relocation type.");
      break;

    case R_AARCH64_ABS64:
        A = *inst + rel->getAddend();
        *inst = S + A;
      break;

    case R_AARCH64_ABS32:
        A = *inst + rel->getAddend();
        *inst32 = static_cast<int32_t>(S + A);
      break;

    case R_AARCH64_ABS16:
        A = *inst + rel->getAddend();
        *inst16 = static_cast<int16_t>(S + A);
      break;

    case R_AARCH64_PREL64:
        A = *inst + rel->getAddend();
        *inst = S + A - P;
      break;

    case R_AARCH64_PREL32:
        A = *inst32 + rel->getAddend();
        *inst32 = static_cast<int32_t>(S + A - P);
      break;

    case R_AARCH64_PREL16:
        A = *inst16 + rel->getAddend();
        *inst16 = static_cast<int16_t>(S + A - P);
      break;

    case R_AARCH64_ADR_PREL_PG_HI21:
      // Relocate an ADRP instruction to the page
      {
        A = rel->getAddend();
        int32_t immed = ((S + A) & ~0xfff) - Page_P;
        immed >>= 12;
        uint32_t immlo = immed & 0b11;              // 2 bits.
        uint32_t immhi = (immed >> 2) & 0x7FFFF;   // 19 bits.
        *inst32 |= static_cast<int32_t>(immlo << 29 | immhi << 5);
      }
      break;

    case R_AARCH64_ADR_PREL_LO21:
      {
        A = rel->getAddend();
        int32_t immed = S + A - P;
        uint32_t immlo = immed & 0b11;              // 2 bits.
        uint32_t immhi = (immed >> 2) & 0x7FFFF;   // 19 bits.
        *inst32 |= static_cast<int32_t>(immlo << 29 | immhi << 5);
      }
      break;

    case R_AARCH64_ADD_ABS_LO12_NC:
      // ADD instruction immediate value.
      {
        A = rel->getAddend();
        int32_t immed = S + A;
        uint32_t imm12 = (immed & 0xFFF);   // 12 bits.
        *inst32 |= static_cast<int32_t>(imm12 << 10);
      }
      break;

    case R_AARCH64_LDST8_ABS_LO12_NC:
    case R_AARCH64_LDST16_ABS_LO12_NC:
    case R_AARCH64_LDST32_ABS_LO12_NC:
    case R_AARCH64_LDST64_ABS_LO12_NC:
    case R_AARCH64_LDST128_ABS_LO12_NC:
      {
        // Set LD/ST (unsigned) immediate instruction to the low 12 bits, shifted depending
        // on relocation.
        A = rel->getAddend();
        uint32_t shift = 0;
        switch (reltype) {
          case R_AARCH64_LDST8_ABS_LO12_NC: shift = 0; break;
          case R_AARCH64_LDST16_ABS_LO12_NC: shift = 1; break;
          case R_AARCH64_LDST32_ABS_LO12_NC: shift = 2; break;
          case R_AARCH64_LDST64_ABS_LO12_NC: shift = 3; break;
          case R_AARCH64_LDST128_ABS_LO12_NC: shift = 4; break;
          default:
            rsl_assert("Cannot reach");
        }

        // Form imm12 by taking 12 bits and shifting by appropriate amount.
        uint32_t imm12 = ((S + A) & 0xFFF) >> shift;

        // Put it into the instruction.
        *inst32 |= static_cast<int32_t>(imm12 << 10);
      }
      break;

    case R_AARCH64_CALL26:
    case R_AARCH64_JUMP26:
      {
#define SIGN_EXTEND(x, l) (((x)^(1<<((l)-1)))-(1<<(l-1)))
        A = (Inst_t)(int64_t)SIGN_EXTEND(*inst32 & 0x3FFFFFF, 26);
        A <<= 2;
#undef SIGN_EXTEND

        void *callee_addr = sym->getAddress(EM_AARCH64);
        bool call_via_stub = false;     // Call via a stub (linker veneer).

        switch (sym->getType()) {
        default:
          rsl_assert(0 && "Wrong type for R_ARM_CALL relocation.");
          abort();
          break;

        case STT_FUNC:
          // NOTE: Callee function is in the object file, but it may be
          // in different PROGBITS section (which may be far call).

          if (callee_addr == 0) {
            rsl_assert(0 && "We should get function address at previous "
                   "sym->getAddress(EM_ARM) function call.");
            abort();
          }
          break;

        case STT_NOTYPE:
          // NOTE: Callee function is an external function.  Call find_sym
          // if it has not resolved yet.

          if (callee_addr == 0) {
            callee_addr = find_sym(context, sym->getName());
            if (!callee_addr) {
              missingSymbols = true;
            }
            sym->setAddress(callee_addr);
          }
          break;
        }

        S = reinterpret_cast<int64_t>(callee_addr);
        uint32_t result = (S + A - P) >> 2;

        // See if we can do the branch without a stub.
        if (result > 0x01FFFFFF && result < 0xFE000000) {
          // Not in range, need a stub.
          call_via_stub = true;
        }

        // Calling via a stub makes a BL instruction to a stub containing the following code:
        // ldr x16, addr
        // br x16
        // addr:
        // .word low32
        // .word high32
        //
        // This loads the PC value from the 64 bits at PC + 8.  Since AARCH64 can't
        // manipulate the PC directly we have to load a register and branch to the contents.
        if (call_via_stub) {
          // Get the stub for this function
          StubLayout *stub_layout = text->getStubLayout();

          if (!stub_layout) {
            __android_log_print(ANDROID_LOG_ERROR, "rs", "unable to get stub layout\n");
            llvm::errs() << "unable to get stub layout." << "\n";
            abort();
          }

          void *stub = stub_layout->allocateStub(callee_addr);

          if (!stub) {
            __android_log_print(ANDROID_LOG_ERROR, "rs", "unable to allocate stub\n");
            llvm::errs() << "unable to allocate stub." << "\n";
            abort();
          }

          //LOGI("Function %s: using stub %p\n", sym->getName(), stub);
          S = (uint64_t)(uintptr_t)stub;

          result = (S + A - P) >> 2;

          if (result > 0x01FFFFFF && result < 0xFE000000) {
            __android_log_print(ANDROID_LOG_ERROR, "rs", "stub is still too far\n");
            rsl_assert(0 && "Stub is still too far");
            abort();
          }
        }

        // 'result' contains the offset from PC to the destination address, encoded
        // in the correct form for the BL or B instructions.
        *inst32 = (result & 0x03FFFFFF) | (*inst & 0xFC000000);
      }
      break;
    case R_AARCH64_MOVW_UABS_G0:
    case R_AARCH64_MOVW_UABS_G0_NC:
    case R_AARCH64_MOVW_UABS_G1:
    case R_AARCH64_MOVW_UABS_G1_NC:
    case R_AARCH64_MOVW_UABS_G2:
    case R_AARCH64_MOVW_UABS_G2_NC:
    case R_AARCH64_MOVW_UABS_G3:
      {
        int shift = 0;
        switch (reltype) {
        case R_AARCH64_MOVW_UABS_G0:
        case R_AARCH64_MOVW_UABS_G0_NC: shift = 0; break;
        case R_AARCH64_MOVW_UABS_G1:
        case R_AARCH64_MOVW_UABS_G1_NC: shift = 16; break;
        case R_AARCH64_MOVW_UABS_G2:
        case R_AARCH64_MOVW_UABS_G2_NC: shift = 32; break;
        case R_AARCH64_MOVW_UABS_G3: shift = 48; break;
        }

        A = (*inst32 >> 5) & 0xFFFF;
        uint32_t value = ((S + A) >> shift) & 0xFFFF;
        *inst32 = (*inst32 & ~(0xFFFF << 6)) | (value << 6);
      }
      break;

    case R_AARCH64_MOVW_SABS_G0:
    case R_AARCH64_MOVW_SABS_G1:
    case R_AARCH64_MOVW_SABS_G2:
      {
        int shift = 0;
        switch (reltype) {
        case R_AARCH64_MOVW_SABS_G0: shift = 0; break;
        case R_AARCH64_MOVW_SABS_G1: shift = 16; break;
        case R_AARCH64_MOVW_SABS_G2: shift = 32; break;
        }

        A = (*inst32 >> 5) & 0xFFFF;
        int32_t value = ((S + A) >> shift) & 0xFFFF;

        *inst32 = (*inst32 & ~(0xFFFF << 6)) | (value << 6);

        // This relocation type must also set the instruction bit 30 to 0 or 1
        // depending on the sign of the value.  The bit corresponds to the
        // movz or movn encoding.  A value of 0 means movn.
        if (value >= 0) {
          // Set the instruction to movz (set bit 30 to 1)
          *inst32 |= 0x40000000;
        } else {
          // Set the instruction to movn (set bit 30 to 0)
          *inst32 &= ~0x40000000;
        }
      }
      break;
    }
    //llvm::errs() << "S:     " << (void *)S << '\n';
    //llvm::errs() << "A:     " << (void *)A << '\n';
    //llvm::errs() << "P:     " << (void *)P << '\n';
    //llvm::errs() << "S+A:   " << (void *)(S+A) << '\n';
    //llvm::errs() << "S+A-P: " << (void *)(S+A-P) << '\n';
  }
}

template <unsigned Bitwidth>
inline void ELFObject<Bitwidth>::
relocateX86_64(void *(*find_sym)(void *context, char const *name),
               void *context,
               ELFSectionRelTableTy *reltab,
               ELFSectionProgBitsTy *text) {
  rsl_assert(Bitwidth == 64 && "Only support X86_64.");

  ELFSectionSymTabTy *symtab =
    static_cast<ELFSectionSymTabTy *>(getSectionByName(".symtab"));
  rsl_assert(symtab && "Symtab is required.");

  for (size_t i = 0; i < reltab->size(); ++i) {
    // FIXME: Can not implement here, use Fixup!
    ELFRelocTy *rel = (*reltab)[i];
    ELFSymbolTy *sym = (*symtab)[rel->getSymTabIndex()];

    typedef intptr_t Inst_t;
    Inst_t *inst = (Inst_t*)&(*text)[rel->getOffset()];
    Inst_t P = (Inst_t)inst;
    Inst_t A = (Inst_t)rel->getAddend();
    Inst_t S = (Inst_t)sym->getAddress(EM_X86_64);

    if (S == 0) {
      S = (Inst_t)find_sym(context, sym->getName());
      if (!S) {
        missingSymbols = true;
      }
      sym->setAddress((void *)S);
    }

    switch (rel->getType()) {
    default:
      rsl_assert(0 && "Not implemented relocation type.");
      break;

    // FIXME, consider other relocation types if RS support dynamic reolcations in future.
    case R_X86_64_64: {//Direct 64-bit.
      int64_t *paddr = (int64_t*)&(*text)[rel->getOffset()];
      int64_t vAddr = S + A;
      *paddr = vAddr;
      break;
    }
    case R_X86_64_PC32: {//PC relative 32-bit signed.
      int32_t *paddr = (int32_t*)&(*text)[rel->getOffset()];
      int64_t vOffset = S + A - P;

      if (vOffset > INT32_MAX || vOffset < INT32_MIN) {
        // Not in range, need a stub.
        StubLayout *stub_layout = text->getStubLayout();
        if (!stub_layout) {
          __android_log_print(ANDROID_LOG_ERROR, "rs", "unable to get stub layout\n");
          llvm::errs() << "unable to get stub layout." << "\n";
          abort();
        }

        void *stub = stub_layout->allocateStub((void *)S);

        if (!stub) {
          __android_log_print(ANDROID_LOG_ERROR, "rs", "unable to allocate stub\n");
          llvm::errs() << "unable to allocate stub." << "\n";
          abort();
        }

        S = (Inst_t)stub;
        vOffset = S + A - P;

        if (vOffset > INT32_MAX || vOffset < INT32_MIN) {
          __android_log_print(ANDROID_LOG_ERROR, "rs", "stub is still too far\n");
          rsl_assert(0 && "Stub is still too far");
          abort();
        }
      }

      rsl_assert(vOffset <= INT32_MAX && vOffset >= INT32_MIN);
      *paddr = (int32_t)(vOffset & 0xFFFFFFFF);
      break;
    }
    case R_X86_64_32: {//Direct 32-bit zero-extended.
      uint32_t *paddr = (uint32_t*)&(*text)[rel->getOffset()];
      int64_t vAddr = S + A;
      rsl_assert(vAddr <= UINT32_MAX);
      *paddr = (uint32_t)(vAddr & 0xFFFFFFFF);
      break;
    }
    case R_X86_64_32S: {//Direct 32-bit sign-extended.
      int32_t *paddr = (int32_t*)&(*text)[rel->getOffset()];
      int64_t vAddr = S + A;
      rsl_assert(vAddr <= INT32_MAX && vAddr >= INT32_MIN);
      *paddr = (uint32_t)(vAddr & 0xFFFFFFFF);
      break;
    }
    case R_X86_64_16: {//Direct 16-bit zero-extended.
      uint16_t *paddr = (uint16_t*)&(*text)[rel->getOffset()];
      int64_t vAddr = S + A;
      rsl_assert(vAddr <= UINT16_MAX);
      *paddr = (uint16_t)(vAddr & 0xFFFF);
      break;
    }
    case R_X86_64_PC16: {//16-bit sign-extended PC relative.
      int16_t *paddr = (int16_t*)&(*text)[rel->getOffset()];
      int64_t vOffset = S + A - P;
      rsl_assert(vOffset <= INT16_MAX && vOffset >= INT16_MIN);
      *paddr = (int16_t)(vOffset & 0xFFFF);
      break;
    }
    case R_X86_64_8: {//Direct 8-bit sign-extended.
      int8_t *paddr = (int8_t*)&(*text)[rel->getOffset()];
      int64_t vAddr = S + A;
      rsl_assert(vAddr <= INT8_MAX && vAddr >= INT8_MIN);
      *paddr = (uint8_t)(vAddr & 0xFF);
      break;
    }
    case R_X86_64_PC8: {//8-bit sign-extended PC relative.
      int8_t *paddr = (int8_t*)&(*text)[rel->getOffset()];
      int64_t vOffset = S + A - P;
      rsl_assert(vOffset <= INT8_MAX && vOffset >= INT8_MIN);
      *paddr = (int8_t)(vOffset & 0xFF);
      break;
    }
    case R_X86_64_PC64: {//PC relative 64-bit.
      int64_t *paddr = (int64_t*)&(*text)[rel->getOffset()];
      *paddr = (int64_t)(S + A - P);
      break;
    }
    }
  }
}

template <unsigned Bitwidth>
inline void ELFObject<Bitwidth>::
relocateX86_32(void *(*find_sym)(void *context, char const *name),
               void *context,
               ELFSectionRelTableTy *reltab,
               ELFSectionProgBitsTy *text) {
  rsl_assert(Bitwidth == 32 && "Only support X86.");

  ELFSectionSymTabTy *symtab =
    static_cast<ELFSectionSymTabTy *>(getSectionByName(".symtab"));
  rsl_assert(symtab && "Symtab is required.");

  for (size_t i = 0; i < reltab->size(); ++i) {
    // FIXME: Can not implement here, use Fixup!
    ELFRelocTy *rel = (*reltab)[i];
    ELFSymbolTy *sym = (*symtab)[rel->getSymTabIndex()];

    typedef intptr_t Inst_t;
    Inst_t *inst = (Inst_t *)&(*text)[rel->getOffset()];
    Inst_t P = (Inst_t)inst;
    Inst_t A = (Inst_t)*inst;
    Inst_t S = (Inst_t)sym->getAddress(EM_386);

    if (S == 0) {
      S = (Inst_t)find_sym(context, sym->getName());
      if (!S) {
        missingSymbols = true;
      }
      sym->setAddress((void *)S);
    }

    switch (rel->getType()) {
    default:
      rsl_assert(0 && "Not implemented relocation type.");
      break;

    case R_386_PC32: {//Add PC-relative symbol value.
      int32_t *paddr = (int32_t*)&(*text)[rel->getOffset()];
      *paddr = (int32_t)(S + A - P);
      break;
    }
    case R_386_32: {//Add symbol value.
      uint32_t *paddr = (uint32_t*)&(*text)[rel->getOffset()];
      *paddr = (uint32_t)(S + A);
      break;
    }
    }
  }
}

template <unsigned Bitwidth>
inline void ELFObject<Bitwidth>::
relocateMIPS(void *(*find_sym)(void *context, char const *name),
             void *context,
             ELFSectionRelTableTy *reltab,
             ELFSectionProgBitsTy *text) {
  rsl_assert(Bitwidth == 32 && "Only support 32-bit MIPS.");

  ELFSectionSymTabTy *symtab =
    static_cast<ELFSectionSymTabTy *>(getSectionByName(".symtab"));
  rsl_assert(symtab && "Symtab is required.");

  for (size_t i = 0; i < reltab->size(); ++i) {
    // FIXME: Can not implement here, use Fixup!
    ELFRelocTy *rel = (*reltab)[i];
    ELFSymbolTy *sym = (*symtab)[rel->getSymTabIndex()];

    typedef int32_t Inst_t;
    Inst_t *inst = (Inst_t *)&(*text)[rel->getOffset()];
    Inst_t P = (Inst_t)(uintptr_t)inst;
    Inst_t A = (Inst_t)(uintptr_t)*inst;
    Inst_t S = (Inst_t)(uintptr_t)sym->getAddress(EM_MIPS);

    bool need_stub = false;

    if (S == 0 && strcmp (sym->getName(), "_gp_disp") != 0) {
      need_stub = true;
      S = (Inst_t)(uintptr_t)find_sym(context, sym->getName());
      if (!S) {
        missingSymbols = true;
      }
#if defined(__LP64__) || defined(__x86_64__)
      llvm::errs() << "Code temporarily disabled for 64bit build";
      abort();
#else
      sym->setAddress((void *)S);
#endif
    }

    switch (rel->getType()) {
    default:
      rsl_assert(0 && "Not implemented relocation type.");
      break;

    case R_MIPS_NONE:
    case R_MIPS_JALR: // ignore this
      break;

    case R_MIPS_16:
      *inst &= 0xFFFF0000;
      A = A & 0xFFFF;
      A = S + (short)A;
      rsl_assert(A >= -32768 && A <= 32767 && "R_MIPS_16 overflow.");
      *inst |= (A & 0xFFFF);
      break;

    case R_MIPS_32:
      *inst = S + A;
      break;

    case R_MIPS_26:
      *inst &= 0xFC000000;
      if (need_stub == false) {
        A = (A & 0x3FFFFFF) << 2;
        if (sym->getBindingAttribute() == STB_LOCAL) { // local binding
          A |= ((P + 4) & 0xF0000000);
          A += S;
          *inst |= ((A >> 2) & 0x3FFFFFF);
        } else { // external binding
          if (A & 0x08000000) // Sign extend from bit 27
            A |= 0xF0000000;
          A += S;
          *inst |= ((A >> 2) & 0x3FFFFFF);
          if (((P + 4) >> 28) != (A >> 28)) { // far local call
#if defined(__LP64__) || defined(__x86_64__)
            llvm::errs() << "Code temporarily disabled for 64bit build";
            abort();
            void* stub = NULL;
#else
            void *stub = text->getStubLayout()->allocateStub((void *)A);
#endif
            rsl_assert(stub && "cannot allocate stub.");
            sym->setAddress(stub);
            S = (int32_t)(intptr_t)stub;
            *inst |= ((S >> 2) & 0x3FFFFFF);
            rsl_assert(((P + 4) >> 28) == (S >> 28) && "stub is too far.");
          }
        }
      } else { // shared-library call
        A = (A & 0x3FFFFFF) << 2;
        rsl_assert(A == 0 && "R_MIPS_26 addend is not zero.");
#if defined(__LP64__) || defined(__x86_64__)
        llvm::errs() << "Code temporarily disabled for 64bit build";
        abort();
        void* stub = NULL;
#else
        void *stub = text->getStubLayout()->allocateStub((void *)S);
#endif
        rsl_assert(stub && "cannot allocate stub.");
        sym->setAddress(stub);
        S = (int32_t)(intptr_t)stub;
        *inst |= ((S >> 2) & 0x3FFFFFF);
        rsl_assert(((P + 4) >> 28) == (S >> 28) && "stub is too far.");
      }
      break;

    case R_MIPS_HI16:
      *inst &= 0xFFFF0000;
      A = (A & 0xFFFF) << 16;
      // Find the nearest LO16 relocation type after this entry
      for (size_t j = i + 1; j < reltab->size(); j++) {
        ELFRelocTy *this_rel = (*reltab)[j];
        ELFSymbolTy *this_sym = (*symtab)[this_rel->getSymTabIndex()];
        if (this_rel->getType() == R_MIPS_LO16 && this_sym == sym) {
          Inst_t *this_inst = (Inst_t *)&(*text)[this_rel->getOffset()];
          Inst_t this_A = (Inst_t)(uintptr_t)*this_inst;
          this_A = this_A & 0xFFFF;
          A += (short)this_A;
          break;
        }
      }
      if (strcmp (sym->getName(), "_gp_disp") == 0) {
          S = (int)(intptr_t)got_address() + GP_OFFSET - (int)P;
#if defined(__LP64__) || defined(__x86_64__)
          llvm::errs() << "Code temporarily disabled for 64bit build";
          abort();
#else
          sym->setAddress((void *)S);
#endif
      }
      *inst |= (((S + A + (int)0x8000) >> 16) & 0xFFFF);
      break;

    case R_MIPS_LO16:
      *inst &= 0xFFFF0000;
      A = A & 0xFFFF;
      if (strcmp (sym->getName(), "_gp_disp") == 0) {
          S = (Inst_t)(intptr_t)sym->getAddress(EM_MIPS);
      }
      *inst |= ((S + A) & 0xFFFF);
      break;

    case R_MIPS_GOT16:
    case R_MIPS_CALL16:
      {
        *inst &= 0xFFFF0000;
        A = A & 0xFFFF;
        if (rel->getType() == R_MIPS_GOT16) {
          if (sym->getBindingAttribute() == STB_LOCAL) {
            A <<= 16;

            // Find the nearest LO16 relocation type after this entry
            for (size_t j = i + 1; j < reltab->size(); j++) {
              ELFRelocTy *this_rel = (*reltab)[j];
              ELFSymbolTy *this_sym = (*symtab)[this_rel->getSymTabIndex()];
              if (this_rel->getType() == R_MIPS_LO16 && this_sym == sym) {
                Inst_t *this_inst = (Inst_t *)&(*text)[this_rel->getOffset()];
                Inst_t this_A = (Inst_t)(uintptr_t)*this_inst;
                this_A = this_A & 0xFFFF;
                A += (short)this_A;
                break;
              }
            }
          } else {
            rsl_assert(A == 0 && "R_MIPS_GOT16 addend is not 0.");
          }
        } else { // R_MIPS_CALL16
          rsl_assert(A == 0 && "R_MIPS_CALL16 addend is not 0.");
        }
#if defined(__LP64__) || defined(__x86_64__)
        llvm::errs() << "Code temporarily disabled for 64bit build";
        abort();
        int got_index = 0;
#else
        int got_index = search_got((int)rel->getSymTabIndex(), (void *)(S + A),
                                   sym->getBindingAttribute());
#endif
        int got_offset = (got_index << 2) - GP_OFFSET;
        *inst |= (got_offset & 0xFFFF);
      }
      break;

    case R_MIPS_GPREL32:
      *inst = A + S - ((int)(intptr_t)got_address() + GP_OFFSET);
      break;
    }
  }
}


// TODO: Refactor all relocations.
template <unsigned Bitwidth>
inline void ELFObject<Bitwidth>::
relocate(void *(*find_sym)(void *context, char const *name), void *context) {
  // Init SHNCommonDataSize.
  // Need refactoring
  size_t SHNCommonDataSize = 0;

  ELFSectionSymTabTy *symtab =
    static_cast<ELFSectionSymTabTy *>(getSectionByName(".symtab"));
  rsl_assert(symtab && "Symtab is required.");

  for (size_t i = 0; i < symtab->size(); ++i) {
    ELFSymbolTy *sym = (*symtab)[i];

    if (sym->getType() != STT_OBJECT) {
      continue;
    }

    size_t idx = (size_t)sym->getSectionIndex();
    switch (idx) {
    default:
      if ((*shtab)[idx]->getType() == SHT_NOBITS) {
        // FIXME(logan): This is a workaround for .lcomm directives
        // bug of LLVM ARM MC code generator.  Remove this when the
        // LLVM bug is fixed.

        size_t align = 16;
        SHNCommonDataSize += (size_t)sym->getSize() + align;
      }
      break;

    case SHN_COMMON:
      {
        size_t align = (size_t)sym->getValue();
        SHNCommonDataSize += (size_t)sym->getSize() + align;
      }
      break;

    case SHN_ABS:
    case SHN_UNDEF:
    case SHN_XINDEX:
      break;
    }
  }
  if (!initSHNCommonDataSize(SHNCommonDataSize)) {
    rsl_assert("Allocate memory for common variable fail!");
    // TODO: Refactor object loading to use proper status/error returns.
    // We mark the object as having missing symbols and return early in this
    // case to signal a loading error (usually due to running out of
    // available memory to allocate).
    missingSymbols = true;
    return;
  }

  for (size_t i = 0; i < stab.size(); ++i) {
    ELFSectionHeaderTy *sh = (*shtab)[i];
    if (sh->getType() != SHT_REL && sh->getType() != SHT_RELA) {
      continue;
    }
    ELFSectionRelTableTy *reltab =
      static_cast<ELFSectionRelTableTy *>(stab[i]);
    rsl_assert(reltab && "Relocation section can't be NULL.");

    const char *reltab_name = sh->getName();
    const char *need_rel_name;
    if (sh->getType() == SHT_REL) {
      need_rel_name = reltab_name + 4;
      // ".rel.xxxx"
      //      ^ start from here.
    } else {
      need_rel_name = reltab_name + 5;
    }

    // TODO: We currently skip relocations of ARM unwind information, because
    // it is unused.
    if (!strcmp(".ARM.exidx", need_rel_name)) {
      continue;
    }

    ELFSectionProgBitsTy *need_rel =
      static_cast<ELFSectionProgBitsTy *>(getSectionByName(need_rel_name));
    rsl_assert(need_rel && "Need be relocated section can't be NULL.");

    switch (getHeader()->getMachine()) {
      case EM_ARM:
        relocateARM(find_sym, context, reltab, need_rel);
        break;
      case EM_AARCH64:
        relocateAARCH64(find_sym, context, reltab, need_rel);
        break;
      case EM_386:
        relocateX86_32(find_sym, context, reltab, need_rel);
        break;
      case EM_X86_64:
        relocateX86_64(find_sym, context, reltab, need_rel);
        break;
      case EM_MIPS:
        relocateMIPS(find_sym, context, reltab, need_rel);
        break;

      default:
        rsl_assert(0 && "Only support ARM, MIPS, X86, and X86_64 relocation.");
        break;
    }
  }

  for (size_t i = 0; i < stab.size(); ++i) {
    ELFSectionHeaderTy *sh = (*shtab)[i];
    if (sh->getType() == SHT_PROGBITS || sh->getType() == SHT_NOBITS) {
      if (stab[i]) {
        static_cast<ELFSectionBitsTy *>(stab[i])->protect();
      }
    }
  }
}

template <unsigned Bitwidth>
inline void ELFObject<Bitwidth>::print() const {
  header->print();
  shtab->print();

  for (size_t i = 0; i < stab.size(); ++i) {
    ELFSectionTy *sec = stab[i];
    if (sec) {
      sec->print();
    }
  }
}

#endif // ELF_OBJECT_HXX
