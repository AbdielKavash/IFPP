#ifndef LFPP_DRIVER_H
#define LFPP_DRIVER_H

#include <string>
#include <map>
#include <ostream>

namespace yy {
	class location;
}

#include "ifppTypes.h"
#include "ifppLogger.h"

class ifppDriver {
public:
	ifppDriver(Logger & logger, bool trLex = false, bool trParse = false)
		: file(),
		numIns(0), numDef(0), numRule(0),
		log(logger), traceLex(trLex), traceParse(trParse),
		filter(NULL),
		varNumber(), varColor(), varFile(), varList(), varStyle() {};

	ifppDriver(const ifppDriver &) = delete;
	ifppDriver & operator=(const ifppDriver &) = delete;
	~ifppDriver() = default;

	std::string file;

	void beginParse();
	int parse(const std::string & filename, ifpp::Filter & f);
	void endParse();

	void defineVariable(const std::string & name, ifpp::VariableType type, int value);
	void defineVariable(const std::string & name, ifpp::VariableType type, const ifpp::Color & value);
	void defineVariable(const std::string & name, ifpp::VariableType type, const std::string & value);
	void defineVariable(const std::string & name, ifpp::VariableType type, const ifpp::NameList & value);
	void defineVariable(const std::string & name, ifpp::VariableType type, const ifpp::Style & value);
	
	void undefineVariable(const std::string & name);

	ifpp::VariableType getVarType(const std::string & name) const;

	int getVarValueNumber(const std::string & name) const;
	const ifpp::Color & getVarValueColor(const std::string & name) const;
	const std::string & getVarValueFile(const std::string & name) const;
	const ifpp::NameList & getVarValueList(const std::string & name) const;
	const ifpp::Style & getVarValueStyle(const std::string & name) const;
	
	void addSection(ifpp::Section * sec);

	std::ostream & warningAt(const yy::location & l);
	std::ostream & errorAt(const yy::location & l);
	std::ostream & criticalAt(const yy::location & l);

	int numIns, numDef, numRule;
private:
	Logger & log;
	bool traceLex;
	bool traceParse;

	ifpp::Filter * filter;

	std::map<std::string, int> varNumber;
	std::map<std::string, ifpp::Color> varColor;
	std::map<std::string, std::string> varFile;
	std::map<std::string, ifpp::NameList> varList;
	std::map<std::string, ifpp::Style> varStyle;	
};
#endif