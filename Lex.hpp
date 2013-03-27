#ifndef LEX_H
#define LEX_H

#include <string>
#include <iostream>
#include <cstdlib>
#include <stack>
#include <set>

#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "Token.h"

using std::string;
using std::cerr;
using std::endl;

using tok::Token;

class FileBuffer
{
public:
  FileBuffer(string fn);
  ~FileBuffer();
  char* ptr;
  char* ptr_end;
  string filename;
private:
  int fd;
  char* buffer;
  size_t buflen;
};

class Lex
{
public:
  Lex(string f);
  int next(Token& tok, FileBuffer* fbuffer);
  ~Lex();
private:
  void inc_direct(void);
  boost::regex pattern;
};

class Preprocess
{
public:
  Preprocess(string f);
  int next(Token& tok);

  void add_inc_path(string path);
  void add_sys_inc_path(string path);

private:
  FileBuffer* fbuffer;
  std::stack<FileBuffer*> buffers;
  std::vector<string> sys_inc_path;
  std::vector<string> inc_path;
  std::set<string>  inc_set;
  bool findfile(string& fn,bool u);
  bool has_inc(string& file);
  Lex lex;
  void direct_inc(void);
};
#endif
