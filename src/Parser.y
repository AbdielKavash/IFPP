%skeleton "lalr1.cc"
%require "3.0.4"

%define parser_class_name { Parser }
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%define parse.trace
%define parse.error verbose

%code requires {
	#include <string>
	#include <sstream>
	#include <climits>

	#include "Types.h"
	#include "Context.h"
}

%param { ifpp::Context & ctx }

%locations

%initial-action {
	// Initialize the initial location.
	@$.begin.filename = @$.end.filename = &ctx.file;
};

%code {
	#define YY_DECL yy::Parser::symbol_type yylex(ifpp::Context & ctx)
	YY_DECL;

	void yy::Parser::error(const location_type & l, const std::string & m) {
		ctx.criticalAt(l) << "Parser returned an error: " << m << std::endl;
	}

	static bool clampValue(ifpp::Context & ctx, const yy::location & l, const std::string & what, int & value, int min, int max) {
		int replace = value;
		if (value < min) replace = min;
		if (value > max) replace = max;

		if (replace != value) {
			ctx.warningAt(l) << "Value " << value << " is outside of allowed range for " << what << ": [" << min << ", " << max << "]. "
				<< " The value was replaced by " << replace << "." << std::endl;
			value = replace;
			return false;
		}
		return true;
	}

	static bool clampInterval(ifpp::Context & ctx, const yy::location & l, const std::string & what, int from, int to, int min, int max) {
		if (from > to) {
			ctx.warningAt(l) << "The range of condition " << what << " is empty. "
				<< "The condition will not match any items." << std::endl;
			return false;
		}
		if (from > max || to < min) {
			ctx.warningAt(l) << "Condition " << what << " does not match any possible values from the interval [" << min << ", " << max << "]. "
				<< "The condition will not match any items." << std::endl;
			return false;
		}
		if (from <= min && to >= max) {
			ctx.warningAt(l) << "Condition " << what << " matches all possible values from the interval [" << min << ", " << max << "]. "
				<< "The condition will match all items." << std::endl;
			return false;
		}
		return true;
	}

	static bool checkVarUse(ifpp::Context & ctx, const yy::location & l, const std::string & name, ifpp::ExprType type) {
		ifpp::ExprType oldType = ctx.getVarType(name);
		if (oldType == ifpp::EXPR_UNDEFINED) {
			if (type == ifpp::EXPR_LIST) {
				ctx.errorAt(l) << "Variable " << name << " has not been defined. "
					<< "It will be ignored in this list." << std::endl;
			} else {
				ctx.errorAt(l) << "Variable " << name << " has not been defined. "
					<< "This command will be ignored." << std::endl;
			}
			return false;
		} else if (oldType != type) {
			if (type == ifpp::EXPR_LIST) {
				ctx.errorAt(l) << "Variable " << name << " is of type " << oldType << ", but this command requires " << type << ". "
					<< "The variable will be ignored in this list." << std::endl;
			} else {
				ctx.errorAt(l) << "Variable " << name << " is of type " << oldType << ", but this command requires " << type << ". "
					<< "This command will be ignored." << std::endl;
			}
			return false;
		}
		return true;
	}

	template <typename T>
	static ifpp::DefinitionBase * magicDefinition(ifpp::Context & ctx, const yy::location & l,
		const std::string & name, ifpp::ExprType type, const T & value) {

		ifpp::ExprType oldType = ctx.getVarType(name);
		if (oldType != ifpp::EXPR_UNDEFINED) {
			ctx.errorAt(l) << "Variable " << name << " has already been defined!";
			return NULL;
		}

		ctx.defineVariable(name, type, value);
		return new ifpp::Definition<T>(name, type, value);
	}

	static ifpp::ConditionInterval * magicInterval(ifpp::Context & ctx, const yy::location & l,
		const std::string & what, int from, int to, ifpp::TagList tags) {

		clampInterval(ctx, l, what, from, to, ifpp::getLimit(what, ifpp::MIN), ifpp::getLimit(what, ifpp::MAX));
		return new ifpp::ConditionInterval(what, from, to, tags);
	}

	static ifpp::ConditionInterval * magicInterval(ifpp::Context & ctx, const yy::location & l,
		const std::string & what, ifpp::Operator op, int value, ifpp::TagList tags) {

		int from = INT_MIN, to = INT_MAX;
		switch(op) {
			case ifpp::OP_LT: to = value - 1; break;
			case ifpp::OP_LE: to = value; break;
			case ifpp::OP_EQ: from = to = value; break;
			case ifpp::OP_GE: from = value; break;
			case ifpp::OP_GT: from = value + 1; break;
			default: throw ifpp::UnhandledCase("Operator", __FILE__, __LINE__);
		}
		return magicInterval(ctx, l, what, from, to, tags);
	}

	template <typename T> static void listNew(std::vector<T> & l) {
		l.clear();
	}
	
	template <typename T, typename E> static void listNew(std::vector<T> & l, E * e) {
		l.clear();
		if (e) l.push_back(e);
	}
	
	template <typename T, typename E> static void listNew(std::vector<T> & l, const E & e) {
		l.clear();
		l.push_back(e);
	}

	template <typename T> static void listCopy(std::vector<T> & l1, std::vector<T> & l2) {
		l1.swap(l2);
	}

	template <typename T, typename E> static void listAppend(std::vector<T> & l1, std::vector<T> & l2, E * e) {
		l1.swap(l2);
		if (e) l1.push_back(e);
	}

	template <typename T, typename E> static void listAppend(std::vector<T> & l1, std::vector<T> & l2, const E & e) {
		l1.swap(l2);
		l1.push_back(e);
	}

	template <typename T, typename E> static void listMerge(std::vector<T> & l1, std::vector<T> & l2, const std::vector<E> & l3) {
		l1.swap(l2);
		l1.insert(l1.end(), l3.begin(), l3.end());
	}
}

