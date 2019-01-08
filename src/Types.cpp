#include "Types.h"

#include <stdexcept>
#include <cstring>
#include <iostream>

namespace ifpp {

int IFPP_GUID() {
	static volatile int id = 0;
	return id++;
}

int IFPP_TABS = 0;

/***********
* BASE TYPES
***********/

std::ostream & operator<<(std::ostream & os, const Rarity & r) {
	switch (r) {
		case Normal: return os << "Normal";
		case Magic: return os << "Magic";
		case Rare: return os << "Rare";
		case Unique: return os << "Unique";
		default: throw UnhandledCase("Rarity", __FILE__, __LINE__);
	}
}

Color::Color(const std::string & hexValue) : r(0), g(0), b(0), a(0) {
	int x = strtol(hexValue.c_str(), NULL, 16);
	switch (hexValue.size()) {
		case 3: // rgb
			r = (x & 0xF00) >> 8; r |= r << 4;
			g = (x & 0x0F0) >> 4; g |= g << 4;
			b = x & 0x00F; b |= b << 4;
			a = 255;
			break;
		case 4: // rgba
			r = (x & 0xF000) >> 12; r |= r << 4;
			g = (x & 0x0F00) >> 8; g |= g << 4;
			b = (x & 0x00F0) >> 4; b |= b << 4;
			a = x & 0x000F; a |= a << 4;
			break;
		case 6: // rrggbb
			r = (x & 0xFF0000) >> 16;
			g = (x & 0x00FF00) >> 8;
			b = x & 0x0000FF;
			a = 255;
			break;
		case 8: // rrggbbaa
			r = (x & 0xFF000000) >> 24;
			g = (x & 0x00FF0000) >> 16;
			b = (x & 0x0000FF00) >> 8;
			a = x & 0x000000FF;
			break;
		default:
			throw InternalError("Invalid hex color value: " + hexValue, __FILE__, __LINE__);
	}
}

std::ostream & operator<<(std::ostream & os, const Color & c) {
	return os << c.r << ' ' << c.g << ' ' << c.b << ' ' << c.a;
}

SocketGroup::SocketGroup(const std::string & sockets) : r(0), g(0), b(0), w(0) {
	for (size_t i = 0; i < sockets.size(); ++i) {
		switch (sockets[i]) {
			case 'r': case 'R': ++r; break;
			case 'g': case 'G': ++g; break;
			case 'b': case 'B': ++b; break;
			case 'w': case 'W': ++w; break;
			default:
				throw InternalError("Invalid socket color!", __FILE__, __LINE__);
		}
	}
}

std::ostream & operator<<(std::ostream & os, const SocketGroup & sg) {
	return os
		<< std::string(sg.r, 'R')
		<< std::string(sg.g, 'G')
		<< std::string(sg.b, 'B')
		<< std::string(sg.w, 'W');
}

std::ostream & operator<<(std::ostream & os, const NameList & nl) {
	bool first = true;
	for (auto && name : nl) {
		if (!first) os << ' ';
		first = false;
		os << '"' << name << '"';
	}
	return os;
}

std::string modString(ModifierList ml) {
	if (ml == MOD_OVERRIDE) return "Override";
	if (ml == MOD_APPEND) return "Append";
	if (ml == MOD_FINAL) return "Final";
	if (ml == MOD_ADDONLY) return "Final";
	if (ml == MOD_SHOW) return "Show";
	if (ml == MOD_HIDE) return "Hide";
	throw InternalError("Attempting to convert unknown or compound modifier to string!", __FILE__, __LINE__);
}

std::ostream & print(std::ostream & os, PrintStyle ps, ModifierList ml) {
	switch (ps) {
		case PRINT_IFPP:
			if (ml & MOD_OVERRIDE) os << "Override ";
			if (ml & MOD_APPEND) os << "Append ";
			if (ml & MOD_FINAL) os << "Final ";
			if (ml & MOD_ADDONLY) os << "AddOnly ";
			if (ml & MOD_SHOW) os << "Show ";
			if (ml & MOD_HIDE) os << "Hide ";
			return os;
		case PRINT_NATIVE: throw InternalError("Attempting to write modifiers to native filter!", __FILE__, __LINE__);
		default: throw UnhandledCase("Print style", __FILE__, __LINE__);
	}
}



/***********
* ENUMS
***********/

std::ostream & operator<<(std::ostream & os, VariableType vt) {
	switch (vt) {
		case VAR_NUMBER: return os << "Number";
		case VAR_COLOR: return os << "Color";
		case VAR_FILE: return os << "File";
		case VAR_LIST: return os << "List";
		case VAR_STYLE: return os << "Style";
		case VAR_UNDEFINED: return os << "Undefined";
		default: throw UnhandledCase("Variable type", __FILE__, __LINE__);
	}
}

std::ostream & operator<<(std::ostream & os, Operator o) {
	switch (o) {
		case OP_LT: return os << '<';
		case OP_LE: return os << "<=";
		case OP_EQ: return os << '=';
		case OP_GE: return os << ">=";
		case OP_GT: return os << '>';
		default: throw UnhandledCase("Operator", __FILE__, __LINE__);
	}
}

std::ostream & Command::printSelf(std::ostream & os, PrintStyle ps) const {
	switch (ps) {
		case PRINT_IFPP: return os << std::string(IFPP_TABS, '\t') << '[' << guid << "] ";
		case PRINT_NATIVE: return os << std::string(IFPP_TABS, '\t');
		default: throw UnhandledCase("Print style", __FILE__, __LINE__);
	}
}

/***********
* CONDITIONS
***********/

std::ostream & Condition::printSelf(std::ostream & os, PrintStyle ps) const {
	return Command::printSelf(os, ps) << what;
}

std::ostream & ConditionInterval::printSelf(std::ostream & os, PrintStyle ps) const {
	if (what == "Rarity") {
		if (from > to) throw InternalError("Condition " + what + " has inverted range!", __FILE__, __LINE__);
		if (to < Normal || from > Unique) throw InternalError("Condition " + what + " does not match any value!", __FILE__, __LINE__);
		if (from == INT_MIN && to == INT_MAX) throw InternalError("Condition " + what + " matches all possible values!", __FILE__, __LINE__);
		if (from == INT_MIN) return Condition::printSelf(os, ps) << " <= " << (Rarity)to << std::endl;
		if (to == INT_MAX) return Condition::printSelf(os, ps) << " >= " << (Rarity)from << std::endl;
		if (from == to) return Condition::printSelf(os, ps) << " = " << (Rarity)from << std::endl;

		switch (ps) {
			case PRINT_IFPP:
				return Condition::printSelf(os, ps) << ' ' << (Rarity)from << " .. " << (Rarity)to << std::endl;
			case PRINT_NATIVE:
				Condition::printSelf(os, ps) << " >= " << (Rarity)from << std::endl;
				Condition::printSelf(os, ps) << " <= " << (Rarity)to << std::endl;
				return os;
			default:
				throw UnhandledCase("Print style", __FILE__, __LINE__);
		}
	}
	else {
		if (from > to) throw InternalError("Condition " + what + " has inverted range!", __FILE__, __LINE__);
		if (to < 0) throw InternalError("Condition " + what + " does not match any value!", __FILE__, __LINE__);
		if (from == INT_MIN && to == INT_MAX) throw InternalError("Condition " + what + " matches all possible values!", __FILE__, __LINE__);
		if (from == INT_MIN) return Condition::printSelf(os, ps) << " <= " << to << std::endl;
		if (to == INT_MAX) return Condition::printSelf(os, ps) << " >= " << from << std::endl;
		if (from == to) return Condition::printSelf(os, ps) << " = " << from << std::endl;
		
		switch (ps) {
			case PRINT_IFPP:
				return Condition::printSelf(os, ps) << ' ' << from << " .. " << to << std::endl;
			case PRINT_NATIVE:
				Condition::printSelf(os, ps) << " >= " << from << std::endl;
				Condition::printSelf(os, ps) << " <= " << to << std::endl;
				return os;
			default:
				throw UnhandledCase("Print style", __FILE__, __LINE__);
		}
	}
}

ConditionInterval * ConditionInterval::clone() const {
	return new ConditionInterval(what, from, to);
}

std::ostream & ConditionNameList::printSelf(std::ostream & os, PrintStyle ps) const {
	return Condition::printSelf(os, ps) << ' ' << nameList << std::endl;
}

ConditionNameList * ConditionNameList::clone() const {
	return new ConditionNameList(what, nameList);
}

std::ostream & ConditionSocketGroup::printSelf(std::ostream & os, PrintStyle ps) const {
	return Condition::printSelf(os, ps) << ' ' << socketGroup << std::endl;
}

ConditionSocketGroup * ConditionSocketGroup::clone() const {
	return new ConditionSocketGroup(what, socketGroup);
}

std::ostream & ConditionBool::printSelf(std::ostream & os, PrintStyle ps) const {
	return Condition::printSelf(os, ps) << (value ? " true" : " false") << std::endl;
}

ConditionBool * ConditionBool::clone() const {
	return new ConditionBool(what, value);
}



/***********
* ACTIONS
***********/

std::ostream & Action::printSelf(std::ostream & os, PrintStyle ps) const {
	Command::printSelf(os, ps);
	switch (ps) {
		case PRINT_IFPP: print(os, ps, modifiers); break;
		case PRINT_NATIVE: break;
		default: throw UnhandledCase("Print style", __FILE__, __LINE__);
	}
	return os << what;
}

bool Action::hasMod(ModifierList ml) const {
	return modifiers & ml;
}

std::ostream & operator<<(std::ostream & os, const Style & s) {
	for (const auto & a : s) {
		a->printSelf(os, PRINT_IFPP);
	}
	return os;
}

std::ostream & DefaultStyle::printSelf(std::ostream & os, PrintStyle ps) const {
	switch (ps) { 
		case PRINT_IFPP:
			Command::printSelf(os, ps);			
			os << what << '{' << std::endl;
			++IFPP_TABS;
			print(os, ps, style);
			--IFPP_TABS;
			os << std::string(IFPP_TABS, '\t') << '}' << std::endl;
			return os;
		case PRINT_NATIVE:
			throw InternalError("Attempting to print a Default action to native filter!", __FILE__, __LINE__);
		default:
			throw UnhandledCase("Print style", __FILE__, __LINE__);
	}
}

DefaultStyle * DefaultStyle::clone() const {
	Style s;
	for (const auto & a : style) s.push_back(a->clone());
	return new DefaultStyle(s);
}


/***********
* STATEMENTS
***********/

std::ostream & Statement::printSelf(std::ostream & os, PrintStyle ps) const {
	switch (ps) {
		case PRINT_IFPP: return os << std::string(IFPP_TABS, '\t') << "[" << guid << "] ";
		case PRINT_NATIVE: return os << std::string(IFPP_TABS, '\t');
		default: throw UnhandledCase("Print style", __FILE__, __LINE__);
	}
}

std::ostream & print(std::ostream & os, PrintStyle ps, const FilterIFPP & f) {
	bool first = true;
	for (const auto & stm : f) {
		if (!first) os << std::endl;
		first = false;
		print(os, ps, stm);
	}
	return os;
}

std::ostream & Instruction::printSelf(std::ostream & os, PrintStyle ps) const {
	switch (ps) {
		case PRINT_IFPP: return Statement::printSelf(os, ps) << what;
		case PRINT_NATIVE: throw InternalError("Attempting to write IFPP-only instruction to native filter!", __FILE__, __LINE__);
		default: throw UnhandledCase("Print style", __FILE__, __LINE__);
	}
}

std::ostream & InstructionFlush::printSelf(std::ostream & os, PrintStyle ps) const {
	return Instruction::printSelf(os, ps) << std::endl;
}

InstructionFlush * InstructionFlush::clone() const {
	return new InstructionFlush();
}

std::ostream & InstructionVersion::printSelf(std::ostream & os, PrintStyle ps) const {
	return Instruction::printSelf(os, ps) << ' ' << vMajor << '.' << vMinor << '.' << vPatch << std::endl;
}

InstructionVersion * InstructionVersion::clone() const {
	return new InstructionVersion(vMajor, vMinor, vPatch);
}


std::ostream & DefinitionBase::printSelf(std::ostream & os, PrintStyle ps) const {
	switch (ps) {
		case PRINT_IFPP: return Statement::printSelf(os, ps) << "Define " << varName << ' ' << varType;
		case PRINT_NATIVE: throw InternalError("Attempting to write a variable definition to native filter!", __FILE__, __LINE__);
		default: throw UnhandledCase("Print style", __FILE__, __LINE__);
	}
}

std::ostream & RuleIFPP::printSelf(std::ostream & os, PrintStyle ps) const {
	switch (ps) {
		case PRINT_IFPP:
			Statement::printSelf(os, ps) << "Rule ";
			print(os, ps, modifiers) << '{' << std::endl;
			++IFPP_TABS;
			print(os, ps, commands);
			--IFPP_TABS;
			os << std::string(IFPP_TABS, '\t') << '}' << std::endl;
			return os;
		case PRINT_NATIVE: throw InternalError("Attempting to write an IFPP rule to native filter!", __FILE__, __LINE__);
		default: throw UnhandledCase("Print style", __FILE__, __LINE__);
	}
}

RuleIFPP * RuleIFPP::clone() const {
	RuleIFPP * r = new RuleIFPP(modifiers);
	r->commands.reserve(commands.size());
	for (const auto & c : commands) {
		r->commands.push_back(c->clone());
	}
	return r;
}

bool RuleIFPP::hasMod(ModifierList ml) const {
	return modifiers & ml;
}

RuleIFPP::~RuleIFPP() {
	for (const auto & c : commands) delete c;
}

int getLimit(const std::string & what, WhichLimit which) {
	static std::map<std::string, std::pair<int, int> > limits;
	static std::map<std::string, int> defaults;
	static bool first = true;
	if (first) {
		first = false;
		limits["ItemLevel"] = std::make_pair(1, 100);
		limits["DropLevel"] = std::make_pair(1, 100);
		limits["Quality"] = std::make_pair(0, 30);
		limits["Sockets"] = std::make_pair(0, 6); // Kaom's stuff has 0 sockets
		limits["LinkedSockets"] = std::make_pair(0, 6); // Kaom's stuff has 0 sockets
		limits["Height"] = std::make_pair(1, 4);
		limits["Width"] = std::make_pair(1, 2);
		limits["StackSize"] = std::make_pair(1, 1000); // Perandus Coins?
		limits["GemLevel"] = std::make_pair(1, 21); // Don't think you can go over this.
		limits["Rarity"] = std::make_pair(1, 4); // Normal, Magic, Rare, Unique
		limits["MapTier"] = std::make_pair(1, 16); // Shaper's Realm (T17) is not a map?

		limits["SetFontSize"] = std::make_pair(17, 45); // https://www.pathofexile.com/forum/view-thread/2199068
		limits["Color"] = std::make_pair(0, 255);
		limits["Volume"] = std::make_pair(0, 300);		
		limits["MinimapIcon"] = std::make_pair(0, 2);		

		defaults["Color"] = 255;
		defaults["Volume"] = 100; // ???
		defaults["FontSize"] = 33;
	}
	try {
		switch (which) {
			case MIN: return limits.at(what).first;
			case MAX: return limits.at(what).second;
			case DEFAULT: return defaults.at(what);
			default: throw UnhandledCase("Limit type", __FILE__, __LINE__);
		}
	} catch (std::exception & e) {
		throw InternalError("Requesting unknown limit value of " + what + "!", __FILE__, __LINE__);
	}
}

}
