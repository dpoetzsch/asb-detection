#ifndef TAINT_ANALYSIS_INSTR_TAINTGRIND_H
#define TAINT_ANALYSIS_INSTR_TAINTGRIND_H

#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include <utility>

using namespace llvm;

namespace TaintAnalysis {
    struct InstrTaintgrindVisitor : public InstVisitor<InstrTaintgrindVisitor> {
    private:
        std::vector<std::pair<Instruction*,bool>> taintSources;
        /// a vector of instructions that contain a function pointer as operand. This function pointer is the second element of the pair
        std::vector<std::pair<Instruction*,ConstantExpr*>> functionPtrSources;
        int logLevel = 10; // log everything with level <=

        void extractFunctionPtrExpr(Instruction* baseInstr, ConstantExpr* fptrExpr) {
            BasicBlock* startBlock = baseInstr->getParent();

            IRBuilder<> builder(getGlobalContext());
            builder.SetInsertPoint(startBlock);

            logState(10, startBlock->getParent(), "extractFunctionPtrExpr start");

            // 1. insert temporary variable with the result of the ConstantExpr
            Instruction* fptrInstr = fptrExpr->getAsInstruction();
            for (auto it2 = startBlock->begin(); it2 != startBlock->end(); ++it2) {
                if (&(*it2) == baseInstr) {
                    startBlock->getInstList().insert(it2, fptrInstr);
                    break;
                }
            }

            logState(10, startBlock->getParent(), "extractFunctionPtrExpr insert");

            // 2. replace usage of ConstantExpr by the new temp value
            for (auto uit = fptrExpr->use_begin(); uit != fptrExpr->use_end(); ++uit) {
                if (uit->getUser() == baseInstr) {
                    // replace the fptrExpr with the fptrInstr
                    uit->set(fptrInstr);
                    // don't break, there might be more uses in this
                }
            }

            logState(10, startBlock->getParent(), "extractFunctionPtrExpr usage repl");

            // 3. add the temp value as taint source
            taintSources.push_back(std::make_pair(fptrInstr, true));
        }

