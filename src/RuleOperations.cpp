#include "RuleOperations.h"

namespace ifpp {

static bool MatchedBy(const std::string & s1, const std::string & s2) {
	return s1.find(s2) != std::string::npos;
}

/***********
* CONTAINMENT - CONDITIONS
***********/

static bool ConditionSubset(const ConditionInterval * first, const ConditionInterval * second) {
	return second->from <= first->from && first->to <= second->to;
}

static bool ConditionSubset(const ConditionBool * first, const ConditionBool * second) {
	return first->value == second->value;
}

static bool ConditionSubset(const ConditionNameList * first, const ConditionNameList * second) {
	// True if every string in the first list is matched by some string in the second list.
	for (const auto & s1 : first->nameList) {
		bool found = false;
		for (const auto & s2 : second->nameList) {
			if (MatchedBy(s1, s2)) {
				// Anything matching s1 can also be matched by s2.
				found = true;
				break;
			}
		}
		if (!found) return false; // This string can not be matched by the second list.
	}
	return true;
}

static bool ConditionSubset(const ConditionSocketGroup * first, const ConditionSocketGroup * second) {
	// True if the first condition needs fewer or equal sockets of every color.
	if (first->socketGroup.r > second->socketGroup.r) return false;
	if (first->socketGroup.g > second->socketGroup.g) return false;
	if (first->socketGroup.b > second->socketGroup.b) return false;
	if (first->socketGroup.w > second->socketGroup.w) return false;
	return true;
}

bool ConditionSubset(const Condition * first, const Condition * second) {
	if (!first || !second) {
		throw InternalError("Attempting to test containment of a NULL condition!", __FILE__, __LINE__);
	}
	if (first->what != second->what) {
		throw InternalError("Attempting to test containment of conditions of different type!", __FILE__, __LINE__);
	}
	switch (first->conType) {
		case CON_INTERVAL:
			return ConditionSubset(static_cast<const ConditionInterval *>(first), static_cast<const ConditionInterval *>(second));
		case CON_BOOL:
			return ConditionSubset(static_cast<const ConditionBool *>(first), static_cast<const ConditionBool *>(second));
		case CON_NAMELIST:
			return ConditionSubset(static_cast<const ConditionNameList *>(first), static_cast<const ConditionNameList *>(second));
		case CON_SOCKETGROUP:
			return ConditionSubset(static_cast<const ConditionSocketGroup *>(first), static_cast<const ConditionSocketGroup *>(second));
		default:
			throw InternalError("Unknown condition type!", __FILE__, __LINE__);
	}
}

/***********
* INTERSECTION - CONDITIONS
***********/

static Condition * ConditionIntersection(const ConditionNameList * first, const ConditionNameList * second) {
	static NameList nl;
	nl.clear();

	for (const std::string & name1 : first->nameList) {
		for (const std::string & name2 : second->nameList) {
			// Add the more restrictive (longer) string.
			std::string toAdd = "";
			if (MatchedBy(name1, name2)) toAdd = name1;
			if (MatchedBy(name2, name1)) toAdd = name2;
			if (toAdd != "") {
				// See if we actually need to add it, or if we need to refine some other name in the list.
				// Note that the new name is *added* to the list, not intersected with it!
				for (auto it = nl.begin(); it != nl.end(); ) {
					if (MatchedBy(toAdd, *it)) {
						// The string is already matched by some other, longer, name in the list.
						toAdd = "";
						break;
					}
					if (MatchedBy(*it, toAdd)) {
						// This name in the list is useless, as the newly added string matches it anyway.
						it = nl.erase(it);
					} else {
						++it;
					}
				}
				if (toAdd != "") {
					nl.push_back(toAdd);
				}
			}
		}
	}

	if (nl.empty()) {
		return NULL;
	} else {
		return new ConditionNameList(first->what, nl);
	}
}

Condition * ConditionIntersection(const Condition * first, const Condition * second) {
	if (!first || !second) {
		throw InternalError("Attempting to take an intersection with an empty condition!", __FILE__, __LINE__);
	}
	if (first->what != second->what) {
		throw InternalError("Attempting to take an intersection of conditions of different type!", __FILE__, __LINE__);
	}

	switch (first->conType) {
		// Interval and bool are computed just by adding them to a rule.
		// SocketGroup is not handled yet (likely not worth the effort?)
		case CON_INTERVAL:
			throw InternalError("Computing an intersection of conditions when not needed!", __FILE__, __LINE__);
		case CON_BOOL:
			throw InternalError("Computing an intersection of conditions when not needed!", __FILE__, __LINE__);
		case CON_NAMELIST:
			return ConditionIntersection(static_cast<const ConditionNameList *>(first), static_cast<const ConditionNameList *>(second));
		case CON_SOCKETGROUP:
			throw InternalError("Computing an intersection of conditions when not needed!", __FILE__, __LINE__);
		default: throw InternalError("Unknown condition type!", __FILE__, __LINE__);
	}
}

/***********
* INTERSECTION - RULES
***********/

template<class T>
static bool contains(const std::vector<T> & v, const T & t) {
	for (const auto & x : v) if (t == x) return true;
	return false;
}

// We deal with these conditions differently; see the comments for ConditionIntersect.
static const std::vector<std::string> special = {"Class", "BaseType"};

RuleNative * RuleIntersection(const RuleNative * first, const RuleNative * second) {
	if (first->hasMod(MOD_FINAL)) {
		// First rule is Final and can not be overridden.
		return const_cast<RuleNative *>(first);
	}

	// Compute the intersection.
	// Preserve modifiers - Final.
	// At this point Show or Hide do not exist,
	// they get deleted when we transform an IFPP rule into a native one and then rebuilt when we print it.
	RuleNative * result = new RuleNative(second->hasMod(MOD_FINAL) ? MOD_FINAL : 0);

	// All other conditions are intersected by simply adding them to the rule.
	for (const auto & c : first->conditions) {
		if (contains(special, c.first)) continue;
		result->addCondition(c.second);
		if (result->useless) {
			goto intersection_empty;
		}
	}

	for (const auto & c : second->conditions) {
		if (contains(special, c.first)) continue;
		result->addCondition(c.second);
		if (result->useless) {
			goto intersection_empty;
		}
	}

	// Deal with special NameList conditions.
	for (const std::string & what : special) {
		const auto & r1 = first->conditions.equal_range(what);
		const auto & r2 = second->conditions.equal_range(what);

		if (r1.first == r1.second && r2.first == r2.second) {
			// Neither rule has a condition of this type.
		}
		else if (r1.first == r1.second) {
			// Only the second rule has conditions of this type.
			for (auto it = r2.first; it != r2.second; ++it) {
				result->addCondition(it->second);
			}
		}
		else if (r2.first == r2.second) {
			// Only the first rule has conditions of this type.
			for (auto it = r1.first; it != r1.second; ++it) {
				result->addCondition(it->second);
			}
		}
		else {
			// Both rules have matching conditions.
			// Intersect every condition of the first rule with every condition of the second rule.
			// Then add all these intersections to the new rule.
			// This will in reasonable cases (one condition per rule) not lead to a blowup in the number of conditions.
			// See comments for ConditionIntersection for details on why and how we do this.
			for (auto it1 = r1.first; it1 != r1.second; ++it1) {
				for (auto it2 = r2.first; it2 != r2.second; ++it2) {
					Condition * c = ConditionIntersection(it1->second, it2->second);
					if (c) {
						result->addCondition(c);
					} else {
						// This condition does not match anything.
						// Since a rule is an intersection of all conditions, it does not match anything either.
						goto intersection_empty;
					}
				}
			}
		}
	}

	{
		// Here we know that the intersection matches some items.
		// We want to know if it actually changes the rule it overrides.
		// It might not, because all of first's actions are Final, or all of second's actions are Append.
		bool changed = false;
		if (second->hasMod(MOD_FINAL)) {
			// The new rule adds a Final modifier to the previous rule.
			changed = true;
		}

		// Compute the actions according to their modifiers.
		// Collect all the actions of both rules together.
		std::map<std::string, std::pair<const Action *, const Action *> > actions;

		for (const auto & a : first->actions) {
			auto it = actions.find(a.first);
			if (it == actions.end()) {
				actions.insert(std::make_pair(a.first, std::make_pair(a.second, (Action *)NULL)));
			} else {
				it->second.first = a.second;
			}
		}

		for (const auto & a : second->actions) {
			auto it = actions.find(a.first);
			if (it == actions.end()) {
				actions.insert(std::make_pair(a.first, std::make_pair((Action *)NULL, a.second)));
			} else {
				it->second.second = a.second;
			}
		}

		for (const auto & a : actions) {
			const Action * a1 = a.second.first;
			const Action * a2 = a.second.second;

			if (second->hasMod(MOD_OVERRIDE)) {
				if (a1 && a1->hasMod(MOD_FINAL)) {
					// First action is final, do not change.
					result->addAction(a1);
				} else if (a2) {
					// Override action in first with an action in second.
					// TODO: if the action has the same parameters there is actually no change.
					result->addAction(a2);
					changed = true;
				}
			} else {
				// Second rule is Append.
				if (!a1 && a2) {
					// First action is not defined, second is.
					result->addAction(a2);
					changed = true;
				} else if (a1 && a1->hasMod(MOD_FINAL)) {
					// First action is Final.
					result->addAction(a1);
				} else if (a2 && a2->hasMod(MOD_OVERRIDE)) {
					// Second action is Override.
					result->addAction(a2);
					changed = true;
				} else {
					// First action is defined and not Final, second action is not Override.
					result->addAction(a1);
				}
			}
		}

		if (changed) {
			// A new, useful rule.
			return result;
		}
		else {
			// The new rule is redundant, as it does not modify the first rule.
			// But the first rule might still reduce the second rule in terms of conditions.
			delete result;
			return const_cast<RuleNative *>(first);
		}
	}

intersection_empty:
	// The two rules do not intersect, and do not interact with each other.
	delete result;
	return NULL;
}

/***********
* DIFFERENCE - CONDITIONS
***********/

static std::pair<DifferenceResult, Condition *> ConditionDifference(const ConditionInterval * first, const ConditionInterval * second) {
	static const auto empty = std::make_pair(EMPTY, (ConditionInterval *)NULL);
	static const auto exactlyFirst = std::make_pair(FIRST, (ConditionInterval *)NULL);
	static const auto invalid = std::make_pair(INVALID, (ConditionInterval *)NULL);

	if (!first) {
		if (second->from == INT_MIN && second->to == INT_MAX) {
			// This shouldn't happen?
			return empty;
		} else if (second->from == INT_MIN) {
			return std::make_pair(NEW, new ConditionInterval(second->what, second->to + 1, INT_MAX));
		} else if (second->to == INT_MAX) {
			return std::make_pair(NEW, new ConditionInterval(second->what, INT_MIN, second->from - 1));
		} else {
			return invalid;
		}
	}

	if (second->to < first->from) {
		//        |---|
		// |---|
		return exactlyFirst;
	}
	else if (first->to < second->from) {
		// |---|
		//        |---|
		return exactlyFirst;
	}
	else if (second->from <= first->from && first->to <= second->to) {
		//   |---|
		// |-------|
		return empty;
	}
	else if (first->from < second->from && second->to < first->to) {
		// |-------|
		//   |---|
		return invalid;
	}
	else if (second->from <= first->from && second->to < first->to) {
		//    |-----|
		// |-----|
		return std::make_pair(NEW, new ConditionInterval(first->what, second->to + 1, first->to));
	}
	else if (first->from < second->from && first->to <= second->to) {
		// |-----|
		//    |-----|
		return std::make_pair(NEW, new ConditionInterval(first->what, first->from, second->from - 1));
	}

	throw InternalError("Undefined behavior when taking difference of intervals!", __FILE__, __LINE__);
}

static std::pair<DifferenceResult, Condition *> ConditionDifference(const ConditionBool * first, const ConditionBool * second) {
	if (!first) {
		return std::make_pair(NEW, new ConditionBool(second->what, !second->value));
	}
	if (first->value != second->value) {
		return std::make_pair(FIRST, (ConditionBool *)NULL);
	}
	if (first->value == second->value) {
		return std::make_pair(EMPTY, (ConditionBool *)NULL);
	}
	throw InternalError("Undefined behavior when taking difference of bool conditions!", __FILE__, __LINE__);
}

static std::pair<DifferenceResult, Condition *> ConditionDifference(const ConditionNameList * first, const ConditionNameList * second) {
	if (!first) {
		return std::make_pair(INVALID, (ConditionSocketGroup *)NULL);
	}

	// Keep those names from first which are not matched by second.
	// This might be an overestimation, but that is okay.
	// (We are not "cheating" as we do with intersections.)
	static NameList nl;
	nl.clear();

	for (const std::string & name1 : first->nameList) {
		bool add = true;
		for (const std::string & name2 : second->nameList) {
			if (MatchedBy(name1, name2)) {
				add = false;
				break;
			}
		}
		if (add) nl.push_back(name1);
	}

	if (nl.empty()) {
		return std::make_pair(EMPTY, (Condition *)NULL);
	}
	else if (nl.size() == first->nameList.size()) {
		return std::make_pair(FIRST, (Condition *)NULL);
	} else {
		return std::make_pair(NEW, new ConditionNameList(first->what, nl));
	}
}

static std::pair<DifferenceResult, Condition *> ConditionDifference(const ConditionSocketGroup * first, const ConditionSocketGroup * second) {
	if (!first) {
		return std::make_pair(INVALID, (ConditionSocketGroup *)NULL);
	}
	if (ConditionSubset(first, second)) {
		return std::make_pair(EMPTY, (ConditionSocketGroup *)NULL);
	}
	// We can't say much otherwise.
	return std::make_pair(FIRST, (ConditionSocketGroup *)NULL);
}

std::pair<DifferenceResult, Condition *> ConditionDifference(const Condition * first, const Condition * second) {
	if (!second) {
		throw InternalError("Attempting to take difference of an empty condition!", __FILE__, __LINE__);
	}
	if (first && first->what != second->what) {
		throw InternalError("Attempting to take difference of conditions of different type!", __FILE__, __LINE__);
	}

	switch (second->conType) {
		case CON_INTERVAL:
			return ConditionDifference(static_cast<const ConditionInterval *>(first), static_cast<const ConditionInterval *>(second));
		case CON_BOOL:
			return ConditionDifference(static_cast<const ConditionBool *>(first), static_cast<const ConditionBool *>(second));
		case CON_NAMELIST:
			return ConditionDifference(static_cast<const ConditionNameList *>(first), static_cast<const ConditionNameList *>(second));
		case CON_SOCKETGROUP:
			return ConditionDifference(static_cast<const ConditionSocketGroup *>(first), static_cast<const ConditionSocketGroup *>(second));
		default: throw InternalError("Unknown condition type!", __FILE__, __LINE__);
	}
}

/***********
* DIFFERENCE - RULES
***********/

RuleNative * RuleDifference(const RuleNative * first, const RuleNative * second) {
/*
Caution: incoming bunch of math.

Let first have conditions a1, a2, a3 and second have conditions b1, b2, b3.
Therefore R1 matches a1 /\ a2 /\ a3, R2 matches b1 /\ b2 /\ b3.
We want to compute R1 - R2.

  R1 - R2
= (a1 /\ a2 /\ a3) - (b1 /\ b2 /\ b3)
= (a1 /\ a2 /\ a3) /\ (b1 /\ b2 /\ b3)'
= (a1 /\ a2 /\ a3) /\ (b1' \/ b2' \/ b3')
=    (a1 /\ a2 /\ a3 /\ b1')
  \/ (a1 /\ a2 /\ a3 /\ b2')
  \/ (a1 /\ a2 /\ a3 /\ b3')

We can not (in general) compute unions of rules; and we do not want to make more rules for this.
Even trying to do the union only makes sense if b_i and b_j are the same condition; which will generally not happen.
But if at most one of the members of the unions is non-empty, we can keep only that part.
Also, if some of the members is exactly R1, the whole union is R1 and we can return it.
(This happens if some a_i and b_i; and thus R1 and R2, are disjoint.)
This should solve several common cases - especially if R2 has only one condition.

Another "computable" case would be if all the unions are equal, and thus equal to R1.
(In other words, R1 and R2 are completely disjoint?)
But this case will be handled automatically since we return first if there is more than one non-empty component.

Only testing will show how complete this is.
*/
	int count = 0; // How many parts of the union are non-trivial.
	Condition * which = NULL; // The new condition that we would add.

	for (const auto & c2 : second->conditions) {
		if (second->conditions.count(c2.first) > 1) {
			// More than one of this type of condition in R2.
			// This will only happen with NameList and SocketGroup.
			// We leave this alone... for now.
			goto return_first;
		}

		const Condition * c1 = NULL;
		switch (first->conditions.count(c2.first)) {
			// How many conditions of the same name do we have in the first rule?
			case 0:
				// If we can compute b_i', we can simply add it as a new condition.
				c1 = NULL;
				break;
			case 1:
				// Calculate a_i /\ b_i' = a_i - b_i.
				c1 = first->conditions.find(c2.first)->second;
				break;
			default:
				// More than one of this type of condition in R2.
				// This will only happen with NameList and SocketGroup.
				// We leave this alone... for now.
				goto return_first;
		}

		auto diff = ConditionDifference(c1, c2.second);
		switch (diff.first) {
			case EMPTY:
				// This part of the union is empty. We're good.
				continue;
			case FIRST:
				// This part of the union matches all of R1. Thus the entire union will also match all of R1.
				// (This means that the conditions c1 and c2, and thus R1 and R2, are disjoint.)
				goto return_first;
			case NEW:
				// This part is non-empty, and we have a condition matching it.
				++count;
				if (which) delete which;
				which = diff.second;
				break;
			case INVALID:
				// This part is non-empty, but we can't compute it.
				++count;
				if (which) delete which;
				which = NULL; // Do not change the first rule.
				break;
			default:
				throw InternalError("Unknown result of condition difference!", __FILE__, __LINE__);
		}
	}

	if (count == 0) {
		// All components of the union are empty, thus the difference is empty as well.
		return NULL;
	}
	else if (count == 1) {
		// One of the components is non-empty.
		// Return a copy of first, with the matching condition a_i replaced by the condition b_i that defines this part.
		if (which == NULL) {
			// The difference is not a valid condition, there is nothing we can do.
			goto return_first;
		}

		// We do not use addCondition here, because we are potentially modifying a NameList condition.
		RuleNative * result = first->clone();
		auto it = result->conditions.find(which->what);
		if (it != result->conditions.end()) {
			// Replace the condition.
			delete it->second;
			it->second = which;
		} else {
			// Add a new condition.
			result->conditions.insert(std::make_pair(which->what, which));
		}
		return result;
	}
	else {
		// More than one component is non-empty; but none of them is all of R1.
		// We can't define the difference exactly.
		goto return_first;
	}

	throw InternalError("Undefined behavior in computing difference of rules!", __FILE__, __LINE__);

return_first:
	if (which) delete which;
	return const_cast<RuleNative *>(first);
}

}