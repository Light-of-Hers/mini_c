//
// Created by herlight on 2019/11/17.
//

#include "tgr_emitter.hh"
#include "tgr.hh"

namespace mc {
namespace tgr {

static inline Operand VR(int vr) {
    return Operand::VirReg(vr);
}
static inline Operand GV(Variable *var) {
    return Operand::GlbVar(var);
}
static inline Operand PR(Reg pr) {
    return Operand::PhyReg(pr);
}
static inline Operand BB(BasicBlock *blk) {
    return Operand::BscBlk(blk);
}
static inline Operand INT(int i) {
    return Operand::Integer(i);
}
static inline Operand FS(int f) {
    return Operand::FrmSlt(f);
}

Module *TgrEmitter::emit(eyr::Module *e_mod) {
    cur_mod = new Module;
    for (auto var: e_mod->global_vars)
        runOnGlobalVar(var);
    for (auto func: e_mod->global_funcs)
        runOnFunction(func);
    return cur_mod;
}
void TgrEmitter::runOnGlobalVar(eyr::Variable *e_var) {
    assert(e_var->isGlobal());
    auto var = allocGV(e_var->width);
    cur_mod->addVar(var);
    var2var[e_var] = var;
}
void TgrEmitter::runOnFunction(eyr::Function *e_func) {
    cur_func = new Function(e_func->name, e_func->params.size());
    cur_mod->addFunc(cur_func);
    for (auto _: e_func->blocks)
        (void) _, cur_func->addBlock(new BasicBlock(cur_mod->next_label++));
    for (size_t i = 0; i < e_func->blocks.size(); ++i) {
        if (auto blk = e_func->blocks[i]->fall_out)
            cur_func->blocks[i]->fall(cur_func->blocks[blk->f_idx]);
        if (auto blk = e_func->blocks[i]->jump_out)
            cur_func->blocks[i]->jump(cur_func->blocks[blk->f_idx]);
    }
    { // retrieve arguments
        cur_blk = cur_func->blocks[0];
        assert(e_func->params.size() <= 8);
        for (size_t i = 0; i < e_func->params.size(); ++i) {
            auto p = e_func->params[i];
            auto rp = loadVar(p);
            gen(Operation::__GET_PARAM, {INT(i), VR(rp)});
        }
    }
    // emit codes
    for (size_t i = 0; i < e_func->blocks.size(); ++i) {
        cur_blk = cur_func->blocks[i];
        runOnBlock(e_func->blocks[i]);
    }
}
void TgrEmitter::runOnBlock(eyr::BasicBlock *e_blk) {
    for (auto inst: e_blk->insts)
        runOnInstruction(inst);
}
void TgrEmitter::runOnInstruction(eyr::Instruction *e_inst) {
    if (auto inst = dynamic_cast<eyr::BinaryInst *>(e_inst)) {
        emitBinaryInst(inst);
    } else if (auto inst = dynamic_cast<eyr::UnaryInst *>(e_inst)) {
        emitUnaryInst(inst);
    } else if (auto inst = dynamic_cast<eyr::CallInst *>(e_inst)) {
        emitCallInst(inst);
    } else if (auto inst = dynamic_cast<eyr::MoveInst *>(e_inst)) {
        emitMoveInst(inst);
    } else if (auto inst = dynamic_cast<eyr::StoreInst *>(e_inst)) {
        emitStoreInst(inst);
    } else if (auto inst = dynamic_cast<eyr::LoadInst *>(e_inst)) {
        emitLoadInst(inst);
    } else if (auto inst = dynamic_cast<eyr::BranchInst *>(e_inst)) {
        emitBranchInst(inst);
    } else if (auto inst = dynamic_cast<eyr::JumpInst *>(e_inst)) {
        emitJumpInst(inst);
    } else if (auto inst = dynamic_cast<eyr::ReturnInst *>(e_inst)) {
        emitReturnInst(inst);
    } else {
        // cannot touch here
        assert(false);
    }
}
void TgrEmitter::emitBinaryInst(eyr::BinaryInst *inst) {
    auto opt = static_cast<Operation::Opt>(inst->opt);
    auto x = inst->dst;
    Operand oy, oz;
    oy = VR(loadOpr(inst->lhs));
    if (inst->rhs.imm) {
        if (opt == Operation::BIN_ADD || opt == Operation::BIN_LT) {
            oz = INT(inst->rhs.val);
        } else {
            oz = VR(loadOpr((inst->rhs)));
        }
    } else {
        oz = VR(loadOpr((inst->rhs)));
    }
    if (x->isGlobal()) {
        auto r1 = allocVR();
        gen(opt, {VR(r1), oy, oz});
        storeVar(VR(r1), x);
    } else {
        auto rx = loadVar(x);
        gen(opt, {VR(rx), oy, oz});
    }

}
void TgrEmitter::emitUnaryInst(eyr::UnaryInst *inst) {
    auto opt = static_cast<Operation::Opt>(inst->opt);
    auto x = inst->dst;
    auto ry = loadOpr(inst->opr);
    if (x->isGlobal()) {
        auto r1 = allocVR();
        gen(opt, {VR(r1), VR(ry)});
        storeVar(VR(r1), x);
    } else {
        auto rx = loadVar(x);
        gen(opt, {VR(rx), VR(ry)});
    }
}
void TgrEmitter::emitCallInst(eyr::CallInst *inst) {
    assert(inst->args.size() <= 8);
    gen(Operation::__BEGIN_PARAM, {});
    for (size_t i = 0; i < inst->args.size(); ++i) {
        auto x = inst->args[i].var;
        if (x == nullptr) {
            gen(Operation::__SET_PARAM, {INT(inst->args[i].val), INT(i)});
        } else if (x->isGlobal()) {
            auto vx = var2var[x];
            gen(Operation::__SET_PARAM, {GV(vx), INT(i)});
        } else if (x->isAddr()) {
            auto it = frm_slt.find(x);
            if (it == frm_slt.end())
                it = frm_slt.insert({x, cur_func->extendFrame(x->width / 4)}).first;
            auto fx = it->second;
            gen(Operation::__SET_PARAM, {FS(fx), INT(i)});
        } else {
            auto rx = loadVar(x);
            gen(Operation::__SET_PARAM, {VR(rx), INT(i)});
        }
    }
    gen(Operation::CALL, {Operand::FuncName(inst->name)});
    auto x = inst->dst;
    assert(x->isLocal());
    auto rx = loadVar(x);
    gen(Operation::__GET_RET, {VR(rx)});
}
void TgrEmitter::emitMoveInst(eyr::MoveInst *inst) {
    auto x = inst->dst;
    auto s = inst->src;
    if (s.imm) {
        storeVar(INT(s.val), x);
    } else {
        auto y = s.var;
        if (y->isLocal()) {
            storeVar(VR(loadVar(y)), x);
        } else if (x->isLocal()) {
            auto rx = loadVar(x);
            gen(Operation::LOAD, {GV(var2var[y]), VR(rx)});
        } else {
            storeVar(VR(loadVar(y)), x);
        }
    }
}
void TgrEmitter::emitStoreInst(eyr::StoreInst *inst) {
    auto rx = loadAddr(inst->base), ry = loadOpr(inst->idx), rz = loadOpr(inst->src);
    gen(Operation::BIN_ADD, {VR(rx), VR(rx), VR(ry)});
    gen(Operation::IDX_ST, {VR(rx), INT(0), VR(rz)});
}
void TgrEmitter::emitLoadInst(eyr::LoadInst *inst) {
    auto x = inst->dst;
    auto ry = loadAddr(inst->src), rz = loadOpr(inst->idx);
    gen(Operation::BIN_ADD, {VR(ry), VR(ry), VR(rz)});
    if (x->isLocal()) {
        auto rx = loadVar(x);
        gen(Operation::IDX_LD, {VR(rx), VR(ry), INT(0)});
    } else {
        auto rx = allocVR();
        gen(Operation::IDX_LD, {VR(rx), VR(ry), INT(0)});
        storeVar(VR(rx), x);
    }
}
void TgrEmitter::emitBranchInst(eyr::BranchInst *inst) {
    assert(inst->opt == eyr::BranchInst::LgcOp::NE);
    assert(inst->rhs.imm && inst->rhs.val == 0);
    auto rx = loadOpr(inst->lhs);
    gen(Operation::BR_NE, {
            VR(rx), PR(Reg::X0), BB(cur_func->blocks[inst->dst->f_idx]),
    });
}
void TgrEmitter::emitJumpInst(eyr::JumpInst *inst) {
    gen(Operation::JUMP, {
            BB(cur_func->blocks[inst->dst->f_idx]),
    });
}
void TgrEmitter::emitReturnInst(eyr::ReturnInst *inst) {
    auto x = inst->opr.var;
    if (x == nullptr) {
        gen(Operation::__SET_RET, {INT(inst->opr.val)});
    } else if (x->isGlobal()) {
        auto vx = var2var[x];
        gen(Operation::__SET_RET, {GV(vx)});
    } else if (x->isAddr()) {
        auto fx = loadAddr(x);
        gen(Operation::__SET_RET, {FS(fx)});
    } else {
        auto rx = loadVar(x);
        gen(Operation::__SET_RET, {VR(rx)});
    }
}
int TgrEmitter::allocVR() {
    return cur_mod->next_vir_reg_id++;
}
Variable *TgrEmitter::allocGV(int width) {
    auto var = new Variable(cur_mod->next_glb_var_id++, width);
    return var;
}
int TgrEmitter::loadVar(eyr::Variable *x) {
    if (x->isGlobal()) {
        auto rx = allocVR();
        gen(Operation::LOAD, {GV(var2var[x]), VR(rx)});
        return rx;
    } else {
        auto it = var2reg.find(x);
        if (it == var2reg.end())
            it = var2reg.insert({x, allocVR()}).first;
        return it->second;
    }
}
int TgrEmitter::loadAddr(eyr::Variable *x) {
    if (x->isGlobal()) {
        auto rx = allocVR();
        gen(Operation::LOAD_ADDR, {GV(var2var[x]), VR(rx)});
        return rx;
    } else if (x->isParam()) {
        auto rx = loadVar(x);
        auto rt = allocVR();
        gen(Operation::MOV, {VR(rt), VR(rx)});
        return rt;
    } else {
        auto rx = allocVR();
        auto it = frm_slt.find(x);
        if (it == frm_slt.end())
            it = frm_slt.insert({x, cur_func->extendFrame(x->width / 4)}).first;
        auto fx = it->second;
        gen(Operation::LOAD_ADDR, {FS(fx), VR(rx)});
        return rx;
    }
}
void TgrEmitter::storeVar(const Operand &opr, eyr::Variable *x) {
    if (x->isGlobal()) {
        auto rx = allocVR();
        gen(Operation::LOAD_ADDR, {GV(var2var[x]), VR(rx)});
        if (opr.tag == Operand::INTEGER) {
            auto ri = allocVR();
            gen(Operation::MOV, {VR(ri), INT(opr.val.integer)});
            gen(Operation::IDX_ST, {VR(rx), INT(0), VR(ri)});
        } else {
            assert(opr.tag == Operand::VIR_REG);
            gen(Operation::IDX_ST, {VR(rx), INT(0), VR(opr.val.vir_reg)});
        }
    } else {
        auto rx = loadVar(x);
        gen(Operation::MOV, {VR(rx), opr});
    }
}
void TgrEmitter::gen(Operation::Opt opt, std::array<Operand, 3> oprs) {
    cur_blk->addOp(opt, oprs);
}
int TgrEmitter::loadOpr(eyr::Operand opr) {
    if (opr.imm) {
        auto rt = allocVR();
        gen(Operation::MOV, {VR(rt), INT(opr.val)});
        return rt;
    } else {
        return loadVar(opr.var);
    }
}

}
}

