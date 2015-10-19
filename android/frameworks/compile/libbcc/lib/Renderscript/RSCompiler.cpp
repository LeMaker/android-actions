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

#include "bcc/Renderscript/RSCompiler.h"

#include <llvm/IR/Module.h>
#include <llvm/PassManager.h>
#include <llvm/Transforms/IPO.h>

#include "bcc/Assert.h"
#include "bcc/Renderscript/RSExecutable.h"
#include "bcc/Renderscript/RSScript.h"
#include "bcc/Renderscript/RSTransforms.h"
#include "bcc/Source.h"
#include "bcc/Support/Log.h"
#include "bcinfo/MetadataExtractor.h"

using namespace bcc;

bool RSCompiler::addInternalizeSymbolsPass(Script &pScript, llvm::PassManager &pPM) {
  // Add a pass to internalize the symbols that don't need to have global
  // visibility.
  RSScript &script = static_cast<RSScript &>(pScript);
  llvm::Module &module = script.getSource().getModule();
  bcinfo::MetadataExtractor me(&module);
  if (!me.extract()) {
    bccAssert(false && "Could not extract metadata for module!");
    return false;
  }

  // The vector contains the symbols that should not be internalized.
  std::vector<const char *> export_symbols;

  // Special RS functions should always be global symbols.
  const char **special_functions = RSExecutable::SpecialFunctionNames;
  while (*special_functions != NULL) {
    export_symbols.push_back(*special_functions);
    special_functions++;
  }

  // Visibility of symbols appeared in rs_export_var and rs_export_func should
  // also be preserved.
  size_t exportVarCount = me.getExportVarCount();
  size_t exportFuncCount = me.getExportFuncCount();
  size_t exportForEachCount = me.getExportForEachSignatureCount();
  const char **exportVarNameList = me.getExportVarNameList();
  const char **exportFuncNameList = me.getExportFuncNameList();
  const char **exportForEachNameList = me.getExportForEachNameList();
  size_t i;

  for (i = 0; i < exportVarCount; ++i) {
    export_symbols.push_back(exportVarNameList[i]);
  }

  for (i = 0; i < exportFuncCount; ++i) {
    export_symbols.push_back(exportFuncNameList[i]);
  }

  // Expanded foreach functions should not be internalized, too.
  // expanded_foreach_funcs keeps the .expand version of the kernel names
  // around until createInternalizePass() is finished making its own
  // copy of the visible symbols.
  std::vector<std::string> expanded_foreach_funcs;
  for (i = 0; i < exportForEachCount; ++i) {
    expanded_foreach_funcs.push_back(
        std::string(exportForEachNameList[i]) + ".expand");
  }

  for (i = 0; i < exportForEachCount; i++) {
      export_symbols.push_back(expanded_foreach_funcs[i].c_str());
  }

  pPM.add(llvm::createInternalizePass(export_symbols));

  return true;
}

bool RSCompiler::addExpandForEachPass(Script &pScript, llvm::PassManager &pPM) {
  // Script passed to RSCompiler must be a RSScript.
  RSScript &script = static_cast<RSScript &>(pScript);

  // Expand ForEach on CPU path to reduce launch overhead.
  bool pEnableStepOpt = true;
  pPM.add(createRSForEachExpandPass(pEnableStepOpt));
  if (script.getEmbedInfo())
    pPM.add(createRSEmbedInfoPass());

  return true;
}

bool RSCompiler::beforeAddLTOPasses(Script &pScript, llvm::PassManager &pPM) {
  if (!addExpandForEachPass(pScript, pPM))
    return false;

  if (!addInternalizeSymbolsPass(pScript, pPM))
    return false;

  return true;
}