%token <std::string>
	NEWLINE "end of line"

	CHR_LEFTBRACKET "{"
	CHR_RIGHTBRACKET "}"
	CHR_PERIOD "."
	CHR_DOTDOT ".."
	CHR_COLON ":"

	KW_DEFINE "Define"
	
	KW_RULE "Rule"
	KW_CONDITIONGROUP "ConditionGroup"
	KW_MODIFIER "Modifier"
	KW_GROUP "Group"	
	KW_DEFAULT "Default"
	
	CON_NUMBER "Condition (Number)"
	CON_RARITY "Condition (Rarity)"
	CON_LIST "Condition (List)"
	CON_BOOL "Condition (Boolean)"
	CON_SOCKETGROUP "Condition (Socket group)"
	
	AC_NUMBER "Action (Number)"
	AC_COLOR "Action (Color)"
	AC_SOUND "Action (Sound)"
	AC_BOOL "Action (Boolean)"
	AC_CUSTOMSOUND "Action (Custom sound)"
	AC_MINIMAPICON "Action (Minimap icon)"
	AC_PLAYEFFECT "Action (Play effect)"
	AC_REMOVE "Action (Remove)"

	AC_USEMACRO "Action (Use macro)"

	CONST_SOUND "sound"
	CONST_COLOR "color"
	CONST_SHAPE "shape"
	CONST_TEMP "Temp"

	VARIABLE "variable"
	HEX "hex number"
	FILENAME "file name"

	NAME "name"
;

%token <int>
	NUMBER "number"
	CONST_RARITY "rarity"
;

%token <ifpp::ExprType>
	TYPE_NUMBER "Number"
	TYPE_COLOR "Color"
	TYPE_FILE "File"
	TYPE_LIST "List"
	TYPE_MACRO "Macro"
;

%token <ifpp::Operator> OPERATOR "operator"

%token <ifpp::SocketGroup> SOCKETGROUP "socket group"

%token <ifpp::TagList> TAG "tag"

%token <bool> CONST_BOOL "boolean value"

%token END 0 "end of file"



%type <ifpp::DefinitionBase *> definition
%type <ifpp::Condition *> condition
%type <ifpp::Action *> action

%type <ifpp::Block *>
	rule
	conditionGroup
	modifier
	group
	defaultRule
;

%type <ifpp::CommandList>
	commandsAny
	commandsGroup
	commandsDefault
	commandsConditions
	actionMacro
;

%type <ifpp::TagList> tags

%type <ifpp::NameList> exprList

%type <ifpp::Color>	exprColor

%type <std::string>
	actionName
	soundId
	exprFile
;

%type <int> exprNumber

%type <bool> exprBool



