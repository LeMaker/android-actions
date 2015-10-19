/*
 * Copyright 2010-2012, The Android Open Source Project
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

#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/Utils.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"

#include "llvm/Option/OptTable.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

#include "rs_cc_options.h"
#include "slang.h"
#include "slang_assert.h"
#include "slang_diagnostic_buffer.h"
#include "slang_rs.h"
#include "slang_rs_reflect_utils.h"

#include <list>
#include <set>
#include <string>

// SaveStringInSet, ExpandArgsFromBuf and ExpandArgv are all copied from
// $(CLANG_ROOT)/tools/driver/driver.cpp for processing argc/argv passed in
// main().
static inline const char *SaveStringInSet(std::set<std::string> &SavedStrings,
                                          llvm::StringRef S) {
  return SavedStrings.insert(S).first->c_str();
}
static void ExpandArgsFromBuf(const char *Arg,
                              llvm::SmallVectorImpl<const char*> &ArgVector,
                              std::set<std::string> &SavedStrings);
static void ExpandArgv(int argc, const char **argv,
                       llvm::SmallVectorImpl<const char*> &ArgVector,
                       std::set<std::string> &SavedStrings);

static const char *DetermineOutputFile(const std::string &OutputDir,
                                       const std::string &PathSuffix,
                                       const char *InputFile,
                                       slang::Slang::OutputType OutputType,
                                       std::set<std::string> &SavedStrings) {
  if (OutputType == slang::Slang::OT_Nothing)
    return "/dev/null";

  std::string OutputFile(OutputDir);

  // Append '/' to Opts.mBitcodeOutputDir if not presents
  if (!OutputFile.empty() &&
      (OutputFile[OutputFile.size() - 1]) != OS_PATH_SEPARATOR)
    OutputFile.append(1, OS_PATH_SEPARATOR);

  if (!PathSuffix.empty()) {
    OutputFile.append(PathSuffix);
    OutputFile.append(1, OS_PATH_SEPARATOR);
  }

  if (OutputType == slang::Slang::OT_Dependency) {
    // The build system wants the .d file name stem to be exactly the same as
    // the source .rs file, instead of the .bc file.
    OutputFile.append(slang::RSSlangReflectUtils::GetFileNameStem(InputFile));
  } else {
    OutputFile.append(
        slang::RSSlangReflectUtils::BCFileNameFromRSFileName(InputFile));
  }

  switch (OutputType) {
    case slang::Slang::OT_Dependency: {
      OutputFile.append(".d");
      break;
    }
    case slang::Slang::OT_Assembly: {
      OutputFile.append(".S");
      break;
    }
    case slang::Slang::OT_LLVMAssembly: {
      OutputFile.append(".ll");
      break;
    }
    case slang::Slang::OT_Object: {
      OutputFile.append(".o");
      break;
    }
    case slang::Slang::OT_Bitcode: {
      OutputFile.append(".bc");
      break;
    }
    case slang::Slang::OT_Nothing:
    default: {
      slangAssert(false && "Invalid output type!");
    }
  }

  return SaveStringInSet(SavedStrings, OutputFile);
}

typedef std::list<std::pair<const char*, const char*> > NamePairList;

/*
 * Compile the Inputs.
 *
 * Returns 0 on success and nonzero on failure.
 *
 * IOFiles - list of (foo.rs, foo.bc) pairs of input/output files.
 * IOFiles32 - list of input/output pairs for 32-bit compilation.
 * Inputs - input filenames.
 * Opts - options controlling compilation.
 * DiagEngine - Clang diagnostic engine (for creating diagnostics).
 * DiagClient - Slang diagnostic consumer (collects and displays diagnostics).
 * SavedStrings - expanded strings copied from argv source input files.
 *
 * We populate IOFiles dynamically while working through the list of Inputs.
 * On any 64-bit compilation, we pass back in the 32-bit pairs of files as
 * IOFiles32. This allows the 64-bit compiler to later bundle up both the
 * 32-bit and 64-bit bitcode outputs to be included in the final reflected
 * source code that is emitted.
 */
