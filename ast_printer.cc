#include "ast.hh"

namespace mc {

void ASTPrinter::print(mc::AST *ast) {
    depths.clear(), depths.push_back(0);
    ast->accept(*this);
}

static inline void indent(std::ostream &os, int width) {
    static const char *content = "|  ";
    while (width--)
        os << content;
}

#define V(x) void ASTPrinter::visit(x *ast)

V(FuncDefn) {
    int d = depths.back();
    indent(os, d);
    os << "FuncDefn: " << *ast->getRetType();
    os << " " << ast->getName() << "(";
    for (auto p : ast->getParams())
        os << *p << ", ";
    os << ")\n";
    for (auto s : ast->getBody())
        depths.push_back(d + 1), s->accept(*this);
    depths.pop_back();
}

V(IfStmt) {
    int d = depths.back();
    indent(os, d);
    os << "IfStmt: \n";
    depths.push_back(d + 1), ast->getCond()->accept(*this);
    if (ast->getThen())
        depths.push_back(d + 1), ast->getThen()->accept(*this);
    if (ast->getAlter())
        depths.push_back(d + 1), ast->getAlter()->accept(*this);
    depths.pop_back();
}

V(WhileStmt) {
    int d = depths.back();
    indent(os, d);
    os << "WhileStmt: \n";
    depths.push_back(d + 1), ast->getTest()->accept(*this);
    if (ast->getLoop())
        depths.push_back(d + 1), ast->getLoop()->accept(*this);
    depths.pop_back();
}

V(ReturnStmt) {
    int d = depths.back();
    indent(os, d);
    os << "ReturnStmt: \n";
    depths.push_back(d + 1), ast->getValue()->accept(*this);
    depths.pop_back();
}

V(BlockStmt) {
    int d = depths.back();
    indent(os, d);
    os << "BlockStmt: \n";
    for (auto s : ast->getStmts())
        depths.push_back(d + 1), s->accept(*this);
    depths.pop_back();
}

V(DeclStmt) {
    int d = depths.back();
    indent(os, d);
    os << "DeclStmt: " << *ast->getVar().type << " " << ast->getVar().id << std::endl;
    depths.pop_back();
}

V(BinaryExpr) {
    int d = depths.back();
    indent(os, d);
    os << "BinaryExpr: " << BinaryExpr::OpStr[static_cast<int>(ast->getOpt())]
       << std::endl;
    depths.push_back(d + 1), ast->getLhs()->accept(*this);
    depths.push_back(d + 1), ast->getRhs()->accept(*this);
    depths.pop_back();
}

V(UnaryExpr) {
    int d = depths.back();
    indent(os, d);
    os << "UnaryExpr: " << UnaryExpr::OpStr[static_cast<int>(ast->getOpt())]
       << std::endl;
    depths.push_back(d + 1), ast->getOpr()->accept(*this);
    depths.pop_back();
}

V(RefExpr) {
    int d = depths.back();
    indent(os, d);
    os << "RefExpr: " << ast->getName();
    for (auto i : ast->getIndex())
        os << '[' << i << ']';
    os << std::endl;
    depths.pop_back();
}

V(CallExpr) {
    int d = depths.back();
    indent(os, d);
    os << "CallExpr: " << ast->getName() << std::endl;
    for (auto arg : ast->getArgs())
        depths.push_back(d + 1), arg->accept(*this);
    depths.pop_back();
}

V(NumExpr) {
    int d = depths.back();
    indent(os, d);
    os << "NumExpr: " << ast->getNum() << std::endl;
    depths.pop_back();
}

#undef V

}; // namespace mc