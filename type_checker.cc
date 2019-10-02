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
#define CHECK() do{if (!okay) return;}while(0)
#define FAIL() do {okay = false; return;}while(0)

V(IfStmt) {
    ast->getCond()->accept(*this);
    CHECK();
    if (ast->getThen()) {
        ast->getThen()->accept(*this);
        CHECK();
    }
    if (ast->getAlter()) {
        ast->getAlter()->accept(*this);
        CHECK();
    }
}

V(WhileStmt) {
    ast->getTest()->accept(*this);
    CHECK();
    if (ast->getLoop()) {
        ast->getLoop()->accept(*this);
        CHECK();
    }
}

V(FuncDefn) {
    def(ast->getName(), ast->getType());
    CHECK();
    cur_ret_type = ast->getRetType();
    enter_scope();
    for (auto &f : ast->getParams()) {
        def(f->id, f->type);
        CHECK();
    }
    for (auto s : ast->getBody()) {
        s->accept(*this);
        CHECK();
    }
    leave_scope();
}

V(ReturnStmt) {
    ast->getValue()->accept(*this);
    CHECK();
    if (ast->getValue()->getType() != cur_ret_type)
        FAIL();
}

V(BlockStmt) {
    enter_scope();
    for (auto s : ast->getStmts()) {
        s->accept(*this);
        CHECK();
    }
    leave_scope();
}

V(DeclStmt) {
    def(ast->getVar().id, ast->getVar().type);
}

V(BinaryExpr) {
    ast->getLhs()->accept(*this);
    CHECK();
    ast->getRhs()->accept(*this);
    CHECK();
    switch (ast->getOpt()) {
        case BinaryExpr::BinOp::COMMA: {
            ast->setType(ast->getRhs()->getType());
            break;
        }
        case BinaryExpr::BinOp::ASSIGN: {
            if (!ast->getLhs()->isLhsValue())
                FAIL();
            auto lhs_type = ast->getLhs()->getType();
            auto rhs_type = ast->getRhs()->getType();
            if (!isPrim(lhs_type) || !isPrim(rhs_type))
                FAIL();
            if (*lhs_type != *rhs_type)
                FAIL();
            ast->setType(lhs_type);
            break;
        }
        default: {
            auto lhs_type = ast->getLhs()->getType();
            auto rhs_type = ast->getRhs()->getType();
            if (!isPrim(lhs_type) || !isPrim(rhs_type))
                FAIL();
            if (*lhs_type != *rhs_type)
                FAIL();
            ast->setType(lhs_type);
            break;
        }
    }
}

V(UnaryExpr) {
    ast->getOpr()->accept(*this);
    CHECK();
    auto opr_type = ast->getOpr()->getType();
    if (!isPrim(opr_type))
        FAIL();
    ast->setType(opr_type);
}

V(RefExpr) {
    auto var_type = lookup(ast->getName());
    if (!var_type)
        FAIL();
    auto ref_type = var_type->indexType(ast->getIndex().size());
    if (!ref_type)
        FAIL();
    for (auto e : ast->getIndex()) {
        e->accept(*this);
        CHECK();
        if (!dynamic_cast<IntType *>(e->getType()))
            FAIL();
    }
    ast->setType(ref_type);
}

V(CallExpr) {
    if (auto func_type = dynamic_cast<FuncType *>(lookup(ast->getName()))) {
        size_t len = func_type->getParams().size();
        if (len != ast->getArgs().size())
            FAIL();
        for (size_t i = 0; i < len; ++i) {
            auto para = func_type->getParams().at(i);
            ast->getArgs().at(i)->accept(*this);
            CHECK();
            auto arg = ast->getArgs().at(i)->getType();
            if (!para->compatible(*arg))
                FAIL();
        }
        ast->setType(func_type->getRet());
    } else {
        FAIL();
    }
}

V(NumExpr) {
    ast->setType(new IntType);
}
bool TypeChecker::check(const std::vector<AST *> &tops) {
    okay = true;
    environ.clear();
    cur_ret_type = nullptr;

    enter_scope();
    for (auto ast: tops) {
        ast->accept(*this);
        if (!okay)
            return false;
    }
    leave_scope();

    return okay;
}
Type *TypeChecker::lookup(const std::string &id) {
    for (auto &e : environ) {
        auto it = e.find(id);
        if (it != e.end())
            return it->second;
    }
    return nullptr;
}
void TypeChecker::def(const std::string &id, Type *type) {
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
