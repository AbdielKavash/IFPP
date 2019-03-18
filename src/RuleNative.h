#ifndef IFPP_RULENATIVE_H
#define IFPP_RULENATIVE_H

#include "Types.h"
#include <string>
#include <vector>
#include <map>

namespace ifpp {

// Native rule, but keeps tags.
// Only one action per type allowed.
// No nested rules.
struct RuleNative {
	RuleNative(TagList t = 0) : tags(t), conditions(), actions(), useless(false) {}
	
	void addCondition(const Condition * c);
	void addAction(const Action * a);
	
	bool hasTag(TagList t) const;
	std::ostream & printSelf(std::ostream & os) const;
	RuleNative * clone() const;
	~RuleNative();
	
	TagList tags;
	
	// There can be more than one of certain conditions (lists, socketGroups).
	// We assume that there is at most one of others (interval, bool).
	std::multimap<std::string, Condition *> conditions;
	
	// Only the last action of each type is preserved.
	std::map<std::string, Action *> actions;
	
	// True if the rule does not match anything.
	// In this case we do not guarantee that the list of conditions will be anything sensible.
	bool useless;
};

bool RuleSubset(const RuleNative * small, const RuleNative * large);

typedef std::vector<RuleNative *> FilterNative;

}

#endif