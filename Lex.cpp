#include "Lex.hpp"
#include <cassert>

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

void Preprocess::except(int k, int l)
{
  Token tok;
  assert(k < tok::totals && l < tok::totals);

  int t = lex.next(tok, fbuffer);
  if ((t != k) || (t != l))
    fatal_error("encount " + tok.str + " but except " + tok::tok_str[k] 
                + " or "  + tok::tok_str[l]);
}

FileBuffer::FileBuffer(string fn)
{
  name = fn;
  fd = open(name.c_str(), O_RDONLY);
  if (fd == -1)
    fatal_error("open file " + name + " error");
  buflen = getfilesize(fd);
  ptr = buffer = (char*)malloc(buflen);
  if (buffer == NULL)
    fatal_error("memory fatal");
  int l = read(fd, buffer, buflen);
  if (l != buflen)
    fatal_error("read file " + name);
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

int Lex::next(Token& t, Buffer* fbuffer)
{
  boost::cmatch matchs;
  boost::regex_constants::match_flag_type flags = boost::match_continuous ;//| boost::match_extra;
  const char* &ptr = fbuffer->ptr;
  const char* &ptr_end = fbuffer->ptr_end;
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
    const char *p;
    if ((p =  strchr(ptr, '\n')) == NULL)
      p = ptr_end;
    fatal_error("unknown character \"" + string(ptr, p - ptr) + "\" at "
                + fbuffer->name);
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
  boost::filesystem::path p(f);
  if (boost::filesystem::exists(p))
  {
    if (!p.has_root_path())
    {
      f = boost::filesystem::absolute(p).string();
    }
  }

  inc_set.insert(f);
  fbuffer = new FileBuffer(f);
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
          macro_expand(tok);
        }
      else
        return t;
      break;
    case tok::TEOF:
      if (!buffers.empty())
      {
        inc_set.erase(fbuffer->name);
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

int Preprocess::parse_arguments(std::vector<string>& vec)
{
  string a;
  int k;
  while (k = parse_macro_arg(a))
  {
    assert((k == tok::COMMA) || (k == tok::RPAREN));
    if ((k == tok::COMMA) && (a == ""))
      fatal_error("macro need arguments");
    if ((k == tok::RPAREN) && (a == ""))
      break;
      
    vec.push_back(a);
    a = "";
    if (k == tok::RPAREN)
      break;
  }
}

int Preprocess::parse_macro_arg(string& s)
{
  Token tok;
  int t;
  
  while (t = lex.next(tok, fbuffer))
  {
    if ((t == tok::RPAREN) || (t == tok:: COMMA))
    {
      return t; 
    }
    else if (t == tok::LPAREN)
    {
      std::vector<string>  vec;
      s += tok::tok_str[tok::LPAREN];
      parse_arguments(vec);

      int i = 0;
      for (; i < vec.size(); ++i)
      {
        s += vec[i];
        if (i != vec.size() - 1)
          s += ",";
      }
      
      //replace ',' with ')'
      s += tok::tok_str[tok::RPAREN];
    }
    else if ((t == tok::TEOF) || (t == tok::LINE))
    {
      fatal_error("need macro arguments");
    }
    else
    {
      s += tok.str;
    }
  }
}

void Preprocess::macro_expand_func(const Token& token)
{
  Token tok;
  int t;
  const string& s = token.str; 
  macro_type& vec = macros_map[s];
  assert(vec.kind == tok::macfunc_type);
  
  do{
    t = lex.next(tok, fbuffer);
  }while (t == tok::SPACE);
    
  // macro followed by ')' don't expand 
  if (t == tok::RPAREN)
  {
    macros_buffer.push_back(token);
    macros_buffer.push_back(tok);
  }
  else if (t == tok::LPAREN)
  {
    std::vector<string> argus;
    // string a;
    // int k;

    // while (k = parse_macro_arg(a))
    // {
    //   assert((k == tok::COMMA) || (k == tok::RPAREN));
      
    //   if ((k == tok::COMMA) && (a == ""))
    //     fatal_error("macro need arguments");
    //   if ((k == tok::RPAREN) && (a == ""))
    //     break;
      
    //   argus.push_back(a);
    //   a = "";
    //   if (k == tok::RPAREN)
    //     break;
    // }

    parse_arguments(argus);
    if (argus.size() != vec.params.size())
      fatal_error("macro " + s + " parameters and arguments size not match");
    
    std::map<string, string> pa_map;
    
    int i = 0;
    for (; i < vec.params.size(); ++i)
    {
      pa_map[vec.params[i].str] = argus[i];
    }
    
    string expands;
    avoid_recursion.insert(token.str);
    for(std::vector<Token>::iterator i = vec.expand.begin(); 
        i != vec.expand.end(); ++i)
    {
      if (avoid_recursion.find(i->str) != avoid_recursion.end())
          expands += "\n";

      if ((pa_map.find(i->str) != pa_map.end()))
      {
        expands += pa_map[i->str];
      }
      else
      {
        expands += i->str;
      }
    }

    
    Buffer buffer;
    buffer.name = fbuffer->name;
    buffer.line_num = fbuffer->line_num;
    buffer.column_num = fbuffer->column_num;
    buffer.ptr = expands.c_str();
    buffer.ptr_end = buffer.ptr + expands.length();
    buffers.push(fbuffer);
    fbuffer = &buffer;

    
    bool next_expand = true;
    while ((t = lex.next(tok, &buffer)) != tok::TEOF)
    {
      if (t == tok::LINE)
      {
        next_expand = false;
        continue;
      }
      if ((macros_map.find(tok.str) != macros_map.end()) && next_expand)
      {
        macro_expand(tok);
      }
      else
        macros_buffer.push_back(tok);
      next_expand = true;
    }

    fbuffer = buffers.top();
    buffers.pop();
  }
  else
  {
    fatal_error("macro " + s + " need arguments");
  }
  avoid_recursion.erase(token.str);
}

void Preprocess::macro_expand(const Token& token)
{
  
  const string& s = token.str;
  macro_type& vec = macros_map[s];
  
  // don't need expand
  // if (avoid_recursion.find(s) != avoid_recursion.end())
  // {
  //   macros_buffer.push_back(token);
  //   return;
  // }


  
  if (vec.kind == tok::macfunc_type)
  {
    macro_expand_func(token);
  }
  else
  {

    assert(vec.kind == tok::macobj_type);

    avoid_recursion.insert(s);
    for (std::vector<Token>::iterator i = vec.expand.begin(); i != vec.expand.end(); ++i)
    {
      Token tok = *i;
      // convert preprocess token to c token
      // maybe create two token table will be better
      if (tok.kind == tok::INCLUDE)
      {
        tok.kind = tok::SHARP;
        tok.str = tok::tok_str[tok::SHARP];
        macros_buffer.push_back(tok);

        tok.kind = tok::IDENT;
        tok.str = "include";
        macros_buffer.push_back(tok);
      }
      else if (macros_map.find(tok.str) == macros_map.end())
      {
        macros_buffer.push_back(tok);
      }
      else if(avoid_recursion.find(tok.str) != avoid_recursion.end())
      {
        macros_buffer.push_back(tok);
      }
      else
      {
        macro_expand(tok);
      }
    }
    avoid_recursion.erase(s);
  }
}

void Preprocess::direct_def(void)
{
  Token tok;

  except(tok::SPACE);

  int t = lex.next(tok, fbuffer);  

  
  if (t != tok::IDENT)
    {
      fatal_error("define need identifier");
    }
  
  string n(tok.str);
  
  if (macros_map.find(n) != macros_map.end())
    std::cout << "redefined " + n << std::endl;
  
  macro_type& vec = macros_map[n];

  t = lex.next(tok, fbuffer);
    
  if (t == tok::LPAREN)
    {
      vec.kind = tok::macfunc_type;
      
      // 
      while ((t = lex.next(tok, fbuffer)) != tok::TEOF)
      {
        if (t == tok::SPACE)
          continue;
        else if ( t == tok::IDENT)
        {
          vec.params.push_back(tok);
          do{
            t = lex.next(tok, fbuffer);
          }while (t == tok::SPACE);
          
          if (t == tok::COMMA)
            continue;
          else if (t == tok::RPAREN)
            break;
          else
           {
             fatal_error(tok::tok_str[t] + 
                         " can not appear macro parameter list");
           }
        }
      }
      
      while ((t = lex.next(tok, fbuffer)) != tok::TEOF)
        {
          if (t == tok::LINE)
            break;
          vec.expand.push_back(tok);
        }
    }
  else if (t == tok::SPACE)
    {
      vec.kind == tok::macobj_type;
      
      bool allspace = true;

      while ((t = lex.next(tok, fbuffer)) != tok::TEOF)
        {
          if (t == tok::LINE)
            break;
          if (t != tok::SPACE)
            allspace = false;
          vec.expand.push_back(tok);
        }
      
      string one("1");
      if (allspace)
        vec.expand.push_back(Token(tok::NUMBER, one));
    }
  else
    {
      fatal_error("parse define error");
    }
}

void Preprocess::direct_inc(void)
{
  const char* &ptr = fbuffer->ptr;
  const char* &ptr_end = fbuffer->ptr_end;
  
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
  assert(fbuffer);
  boost::filesystem::path p(fn);
  
  if (boost::filesystem::exists(p) && p.has_root_path())
    return true;
  
  std::vector<string>::iterator i;
  
  //boost::filesystem::path b;
  string b;
  if (user)
  {
    boost::filesystem::path
      pp = boost::filesystem::path(fbuffer->name).parent_path();

    if (boost::filesystem::exists(pp.string() + "/" + fn))
    {
      fn = pp.string() + "/" + fn;
      return true;
    }

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
