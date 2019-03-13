#include "Context.h"
#include "location.hh"

// Autogenerated files are not super strict.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include "Parser.h"
#include "Lexer.h"
#pragma GCC diagnostic pop

#include <cerrno>
#include <sstream>
#include <iosfwd>

extern const int IFPP_VERSION_MAJOR;
extern const int IFPP_VERSION_MINOR;
extern const int IFPP_VERSION_PATCH;

namespace ifpp {
	
void Context::reset() {
	for (const auto stm : filter) delete stm;
	filter.clear();
	
	countIns = 0;
	countDef = 0;
	countRule = 0;
	
	varNumber.clear();
	varColor.clear();
	varFile.clear();
	varList.clear();
	
	for (const auto & s : varStyle) {
		for (const auto a : s.second) {
			delete a;
		}
	}
	varStyle.clear();
}

void Context::parse() {
	if (!(yyin = fopen(file.c_str(), "r"))) {
		std::stringstream ss;
		ss << "Unable to open input file \"" << file << "\"!" << std::endl;
		ss << "Reason: " << strerror(errno);
		throw std::runtime_error(ss.str());
	}
	
	yy::Parser parser(*this);
	
	yyset_debug(false);
	parser.set_debug_level(false);
	
	log.message() << "Parser initialized." << std::endl;
	log.message() << "Parsing file \"" << file << "\"..." << std::endl;
	
	int result = parser.parse();
	fclose(yyin);
	
	if (result != 0) throw InternalError("Parser finished with an error!", __FILE__, __LINE__);
}

bool Context::versionCheck(int vMajor, int vMinor, int vPatch) const {
	return vMajor == IFPP_VERSION_MAJOR
		&& vMinor == IFPP_VERSION_MINOR
		&& vPatch == IFPP_VERSION_PATCH;
}

void Context::defineVariable(const std::string & name, ExprType type, int value) {
	if (type != EXPR_NUMBER) throw InternalError("Defining a variable with the wrong type! Expected EXPR_NUMBER.", __FILE__, __LINE__);
	varNumber[name] = value;
}

void Context::defineVariable(const std::string & name, ExprType type, const Color & value) {
	if (type != EXPR_COLOR) throw InternalError("Defining a variable with the wrong type! Expected EXPR_COLOR.", __FILE__, __LINE__);
	varColor[name] = value;
}

void Context::defineVariable(const std::string & name, ExprType type, const std::string & value) {
	if (type != EXPR_FILE) throw InternalError("Defining a variable with the wrong type! Expected EXPR_FILE.", __FILE__, __LINE__);
	varFile[name] = value;
}

void Context::defineVariable(const std::string & name, ExprType type, const NameList & value) {
	if (type != EXPR_LIST) throw InternalError("Defining a variable with the wrong type! Expected EXPR_LIST.", __FILE__, __LINE__);
	varList[name] = value;
}

void Context::defineVariable(const std::string & name, ExprType type, const Style & value) {
	if (type != EXPR_STYLE) throw InternalError("Defining a variable with the wrong type! Expected EXPR_STYLE.", __FILE__, __LINE__);
	varStyle[name] = value;
}

void Context::undefineVariable(const std::string & name) {
	switch (getVarType(name)) {
		case EXPR_NUMBER: varNumber.erase(name); break;
		case EXPR_COLOR: varColor.erase(name); break;
		case EXPR_FILE: varFile.erase(name); break;
		case EXPR_LIST: varList.erase(name); break;
		case EXPR_STYLE:
			for (const auto & a : varStyle[name]) delete a;
			varStyle.erase(name);
			break;
		case EXPR_UNDEFINED: break;
		default: throw UnhandledCase("Variable type", __FILE__, __LINE__);
	}
}

ExprType Context::getVarType(const std::string & name) const {
	if (varNumber.count(name) > 0) return EXPR_NUMBER;
	if (varColor.count(name) > 0) return EXPR_COLOR;
	if (varFile.count(name) > 0) return EXPR_FILE;
	if (varList.count(name) > 0) return EXPR_LIST;
	if (varStyle.count(name) > 0) return EXPR_STYLE;
	return EXPR_UNDEFINED;
}

// std::map::at throws std::out_of_range if value was not found.
// We should not rely on this, always check before fetching.
int Context::getVarValueNumber(const std::string & name) const {
	return varNumber.at(name);
}

const Color & Context::getVarValueColor(const std::string & name) const {
	return varColor.at(name);
}

const std::string & Context::getVarValueFile(const std::string & name) const {
	return varFile.at(name);
}

const NameList & Context::getVarValueList(const std::string & name) const {
	return varList.at(name);
}

const Style & Context::getVarValueStyle(const std::string & name) const {
	return varStyle.at(name);
}

void Context::addStatement(Statement * stm) {
	switch (stm->stmType) {
		case STM_INSTRUCTION: ++countIns; break;
		case STM_DEFINITION: ++countDef; break;
		case STM_RULE: ++countRule; break;
		default: throw UnhandledCase("Statement type", __FILE__, __LINE__);
	}
	filter.push_back(stm);
}

std::ostream & Context::warningAt(const yy::location & l) {
	return log.warning() << "Line " << l << ": ";
}

std::ostream & Context::errorAt(const yy::location & l) {
	return log.error() << "Line " << l << ": ";
}

std::ostream & Context::criticalAt(const yy::location & l) {
	return log.critical() << "Line " << l << ": ";
}


}