        /**
         * Insert instructions to taint or untaint a source
         */
        void instrumentValue(Instruction* taintSource, bool taint) {
            BasicBlock* startBlock = taintSource->getParent();

            logState(10, startBlock->getParent(), "instrumentValue 0 start");

            IRBuilder<> builder(getGlobalContext());
            builder.SetInsertPoint(startBlock);

            // 1. move all instructions after the source to a new BasicBlock
            BasicBlock* doBodyBlock = BasicBlock::Create(startBlock->getContext(), "doBody_" + taintSource->getName(), startBlock->getParent());
                
            BasicBlock* endBlock = BasicBlock::Create(startBlock->getContext(), "instrEnd_" + taintSource->getName(), startBlock->getParent());

            startBlock->replaceSuccessorsPhiUsesWith(endBlock);

            bool move = false;
            std::vector<Instruction*> toMove;
            for (auto it2 = startBlock->begin(); it2 != startBlock->end(); ++it2) {
                if (move) {
                    toMove.push_back(&(*it2));
                } else {
                    move = (&(*it2) == taintSource);
                }
            }

            for (auto it3 = toMove.begin(); it3 != toMove.end(); ++it3) {
                (*it3)->removeFromParent();
                endBlock->getInstList().push_back(*it3);
            }

            logState(10, endBlock->getParent(), "instrumentValue 1 move");

            // 2. Store the value in a newly allocated memory cell
            AllocaInst* taintCell = builder.CreateAlloca(taintSource->getType(), nullptr, "taintCell_" + taintSource->getName()); // TODO align 4?
            Instruction* storeTaintSrc = builder.CreateStore(taintSource, taintCell); // TODO align??

            logState(10, endBlock->getParent(), "instrumentValue 2 store");
                
            // 3. insert the taintgrind instrumentation
            // 3.a) create a taint label -- not needed
            //Value* taintLabel = builder.CreateGlobalStringPtr("taintBlue", "taintLabel"); // TODO align 1?
                
            // 3.b) allocate tmp vars
            Type* i64Ty = builder.getInt64Ty();
            Type* i64x6Ty = ArrayType::get(i64Ty, 6);
            AllocaInst* zzq_args = builder.CreateAlloca(i64x6Ty, nullptr, "_zzq_args"); // TODO align 16?
            AllocaInst* zzq_result = builder.CreateAlloca(i64Ty, nullptr, "_zzq_result"); // TODO align 8?
            AllocaInst* tmp = builder.CreateAlloca(i64Ty, nullptr, "tmp"); // TODO align 8?

            builder.CreateBr(doBodyBlock);
            builder.SetInsertPoint(doBodyBlock);
                
            // 3.c) spawn taintgrind instrumentation
            int val = taint ? 12 : 13; // 12 means taint, 13 means untaint
            Value* arrayidx = builder.CreateConstInBoundsGEP2_64(zzq_args, 0, 0, "arrayidx");
            builder.CreateStore(ConstantInt::get(i64Ty, val), arrayidx, true); // TODO align 16?

            Value* taintCellI64 = builder.CreatePtrToInt(taintCell, i64Ty);
            Value* arrayidx1 = builder.CreateConstInBoundsGEP2_64(zzq_args, 0, 1, "arrayidx1");
            builder.CreateStore(taintCellI64, arrayidx1, true); // TODO align 8?

            Value* arrayidx2 = builder.CreateConstInBoundsGEP2_64(zzq_args, 0, 2, "arrayidx2");
            DataLayout dl(taintSource->getModule());
            builder.CreateStore(ConstantInt::get(i64Ty, dl.getTypeAllocSize(taintSource->getType())), arrayidx2, true); // TODO align 16?

            Value* arrayidx3 = builder.CreateConstInBoundsGEP2_64(zzq_args, 0, 3, "arrayidx3");
            //builder.CreateStore(builder.CreatePtrToInt(taintLabel, i64Ty), arrayidx3, true); // TODO align 8?
            builder.CreateStore(ConstantInt::get(i64Ty, 0), arrayidx3, true); // TODO align 8?

            Value* arrayidx4 = builder.CreateConstInBoundsGEP2_64(zzq_args, 0, 4, "arrayidx4");
            builder.CreateStore(ConstantInt::get(i64Ty, 0), arrayidx4, true); // TODO align 16?

            Value* arrayidx5 = builder.CreateConstInBoundsGEP2_64(zzq_args, 0, 5, "arrayidx5");
            builder.CreateStore(ConstantInt::get(i64Ty, 0), arrayidx5, true); // TODO align 8?

            Value* arrayidx6 = builder.CreateConstInBoundsGEP2_64(zzq_args, 0, 0, "arrayidx6");
            FunctionType* asmTy = FunctionType::get(i64Ty, {PointerType::getUnqual(i64Ty), i64Ty}, false);
            InlineAsm* tgAsm = InlineAsm::get(asmTy, "rolq $$3,  %rdi ; rolq $$13, %rdi\x0A\x09rolq $$61, %rdi ; rolq $$51, %rdi\x0A\x09xchgq %rbx,%rbx", "={dx},{ax},0,~{cc},~{memory},~{dirflag},~{fpsr},~{flags}", true);
            CallInst* callAsm = builder.CreateCall(tgAsm, {arrayidx6, ConstantInt::get(i64Ty, 0)});
            // TODO callAsm->addAttribute(0, Attribute::AttrKind::NoUnwind);

            builder.CreateStore(callAsm, zzq_result, true); // TODO align 8?
            Value* zzq_resultLoad = builder.CreateLoad(zzq_result, true); // TODO align 8?

            builder.CreateStore(zzq_resultLoad, tmp); // TODO align 8?
            builder.CreateLoad(tmp); // TODO align 8?
                
            logState(10, endBlock->getParent(), "instrumentValue 3 instr");
                
            // 4. load the value from memory again and replace all usages of the taint source with the loaded value
            Value* taintedPtr = builder.CreateLoad(taintCell, "tainted_" + taintSource->getName());

            while (taintSource->hasNUsesOrMore(2)) {
                auto uit = taintSource->use_begin();

                if (uit->getUser() == storeTaintSrc) {
                    ++uit;
                }
                assert(uit->getUser() != storeTaintSrc && "should be covered above");
                
                // replace the taintSource with the taintedPtr
                uit->set(taintedPtr);
            }
                
            logState(10, endBlock->getParent(), "instrumentValue 4 load");

            // 5. branch to the endBlock
            builder.CreateBr(endBlock);

            logState(20, endBlock->getParent(), "instrumentValue 5 br");
        }