static int compileFiles(NamePairList *IOFiles, NamePairList *IOFiles32,
    const llvm::SmallVector<const char*, 16> &Inputs, slang::RSCCOptions &Opts,
    clang::DiagnosticsEngine *DiagEngine, slang::DiagnosticBuffer *DiagClient,
    std::set<std::string> *SavedStrings) {
  NamePairList DepFiles;
  std::string PathSuffix = "";
  bool CompileSecondTimeFor64Bit = false;

  // In our mixed 32/64-bit path, we need to suffix our files differently for
  // both 32-bit and 64-bit versions.
  if (Opts.mEmit3264) {
    if (Opts.mBitWidth == 64) {
      PathSuffix = "bc64";
      CompileSecondTimeFor64Bit = true;
    } else {
      PathSuffix = "bc32";
    }
  }

  for (int i = 0, e = Inputs.size(); i != e; i++) {
    const char *InputFile = Inputs[i];

    const char *BCOutputFile = DetermineOutputFile(Opts.mBitcodeOutputDir,
                                                   PathSuffix, InputFile,
                                                   slang::Slang::OT_Bitcode,
                                                   *SavedStrings);
    const char *OutputFile = BCOutputFile;

    if (Opts.mEmitDependency) {
      // The dependency file is always emitted without a PathSuffix.
      // Collisions between 32-bit and 64-bit files don't make a difference,
      // because they share the same sources/dependencies.
      const char *DepOutputFile =
          DetermineOutputFile(Opts.mDependencyOutputDir, "", InputFile,
                              slang::Slang::OT_Dependency, *SavedStrings);
      if (Opts.mOutputType == slang::Slang::OT_Dependency) {
        OutputFile = DepOutputFile;
      }

      DepFiles.push_back(std::make_pair(BCOutputFile, DepOutputFile));
    }

    IOFiles->push_back(std::make_pair(InputFile, OutputFile));
  }

  std::unique_ptr<slang::SlangRS> Compiler(new slang::SlangRS());
  Compiler->init(Opts.mBitWidth, DiagEngine, DiagClient);
  int CompileFailed = !Compiler->compile(*IOFiles, *IOFiles32, DepFiles, Opts);
  // We suppress warnings (via reset) if we are doing a second compilation.
  Compiler->reset(CompileSecondTimeFor64Bit);
  return CompileFailed;
}

#define str(s) #s
#define wrap_str(s) str(s)
static void llvm_rs_cc_VersionPrinter() {
  llvm::raw_ostream &OS = llvm::outs();
  OS << "llvm-rs-cc: Renderscript compiler\n"
     << "  (http://developer.android.com/guide/topics/renderscript)\n"
     << "  based on LLVM (http://llvm.org):\n";
  OS << "  Built " << __DATE__ << " (" << __TIME__ ").\n";
  OS << "  Target APIs: " << SLANG_MINIMUM_TARGET_API << " - "
     << SLANG_MAXIMUM_TARGET_API;
  OS << "\n  Build type: " << wrap_str(TARGET_BUILD_VARIANT);
#ifndef __DISABLE_ASSERTS
  OS << " with assertions";
#endif
  OS << ".\n";
}
#undef wrap_str
#undef str

