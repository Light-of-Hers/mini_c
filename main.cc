#include "ast.hh"
#include "config.hh"
#include "eyr_emitter.hh"
#include "live_analyzer.hh"
#include "ra_greedy.hh"
#include "riscv_printer.hh"
#include "tgr_emitter.hh"
#include "type_checker.hh"
#include <cstdio>
#include <fstream>
#include <iostream>

extern int yyparse();

extern FILE *yyin;
extern int yylineno;

extern void yyerror(const char *s) {
    std::cerr << "error:" << yylineno << ": " << s << std::endl;
    exit(1);
}

int main(int argc, char **argv) {
    using namespace mc;
    using namespace std;
    using namespace eyr;

    freopen("./debug.log", "w", stderr);
    std::ofstream eyr_out("./output_eeyore"), ftgr_out("./output_ftigger"),
            tgr_out("./output_tigger"), riscv_out("./output_riscv");

    if (argc > 1)
        yyin = fopen(argv[1], "r");
    yyparse();

#ifdef MC_DEBUG
//    ASTPrinter printer(cout);
//    printer.print(*global_prog);
#endif
    TypeChecker checker;
    auto ok = checker.check(*global_prog);
    (void) ok;
#ifdef MC_DEBUG
//    std::cout << std::boolalpha;
//    std::cout << std::endl << "Type Check OK: " << ok << std::endl;
//    if (!ok)
//        return 1;
#endif
    EyrEmitter emitter;
    auto mod = emitter.emit(*global_prog);
    eyr_out << *mod << std::endl;

    tgr::TgrEmitter te;
    auto tmod = te.emit(mod);
    ftgr_out << *tmod << std::endl;
    tgr::LiveAnalyzer::analyze(tmod);
    tgr::RAGreedy greedy;
    greedy.allocate(tmod);
    tgr_out << *tmod << std::endl;

    tgr::RiscvPrinter rp(riscv_out);
    rp.printModule(tmod);

    tgr::RiscvPrinter rp1(std::cout);
    rp1.printModule(tmod);
}