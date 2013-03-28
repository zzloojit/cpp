#ifndef BUFFER_HPP
#define BUFFER_HPP
#include <string>

struct Buffer
{
  std::string name;
  const char* ptr;
  const char* ptr_end;
  int line_num;
  int column_num;
};
#endif
