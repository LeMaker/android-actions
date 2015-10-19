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

#include "slang_rs_export_foreach.h"

#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/TypeLoc.h"

#include "llvm/IR/DerivedTypes.h"

#include "slang_assert.h"
#include "slang_rs_context.h"
#include "slang_rs_export_type.h"
#include "slang_version.h"

namespace slang {

// This function takes care of additional validation and construction of
// parameters related to forEach_* reflection.
bool RSExportForEach::validateAndConstructParams(
    RSContext *Context, const clang::FunctionDecl *FD) {
  slangAssert(Context && FD);
  bool valid = true;

  numParams = FD->getNumParams();

  if (Context->getTargetAPI() < SLANG_JB_TARGET_API) {
    // Before JellyBean, we allowed only one kernel per file.  It must be called "root".
    if (!isRootRSFunc(FD)) {
      Context->ReportError(FD->getLocation(),
                           "Non-root compute kernel %0() is "
                           "not supported in SDK levels %1-%2")
          << FD->getName() << SLANG_MINIMUM_TARGET_API
          << (SLANG_JB_TARGET_API - 1);
      return false;
    }
  }

  mResultType = FD->getReturnType().getCanonicalType();
  // Compute kernel functions are defined differently when the
  // "__attribute__((kernel))" is set.
  if (FD->hasAttr<clang::KernelAttr>()) {
    valid |= validateAndConstructKernelParams(Context, FD);
  } else {
    valid |= validateAndConstructOldStyleParams(Context, FD);
  }

  valid |= setSignatureMetadata(Context, FD);
  return valid;
}

bool RSExportForEach::validateAndConstructOldStyleParams(
    RSContext *Context, const clang::FunctionDecl *FD) {
  slangAssert(Context && FD);
  // If numParams is 0, we already marked this as a graphics root().
  slangAssert(numParams > 0);

  bool valid = true;

  // Compute kernel functions of this style are required to return a void type.
  clang::ASTContext &C = Context->getASTContext();
  if (mResultType != C.VoidTy) {
    Context->ReportError(FD->getLocation(),
                         "Compute kernel %0() is required to return a "
                         "void type")
        << FD->getName();
    valid = false;
  }

  // Validate remaining parameter types
  // TODO(all): Add support for LOD/face when we have them

  size_t IndexOfFirstIterator = numParams;
  valid |= validateIterationParameters(Context, FD, &IndexOfFirstIterator);

  // Validate the non-iterator parameters, which should all be found before the
  // first iterator.
  for (size_t i = 0; i < IndexOfFirstIterator; i++) {
    const clang::ParmVarDecl *PVD = FD->getParamDecl(i);
    clang::QualType QT = PVD->getType().getCanonicalType();

    if (!QT->isPointerType()) {
      Context->ReportError(PVD->getLocation(),
                           "Compute kernel %0() cannot have non-pointer "
                           "parameters besides 'x' and 'y'. Parameter '%1' is "
                           "of type: '%2'")
          << FD->getName() << PVD->getName() << PVD->getType().getAsString();
      valid = false;
      continue;
    }

    // The only non-const pointer should be out.
    if (!QT->getPointeeType().isConstQualified()) {
      if (mOut == NULL) {
        mOut = PVD;
      } else {
        Context->ReportError(PVD->getLocation(),
                             "Compute kernel %0() can only have one non-const "
                             "pointer parameter. Parameters '%1' and '%2' are "
                             "both non-const.")
            << FD->getName() << mOut->getName() << PVD->getName();
        valid = false;
      }
    } else {
      if (mIns.empty() && mOut == NULL) {
        mIns.push_back(PVD);
      } else if (mUsrData == NULL) {
        mUsrData = PVD;
      } else {
        Context->ReportError(
            PVD->getLocation(),
            "Unexpected parameter '%0' for compute kernel %1()")
            << PVD->getName() << FD->getName();
        valid = false;
      }
    }
  }

  if (mIns.empty() && !mOut) {
    Context->ReportError(FD->getLocation(),
                         "Compute kernel %0() must have at least one "
                         "parameter for in or out")
        << FD->getName();
    valid = false;
  }

  return valid;
}

bool RSExportForEach::validateAndConstructKernelParams(
    RSContext *Context, const clang::FunctionDecl *FD) {
  slangAssert(Context && FD);
  bool valid = true;
  clang::ASTContext &C = Context->getASTContext();

  if (Context->getTargetAPI() < SLANG_JB_MR1_TARGET_API) {
    Context->ReportError(FD->getLocation(),
                         "Compute kernel %0() targeting SDK levels "
                         "%1-%2 may not use pass-by-value with "
                         "__attribute__((kernel))")
        << FD->getName() << SLANG_MINIMUM_TARGET_API
        << (SLANG_JB_MR1_TARGET_API - 1);
    return false;
  }

  // Denote that we are indeed a pass-by-value kernel.
  mIsKernelStyle = true;
  mHasReturnType = (mResultType != C.VoidTy);

  if (mResultType->isPointerType()) {
    Context->ReportError(
        FD->getTypeSpecStartLoc(),
        "Compute kernel %0() cannot return a pointer type: '%1'")
        << FD->getName() << mResultType.getAsString();
    valid = false;
  }

  // Validate remaining parameter types
  // TODO(all): Add support for LOD/face when we have them

  size_t IndexOfFirstIterator = numParams;
  valid |= validateIterationParameters(Context, FD, &IndexOfFirstIterator);

  // Validate the non-iterator parameters, which should all be found before the
  // first iterator.
  for (size_t i = 0; i < IndexOfFirstIterator; i++) {
    const clang::ParmVarDecl *PVD = FD->getParamDecl(i);

    /*
     * FIXME: Change this to a test against an actual API version when the
     *        multi-input feature is officially supported.
     */
    if (Context->getTargetAPI() == SLANG_DEVELOPMENT_TARGET_API || i == 0) {
      mIns.push_back(PVD);
    } else {
      Context->ReportError(PVD->getLocation(),
                           "Invalid parameter '%0' for compute kernel %1(). "
                           "Kernels targeting SDK levels %2-%3 may not use "
                           "multiple input parameters.") << PVD->getName() <<
                           FD->getName() << SLANG_MINIMUM_TARGET_API <<
                           SLANG_MAXIMUM_TARGET_API;
      valid = false;
    }
    clang::QualType QT = PVD->getType().getCanonicalType();
    if (QT->isPointerType()) {
      Context->ReportError(PVD->getLocation(),
                           "Compute kernel %0() cannot have "
                           "parameter '%1' of pointer type: '%2'")
          << FD->getName() << PVD->getName() << PVD->getType().getAsString();
      valid = false;
    }
  }

  // Check that we have at least one allocation to use for dimensions.
  if (valid && mIns.empty() && !mHasReturnType) {
    Context->ReportError(FD->getLocation(),
                         "Compute kernel %0() must have at least one "
                         "input parameter or a non-void return "
                         "type")
        << FD->getName();
    valid = false;
  }

  return valid;
}

// Search for the optional x and y parameters.  Returns true if valid.   Also
// sets *IndexOfFirstIterator to the index of the first iterator parameter, or
// FD->getNumParams() if none are found.
bool RSExportForEach::validateIterationParameters(
    RSContext *Context, const clang::FunctionDecl *FD,
    size_t *IndexOfFirstIterator) {
  slangAssert(IndexOfFirstIterator != NULL);
  slangAssert(mX == NULL && mY == NULL);
  clang::ASTContext &C = Context->getASTContext();

  // Find the x and y parameters if present.
  size_t NumParams = FD->getNumParams();
  *IndexOfFirstIterator = NumParams;
  bool valid = true;
  for (size_t i = 0; i < NumParams; i++) {
    const clang::ParmVarDecl *PVD = FD->getParamDecl(i);
    llvm::StringRef ParamName = PVD->getName();
    if (ParamName.equals("x")) {
      slangAssert(mX == NULL);  // We won't be invoked if two 'x' are present.
      mX = PVD;
      if (mY != NULL) {
        Context->ReportError(PVD->getLocation(),
                             "In compute kernel %0(), parameter 'x' should "
                             "be defined before parameter 'y'")
            << FD->getName();
        valid = false;
      }
    } else if (ParamName.equals("y")) {
      slangAssert(mY == NULL);  // We won't be invoked if two 'y' are present.
      mY = PVD;
    } else {
      // It's neither x nor y.
      if (*IndexOfFirstIterator < NumParams) {
        Context->ReportError(PVD->getLocation(),
                             "In compute kernel %0(), parameter '%1' cannot "
                             "appear after the 'x' and 'y' parameters")
            << FD->getName() << ParamName;
        valid = false;
      }
      continue;
    }
    // Validate the data type of x and y.
    clang::QualType QT = PVD->getType().getCanonicalType();
    clang::QualType UT = QT.getUnqualifiedType();
    if (UT != C.UnsignedIntTy && UT != C.IntTy) {
      Context->ReportError(PVD->getLocation(),
                           "Parameter '%0' must be of type 'int' or "
                           "'unsigned int'. It is of type '%1'")
          << ParamName << PVD->getType().getAsString();
      valid = false;
    }
    // If this is the first time we find an iterator, save it.
    if (*IndexOfFirstIterator >= NumParams) {
      *IndexOfFirstIterator = i;
    }
  }
  // Check that x and y have the same type.
  if (mX != NULL and mY != NULL) {
    clang::QualType XType = mX->getType();
    clang::QualType YType = mY->getType();

    if (XType != YType) {
      Context->ReportError(mY->getLocation(),
                           "Parameter 'x' and 'y' must be of the same type. "
                           "'x' is of type '%0' while 'y' is of type '%1'")
          << XType.getAsString() << YType.getAsString();
      valid = false;
    }
  }
  return valid;
}

bool RSExportForEach::setSignatureMetadata(RSContext *Context,
                                           const clang::FunctionDecl *FD) {
  mSignatureMetadata = 0;
  bool valid = true;

  if (mIsKernelStyle) {
    slangAssert(mOut == NULL);
    slangAssert(mUsrData == NULL);
  } else {
    slangAssert(!mHasReturnType);
  }

  // Set up the bitwise metadata encoding for runtime argument passing.
  // TODO: If this bit field is re-used from C++ code, define the values in a header.
  const bool HasOut = mOut || mHasReturnType;
  mSignatureMetadata |= (hasIns() ?       0x01 : 0);
  mSignatureMetadata |= (HasOut ?         0x02 : 0);
  mSignatureMetadata |= (mUsrData ?       0x04 : 0);
  mSignatureMetadata |= (mX ?             0x08 : 0);
  mSignatureMetadata |= (mY ?             0x10 : 0);
  mSignatureMetadata |= (mIsKernelStyle ? 0x20 : 0);  // pass-by-value

  if (Context->getTargetAPI() < SLANG_ICS_TARGET_API) {
    // APIs before ICS cannot skip between parameters. It is ok, however, for
    // them to omit further parameters (i.e. skipping X is ok if you skip Y).
    if (mSignatureMetadata != 0x1f &&  // In, Out, UsrData, X, Y
        mSignatureMetadata != 0x0f &&  // In, Out, UsrData, X
        mSignatureMetadata != 0x07 &&  // In, Out, UsrData
        mSignatureMetadata != 0x03 &&  // In, Out
        mSignatureMetadata != 0x01) {  // In
      Context->ReportError(FD->getLocation(),
                           "Compute kernel %0() targeting SDK levels "
                           "%1-%2 may not skip parameters")
          << FD->getName() << SLANG_MINIMUM_TARGET_API
          << (SLANG_ICS_TARGET_API - 1);
      valid = false;
    }
  }
  return valid;
}

RSExportForEach *RSExportForEach::Create(RSContext *Context,
                                         const clang::FunctionDecl *FD) {
  slangAssert(Context && FD);
  llvm::StringRef Name = FD->getName();
  RSExportForEach *FE;

  slangAssert(!Name.empty() && "Function must have a name");

  FE = new RSExportForEach(Context, Name);

  if (!FE->validateAndConstructParams(Context, FD)) {
    return NULL;
  }

  clang::ASTContext &Ctx = Context->getASTContext();

  std::string Id = CreateDummyName("helper_foreach_param", FE->getName());

  // Extract the usrData parameter (if we have one)
  if (FE->mUsrData) {
    const clang::ParmVarDecl *PVD = FE->mUsrData;
    clang::QualType QT = PVD->getType().getCanonicalType();
    slangAssert(QT->isPointerType() &&
                QT->getPointeeType().isConstQualified());

    const clang::ASTContext &C = Context->getASTContext();
    if (QT->getPointeeType().getCanonicalType().getUnqualifiedType() ==
        C.VoidTy) {
      // In the case of using const void*, we can't reflect an appopriate
      // Java type, so we fall back to just reflecting the ain/aout parameters
      FE->mUsrData = NULL;
    } else {
      clang::RecordDecl *RD =
          clang::RecordDecl::Create(Ctx, clang::TTK_Struct,
                                    Ctx.getTranslationUnitDecl(),
                                    clang::SourceLocation(),
                                    clang::SourceLocation(),
                                    &Ctx.Idents.get(Id));

      clang::FieldDecl *FD =
          clang::FieldDecl::Create(Ctx,
                                   RD,
                                   clang::SourceLocation(),
                                   clang::SourceLocation(),
                                   PVD->getIdentifier(),
                                   QT->getPointeeType(),
                                   NULL,
                                   /* BitWidth = */ NULL,
                                   /* Mutable = */ false,
                                   /* HasInit = */ clang::ICIS_NoInit);
      RD->addDecl(FD);
      RD->completeDefinition();

      // Create an export type iff we have a valid usrData type
      clang::QualType T = Ctx.getTagDeclType(RD);
      slangAssert(!T.isNull());

      RSExportType *ET = RSExportType::Create(Context, T.getTypePtr());

      if (ET == NULL) {
        fprintf(stderr, "Failed to export the function %s. There's at least "
                        "one parameter whose type is not supported by the "
                        "reflection\n", FE->getName().c_str());
        return NULL;
      }

      slangAssert((ET->getClass() == RSExportType::ExportClassRecord) &&
                  "Parameter packet must be a record");

      FE->mParamPacketType = static_cast<RSExportRecordType *>(ET);
    }
  }

  if (FE->hasIns()) {

    for (InIter BI = FE->mIns.begin(), EI = FE->mIns.end(); BI != EI; BI++) {
      const clang::Type *T = (*BI)->getType().getCanonicalType().getTypePtr();
      RSExportType *InExportType = RSExportType::Create(Context, T);

      if (FE->mIsKernelStyle) {
        slangAssert(InExportType != NULL);
      }

      FE->mInTypes.push_back(InExportType);
    }
  }

  if (FE->mIsKernelStyle && FE->mHasReturnType) {
    const clang::Type *T = FE->mResultType.getTypePtr();
    FE->mOutType = RSExportType::Create(Context, T);
    slangAssert(FE->mOutType);
  } else if (FE->mOut) {
    const clang::Type *T = FE->mOut->getType().getCanonicalType().getTypePtr();
    FE->mOutType = RSExportType::Create(Context, T);
  }

  return FE;
}

RSExportForEach *RSExportForEach::CreateDummyRoot(RSContext *Context) {
  slangAssert(Context);
  llvm::StringRef Name = "root";
  RSExportForEach *FE = new RSExportForEach(Context, Name);
  FE->mDummyRoot = true;
  return FE;
}

bool RSExportForEach::isGraphicsRootRSFunc(unsigned int targetAPI,
                                           const clang::FunctionDecl *FD) {
  if (FD->hasAttr<clang::KernelAttr>()) {
    return false;
  }

  if (!isRootRSFunc(FD)) {
    return false;
  }

  if (FD->getNumParams() == 0) {
    // Graphics root function
    return true;
  }

  // Check for legacy graphics root function (with single parameter).
  if ((targetAPI < SLANG_ICS_TARGET_API) && (FD->getNumParams() == 1)) {
    const clang::QualType &IntType = FD->getASTContext().IntTy;
    if (FD->getReturnType().getCanonicalType() == IntType) {
      return true;
    }
  }

  return false;
}

bool RSExportForEach::isRSForEachFunc(unsigned int targetAPI,
                                      slang::RSContext* Context,
                                      const clang::FunctionDecl *FD) {
  slangAssert(Context && FD);
  bool hasKernelAttr = FD->hasAttr<clang::KernelAttr>();

  if (FD->getStorageClass() == clang::SC_Static) {
    if (hasKernelAttr) {
      Context->ReportError(FD->getLocation(),
                           "Invalid use of attribute kernel with "
                           "static function declaration: %0")
          << FD->getName();
    }
    return false;
  }

  // Anything tagged as a kernel is definitely used with ForEach.
  if (hasKernelAttr) {
    return true;
  }

  if (isGraphicsRootRSFunc(targetAPI, FD)) {
    return false;
  }

  // Check if first parameter is a pointer (which is required for ForEach).
  unsigned int numParams = FD->getNumParams();

  if (numParams > 0) {
    const clang::ParmVarDecl *PVD = FD->getParamDecl(0);
    clang::QualType QT = PVD->getType().getCanonicalType();

    if (QT->isPointerType()) {
      return true;
    }

    // Any non-graphics root() is automatically a ForEach candidate.
    // At this point, however, we know that it is not going to be a valid
    // compute root() function (due to not having a pointer parameter). We
    // still want to return true here, so that we can issue appropriate
    // diagnostics.
    if (isRootRSFunc(FD)) {
      return true;
    }
  }

  return false;
}

bool
RSExportForEach::validateSpecialFuncDecl(unsigned int targetAPI,
                                         slang::RSContext *Context,
                                         clang::FunctionDecl const *FD) {
  slangAssert(Context && FD);
  bool valid = true;
  const clang::ASTContext &C = FD->getASTContext();
  const clang::QualType &IntType = FD->getASTContext().IntTy;

  if (isGraphicsRootRSFunc(targetAPI, FD)) {
    if ((targetAPI < SLANG_ICS_TARGET_API) && (FD->getNumParams() == 1)) {
      // Legacy graphics root function
      const clang::ParmVarDecl *PVD = FD->getParamDecl(0);
      clang::QualType QT = PVD->getType().getCanonicalType();
      if (QT != IntType) {
        Context->ReportError(PVD->getLocation(),
                             "invalid parameter type for legacy "
                             "graphics root() function: %0")
            << PVD->getType();
        valid = false;
      }
    }

    // Graphics root function, so verify that it returns an int
    if (FD->getReturnType().getCanonicalType() != IntType) {
      Context->ReportError(FD->getLocation(),
                           "root() is required to return "
                           "an int for graphics usage");
      valid = false;
    }
  } else if (isInitRSFunc(FD) || isDtorRSFunc(FD)) {
    if (FD->getNumParams() != 0) {
      Context->ReportError(FD->getLocation(),
                           "%0(void) is required to have no "
                           "parameters")
          << FD->getName();
      valid = false;
    }

    if (FD->getReturnType().getCanonicalType() != C.VoidTy) {
      Context->ReportError(FD->getLocation(),
                           "%0(void) is required to have a void "
                           "return type")
          << FD->getName();
      valid = false;
    }
  } else {
    slangAssert(false && "must be called on root, init or .rs.dtor function!");
  }

  return valid;
}

}  // namespace slang
