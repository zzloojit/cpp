#ifndef EXPR_HPP
#define EXPR_HPP
#include "Lex.hpp"

class Expr
{
public:
  // if err is "" parse is ok, else error
  Expr(Preprocess& pp);
  int parse_expr(void);
private:
  Preprocess& pp;
  void reduce(std::stack<int>& operators, std::stack<int>& operands);
};
#endif