int main(int argc, const char **argv) {
  std::set<std::string> SavedStrings;
  llvm::SmallVector<const char*, 256> ArgVector;
  slang::RSCCOptions Opts;
  llvm::SmallVector<const char*, 16> Inputs;
  std::string Argv0;

  llvm::llvm_shutdown_obj Y;  // Call llvm_shutdown() on exit.

  ExpandArgv(argc, argv, ArgVector, SavedStrings);

  // Argv0
  Argv0 = llvm::sys::path::stem(ArgVector[0]);

  // Setup diagnostic engine
  slang::DiagnosticBuffer *DiagClient = new slang::DiagnosticBuffer();

  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagIDs(
    new clang::DiagnosticIDs());

  llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> DiagOpts(
    new clang::DiagnosticOptions());
  clang::DiagnosticsEngine DiagEngine(DiagIDs, &*DiagOpts, DiagClient, true);

  slang::Slang::GlobalInitialization();

  slang::ParseArguments(ArgVector, Inputs, Opts, DiagEngine);

  // Exits when there's any error occurred during parsing the arguments
  if (DiagEngine.hasErrorOccurred()) {
    llvm::errs() << DiagClient->str();
    return 1;
  }

  if (Opts.mShowHelp) {
    std::unique_ptr<llvm::opt::OptTable> OptTbl(slang::createRSCCOptTable());
    OptTbl->PrintHelp(llvm::outs(), Argv0.c_str(),
                      "Renderscript source compiler");
    return 0;
  }

  if (Opts.mShowVersion) {
    llvm_rs_cc_VersionPrinter();
    return 0;
  }

  // No input file
  if (Inputs.empty()) {
    DiagEngine.Report(clang::diag::err_drv_no_input_files);
    llvm::errs() << DiagClient->str();
    return 1;
  }

  // Prepare input data for RS compiler.
  NamePairList IOFiles64;
  NamePairList IOFiles32;

  int CompileFailed = compileFiles(&IOFiles32, &IOFiles32, Inputs, Opts,
                                   &DiagEngine, DiagClient, &SavedStrings);

  // Handle the 64-bit case too!
  if (Opts.mEmit3264 && !CompileFailed) {
    Opts.mBitWidth = 64;
    CompileFailed = compileFiles(&IOFiles64, &IOFiles32, Inputs, Opts,
                                 &DiagEngine, DiagClient, &SavedStrings);
  }

  return CompileFailed;
}

///////////////////////////////////////////////////////////////////////////////

// ExpandArgsFromBuf -
static void ExpandArgsFromBuf(const char *Arg,
                              llvm::SmallVectorImpl<const char*> &ArgVector,
                              std::set<std::string> &SavedStrings) {
  const char *FName = Arg + 1;
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> MBOrErr =
      llvm::MemoryBuffer::getFile(FName);
  if (MBOrErr.getError()) {
    // Unable to open the file
    ArgVector.push_back(SaveStringInSet(SavedStrings, Arg));
    return;
  }
  std::unique_ptr<llvm::MemoryBuffer> MemBuf = std::move(MBOrErr.get());

  const char *Buf = MemBuf->getBufferStart();
  char InQuote = ' ';
  std::string CurArg;

  for (const char *P = Buf; ; ++P) {
    if (*P == '\0' || (isspace(*P) && InQuote == ' ')) {
      if (!CurArg.empty()) {
        if (CurArg[0] != '@') {
          ArgVector.push_back(SaveStringInSet(SavedStrings, CurArg));
        } else {
          ExpandArgsFromBuf(CurArg.c_str(), ArgVector, SavedStrings);
        }

        CurArg = "";
      }
      if (*P == '\0')
        break;
      else
        continue;
    }

    if (isspace(*P)) {
      if (InQuote != ' ')
        CurArg.push_back(*P);
      continue;
    }

    if (*P == '"' || *P == '\'') {
      if (InQuote == *P)
        InQuote = ' ';
      else if (InQuote == ' ')
        InQuote = *P;
      else
        CurArg.push_back(*P);
      continue;
    }

    if (*P == '\\') {
      ++P;
      if (*P != '\0')
        CurArg.push_back(*P);
      continue;
    }
    CurArg.push_back(*P);
  }
}

// ExpandArgsFromBuf -
static void ExpandArgv(int argc, const char **argv,
                       llvm::SmallVectorImpl<const char*> &ArgVector,
                       std::set<std::string> &SavedStrings) {
  for (int i = 0; i < argc; ++i) {
    const char *Arg = argv[i];
    if (Arg[0] != '@') {
      ArgVector.push_back(SaveStringInSet(SavedStrings, std::string(Arg)));
      continue;
    }

    ExpandArgsFromBuf(Arg, ArgVector, SavedStrings);
  }
}
