/*
 * Copyright 2014, The Android Open Source Project
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
#include "clang/Frontend/Utils.h"

#include "llvm/Option/Arg.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Option/Option.h"
#include "llvm/Option/OptTable.h"

#include "rs_cc_options.h"
#include "slang.h"
#include "slang_assert.h"

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

enum {
  OPT_INVALID = 0,  // This is not an option ID.
#define PREFIX(NAME, VALUE)
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM, \
               HELPTEXT, METAVAR)                                             \
  OPT_##ID,
#include "RSCCOptions.inc"
  LastOption
#undef OPTION
#undef PREFIX
};

#define PREFIX(NAME, VALUE) const char *const NAME[] = VALUE;
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM, \
               HELPTEXT, METAVAR)
#include "RSCCOptions.inc"
#undef OPTION
#undef PREFIX

static const llvm::opt::OptTable::Info RSCCInfoTable[] = {
#define PREFIX(NAME, VALUE)
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM,  \
               HELPTEXT, METAVAR)                                              \
  {                                                                            \
    PREFIX, NAME, HELPTEXT, METAVAR, OPT_##ID, llvm::opt::Option::KIND##Class, \
        PARAM, FLAGS, OPT_##GROUP, OPT_##ALIAS, ALIASARGS                      \
  }                                                                            \
  ,
#include "RSCCOptions.inc"
#undef OPTION
#undef PREFIX
};

namespace {

class RSCCOptTable : public llvm::opt::OptTable {
 public:
  RSCCOptTable()
      : OptTable(RSCCInfoTable,
                 sizeof(RSCCInfoTable) / sizeof(RSCCInfoTable[0])) {}
};
}

llvm::opt::OptTable *slang::createRSCCOptTable() { return new RSCCOptTable(); }

void slang::ParseArguments(llvm::SmallVectorImpl<const char *> &ArgVector,
                           llvm::SmallVectorImpl<const char *> &Inputs,
                           slang::RSCCOptions &Opts,
                           clang::DiagnosticsEngine &DiagEngine) {
  if (ArgVector.size() > 1) {
    const char **ArgBegin = ArgVector.data() + 1;
    const char **ArgEnd = ArgVector.data() + ArgVector.size();
    unsigned MissingArgIndex, MissingArgCount;
    std::unique_ptr<llvm::opt::OptTable> OptParser(slang::createRSCCOptTable());
    std::unique_ptr<llvm::opt::InputArgList> Args(OptParser->ParseArgs(
        ArgBegin, ArgEnd, MissingArgIndex, MissingArgCount));

    // Check for missing argument error.
    if (MissingArgCount)
      DiagEngine.Report(clang::diag::err_drv_missing_argument)
          << Args->getArgString(MissingArgIndex) << MissingArgCount;

    clang::DiagnosticOptions DiagOpts;
    DiagOpts.IgnoreWarnings = Args->hasArg(OPT_w);
    DiagOpts.Warnings = Args->getAllArgValues(OPT_W);
    clang::ProcessWarningOptions(DiagEngine, DiagOpts);

    // Issue errors on unknown arguments.
    for (llvm::opt::arg_iterator it = Args->filtered_begin(OPT_UNKNOWN),
                                 ie = Args->filtered_end();
         it != ie; ++it)
      DiagEngine.Report(clang::diag::err_drv_unknown_argument)
          << (*it)->getAsString(*Args);

    for (llvm::opt::ArgList::const_iterator it = Args->begin(),
                                            ie = Args->end();
         it != ie; ++it) {
      const llvm::opt::Arg *A = *it;
      if (A->getOption().getKind() == llvm::opt::Option::InputClass)
        Inputs.push_back(A->getValue());
    }

    Opts.mIncludePaths = Args->getAllArgValues(OPT_I);

    Opts.mBitcodeOutputDir = Args->getLastArgValue(OPT_o);

    if (const llvm::opt::Arg *A = Args->getLastArg(OPT_M_Group)) {
      switch (A->getOption().getID()) {
        case OPT_M: {
          Opts.mEmitDependency = true;
          Opts.mOutputType = slang::Slang::OT_Dependency;
          break;
        }
        case OPT_MD: {
          Opts.mEmitDependency = true;
          Opts.mOutputType = slang::Slang::OT_Bitcode;
          break;
        }
        default: { slangAssert(false && "Invalid option in M group!"); }
      }
    }

    if (const llvm::opt::Arg *A = Args->getLastArg(OPT_Output_Type_Group)) {
      switch (A->getOption().getID()) {
        case OPT_emit_asm: {
          Opts.mOutputType = slang::Slang::OT_Assembly;
          break;
        }
        case OPT_emit_llvm: {
          Opts.mOutputType = slang::Slang::OT_LLVMAssembly;
          break;
        }
        case OPT_emit_bc: {
          Opts.mOutputType = slang::Slang::OT_Bitcode;
          break;
        }
        case OPT_emit_nothing: {
          Opts.mOutputType = slang::Slang::OT_Nothing;
          break;
        }
        default: {
          slangAssert(false && "Invalid option in output type group!");
        }
      }
    }

    if (Opts.mEmitDependency &&
        ((Opts.mOutputType != slang::Slang::OT_Bitcode) &&
         (Opts.mOutputType != slang::Slang::OT_Dependency)))
      DiagEngine.Report(clang::diag::err_drv_argument_not_allowed_with)
          << Args->getLastArg(OPT_M_Group)->getAsString(*Args)
          << Args->getLastArg(OPT_Output_Type_Group)->getAsString(*Args);

    Opts.mAllowRSPrefix = Args->hasArg(OPT_allow_rs_prefix);

    Opts.mJavaReflectionPathBase =
        Args->getLastArgValue(OPT_java_reflection_path_base);
    Opts.mJavaReflectionPackageName =
        Args->getLastArgValue(OPT_java_reflection_package_name);

    Opts.mRSPackageName = Args->getLastArgValue(OPT_rs_package_name);

    llvm::StringRef BitcodeStorageValue =
        Args->getLastArgValue(OPT_bitcode_storage);
    if (BitcodeStorageValue == "ar")
      Opts.mBitcodeStorage = slang::BCST_APK_RESOURCE;
    else if (BitcodeStorageValue == "jc")
      Opts.mBitcodeStorage = slang::BCST_JAVA_CODE;
    else if (!BitcodeStorageValue.empty())
      DiagEngine.Report(clang::diag::err_drv_invalid_value)
          << OptParser->getOptionName(OPT_bitcode_storage)
          << BitcodeStorageValue;

    llvm::opt::Arg *lastBitwidthArg = Args->getLastArg(OPT_m32, OPT_m64);
    if (Args->hasArg(OPT_reflect_cpp)) {
      Opts.mBitcodeStorage = slang::BCST_CPP_CODE;
      // mJavaReflectionPathBase can be set for C++ reflected builds.
      // Set it to the standard mBitcodeOutputDir (via -o) by default.
      if (Opts.mJavaReflectionPathBase.empty()) {
        Opts.mJavaReflectionPathBase = Opts.mBitcodeOutputDir;
      }

      // Check for bitwidth arguments.
      if (lastBitwidthArg) {
        if (lastBitwidthArg->getOption().matches(OPT_m32)) {
          Opts.mBitWidth = 32;
        } else {
          Opts.mBitWidth = 64;
        }
      }
    } else if (lastBitwidthArg) {
      // -m32/-m64 are forbidden for non-C++ reflection paths.
      DiagEngine.Report(DiagEngine.getCustomDiagID(
          clang::DiagnosticsEngine::Error,
          "cannot use -m32/-m64 without specifying C++ reflection (-reflect-c++)"));
    }

    Opts.mDependencyOutputDir =
        Args->getLastArgValue(OPT_output_dep_dir, Opts.mBitcodeOutputDir);
    Opts.mAdditionalDepTargets =
        Args->getAllArgValues(OPT_additional_dep_target);

    Opts.mShowHelp = Args->hasArg(OPT_help);
    Opts.mShowVersion = Args->hasArg(OPT_version);
    Opts.mDebugEmission = Args->hasArg(OPT_emit_g);
    Opts.mVerbose = Args->hasArg(OPT_verbose);

    // If we are emitting both 32-bit and 64-bit bitcode, we must embed it.

    size_t OptLevel =
        clang::getLastArgIntValue(*Args, OPT_optimization_level, 3, DiagEngine);

    Opts.mOptimizationLevel =
        OptLevel == 0 ? llvm::CodeGenOpt::None : llvm::CodeGenOpt::Aggressive;

    Opts.mTargetAPI = clang::getLastArgIntValue(*Args, OPT_target_api,
                                                RS_VERSION, DiagEngine);

    if (Opts.mTargetAPI == 0) {
      Opts.mTargetAPI = UINT_MAX;
    }

    Opts.mEmit3264 = (Opts.mTargetAPI >= 21) && (Opts.mBitcodeStorage != slang::BCST_CPP_CODE);
    if (Opts.mEmit3264) {
        Opts.mBitcodeStorage = slang::BCST_JAVA_CODE;
    }
  }
}
