//
// Created by herlight on 2019/11/12.
//

#include "tgr.hh"
#include "ast.hh"

namespace mc {
namespace tgr {

const char *RegStr(Reg reg) {
    static const char *str[] = {
            "x0",
            "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
            "t0", "t1", "t2", "t3", "t4", "t5", "t6",
            "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
    };
    return str[R2I(reg)];
}
const std::vector<Reg> &CallerSavedRegs() {
    static std::vector<Reg> regs;
    if (regs.empty()) {
        for (int i = R2I(Reg::T0); i <= R2I(Reg::A7); ++i)
            regs.push_back(I2R(i));
    }
    return regs;
}
const std::vector<Reg> &CalleeSavedRegs() {
    static std::vector<Reg> regs;
    if (regs.empty()) {
        for (int i = R2I(Reg::S0); i <= R2I(Reg::S11); ++i)
            regs.push_back(I2R(i));
    }
    return regs;
}
const std::vector<Reg> &AllRegs() {
    static std::vector<Reg> regs;
    if (regs.empty()) {
        for (int i = 1; i < RegNum; ++i)
            regs.push_back(I2R(i));
    }
    return regs;
}
std::ostream &operator<<(std::ostream &os, const Module &mod) {
    for (auto var: mod.vars)
        os << *var << std::endl;
    for (auto func: mod.funcs)
        os << *func << std::endl;
    return os;
}
std::ostream &operator<<(std::ostream &os, const Variable &var) {
    os << "v" << var.id << " = ";
    if (var.width > 0) {
        os << "malloc " << var.width;
    } else {
        os << "0";
    }
    return os;
}
std::ostream &operator<<(std::ostream &os, const Function &func) {
    os << "f_" << func.name << '[' << func.argc << "][" << func.frame_size << ']' << std::endl;
    for (auto blk: func.blocks)
        os << *blk;
    os << "end f_" << func.name;
    return os;
}
std::ostream &operator<<(std::ostream &os, const BasicBlock &blk) {
    os << "l" << blk.label << ":" << std::endl;
    for (auto op: blk.ops)
        os << "\t" << *op << std::endl;
    return os;
}
std::ostream &operator<<(std::ostream &os, const Operand &opr) {
    switch (opr.tag) {
        when(Operand::PHY_REG,
             os << RegStr(opr.val.phy_reg);)
        when(Operand::VIR_REG,
             os << "r" << opr.val.vir_reg;)
        when(Operand::INTEGER,
             os << opr.val.integer;)
        when(Operand::FRM_SLT,
             os << opr.val.frm_slt;)
        when(Operand::BSC_BLK,
             os << "l" << opr.val.bsc_blk->label;)
        when(Operand::GLB_VAR,
             os << "v" << opr.val.glb_var->id;)
        when(Operand::FUN_NME,
             os << *opr.val.fun_nme;)
        default:
            assert(false);
    }
    return os;
}
std::ostream &operator<<(std::ostream &os, const Operation &op) {
    if (op.isBinOp()) {
        os << op.oprs[0] << " = " << op.oprs[1] << " ";
        os << BinaryExpr::OpStr[static_cast<int>(op.opt)] << " ";
        os << op.oprs[2];
    } else if (op.isUnOp()) {
        os << op.oprs[0] << " = ";
        os << UnaryExpr::OpStr[static_cast<int>(op.opt)];
        os << op.oprs[1];
    } else if (op.isBrOp()) {
        os << "if " << op.oprs[0] << " ";
        os << BinaryExpr::OpStr[static_cast<int>(op.opt - Operation::BR_EQ + Operation::BIN_EQ)]
           << " ";
        os << op.oprs[1] << " goto " << op.oprs[2];
    } else {
        switch (op.opt) {
            when(Operation::MOV, {
                os << op.oprs[0] << " = " << op.oprs[1];
            })
            when(Operation::IDX_LD, {
                os << op.oprs[0] << " = " << op.oprs[1] << '[' << op.oprs[2] << ']';
            })
            when(Operation::IDX_ST, {
                os << op.oprs[0] << '[' << op.oprs[1] << ']' << " = " << op.oprs[2];
            })
            when(Operation::JUMP, {
                os << "goto " << op.oprs[0];
            })
            when(Operation::CALL, {
                os << "call f_" << op.oprs[0];
            })
            when(Operation::STORE, {
                os << "store " << op.oprs[0] << " " << op.oprs[1];
            })
            when(Operation::LOAD, {
                os << "load " << op.oprs[0] << " " << op.oprs[1];
            })
            when(Operation::LOAD_ADDR, {
                os << "loadaddr " << op.oprs[0] << " " << op.oprs[1];
            })
            when(Operation::RET, {
                os << "return";
            })
            when(Operation::__SET_PARAM, {
                os << "__set_param " << op.oprs[0] << " " << op.oprs[1];
            })
            when(Operation::__GET_PARAM, {
                os << "__get_param " << op.oprs[0] << " " << op.oprs[1];
            })
            when(Operation::__BEGIN_PARAM, {
                os << "__begin_param";
            })
            when(Operation::__SET_RET, {
                os << "__set_ret " << op.oprs[0];
            })
            when(Operation::__GET_RET, {
                os << "__get_ret " << op.oprs[0];
            })
            default:
                assert(false);
        }
    }
    return os;
}
Operand::Operand(Operand::Tag t, int i) : tag(t) {
    switch (t) {
        when(VIR_REG, val.vir_reg = i;)
        when(INTEGER, val.integer = i;)
        when(FRM_SLT, val.frm_slt = i;)
        default:
            assert(false); // cannot touch here
    }
}
Operand Operand::PhyReg(Reg r) {
    return Operand(r);
}
Operand Operand::GlbVar(Variable *v) {
    return Operand(v);
}
Operand Operand::BscBlk(BasicBlock *b) {
    return Operand(b);
}
Operand Operand::VirReg(int r) {
    return Operand(VIR_REG, r);
}
Operand Operand::FrmSlt(int f) {
    return Operand(FRM_SLT, f);
}
Operand Operand::Integer(int i) {
    return Operand(INTEGER, i);
}
Operand Operand::FuncName(const std::string &name) {
    return Operand(name);
}
void Module::addVar(Variable *var) {
    var->module = this;
    vars.push_back(var);
}
void Module::addFunc(Function *func) {
    func->module = this;
    funcs.push_back(func);
}
void Function::addBlock(BasicBlock *blk) {
    blk->function = this;
    blocks.push_back(blk);
}
int Function::extendFrame(int cnt) {
    int ret = frame_size;
    frame_size += cnt;
    return ret;
}
void BasicBlock::jump(BasicBlock *blk) {
    this->jump_out = blk;
    blk->jump_in.insert(this);
}
void BasicBlock::fall(BasicBlock *blk) {
    this->fall_out = blk;
    blk->fall_in = this;
}
std::vector<BasicBlock *> BasicBlock::outBlocks() {
    std::vector<BasicBlock *> ret;
    if (fall_out)
        ret.push_back(fall_out);
    if (jump_out)
        ret.push_back(jump_out);
    return ret;
}
std::vector<BasicBlock *> BasicBlock::inBlocks() {
    std::vector<BasicBlock *> ret;
    if (fall_in)
        ret.push_back(fall_in);
    for (auto blk: jump_in)
        ret.push_back(blk);
    return ret;
}
void BasicBlock::addOp(Operation *op) {
    op->link = ops.insert(ops.end(), op);
    op->block = this;
}
void BasicBlock::addOp(Operation::Opt opt, std::array<Operand, 3> opr) {
    addOp(new Operation(opt, std::move(opr)));
}
Operation *BasicBlock::prevOpOf(Operation *op) {
    auto it = op->link;
    if (it != ops.begin())
        return *(--it);
    return nullptr;
}
Operation *BasicBlock::nextOpOf(Operation *op) {
    auto it = op->link;
    if (++it != ops.end())
        return *it;
    return nullptr;
}
void BasicBlock::addOpBefore(Operation *pos, Operation *op) {
    assert(pos->block == this);
    auto it = pos->link;
    op->link = ops.insert(it, op);
    op->block = this;
}
void BasicBlock::addOpAfter(Operation *pos, Operation *op) {
    assert(pos->block == this);
    auto it = pos->link;
    op->link = ops.insert(++it, op);
    op->block = this;
}
void BasicBlock::removeOp(Operation *op) {
    ops.erase(op->link);
    op->block = nullptr;
}
Operation *Operation::prev() {
    return block->prevOpOf(this);
}
Operation *Operation::next() {
    return block->nextOpOf(this);
}
void Operation::addBefore(Operation *op) {
    block->addOpBefore(this, op);
}
void Operation::addAfter(Operation *op) {
    block->addOpAfter(this, op);
}
const std::set<int> &Operation::getUsedVirRegs() {
    if (modified[0]) {
        modified[0] = false;
        uses.clear();
        getDefinedVirRegs();
        for (size_t i = 0; i < oprs.size(); ++i) {
            if (oprs[i].tag == Operand::VIR_REG && !def_bits[i])
                uses.insert(oprs[i].val.vir_reg);
        }
    }
    return uses;
}
const std::set<int> &Operation::getDefinedVirRegs() {
    if (modified[1]) {
        modified[1] = false;
        defs.clear();
        def_bits.fill(false);
        if (isUnOp() || isBinOp() || opt == MOV || opt == IDX_LD || opt == __GET_RET) {
            if (oprs[0].tag == Operand::VIR_REG)
                defs.insert(oprs[0].val.vir_reg), def_bits[0] = true;
        } else if (opt == LOAD || opt == LOAD_ADDR || opt == __GET_PARAM) {
            if (oprs[1].tag == Operand::VIR_REG)
                defs.insert(oprs[1].val.vir_reg), def_bits[1] = true;
        }
    }
    return defs;
}
void Operation::rewrite(int vr, Reg pr) {
    for (auto &opr: oprs) {
        if (opr.tag == Operand::VIR_REG && opr.val.vir_reg == vr)
            opr = Operand(pr);
    }
}

}
}