//
// Created by herlight on 2019/12/19.
//

#include "eyr_optimizer.hh"

namespace mc {
namespace eyr {

bool EyrOptimizer::ConstFolder::optimize(Module *mod) {
    flow_changed = false;
    for (auto fun: mod->global_funcs)
        runOnFunction(fun);
    return flow_changed;
}
void EyrOptimizer::ConstFolder::runOnFunction(Function *fun) {
    in_consts.clear(), out_consts.clear();
    work_list.clear();

    work_list.insert(fun->entry);
    rewrite = false;
    while (!work_list.empty()) {
        auto it = work_list.begin();
        auto blk = *it;
        work_list.erase(it);
        runOnBlock(blk);
    }

    rewrite = true;
    for (auto blk: fun->blocks) {
        runOnBlock(blk);
    }
    fun->arrangeBlock();
}
void EyrOptimizer::ConstFolder::runOnBlock(BasicBlock *blk) {
    if (rewrite) {
        cur_consts = in_consts[blk];
        cur_ins = blk->insts.front();
        while (cur_ins) {
            runOnInstruction(cur_ins);
        }
    } else {
        cur_consts.clear();
        for (auto inb: blk->inBlocks()) {
            for (auto vc: out_consts[inb]) {
                auto v = vc.first;
                auto c = vc.second;
                if (cur_consts.count(v) == 0) {
                    cur_consts[v] = c;
                } else if (cur_consts[v] != c) {
                    cur_consts[v] = {};
                }
            }
        }
        in_consts[blk] = cur_consts;
        for (auto ins: blk->insts) {
            runOnInstruction(ins);
        }
        if (out_consts[blk] != cur_consts) {
            out_consts[blk] = cur_consts;
            for (auto outb: blk->outBlocks())
                work_list.insert(outb);
        }
    }
}
void EyrOptimizer::ConstFolder::runOnInstruction(Instruction *ins) {
#define MATCH(type) else if (auto i = dynamic_cast<type *>(ins)) { handle(i); }
#define MATCHES MATCH(BinaryInst)MATCH(UnaryInst)MATCH(CallInst)MATCH(MoveInst)\
MATCH(StoreInst)MATCH(LoadInst)MATCH(BranchInst)MATCH(JumpInst)MATCH(ReturnInst)

    if (false) {

    } MATCHES
    else {
        assert(false);
    }
    if (rewrite && cur_ins)
        cur_ins = cur_ins->next();

#undef MATCHES
#undef MATCH
}
void EyrOptimizer::ConstFolder::handle(BinaryInst *ins) {
#define MATCH(match, op) when(BinaryInst::BinOp::match, c = lc op rc;)
#define MATCHES  MATCH(EQ, ==)MATCH(NE, !=)MATCH(LT, <)MATCH(GT, >)MATCH(OR, ||)\
MATCH(AND, &&)MATCH(ADD, +)MATCH(SUB, -)MATCH(MUL, *)MATCH(DIV, /)MATCH(REM, %)

    if (isConst(ins->lhs) && isConst(ins->rhs)) {
        auto lc = getConst(ins->lhs), rc = getConst(ins->rhs);
        int c = 0;
        switch (ins->opt) {
            MATCHES
            default:
                assert(false);
        }
        cur_consts[ins->dst] = c;
        if (rewrite) {
            cur_ins = ins->addAfter(new MoveInst(ins->dst, Operand(c)));
            ins->remove();
        }
    } else {
        if (rewrite) {
            if (isConst(ins->lhs))
                ins->lhs = Operand(getConst(ins->lhs));
            if (isConst(ins->rhs))
                ins->rhs = Operand(getConst(ins->rhs));
        }
        cur_consts[ins->dst] = {};
    }

#undef MATCHES
#undef MATCH
}
void EyrOptimizer::ConstFolder::handle(UnaryInst *ins) {
    if (isConst(ins->opr)) {
        auto oc = getConst(ins->opr);
        int c = 0;
        switch (ins->opt) {
            when(UnaryInst::UnOp::NEG, c = -oc;)
            when(UnaryInst::UnOp::NOT, c = !oc;)
            default:
                assert(false);
        }
        cur_consts[ins->dst] = c;
        if (rewrite) {
            cur_ins = ins->addAfter(new MoveInst(ins->dst, Operand(c)));
            ins->remove();
        }
    } else {
        cur_consts[ins->dst] = {};
    }
}
void EyrOptimizer::ConstFolder::handle(CallInst *ins) {
    if (rewrite) {
        int len = ins->args.size();
        for (int i = 0; i < len; ++i) {
            if (isConst(ins->args[i]))
                ins->args[i] = Operand(getConst(ins->args[i]));
        }
    }
    for (auto vc: cur_consts) {
        if (vc.first->is_global())
            cur_consts[vc.first] = {};
    }
}
void EyrOptimizer::ConstFolder::handle(MoveInst *ins) {
    if (isConst(ins->src)) {
        if (rewrite) {
            ins->src = Operand(getConst(ins->src));
        }
        cur_consts[ins->dst] = getConst(ins->src);
    } else {
        cur_consts[ins->dst] = {};
    }
}
void EyrOptimizer::ConstFolder::handle(StoreInst *ins) {
    if (rewrite) {
        if (isConst(ins->src))
            ins->src = Operand(getConst(ins->src));
        if (isConst(ins->idx))
            ins->idx = Operand(getConst(ins->idx));
    }
}
void EyrOptimizer::ConstFolder::handle(LoadInst *ins) {
    if (rewrite) {
        if (isConst(ins->idx))
            ins->idx = Operand(getConst(ins->idx));
    }
    cur_consts[ins->dst] = {};
}
void EyrOptimizer::ConstFolder::handle(BranchInst *ins) {
#define MATCH(match, op) when(BranchInst::LgcOp::match, c = lc op rc;)
#define MATCHES  MATCH(EQ, ==)MATCH(NE, !=)MATCH(LT, <)MATCH(GT, >)MATCH(OR, ||)MATCH(AND, &&)

    if (rewrite) {
        if (isConst(ins->lhs))
            ins->lhs = Operand(getConst(ins->lhs));
        if (isConst(ins->rhs))
            ins->rhs = Operand(getConst(ins->rhs));
        if (isConst(ins->lhs) && isConst(ins->rhs)) {
            auto lc = getConst(ins->lhs), rc = getConst(ins->rhs);
            int c = 0;
            switch (ins->opt) {
                MATCHES
                default:
                    assert(false);
            }
            if (c) {
                ins->block->unfall();
                cur_ins = ins->addAfter(new JumpInst(ins->block->jump_out));
                ins->remove();
                flow_changed = true;
            } else {
                ins->block->unjump();
                cur_ins = ins->next();
                ins->remove();
                flow_changed = true;
            }
        }
    }

#undef MATCHES
#undef MATCH
}
void EyrOptimizer::ConstFolder::handle(JumpInst *ins) {
    (void) cur_consts;
}
void EyrOptimizer::ConstFolder::handle(ReturnInst *ins) {
    if (rewrite) {
        if (isConst(ins->opr))
            ins->opr = Operand(getConst(ins->opr));
    }
}
bool EyrOptimizer::ConstFolder::isConst(Operand opr) {
    if (opr.imm)
        return true;
    auto it = cur_consts.find(opr.var);
    if (it == cur_consts.end())
        return false;
    return it->second.is_const;
}
int EyrOptimizer::ConstFolder::getConst(Operand opr) {
    return opr.imm ? opr.val : cur_consts[opr.var].val;
}
void EyrOptimizer::optimize(Module *mod) {
    while (const_folder.optimize(mod));
    while (simplifier.optimize(mod));
}

bool EyrOptimizer::Simplifier::optimize(Module *mod) {
    changed = false;
    for (auto fun: mod->global_funcs)
        runOnFunction(fun);
    return changed;
}
void EyrOptimizer::Simplifier::runOnFunction(Function *fun) {
    live_gen.clear(), live_kill.clear();
    out_lives.clear(), in_lives.clear();
    for (auto blk: fun->blocks) {
        VarSet gen, kill;
        for (auto ins: blk->insts) {
            for (auto use: ins->uses()) {
                if (kill.count(use) == 0)
                    gen.insert(use);
            }
            for (auto def: ins->defs()) {
                kill.insert(def);
            }
        }
        live_gen[blk] = gen, live_kill[blk] = kill;
    }
    work_list.clear();
    for (auto blk: fun->blocks) {
        work_list.insert(blk);
    }
    rewrite = false;
    while (!work_list.empty()) {
        auto it = work_list.begin();
        auto blk = *it;
        work_list.erase(it);
        runOnBlock(blk);
    }
    rewrite = true;
    for (auto blk: fun->blocks) {
        runOnBlock(blk);
    }
}
void EyrOptimizer::Simplifier::runOnBlock(BasicBlock *blk) {
    if (rewrite) {
        cur_lives = out_lives[blk];
        cur_ins = blk->insts.back();
        while (cur_ins)
            runOnInstruction(cur_ins);
    } else {
        cur_lives.clear();
        for (auto outb: blk->outBlocks()) {
            for (auto var: in_lives[outb])
                cur_lives.insert(var);
        }
        out_lives[blk] = cur_lives;
        for (auto var: live_kill[blk])
            cur_lives.erase(var);
        for (auto var: live_gen[blk])
            cur_lives.insert(var);
        if (in_lives[blk] != cur_lives) {
            std::cerr << std::endl;
            in_lives[blk] = cur_lives;
            for (auto inb: blk->inBlocks())
                work_list.insert(inb);
        }
    }
}
void EyrOptimizer::Simplifier::runOnInstruction(Instruction *ins) {
#define MATCH(type) else if (auto i = dynamic_cast<type *>(ins)) { handle(i); }
#define MATCHES MATCH(CallInst)MATCH(AssignInst)\
MATCH(StoreInst)MATCH(BranchInst)MATCH(JumpInst)MATCH(ReturnInst)

    cur_ins = ins->prev();

    if (false) {
    } MATCHES
    else {
        assert(false);
    }

#undef MATCHES
#undef MATCH
}
void EyrOptimizer::Simplifier::handle(CallInst *ins) {
    updateLives(ins);
}
void EyrOptimizer::Simplifier::handle(StoreInst *ins) {
    updateLives(ins);
}
void EyrOptimizer::Simplifier::handle(BranchInst *ins) {
    updateLives(ins);
}
void EyrOptimizer::Simplifier::handle(JumpInst *ins) {
    updateLives(ins);
}
void EyrOptimizer::Simplifier::handle(ReturnInst *ins) {
    updateLives(ins);
}
void EyrOptimizer::Simplifier::updateLives(Instruction *ins) {
    for (auto def: ins->defs())
        cur_lives.erase(def);
    for (auto use: ins->uses())
        cur_lives.insert(use);
}
void EyrOptimizer::Simplifier::handle(AssignInst *ins) {
    if (ins->dst->is_local() && cur_lives.count(ins->dst) == 0) {
        ins->remove();
        changed = true;
    } else {
        updateLives(ins);
    }
}

}
}