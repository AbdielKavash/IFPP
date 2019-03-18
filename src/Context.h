#ifndef IFPP_CONTEXT_H
#define IFPP_CONTEXT_H

#include "Types.h"
#include "Logger.h"

namespace yy {
	class location;
}

namespace ifpp {

class Context {
public:
	Context(const std::string & f, FilterIFPP & F, Logger & l) :
		file(f), filter(F), countIns(0), countDef(0), countBlock(0), log(l),
		varNumber(), varColor(), varFile(), varList(), varMacro() {};
		
	void reset();
	void parse();
	
	bool versionCheck(int vMajor, int vMinor, int vPatch) const;
		
	void defineVariable(const std::string & name, ExprType type, int value);
	void defineVariable(const std::string & name, ExprType type, const Color & value);
	void defineVariable(const std::string & name, ExprType type, const std::string & value);
	void defineVariable(const std::string & name, ExprType type, const NameList & value);
	void defineVariable(const std::string & name, ExprType type, const CommandList & value);
	
	void undefineVariable(const std::string & name);

	ExprType getVarType(const std::string & name) const;

	int getVarValueNumber(const std::string & name) const;
	const Color & getVarValueColor(const std::string & name) const;
	const std::string & getVarValueFile(const std::string & name) const;
	const NameList & getVarValueList(const std::string & name) const;
	const CommandList & getVarValueMacro(const std::string & name) const;
	
	void addInstruction(Instruction * ins);
	
	std::ostream & warningAt(const yy::location & l);
	std::ostream & errorAt(const yy::location & l);
	std::ostream & criticalAt(const yy::location & l);

	std::string file;
	FilterIFPP & filter;
	int countIns, countDef, countBlock;

private:
	Logger & log;
	
	std::map<std::string, int> varNumber;
	std::map<std::string, Color> varColor;
	std::map<std::string, std::string> varFile;
	std::map<std::string, NameList> varList;
	std::map<std::string, CommandList> varMacro;	
};

}

#endif