%printer { ifpp::print(yyoutput, $$); }	
	<ifpp::DefinitionBase *>
	<ifpp::Condition *>
	<ifpp::Action *>
	<ifpp::Block *>
	<ifpp::CommandList>
	<ifpp::NameList>
	<ifpp::TagList>
;

%printer { yyoutput << $$; } <*>



%%



%start filterIFPP;

filterIFPP:
%empty { }
| filterIFPP definition { ctx.addInstruction($definition); }
| filterIFPP rule { ctx.addInstruction($rule); }
| filterIFPP group { ctx.addInstruction($group); }
| filterIFPP NEWLINE { }
| filterIFPP error NEWLINE { }

definition:
  KW_DEFINE VARIABLE[name] TYPE_NUMBER[type] exprNumber[value] NEWLINE
	{ $$ = magicDefinition(ctx, @$, $name, $type, $value); }
| KW_DEFINE VARIABLE[name] TYPE_COLOR[type] exprColor[value] NEWLINE
	{ $$ = magicDefinition(ctx, @$, $name, $type, $value); }
| KW_DEFINE VARIABLE[name] TYPE_FILE[type] exprFile[value] NEWLINE
	{ $$ = magicDefinition(ctx, @$, $name, $type, $value); }
| KW_DEFINE VARIABLE[name] TYPE_LIST[type] exprList[value] NEWLINE
	{ $$ = magicDefinition(ctx, @$, $name, $type, $value); }
| KW_DEFINE VARIABLE[name] TYPE_MACRO[type] newlines CHR_LEFTBRACKET NEWLINE commandsAny[value] CHR_RIGHTBRACKET NEWLINE
	{ $$ = magicDefinition(ctx, @$, $name, $type, $value); }

rule:
tags KW_RULE[what] newlines CHR_LEFTBRACKET NEWLINE commandsAny CHR_RIGHTBRACKET NEWLINE {
	$$ = new ifpp::Block(ifpp::BLOCK_RULE, $what, $commandsAny, $tags);
}

conditionGroup:
tags KW_CONDITIONGROUP[what] newlines CHR_LEFTBRACKET NEWLINE commandsConditions CHR_RIGHTBRACKET NEWLINE {
	$$ = new ifpp::Block(ifpp::BLOCK_CONDITIONGROUP, $what, $commandsConditions, $tags);
}

modifier:
tags KW_MODIFIER[what] newlines CHR_LEFTBRACKET NEWLINE commandsAny CHR_RIGHTBRACKET NEWLINE {
	$$ = new ifpp::Block(ifpp::BLOCK_MODIFIER, $what, $commandsAny, $tags);
}

group:
tags KW_GROUP[what] newlines CHR_LEFTBRACKET NEWLINE commandsGroup CHR_RIGHTBRACKET NEWLINE {
	$$ = new ifpp::Block(ifpp::BLOCK_GROUP, $what, $commandsGroup, $tags);
}

defaultRule:
tags KW_DEFAULT[what] newlines CHR_LEFTBRACKET NEWLINE commandsDefault CHR_RIGHTBRACKET NEWLINE {
	$$ = new ifpp::Block(ifpp::BLOCK_DEFAULT, $what, $commandsDefault, $tags);
}



commandsAny:
%empty { listNew($$); }
| commandsAny condition { listAppend($$, $1, $2); }
| commandsAny conditionGroup { listAppend($$, $1, $2); }
| commandsAny action { listAppend($$, $1, $2); }
| commandsAny actionMacro { listMerge($$, $1, $2); }
| commandsAny rule { listAppend($$, $1, $2); }
| commandsAny modifier { listAppend($$, $1, $2); }
| commandsAny group { listAppend($$, $1, $2); }
| commandsAny defaultRule { listAppend($$, $1, $2); }
| commandsAny NEWLINE { listCopy($$, $1); }
| commandsAny error NEWLINE { listCopy($$, $1); }

commandsGroup:
%empty { listNew($$); }
| commandsGroup actionMacro { listMerge($$, $1, $2); }
| commandsGroup rule { listAppend($$, $1, $2); }
| commandsGroup modifier { listAppend($$, $1, $2); }
| commandsGroup group { listAppend($$, $1, $2); }
| commandsGroup NEWLINE { listCopy($$, $1); }
| commandsGroup error NEWLINE { listCopy($$, $1); }

