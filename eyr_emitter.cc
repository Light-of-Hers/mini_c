//
// Created by herlight on 2019/10/7.
//

#include "eyr_emitter.hh"
#include <cassert>

namespace mc {
namespace eyr {

static inline bool isLogicOp(BinaryExpr::BinOp op) {
    return op >= BinaryExpr::BinOp::EQ && op <= BinaryExpr::BinOp::AND;
}

void EyrEmitter::visit(FuncDefn *ast) {
    auto func = new Function(module, ast->getName(), ast->getParams().size());
    module->addFunction(func);
    enter_scope();
    for (size_t i = 0; i < func->params.size(); ++i)
        def(ast->getParams()[i]->id, func->params[i]);
    cur_func = func;
    cur_blk = func->entry;
    for (auto s : ast->getBody())
        s->accept(*this);
    leave_scope();
    cur_func = nullptr;
    cur_blk = nullptr;
}
void EyrEmitter::visit(IfStmt *ast) {
    auto then_blk = cur_func->allocBlock();
    auto else_blk = cur_func->allocBlock();
    auto merge_blk = cur_func->allocBlock();
    cur_blk->fall(else_blk);
    cur_blk->jump(then_blk);

    auto cond = ast->getCond();
    auto expr = dynamic_cast<BinaryExpr *>(cond);
    if (expr && isLogicOp(expr->getOpt())) {
        expr->getLhs()->accept(*this);
        auto lhs = cur_opr;
        expr->getRhs()->accept(*this);
        auto rhs = cur_opr;
        cur_blk->addInst(new BranchInst(cur_blk, then_blk,
                                        static_cast<BranchInst::LgcOp>(expr->getOpt()), lhs, rhs));
    } else {
        cond->accept(*this);
        auto res = cur_opr;
        cur_blk->addInst(new BranchInst(cur_blk, then_blk,
                                        BranchInst::LgcOp::NE, res, Operand(0)));
    }

    cur_blk = else_blk;
    if (ast->getAlter())
        ast->getAlter()->accept(*this);
    cur_blk->addInst(new JumpInst(cur_blk, merge_blk));
    cur_blk->jump(merge_blk);

    cur_blk = then_blk;
    if (ast->getThen())
        ast->getThen()->accept(*this);
    cur_blk->fall(merge_blk);

    cur_blk = merge_blk;
}
void EyrEmitter::visit(WhileStmt *ast) {
    auto loop_blk = cur_func->allocBlock();
    auto test_blk = cur_func->allocBlock();
    auto break_blk = cur_func->allocBlock();

    cur_blk->addInst(new JumpInst(cur_blk, test_blk));
    cur_blk->jump(test_blk);

    cur_blk = test_blk;
    auto test = ast->getTest();
    auto expr = dynamic_cast<BinaryExpr *>(test);
    if (expr && isLogicOp(expr->getOpt())) {
        expr->getLhs()->accept(*this);
        auto lhs = cur_opr;
        expr->getRhs()->accept(*this);
        auto rhs = cur_opr;
        cur_blk->addInst(new BranchInst(cur_blk, loop_blk,
                                        static_cast<BranchInst::LgcOp>(expr->getOpt()), lhs, rhs));
    } else {
        test->accept(*this);
        auto out = cur_opr;
        cur_blk->addInst(new BranchInst(cur_blk, loop_blk,
                                        BranchInst::LgcOp::NE, out, Operand(0)));
    }
    cur_blk->jump(loop_blk);
    cur_blk->fall(break_blk);

    cur_blk = loop_blk;
    if (ast->getLoop())
        ast->getLoop()->accept(*this);
    cur_blk->fall(test_blk);

    cur_blk = break_blk;
}
void EyrEmitter::visit(ReturnStmt *ast) {
    ast->getValue()->accept(*this);
    cur_blk->addInst(new ReturnInst(cur_blk, cur_opr));
    cur_blk->fall(cur_func->exit);
    cur_blk = cur_func->allocBlock();
}
void EyrEmitter::visit(BlockStmt *ast) {
    enter_scope();
    for (auto s :ast->getStmts())
        s->accept(*this);
    leave_scope();
}
void EyrEmitter::visit(DeclStmt *ast) {
    auto type = ast->getVar().type;
    if (dynamic_cast<FuncType *>(type))
        return;
    assert(type->byteSize() >= 0);
    int width = dynamic_cast<ArrayType *>(type) ? type->byteSize() : -1;
    bool constant = dynamic_cast<VariantArrayType *>(type) != nullptr;
    if (cur_func) {
        def(ast->getVar().id, cur_func->allocLocalVar(cur_blk, false, width, constant));
    } else {
        def(ast->getVar().id, module->allocGlobalVar(width, constant));
    }
}
void EyrEmitter::visit(BinaryExpr *ast) {
    switch (ast->getOpt()) {
        case BinaryExpr::BinOp::COMMA: {
            ast->getLhs()->accept(*this);
            ast->getRhs()->accept(*this);
            break;
        }
        case BinaryExpr::BinOp::ASSIGN: {
            ast->getRhs()->accept(*this);
            will_store = true;
            ast->getLhs()->accept(*this);
            break;
        }
        default: {
            ast->getLhs()->accept(*this);
            auto lhs = cur_opr;
            ast->getRhs()->accept(*this);
            auto rhs = cur_opr;
            auto tmp = allocTemp();
            cur_blk->addInst(
                    new BinaryInst(cur_blk, tmp,
                                   static_cast<BinaryInst::BinOp>(ast->getOpt()), lhs, rhs));
            cur_opr = Operand(tmp);
            break;
        }
    }
}
void EyrEmitter::visit(UnaryExpr *ast) {
    ast->getOpr()->accept(*this);
    auto tmp_var = allocTemp();
    cur_blk->addInst(
            new UnaryInst(cur_blk, tmp_var, static_cast<UnaryInst::UnOp>(ast->getOpt()), cur_opr));
    cur_opr = Operand(tmp_var);
}
void EyrEmitter::visit(RefExpr *ast) {
    auto src = cur_opr;
    auto store = will_store;
    will_store = false;
    if (dynamic_cast<VariantArrayType *>(ast->getVarType()) && !ast->getIndex().empty()) {
        auto arr = dynamic_cast<VariantArrayType *>(ast->getVarType());
        auto base = static_cast<Type *>(arr);
        auto idx_var = allocTemp();
        auto tmp_off = allocTemp();
        cur_blk->addInst(new MoveInst(cur_blk, idx_var, Operand(0)));
        for (auto e : ast->getIndex()) {
            base = dynamic_cast<VariantArrayType *>(base)->getBase();
            assert(base != nullptr);
            e->accept(*this);
            auto ret = cur_opr;
            cur_blk->addInst(new BinaryInst(cur_blk, tmp_off, BinaryInst::BinOp::MUL, ret,
                                            Operand(base->byteSize())));
            cur_blk->addInst(
                    new BinaryInst(cur_blk, idx_var, BinaryInst::BinOp::ADD, Operand(idx_var),
                                   Operand(tmp_off)));
        }
        if (store) {
            auto dst = lookup(ast->getName());
            cur_blk->addInst(new StoreInst(cur_blk, dst, Operand(idx_var), src));
            cur_opr = Operand(dst);
        } else {
            auto mem = lookup(ast->getName());
            cur_blk->addInst(new LoadInst(cur_blk, tmp_off, mem, Operand(idx_var)));
            cur_opr = Operand(tmp_off);
        }
    } else {
        if (store) {
            auto dst = lookup(ast->getName());
            cur_blk->addInst(new MoveInst(cur_blk, dst, src));
            cur_opr = Operand(dst);
        } else {
            cur_opr = Operand(lookup(ast->getName()));
        }
    }
}
void EyrEmitter::visit(CallExpr *ast) {
    std::vector<Operand> args;
    for (auto arg: ast->getArgs()) {
        arg->accept(*this);
        if (cur_opr.imm || cur_opr.var->temp || cur_opr.var->constant) {
            args.push_back(cur_opr);
        } else {
            auto tmp_var = allocTemp();
            cur_blk->addInst(new MoveInst(cur_blk, tmp_var, cur_opr));
            args.emplace_back(tmp_var);
        }
    }
    auto ret_var = allocTemp();
    cur_blk->addInst(new CallInst(cur_blk, ret_var, ast->getName(), args));
    cur_opr = Operand(ret_var);
}
void EyrEmitter::visit(NumExpr *ast) {
    cur_opr = Operand(ast->getNum());
}
Module *EyrEmitter::emit(const Program &prog) {
    module = new Module;
    environ.clear();
    will_store = false;
    cur_blk = nullptr;
    cur_func = nullptr;
    enter_scope();
    for (auto ast: prog)
        ast->accept(*this);
    leave_scope();
    return module;
}
DeclInst *EyrEmitter::lookup(const std::string &id) {
    for (auto &e:environ) {
        auto it = e.find(id);
        if (it != e.end())
            return it->second;
    }
    return nullptr;
}
void EyrEmitter::def(const std::string &id, DeclInst *var) {
    environ.begin()->insert({id, var});
}

};
};