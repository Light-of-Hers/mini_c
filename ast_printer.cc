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
    os << "FuncDefn: " << *ast->get_ret_type();
    os << " " << ast->get_name() << "(";
    for (auto p : ast->get_params())
        os << *p << ", ";
    os << ")\n";
    for (auto s : ast->get_body())
        depths.push_back(d + 1), s->accept(*this);
    depths.pop_back();
}

V(IfStmt) {
    int d = depths.back();
    indent(os, d);
    os << "IfStmt: \n";
    depths.push_back(d + 1), ast->get_cond()->accept(*this);
    if (ast->get_then())
        depths.push_back(d + 1), ast->get_then()->accept(*this);
    if (ast->get_alter())
        depths.push_back(d + 1), ast->get_alter()->accept(*this);
    depths.pop_back();
}

V(WhileStmt) {
    int d = depths.back();
    indent(os, d);
    os << "WhileStmt: \n";
    depths.push_back(d + 1), ast->get_test()->accept(*this);
    if (ast->get_loop())
        depths.push_back(d + 1), ast->get_loop()->accept(*this);
    depths.pop_back();
}

V(ReturnStmt) {
    int d = depths.back();
    indent(os, d);
    os << "ReturnStmt: \n";
    depths.push_back(d + 1), ast->get_value()->accept(*this);
    depths.pop_back();
}

V(BlockStmt) {
    int d = depths.back();
    indent(os, d);
    os << "BlockStmt: \n";
    for (auto s : ast->get_stmts())
        depths.push_back(d + 1), s->accept(*this);
    depths.pop_back();
}

V(DeclStmt) {
    int d = depths.back();
    indent(os, d);
    os << "DeclStmt: " << *ast->get_var().type << " " << ast->get_var().id << std::endl;
    depths.pop_back();
}

V(BinaryExpr) {
    int d = depths.back();
    indent(os, d);
    os << "BinaryExpr: " << BinaryExpr::OpStr[static_cast<int>(ast->get_opt())]
       << std::endl;
    depths.push_back(d + 1), ast->get_lhs()->accept(*this);
    depths.push_back(d + 1), ast->get_rhs()->accept(*this);
    depths.pop_back();
}

V(UnaryExpr) {
    int d = depths.back();
    indent(os, d);
    os << "UnaryExpr: " << UnaryExpr::OpStr[static_cast<int>(ast->get_opt())]
       << std::endl;
    depths.push_back(d + 1), ast->get_opr()->accept(*this);
    depths.pop_back();
}

V(RefExpr) {
    int d = depths.back();
    indent(os, d);
    os << "RefExpr: " << ast->get_name() << std::endl;
    for (auto e : ast->get_index())
        depths.push_back(d + 1), e->accept(*this);
    depths.pop_back();
}

V(CallExpr) {
    int d = depths.back();
    indent(os, d);
    os << "CallExpr: " << ast->get_name() << std::endl;
    for (auto arg : ast->get_args())
        depths.push_back(d + 1), arg->accept(*this);
    depths.pop_back();
}

V(NumExpr) {
    int d = depths.back();
    indent(os, d);
    os << "NumExpr: " << ast->get_num() << std::endl;
    depths.pop_back();
}
void ASTPrinter::print(const std::vector<AST *> &tops) {
    for (auto ast: tops)
        print(ast);
}

#undef V

}; // namespace mc