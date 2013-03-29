#ifndef TOKEN_H
#define TOKEN_H

#include <string>
namespace tok
{
  enum Kind
  {
    TEOF,
#define DEF(x, y, z) x
#include "tok.def"
#undef DEF
    totals
  };
  
  const std::string tok_desc[] = 
    {
      "TEOF",
#define DEF(x, y, z) #x
#include "tok.def"
#undef DEF
    };
  
  const std::string tok_str[] =
    {
      "TEOF",
#define DEF(x, y, z) y
#include "tok.def"
#undef  DEF
    };
  
  const std::string patterns[] = 
    {
#define DEF(x, y, z) z
#include "tok.def"
#undef DEF
    };

  struct Token
  {
    int kind;
    int line_num;
    int column_num;
    std::string str;
    std::string filename;
  Token(Kind k, std::string& s):kind(k), str(s)
    {
      line_num = 0;
      column_num = 0;
      filename = "<built-in>";
    }
  
    Token()
    {}

    Token(const Token& tok)
    {
      kind = tok.kind;
      str = tok.str;
      line_num = tok.line_num;
      column_num = tok.column_num;
      filename = tok.filename;
    }

    Token& operator = (Token& t)
    {
      this->kind = t.kind;
      this->str = t.str;
      return *this;
    }
      
    std::string dump()
    {
      return tok_desc[kind] + str + "\n";
    }
  };
  
  enum
  {
    macobj_type,
    macfunc_type,
  };
};
#endif
