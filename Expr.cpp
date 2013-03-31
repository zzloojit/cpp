#include "Expr.hpp"
#include "Token.h"

#include <cassert>

static struct
{
  int tok_op;
  int prec;
}op_prec[] = 
  {
    {-1              , -1},
    {tok::COMMA      ,  0},
    //    {tok::EQ         ,  1},
    {tok::QUES       ,  2},
    {tok::COLON      ,  2},
    {tok::OROR       ,  3},
    {tok::ANDAND     ,  4},
    {tok::OR         ,  5},
    {tok::XOR        ,  6},
    {tok::AND        ,  7},
    {tok::EQEQ       ,  8},
    {tok::NEQ        ,  8},
    {tok::LT         ,  9},
    {tok::LE         ,  9},
    {tok::GT         ,  9},
    {tok::GE         ,  9},
    {tok::LEFT       , 10},
    {tok::RIGHT      , 10},
    {tok::PLUS       , 11},
    {tok::SUB        , 11},
    {tok::SLASH      , 12},
    {tok::MUL        , 12},
    {tok::MOD        , 12},
#define UPLUS  tok::totals + 1
#define UMINUS tok::totals + 2    
    {UPLUS      , 13},  //UMINUS
    {UMINUS     , 13},  //UPLUS
    {tok::BANG       , 13},
    {tok::COMPL      , 13},
  };

Expr::Expr(Preprocess& p):pp(p)
{

}

inline void Expr::reduce(std::stack<int>& operators, std::stack<int>& operands)
{
  int op = operators.top(); operators.pop();
  int r = operands.top(); operands.pop();  

  if (op == tok::BANG)
    {
      operands.push(!r);
      return ;
    }
  else if (op == tok::COMPL)
    {
      operands.push(~r);
      return ;
    }
  else if (op == UMINUS)
    {
      operands.push(-r);
      return;
    }
  else if (op == UPLUS)
    {
      operands.push(+r);
      return;
    }
  else if (op == tok::QUES)
    {
      operands.push(r);
      operators.push(op);
      return;
    }

  if (operands.size() == 0)
    {
      operands.push(r);
      return;
    }

  int l = operands.top(); operands.pop();
  
  int value = 0;

  switch(op)
    {
    case tok::MOD:
      if (r == 0)
        fatal_error("division by zeor in #if ");
      value = l % r;
      break;

    case tok::MUL :
      value = l * r;
      break;

    case tok::SLASH :
      if (r == 0)
        fatal_error("division by zeor in #if ");
      value = l / r;
      break;

    case tok::SUB :
      value = l - r;
      break;

    case tok::PLUS :
      value = l + r;
      break;
      
    case tok::RIGHT :
      value = l >> r;
      break;
      
    case tok::LEFT :
      value = l << r;
      break;
      
    case tok::GE :
      value = l >= r;
      break;

    case tok::GT :
      value = l > r;
      break;

    case tok::LE :
      value = l <= r;
      break;
      
    case tok::LT :
      value = l < r;
      break;
  
    case tok::NEQ :
      value = l != r;
      break;

    case tok::EQEQ :
      value = l == r;
      break;
      
    case tok::AND :
      value = l & r;
      break;
      
    case tok::XOR :
      value = l ^ r;
      break;
      
    case tok::OR :
      value = l | r;
      break;

    case tok::ANDAND :
      value = l && r;
      break;
      
    case tok::OROR :
      value = l || r;
      break;

    case tok::COMMA :
      value = r;
      break;

    case tok::COLON:
      {
        int b = operands.top(); operands.pop();
        int t = operators.top(); operators.pop();
        assert (t == tok::QUES);
        if (b != 0)
          value = l;
        else
          value = r;
      }
      break;

    default:
      assert(0);
      break;
    }
  operands.push(value);
}

bool Expr::defined()
{
  int t;
  Token tok;
  bool seen = false;
  bool has_def = false;
  int lparen = 0;

  while (t = pp.lex.next(tok, pp.fbuffer))
    {
      switch(t)
        {
        case tok::SPACE :
          continue;

        case tok::LPAREN :
          if (seen == true)
            fatal_error("defined need only one identifier");
          lparen++;
          break;

        case tok::RPAREN :
          if (seen == false)
            fatal_error("defined need identifier");
          lparen--;
          break;

        case tok::IDENT :
          if (seen == true)
            fatal_error("defined need only one identifier");
          
          seen = true;
          if (pp.macros_map.find(tok.str) != pp.macros_map.end())
            has_def = true;
          else 
            has_def = false;
          break;
        }
      
      if (seen && (lparen == 0))
        return has_def;
    }
}

int Expr::parse_expr()
{
  bool want_value = true;
  int prev = -1;

  static std::map <int, int> op_map;
  if (op_map.empty())
    {
      int i = 0;
      for (; i < sizeof(op_prec) / sizeof (op_prec[0]); i++)
        op_map[op_prec[i].tok_op] = op_prec[i].prec;
    }
  int value = 0;
  std::stack<int>  operands;
  std::stack<int>  operators;

  operators.push(-1);
  Token tok;
  int t;
  
  while (t = pp.next(tok))
    {
      if (t == tok::SPACE)
        continue;
      
      if (t == tok::NEWLINE || t == tok::TEOF)
        {
          pp.push_tok(tok);
          break;
        }
      
      if (t == tok::LPAREN)
        {
          if (want_value == false)
            fatal_error("#if need macro operator");

          int l = parse_expr();
          operands.push(l);
          want_value = false;
        }
      else if (t == tok::RPAREN)
        {
          if (want_value == true)
            fatal_error("#if need operand");

          while (operands.size() != 1)
            reduce(operators, operands);

          return operands.top();

        }
      else if (t == tok::NUMBER)
        {
          if (want_value == false)
             fatal_error("#if need macro operator");
          int l = strtol(tok.str.c_str(), NULL, 0);

          operands.push(l);
          want_value = false;
        }
      else if (t == tok::COLON)
        {
          if (want_value == true)
            fatal_error("#if macro need operand");
          while (operators.top() != tok::QUES)
            {
              if (operators.size() == 1)
                {
                  fatal_error("token : unmatch ? ");
                }
              reduce(operators, operands);
            }
          want_value = true;
          operators.push(t);
        }
      else if (t == tok::IDENT)
        {
          if (want_value == false)
            fatal_error("if macro need operantor");

          want_value = false;

          if (tok.str == "defined")
            {
              bool has_def = defined();
              operands.push(has_def ? 1 : 0);
            }
          else
            {
              fatal_error("is not valid preprocess expression");
            }
        }
      else
        {
          if(op_map.find(t) == op_map.end())
            {
              fatal_error("token: " + tok.str + "is not valid"
                          " in preprocess expression");
            }

          if (want_value == true)
            {
              if (t == tok::PLUS || t == tok::SUB)
                {
                  operators.push(t);
                  continue;
                }
            }

          int prec = op_map[t];
          
          if (prec <= op_map[operators.top()])
            {
              reduce(operators, operands);
            }
          operators.push(t);
          want_value = true;
        }
    }

  if (want_value == true)
    fatal_error("#if need operand");
  
  while(operators.size() != 1)
    {
      if (operators.top() == tok::QUES)
        fatal_error("in #if macro token '?' need ':'");
      reduce(operators, operands);
    }

  return operands.top();
}

#undef UMINUS
#undef UPLUS
