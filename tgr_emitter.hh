//
// Created by herlight on 2019/11/17.
//

#ifndef __MC_TGR_EMITTER_HH__
#define __MC_TGR_EMITTER_HH__

#include "tgr.hh"
#include "eyr.hh"
#include <map>

namespace mc {
namespace tgr {

class TgrEmitter {
public:
    Module *emit(eyr::Module *e_mod);

private:
    void runOnGlobalVar(eyr::Variable *e_var);
    void runOnFunction(eyr::Function *e_func);
    void runOnBlock(eyr::BasicBlock *e_blk);
    void runOnInstruction(eyr::Instruction *e_inst);

    Module *cur_mod{nullptr};
    Function *cur_func{nullptr};
    BasicBlock *cur_blk{nullptr};

    std::map<eyr::Variable *, int> var2reg;
    std::map<eyr::Variable *, Variable *> var2var;
    std::map<eyr::Variable *, int> frm_slt;

    int allocVR();
    Variable *allocGV(int width = 0);
    int loadVar(eyr::Variable *x);
    int loadAddr(eyr::Variable *x);
    void storeVar(const Operand& opr, eyr::Variable *var);
    void gen(Operation::Opt opt, std::array<Operand, 3> oprs);

    void emitBinaryInst(eyr::BinaryInst *inst);
    void emitUnaryInst(eyr::UnaryInst *inst);
    void emitCallInst(eyr::CallInst *inst);
    void emitMoveInst(eyr::MoveInst *inst);
    void emitStoreInst(eyr::StoreInst *inst);
    void emitLoadInst(eyr::LoadInst *inst);
    void emitBranchInst(eyr::BranchInst *inst);
    void emitJumpInst(eyr::JumpInst *inst);
    void emitReturnInst(eyr::ReturnInst *inst);
};

};
};

#endif //__MC_TGR_EMITTER_HH__
