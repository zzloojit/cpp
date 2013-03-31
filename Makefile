Lex:Lex.o Buffer.o Expr.o
	g++ Lex.o Buffer.o Expr.o -g -o Lex -lboost_regex -lboost_filesystem -lboost_system

Lex.o:Lex.cpp
	g++ Lex.cpp -o Lex.o -g -c
Expr.o:Expr.cpp Expr.hpp
	g++ Expr.cpp -o Expr.o -g -c

Buffer.o:Buffer.cpp Buffer.hpp
	g++ Buffer.cpp -o Buffer.o -g -c

clean:
	rm Lex Lex.o Buffer.o Expr.o