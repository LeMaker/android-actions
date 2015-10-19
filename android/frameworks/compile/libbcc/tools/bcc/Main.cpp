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

#include <string>
#include <vector>

#include <dlfcn.h>
#include <stdlib.h>

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Config/config.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/PluginLoader.h>
#include <llvm/Support/raw_ostream.h>

#include <bcc/BCCContext.h>
#include <bcc/Compiler.h>
#include <bcc/Config/Config.h>
#include <bcc/ExecutionEngine/CompilerRTSymbolResolver.h>
#include <bcc/ExecutionEngine/ObjectLoader.h>
#include <bcc/ExecutionEngine/SymbolResolverProxy.h>
#include <bcc/ExecutionEngine/SymbolResolvers.h>
#include <bcc/Renderscript/RSCompilerDriver.h>
#include <bcc/Script.h>
#include <bcc/Source.h>
#include <bcc/Support/CompilerConfig.h>
#include <bcc/Support/Initialization.h>
#include <bcc/Support/InputFile.h>
#include <bcc/Support/OutputFile.h>

using namespace bcc;

#define STR2(a) #a
#define STR(a) STR2(a)

//===----------------------------------------------------------------------===//
// General Options
//===----------------------------------------------------------------------===//
namespace {

llvm::cl::opt<std::string>
OptInputFilename(llvm::cl::Positional, llvm::cl::ValueRequired,
                 llvm::cl::desc("<input bitcode file>"));

llvm::cl::opt<std::string>
OptOutputFilename("o", llvm::cl::desc("Specify the output filename"),
                  llvm::cl::value_desc("filename"),
                  llvm::cl::init("bcc_output"));

llvm::cl::opt<std::string>
OptBCLibFilename("bclib", llvm::cl::desc("Specify the bclib filename"),
                 llvm::cl::value_desc("bclib"));

llvm::cl::opt<std::string>
OptOutputPath("output_path", llvm::cl::desc("Specify the output path"),
              llvm::cl::value_desc("output path"),
              llvm::cl::init("."));

llvm::cl::opt<bool>
OptEmitLLVM("emit-llvm",
            llvm::cl::desc("Emit an LLVM-IR version of the generated program"));

llvm::cl::opt<std::string>
OptTargetTriple("mtriple",
                llvm::cl::desc("Specify the target triple (default: "
                               DEFAULT_TARGET_TRIPLE_STRING ")"),
                llvm::cl::init(DEFAULT_TARGET_TRIPLE_STRING),
                llvm::cl::value_desc("triple"));

llvm::cl::alias OptTargetTripleC("C", llvm::cl::NotHidden,
                                 llvm::cl::desc("Alias for -mtriple"),
                                 llvm::cl::aliasopt(OptTargetTriple));

llvm::cl::opt<bool>
OptRSDebugContext("rs-debug-ctx",
    llvm::cl::desc("Enable build to work with a RenderScript debug context"));

//===----------------------------------------------------------------------===//
// Compiler Options
//===----------------------------------------------------------------------===//

// RenderScript uses -O3 by default
llvm::cl::opt<char>
OptOptLevel("O", llvm::cl::desc("Optimization level. [-O0, -O1, -O2, or -O3] "
                                "(default: -O3)"),
            llvm::cl::Prefix, llvm::cl::ZeroOrMore, llvm::cl::init('3'));

// Override "bcc -version" since the LLVM version information is not correct on
// Android build.
void BCCVersionPrinter() {
  llvm::raw_ostream &os = llvm::outs();
  os << "libbcc (The Android Open Source Project, http://www.android.com/):\n"
     << "  Default target: " << DEFAULT_TARGET_TRIPLE_STRING << "\n\n"
     << "LLVM (http://llvm.org/):\n"
     << "  Version: " << PACKAGE_VERSION << "\n";
  return;
}

} // end anonymous namespace

static inline
bool ConfigCompiler(RSCompilerDriver &pRSCD) {
  RSCompiler *RSC = pRSCD.getCompiler();
  CompilerConfig *config = NULL;

  config = new (std::nothrow) CompilerConfig(OptTargetTriple);
  if (config == NULL) {
    llvm::errs() << "Out of memory when create the compiler configuration!\n";
    return false;
  }

  // llvm3.5 has removed the auto-detect feature for x86 subtarget,
  // so set features explicitly in bcc.
  if ((config->getTriple().find("i686") != std::string::npos) ||
    (config->getTriple().find("x86_64") != std::string::npos)) {
    std::vector<std::string> fv;

#if defined(__SSE3__)
    fv.push_back("+sse3");
#endif
#if defined(__SSSE3__)
    fv.push_back("+ssse3");
#endif
#if defined(__SSE4_1__)
    fv.push_back("+sse4.1");
#endif
#if defined(__SSE4_2__)
    fv.push_back("+sse4.2");
#endif

    if (fv.size()) {
      config->setFeatureString(fv);
    }
  }

  switch (OptOptLevel) {
    case '0': config->setOptimizationLevel(llvm::CodeGenOpt::None); break;
    case '1': config->setOptimizationLevel(llvm::CodeGenOpt::Less); break;
    case '2': config->setOptimizationLevel(llvm::CodeGenOpt::Default); break;
    case '3':
    default: {
      config->setOptimizationLevel(llvm::CodeGenOpt::Aggressive);
      break;
    }
  }

  pRSCD.setConfig(config);
  Compiler::ErrorCode result = RSC->config(*config);

  if (OptRSDebugContext) {
    pRSCD.setDebugContext(true);
  }

  if (result != Compiler::kSuccess) {
    llvm::errs() << "Failed to configure the compiler! (detail: "
                 << Compiler::GetErrorString(result) << ")\n";
    return false;
  }

  return true;
}

int main(int argc, char **argv) {
  llvm::cl::SetVersionPrinter(BCCVersionPrinter);
  llvm::cl::ParseCommandLineOptions(argc, argv);
  std::string commandLine = bcc::getCommandLine(argc, argv);
  init::Initialize();

  BCCContext context;
  RSCompilerDriver RSCD;

  if (OptBCLibFilename.empty()) {
    ALOGE("Failed to compile bit code, -bclib was not specified");
    return EXIT_FAILURE;
  }

  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> mb_or_error =
      llvm::MemoryBuffer::getFile(OptInputFilename.c_str());
  if (mb_or_error.getError()) {
    ALOGE("Failed to load bitcode from path %s! (%s)",
          OptInputFilename.c_str(), mb_or_error.getError().message().c_str());
    return EXIT_FAILURE;
  }
  std::unique_ptr<llvm::MemoryBuffer> input_data = std::move(mb_or_error.get());

  const char *bitcode = input_data->getBufferStart();
  size_t bitcodeSize = input_data->getBufferSize();

  if (!ConfigCompiler(RSCD)) {
    ALOGE("Failed to configure compiler");
    return EXIT_FAILURE;
  }

  // Attempt to dynamically initialize the compiler driver if such a function
  // is present. It is only present if passed via "-load libFOO.so".
  RSCompilerDriverInit_t rscdi = (RSCompilerDriverInit_t)
      dlsym(RTLD_DEFAULT, STR(RS_COMPILER_DRIVER_INIT_FN));
  if (rscdi != NULL) {
    rscdi(&RSCD);
  }

  bool built = RSCD.build(context, OptOutputPath.c_str(), OptOutputFilename.c_str(), bitcode,
                          bitcodeSize, commandLine.c_str(), OptBCLibFilename.c_str(), NULL,
                          OptEmitLLVM);

  if (!built) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
