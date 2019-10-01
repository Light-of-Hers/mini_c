#include "ast.hh"
#include "type_checker.hh"
#include <cstdio>
#include <iostream>

extern int yyparse();

extern FILE *yyin;

extern void yyerror(const char *s) {
    std::cout << "unknown token: " << s << std::endl;
}

int main(int argc, char **argv) {
    using namespace mc;
    using namespace std;

    if (argc > 1)
        yyin = fopen(argv[1], "r");

    yyparse();

    ASTPrinter printer(cout);
    for (auto top : *prog)
        printer.print(top);

    TypeChecker checker;
    auto ok = checker.check(*prog);
    std::cout << std::boolalpha;
    std::cout << ok << std::endl;
}