commandsDefault:
%empty { listNew($$); }
| commandsDefault action { listAppend($$, $1, $2); }
| commandsDefault actionMacro { listMerge($$, $1, $2); }
| commandsDefault NEWLINE { listCopy($$, $1); }
| commandsDefault error NEWLINE { listCopy($$, $1); }

commandsConditions:
%empty { listNew($$); }
| commandsConditions condition { listAppend($$, $1, $2); }
| commandsConditions conditionGroup { listAppend($$, $1, $2); }
| commandsConditions actionMacro { listMerge($$, $1, $2); }
| commandsConditions NEWLINE { listCopy($$, $1); }
| commandsConditions error NEWLINE { listCopy($$, $1); }


condition:
  tags CON_NUMBER[what] exprNumber[value] NEWLINE
	{ $$ = magicInterval(ctx, @$, $what, $value, $value, $tags); }
| tags CON_NUMBER[what] OPERATOR[op] exprNumber[value] NEWLINE
	{ $$ = magicInterval(ctx, @$, $what, $op, $value, $tags); }
| tags CON_NUMBER[what] exprNumber[from] CHR_DOTDOT exprNumber[to] NEWLINE
	{ $$ = magicInterval(ctx, @$, $what, $from, $to, $tags); }
| tags CON_RARITY[what] OPERATOR[op] CONST_RARITY[value] NEWLINE
	{ $$ = magicInterval(ctx, @$, $what, $op, $value, $tags); }
| tags CON_RARITY[what] CONST_RARITY[value] NEWLINE
	{ $$ = magicInterval(ctx, @$, $what, $value, $value, $tags); }
| tags CON_RARITY[what] CONST_RARITY[from] CHR_DOTDOT CONST_RARITY[to] NEWLINE
	{ $$ = magicInterval(ctx, @$, $what, $from, $to, $tags); }
| tags CON_LIST[what] exprList[list] NEWLINE
	{ $$ = new ifpp::ConditionNameList($what, $list, $tags); }
| tags CON_BOOL[what] exprBool[value] NEWLINE
	{ $$ = new ifpp::ConditionBool($what, $value, $tags); }
| tags CON_SOCKETGROUP[what] SOCKETGROUP[value] NEWLINE
	{ $$ = new ifpp::ConditionSocketGroup($what, $value, $tags); }



action:
  tags AC_NUMBER[what] exprNumber[value] NEWLINE {
	clampValue(ctx, @$, $what, $value, ifpp::getLimit($what, ifpp::MIN), ifpp::getLimit($what, ifpp::MAX));
	$$ = new ifpp::ActionNumber($what, $value, $tags);
}
| tags AC_COLOR[what] exprColor[value] NEWLINE
	{ $$ = new ifpp::ActionColor($what, $value, $tags); }
| tags AC_BOOL[what] exprBool[value] NEWLINE
	{ $$ = new ifpp::ActionBool($what, $value, $tags); }
| tags AC_SOUND[what] soundId NEWLINE
	{ $$ = new ifpp::ActionSound($what, $soundId, ifpp::getLimit("Volume", ifpp::DEFAULT), $tags); }
| tags AC_SOUND[what] soundId exprNumber[volume] NEWLINE {
	clampValue(ctx, @volume, "Sound volume", $volume, ifpp::getLimit("Volume", ifpp::MIN), ifpp::getLimit("Volume", ifpp::MAX));
	$$ = new ifpp::ActionSound($what, $soundId, $volume, $tags);
}
| tags AC_CUSTOMSOUND[what] exprFile[file] NEWLINE
	{ $$ = new ifpp::ActionFile($what, $file, $tags); }
| tags AC_MINIMAPICON[what] exprNumber[size] CONST_COLOR[color] CONST_SHAPE[shape] NEWLINE {
	clampValue(ctx, @$, $what, $size, ifpp::getLimit($what, ifpp::MIN), ifpp::getLimit($what, ifpp::MAX));
	$$ = new ifpp::ActionMapIcon($what, $size, $color, $shape, $tags);
}
| tags AC_PLAYEFFECT[what] CONST_COLOR[color] NEWLINE
	{ $$ = new ifpp::ActionEffect($what, $color, "", $tags); }
