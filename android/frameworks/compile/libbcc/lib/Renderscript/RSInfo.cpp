/*
 * Copyright 2012, The Android Open Source Project
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

// #define LOG_NDEBUG 0
#include "bcc/Renderscript/RSInfo.h"

#if !defined(_WIN32)  /* TODO create a HAVE_DLFCN_H */
#include <dlfcn.h>
#endif

#include <cstring>
#include <new>
#include <string>

#include "bcc/Support/FileBase.h"
#include "bcc/Support/Log.h"

#ifdef HAVE_ANDROID_OS
#include <cutils/properties.h>
#endif

using namespace bcc;

android::String8 RSInfo::GetPath(const char *pFilename) {
  android::String8 result(pFilename);
  result.append(".info");
  return result;
}

static std::string stringFromSourceHash(const RSInfo::DependencyHashTy& hash) {
    std::string s;
    s.reserve(SHA1_DIGEST_LENGTH + 1);
    for (int i = 0; i < SHA1_DIGEST_LENGTH; i++) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02x", hash[i]);
        s.append(buf);
    }
    return s;
}

std::string bcc::getCommandLine(int argc, const char* const* argv) {
    std::string s;
    for (int i = 0; i < argc; i++) {
        if (i > 0) {
            s += ' ';
        }
        s += argv[i];
    }
    return s;
}

bool RSInfo::IsConsistent(const char* pInputFilename, const DependencyHashTy& expectedSourceHash,
                          const char* expectedCompileCommandLine,
                          const char* expectedBuildFingerprint) {
    if (::memcmp(mSourceHash, expectedSourceHash, SHA1_DIGEST_LENGTH) != 0) {
        ALOGD("Cache %s is dirty due to the source it depends on has been changed:",
              pInputFilename);
        ALOGD("expected: %s", stringFromSourceHash(expectedSourceHash).c_str());
        ALOGD("cached  : %s", stringFromSourceHash(mSourceHash).c_str());
        return false;
    }
    if (strcmp(expectedCompileCommandLine, mCompileCommandLine) != 0) {
        ALOGD("Cache %s is dirty because the command line used to compile it has changed:",
              pInputFilename);
        ALOGD("expected: %s", expectedCompileCommandLine);
        ALOGD("cached  : %s", mCompileCommandLine);
        return false;
    }
    if (strcmp(expectedBuildFingerprint, mBuildFingerprint) != 0) {
        ALOGD("Cache %s is dirty because the build fingerprint has changed:", pInputFilename);
        ALOGD("expected: %s", expectedBuildFingerprint);
        ALOGD("cached  : %s", mBuildFingerprint);
        return false;
    }
    return true;
}

RSInfo::RSInfo(size_t pStringPoolSize) : mStringPool(NULL) {
  ::memset(&mHeader, 0, sizeof(mHeader));

  ::memcpy(mHeader.magic, RSINFO_MAGIC, sizeof(mHeader.magic));
  ::memcpy(mHeader.version, RSINFO_VERSION, sizeof(mHeader.version));

  mHeader.headerSize = sizeof(mHeader);

  mHeader.pragmaList.itemSize = sizeof(rsinfo::PragmaItem);
  mHeader.objectSlotList.itemSize = sizeof(rsinfo::ObjectSlotItem);
  mHeader.exportVarNameList.itemSize = sizeof(rsinfo::ExportVarNameItem);
  mHeader.exportFuncNameList.itemSize = sizeof(rsinfo::ExportFuncNameItem);
  mHeader.exportForeachFuncList.itemSize = sizeof(rsinfo::ExportForeachFuncItem);

  if (pStringPoolSize > 0) {
    mHeader.strPoolSize = pStringPoolSize;
    mStringPool = new (std::nothrow) char [ mHeader.strPoolSize ];
    if (mStringPool == NULL) {
      ALOGE("Out of memory when allocate memory for string pool in RSInfo "
            "constructor (size: %u)!", mHeader.strPoolSize);
    }
    ::memset(mStringPool, 0, mHeader.strPoolSize);
  }
  mSourceHash = NULL;
  mCompileCommandLine = NULL;
  mBuildFingerprint = NULL;
}

