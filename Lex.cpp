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

FileBuffer::FileBuffer(string fn):filename(fn)
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
}

FileBuffer::~FileBuffer()
{
  if(buffer)
    free(buffer);
  if(fd)
    close(fd);

}

Lex::Lex(string f)
{
  //collect token infomation
  //build pattern
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
  
}

int Lex::next(Token& t, FileBuffer* fbuffer)
{
  boost::cmatch matchs;
  boost::regex_constants::match_flag_type flags = boost::match_continuous ;//| boost::match_extra;
  char* &ptr = fbuffer->ptr;
  char* &ptr_end = fbuffer->ptr_end;

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
        // if idnet not keyword
        // if (matchs[i].length() < j)
        //   t.kind = tok::IDENT;
        // else
        t.kind = (tok::kind)i;
        //std::cout << "ident = " << matchs[tok::IDENT] << std::endl;
        t.ptr = ptr;
        t.len = matchs[i].length();
      }
    }
  }
  else
  {
    fatal_error("unknown character");// at line " + line + "column " + column);
  }

  ptr += matchs[0].length();
  if (ptr > ptr_end)
    fatal_error("unknown token");

  return t.kind;
}

Preprocess::Preprocess(string f):lex(f)
{
  add_inc_path(boost::filesystem::current_path().string());
  
  if (findfile(f, true))
  {
    inc_set.insert(f);
    fbuffer = new FileBuffer(f);
  }

}

void Preprocess::add_inc_path(string path)
{
  if (path[path.length()] != '/')
    path += "/";
  inc_path.push_back(path);
}

void Preprocess::add_sys_inc_path(string path)
{
  if (path[path.length()] != '/')
    path += "/";
  sys_inc_path.push_back(path);
}

int Preprocess::next(Token& tok)
{
  for(;;)
  {
    int t = lex.next(tok, fbuffer);
    switch(t)
    {
    default:
      return t;
    case tok::INCLUDE:
      direct_inc();
      next(tok);
      break;
    case tok::TEOF:
      if (!buffers.empty())
      {
        inc_set.erase(fbuffer->filename);
        delete fbuffer;
        fbuffer = buffers.top();
        buffers.pop();
        break;
      }
      delete fbuffer;
      return t;
    }
  }
  assert(0);
  return tok::TEOF;
}

bool Preprocess::has_inc(string& file)
{
  return inc_set.find(file) != inc_set.end();
}

void Preprocess::direct_inc(void)
{
  char* &ptr = fbuffer->ptr;
  char* &ptr_end = fbuffer->ptr_end;
  
  bool userdir = false;
  boost::cmatch matchs;
  boost::regex_constants::match_flag_type flags = boost::match_continuous |
    boost::match_not_dot_newline;

  string p = "\\s*<\\s*(.+)\\s*>\\s*|\\s*\"\\s*(.+)\\s*\"\\s*";
  string filename;
  boost::regex re(p, boost::regex::perl);

  if (boost::regex_search(ptr,  matchs , re, flags))
  {
    int i;
    for (i = 1; i < matchs.size(); ++i)
    {
      if (matchs[i].matched)
      {
        filename = matchs[i];
        break;
      }
    }
    userdir = i == 2 ;
  }
  else
  {
    fatal_error("unknown token");
  }
  
  ptr += matchs[0].length();
  
  if (ptr > ptr_end)
  {
    fatal_error("include need filename");
  }
 
  if (findfile(filename, userdir))
  {
    if (!has_inc(filename))
    {
      buffers.push(fbuffer);
      inc_set.insert(filename);
      fbuffer = new FileBuffer(filename);
    }
  }
  else
  {
    fatal_error("include file not found" + filename);
  }
}

bool Preprocess::findfile(string& fn, bool user)
{
  boost::filesystem::path p(fn);
  
  if (boost::filesystem::exists(p) && p.has_root_path())
    return true;

  string b = fn;
  std::vector<string>::iterator i;

  if (user)
  {
    for (i = inc_path.begin(); i < inc_path.end(); ++i) 
    {
      b = *i + fn;
      if (boost::filesystem::exists(b) && boost::filesystem::is_regular_file(b))
      {
        fn = b;
        return true;
      }
    }
  }
  
  for (i = sys_inc_path.begin(); i < sys_inc_path.end(); ++i)
  {
    b = *i + fn;
    if (boost::filesystem::exists(b) && 
        boost::filesystem::is_regular_file(b))
    {
      fn = b;
      return true;
    }
  }

  return false;
}

#ifdef DEBUG
int main()
{
  Preprocess pp("test.c");
  Token t;

  while (pp.next(t) != tok::TEOF)
    std::cout << t.str() << std::endl;
}
#endif
