#include "Types.h"

#include <stdexcept>
#include <cstring>
#include <iostream>

namespace ifpp {

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
	for (char s : sockets) {
		switch (s) {
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
	for (const auto & name : nl) {
		if (!first) os << ' ';
		first = false;
		os << '"' << name << '"';
	}
	return os;
}

std::ostream & print(std::ostream & os, TagList t) {
	if (t & TAG_OVERRIDE) os << "Override ";
	if (t & TAG_FINAL) os << "Final ";
	return os;
}


/***********
* ENUMS
***********/

std::ostream & operator<<(std::ostream & os, ExprType et) {
	switch (et) {
		case EXPR_NUMBER: return os << "Number";
		case EXPR_COLOR: return os << "Color";
		case EXPR_FILE: return os << "File";
		case EXPR_LIST: return os << "List";
		case EXPR_MACRO: return os << "Macro";
		case EXPR_UNDEFINED: return os << "Undefined";
		default: throw UnhandledCase("Expression type", __FILE__, __LINE__);
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

/*
Instructions
*/

std::ostream & Instruction::printSelf(std::ostream & os) const {
	return os << std::string(IFPP_TABS, '\t') << what;
}

/*
std::ostream & InstructionVersion::printSelf(std::ostream & os, PrintStyle ps) const {
	return Instruction::printSelf(os, ps) << ' ' << vMajor << '.' << vMinor << '.' << vPatch << std::endl;
}

InstructionVersion * InstructionVersion::clone() const {
	return new InstructionVersion(vMajor, vMinor, vPatch);
}
*/

std::ostream & DefinitionBase::printSelf(std::ostream & os) const {
	return Instruction::printSelf(os) << ' ' << varName << ' ' << varType;
}

/***********
* COMMANDS
***********/

std::ostream & Command::printSelf(std::ostream & os) const {
	return os << std::string(IFPP_TABS, '\t') << what;
}

/***********
* CONDITIONS
***********/

std::ostream & ConditionInterval::printSelf(std::ostream & os) const {
	if (what == "Rarity") {
		if (from > to) throw InternalError("Condition " + what + " has inverted range!", __FILE__, __LINE__);
		if (to < Normal || from > Unique) throw InternalError("Condition " + what + " does not match any value!", __FILE__, __LINE__);
		if (from == INT_MIN && to == INT_MAX) throw InternalError("Condition " + what + " matches all possible values!", __FILE__, __LINE__);
		if (from == INT_MIN) return Condition::printSelf(os) << " <= " << (Rarity)to << std::endl;
		if (to == INT_MAX) return Condition::printSelf(os) << " >= " << (Rarity)from << std::endl;
		if (from == to) return Condition::printSelf(os) << " = " << (Rarity)from << std::endl;

		Condition::printSelf(os) << " >= " << (Rarity)from << std::endl;
		Condition::printSelf(os) << " <= " << (Rarity)to << std::endl;
		return os;
	}
	else {
		if (from > to) throw InternalError("Condition " + what + " has inverted range!", __FILE__, __LINE__);
		if (to < 0) throw InternalError("Condition " + what + " does not match any value!", __FILE__, __LINE__);
		if (from == INT_MIN && to == INT_MAX) throw InternalError("Condition " + what + " matches all possible values!", __FILE__, __LINE__);
		if (from == INT_MIN) return Condition::printSelf(os) << " <= " << to << std::endl;
		if (to == INT_MAX) return Condition::printSelf(os) << " >= " << from << std::endl;
		if (from == to) return Condition::printSelf(os) << " = " << from << std::endl;

		Condition::printSelf(os) << " >= " << from << std::endl;
		Condition::printSelf(os) << " <= " << to << std::endl;
		return os;
	}
}

ConditionInterval * ConditionInterval::clone() const {
	return new ConditionInterval(what, from, to, tags);
}

std::ostream & ConditionBool::printSelf(std::ostream & os) const {
	return Condition::printSelf(os) << (value ? " true" : " false") << std::endl;
}

ConditionBool * ConditionBool::clone() const {
	return new ConditionBool(what, value, tags);
}

std::ostream & ConditionNameList::printSelf(std::ostream & os) const {
	return Condition::printSelf(os) << ' ' << nameList << std::endl;
}

ConditionNameList * ConditionNameList::clone() const {
	return new ConditionNameList(what, nameList, tags);
}

std::ostream & ConditionSocketGroup::printSelf(std::ostream & os) const {
	return Condition::printSelf(os) << ' ' << socketGroup << std::endl;
}

ConditionSocketGroup * ConditionSocketGroup::clone() const {
	return new ConditionSocketGroup(what, socketGroup, tags);
}




/***********
* ACTIONS
***********/

/*
Defined in header file due to template shenanigans.
*/

std::ostream & Block::printSelf(std::ostream & os) const {
	Command::printSelf(os) << " {" << std::endl;
	++IFPP_TABS;
	print(os, commands);
	--IFPP_TABS;
	return os << std::string(IFPP_TABS, '\t') << '}' << std::endl << std::endl;
}

Block * Block::clone() const {
	auto cb = new Block(blockType, Command::what, tags);
	cb->commands.reserve(commands.size());
	for (auto c : commands) cb->commands.push_back(c->clone());
	return cb;
}

Block::~Block() {
	for (auto c : commands) delete c;
}

std::ostream & Ignore::printSelf(std::ostream & os) const {
	return Command::printSelf(os);
}

Ignore * Ignore::clone() const {
	return new Ignore(tags);
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
		defaults["Volume"] = 300; // LOUDER
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
