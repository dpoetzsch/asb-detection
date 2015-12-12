#include <assert.h>

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/ADT/ArrayRef.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"

using namespace llvm;

namespace {
    enum TaintKind {
        TAINT_NONE,
        TAINT_MAYBE,
        TAINT_DEFINITELY
    };

    struct CountAllocaVisitor : public InstVisitor<CountAllocaVisitor, TaintKind> {
        int verbosity;
        
        CountAllocaVisitor() : verbosity(1) {}

        TaintKind mergeTaintKinds(TaintKind t1, TaintKind t2) {
            switch (t1) {
            case TAINT_NONE:
                return t2;
            case TAINT_MAYBE:
                if (t2 != TAINT_DEFINITELY) {
                    return TAINT_MAYBE;
                } else {
                    return t2;
                }
            case TAINT_DEFINITELY:
                return TAINT_DEFINITELY;
            default:
                assert(false);
            }
        }

        TaintKind treatInstruction(Instruction &inst) {
            if (verbosity > 0) inst.print(errs());
            if (verbosity > 1) errs() << "  :\n";
            
            TaintKind taint = TAINT_NONE;
            
            for (int i = 0; i < inst.getNumOperands(); ++i) {
                Value *op = inst.getOperand(i);
                if (verbosity > 1) errs() << "    ";
                if (verbosity > 1) op->printAsOperand(errs());

                if (isa<Constant>(op)) {
                    if (verbosity > 1) errs() << " :: CONSTANT -> TAINT_NONE";
                } else if (isa<Instruction>(op)) {
                    if (verbosity > 1) errs() << " :: INSTRUCTION -> ";
                    bool oldVerbosity = verbosity;
                    verbosity = false;
                    TaintKind opTaint = visit(dyn_cast<Instruction>(op));
                    verbosity = oldVerbosity;

                    if (verbosity > 1) printTaintKind(opTaint);
                    
                    taint = mergeTaintKinds(taint, opTaint);
                } else if (isa<Argument>(op)) {
                    if (verbosity > 1) errs() << " :: VALUE -> TAINT_MAYBE";

                    taint = mergeTaintKinds(taint, TAINT_MAYBE);
                } else if (!isa<MetadataAsValue>(op)) {
                    assert(false); // NOT IMPLEMENTED
                }
                
                if (verbosity > 1) errs() << "\n";
            }
            
            if (verbosity > 0) errs() << "    -> ";
            if (verbosity > 0) printTaintKind(taint);
            if (verbosity > 0) errs() << "\n";
            
            if (verbosity > 1) errs() << "\n";

//            if (inst.getDebugLoc())
//                errs() << inst.getDebugLoc().getLine();

            return taint;
        }

        void printTaintKind(TaintKind t) {
            switch (t) {
            case TAINT_NONE:
                errs() << "TAINT_NONE";
                break;
            case TAINT_MAYBE:
                errs() << "TAINT_MAYBE";
                break;
            case TAINT_DEFINITELY:
                errs() << "TAINT_DEFINITELY";
                break;
            }
        }

        TaintKind printWithTaint(Instruction& instr, TaintKind t) {
            if (verbosity > 0) {
                instr.print(errs());
                errs() << "    -> ";
                printTaintKind(t);
                errs() << "\n";
            }
            if (verbosity > 1) {
                errs() << "\n";
            }
            return t;
        }
        
