//
// Created by herlight on 2019/10/1.
//

#include <iostream>
#include "type_checker.hh"

namespace mc {

static inline bool isPrim(Type *t) {
    return dynamic_cast<PrimitiveType *>(t) != nullptr;
}

#define V(x) void TypeChecker::visit(x *ast)
#define CHECK() do{if (!check_ok) return;}while(0)
#define FAIL() do {check_ok = false; std::cerr << __LINE__ << std::endl; return;}while(0)

V(IfStmt) {
    ast->get_cond()->accept(*this);
    CHECK();
    if (ast->get_then()) {
        ast->get_then()->accept(*this);
        CHECK();
    }
    if (ast->get_alter()) {
        ast->get_alter()->accept(*this);
        CHECK();
    }
}

V(WhileStmt) {
    ast->get_test()->accept(*this);
    CHECK();
    if (ast->get_loop()) {
        ast->get_loop()->accept(*this);
        CHECK();
    }
}

V(FuncDefn) {
    bind(ast->get_name(), ast->get_type());
    CHECK();
    cur_ret_type = ast->get_ret_type();
    enter_scope();
    for (auto &f : ast->get_params()) {
        bind(f->id, f->type);
        CHECK();
    }
    for (auto s : ast->get_body()) {
        s->accept(*this);
        CHECK();
    }
    leave_scope();
}

V(ReturnStmt) {
    ast->get_value()->accept(*this);
    CHECK();
    if (*ast->get_value()->get_type() != *cur_ret_type)
        FAIL();
}

V(BlockStmt) {
    enter_scope();
    for (auto s : ast->get_stmts()) {
        s->accept(*this);
        CHECK();
    }
    leave_scope();
}

V(DeclStmt) {
    bind(ast->get_var().id, ast->get_var().type);
}

V(BinaryExpr) {
    ast->get_lhs()->accept(*this);
    CHECK();
    ast->get_rhs()->accept(*this);
    CHECK();
    switch (ast->get_opt()) {
        case BinaryExpr::BinOp::COMMA: {
            ast->set_type(ast->get_rhs()->get_type());
            break;
        }
        case BinaryExpr::BinOp::ASSIGN: {
            if (!ast->get_lhs()->isLhsValue())
                FAIL();
            auto lhs_type = ast->get_lhs()->get_type();
            auto rhs_type = ast->get_rhs()->get_type();
            if (!isPrim(lhs_type) || !isPrim(rhs_type))
                FAIL();
            if (*lhs_type != *rhs_type)
                FAIL();
            ast->set_type(lhs_type);
            break;
        }
        default: {
            auto lhs_type = ast->get_lhs()->get_type();
            auto rhs_type = ast->get_rhs()->get_type();
            if (!isPrim(lhs_type) || !isPrim(rhs_type))
                FAIL();
            if (*lhs_type != *rhs_type)
                FAIL();
            ast->set_type(lhs_type);
            break;
        }
    }
}

V(UnaryExpr) {
    ast->get_opr()->accept(*this);
    CHECK();
    auto opr_type = ast->get_opr()->get_type();
    if (!isPrim(opr_type))
        FAIL();
    ast->set_type(opr_type);
}

V(RefExpr) {
    auto var_type = lookup(ast->get_name());
    if (!var_type)
        FAIL();
    auto ref_type = var_type->indexType(ast->get_index().size());
    if (!ref_type)
        FAIL();
    for (auto e : ast->get_index()) {
        e->accept(*this);
        CHECK();
        if (!dynamic_cast<IntType *>(e->get_type()))
            FAIL();
    }
    ast->set_type(ref_type);
    ast->set_var_type(var_type);
}

V(CallExpr) {
    if (auto func_type = dynamic_cast<FuncType *>(lookup(ast->get_name()))) {
        size_t len = func_type->getParams().size();
        if (len != ast->get_args().size())
            FAIL();
        for (size_t i = 0; i < len; ++i) {
            auto para = func_type->getParams().at(i);
            ast->get_args().at(i)->accept(*this);
            CHECK();
            auto arg = ast->get_args().at(i)->get_type();
            if (!para->compatible(*arg))
                FAIL();
        }
        ast->set_type(func_type->getRet());
    } else {
        FAIL();
    }
}

V(NumExpr) {
    ast->set_type(new IntType);
}
bool TypeChecker::check(const std::vector<AST *> &tops) {
    check_ok = true;
    environ.clear();
    cur_ret_type = nullptr;

    enter_scope();
    for (auto ast: tops) {
        ast->accept(*this);
        if (!check_ok)
            return false;
    }
    leave_scope();

    return check_ok;
}
Type *TypeChecker::lookup(const std::string &id) {
    for (auto &e : environ) {
        auto it = e.find(id);
        if (it != e.end())
            return it->second;
    }
    return nullptr;
}
void TypeChecker::bind(const std::string &id, Type *type) {
    auto &e = *environ.begin();
    auto it = e.find(id);
    if (it == e.end()) {
        e[id] = type;
    } else {
        auto old = it->second;
        if (!dynamic_cast<FuncType *>(old))
            FAIL();
        if (*old != *type)
            FAIL();
    }
}

#undef V

};
