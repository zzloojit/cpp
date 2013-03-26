#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include "$"
namespace tok
{
  enum kind
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
#define DEF(x, y, z) #y
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
    char* ptr;
    unsigned long len;
    std::string str()
    {
      std::string s = tok_desc[kind] + " " + tok_str[kind];
      return s;
    }
  };
};
#endif
