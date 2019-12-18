//
// Created by herlight on 2019/12/5.
//

#include "riscv_printer.hh"
#include "ast.hh"
#include "tgr.hh"
#include <cassert>

namespace mc {
namespace tgr {

void RiscvPrinter::printModule(Module *mod) {
    for (auto var: mod->vars)
        printVariable(var);
    for (auto fun: mod->funcs)
        printFunction(fun);
}
void RiscvPrinter::printVariable(Variable *var) {
    std::string v = std::string("v") + std::to_string(var->id);
    if (var->width > 0) {
        os << "\t.comm\t" << v << ", " << (var->width * 4) << ", 4" << std::endl;
    } else {
        os << "\t.global\t" << v << std::endl;
        os << "\t.section\t.sdata" << std::endl;
        os << "\t.align\t2" << std::endl;
        os << "\t.type\t" << v << ", @object" << std::endl;
        os << "\t.size\t" << v << ", 4" << std::endl;
        os << v << ":" << std::endl;
        os << "\t.word\t0" << std::endl;
    }
    os << std::endl;
}
void RiscvPrinter::printFunction(Function *fun) {
    cur_stk = (fun->frame_size / 4 + 1) * 16;
    const auto &fn = fun->name;
    os << "\t.text" << std::endl;
    os << "\t.align\t2" << std::endl;
    os << "\t.global\t" << fn << std::endl;
    os << "\t.type\t" << fn << ", @function" << std::endl;
    os << fn << ":" << std::endl;
    os << "\taddi\tsp, sp, " << -cur_stk << std::endl;
    os << "\tsw\tra, " << (cur_stk - 4) << "(sp)" << std::endl;
    for (auto blk: fun->blocks)
        printBasicBlock(blk);
    os << "\t.size\t" << fn << ", .-" << fn << std::endl;
    os << std::endl;
}
void RiscvPrinter::printBasicBlock(BasicBlock *blk) {
    os << ".l" << blk->label << ":" << std::endl;
    for (auto op: blk->ops)
        printOperation(op);
}
void RiscvPrinter::printOperation(Operation *op) {
    if (op->isBinOp()) {
        printBinOp(op);
    } else if (op->isUnOp()) {
        printUnOp(op);
    } else if (op->isBrOp()) {
        printBrOp(op);
    } else {
        switch (op->opt) {
            when(Operation::MOV, printMov(op);)
            when(Operation::IDX_LD, printIdxLd(op);)
            when(Operation::IDX_ST, printIdxSt(op);)
            when(Operation::JUMP, printJump(op);)
            when(Operation::CALL, printCall(op);)
            when(Operation::STORE, printStore(op);)
            when(Operation::LOAD, printLoad(op);)
            when(Operation::LOAD_ADDR, printLoadAddr(op);)
            when(Operation::RET, printRet(op);)
            default:
                assert(false);
        }
    }
}
void RiscvPrinter::printBinOp(Operation *op) {
    auto oprs = op->oprs;
    auto r1 = oprs[0], r2 = oprs[1], r3 = oprs[2];
    (void) r2;
    switch (op->opt) {
        when(Operation::BIN_EQ, {
            gen("xor", oprs);
            gen("seqz", {r1, r1});
        })
        when(Operation::BIN_NE, {
            gen("xor", oprs);
            gen("snez", {r1, r1});
        })
        when(Operation::BIN_LT, {
            if (r3.tag == Operand::INTEGER) {
                gen("slti", oprs);
            } else {
                gen("slt", oprs);
            }
        })
        when(Operation::BIN_GT, {
            gen("sgt", oprs);
        })
        when(Operation::BIN_OR, {
            gen("or", oprs);
            gen("snez", {r1, r1});
        })
        when(Operation::BIN_AND, {
            gen("mul", oprs);
            gen("snez", {r1, r1});
        })
        when(Operation::BIN_ADD, {
            if (r3.tag == Operand::INTEGER) {
                gen("addi", oprs);
            } else {
                gen("add", oprs);
            }
        })
        when(Operation::BIN_SUB, {
            gen("sub", oprs);
        })
        when(Operation::BIN_MUL, {
            gen("mul", oprs);
        })
        when(Operation::BIN_DIV, {
            gen("div", oprs);
        })
        when(Operation::BIN_REM, {
            gen("rem", oprs);
        })
        default:
            assert(false);
    }
}
void RiscvPrinter::printUnOp(Operation *op) {
    auto oprs = op->oprs;
    switch (op->opt) {
        when(Operation::UN_NEG, {
            gen("neg", oprs);
        })
        when(Operation::UN_NOT, {
            gen("seqz", oprs);
        })
        default:
            assert(false);
    }
}
void RiscvPrinter::printMov(Operation *op) {
    auto oprs = op->oprs;
    if (oprs[1].tag == Operand::INTEGER) {
        gen("li", oprs);
    } else {
        gen("mv", oprs);
    }
}
void RiscvPrinter::printIdxLd(Operation *op) {
    auto oprs = op->oprs;
    os << "\tlw\t" << oprs[0] << ", " << oprs[2] << "(" << oprs[1] << ")" << std::endl;
}
void RiscvPrinter::printIdxSt(Operation *op) {
    auto oprs = op->oprs;
    os << "\tsw\t" << oprs[2] << ", " << oprs[1] << "(" << oprs[0] << ")" << std::endl;
}
void RiscvPrinter::printBrOp(Operation *op) {
    auto oprs = op->oprs;
    switch (op->opt) {
        when(Operation::BR_EQ, {
            gen("beq", oprs);
        })
        when(Operation::BR_NE, {
            gen("bne", oprs);
        })
        when(Operation::BR_LT, {
            gen("blt", oprs);
        })
        when(Operation::BR_GT, {
            gen("bgt", oprs);
        })
        default:
            assert(false);
    }
}
void RiscvPrinter::printJump(Operation *op) {
    gen("j", {op->oprs[0]});
}
void RiscvPrinter::printCall(Operation *op) {
    gen("call", {op->oprs[0]});
}
void RiscvPrinter::printStore(Operation *op) {
    auto reg = op->oprs[0], slt = op->oprs[1];
    assert(slt.tag == Operand::FRM_SLT);
    int off = slt.val.frm_slt * 4;
    os << "\tsw\t" << reg << ", " << off << "(sp)" << std::endl;
}
void RiscvPrinter::printLoad(Operation *op) {
    auto fr = op->oprs[0], reg = op->oprs[1];
    if (fr.tag == Operand::GLB_VAR) {
        os << "\tlui\t" << reg << ", " << "%hi(" << fr << ")" << std::endl;
        os << "\tlw\t" << reg << ", " << "%lo(" << fr << ")(" << reg << ")" << std::endl;
    } else {
        assert(fr.tag == Operand::FRM_SLT);
        int off = fr.val.frm_slt * 4;
        os << "\tlw\t" << reg << ", " << off << "(sp)" << std::endl;
    }
}
void RiscvPrinter::printLoadAddr(Operation *op) {
    auto fr = op->oprs[0], reg = op->oprs[1];
    if (fr.tag == Operand::GLB_VAR) {
        os << "\tlui\t" << reg << ", " << "%hi(" << fr << ")" << std::endl;
        os << "\taddi\t" << reg << ", " << reg << ", " << "%lo(" << fr << ")" << std::endl;
    } else {
        assert(fr.tag == Operand::FRM_SLT);
        int off = fr.val.frm_slt * 4;
        os << "\taddi\t" << reg << ", sp, " << off << std::endl;
    }
}
void RiscvPrinter::printRet(Operation *op) {
    os << "\tlw\tra, " << (cur_stk - 4) << "(sp)" << std::endl;
    os << "\taddi\tsp, sp, " << cur_stk << std::endl;
    os << "\tjr\tra" << std::endl;
}
void RiscvPrinter::gen(const char *opt, std::array<Operand, 3> oprs) {
    if (oprs[0].tag == Operand::NONE)
        return;
    os << "\t" << opt << "\t", printOperand(oprs[0]);
    for (int i = 1; i < 3; ++i) {
        if (oprs[i].tag != Operand::NONE)
            os << ", ", printOperand(oprs[i]);
    }
    os << std::endl;
}
void RiscvPrinter::printOperand(Operand opr) {
    if (opr.tag == Operand::BSC_BLK)
        os << ".";
    os << opr;
}

}
}