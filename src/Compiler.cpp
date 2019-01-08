#include "Compiler.h"
#include "RuleOperations.h"
#include <stdexcept>
#include <climits>
#include <stack>
#include <map>

namespace ifpp {

std::ostream & RuleNative::printSelf(std::ostream & os, PrintStyle ps) const {
	switch (ps) {
		case PRINT_NATIVE: {
			if (useless) {
				throw InternalError("Writing a useless rule to native filter!", __FILE__, __LINE__);
			}

			auto it = actions.find("Hidden");
			if (it != actions.end() && static_cast<ActionBool *>(it->second)->par1) {
				os << "Show" << std::endl;
			} else {
				os << "Hide" << std::endl;
			}
			break;
		}
		case PRINT_IFPP:
			os << '[' << guid << "] Rule ";
			if (useless) os << "USELESS ";
			print(os, ps, modifiers) << '{' << std::endl;
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

bool RuleNative::hasMod(ModifierList ml) const {
	return modifiers & ml;
}

RuleNative::~RuleNative() {
	for (auto & c : conditions) delete c.second;
	for (auto & a : actions) delete a.second;
}

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
Does NOT take modifiers into account. This should be handled outside.

TODO: possibly only change the parameters of an action when replacing it?
*/
void RuleNative::addAction(const Action * a) {
	// Possibly override other actions of the same type.
	auto it = actions.find(a->what);
	if (it != actions.end()) {
		delete it->second;
		it->second = static_cast<Action *>(a->clone());
	} else {
		actions.insert(std::make_pair(a->what, static_cast<Action *>(a->clone())));
	}
}

RuleNative * RuleNative::clone() const {
	// We do not need to do checking, just copy the conditions and actions.
	// Beware, this might break if addCondition or addAction start to do other things.
	RuleNative * r = new RuleNative(modifiers);
	r->useless = useless;

	for (const auto & c : conditions) {
		r->conditions.insert(std::make_pair(c.first, static_cast<Condition *>(c.second->clone())));
	}
	for (const auto & a : actions) {
		r->actions.insert(std::make_pair(a.first, static_cast<Action *>(a.second->clone())));
	}

	return r;
}

std::ostream & print(std::ostream & os, PrintStyle ps, const FilterNative & f) {
	for (size_t i = 0; i < f.size(); ++i) {
		if (i) os << std::endl;
		print(os, ps, f[i]);
	}
	return os;
}

/*
Adds an IFPP rule (with potential sub-rules) to a native filter.
This rule will be *intersected* with all rules already in the filter,
so we do not want to do this with top-level rules!
*/
static void AddRuleRecursive(FilterNative & filter, RuleNative * base, RuleIFPP * rule) {
	for (const auto & c : rule->commands) {
		switch (c->comType) {
			case COM_CONDITION:
				base.addCondition(static_cast<Condition *>(c));
				break;
			case COM_ACTION:
				base.addAction(static_cast<Action *>(c));
				break;
			case COM_RULE: {
				RuleNative b = base->clone();
				AddRuleRecursive(filter, b, static_cast<RuleIFPP *>(c));
				delete b;
				break;
			case COM_DEFAULT:
				// TODO: This should only do stuff for non-intersecting conditions.
				for (const auto & a : static_cast<DefaultStyle *>(c)->style) {
					base.addAction(a);
				}
				break;
			default:
				throw UnhandledCase("Command type", __FILE__, __LINE__);
		}
	}
	
	// TODO: WE ARE HERE. ADD THE RULE AND INTERSECT WITH ALL ACTIONS.
}

static void AddRule(FilterNative & filter, RuleIFPP * rule) {
	FilterNative temp;
	RuleNative r = new RuleNative(rule->modifiers);
	AddRuleRecursive(temp, r, rule);
	delete r;
	filter.insert(filter.end(), temp.begin(), temp.end();
}





/*
Processes an incremental rule.
The rule first is modified by the rule second:

Conditions of the same type are overwritten.
Conditions which are only in first are retained.
Conditions only in second are added.

Actions - same.

All conditions and actions in the second rule are deleted!
*/
static void IncrementRule(RuleNative * first, RuleNative * second) {
	for (const auto & c : second->conditions) {
		// Delete all matching conditions from first.
		// Caution, need to call destructors properly.
		const auto & r = first->conditions.equal_range(c.first);
		for (auto it = r.first; it != r.second; ++it) {
			delete it->second;
		}
		first->conditions.erase(r.first, r.second);
	}
	// We deleted some conditions, the rule is possibly not useless anymore.
	first->useless = false;

	// Add all conditions and actions.
	for (const auto & c : second->conditions) {
		// Any conditions of the same type have been deleted.
		first->addCondition(c.second);
		delete c.second;
	}
	for (const auto & a : second->actions) {
		// This overrides any actions of the same type.
		first->addAction(a.second);
		delete a.second;
	}

	// Clean up the second rule.
	second->conditions.clear();
	second->actions.clear();
}

/*
Processes a rule from input to get it ready for compilation.
This:
- TODO: splits incremental rules.
- Organizes conditions and actions into maps.
- Resolves duplicate conditions and actions.
-- Duplicate conditions are intersected.
-- Only the latter of duplicate actions is kept.
*/
void preprocessRule(const Rule * rule, std::vector<RuleNative *> & rules) {
	// Preprocess the rule into native rules.

	// Compatibility with native rules. Do not process incremental rules at all.
	if (rule->hasMod(MOD_SHOW) || rule->hasMod(MOD_HIDE)) {
		RuleNative * rn = new RuleNative(MOD_APPEND & MOD_FINAL);

		for (const auto & com : rule->commands) {
			switch (com->comType) {
				case COM_CONDITION:
					// This only removes conditions that are a subset/superset of one another.
					// We don't do intersections here.
					rn->addCondition(static_cast<const Condition *>(com));
					break;

				case COM_ACTION:
					rn->addAction(static_cast<const Action *>(com));
					break;

				default:
					throw InternalError("Unknown command type!", __FILE__, __LINE__);
			}
		}

		// Add action Hidden to native Hide rules.
		// This action should not be Override (?)
		if (rule->hasMod(MOD_HIDE)) rn->addAction(new ActionBool(0, "Hidden", true));

		if (!rn->useless) {
			rules.push_back(rn);
		} else {
			delete rn;
		}
	}
	else {
		// IFPP rule, possibly incremental.
		RuleNative * fullRule = new RuleNative(rule->modifiers);
		RuleNative * currentRule = new RuleNative(rule->modifiers);

		// Whether the last thing we have read was an action.
		bool lastAction = false;

		for (const auto & com : rule->commands) {
			switch (com->comType) {
				case COM_CONDITION:
					if (lastAction) {
						// We have a piece of an incremental rule.
						IncrementRule(fullRule, currentRule);
						if (!fullRule->useless) {
							rules.push_back(fullRule->clone());
						}
					}
					currentRule->addCondition(static_cast<const Condition *>(com));
					lastAction = false;
					break;

				case COM_ACTION:
					currentRule->addAction(static_cast<const Action *>(com));
					lastAction = true;
					break;

				default:
					throw InternalError("Unknown command type!", __FILE__, __LINE__);
			}
		}

		// Add the last incremental section.
		IncrementRule(fullRule, currentRule);
		if (!fullRule->useless) {
			rules.push_back(fullRule); // We do not need one more copy.
		} else {
			delete fullRule;
		}
		delete currentRule;
	}
}

/*
Appends all rules from the second filter to the first. Clears the second filter.
Currently there is no optimization being done between different sections - this could be added?
*/
void AppendFilter(FilterNative & first, FilterNative & second) {
	// first.reserve(first.size() + second.size()); // This is probably smart enough to not be needed?
	first.insert(first.end(), second.begin(), second.end());
}

/*
Compiles a single IFPP rule and appends the native rules to a filter.
*/
void Compiler::CompileRule(const RuleIFPP * rule, FilterNative & outFilter) {
}

/*
Compiles an IFPP filter into a native filter.
*/
void Compiler::Compile(const FilterIFPP & inFilter, FilterNative & outFilter) {
	outFilter.clear();

	FilterNative partFilter;

	for (auto statement : inFilter) {
		switch (statement->stmType) {
			case STM_DEFINITION:
				// Nothing to do, variables are handled in the parser.
				break;

			case STM_INSTRUCTION:
				// Version is handled in the parser. // TODO
				break;

			case STM_RULE: {
				CompileRule(static_cast<Rule *>(statement), outFilter);
				break;
			
			default:
				throw UnhandledCase("Statement type", __FILE__, __LINE__);
				
				/*
				rules.clear();
				preprocessRule(, rules);

				if (rules.size() == 0) {
					if (writePartial) {
						partialStream << "######################" << std::endl;
						partialStream << "Rule [" << section->guid << "] skipped, matches nothing" << std::endl;
						print(partialStream, PRINT_IFPP, section);
						partialStream << "###########" << std::endl << std::endl;
					}
				}
				else {
					if (writePartial) {
						partialStream << "######################" << std::endl;
						partialStream << "Adding rule [" << section->guid << ']' << std::endl;
						print(partialStream, PRINT_IFPP, section);

						if (rules.size() > 1) {
							partialStream << "###########" << std::endl;
							partialStream << "# Incremental rule, preprocessed into:" << std::endl;
							partialStream << "###########" << std::endl << std::endl;
							print(partialStream, PRINT_IFPP, rules);
						}
						partialStream << "###########" << std::endl << std::endl;
					}

					static FilterNative temp;
					temp.clear();
					temp.swap(partFilter);

					// TODO: comment this better.
					for (auto & ruleOld : temp) {
						for (auto & ruleNew : rules) {
							if (ruleNew) {
								RuleNative * top = RuleIntersection(ruleOld, ruleNew);

								if (!top) {
									// There is no intersection. The rules don't interact at all.
									// Save us some time by not having to compute differences.
									continue;
								}

								RuleNative * mid = RuleDifference(ruleOld, ruleNew);
								RuleNative * bot = RuleDifference(ruleNew, ruleOld);

								if (!ruleOld->hasMod(MOD_FINAL) && top != ruleOld) {
									// Add the rule for the intersection, since it is Overriding some parts of old.
									partFilter.push_back(top);

									// Modify the old rule.
									if (mid != ruleOld) {
										delete ruleOld;
										ruleOld = mid;
									}
								}

								if (bot != ruleNew) {
									// Modify the new rule.
									// Do this even if it is Final - Final only applies to following rules!
									delete ruleNew;
									ruleNew = bot;
								}

								if (!ruleOld) {
									// We have added all possible combinations with the new rule. We can stop here.
									break;
								}
							}
						}

						if (ruleOld) partFilter.push_back(ruleOld);
					}

					for (auto & rule : rules) {
						if (rule && !rule->hasMod(MOD_ADDONLY)) partFilter.push_back(rule);
					}

					if (writePartial) {
						print(partialStream, PRINT_IFPP, partFilter) << std::endl;
					}
				}
				break;
				
			}

			default:
				throw InternalError("Unknown section type!", __FILE__, __LINE__);
			*/
		}
	}

	// Attach the rest of the filter since the last Flush.
	AppendFilter(outFilter, partFilter);
}

}