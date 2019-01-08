#ifndef IFPP_RULE_OPERATIONS_H
#define IFPP_RULE_OPERATIONS_H

#include "ifppTypes.h"
#include "ifppCompiler.h"

namespace ifpp {

/*
Returns true if the first condition matches a subset of the second.
Assumes both inputs are non-NULL.
*/
bool ConditionSubset(const Condition * first, const Condition * second);

/*
Returns a condition obtained as an intersection of two conditions.
Assumes both inputs are non-NULL.

Always returns a new condition.
If the intersection doesn't match anything, returns NULL.

For most conditions, adding them to a rule behaves like intersecting them.
RuleNative is smart enough to trim conditions of the same type.
So we do not need this function (and using it is treated as an internal error).

But we deal with Class and BaseType separately to avoid generating many useless rules.
The assumption here is that the matching is done on the *input* strings, rather than all possible strings.
Thus two strings which are incomparable as substrings are assumed to not match the same string.
For example, the intersection of BaseType "Scroll" and BaseType "Wisdom" does not match any items.
This can exclude some valid matches (such as the example above), but for "reasonable" use cases
it leads to much fewer useless rules: consider a different style for every currency item.

Note that we do not do this for HasExplicitMod, as it is quite reasonable for an item to match
multiple such conditions simultaneously - when searching for an item with two or more different mods.

Returns NULL if the intersection of the conditions is empty (according to the above rules).
*/
Condition * ConditionIntersection(const Condition * first, const Condition * second);

/*
Returns a rule obtained as an intersection of two rules.
The intersection is always exact (but see above for intersecting NameLists).

Returns NULL if the two rules do not intersect at all.
Returns first if the two rules potentially intersect, but the intersection does not need a separate rule.
	This might be because first is Final, or because second does not override any action in first.
Returns a new rule otherwise.

Conditions in the returned rule are an exact intersection of the two rules' conditions.
Actions are resolved according to the actions' modifiers.
*/
RuleNative * RuleIntersection(const RuleNative * first, const RuleNative * second);

/*
Computes a difference of the conditions first - second.
If the first input is NULL, we assume there is no condition (i.e. matches everything).
This lets us compute the complement of a condition.

Output is:
first:
	EMPTY if the difference is empty (matches nothing).
	FIRST if the difference is exactly first.
	NEW if the difference is a condition distinct from first.
	INVALID if the difference is not a valid condition (e.g. not an interval for interval conditions).
	
second:	
	If first is NEW, second is a new condition matching the interval.
	Otherwise second is NULL.
*/
enum DifferenceResult { EMPTY, FIRST, NEW, INVALID };
std::pair<DifferenceResult, Condition *> ConditionDifference(const Condition * first, const Condition * second);

/*
Returns a rule obtained as the difference first - second.
This can not always be computed exactly (conditions are not closed under complement).
Thus we result an overestimate: (first - second) <= result <= first. (Subset relations.)

Returns NULL if the difference is empty - the second condition matches everything the first does.
Returns first if the result is first - either because the rules do not intersect, or because we can not give a better estimate.
Returns a new rule otherwise.

This should *not* ignore first if first is final.
We use it to restrict the rule being added, which can be final.
Actions in the new rule always copy actions in first.
*/
RuleNative * RuleDifference(const RuleNative * first, const RuleNative * second);
	
}

#endif