#include "Compiler.h"

namespace ifpp {
	
static RuleNative * ModifyRule(RuleNative * ruleOld, const Modifier * mod) {
	RuleNative * ruleNew = ruleOld->clone();
	for (const auto c : mod->commands) {
		switch (c->comType) {
			case COM_CONDITION:
				ruleNew->addCondition(static_cast<Condition *>(c));
				break;
				
			case COM_ACTION:
				ruleNew->addAction(static_cast<Action *>(c));
				break;
				
			case COM_RULE:
				throw InternalError("Rules inside modifiers are not implemented yet.", __FILE__, __LINE__);
				
			case COM_MODIFIER:
				throw InternalError("Nested modifiers are not implemented yet.", __FILE__, __LINE__);
				
			case COM_DEFAULT:
			case COM_IGNORE:
				throw InternalError("Deafult styles inside modifiers are not implemented yet.", __FILE__, __LINE__);
				
			default:
				throw UnhandledCase("Command type", __FILE__, __LINE__);					
		}
		if (ruleNew->useless) break;
	}
	return ruleNew;
}
	
/*
Modifies the entire input filter by adding the given modifier to all the rules.
*/
static void AddModifier(FilterNative & filter, const Modifier * mod) {
	FilterNative tempFilter;
	tempFilter.swap(filter);
	
	// Modify each rule by the modifier.
	for (auto ruleOld : tempFilter) {
		RuleNative * ruleNew = ModifyRule(ruleOld, mod);
		
		if (!ruleNew->useless) filter.push_back(ruleNew);
		else delete ruleNew;
		
		if (!ruleOld->useless) filter.push_back(ruleOld);
		else delete ruleOld;
	}
}

/*
Appends all rules from the second filter to the first. Clears the second filter.
Currently there is no optimization being done between different sections - this could be added?
*/
void AppendFilter(FilterNative & first, FilterNative & second) {
	first.reserve(first.size() + second.size());
	first.insert(first.end(), second.begin(), second.end());
}

/*
Compiles a single top-level IFPP rule and appends the native rules to a filter.
*/
void CompileRule(const RuleIFPP * rule, FilterNative & outFilter, const RuleNative * baseRule) {
	FilterNative tempFilter;
	RuleNative * base = baseRule->clone();
	
	for (const auto c : rule->commands) {
		switch (c->comType) {
			case COM_CONDITION:
				base->addCondition(static_cast<Condition *>(c));
				break;
				
			case COM_ACTION:
				base->addAction(static_cast<Action *>(c));
				break;
				
			case COM_RULE:
				CompileRule(static_cast<RuleIFPP *>(c), tempFilter, base);
				break;
				
			case COM_MODIFIER: {
				AddModifier(tempFilter, static_cast<Modifier *>(c));
				RuleNative * ruleNew = ModifyRule(base, static_cast<Modifier *>(c));
				if (!ruleNew->useless) tempFilter.push_back(ruleNew);
				else delete ruleNew;
				break;
			}
				
			case COM_DEFAULT:
				// TODO: This should only do stuff for non-intersecting conditions.
				for (const auto a : static_cast<DefaultStyle *>(c)->style) {
					base->addAction(a);
				}
				break;
				
			case COM_IGNORE:
				// Somewhat hacky but w/e.
				base->useless = true;
				break;
				
			default:
				throw UnhandledCase("Command type", __FILE__, __LINE__);
		}
	}
	
	if (!base->useless) {
		tempFilter.push_back(base);
	} else {
		delete base;
	}
	
	AppendFilter(outFilter, tempFilter);
}

/*
Compiles an IFPP filter into a native filter.
*/
void Compiler::Compile(const FilterIFPP & inFilter, FilterNative & outFilter) {
	outFilter.clear();
	RuleNative * blank = new RuleNative();

	for (auto statement : inFilter) {
		switch (statement->stmType) {
			case STM_DEFINITION:
				// Nothing to do, variables are handled in the parser.
				break;

			case STM_INSTRUCTION:
				// Version is handled in the parser. // TODO
				break;

			case STM_RULE:
				CompileRule(static_cast<RuleIFPP *>(statement), outFilter, blank);
				break;
			
			default:
				throw UnhandledCase("Statement type", __FILE__, __LINE__);
		}
	}
	
	delete blank;
}

}