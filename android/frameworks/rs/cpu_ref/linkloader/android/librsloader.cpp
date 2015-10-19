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

#include "librsloader.h"

#include "ELFObject.h"
#include "ELFSectionSymTab.h"
#include "ELFSymbol.h"

#include "utils/serialize.h"

#define LOG_TAG "bcc"
#include "cutils/log.h"

#include <llvm/Support/ELF.h>

#if defined(__LP64__) || defined(__x86_64__)
static inline RSExecRef wrap(ELFObject<64> *object) {
  return reinterpret_cast<RSExecRef>(object);
}
#else
static inline RSExecRef wrap(ELFObject<32> *object) {
  return reinterpret_cast<RSExecRef>(object);
}
#endif

#if defined(__LP64__) || defined(__x86_64__)
static inline ELFObject<64> *unwrap(RSExecRef object) {
  return reinterpret_cast<ELFObject<64> *>(object);
}
#else
static inline ELFObject<32> *unwrap(RSExecRef object) {
  return reinterpret_cast<ELFObject<32> *>(object);
}
#endif

extern "C" RSExecRef rsloaderCreateExec(unsigned char const *buf,
                                        size_t buf_size,
                                        RSFindSymbolFn find_symbol,
                                        void *find_symbol_context) {
  RSExecRef object = rsloaderLoadExecutable(buf, buf_size);
  if (!object) {
    return NULL;
  }

  if (!rsloaderRelocateExecutable(object, find_symbol, find_symbol_context)) {
    rsloaderDisposeExec(object);
    return NULL;
  }

  return object;
}

extern "C" RSExecRef rsloaderLoadExecutable(unsigned char const *buf,
                                            size_t buf_size) {
  ArchiveReaderLE AR(buf, buf_size);

#if defined(__LP64__) || defined(__x86_64__)
  std::unique_ptr<ELFObject<64> > object(ELFObject<64>::read(AR));
#else
  std::unique_ptr<ELFObject<32> > object(ELFObject<32>::read(AR));
#endif
  if (!object) {
    ALOGE("Unable to load the ELF object.");
    return NULL;
  }

  return wrap(object.release());
}

extern "C" int rsloaderRelocateExecutable(RSExecRef object_,
                                          RSFindSymbolFn find_symbol,
                                          void *find_symbol_context) {
#if defined(__LP64__) || defined(__x86_64__)
  ELFObject<64>* object = unwrap(object_);
#else
  ELFObject<32>* object = unwrap(object_);
#endif
  object->relocate(find_symbol, find_symbol_context);
  return (object->getMissingSymbols() == 0);
}

extern "C" void rsloaderUpdateSectionHeaders(RSExecRef object_,
                                             unsigned char *buf) {
#if defined(__LP64__) || defined(__x86_64__)
  ELFObject<64> *object = unwrap(object_);
#else
  ELFObject<32> *object = unwrap(object_);
#endif

  // Remap the section header addresses to match the loaded code
#if defined(__LP64__) || defined(__x86_64__)
  llvm::ELF::Elf64_Ehdr* header = reinterpret_cast<llvm::ELF::Elf64_Ehdr*>(buf);
#else
  llvm::ELF::Elf32_Ehdr* header = reinterpret_cast<llvm::ELF::Elf32_Ehdr*>(buf);
#endif

#if defined(__LP64__) || defined(__x86_64__)
  llvm::ELF::Elf64_Shdr* shtab =
      reinterpret_cast<llvm::ELF::Elf64_Shdr*>(buf + header->e_shoff);
#else
  llvm::ELF::Elf32_Shdr* shtab =
      reinterpret_cast<llvm::ELF::Elf32_Shdr*>(buf + header->e_shoff);
#endif

  for (int i = 0; i < header->e_shnum; i++) {
    if (shtab[i].sh_flags & SHF_ALLOC) {
#if defined(__LP64__) || defined(__x86_64__)
      ELFSectionBits<64>* bits =
          static_cast<ELFSectionBits<64>*>(object->getSectionByIndex(i));
#else
      ELFSectionBits<32>* bits =
          static_cast<ELFSectionBits<32>*>(object->getSectionByIndex(i));
#endif
      if (bits) {
        const unsigned char* addr = bits->getBuffer();
#if defined(__LP64__) || defined(__x86_64__)
        shtab[i].sh_addr = reinterpret_cast<llvm::ELF::Elf64_Addr>(addr);
#else
        shtab[i].sh_addr = reinterpret_cast<llvm::ELF::Elf32_Addr>(addr);
#endif
      }
    }
  }
}

extern "C" void rsloaderDisposeExec(RSExecRef object) {
  delete unwrap(object);
}

extern "C" void *rsloaderGetSymbolAddress(RSExecRef object_,
                                          char const *name) {
#if defined(__LP64__) || defined(__x86_64__)
  ELFObject<64> *object = unwrap(object_);

  ELFSectionSymTab<64> *symtab =
    static_cast<ELFSectionSymTab<64> *>(object->getSectionByName(".symtab"));
#else
  ELFObject<32> *object = unwrap(object_);

  ELFSectionSymTab<32> *symtab =
    static_cast<ELFSectionSymTab<32> *>(object->getSectionByName(".symtab"));
#endif

  if (!symtab) {
    return NULL;
  }

#if defined(__LP64__) || defined(__x86_64__)
  ELFSymbol<64> *symbol = symtab->getByName(name);
#else
  ELFSymbol<32> *symbol = symtab->getByName(name);
#endif

  if (!symbol) {
    ALOGV("Symbol not found: %s\n", name);
    return NULL;
  }

  int machine = object->getHeader()->getMachine();

  return symbol->getAddress(machine, false);
}

extern "C" size_t rsloaderGetSymbolSize(RSExecRef object_, char const *name) {
#if defined(__LP64__) || defined(__x86_64__)
  ELFObject<64> *object = unwrap(object_);

  ELFSectionSymTab<64> *symtab =
    static_cast<ELFSectionSymTab<64> *>(object->getSectionByName(".symtab"));
#else
  ELFObject<32> *object = unwrap(object_);

  ELFSectionSymTab<32> *symtab =
    static_cast<ELFSectionSymTab<32> *>(object->getSectionByName(".symtab"));
#endif
  if (!symtab) {
    return 0;
  }

#if defined(__LP64__) || defined(__x86_64__)
  ELFSymbol<64> *symbol = symtab->getByName(name);
#else
  ELFSymbol<32> *symbol = symtab->getByName(name);
#endif

  if (!symbol) {
    ALOGV("Symbol not found: %s\n", name);
    return 0;
  }

  return (size_t)symbol->getSize();
}

extern "C" size_t rsloaderGetFuncCount(RSExecRef object) {
#if defined(__LP64__) || defined(__x86_64__)
  ELFSectionSymTab<64> *symtab = static_cast<ELFSectionSymTab<64> *>(
#else
  ELFSectionSymTab<32> *symtab = static_cast<ELFSectionSymTab<32> *>(
#endif
    unwrap(object)->getSectionByName(".symtab"));

  if (!symtab) {
    return 0;
  }

  return symtab->getFuncCount();
}

extern "C" void rsloaderGetFuncNameList(RSExecRef object,
                                        size_t size,
                                        char const **list) {
#if defined(__LP64__) || defined(__x86_64__)
  ELFSectionSymTab<64> *symtab = static_cast<ELFSectionSymTab<64> *>(
#else
  ELFSectionSymTab<32> *symtab = static_cast<ELFSectionSymTab<32> *>(
#endif
    unwrap(object)->getSectionByName(".symtab"));

  if (symtab) {
    symtab->getFuncNameList(size, list);
  }
}
