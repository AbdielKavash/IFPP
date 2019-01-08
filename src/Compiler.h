#ifndef IFPP_COMPILER_H
#define IFPP_COMPILER_H

#include "ifppTypes.h"
#include "ifppLogger.h"

namespace ifpp {
	
// Native rule, but keeps modifiers.
// Only one action per type allowed.
// No nested rules.
struct RuleNative {
	RuleNative(ModifierList m) : modifiers(m), conditions(), actions(), useless(false), guid(IFPP_GUID()) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const;
	bool hasMod(ModifierList ml) const;
	~RuleNative();
	
	void addCondition(const Condition * c);
	void addAction(const Action * a);
	RuleNative * clone() const;
	
	ModifierList modifiers;
	
	// There can be more than one of certain conditions (lists, socketGroups).
	// We assume that there is at most one of others (interval, bool).
	std::multimap<std::string, Condition *> conditions;
	
	// Only the last action of each type is preserved.
	std::map<std::string, Action *> actions;
	
	// True if the rule does not match anything.
	// In this case we do not guarantee that the list of conditions will be anything sensible.
	bool useless;
	
	int guid;
};

typedef std::vector<RuleNative *> FilterNative;
std::ostream & print(std::ostream & os, PrintStyle ps, const FilterNative & f);
	
class Compiler {
public:
	Compiler(Logger & l, bool wp, std::ostream & ps) : log(l), writePartial(wp), partialStream(ps) {};
	void Compile(const Filter & inFilter, FilterNative & outFilter);
	
private:
	Logger & log;
	bool writePartial;
	std::ostream & partialStream;
};

}

#endif