#include "Buffer.hpp"
#include <iostream>

Buffer::Buffer()
{
  name = "";
  ptr = NULL;
  ptr_end = NULL;
  line_num = 1;
  column_num = 0;
};
