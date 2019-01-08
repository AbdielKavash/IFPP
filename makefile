GenDir = gen
SrcDir = src
TestDir = tests

GenClass = Lexer Parser
SrcClass = Types Logger Context ParserWrapper
#ifppDriver ifppRuleOperations ifppCompiler

TestParse = basicSyntax
TestCompile = overrideWithoutChange overrideSameAction overrideNumeric nameLists incremental
AllTests = $(TestParse) $(TestCompile)

GenObj = $(addprefix $(GenDir)/,$(addsuffix .o,$(GenClass)))
SrcObj = $(addprefix $(GenDir)/,$(addsuffix .o,$(SrcClass)))
AllObjs = $(GenObj) $(SrcObj)

Flex = flex -i
Bison = bison
Gcc = g++ -Wall -Wextra -pedantic -I src -I gen
GccStrict = g++ -Wall -Wextra -pedantic -Weffc++ -Werror -I src -I gen

.PHONY: lex parser analyze tests doc clean

ifpp: $(AllObjs) src/ifpp.cpp
	$(GccStrict) -o ifpp $(AllObjs) src/ifpp.cpp

lex: $(GenDir)/Lexer.cpp $(GenDir)/Lexer.h

parser: $(GenDir)/Parser.cpp $(GenDir)/Parser.h

analyze: src/Parser.y
	$(Bison) -Wall --report=all --report-file=BisonReport.txt $<

tests: $(AllTests)

doc: doc/ifpp-manual.html

clean:
	-rm gen/*
	-rm tests/*.parsed.ifpp
	-rm tests/*.partial.ifpp
	-rm tests/*.filter
	-rm ifpp.exe



$(GenDir)/Lexer.cpp $(GenDir)/Lexer.h: $(SrcDir)/Lexer.l $(SrcDir)/Types.h $(GenDir)/Parser.h
	$(Flex) --outfile="gen/Lexer.cpp" --header-file="gen/Lexer.h" $(SrcDir)/Lexer.l

$(GenDir)/Parser.cpp $(GenDir)/Parser.h $(GenDir)/location.hh: $(SrcDir)/Parser.y $(SrcDir)/Types.h $(SrcDir)/Context.h
	$(Bison) --output="gen/Parser.cpp" --defines="gen/Parser.h" $(SrcDir)/Parser.y

$(GenObj): $(GenDir)/%.o: $(GenDir)/%.cpp $(SrcDir)/Types.h $(SrcDir)/Logger.h
	$(Gcc) -c -o $@ $<

$(SrcObj): $(GenDir)/%.o: $(SrcDir)/%.cpp $(SrcDir)/%.h $(SrcDir)/Types.h
	$(GccStrict) -c -o $@ $<

$(GenDir)/Context.o: $(SrcDir)/Logger.h



$(TestParse): %: $(TestDir)/%.parsed.ifpp

$(TestCompile): %: $(TestDir)/%.filter

$(TestDir)/%.parsed.ifpp: $(TestDir)/%.ifpp ifpp
	./ifpp -c -Dpartial -DparseOnly $<

$(TestDir)/%.filter: $(TestDir)/%.ifpp ifpp
	./ifpp -c -Dpartial $<

doc/ifpp-manual.html: src/ifpp-manual.texinfo
	makeinfo --html --no-split --css-include="src/ifpp-manual.css" -o "doc/ifpp-manual.html" "src/ifpp-manual.texinfo"


