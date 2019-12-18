//
// Created by herlight on 2019/12/5.
//

#ifndef __MC_RISCV_PRINTER_HH__
#define __MC_RISCV_PRINTER_HH__

#include "tgr.hh"
#include <iostream>

namespace mc {
namespace tgr {

class RiscvPrinter {
public:
    RiscvPrinter(std::ostream &os) : os(os) {}
    void printModule(Module *mod);

private:
    std::ostream &os;
    int cur_stk{0};

    void printVariable(Variable *var);
    void printFunction(Function *fun);
    void printBasicBlock(BasicBlock *blk);
    void printOperation(Operation *op);

    void printBinOp(Operation *op);
    void printUnOp(Operation *op);
    void printMov(Operation *op);
    void printIdxLd(Operation *op);
    void printIdxSt(Operation *op);
    void printBrOp(Operation *op);
    void printJump(Operation *op);
    void printCall(Operation *op);
    void printStore(Operation *op);
    void printLoad(Operation *op);
    void printLoadAddr(Operation *op);
    void printRet(Operation *op);

    void printOperand(Operand opr);
    void gen(const char *opt, std::array<Operand, 3> oprs);
};

}
}

#endif //__MC_RISCV_PRINTER_HH__
