#ifndef IFPP_TYPES_H
#define IFPP_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <ostream>
#include <climits>

namespace ifpp {

struct InternalError : public std::logic_error {
	const std::string file;
	int line;

	InternalError(const std::string & what, const std::string & f, int l) : std::logic_error(what), file(f), line(l) {}
};

struct UnhandledCase : public InternalError {
	UnhandledCase(const std::string & what, const std::string & f, int l) : InternalError("Unhandled case value of " + what, f, l) {}
};

int IFPP_GUID();

extern int IFPP_TABS;

enum PrintStyle { PRINT_IFPP, PRINT_NATIVE };

template<typename T>
std::ostream & print(std::ostream & os, PrintStyle ps, T * t) {
	if (t) {
		return t->printSelf(os, ps);
	} else {
		return os << std::string(IFPP_TABS, '\t') << "-NULL-" << std::endl;
	}
}

template<typename T>
std::ostream & print(std::ostream & os, PrintStyle ps, const T & t) {
	return t.printSelf(os, ps);
}

template<typename T>
std::ostream & print(std::ostream & os, PrintStyle ps, const std::vector<T> & v) {
	for (const auto & x : v) {
		print(os, ps, x);
	}
	return os;
}

/***********
* BASE TYPES
***********/

enum Rarity { Normal = 1, Magic = 2, Rare = 3, Unique = 4 };
std::ostream & operator<<(std::ostream & os, const Rarity & r);

struct Color {
	int r, g, b, a;
	Color() : r(0), g(0), b(0), a(255) {}
	Color(int R, int G, int B, int A = 255) : r(R), g(G), b(B), a(A) {}
	Color(const std::string & hexValue);
};
std::ostream & operator<<(std::ostream & os, const Color & r);

struct SocketGroup {
	int r, g, b, w;
	SocketGroup() : r(0), g(0), b(0), w(0) {}
	SocketGroup(const std::string & sockets);
};
std::ostream & operator<<(std::ostream & os, const SocketGroup & sg);

typedef std::vector<std::string> NameList;
std::ostream & operator<<(std::ostream & os, const NameList & nl);

typedef unsigned int ModifierList;
const unsigned int MOD_OVERRIDE = 	1 <<  0;
const unsigned int MOD_APPEND = 	1 <<  1;
const unsigned int MOD_FINAL = 		1 <<  2;
const unsigned int MOD_ADDONLY =	1 <<  3;
const unsigned int MOD_SHOW = 		1 << 10;
const unsigned int MOD_HIDE = 		1 << 11;
std::string modString(ModifierList ml);
std::ostream & print(std::ostream & os, PrintStyle ps, ModifierList ml);


/***********
* ENUMS
***********/

enum StatementType { STM_DEFINITION, STM_INSTRUCTION, STM_RULE };
enum CommandType { COM_CONDITION, COM_ACTION, COM_RULE, COM_DEFAULT };
enum ConditionType { CON_INTERVAL, CON_RARITY, CON_BOOL, CON_NAMELIST, CON_SOCKETGROUP };
//enum ActionType { AC_COLOR, AC_NUMBER, AC_SOUND, AC_FILENAME, AC_BOOLEAN, AC_STYLE };

enum VariableType { VAR_NUMBER, VAR_COLOR, VAR_FILE, VAR_LIST, VAR_STYLE, VAR_UNDEFINED };
std::ostream & operator<<(std::ostream & os, VariableType vt);

enum Operator { OP_LT, OP_LE, OP_EQ, OP_GE, OP_GT };
std::ostream & operator<<(std::ostream & os, Operator o);

struct Command {
	CommandType comType;
	std::string what;
	int guid;

	Command(CommandType t, const std::string & w) : comType(t), what(w), guid(IFPP_GUID()) {}
	virtual std::ostream & printSelf(std::ostream & os, PrintStyle ps) const;
	virtual Command * clone() const = 0;
	virtual ~Command() {}
};
typedef std::vector<Command *> CommandList;



/***********
* CONDITIONS
***********/

struct Condition : public Command {
	ConditionType conType;

	Condition(ConditionType t, const std::string & w) :
		Command(COM_CONDITION, w), conType(t) {}
	virtual std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
};

struct ConditionInterval : public Condition {
	int from, to;

	ConditionInterval(const std::string & w, int f, int t) :
		Condition(CON_INTERVAL, w), from(f), to(t) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	ConditionInterval * clone() const override;
};

struct ConditionBool : public Condition {
	bool value;

	ConditionBool(const std::string & w, bool v) :
		Condition(CON_BOOL, w), value(v) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	ConditionBool * clone() const override;
};

struct ConditionNameList : public Condition {
	NameList nameList;

	ConditionNameList(const std::string & w, const NameList & nl) :
		Condition(CON_NAMELIST, w), nameList(nl) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	ConditionNameList * clone() const override;
};

struct ConditionSocketGroup : public Condition {
	SocketGroup socketGroup;

	ConditionSocketGroup(const std::string & w, const SocketGroup & sg) :
		Condition(CON_SOCKETGROUP, w), socketGroup(sg) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	ConditionSocketGroup * clone() const override;
};



/***********
* ACTIONS
***********/

struct Action : public Command {
	ModifierList modifiers;

	Action(ModifierList m, const std::string & w) :
		Command(COM_ACTION, w), modifiers(m) {}
	virtual std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	virtual Action * clone() const override = 0;
	bool hasMod(ModifierList ml) const;
};
typedef std::vector<Action *> Style;
std::ostream & operator<<(std::ostream & os, const Style & s);

template<class T1>
struct Action1 : public Action {
	T1 arg1;