RSInfo::~RSInfo() {
  delete [] mStringPool;
}

bool RSInfo::layout(off_t initial_offset) {
  mHeader.pragmaList.offset = initial_offset +
                              mHeader.headerSize +
                              mHeader.strPoolSize;
  mHeader.pragmaList.count = mPragmas.size();

#define AFTER(_list) ((_list).offset + (_list).itemSize * (_list).count)
  mHeader.objectSlotList.offset = AFTER(mHeader.pragmaList);
  mHeader.objectSlotList.count = mObjectSlots.size();

  mHeader.exportVarNameList.offset = AFTER(mHeader.objectSlotList);
  mHeader.exportVarNameList.count = mExportVarNames.size();

  mHeader.exportFuncNameList.offset = AFTER(mHeader.exportVarNameList);
  mHeader.exportFuncNameList.count = mExportFuncNames.size();

  mHeader.exportForeachFuncList.offset = AFTER(mHeader.exportFuncNameList);
  mHeader.exportForeachFuncList.count = mExportForeachFuncs.size();
#undef AFTER

  return true;
}

void RSInfo::dump() const {
  // Hide the codes to save the code size when debugging is disabled.
#if !LOG_NDEBUG

  // Dump header
  ALOGV("RSInfo Header:");
  ALOGV("\tIs threadable: %s", ((mHeader.isThreadable) ? "true" : "false"));
  ALOGV("\tHeader size: %u", mHeader.headerSize);
  ALOGV("\tString pool size: %u", mHeader.strPoolSize);

  if (mSourceHash == NULL) {
      ALOGV("Source hash: NULL!");
  } else {
      ALOGV("Source hash: %s", stringFromSourceHash(mSourceHash).c_str());
  }

  ALOGV("Compile Command Line: ", mCompileCommandLine ? mCompileCommandLine : "(NULL)");
  ALOGV("mBuildFingerprint: ", mBuildFingerprint ? mBuildFingerprint : "(NULL)");

#define DUMP_LIST_HEADER(_name, _header) do { \
  ALOGV(_name ":"); \
  ALOGV("\toffset: %u", (_header).offset);  \
  ALOGV("\t# of item: %u", (_header).count);  \
  ALOGV("\tsize of each item: %u", (_header).itemSize); \
} while (false)

  DUMP_LIST_HEADER("Pragma list", mHeader.pragmaList);
  for (PragmaListTy::const_iterator pragma_iter = mPragmas.begin(),
        pragma_end = mPragmas.end(); pragma_iter != pragma_end; pragma_iter++) {
    ALOGV("\tkey: %s, value: %s", pragma_iter->first, pragma_iter->second);
  }

  DUMP_LIST_HEADER("RS object slots", mHeader.objectSlotList);
  for (ObjectSlotListTy::const_iterator slot_iter = mObjectSlots.begin(),
          slot_end = mObjectSlots.end(); slot_iter != slot_end; slot_iter++) {
    ALOGV("slot: %u", *slot_iter);
  }

  DUMP_LIST_HEADER("RS export variables", mHeader.exportVarNameList);
  for (ExportVarNameListTy::const_iterator var_iter = mExportVarNames.begin(),
          var_end = mExportVarNames.end(); var_iter != var_end; var_iter++) {
    ALOGV("name: %s", *var_iter);
  }

  DUMP_LIST_HEADER("RS export functions", mHeader.exportFuncNameList);
  for (ExportFuncNameListTy::const_iterator func_iter = mExportFuncNames.begin(),
        func_end = mExportFuncNames.end(); func_iter != func_end; func_iter++) {
    ALOGV("name: %s", *func_iter);
  }

  DUMP_LIST_HEADER("RS foreach list", mHeader.exportForeachFuncList);
  for (ExportForeachFuncListTy::const_iterator
          foreach_iter = mExportForeachFuncs.begin(),
          foreach_end = mExportForeachFuncs.end(); foreach_iter != foreach_end;
          foreach_iter++) {
    ALOGV("name: %s, signature: %05x", foreach_iter->first,
                                       foreach_iter->second);
  }
