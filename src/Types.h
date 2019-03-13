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



/***********
* ENUMS
***********/

enum StatementType { STM_DEFINITION, STM_INSTRUCTION, STM_RULE };
enum CommandType { COM_CONDITION, COM_ACTION, COM_RULE, COM_MODIFIER, COM_DEFAULT, COM_IGNORE };
enum ConditionType { CON_INTERVAL, CON_RARITY, CON_BOOL, CON_NAMELIST, CON_SOCKETGROUP };
enum ActionType { AC_NUMBER, AC_COLOR, AC_FILE, AC_SOUND, AC_BOOLEAN, AC_STYLE };
enum ExprType { EXPR_NUMBER, EXPR_COLOR, EXPR_FILE, EXPR_LIST, EXPR_STYLE, EXPR_ERROR, EXPR_UNDEFINED };

enum Operator { OP_LT, OP_LE, OP_EQ, OP_GE, OP_GT };
std::ostream & operator<<(std::ostream & os, Operator o);

typedef unsigned int TagList;
const unsigned int TAG_OVERRIDE = 	1 <<  0;
/*const unsigned int MOD_APPEND = 	1 <<  1;
const unsigned int MOD_FINAL = 		1 <<  2;
const unsigned int MOD_ADDONLY =	1 <<  3;
const unsigned int MOD_SHOW = 		1 << 10;
const unsigned int MOD_HIDE = 		1 << 11;*/
std::ostream & print(std::ostream & os, PrintStyle ps, TagList t);

/*
Statements - top level commands.
This can be:
- Instruction
	- Version
	- TODO: conditional compilation
- Definition
- Rule
- Modifier
*/

struct Statement {
	int guid;	
	StatementType stmType;	

	Statement(StatementType t) : guid(IFPP_GUID()), stmType(t) {}
	virtual std::ostream & printSelf(std::ostream & os, PrintStyle ps) const;
	virtual Statement * clone() const = 0;
	virtual ~Statement() {}
};
typedef std::vector<Statement *> FilterIFPP;
std::ostream & print(std::ostream & os, PrintStyle ps, const FilterIFPP & f);

struct Instruction : public Statement {
	std::string what;

	Instruction(const std::string & w) : Statement(STM_INSTRUCTION), what(w) {}
	virtual std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
};

struct InstructionVersion : public Instruction {
	int vMajor, vMinor, vPatch; // Minor and major are reserved by GCC?

	InstructionVersion(int M, int m, int p) :
		Instruction("Version"), vMajor(M), vMinor(m), vPatch(p) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	InstructionVersion * clone() const override;
};

/*
struct Definition : public Statement {
	std::string varName;
	ExprType varType;
	Expression value;	

	Definition(const std::string & vn, ExprType vt, const Expression * value);
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
};
*/

struct DefinitionBase : public Statement {
	std::string varName;
	ExprType varType;
	DefinitionBase(const std::string & vn, ExprType et) :
		Statement(STM_DEFINITION), varName(vn), varType(et) {}
	virtual std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
};

template<class T>
struct Definition : public DefinitionBase {
	T value;

	Definition(const std::string & vn, ExprType et, const T & v) :
		DefinitionBase(vn, et), value(v) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override {
		return DefinitionBase::printSelf(os, ps) << ' ' << value << std::endl << std::endl;
	}
	Definition<T> * clone() const override {
		return new Definition<T>(varName, varType, value);
	}
};

// DefinitionStyle - see below after Actions


/*
Commands - parts of a rule.
This can be:
- Condition
- Action
- Rule
- Modifier
*/

struct Command {
	int guid;	
	CommandType comType;	
	std::string what;
	TagList tags;
	
	Command(CommandType ct, const std::string & w, TagList t = 0) :
		guid(IFPP_GUID()), comType(ct), what(w), tags(t) {}
	bool hasTag(TagList t) const { return tags & t; }
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

	Condition(ConditionType ct, const std::string & w, TagList t = 0) :
		Command(COM_CONDITION, w, t), conType(ct) {}
	virtual std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
};

struct ConditionInterval : public Condition {
	int from, to;

	ConditionInterval(const std::string & w, int f, int T, TagList t = 0) :
		Condition(CON_INTERVAL, w, t), from(f), to(T) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	ConditionInterval * clone() const override;
};

struct ConditionBool : public Condition {
	bool value;

	ConditionBool(const std::string & w, bool v, TagList t = 0) :
		Condition(CON_BOOL, w, t), value(v) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	ConditionBool * clone() const override;
};

struct ConditionNameList : public Condition {
	NameList nameList;

	ConditionNameList(const std::string & w, const NameList & nl, TagList t = 0) :
		Condition(CON_NAMELIST, w, t), nameList(nl) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	ConditionNameList * clone() const override;
};