        TaintKind visitReturnInst(ReturnInst &I)            { return treatInstruction(I);}
        TaintKind visitBranchInst(BranchInst &I)            { return treatInstruction(I);}
        TaintKind visitSwitchInst(SwitchInst &I)            { return treatInstruction(I);}
        TaintKind visitIndirectBrInst(IndirectBrInst &I)    { return treatInstruction(I);}
        TaintKind visitResumeInst(ResumeInst &I)            { return treatInstruction(I);}
        TaintKind visitUnreachableInst(UnreachableInst &I)  { return treatInstruction(I);}
        TaintKind visitCleanupReturnInst(CleanupReturnInst &I) { return treatInstruction(I);}
        TaintKind visitCleanupEndPadInst(CleanupEndPadInst &I) { return treatInstruction(I); }
        TaintKind visitCatchReturnInst(CatchReturnInst &I)  { return treatInstruction(I); }
        TaintKind visitCatchPadInst(CatchPadInst &I)    { return treatInstruction(I);}
        TaintKind visitCatchEndPadInst(CatchEndPadInst &I) { return treatInstruction(I); }
        TaintKind visitTerminatePadInst(TerminatePadInst &I) { return treatInstruction(I);}
        TaintKind visitICmpInst(ICmpInst &I)                { return treatInstruction(I);}
        TaintKind visitFCmpInst(FCmpInst &I)                { return treatInstruction(I);}
        TaintKind visitAllocaInst(AllocaInst &I)            {
            return printWithTaint(I, TAINT_DEFINITELY);
        }
        TaintKind visitLoadInst(LoadInst     &I)            {
            return printWithTaint(I, TAINT_MAYBE);
        }
        TaintKind visitStoreInst(StoreInst   &I)            {
            return printWithTaint(I, TAINT_NONE);
        }
        TaintKind visitAtomicCmpXchgInst(AtomicCmpXchgInst &I) { return treatInstruction(I);}
        TaintKind visitAtomicRMWInst(AtomicRMWInst &I)      { return treatInstruction(I);}
        TaintKind visitFenceInst(FenceInst   &I)            { return treatInstruction(I);}
        TaintKind visitGetElementPtrInst(GetElementPtrInst &I){ return treatInstruction(I);}
        TaintKind visitPHINode(PHINode       &I)            { return treatInstruction(I);}
        TaintKind visitTruncInst(TruncInst &I)              { return treatInstruction(I);}
        TaintKind visitZExtInst(ZExtInst &I)                { return treatInstruction(I);}
        TaintKind visitSExtInst(SExtInst &I)                { return treatInstruction(I);}
        TaintKind visitFPTruncInst(FPTruncInst &I)          { return treatInstruction(I);}
        TaintKind visitFPExtInst(FPExtInst &I)              { return treatInstruction(I);}
        TaintKind visitFPToUIInst(FPToUIInst &I)            { return treatInstruction(I);}
        TaintKind visitFPToSIInst(FPToSIInst &I)            { return treatInstruction(I);}
        TaintKind visitUIToFPInst(UIToFPInst &I)            { return treatInstruction(I);}
        TaintKind visitSIToFPInst(SIToFPInst &I)            { return treatInstruction(I);}
        TaintKind visitPtrToIntInst(PtrToIntInst &I)        { return treatInstruction(I);}
        TaintKind visitIntToPtrInst(IntToPtrInst &I)        { return treatInstruction(I);}
        TaintKind visitBitCastInst(BitCastInst &I)          { return treatInstruction(I);}
        TaintKind visitAddrSpaceCastInst(AddrSpaceCastInst &I) { return treatInstruction(I);}
        TaintKind visitSelectInst(SelectInst &I)            { return treatInstruction(I);}
        TaintKind visitVAArgInst(VAArgInst   &I)            { return treatInstruction(I);}
        TaintKind visitExtractElementInst(ExtractElementInst &I) { return treatInstruction(I);}
        TaintKind visitInsertElementInst(InsertElementInst &I) { return treatInstruction(I);}
        TaintKind visitShuffleVectorInst(ShuffleVectorInst &I) { return treatInstruction(I);}
        TaintKind visitExtractValueInst(ExtractValueInst &I){ return treatInstruction(I);}
        TaintKind visitInsertValueInst(InsertValueInst &I)  { return treatInstruction(I); }
        TaintKind visitLandingPadInst(LandingPadInst &I)    { return treatInstruction(I); }
        TaintKind visitCleanupPadInst(CleanupPadInst &I) { return treatInstruction(I); }
        
