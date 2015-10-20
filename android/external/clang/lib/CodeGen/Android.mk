LOCAL_PATH:= $(call my-dir)

clang_codegen_TBLGEN_TABLES := \
  AttrList.inc \
  AttrParsedAttrList.inc \
  Attrs.inc \
  AttrVisitor.inc \
  CommentCommandList.inc \
  CommentNodes.inc \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \
  DiagnosticFrontendKinds.inc \
  DiagnosticSemaKinds.inc \
  StmtNodes.inc \
  arm_neon.inc

clang_codegen_SRC_FILES := \
  BackendUtil.cpp \
  CGAtomic.cpp \
  CGBlocks.cpp \
  CGBuiltin.cpp \
  CGCUDANV.cpp \
  CGCUDARuntime.cpp \
  CGCXX.cpp \
  CGCXXABI.cpp \
  CGCall.cpp \
  CGClass.cpp \
  CGCleanup.cpp \
  CGDebugInfo.cpp \
  CGDecl.cpp \
  CGDeclCXX.cpp \
  CGException.cpp \
  CGExpr.cpp \
  CGExprAgg.cpp \
  CGExprCXX.cpp \
  CGExprComplex.cpp \
  CGExprConstant.cpp \
  CGExprScalar.cpp \
  CGObjC.cpp \
  CGObjCGNU.cpp \
  CGObjCMac.cpp \
  CGObjCRuntime.cpp \
  CGOpenCLRuntime.cpp \
  CGOpenMPRuntime.cpp \
  CGLoopInfo.cpp \
  CGRecordLayoutBuilder.cpp \
  CGStmt.cpp \
  CGStmtOpenMP.cpp \
  CGVTT.cpp \
  CGVTables.cpp \
  CodeGenABITypes.cpp \
  CodeGenAction.cpp \
  CodeGenFunction.cpp \
  CodeGenModule.cpp \
  CodeGenPGO.cpp \
  CodeGenTBAA.cpp \
  CodeGenTypes.cpp \
  ItaniumCXXABI.cpp \
  MicrosoftCXXABI.cpp \
  ModuleBuilder.cpp \
  SanitizerBlacklist.cpp \
  TargetInfo.cpp

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

LOCAL_MODULE:= libclangCodeGen
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(clang_codegen_SRC_FILES)
TBLGEN_TABLES := $(clang_codegen_TBLGEN_TABLES)

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)
