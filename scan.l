%{
    #include "ast.hh"
    #include "type.hh"
    using namespace std;
    using namespace mc;

    #include <vector>
    #include PARSER_H
%}

%option nounput noyywrap

alpha   [a-zA-Z]
digit   [0-9]

%x COMMENT

%%

"/*"            { BEGIN(COMMENT); }
<COMMENT>"*/"   { BEGIN(INITIAL); }
<COMMENT>\n     { yylineno++; }
<COMMENT>.      {}

"//".*[\n\r]  { yylineno++; }

"if"        { return IF; }
"else"      { return ELSE; }
"while"     { return WHILE; }
"return"    { return RETURN; }
"int"       { return INT; }
"=="        { return EQ; }
"!="        { return NE; }
"||"        { return OR; }
"&&"        { return AND; }

[+\-*/%\[\](){},;=<>!]  { return yytext[0]; }

{digit}+    { yylval.i = atoi(yytext); return NUM; }

({alpha}|_)({alpha}|{digit}|_)* { yylval.s = new string(yytext); return ID; }

[ \r\t] {}
\n { yylineno++; }

%%