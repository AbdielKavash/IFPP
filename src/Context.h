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
		file(f), filter(F), countIns(0), countDef(0), countRule(0), log(l),
		varNumber(), varColor(), varFile(), varList(), varStyle() {};
		
	void reset();
	void parse();
	
	bool versionCheck(int vMajor, int vMinor, int vPatch) const;
		
	void defineVariable(const std::string & name, ifpp::ExprType type, int value);
	void defineVariable(const std::string & name, ifpp::ExprType type, const ifpp::Color & value);
	void defineVariable(const std::string & name, ifpp::ExprType type, const std::string & value);
	void defineVariable(const std::string & name, ifpp::ExprType type, const ifpp::NameList & value);
	void defineVariable(const std::string & name, ifpp::ExprType type, const ifpp::Style & value);
	
	void undefineVariable(const std::string & name);

	ifpp::ExprType getVarType(const std::string & name) const;

	int getVarValueNumber(const std::string & name) const;
	const ifpp::Color & getVarValueColor(const std::string & name) const;
	const std::string & getVarValueFile(const std::string & name) const;
	const ifpp::NameList & getVarValueList(const std::string & name) const;
	const ifpp::Style & getVarValueStyle(const std::string & name) const;
	
	void addStatement(Statement * stm);
	
	std::ostream & warningAt(const yy::location & l);
	std::ostream & errorAt(const yy::location & l);
	std::ostream & criticalAt(const yy::location & l);

	std::string file;
	FilterIFPP & filter;
	int countIns, countDef, countRule;

private:
	Logger & log;
	
	std::map<std::string, int> varNumber;
	std::map<std::string, ifpp::Color> varColor;
	std::map<std::string, std::string> varFile;
	std::map<std::string, ifpp::NameList> varList;
	std::map<std::string, ifpp::Style> varStyle;	
};

}

#endif