        // Handle the special instrinsic instruction classes.
        TaintKind visitDbgDeclareInst(DbgDeclareInst &I)    { return treatInstruction(I);}
        TaintKind visitDbgValueInst(DbgValueInst &I)        { return treatInstruction(I);}
        TaintKind visitDbgInfoIntrinsic(DbgInfoIntrinsic &I) { return treatInstruction(I); }
        TaintKind visitMemSetInst(MemSetInst &I)            { return treatInstruction(I); }
        TaintKind visitMemCpyInst(MemCpyInst &I)            { return treatInstruction(I); }
        TaintKind visitMemMoveInst(MemMoveInst &I)          { return treatInstruction(I); }
        TaintKind visitMemTransferInst(MemTransferInst &I)  { return treatInstruction(I); }
        TaintKind visitMemIntrinsic(MemIntrinsic &I)        { return treatInstruction(I); }
        TaintKind visitVAStartInst(VAStartInst &I)          { return treatInstruction(I); }
        TaintKind visitVAEndInst(VAEndInst &I)              { return treatInstruction(I); }
        TaintKind visitVACopyInst(VACopyInst &I)            { return treatInstruction(I); }
        TaintKind visitIntrinsicInst(IntrinsicInst &I)      { return treatInstruction(I); }
        
        TaintKind visitInstruction(Instruction &I) { return treatInstruction(I); }  // Ignore unhandled instructions
    };


    struct bishe_insert : public ModulePass {
        static char ID;  
        Function* hook;

        bishe_insert() : ModulePass(ID) {}

        virtual bool runOnModule(Module &M) {
            // void print(int64)
            /*LLVMContext& ctx = M.getContext();
            Constant* hookFunc = M.getOrInsertFunction("print",
                FunctionType::getVoidTy(ctx), Type::getInt64Ty(ctx), nullptr);
                hook = cast<Function>(hookFunc);*/
            
            // iterate over the functions in the module
            for (Module::iterator mi = M.begin(), me = M.end(); mi != me; ++mi) {
                errs() << mi->getName() << "(...):\n";
                
                // iterate over the basic blocks in the function
                for (Function::iterator fi = mi->begin(), fe = mi->end(); fi != fe; ++fi) {
                    /*fi->dump();
                    if (fi->hasName())
                    errs() << fi->getName();*/
                    runOnBasicBlock(fi);
                }
            }

            return false;
        }
        
        virtual bool runOnBasicBlock(Function::iterator &fi) {
            errs() << fi->getName() << ":\n";
            
            CountAllocaVisitor v;
            v.visit(*fi);

            errs() << "\n";
        
            // Iterate over the items in the basic block
            for (BasicBlock::iterator bi = fi->begin(), be = fi->end(); bi != be; ++bi) {
                // find all cast instructions
                if (CastInst* castInst = dyn_cast<CastInst>(bi)) {
                    // only if this is a cast from ptr to int
                    if (castInst->getSrcTy()->isPointerTy() && castInst->getDestTy()->isIntegerTy()) {
                        /*Value *operand = castInst->getOperand(0);
                        operand->printAsOperand(errs());
                        errs() << "\n";*/
                        
                        errs() << "Found cast instruction of type: ";
                        castInst->getType()->print(errs());
                        errs() << "\n";
                        
                        // create and insert the print instruction that prints the value of the pointer
                        ArrayRef<Value*> args = ArrayRef<Value*>(castInst);
                        Instruction *newInst = CallInst::Create(hook, args, "");
                        fi->getInstList().insertAfter((Instruction*) castInst, newInst);
                    }
                }
            }
            return true;
        }
    };
}

char bishe_insert::ID = 0;
static RegisterPass<bishe_insert> X("bishe_insert", "test function exist", false, false);
