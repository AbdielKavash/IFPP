extern const int IFPP_VERSION_MAJOR;
extern const int IFPP_VERSION_MINOR;
extern const int IFPP_VERSION_PATCH;

#include "Context.h"

#include <iosfwd>

#include "location.hh"

namespace ifpp {
	
void Context::reset() {
	for (const auto & stm : filter) delete stm;
	filter.clear();
	
	countIns = 0;
	countDef = 0;
	countRule = 0;
	
	varNumber.clear();
	varColor.clear();
	varFile.clear();
	varList.clear();
	
	for (const auto & s : varStyle) {
		for (const auto & a : s.second) {
			delete a;
		}
	}
	varStyle.clear();
}

bool Context::versionCheck(int vMajor, int vMinor, int vPatch) const {
	return vMajor == IFPP_VERSION_MAJOR
		&& vMinor == IFPP_VERSION_MINOR
		&& vPatch == IFPP_VERSION_PATCH;
}

void Context::defineVariable(const std::string & name, VariableType type, int value) {
	if (type != VAR_NUMBER) throw InternalError("Defining a variable with the wrong type! Expected VAR_NUMBER.", __FILE__, __LINE__);
	varNumber[name] = value;
}

void Context::defineVariable(const std::string & name, VariableType type, const Color & value) {
	if (type != VAR_COLOR) throw InternalError("Defining a variable with the wrong type! Expected VAR_COLOR.", __FILE__, __LINE__);
	varColor[name] = value;
}

void Context::defineVariable(const std::string & name, VariableType type, const std::string & value) {
	if (type != VAR_FILE) throw InternalError("Defining a variable with the wrong type! Expected VAR_FILE.", __FILE__, __LINE__);
	varFile[name] = value;
}

void Context::defineVariable(const std::string & name, VariableType type, const NameList & value) {
	if (type != VAR_LIST) throw InternalError("Defining a variable with the wrong type! Expected VAR_LIST.", __FILE__, __LINE__);
	varList[name] = value;
}

void Context::defineVariable(const std::string & name, VariableType type, const Style & value) {
	if (type != VAR_STYLE) throw InternalError("Defining a variable with the wrong type! Expected VAR_STYLE.", __FILE__, __LINE__);
	varStyle[name] = value;
}

void Context::undefineVariable(const std::string & name) {
	switch (getVarType(name)) {
		case VAR_NUMBER: varNumber.erase(name); break;
		case VAR_COLOR: varColor.erase(name); break;
		case VAR_FILE: varFile.erase(name); break;
		case VAR_LIST: varList.erase(name); break;
		case VAR_STYLE:
			for (const auto & a : varStyle[name]) delete a;
			varStyle.erase(name);
			break;
		case VAR_UNDEFINED: break;
		default: throw UnhandledCase("Variable type", __FILE__, __LINE__);
	}
}

VariableType Context::getVarType(const std::string & name) const {
	if (varNumber.count(name) > 0) return VAR_NUMBER;
	if (varColor.count(name) > 0) return VAR_COLOR;
	if (varFile.count(name) > 0) return VAR_FILE;
	if (varList.count(name) > 0) return VAR_LIST;
	if (varStyle.count(name) > 0) return VAR_STYLE;
	return VAR_UNDEFINED;
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
