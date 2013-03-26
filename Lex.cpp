#include "Lex.hpp"

#define DEBUG

void fatal_error(string s)
{
  cerr << s << endl;
  exit(255);
}

size_t getfilesize(int fd)
{
  struct stat s;
  fstat(fd, &s);
  return s.st_size;
}

Lex::Lex(string f):filename(f)
{
  fd = open(filename.c_str(), O_RDONLY);
  if (fd == -1)
    fatal_error("open file " + filename + " error");
  buflen = getfilesize(fd);
  ptr = buffer = (char*)malloc(buflen);
  if (buffer == NULL)
    fatal_error("memory fatal");
  int l = read(fd, buffer, buflen);
  if (l != buflen)
    fatal_error("read file " + filename);
  ptr_end = buffer + l;
  
  string p;
  
  int i = 1;

  for (p = tok::patterns[0]; i < tok::totals - 1; ++i)
    {
      p += string("|") + tok::patterns[i];
    }
  
  pattern = boost::regex(p, boost::regex::perl);
}

Lex::~Lex()
{
  if(buffer)
    free(buffer);
  if(fd)
    close(fd);
}

int Lex::next(Token& t)
{
  boost::cmatch matchs;
  boost::regex_constants::match_flag_type flags = boost::match_continuous;

  if (ptr == ptr_end)
    {
      t.kind = tok::TEOF;
      return t.kind;
    }

  if (boost::regex_search(ptr,  matchs , pattern, flags))
    {
      int i;
      for (i = 1; i < matchs.size(); ++i)
        {
          if (matchs[i].matched)
            {
              t.kind = (tok::kind)(i);
              t.ptr = ptr;
              t.len = matchs[i].length();
              ptr += t.len;
              return t.kind;
            }
        }
    }
  else
    {
      //fatal_error("unknown character at line " + line + "column " + column);
    }
}

#ifdef DEBUG
int main()
{
  Lex l("test.c");
  Token t;

  while (l.next(t) != tok::TEOF)
    std::cout << t.str() << std::endl;
}
#endif