| tags AC_PLAYEFFECT[what] CONST_COLOR[color] CONST_TEMP NEWLINE
	{ $$ = new ifpp::ActionEffect($what, $color, "Temp", $tags); }
| tags AC_REMOVE actionName[what] NEWLINE { $$ = new ifpp::ActionRemove($what, $tags); }

actionMacro:
tags AC_USEMACRO[what] VARIABLE[varName] NEWLINE {
	if (checkVarUse(ctx, @varName, $varName, ifpp::EXPR_MACRO)) {
		$$ = ctx.getVarValueMacro($varName);
	} else {
		YYERROR;
	}
	for (auto a : $$) {
		a->tags |= $tags;
	}
}

actionName:
  AC_NUMBER { $$ = $1; }
| AC_COLOR { $$ = $1; }
| AC_BOOL { $$ = $1; }
| AC_SOUND { $$ = $1; }
| AC_CUSTOMSOUND { $$ = $1; }
| AC_MINIMAPICON { $$ = $1; }
| AC_PLAYEFFECT { $$ = $1; }



exprNumber:
NUMBER { $$ = $1; }
| VARIABLE {
	if (checkVarUse(ctx, @1, $1, ifpp::EXPR_NUMBER)) {
		$$ = ctx.getVarValueNumber($1);
	} else {
		YYERROR;
	}
}

exprColor:
HEX { $$ = ifpp::Color($1); }
| exprNumber exprNumber exprNumber {
	clampValue(ctx, @1, "Color value", $1, ifpp::getLimit("Color", ifpp::MIN), ifpp::getLimit("Color", ifpp::MAX));
	clampValue(ctx, @2, "Color value", $2, ifpp::getLimit("Color", ifpp::MIN), ifpp::getLimit("Color", ifpp::MAX));
	clampValue(ctx, @3, "Color value", $3, ifpp::getLimit("Color", ifpp::MIN), ifpp::getLimit("Color", ifpp::MAX));
	$$ = ifpp::Color($1, $2, $3);
}
| exprNumber exprNumber exprNumber exprNumber {
	clampValue(ctx, @1, "Color value", $1, ifpp::getLimit("Color", ifpp::MIN), ifpp::getLimit("Color", ifpp::MAX));
	clampValue(ctx, @2, "Color value", $2, ifpp::getLimit("Color", ifpp::MIN), ifpp::getLimit("Color", ifpp::MAX));
	clampValue(ctx, @3, "Color value", $3, ifpp::getLimit("Color", ifpp::MIN), ifpp::getLimit("Color", ifpp::MAX));
	clampValue(ctx, @4, "Color value", $4, ifpp::getLimit("Color", ifpp::MIN), ifpp::getLimit("Color", ifpp::MAX));
	$$ = ifpp::Color($1, $2, $3, $4);
}
| VARIABLE {
	if (checkVarUse(ctx, @1, $1, ifpp::EXPR_COLOR)) {
		$$ = ctx.getVarValueColor($1);
	} else {
		YYERROR;
	}
}
| VARIABLE CHR_COLON exprNumber {
	if (checkVarUse(ctx, @1, $1, ifpp::EXPR_COLOR)) {
		$$ = ctx.getVarValueColor($1);
	} else {
		YYERROR;
	}
	$$.a = $3;
}

exprBool:
%empty { $$ = true; }
| CONST_BOOL { $$ = $1; }

exprFile:
FILENAME { $$ = $1; }
| VARIABLE {
	if (checkVarUse(ctx, @1, $1, ifpp::EXPR_FILE)) {
		$$ = ctx.getVarValueFile($1);
	} else {
		YYERROR;
	}
}

exprList:
%empty { listNew($$); }
| exprList NAME	{ listAppend($$, $1, $2); }
| exprList VARIABLE {
	if (checkVarUse(ctx, @2, $2, ifpp::EXPR_LIST)) {
		listMerge($$, $1, ctx.getVarValueList($2));
	} else {
		listCopy($$, $1);
	}
}


tags:
%empty { $$ = 0; }
| tags TAG { $$ = $1 | $2; }

soundId:
exprNumber {
	std::stringstream ss;
	ss << $1;
	$$ = ss.str();
}
| CONST_SOUND { $$ = $1; }

newlines:
%empty {}
| newlines NEWLINE {}

%%
