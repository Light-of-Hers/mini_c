//
// Created by herlight on 2019/11/18.
//

#include "ra_greedy.hh"
#include "tgr.hh"
#include <algorithm>
#include <cstdlib>
#include <functional>

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

void RAGreedy::allocate(Module *mod) {
    cur_mod = mod;

    for (auto fun : mod->funcs) {
        runOnFunction(fun);
    }
}
void RAGreedy::runOnFunction(Function *fun) {
    cur_fun = fun;

    cur_exit = new BasicBlock(cur_mod->next_label++);
    taint_regs.clear(), vr2slt.clear();
    for (auto blk : fun->blocks) {
        runOnBlock(blk);
    }
    auto end_blk = cur_fun->blocks.back();
    cur_fun->addBlock(cur_exit);
    assert(!end_blk->fall_out);
    if (!end_blk->jump_out)
        end_blk->fall(cur_exit);
    saveCalleeSavedRegs();
    cur_exit->addOp(Operation::RET, {});
}
void RAGreedy::runOnBlock(BasicBlock *blk) {
    cur_blk = blk;

    live_vrs.clear(), vr2pr.clear(), pr2vr.clear();
    in_slt_vr = blk->live_in;
    liveAnalyze();
    cur_op = cur_blk->ops.empty() ? nullptr : cur_blk->ops.front();
    while (cur_op) {
        runOnOperation(cur_op);
    }
    saveRegsInBlockEnd();
}
void RAGreedy::runOnOperation(Operation *op) {
    debugv(*op);
    auto opt = op->opt;
    if (opt == Operation::MOV) {
        handleMov(op);
    } else if (opt == Operation::__BEGIN_PARAM) {
        handleBeginParam(op);
    } else if (opt == Operation::__SET_PARAM) {
        handleSetParam(op);
    } else if (opt == Operation::__GET_PARAM) {
        handleGetParam(op);
    } else if (opt == Operation::__SET_RET) {
        handleSetRet(op);
    } else if (opt == Operation::__GET_RET) {
        handleGetRet(op);
    } else {
        handleOthers(op);
    }
}
void RAGreedy::link(int r, Reg p) {
    refPhyRegs(r).insert(p);
    refVirRegs(p).insert(r);
}
void RAGreedy::unlink(int r) {
    leaveFrame(r);
    auto &my_prs = refPhyRegs(r);
    for (auto pr : my_prs)
        refVirRegs(pr).erase(r);
    my_prs.clear();
}
void RAGreedy::unlink(Reg p) {
    auto &my_vrs = refVirRegs(p);
    for (auto r : my_vrs) {
        auto &my_prs = refPhyRegs(r);
        my_prs.erase(p);
        if (my_prs.empty())
            spill(r, p);
    }
    my_vrs.clear();
}
void RAGreedy::spill(int r, Reg p) {
    if (!isInFrame(r) && isAlive(r)) {
        auto slt = getFrmSlt(r);
        genBefore(Operation::STORE, {PR(p), FS(slt)});
        enterFrame(r);
    }
}
void RAGreedy::genAfter(Operation::Opt opt, std::array<Operand, 3> oprs) {
    auto new_op = new Operation(opt, oprs);
    cur_op->addAfter(new_op);
}
std::set<int> &RAGreedy::refVirRegs(Reg pr) {
    return pr2vr[pr];
}
std::set<Reg> &RAGreedy::refPhyRegs(int vr) {
    return vr2pr[vr];
}
int RAGreedy::getFrmSlt(int vr) {
    auto it = vr2slt.find(vr);
    if (it == vr2slt.end())
        it = vr2slt.insert({vr, cur_fun->extendFrame(1)}).first;
    return it->second;
}
void RAGreedy::enterFrame(int vr) {
    in_slt_vr.insert(vr);
}
void RAGreedy::leaveFrame(int vr) {
    in_slt_vr.erase(vr);
}
bool RAGreedy::isInFrame(int vr) {
    return in_slt_vr.count(vr) > 0;
}
void RAGreedy::liveAnalyze() {
    live_vrs.clear();
    auto cur_live = cur_blk->live_out;
    for (Operation *op : reverse(cur_blk->ops)) {
        for (auto def : op->getDefinedVirRegs())
            cur_live.erase(def);
        for (auto use : op->getUsedVirRegs())
            cur_live.insert(use);
        live_vrs[op] = cur_live;
    }
}
void RAGreedy::genBefore(Operation::Opt opt, std::array<Operand, 3> oprs) {
    auto new_op = new Operation(opt, oprs);
    cur_op->addBefore(new_op);
}
bool RAGreedy::isAlive(int r) {
    return live_vrs[cur_op].count(r) > 0;
}
bool RAGreedy::isInPhyReg(int vr) {
    return !refPhyRegs(vr).empty();
}
bool RAGreedy::isOccupied(Reg pr) {
    auto &my_vrs = refVirRegs(pr);
    for (auto vr : my_vrs) {
        if (isAlive(vr))
            return true;
    }
    return false;
}
void RAGreedy::allocPhyRegsFor(Operation *op) {
    auto uses = op->getUsedVirRegs();
    auto defs = op->getDefinedVirRegs();
    for (auto vr : uses) {
        if (!isInPhyReg(vr)) {
            assert(isInFrame(vr));
            auto pr = findPhyRegBeyond(uses);
            auto slt = getFrmSlt(vr);
            unlink(pr);
            genBefore(Operation::LOAD, {FS(slt), PR(pr)});
            taint_regs.insert(pr);
            link(vr, pr);
            op->rewrite(vr, pr);
        } else {
            auto pr = getPhyReg(vr);
            op->rewrite(vr, pr);
        }
    }
    for (auto vr : defs) {
        auto pr = isInPhyReg(vr) ? getPhyReg(vr) : findPhyRegBeyond(defs);
        unlink(vr);
        unlink(pr);
        link(vr, pr);
        op->rewrite(vr, pr);
        taint_regs.insert(pr);
    }
}
Reg RAGreedy::findPhyRegBeyond(const std::set<int> &vrs) {
    std::vector<Reg> left;
    for (auto pr : AllRegs()) {
        if (!isOccupied(pr))
            return pr;
        bool ok = true;
        for (auto vr : refVirRegs(pr)) {
            if (vrs.count(vr) > 0) {
                ok = false;
                break;
            }
        }
        if (ok)
            left.push_back(pr);
    }
    return choseEvictor(left);
}
Reg RAGreedy::choseEvictor(const std::vector<Reg> &regs) {
    static std::default_random_engine rng;

    (void) vr2pr;
    assert(!regs.empty());
    std::uniform_int_distribution<int> dist(0, regs.size() - 1);
    return regs[dist(rng)];
}
Reg RAGreedy::getPhyReg(int vr) {
    return *refPhyRegs(vr).begin();
}
void RAGreedy::removeAndStepIn() {
    // FIXME: BUG
    auto op = cur_op;
    cur_op = op->next();
    cur_blk->removeOp(op);
    delete (op);
}
void RAGreedy::handleMov(Operation *op) {
    auto dst = op->oprs[0];
    auto src = op->oprs[1];
    if (src.tag != Operand::VIR_REG || dst.tag != Operand::VIR_REG) {
        allocPhyRegsFor(op);
        cur_op = cur_op->next();
    } else {
        auto r1 = dst.val.vir_reg, r2 = src.val.vir_reg;
        if (!isInPhyReg(r2)) {
            assert(isInFrame(r2));
            auto p2 = findPhyRegBeyond({r2});
            auto slt = getFrmSlt(r2);
            unlink(p2);
            genBefore(Operation::LOAD, {FS(slt), PR(p2)});
            taint_regs.insert(p2);
            link(r2, p2);

            unlink(r1), link(r1, p2);
        } else {
            auto p2 = getPhyReg(r2);
            if (!(isInPhyReg(r1) && getPhyReg(r1) == p2)) {
                unlink(r1);
                link(r1, p2);
            }
        }
        removeAndStepIn();
    }
}
void RAGreedy::handleBeginParam(Operation *op) {
    for (auto pr : CallerSavedRegs())
        unlink(pr);
    removeAndStepIn();
}
void RAGreedy::handleSetParam(Operation *op) {
    auto &oprs = op->oprs;
    auto dst = oprs[1];
    auto src = oprs[0];
    assert(dst.tag == Operand::INTEGER);
    auto ai = I2R(dst.val.integer + R2I(Reg::A0));
    unlink(ai);
    moveOpr2PhyReg(src, ai);
    removeAndStepIn();
}
void RAGreedy::handleGetParam(Operation *op) {
    auto src = op->oprs[0];
    auto dst = op->oprs[1];
    assert(src.tag == Operand::INTEGER);
    auto ai = I2R(src.val.integer + R2I(Reg::A0));
    assert(dst.tag == Operand::VIR_REG);
    auto vr = dst.val.vir_reg;
    link(vr, ai);
    removeAndStepIn();
}
void RAGreedy::handleGetRet(Operation *op) {
    auto dst = op->oprs[0];
    assert(dst.tag == Operand::VIR_REG);
    auto vr = dst.val.vir_reg;
    unlink(vr);
    link(vr, Reg::A0);
    removeAndStepIn();
}
void RAGreedy::handleSetRet(Operation *op) {
    auto src = op->oprs[0];
    unlink(Reg::A0);
    moveOpr2PhyReg(src, Reg::A0);
    genBefore(Operation::JUMP, {BB(cur_exit)});
    cur_blk->jump(cur_exit);
    removeAndStepIn();
}
void RAGreedy::handleOthers(Operation *op) {
    allocPhyRegsFor(op);
    cur_op = cur_op->next();
}
void RAGreedy::moveOpr2PhyReg(const Operand &src, Reg ai) {
    taint_regs.insert(ai);
    if (src.tag == Operand::VIR_REG) {
        auto vr = src.val.vir_reg;
        if (isInPhyReg(vr)) {
            genBefore(Operation::MOV, {PR(ai), PR(getPhyReg(vr))});
        } else {
            assert(isInFrame(vr));
            auto fs = getFrmSlt(vr);
            genBefore(Operation::LOAD, {FS(fs), PR(ai)});
        }
    } else if (src.tag == Operand::GLB_VAR) {
        auto gv = src.val.glb_var;
        if (gv->width > 0) {
            genBefore(Operation::LOAD_ADDR, {GV(gv), PR(ai)});
        } else {
            genBefore(Operation::LOAD, {GV(gv), PR(ai)});
        }
    } else if (src.tag == Operand::FRM_SLT) {
        auto fs = src.val.frm_slt;
        genBefore(Operation::LOAD_ADDR, {FS(fs), PR(ai)});
    } else {
        assert(false);
    }
}
void RAGreedy::saveRegsInBlockEnd() {
    if (cur_blk->ops.empty())
        return;
    auto last_op = cur_blk->ops.back();
    auto opt = last_op->opt;
    if (opt == Operation::JUMP && cur_blk->jump_out == cur_exit)
        return;
    std::function<void(Operation::Opt opt, std::array<Operand, 3> oprs)>
            gen_lmd;
    if (opt == Operation::JUMP || opt == Operation::BRANCH) {
        gen_lmd = [&](Operation::Opt opt, std::array<Operand, 3> oprs) -> void {
            last_op->addBefore(new Operation(opt, oprs));
        };
    } else {
        gen_lmd = [&](Operation::Opt opt, std::array<Operand, 3> oprs) -> void {
            cur_blk->addOp(opt, oprs);
        };
    }
    for (auto vr : cur_blk->live_out) {
        if (!isInFrame(vr)) {
            assert(isInPhyReg(vr));
            gen_lmd(Operation::STORE, {PR(getPhyReg(vr)), FS(getFrmSlt(vr))});
        }
    }
}
void RAGreedy::saveCalleeSavedRegs() {
    auto entry = cur_fun->blocks[0];
    auto exit = cur_exit;
    std::function<void(Operation::Opt opt, std::array<Operand, 3> oprs)>
            gen_lmd;
    if (entry->ops.empty()) {
        gen_lmd = [&](Operation::Opt opt, std::array<Operand, 3> oprs) -> void {
            entry->addOp(opt, oprs);
        };
    } else {
        gen_lmd = [&](Operation::Opt opt, std::array<Operand, 3> oprs) -> void {
            entry->ops.front()->addBefore(new Operation(opt, oprs));
        };
    }
    for (auto pr : taint_regs) {
        if (isCalleeSaved(pr)) {
            auto slt = cur_fun->extendFrame(1);
            gen_lmd(Operation::STORE, {PR(pr), FS(slt)});
            exit->addOp(Operation::LOAD, {FS(slt), PR(pr)});
        }
    }
}

} // namespace tgr
} // namespace mc