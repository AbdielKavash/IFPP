#include "RuleNative.h"

namespace ifpp {
	
/***********
* TESTING CONTAINMENT OF CONDITIONS
***********/
	
static bool ConditionSubset(const ConditionInterval * small, const ConditionInterval * large) {
	return large->from <= small->from && small->to <= large->to;
}

static bool ConditionSubset(const ConditionBool * small, const ConditionBool * large) {
	return small->value == large->value;
}

static bool ConditionSubset(const ConditionNameList * small, const ConditionNameList * large)
{
	// True if every string in the small list is matched by some string in the large list.
	for (const auto & s1 : small->nameList) {
		bool matched = false;
		for (const auto & s2 : large->nameList) {
			if (s1.find(s2) != std::string::npos) {
				// Anything matching s1 can also be matched by s2.
				matched = true;
				break;
			}
		}
		if (!matched) return false; // This string can not be matched by the large list.
	}
	return true;
}

static bool ConditionSubset(const ConditionSocketGroup * small, const ConditionSocketGroup * large) {
	// True if the small condition needs fewer or equal sockets of every color.
	if (small->socketGroup.r > large->socketGroup.r) return false;
	if (small->socketGroup.g > large->socketGroup.g) return false;
	if (small->socketGroup.b > large->socketGroup.b) return false;
	if (small->socketGroup.w > large->socketGroup.w) return false;
	return true;
}

static bool ConditionSubset(const Condition * small, const Condition * large) {
	if (!small || !large) {
		throw InternalError("Attempting to test containment of a NULL condition!", __FILE__, __LINE__);
	}
	if (small->what != large->what) {
		throw InternalError("Attempting to test containment of conditions of different type!", __FILE__, __LINE__);
	}
	switch (small->conType) {
		case CON_INTERVAL:
			return ConditionSubset(
				static_cast<const ConditionInterval *>(small),
				static_cast<const ConditionInterval *>(large));
		case CON_BOOL:
			return ConditionSubset(
				static_cast<const ConditionBool *>(small),
				static_cast<const ConditionBool *>(large));
		case CON_NAMELIST:
			return ConditionSubset(
				static_cast<const ConditionNameList *>(small),
				static_cast<const ConditionNameList *>(large));
		case CON_SOCKETGROUP:
			return ConditionSubset(
				static_cast<const ConditionSocketGroup *>(small),
				static_cast<const ConditionSocketGroup *>(large));
		default:
			throw UnhandledCase("Condition type", __FILE__, __LINE__);
	}
}

/*
True if small is a subset of large.
I.e. anything that is matched by small is also matched by large.

*All* conditions of the same what of large must match every condition of small.
(Multiple conditions are interpreted as conjunction.)
*/
bool RuleSubset(const RuleNative * small, const RuleNative * large) {
	for (const auto ci : small->conditions) {
		const Condition * c = ci.second;
		
		auto range = large->conditions.equal_range(c->what);
		if (range.first == range.second) {
			// The large rule does not have this condition, i.e. matches everything.
			continue;
		}

		for (auto ci2 = range.first; ci2 != range.second; ci2++) {
			if (!ConditionSubset(c, ci2->second)) {
				return false;
			}
		}
	}
	// All conditions have been matched.
	
	for (const auto ci2 : large->conditions) {
		// Check that the large rule does not contain any extra conditions.
		if (small->conditions.find(ci2.first) == small->conditions.end()) {
			return false;
		}
	}
	
	return true;
}


/***********
* RULENATIVE
***********/

static bool ConditionUseless(const Condition * c) {
	switch (c->conType) {
		case CON_INTERVAL: {
			auto ci = static_cast<const ConditionInterval *>(c);
			return ci->from > ci->to;
		}
		case CON_SOCKETGROUP: {
			auto csg = static_cast<const ConditionSocketGroup *>(c);
			return
				csg->socketGroup.r
				+ csg->socketGroup.g
				+ csg->socketGroup.b
				+ csg->socketGroup.w
				> getLimit("LinkedSockets", MAX);
		}
		case CON_BOOL: // Always matches something.			
		case CON_NAMELIST: // We can not determine what this matches.
			return false;
		default:
			throw UnhandledCase("Condition type", __FILE__, __LINE__);
	}
}

/*
Always adds a clone of c, if necessary.
*/
void RuleNative::addCondition(const Condition * c) {
	auto cOld = conditions.find(c->what);
	if (cOld == conditions.end()) {
		// We do not have this type of condition yet.		
		
		// Add the condition and take ownership.
		conditions.insert(std::make_pair(c->what, static_cast<Condition *>(c->clone())));
		
		// Check if the condition matches anything.
		// We still add it though.
		if (ConditionUseless(c)) useless = true;
		return;
	}
	
	if (cOld->second->hasTag(TAG_FINAL)) {
		// Do not override final conditions.
		return;
	}

	// Override - replace the condition.
	if (c->hasTag(TAG_OVERRIDE)) {
		delete cOld->second;
		cOld->second = static_cast<Condition *>(c->clone());
		if (ConditionUseless(c)) useless = true;
		return;
	}
	
	// Intersect with the old condition
	switch (c->conType) {
		case CON_INTERVAL: {
			// Refine the interval in existing condition.
			auto c1 = static_cast<ConditionInterval *>(cOld->second);
			auto c2 = static_cast<const ConditionInterval *>(c);

			if (c1->from < c2->from) c1->from = c2->from;
			if (c1->to > c2->to) c1->to = c2->to;
			if (c1->from > c1->to) useless = true;
			break;
		}
		case CON_BOOL: {
			// A condition can not be true and false at the same time.
			auto c1 = static_cast<const ConditionBool *>(cOld->second);
			auto c2 = static_cast<const ConditionBool *>(c);

			if (c1->value != c2->value) useless = true;
			break;
		}
		case CON_SOCKETGROUP: {
			auto c1 = static_cast<const ConditionSocketGroup *>(c);
			int r = c1->socketGroup.r;
			int g = c1->socketGroup.g;
			int b = c1->socketGroup.b;
			int w = c1->socketGroup.w;

			bool add = true;
			auto range = conditions.equal_range(c->what);
			for (auto it = range.first; it != range.second; ) {
				// Check if we need more than 6 different sockets.
				auto c2 = static_cast<ConditionSocketGroup *>(it->second);
				if (c2->socketGroup.r > r) r = c2->socketGroup.r;
				if (c2->socketGroup.g > g) g = c2->socketGroup.g;
				if (c2->socketGroup.b > b) b = c2->socketGroup.b;
				if (c2->socketGroup.w > w) w = c2->socketGroup.w;

				// If some existing condition is stricter than the new condition, we do not need to do anything.
				if (ConditionSubset(c2, c1)) {
					add = false;
					break;
				}
				// If the new condition is stricter than some existing condition, we can remove the existing one.
				if (ConditionSubset(c1, c2)) {
					delete it->second;
					it = conditions.erase(it);
				} else {
					++it;
				}
			}

			// Check if we need more than 6 different sockets.
			if (r + g + b + w > getLimit("SocketGroup", MAX)) useless = true;

			// Add the new condition.
			if (add) {
				conditions.insert(std::make_pair(c->what, static_cast<Condition *>(c->clone())));
			}
			break;
		}
		case CON_NAMELIST: {
			// There can be more than one of these conditions, in case of intersections.
			auto c1 = static_cast<const ConditionNameList *>(c);

			bool add = true;
			auto range = conditions.equal_range(c->what);
			for (auto it = range.first; it != range.second; ) {
				auto c2 = static_cast<ConditionNameList *>(it->second);

				// If some existing condition is stricter than the new condition, we do not need to do anything.
				if (ConditionSubset(c2, c1)) {
					add = false;
					break;
				}
				// If the new condition is stricter than some existing condition, we can remove the existing one.
				if (ConditionSubset(c1, c2)) {
					delete it->second;
					it = conditions.erase(it);
				} else {
					++it;
				}
			}

			// Add the new condition.
			if (add) {
				conditions.insert(std::make_pair(c->what, static_cast<Condition *>(c->clone())));
			}
			break;
		}
		default:
			throw UnhandledCase("Condition type", __FILE__, __LINE__);
	}
}

/*
Adds a clone of the action.
If the action has the Override tag, replaces an existing action with the same name.
Otherwise the old action is preserved.
*/
void RuleNative::addAction(const Action * a) {
	// Possibly override other actions of the same type.
	auto it = actions.find(a->what);
	if (it != actions.end() && a->hasTag(TAG_OVERRIDE)) {
		delete it->second;
		it->second = static_cast<Action *>(a->clone());
	} else {
		actions.insert(std::make_pair(a->what, static_cast<Action *>(a->clone())));
	}
}

bool RuleNative::hasTag(TagList t) const {
	return tags & t;
}

std::ostream & RuleNative::printSelf(std::ostream & os) const {
	if (useless) {
		throw InternalError("Writing a useless rule to native filter!", __FILE__, __LINE__);
	}

	auto it = actions.find("Hidden");
	if (it != actions.end() && static_cast<ActionBool *>(it->second)->arg1) {
		os << "Hide" << std::endl;
	} else {
		os << "Show" << std::endl;
	}

	++IFPP_TABS;
	for (const auto & c : conditions) {
		print(os, c.second);
	}
	for (const auto & a : actions) {
		print(os, a.second);
	}
	--IFPP_TABS;
	os << std::endl;

	return os;
}

RuleNative * RuleNative::clone() const {
	// We do not need to do checking, just copy the conditions and actions.
	// Beware, this might break if addCondition or addAction start to do other things.
	RuleNative * r = new RuleNative(tags);
	r->useless = useless;

	for (const auto & c : conditions) {
		r->conditions.insert(std::make_pair(c.first, static_cast<Condition *>(c.second->clone())));
	}
	for (const auto & a : actions) {
		r->actions.insert(std::make_pair(a.first, static_cast<Action *>(a.second->clone())));
	}

	return r;
}

RuleNative::~RuleNative() {
	for (auto & c : conditions) delete c.second;
	for (auto & a : actions) delete a.second;
}
	
}