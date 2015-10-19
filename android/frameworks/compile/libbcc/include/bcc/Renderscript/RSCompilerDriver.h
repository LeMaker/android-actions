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

#ifndef BCC_RS_COMPILER_DRIVER_H
#define BCC_RS_COMPILER_DRIVER_H

#include "bcc/ExecutionEngine/CompilerRTSymbolResolver.h"
#include "bcc/ExecutionEngine/SymbolResolvers.h"
#include "bcc/ExecutionEngine/SymbolResolverProxy.h"
#include "bcc/Renderscript/RSInfo.h"
#include "bcc/Renderscript/RSCompiler.h"
#include "bcc/Renderscript/RSScript.h"

namespace bcc {

class BCCContext;
class CompilerConfig;
class RSCompilerDriver;
class RSExecutable;

// Type signature for dynamically loaded initialization of an RSCompilerDriver.
typedef void (*RSCompilerDriverInit_t) (bcc::RSCompilerDriver *);
// Name of the function that we attempt to dynamically load/execute.
#define RS_COMPILER_DRIVER_INIT_FN rsCompilerDriverInit

class RSCompilerDriver {
private:
  CompilerConfig *mConfig;
  RSCompiler mCompiler;

  // Are we compiling under an RS debug context with additional checks?
  bool mDebugContext;

  // Callback before linking with the runtime library.
  RSLinkRuntimeCallback mLinkRuntimeCallback;

  // Do we merge global variables on ARM using LLVM's optimization pass?
  // Disabling LLVM's global merge pass allows static globals to be correctly
  // emitted to ELF. This can result in decreased performance due to increased
  // register pressure, but it does make the resulting code easier to debug
  // and work with.
  bool mEnableGlobalMerge;

  // Setup the compiler config for the given script. Return true if mConfig has
  // been changed and false if it remains unchanged.
  bool setupConfig(const RSScript &pScript);

  // Compiles the provided bitcode, placing the binary at pOutputPath.
  // - If saveInfoFile is true, it also stores the RSInfo data in a file with a path derived from
  //   pOutputPath.
  // - pSourceHash and commandLineToEmbed are values to embed in the RSInfo for future cache
  //   invalidation decision.
  // - If pDumpIR is true, a ".ll" file will also be created.
  Compiler::ErrorCode compileScript(RSScript& pScript, const char* pScriptName,
                                    const char* pOutputPath, const char* pRuntimePath,
                                    const RSInfo::DependencyHashTy& pSourceHash,
                                    const char* commandLineToEmbed, bool saveInfoFile, bool pDumpIR);

public:
  RSCompilerDriver(bool pUseCompilerRT = true);
  ~RSCompilerDriver();

  RSCompiler *getCompiler() {
    return &mCompiler;
  }

  void setConfig(CompilerConfig *config) {
    mConfig = config;
  }

  void setDebugContext(bool v) {
    mDebugContext = v;
  }

  void setLinkRuntimeCallback(RSLinkRuntimeCallback c) {
    mLinkRuntimeCallback = c;
  }

  RSLinkRuntimeCallback getLinkRuntimeCallback() const {
    return mLinkRuntimeCallback;
  }

  // This function enables/disables merging of global static variables.
  // Note that it only takes effect on ARM architectures (other architectures
  // do not offer this option).
  void setEnableGlobalMerge(bool v) {
    mEnableGlobalMerge = v;
  }

  bool getEnableGlobalMerge() const {
    return mEnableGlobalMerge;
  }

  // FIXME: This method accompany with loadScript and compileScript should
  //        all be const-methods. They're not now because the getAddress() in
  //        SymbolResolverInterface is not a const-method.
  // Returns true if script is successfully compiled.
  bool build(BCCContext& pContext, const char* pCacheDir, const char* pResName,
             const char* pBitcode, size_t pBitcodeSize, const char* commandLine,
             const char* pRuntimePath, RSLinkRuntimeCallback pLinkRuntimeCallback = NULL,
             bool pDumpIR = false);

  // Returns true if script is successfully compiled.
  bool buildForCompatLib(RSScript &pScript, const char *pOut, const char *pRuntimePath);

  // Tries to load the the compiled bit code at pCacheDir of the given name.  It checks that
  // the file has been compiled from the same bit code and with the same compile arguments as
  // provided.
  static RSExecutable* loadScript(const char* pCacheDir, const char* pResName, const char* pBitcode,
                                  size_t pBitcodeSize, const char* expectedCompileCommandLine,
                                  SymbolResolverProxy& pResolver);
};

} // end namespace bcc

#endif // BCC_RS_COMPILER_DRIVER_H
