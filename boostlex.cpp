#include <boost/regex.hpp>
#include <string>

using namespace boost;
using std::string;

// namespace tok
// {
//   enum
//     {
//       tok_start,
// #define DEF(x, y, z) x
// #include "tok.def"
// #undef DEF
//       total_kinds
//     };
// };

// const string cpatterns[] =
//   {
// #define DEF(x, y, z) z
//     #include "tok.def"
// #undef DEF
//   };
// 
#include "Token.h"

int main()
{
  string p;
  int i = 1;
  for (p = tok::patterns[0]; i < tok::totals; ++i)
    {
      p += string("|") + tok::patterns[i];
    }

  std::string preprocess_lex;
  //boost::regex pattern("([ ]+)|(define)|(undef)", boost::regex::perl);
  boost::regex pattern(p, boost::regex::perl);
  const char str[] = "undef";
  cmatch matchs;
  regex_constants::match_flag_type flags = boost::match_continuous;

  if (regex_search(str, str + sizeof(str), matchs , pattern, flags))
    {
      int i;
      for (i = 1; i < matchs.size(); ++i)
        {
          if (matchs[i].matched)
            std::cout << i << std::endl;
        }
    }
}