#undef DUMP_LIST_HEADER

#endif // LOG_NDEBUG
  return;
}

const char *RSInfo::getStringFromPool(rsinfo::StringIndexTy pStrIdx) const {
  // String pool uses direct indexing. Ensure that the pStrIdx is within the
  // range.
  if (pStrIdx >= mHeader.strPoolSize) {
    ALOGE("String index #%u is out of range in string pool (size: %u)!",
          pStrIdx, mHeader.strPoolSize);
    return NULL;
  }
  return &mStringPool[ pStrIdx ];
}

rsinfo::StringIndexTy RSInfo::getStringIdxInPool(const char *pStr) const {
  // Assume we are on the flat memory architecture (i.e., the memory space is
  // continuous.)
  if ((mStringPool + mHeader.strPoolSize) < pStr) {
    ALOGE("String %s does not in the string pool!", pStr);
    return rsinfo::gInvalidStringIndex;
  }
  return (pStr - mStringPool);
}

RSInfo::FloatPrecision RSInfo::getFloatPrecisionRequirement() const {
  // Check to see if we have any FP precision-related pragmas.
  std::string relaxed_pragma("rs_fp_relaxed");
  std::string imprecise_pragma("rs_fp_imprecise");
  std::string full_pragma("rs_fp_full");
  bool relaxed_pragma_seen = false;
  bool full_pragma_seen = false;

  for (PragmaListTy::const_iterator pragma_iter = mPragmas.begin(),
           pragma_end = mPragmas.end(); pragma_iter != pragma_end;
       pragma_iter++) {
    const char *pragma_key = pragma_iter->first;
    if (!relaxed_pragma.compare(pragma_key)) {
      relaxed_pragma_seen = true;
    } else if (!imprecise_pragma.compare(pragma_key)) {
      ALOGW("rs_fp_imprecise is deprecated.  Assuming rs_fp_relaxed instead.");
      relaxed_pragma_seen = true;
    } else if (!full_pragma.compare(pragma_key)) {
      full_pragma_seen = true;
    }
  }

  if (relaxed_pragma_seen && full_pragma_seen) {
    ALOGE("Full and relaxed precision specified at the same time!");
  }
  RSInfo::FloatPrecision result = relaxed_pragma_seen ? FP_Relaxed : FP_Full;

#ifdef HAVE_ANDROID_OS
  // Provide an override for precsion via adb shell setprop
  // adb shell setprop debug.rs.precision rs_fp_full
  // adb shell setprop debug.rs.precision rs_fp_relaxed
  // adb shell setprop debug.rs.precision rs_fp_imprecise
  char precision_prop_buf[PROPERTY_VALUE_MAX];
  property_get("debug.rs.precision", precision_prop_buf, "");

  if (precision_prop_buf[0]) {
    if (!relaxed_pragma.compare(precision_prop_buf)) {
      ALOGI("Switching to RS FP relaxed mode via setprop");
      result = FP_Relaxed;
    } else if (!imprecise_pragma.compare(precision_prop_buf)) {
      ALOGW("Switching to RS FP relaxed mode via setprop. rs_fp_imprecise was specified but is "
              "deprecated ");
      result = FP_Relaxed;
    } else if (!full_pragma.compare(precision_prop_buf)) {
      ALOGI("Switching to RS FP full mode via setprop");
      result = FP_Full;
    } else {
      ALOGE("Unrecognized debug.rs.precision %s", precision_prop_buf);
    }
  }
#endif

  return result;
}
