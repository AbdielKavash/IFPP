#include "Compiler.h"

namespace ifpp {
	
/*
Appends all rules from the second filter to the first. Clears the second filter.
Currently there is no optimization being done between different sections - this could be added?
*/
static void AppendFilter(FilterNative & first, const FilterNative & second) {
	first.reserve(first.size() + second.size());
	first.insert(first.end(), second.begin(), second.end());
}

/*
TODO: crop the old rule by the conditions in modifier
to optimize situations where the old rule does not match anything after all mods.
*/
static RuleNative * ModifyRule(RuleNative * ruleOld, const RuleNative * modifier) {
	auto * ruleNew = ruleOld->clone();
	for (const auto c : modifier->conditions) {
		ruleNew->addCondition(c.second);
		if (ruleNew->useless) break;
	}
	
	if (!ruleNew->useless) {
		for (const auto a : modifier->actions) {
			ruleNew->addAction(a.second);
		}
		if (RuleSubset(ruleOld, ruleNew)) ruleOld->useless = true;
	}

	return ruleNew;
}

/*
Appends a rule modified by the modifier to the filter.
*/
static void ModifyRule(FilterNative & outFilter, RuleNative * ruleOld, const FilterNative & modifier) {
	for (const auto modOld : modifier) {
		const auto ruleNew = ModifyRule(ruleOld, modOld);
		
		if (!ruleNew->useless) outFilter.push_back(ruleNew);
		else delete ruleNew;
	}
}

/*
Modifies the first filter by the second (cartesian product).
Few optimizations are preformed right now, TODO.
*/
static void ModifyFilter(FilterNative & inFilter, const FilterNative & modifier, bool required) {
	if (inFilter.empty()) {
		AppendFilter(inFilter, modifier);
		return;
	}
	
	FilterNative outFilter;
	
	for (auto ruleOld : inFilter) {
		ModifyRule(outFilter, ruleOld, modifier);
		
		if (!required && !ruleOld->useless) outFilter.push_back(ruleOld);
		else delete ruleOld;
	}
	
	inFilter.swap(outFilter);
}

/*
Compiles a single top-level IFPP rule and appends the native rules to a filter.
*/
static void CompileBlock(FilterNative & outFilter, const Block * inBlock, const RuleNative * baseRule = NULL) {
	RuleNative * base = baseRule ? baseRule->clone() : new RuleNative();
	//bool hasConditions = false;
	bool hasDefault = true;
	
	for (const auto c : inBlock->commands) {
		switch (c->comType) {
			case COM_CONDITION:
				base->addCondition(static_cast<Condition *>(c));
				//hasConditions = true;
				break;

			case COM_ACTION:
				base->addAction(static_cast<Action *>(c));
				break;

			case COM_BLOCK: {
				const auto block = static_cast<Block *>(c);
				FilterNative blockFilter;
				CompileBlock(blockFilter, block, base);
				
				switch (block->blockType) {
					case BLOCK_RULE:
					case BLOCK_GROUP:
						AppendFilter(outFilter, blockFilter);
						break;
						
					case BLOCK_MODIFIER:
						ModifyFilter(outFilter, blockFilter, block->hasTag(TAG_REQUIRED));
						if (block->hasTag(TAG_REQUIRED)) hasDefault = false;
						break;

					case BLOCK_DEFAULT:
						AppendFilter(outFilter, blockFilter);
						hasDefault = false;
						break;

					default:
						throw UnhandledCase("Block type", __FILE__, __LINE__);
				}
				break;
			}

			default:
				throw UnhandledCase("Command type", __FILE__, __LINE__);
		}
	}

	if (inBlock->blockType == BLOCK_GROUP
		|| inBlock->hasTag(TAG_NODEFAULT)
		//|| (inBlock->hasTag(TAG_REQUIRED) && !hasConditions)
		|| !hasDefault
		|| base->useless
		|| base->actions.empty()) {
		// If any of the conditions above is true, skip the default rule.
		delete base;
	} else {
		outFilter.push_back(base);		
	}
}

/*
Compiles an IFPP filter into a native filter.
*/
void Compiler::Compile(FilterNative & outFilter, const FilterIFPP & inFilter) {
	for (const auto r : outFilter) delete r;
	outFilter.clear();

	for (const auto ins : inFilter) {
		switch (ins->insType) {
			case INS_DEFINITION:
				// Nothing to do, variables are handled in the parser.
				break;

			case INS_BLOCK: {
				const auto block = static_cast<Block *>(ins);
				switch (block->blockType) {
					case BLOCK_RULE:
					case BLOCK_GROUP: {
						FilterNative blockFilter;
						CompileBlock(blockFilter, block);
						AppendFilter(outFilter, blockFilter);
						break;
					}
						
					default:
						throw InternalError("Attempting to compile an invalid top-level block!", __FILE__, __LINE__);
				}
				break;
			}

			default:
				throw UnhandledCase("Instruction type", __FILE__, __LINE__);
		}
	}
}

}