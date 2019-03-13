#include "RuleNative.h"

namespace ifpp {
	
/***********
* TESTING CONTAINMENT OF CONDITIONS
***********/
	
static bool MatchedBy(const std::string & s1, const std::string & s2) {
	return s1.find(s2) != std::string::npos;
}

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static bool ConditionSubset(const Condition * first, const Condition * second) {
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
			throw UnhandledCase("Condition type", __FILE__, __LINE__);
	}
}
#pragma GCC diagnostic pop



/***********
* RULENATIVE
***********/

/*
Always adds a clone of c, if necessary.
*/
void RuleNative::addCondition(const Condition * c) {
	auto cOld = conditions.find(c->what);
	if (cOld == conditions.end()) {
		// We do not have this type of condition yet.
		// Check if the condition matches anything.
		// We still add it though.
		switch (c->conType) {
			case CON_INTERVAL: {
				auto ci = static_cast<const ConditionInterval *>(c);
				if (ci->from > ci->to) useless = true;
				break;
			}
			case CON_SOCKETGROUP: {
				auto csg = static_cast<const ConditionSocketGroup *>(c);
				if (csg->socketGroup.r + csg->socketGroup.g + csg->socketGroup.b + csg->socketGroup.w > getLimit("LinkedSockets", MAX)) {
					useless = true;
				}
				break;
			}
			case CON_BOOL: // Always matches something.
				break;
			case CON_NAMELIST: // We can not determine what this matches.
				break;
			default:
				throw UnhandledCase("Condition type", __FILE__, __LINE__);
		}
		// Add the condition and take ownership.
		conditions.insert(std::make_pair(c->what, static_cast<Condition *>(c->clone())));
		return;
	}

	// A condition of this type already exists. Intersect with it.
	
	// Override - replace the condition.
	if (c->hasTag(TAG_OVERRIDE)) {
		delete cOld->second;
		cOld->second = static_cast<Condition *>(c->clone());
		return;
	}
	
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

std::ostream & RuleNative::printSelf(std::ostream & os, PrintStyle ps) const {
	switch (ps) {
		case PRINT_NATIVE: {
			if (useless) {
				throw InternalError("Writing a useless rule to native filter!", __FILE__, __LINE__);
			}

			auto it = actions.find("Hidden");
			if (it != actions.end() && static_cast<ActionBool *>(it->second)->arg1) {
				os << "Hide" << std::endl;
			} else {
				os << "Show" << std::endl;
			}
			break;
		}
		case PRINT_IFPP:
			os << '[' << guid << "] Rule ";
			if (useless) os << "USELESS ";
			print(os, ps, tags) << '{' << std::endl;
			break;

		default:
			throw UnhandledCase("Print style", __FILE__, __LINE__);
	}

	++IFPP_TABS;
	for (const auto & c : conditions) {
		print(os, ps, c.second);
	}
	for (const auto & a : actions) {
		print(os, ps, a.second);
	}
	--IFPP_TABS;

	if (ps == PRINT_IFPP) {
		os << '}' << std::endl;
	}
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

std::ostream & print(std::ostream & os, PrintStyle ps, const FilterNative & f) {
	for (size_t i = 0; i < f.size(); ++i) {
		if (i) os << std::endl;
		print(os, ps, f[i]);
	}
	return os;
}
	
}