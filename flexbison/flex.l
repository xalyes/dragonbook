%{

#include "common.h"

#include "bison.tab.h"

int installID();
int installNum();

std::map<size_t, std::pair<std::string, std::optional<SemanticValue>>> symbols;

%}

delim [ \t\n]
ws {delim}+
letter [A-Za-z]
digit [0-9]
id {letter}({letter}|{digit})*
number {digit}+

%%

{ws} { /* no actions */ }
class { return (CLASS); }
record { return (RECORD); }
int { return (INT); }
float { return (FLOAT); }
"[" { return (L_BRACKET); }
"]" { return (R_BRACKET); }
"{" { return (L_BRACE); }
"}" { return (R_BRACE); }
";" { return (SEMICOLON); }
{id} {
yylval.idIdx = (int) installID(); return (ID);
}
{number} {yylval.size = (int) installNum(); return (NUMBER);}

%%

int installID()
{
    std::string id(yytext, yyleng);
    const auto it = std::find_if(symbols.begin(), symbols.end(), [&](const auto& item) { return item.second.first == id; });
    if (it == symbols.end())
    {
        symbols.insert({symbols.size() + 1, { id, std::nullopt }});
        return symbols.size();
    }
    else
    {
        return it->first;
    }
}

int installNum()
{ return (yytext[0]-'0'); }