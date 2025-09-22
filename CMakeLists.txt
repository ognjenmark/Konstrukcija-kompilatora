add_llvm_library( LLVMOurDominatorPass MODULE BUILDTREE_ONLY
  OurCFG.cpp
  ConstantPropagationInstruction.cpp
  OurConstantPropagationPass.cpp
  OurConstantFoldingPass.cpp
  OurCSEPass.cpp
  OurDeadCodeEliminationPass.cpp
  DeadBasicBlockEliminationPass.cpp
  
  PLUGIN_TOOL
  opt
  )
