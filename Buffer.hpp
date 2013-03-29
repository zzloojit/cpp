#ifndef BUFFER_HPP
#define BUFFER_HPP
#include <string>

class Buffer
{
public:
  Buffer();
  std::string name;
  const char* ptr;
  const char* ptr_end;
  int line_num;
  int column_num;
};
#endif
