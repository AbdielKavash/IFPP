#ifndef IFPP_COMPILER_H
#define IFPP_COMPILER_H

#include "Types.h"
#include "RuleNative.h"
#include "Logger.h"

namespace ifpp {
	
class Compiler {
public:
	Compiler(Logger & l, bool wp, std::ostream & ps) : log(l), writePartial(wp), partialStream(ps) {};
	void Compile(const FilterIFPP & inFilter, FilterNative & outFilter);
	
private:
	Logger & log;
	bool writePartial;
	std::ostream & partialStream;
};

}

#endif