struct ConditionSocketGroup : public Condition {
	SocketGroup socketGroup;

	ConditionSocketGroup(const std::string & w, const SocketGroup & sg, TagList t = 0) :
		Condition(CON_SOCKETGROUP, w, t), socketGroup(sg) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	ConditionSocketGroup * clone() const override;
};



/***********
* ACTIONS
***********/

struct Action : public Command {
	Action(const std::string & w, TagList t = 0) :
		Command(COM_ACTION, w, t) {}
	virtual std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	virtual Action * clone() const override = 0;
};
typedef std::vector<Action *> Style;
std::ostream & operator<<(std::ostream & os, const Style & s);

template<>
struct Definition<Style> : public DefinitionBase {
	Style value;
	
	Definition(const std::string & vn, ExprType et, const Style & v) :
		DefinitionBase(vn, et), value(v) {}
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

template<class T1>
struct Action1 : public Action {
	T1 arg1;

	Action1(const std::string & w, const T1 & a1, TagList t = 0) :
		Action(w, t), arg1(a1) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override{
		return Action::printSelf(os, ps) << ' ' << arg1 << std::endl;
	}
	Action1<T1> * clone() const override {
		return new Action1<T1>(what, arg1, tags);
	}
};

template<>
struct Action1<bool> : public Action {
	bool arg1;

	Action1(const std::string & w, const bool & a1, TagList t = 0) :
		Action(w, t), arg1(a1) {}
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
		return new Action1<bool>(what, arg1, tags);
	}
};

template<class T1, class T2>
struct Action2 : public Action {
	T1 arg1;
	T2 arg2;

	Action2(const std::string & w, const T1 & a1, const T2 & a2, TagList t = 0) :
		Action(w, t), arg1(a1), arg2(a2) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override {
		return Action::printSelf(os, ps) << ' ' << arg1 << ' ' << arg2 << std::endl;
	}
	Action2<T1, T2> * clone() const override {
		return new Action2<T1, T2>(what, arg1, arg2, tags);
	}
};

template<class T1, class T2, class T3>
struct Action3 : public Action {
	T1 arg1;
	T2 arg2;
	T3 arg3;

	Action3(const std::string & w, const T1 & a1, const T2 & a2, const T3 & a3, TagList t = 0) :
		Action(w, t), arg1(a1), arg2(a2) , arg3(a3) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override {
		return Action::printSelf(os, ps) << ' ' << arg1 << ' ' << arg2 << ' ' << arg3 << std::endl;
	}
	Action3<T1, T2, T3> * clone() const override {
		return new Action3<T1, T2, T3>(what, arg1, arg2, arg3, tags);
	}
};

typedef Action1<int> ActionNumber;
typedef Action1<Color> ActionColor;
typedef Action1<bool> ActionBool;
typedef Action1<std::string> ActionFile;
typedef Action2<std::string, int> ActionSound;
typedef Action2<std::string, std::string> ActionEffect;
typedef Action3<int, std::string, std::string> ActionMapIcon;

struct Ignore : public Command {
	Ignore(TagList t = 0) : Command(COM_IGNORE, "DefaultIgnore", t) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	Ignore * clone() const override;
};


/***********
* RULES
***********/

struct RuleIFPP : public Statement, public Command {
	CommandList commands;
	
	RuleIFPP(TagList t = 0) : Statement(STM_RULE), Command(COM_RULE, "Rule", t), commands() {}
	RuleIFPP(const CommandList & c, TagList t = 0) : Statement(STM_RULE), Command(COM_RULE, "Rule", t), commands(c) {}	
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	RuleIFPP * clone() const override;
	~RuleIFPP() override;
};

struct Modifier : public Command {	
	CommandList commands;
	
	Modifier(TagList t = 0) : Command(COM_MODIFIER, "Modifier", t), commands() {}
	Modifier(const CommandList & c, TagList t = 0) : Command(COM_MODIFIER, "Modifier", t), commands(c) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	Modifier * clone() const override;
	~Modifier() override;
};

struct DefaultStyle : public Command {
	Style style;
	
	DefaultStyle(TagList t = 0) : Command(COM_DEFAULT, "Default", t), style() {}
	DefaultStyle(const Style & s, TagList t = 0) : Command(COM_DEFAULT, "Default", t), style(s) {}
	std::ostream & printSelf(std::ostream & os, PrintStyle ps) const override;
	DefaultStyle * clone() const override;
	~DefaultStyle() override;
};



enum WhichLimit { MIN, MAX, DEFAULT };
int getLimit(const std::string & what, WhichLimit which);

}

#endif