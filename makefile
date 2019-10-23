CC		:= gcc
CFLAGS	:= -Wall -O2 -g
CXX 	:= g++
CXXFLAGS:= -Wall -O2 -std=c++11 -g

LEX		:= flex 
YACC	:= bison

SCANNER	:= scanner
PARSER	:= parser

SRCS	:= $(wildcard *.cc)
OBJS	:= $(patsubst %.cc, %.o, $(SRCS)) $(SCANNER).o $(PARSER).o

LEXS	:= $(wildcard *.lex *.l)
YACCS	:= $(wildcard *.yacc *.y)

TARGET	:= eeyore

LIBS	:= -lfl

RM		:= rm -rf

all: $(TARGET)

run: all
	./$(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

$(SCANNER).o: $(SCANNER).cc 
	$(CXX) $(CXXFLAGS) -DPARSER_H="\"${PARSER}.hh\"" -c -o $@ $^

$(SCANNER).cc: $(LEXS) $(PARSER).hh
	$(LEX) -o $@ $^

$(PARSER).hh: $(PARSER).cc

$(PARSER).cc: $(YACCS)
	$(YACC) -d -o $@ $^

clean:
	$(RM) $(TARGET) $(OBJS) $(SCANNER).* $(PARSER).*