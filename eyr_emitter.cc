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
    auto func = new Function(module, ast->get_name(), ast->get_params().size());
    module->addFunction(func);
    enter_scope();
    for (size_t i = 0; i < func->params.size(); ++i)
        def(ast->get_params()[i]->id, func->params[i]);
    cur_func = func;
    cur_blk = func->entry;
    for (auto s : ast->get_body())
        s->accept(*this);
    leave_scope();
    cur_func->arrangeBlock();
    cur_func = nullptr;
    cur_blk = nullptr;
}
void EyrEmitter::visit(IfStmt *ast) {
    auto then_blk = cur_func->allocBlock();
    auto else_blk = cur_func->allocBlock();
    auto merge_blk = cur_func->allocBlock();

    ast->get_cond()->accept(*this);
    auto pred = cur_opr;
    cur_blk->addInst(new BranchInst(cur_blk, then_blk,
                                    BranchInst::LgcOp::NE, pred, Operand(0)));

    cur_blk->fall(else_blk);
    cur_blk->jump(then_blk);

    cur_blk = else_blk;
    if (ast->get_alter())
        ast->get_alter()->accept(*this);
    cur_blk->addInst(new JumpInst(cur_blk, merge_blk));
    cur_blk->jump(merge_blk);

    cur_blk = then_blk;
    if (ast->get_then())
        ast->get_then()->accept(*this);
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
    ast->get_test()->accept(*this);
    auto pred = cur_opr;
    cur_blk->addInst(new BranchInst(cur_blk, loop_blk,
                                    BranchInst::LgcOp::NE, pred, Operand(0)));
    cur_blk->jump(loop_blk);
    cur_blk->fall(break_blk);

    cur_blk = loop_blk;
    if (ast->get_loop())
        ast->get_loop()->accept(*this);
    cur_blk->fall(test_blk);

    cur_blk = break_blk;
}
void EyrEmitter::visit(ReturnStmt *ast) {
    ast->get_value()->accept(*this);
    cur_blk->addInst(new ReturnInst(cur_blk, cur_opr));
    cur_blk = cur_func->allocBlock();
}
void EyrEmitter::visit(BlockStmt *ast) {
    enter_scope();
    for (auto s :ast->get_stmts())
        s->accept(*this);
    leave_scope();
}
void EyrEmitter::visit(DeclStmt *ast) {
    auto type = ast->get_var().type;
    if (dynamic_cast<FuncType *>(type))
        return;
    assert(type->byteSize() >= 0);
    int width = dynamic_cast<ArrayType *>(type) ? type->byteSize() : -1;
    bool constant = dynamic_cast<VariantArrayType *>(type) != nullptr;
    if (cur_func) {
        def(ast->get_var().id, cur_func->allocLocalVar(false, width, constant));
    } else {
        def(ast->get_var().id, module->allocGlobalVar(width, constant));
    }
}

void EyrEmitter::visit(BinaryExpr *ast) {
    switch (ast->get_opt()) {
        case BinaryExpr::BinOp::COMMA: {
            ast->get_lhs()->accept(*this);
            ast->get_rhs()->accept(*this);
            break;
        }
        case BinaryExpr::BinOp::ASSIGN: {
            ast->get_rhs()->accept(*this);
            will_store = true;
            ast->get_lhs()->accept(*this);
            break;
        }
        default: {
            ast->get_lhs()->accept(*this);
            auto lhs = cur_opr;
            ast->get_rhs()->accept(*this);
            auto rhs = cur_opr;
            auto tmp = allocTemp();
            cur_blk->addInst(
                    new BinaryInst(cur_blk, tmp,
                                   static_cast<BinaryInst::BinOp>(ast->get_opt()), lhs, rhs));
            cur_opr = Operand(tmp);
            break;
        }
    }
}
void EyrEmitter::visit(UnaryExpr *ast) {
    ast->get_opr()->accept(*this);
    auto tmp_var = allocTemp();
    cur_blk->addInst(
            new UnaryInst(cur_blk, tmp_var, static_cast<UnaryInst::UnOp>(ast->get_opt()), cur_opr));
    cur_opr = Operand(tmp_var);
}
void EyrEmitter::visit(RefExpr *ast) {
    auto src = cur_opr;
    auto store = will_store;
    will_store = false;
    if (dynamic_cast<VariantArrayType *>(ast->get_var_type()) && !ast->get_index().empty()) {
        auto arr = dynamic_cast<VariantArrayType *>(ast->get_var_type());
        auto base = static_cast<Type *>(arr);
        auto idx_var = allocTemp();
        auto tmp_off = allocTemp();
        cur_blk->addInst(new MoveInst(cur_blk, idx_var, Operand(0)));
        for (auto e : ast->get_index()) {
            base = dynamic_cast<VariantArrayType *>(base)->getBase();
            assert(base != nullptr);
            e->accept(*this);
            auto ret = cur_opr;
//            auto bs = allocTemp();
//            cur_blk->addInst(new MoveInst(cur_blk, bs, Operand(base->byteSize())));
            cur_blk->addInst(new BinaryInst(cur_blk, tmp_off, BinaryInst::BinOp::MUL, ret,
                                            Operand(base->byteSize())));
            cur_blk->addInst(
                    new BinaryInst(cur_blk, idx_var, BinaryInst::BinOp::ADD, Operand(idx_var),
                                   Operand(tmp_off)));
        }
        if (store) {
            auto dst = lookup(ast->get_name());
            cur_blk->addInst(new StoreInst(cur_blk, dst, Operand(idx_var), src));
            cur_opr = Operand(dst);
        } else {
            auto mem = lookup(ast->get_name());
            cur_blk->addInst(new LoadInst(cur_blk, tmp_off, mem, Operand(idx_var)));
            cur_opr = Operand(tmp_off);
        }
    } else {
        if (store) {
            auto dst = lookup(ast->get_name());
            cur_blk->addInst(new MoveInst(cur_blk, dst, src));
            cur_opr = Operand(dst);
        } else {
            cur_opr = Operand(lookup(ast->get_name()));
        }
    }
}
void EyrEmitter::visit(CallExpr *ast) {
    std::vector<Operand> args;
    for (auto arg: ast->get_args()) {
        arg->accept(*this);
        if (cur_opr.imm || cur_opr.var->isTemp() || cur_opr.var->isAddr()) {
            args.push_back(cur_opr);
        } else {
            auto tmp_var = allocTemp();
            cur_blk->addInst(new MoveInst(cur_blk, tmp_var, cur_opr));
            args.emplace_back(tmp_var);
        }
    }
    auto ret_var = allocTemp(); // always temp
    cur_blk->addInst(new CallInst(cur_blk, ret_var, ast->get_name(), args));
    cur_opr = Operand(ret_var);
}
void EyrEmitter::visit(NumExpr *ast) {
//    auto tmp = allocTemp();
//    cur_blk->addInst(new MoveInst(cur_blk, tmp, Operand(ast->get_num())));
//    cur_opr = Operand(tmp);
    cur_opr = Operand(ast->get_num());
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
Variable *EyrEmitter::lookup(const std::string &id) {
    for (auto &e:environ) {
        auto it = e.find(id);
        if (it != e.end())
            return it->second;
    }
    return nullptr;
}
void EyrEmitter::def(const std::string &id, Variable *var) {
    environ.begin()->insert({id, var});
}

};
};