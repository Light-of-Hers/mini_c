//
// Created by herlight on 2019/11/18.
//

#ifndef __MC_RA_GREEDY_HH__
#define __MC_RA_GREEDY_HH__

#include "tgr.hh"
#include <set>
#include <array>
#include <map>

namespace mc {
namespace tgr {

class RAGreedy {
public:
    void allocate(Module *mod);

private:

    std::map<int, int> vr2slt;
    std::set<Reg> taint_regs;
    std::set<int> in_frm_vr;
    std::map<int, std::set<Reg>> vr2pr;
    std::map<Reg, std::set<int>> pr2vr;

    Module *cur_mod;
    Function *cur_fun;
    BasicBlock *cur_exit;
    BasicBlock *cur_blk;
    Operation *cur_op;

    std::map<Operation *, std::set<int>> live_vrs;

    std::set<Reg> arg_prs;

    void runOnFunction(Function *fun);
    void runOnBlock(BasicBlock *blk);

    void runOnOperation(Operation *op);
    void handleMov(Operation *op);
    void handleBeginParam(Operation *op);
    void handleSetParam(Operation *op);
    void handleGetParam(Operation *op);
    void handleGetRet(Operation *op);
    void handleSetRet(Operation *op);
    void handleCall(Operation *op);

    void handleOthers(Operation *op);
    std::set<int> &refVirRegs(Reg pr);
    std::set<Reg> &refPhyRegs(int vr);
    Reg getPhyReg(int vr);
    int getFrmSlt(int vr);
    void enterFrame(int vr);
    void leaveFrame(int vr);
    bool isAlive(int r);
    bool isInFrame(int vr);
    bool isInPhyReg(int vr);

    bool isOccupied(Reg pr);
    void link(int r, Reg p);
    void unlink(int r);
    void unlink(Reg p);
    void spill(int r, Reg p);
    void genAfter(Operation::Opt opt, std::array<Operand, 3> oprs);
    void genBefore(Operation::Opt opt, std::array<Operand, 3> oprs);
    void removeAndStepIn();
    void moveOpr2PhyReg(const Operand &opr, Reg pr);
    void saveRegsInBlockEnd();
    void saveCalleeSavedRegs();
    void allocPhyRegsFor(Operation *op);
    Reg findPhyRegBeyond(const std::set<int> &vrs);

    Reg choseEvictor(const std::vector<Reg> &regs);

    void liveAnalyze();
};

}
}

#endif //__MC_RA_GREEDY_HH__
