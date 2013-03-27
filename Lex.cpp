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

void Preprocess::except(int k)
{
  Token tok;
  if (lex.next(tok, fbuffer) != k)
    fatal_error("encount " + tok.str + " but except " + tok::tok_str[k]);
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
  int& line_num = fbuffer->line_num;
  int& column_num = fbuffer->column_num;
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
        t.kind = (tok::Kind)i;
        t.str = matchs[i];
        break;
      }
    }
  }
  else
  {
    fatal_error("unknown character");// at line " + line + "column " + column);
  }

  if (t.kind == tok::LINE)
    {
      line_num++;
      column_num = 0;
    }
  
  ptr += matchs[0].length();
  column_num = matchs[0].length();
  
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
    if (!macros_buffer.empty())
      {
        tok = macros_buffer.front();
        macros_buffer.pop_front();
        return tok.kind;
      }
    int t = lex.next(tok, fbuffer);
    switch(t)
    {
    default:
      return t;
    case tok::INCLUDE:
      direct_inc();
      break;
    case tok::DEFINE:
      direct_def();
      break;
    case tok::IDENT:
      if (macros_map.find(tok.str) != macros_map.end())
        {
          macro_expand(tok.str);
        }
      else
        return t;
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

void Preprocess::macro_expand(std::string s)
{
  avoid_recursion.insert(s);
  std::vector<Token>& vec = macros_map[s];
  Token tok;
  for (std::vector<Token>::iterator i = vec.begin(); i != vec.end(); ++i)
    {
      tok = *i;
      if (macros_map.find(tok.str) == macros_map.end())
        {
          macros_buffer.push_back(tok);
        }
      else if(avoid_recursion.find(tok.str) != avoid_recursion.end())
        {
          macros_buffer.push_back(tok);
        }
      else
        {
          macro_expand(tok.str);
        }
    }
  avoid_recursion.erase(s);
}

void Preprocess::direct_def(void)
{
  Token tok;
  int type;

  except(tok::SPACE);

  int t = lex.next(tok, fbuffer);  

  
  if (t != tok::IDENT)
    {
      fatal_error("define need identifier");
    }
  
  string n(tok.str);
  
  if (macros_map.find(n) != macros_map.end())
    std::cout << "redefined " + n << std::endl;
  
  std::vector<Token>& vec = macros_map[n];

  t = lex.next(tok, fbuffer);
    
  if (t == tok::LPAREN)
    {
      type = tok::macfunc_type;
    }
  else if (t == tok::SPACE)
    {
      type == tok::macobj_type;
      bool allspace = true;

      while ((t = lex.next(tok, fbuffer)) != tok::TEOF)
        {
          if (t == tok::LINE)
            break;
          if (t != tok::SPACE)
            allspace = false;
          vec.push_back(tok);
        }
      
      string one("1");
      if (allspace)
        vec.push_back(Token(tok::NUMBER, one));
    }
  else
    {
      fatal_error("parse define error");
    }
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

Preprocess::~Preprocess()
{
  
}

#ifdef DEBUG
int main()
{
  Preprocess pp("test.c");
  Token t;

  while (pp.next(t) != tok::TEOF)
    std::cout << t.dump();
}
#endif
