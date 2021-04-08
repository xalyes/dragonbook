%{

#include "common.h"

int yylex(void);

void yyerror(const char *str) {}

 extern "C" {
        int yywrap( void ) {
            return 1;
        }
    }

#include <cstdio>
#include <iostream>
#include <tuple>
#include <vector>
#include <regex>

extern FILE* yyin;
SemanticValue* result;

%}

%union
{
    size_t size;
    size_t idIdx;
    SemanticValue* semVal;
}

%token<idIdx> ID
%token CLASS
%token RECORD
%token L_BRACE
%token R_BRACE
%token L_BRACKET
%token R_BRACKET
%token NUMBER
%token SEMICOLON
%token<size> INT
%token<size> FLOAT

%%

defs : type ID SEMICOLON defs
       {
            size_t sz = 0;
            const auto id = symbols.at($<idIdx>2).first;
            if (($<semVal>1->size() == 1) && ($<semVal>1->front().first.empty()))
            {
                sz = $<semVal>1->front().second;
                delete $<semVal>1;
            }

            std::vector<std::pair<std::string, size_t>>* stackVec;
            if ($<semVal>4)
            {
                stackVec = $<semVal>4;
            }
            else
            {
                stackVec = new std::vector<std::pair<std::string, size_t>>();
            }

            if (sz == 0)
            {
                for (auto& i : *$<semVal>1)
                {
                    i.first = id + "." + i.first;
                }
                stackVec->insert(stackVec->begin(), $<semVal>1->begin(), $<semVal>1->end());
            }
            else
            {
                stackVec->insert(stackVec->begin(), { id, sz });
            }

            $<semVal>$ = stackVec;
            if (result != stackVec)
            {
                result = stackVec;
            }
       }
     | { $<semVal>$ = nullptr; }
     | CLASS ID L_BRACE defs R_BRACE SEMICOLON defs
       {
           auto& pair = symbols.at($<idIdx>2);

           for (auto it = result->begin(); it != result->end(); )
           {
               if (it->first.find("<" + pair.first + ">") != std::string::npos)
               {
                   std::vector<std::pair<std::string, size_t>> newItems;
                   
                   for (auto& i : *$<semVal>4)
                   {
                       newItems.push_back({ std::regex_replace(it->first, std::regex("<" + pair.first + ">"), i.first), i.second});
                   }

                   it = result->insert(it, newItems.begin(), newItems.end());
                   std::advance(it, newItems.size());
                   it = result->erase(it);
               }
               else
               {
                   it++;
               }
           }

           $<semVal>$ = result;
       }
     ;

type : base comp 
     {
         auto& base = *$<semVal>1;
         if (base.size() == 1)
         {
             base[0].second *= $<size>2;
         }
         else
         {
             auto i = $<size>2;
             while (i > 1)
             {
                 i--;
                 base.insert(base.end(), base.begin(), base.end());
             }
         }
         $<semVal>$ = $<semVal>1;
     }
     | RECORD L_BRACE defs R_BRACE
     {
         $<semVal>$ = $<semVal>3;
     }
     ;

base : INT
        {
            $<semVal>$ = new std::vector<std::pair<std::string, size_t>>{ { "", 8 } };
        }
     | FLOAT
        {
            $<semVal>$ = new std::vector<std::pair<std::string, size_t>>{ { "", 4 } };
        }
     | ID
         {
             auto& pair = symbols.at($<idIdx>1);

             $<semVal>$ = new std::vector<std::pair<std::string, size_t>>{ { "<" + pair.first + ">", 0 } };
         }
     ;

comp : /* empty */ { $<size>$ = 1; }
     | L_BRACKET NUMBER R_BRACKET comp
     {
         $<size>$ = $<size>2 * $<size>4;
     }
     ;

%%

int main(int argc, char **argv)
{
    const auto ec = fopen_s(&yyin, argv[1], "r");
    if (ec)
    {
        return ec;
    }

    if (!yyparse())
    {
        std::cout << "Success parsing" << std::endl;
        size_t counter = 0;
        if (result)
        {
            for (auto& i : *result)
            {
                std::cout << i.first << " " << counter << " " << i.second << std::endl;
                counter += i.second;
            }
        }
    }

    fclose(yyin);

    if (result)
    {
        delete result;
    }

    return 0;
}

