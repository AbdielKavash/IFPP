#include "ifppDriver.h"

#include <string>

// This autogenerated file is not super strict.
#pragma GCC diagnostic ignored "-Weffc++"
#include "ifppParser.h"
#pragma GCC diagnostic error "-Weffc++"

int ifppDriver::parse(const std::string & filename, ifpp::Filter & f) {
	file = filename;
	filter = &f;
	filter->clear();
	
	numIns = numDef = numRule = 0;

	beginParse();	
	yy::ifppParser parser(*this);
	parser.set_debug_level(traceParse);
	int result = parser.parse();
	endParse();
	return result;
}

void ifppDriver::defineVariable(const std::string & name, ifpp::VariableType type, int value) {
	if (type != ifpp::VAR_NUMBER) throw ifpp::InternalError("Defining a variable with the wrong type! Expected VAR_NUMBER.", __FILE__, __LINE__);
	varNumber[name] = value;
}

void ifppDriver::defineVariable(const std::string & name, ifpp::VariableType type, const ifpp::Color & value) {
	if (type != ifpp::VAR_COLOR) throw ifpp::InternalError("Defining a variable with the wrong type! Expected VAR_COLOR.", __FILE__, __LINE__);
	varColor[name] = value;
}

void ifppDriver::defineVariable(const std::string & name, ifpp::VariableType type, const std::string & value) {
	if (type != ifpp::VAR_FILE) throw ifpp::InternalError("Defining a variable with the wrong type! Expected VAR_FILE.", __FILE__, __LINE__);
	varFile[name] = value;
}

void ifppDriver::defineVariable(const std::string & name, ifpp::VariableType type, const ifpp::NameList & value) {
	if (type != ifpp::VAR_LIST) throw ifpp::InternalError("Defining a variable with the wrong type! Expected VAR_LIST.", __FILE__, __LINE__);
	varList[name] = value;
}

void ifppDriver::defineVariable(const std::string & name, ifpp::VariableType type, const ifpp::Style & value) {
	if (type != ifpp::VAR_STYLE) throw ifpp::InternalError("Defining a variable with the wrong type! Expected VAR_STYLE.", __FILE__, __LINE__);
	varStyle[name] = value;
}

void ifppDriver::undefineVariable(const std::string & name) {
	varNumber.erase(name);
	varColor.erase(name);
	varFile.erase(name);
	varList.erase(name);
	varStyle.erase(name);
}

ifpp::VariableType ifppDriver::getVarType(const std::string & name) const {
	if (varNumber.count(name) > 0) return ifpp::VAR_NUMBER;
	if (varColor.count(name) > 0) return ifpp::VAR_COLOR;
	if (varFile.count(name) > 0) return ifpp::VAR_FILE;
	if (varList.count(name) > 0) return ifpp::VAR_LIST;
	if (varStyle.count(name) > 0) return ifpp::VAR_STYLE;
	return ifpp::VAR_UNDEFINED;
}

// std::map::at throws std::out_of_range if value was not found.
// We should not rely on this, always check before fetching.
int ifppDriver::getVarValueNumber(const std::string & name) const {
	return varNumber.at(name);
}

const ifpp::Color & ifppDriver::getVarValueColor(const std::string & name) const {
	return varColor.at(name);
}

const std::string & ifppDriver::getVarValueFile(const std::string & name) const {
	return varFile.at(name);
}

const ifpp::NameList & ifppDriver::getVarValueList(const std::string & name) const {
	return varList.at(name);
}

const ifpp::Style & ifppDriver::getVarValueStyle(const std::string & name) const {
	return varStyle.at(name);
}

void ifppDriver::addSection(ifpp::Section * sec) {
	switch (sec->secType) {
		case ifpp::SEC_INSTRUCTION: ++numIns; break;
		case ifpp::SEC_DEFINITION: ++numDef; break;
		case ifpp::SEC_RULE: ++numRule; break;
		default: throw ifpp::InternalError("Unknown section type!", __FILE__, __LINE__);
	}
	filter->push_back(sec);
}

std::ostream & ifppDriver::warningAt(const yy::location & l) {
	return log.warning() << "Line " << l << ": ";
}

std::ostream & ifppDriver::errorAt(const yy::location & l) {
	return log.error() << "Line " << l << ": ";
}

std::ostream & ifppDriver::criticalAt(const yy::location & l) {
	return log.critical() << "Line " << l << ": ";
}
