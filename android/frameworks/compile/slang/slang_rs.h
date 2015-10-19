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

#ifndef _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_H_  // NOLINT
#define _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_H_

#include "slang.h"

#include <list>
#include <string>
#include <utility>
#include <vector>

#include "llvm/ADT/StringMap.h"

#include "slang_rs_reflect_utils.h"
#include "slang_version.h"

namespace slang {
  class RSCCOptions;
  class RSContext;
  class RSExportRecordType;

class SlangRS : public Slang {
 private:
  // Context for Renderscript
  RSContext *mRSContext;

  bool mAllowRSPrefix;

  unsigned int mTargetAPI;

  bool mVerbose;

  bool mIsFilterscript;

  // Custom diagnostic identifiers
  unsigned mDiagErrorInvalidOutputDepParameter;
  unsigned mDiagErrorODR;
  unsigned mDiagErrorTargetAPIRange;

  // Collect generated filenames (without the .java) for dependency generation
  std::vector<std::string> mGeneratedFileNames;

  // FIXME: Should be std::list<RSExportable *> here. But currently we only
  //        check ODR on record type.
  //
  // ReflectedDefinitions maps record type name to a pair:
  //  <its RSExportRecordType instance,
  //   the first file contains this record type definition>
  typedef std::pair<RSExportRecordType*, const char*> ReflectedDefinitionTy;
  typedef llvm::StringMap<ReflectedDefinitionTy> ReflectedDefinitionListTy;
  ReflectedDefinitionListTy ReflectedDefinitions;

  bool generateJavaBitcodeAccessor(const std::string &OutputPathBase,
                                   const std::string &PackageName,
                                   const std::string *LicenseNote);

  // CurInputFile is the pointer to a char array holding the input filename
  // and is valid before compile() ends.
  bool checkODR(const char *CurInputFile);

  // Returns true if this is a Filterscript file.
  static bool isFilterscript(const char *Filename);

 protected:
  virtual void initDiagnostic();
  virtual void initPreprocessor();
  virtual void initASTContext();

  virtual clang::ASTConsumer
  *createBackend(const clang::CodeGenOptions& CodeGenOpts,
                 llvm::raw_ostream *OS,
                 Slang::OutputType OT);


 public:
  static bool IsRSHeaderFile(const char *File);
  // FIXME: Determine whether a location is in RS header (i.e., one of the RS
  //        built-in APIs) should only need its names (we need a "list" of RS
  //        built-in APIs).
  static bool IsLocInRSHeaderFile(const clang::SourceLocation &Loc,
                                  const clang::SourceManager &SourceMgr);

  SlangRS();

  // Compile bunch of RS files given in the llvm-rs-cc arguments. Return true if
  // all given input files are successfully compiled without errors.
  //
  // @IOFiles - List of pairs of <input file path, output file path>.
  //
  // @DepFiles - List of pairs of <output dep. file path, dependent bitcode
  //             target>. If @OutputDep is true, this parameter must be given
  //             with the same number of pairs given in @IOFiles.
  //
  // @Opts - Selection of options defined from invoking llvm-rs-cc
  bool compile(const std::list<std::pair<const char*, const char*> > &IOFiles64,
               const std::list<std::pair<const char*, const char*> > &IOFiles32,
               const std::list<std::pair<const char*, const char*> > &DepFiles,
               const RSCCOptions &Opts);

  virtual void reset(bool SuppressWarnings = false);

  virtual ~SlangRS();

  virtual void makeModuleVisible(clang::Module *Mod,
                                 clang::Module::NameVisibilityKind Visibility,
                                 clang::SourceLocation ImportLoc,
                                 bool Complain = false) { }

  virtual clang::GlobalModuleIndex *loadGlobalModuleIndex(
      clang::SourceLocation TriggerLoc) { }

  virtual bool lookupMissingImports(llvm::StringRef Name,
                                    clang::SourceLocation TriggerLoc) { }
};
}  // namespace slang

#endif  // _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_H_  NOLINT