        void checkOperandForFuncPtrs(Instruction* baseInstr, Value* op) {
            if (isa<ConstantExpr>(op)) {
                ConstantExpr* ce = dyn_cast<ConstantExpr>(op);
                for (auto uit = ce->op_begin(); uit != ce->op_end(); ++uit) {
                    Value* ce_op = *uit;
                    if (isa<Function>(ce_op)) {
                        functionPtrSources.push_back(std::make_pair(baseInstr, ce));
                    }
                }
            }
        }
 
    public:
        InstrTaintgrindVisitor() {}
        InstrTaintgrindVisitor(int logLevel) : logLevel(logLevel) {}

        void visitAllocaInst(AllocaInst &I) {
            taintSources.push_back(std::make_pair(&I, true));
        }

        void visitStoreInst(StoreInst &I) {
            checkOperandForFuncPtrs(&I, I.getValueOperand());
        }

        void visitReturnInst(ReturnInst &I) {
            if (I.getReturnValue() != nullptr) {
                checkOperandForFuncPtrs(&I, I.getReturnValue());
            }
        }

        void visitCallInst(CallInst &I) {
            for (auto uit = I.arg_begin(); uit != I.arg_end(); ++uit) {
                checkOperandForFuncPtrs(&I, *uit);
            }
        }

        void visitICmpInst(ICmpInst &i) {
            //
            for (auto uit = i.op_begin(); uit != i.op_end(); ++uit) {
                Value* op = *uit;
                if (isa<ConstantPointerNull>(op)) {
                    // untaint
                    taintSources.push_back(std::make_pair(&i, false));
                    break;
                } else if (isa<ConstantInt>(op)) {
                    ConstantInt* c = dyn_cast<ConstantInt>(op);
                    if (c->isZero()) {
                        // untaint
                        taintSources.push_back(std::make_pair(&i, false));
                        break;
                    }
                }
            }
        }
        
        void visitInstruction(Instruction &I) {}  // Ignore unhandled instructions
                
        /// @return true if the taint for this function changed
        bool instrumentFunction(Function& f) {
            taintSources.clear();
            functionPtrSources.clear();
            visit(f);

            for (auto it = functionPtrSources.begin(); it != functionPtrSources.end(); ++it) {
                extractFunctionPtrExpr(it->first, it->second);
            }

            for (auto it = taintSources.begin(); it != taintSources.end(); ++it) {
                // Ok, let's taint/untaint that value!
                instrumentValue(it->first, it->second);
            }

            return !taintSources.empty();
        }

        void logState(int logLevel, Function* f, std::string step) {
            if (logLevel <= this->logLevel) {
                f->print(errs());
                errs() << "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ end of step " << step << " in function " << f->getName() << "\n";
            }
        }

        /// @return true if the module was modified
        bool instrumentModule(Module& M) {
            bool modified = false;

            // iterate over the functions in the module and instrument them
            for (Module::iterator mi = M.begin(), me = M.end(); mi != me; ++mi) {
                Function& f = *mi;
                bool tc = instrumentFunction(f);
                modified = modified || tc;
            }

            return modified;
        }
    };
}

#endif
