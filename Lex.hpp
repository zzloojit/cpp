#ifndef LEX_H
#define LEX_H

#include <string>
#include <iostream>
#include <cstdlib>
#include <boost/regex.hpp>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "Token.h"

using std::string;
using std::cerr;
using std::endl;

using tok::Token;
class Lex
{
public:
  Lex(string f);
  int next(Token& tok);
  ~Lex();
private:
  int fd;
  string filename;
  char* buffer;
  size_t buflen;
  char* ptr;
  char* ptr_end;
  boost::regex pattern;
};
#endif
