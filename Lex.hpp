#ifndef LEX_H
#define LEX_H

#include <string>
#include <iostream>
#include <cstdlib>
#include <stack>
#include <set>
#include <map>
#include <deque>

#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "Token.h"
#include "Buffer.hpp"

using std::string;
using std::cerr;
using std::endl;

using tok::Token;

class FileBuffer : public Buffer
{
public:
  FileBuffer(string fn);
  ~FileBuffer();
  // char* ptr;
  // char* ptr_end;
  //string filename;
  // int line_num;
  // int column_num;
private:
  int fd;
  char* buffer;
  size_t buflen;
};

class Lex
{
public:
  Lex(string f);
  int next(Token& tok, Buffer* fbuffer);
  ~Lex();
private:
  void inc_direct(void);
  boost::regex pattern;
};

struct macro_type
{
  std::vector<Token> params;
  std::vector<Token> expand;
  int kind;
  macro_type()
    {
      kind = tok::macobj_type;
    }
};

class Preprocess
{
public:
  Preprocess(string f);
  ~Preprocess();
  int next(Token& tok);

  void add_inc_path(string path);
  void add_sys_inc_path(string path);

private:
  Buffer* fbuffer;
  std::stack<Buffer*> buffers;
  std::vector<string> sys_inc_path;
  std::vector<string> inc_path;
  std::set<string>  inc_set;
  bool findfile(string& fn,bool u);
  bool has_inc(string& file);
  Lex lex;
  void except(int k);
  void except(int k, int l);
  void direct_inc(void);
  void direct_def(void);
  int  parse_macro_arg(string& s);
  int  parse_arguments(std::vector<string>& vec);
  void macro_expand(const Token& macro);
  void macro_expand_func(const Token& macro);
  std::deque<Token> macros_buffer;
  std::map<string, macro_type> macros_map;
  std::set<string> avoid_recursion;
};
#endif