	Action1(ModifierList m, const std::string & w, const T1 & a1) :
		Action(m, w), arg1(a1) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override{
		return Action::printSelf(os, ps) << ' ' << arg1 << std::endl;
	}
	Action1<T1> * clone() const override {
		return new Action1<T1>(modifiers, what, arg1);
	}
};

template<>
struct Action1<bool> : public Action {
	bool arg1;

	Action1(ModifierList m, const std::string & w, const bool & a1) :
		Action(m, w), arg1(a1) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override {
		switch (ps) {
			case PRINT_IFPP:
				return Action::printSelf(os, ps) << (arg1 ? " true" : " false") << std::endl;
			case PRINT_NATIVE:
				// Do not print Hidden to native filters, this is handled in compiler.
				// However it is impractical to remove this action.
				if (what == "Hidden") return os;

				if (arg1) {
					// Print this action if it is true.
					return Action::printSelf(os, ps) << std::endl;
				} else {
					// If it is false do nothing.
					return os;
				}
			default:
				throw UnhandledCase("Print style", __FILE__, __LINE__);
		}
	}
	Action1<bool> * clone() const override {
		return new Action1<bool>(modifiers, what, arg1);
	}
};

template<class T1, class T2>
struct Action2 : public Action {
	T1 arg1;
	T2 arg2;

	Action2(ModifierList m, const std::string & w, const T1 & a1, const T2 & a2) :
		Action(m, w), arg1(a1), arg2(a2) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override {
		return Action::printSelf(os, ps) << ' ' << arg1 << ' ' << arg2 << std::endl;
	}
	Action2<T1, T2> * clone() const override {
		return new Action2<T1, T2>(modifiers, what, arg1, arg2);
	}
};

template<class T1, class T2, class T3>
struct Action3 : public Action {
	T1 arg1;
	T2 arg2;
	T3 arg3;

	Action3(ModifierList m, const std::string & w, const T1 & a1, const T2 & a2, const T3 & a3) :
		Action(m, w), arg1(a1), arg2(a2) , arg3(a3) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override {
		return Action::printSelf(os, ps) << ' ' << arg1 << ' ' << arg2 << ' ' << arg3 << std::endl;
	}
	Action3<T1, T2, T3> * clone() const override {
		return new Action3<T1, T2, T3>(modifiers, what, arg1, arg2, arg3);
	}
};

typedef Action1<int> ActionNumber;
typedef Action1<Color> ActionColor;
typedef Action2<std::string, int> ActionSound;
typedef Action1<bool> ActionBool;
typedef Action1<std::string> ActionFile;
typedef Action3<int, std::string, std::string> ActionMapIcon;
typedef Action2<std::string, std::string> ActionEffect;

struct DefaultStyle : public Command {
	Style style;
	
	DefaultStyle(const Style & s) : Command(COM_DEFAULT, "Default"), style(s) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	DefaultStyle * clone() const override;
};


/***********
* STATEMENTS
***********/

struct Statement {
	StatementType stmType;
	int guid;

	Statement(StatementType t) : stmType(t), guid(IFPP_GUID()) {}
	virtual std::ostream & printSelf(std::ostream & os, PrintStyle ps) const;
	virtual Statement * clone() const = 0;
	virtual ~Statement() {}
};
typedef std::vector<Statement *> FilterIFPP;
std::ostream & print(std::ostream & os, PrintStyle ps, const FilterIFPP & f);

struct Instruction : public Statement {
	std::string what;

	Instruction(const std::string & w) :
		Statement(STM_INSTRUCTION), what(w) {}
	virtual std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
};

struct InstructionFlush : public Instruction {
	InstructionFlush() : Instruction("Flush") {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	InstructionFlush * clone() const override;
};

struct InstructionVersion : public Instruction {
	int vMajor, vMinor, vPatch; // Minor and major are reserved by GCC?

	InstructionVersion(int M, int m, int p) :
		Instruction("Version"), vMajor(M), vMinor(m), vPatch(p) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	InstructionVersion * clone() const override;
};

struct DefinitionBase : public Statement {
	VariableType varType;
	std::string varName;

	DefinitionBase(const std::string & vn, VariableType vt) :
		Statement(STM_DEFINITION), varType(vt), varName(vn) {}
	virtual std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
};

template<class T>
struct Definition : public DefinitionBase {
	T value;

	Definition(const std::string & vn, VariableType vt, const T & v) :
		DefinitionBase(vn, vt), value(v) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override {
		return DefinitionBase::printSelf(os, ps) << ' ' << value << std::endl << std::endl;
	}
	Definition<T> * clone() const override {
		return new Definition<T>(varName, varType, value);
	}
};

template<>
struct Definition<Style> : public DefinitionBase {
	Style value;
	
	Definition(const std::string & vn, VariableType vt, const Style & v) :
		DefinitionBase(vn, vt), value(v) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override {
		return DefinitionBase::printSelf(os, ps) << " {" << std::endl << value << '}' << std::endl << std::endl;
	}
	Definition<Style> * clone() const override {
		return new Definition<Style>(varName, varType, value);
	}
	~Definition<Style>() override {
		for (const auto & a : value) delete a;
	}
};

struct RuleIFPP : public Statement, public Command {
	ModifierList modifiers;
	CommandList commands;
	
	RuleIFPP(ModifierList m) : Statement(STM_RULE), Command(COM_RULE, "Rule"), modifiers(m), commands() {}
	RuleIFPP(ModifierList m, const CommandList & c) : Statement(STM_RULE), Command(COM_RULE, "Rule"), modifiers(m), commands(c) {}
	bool hasMod(ModifierList ml) const;
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	RuleIFPP * clone() const override;
	~RuleIFPP() override;
};

enum WhichLimit { MIN, MAX, DEFAULT };
int getLimit(const std::string & what, WhichLimit which);

}

#endif