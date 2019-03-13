GenDir = gen
SrcDir = src

GenClass = Lexer Parser
SrcClass = Types Logger Context RuleNative Compiler 

GenObj = $(addprefix $(GenDir)/,$(addsuffix .o,$(GenClass)))
SrcObj = $(addprefix $(GenDir)/,$(addsuffix .o,$(SrcClass)))
AllObjs = $(GenObj) $(SrcObj)

Flex = flex -i
Bison = bison
Gcc = g++ -Wall -Wextra -pedantic -Wno-unused-function -Wfatal-errors -I src -I gen
GccStrict = g++ -Wall -Wextra -pedantic -Weffc++ -Werror -Wfatal-errors -I src -I gen

.PHONY: lex parser analyze tests doc clean

ifpp: $(AllObjs) src/ifpp.cpp
	$(GccStrict) -o ifpp $(AllObjs) src/ifpp.cpp

lex: $(GenDir)/Lexer.cpp $(GenDir)/Lexer.h

parser: $(GenDir)/Parser.cpp $(GenDir)/Parser.h $(GenDir)/location.hh $(GenDir)/position.hh

analyze: src/Parser.y
	$(Bison) -Wall --report=all --report-file=BisonReport.txt $<

doc: doc/ifpp-manual.html

clean:
	-rm gen/*
	-rm doc/*
	-rm ifpp.exe



$(GenDir)/Lexer.cpp $(GenDir)/Lexer.h: $(SrcDir)/Lexer.l $(SrcDir)/Types.h $(GenDir)/Parser.h
	$(Flex) --outfile="gen/Lexer.cpp" --header-file="gen/Lexer.h" $(SrcDir)/Lexer.l

$(GenDir)/Parser.cpp $(GenDir)/Parser.h $(GenDir)/location.hh $(GenDir)/position.hh: $(SrcDir)/Parser.y $(SrcDir)/Types.h $(SrcDir)/Context.h
	$(Bison) --output="gen/Parser.cpp" --defines="gen/Parser.h" $(SrcDir)/Parser.y

$(GenObj): $(GenDir)/%.o: $(GenDir)/%.cpp $(GenDir)/%.h
	$(Gcc) -c -o $@ $<

$(SrcObj): $(GenDir)/%.o: $(SrcDir)/%.cpp $(SrcDir)/%.h
	$(GccStrict) -c -o $@ $<


doc/ifpp-manual.html: src/ifpp-manual.texinfo
	makeinfo --html --no-split --css-include="src/ifpp-manual.css" -o "doc/ifpp-manual.html" "src/ifpp-manual.texinfo"


