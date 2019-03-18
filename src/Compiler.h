#ifndef IFPP_COMPILER_H
#define IFPP_COMPILER_H

#include "Types.h"
#include "RuleNative.h"
#include "Logger.h"

namespace ifpp {
	
class Compiler {
public:
	Compiler(Logger & l) : log(l) {};
	void Compile(FilterNative & outFilter, const FilterIFPP & inFilter);
	
private:
	Logger & log;
};

}

#endif