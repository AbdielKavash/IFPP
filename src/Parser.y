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

	static bool checkVarUse(ifpp::Context & ctx, const yy::location & l, const std::string & name, ifpp::VariableType type) {
		ifpp::VariableType oldType = ctx.getVarType(name);
		if (oldType == ifpp::VAR_UNDEFINED) {
			if (type == ifpp::VAR_LIST) {
				ctx.errorAt(l) << "Variable " << name << " has not been defined. "
					<< "It will be ignored in this list." << std::endl;
			} else {
				ctx.errorAt(l) << "Variable " << name << " has not been defined. "
					<< "This command will be ignored." << std::endl;
			}
			return false;
		} else if (oldType != type) {
			if (type == ifpp::VAR_LIST) {
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
		const std::string & name, ifpp::VariableType type, const T & value, bool redefine = false) {

		ifpp::VariableType oldType = ctx.getVarType(name);
		if (redefine) {
			if (oldType == ifpp::VAR_UNDEFINED) {
				ctx.warningAt(l) << "Variable " << name << " has not been defined yet. "
					<< "It will be defined now. Use the Define command instead to avoid this warning." << std::endl;
			} else if (oldType != type) {
				ctx.errorAt(l) << "Variable " << name << " has already been defined as type " << oldType << "! "
					<< "The variable will be replaced by the new value of type " << type << "." << std::endl;
				ctx.undefineVariable(name);
			}
		} else {
			if (oldType == type) {
				ctx.warningAt(l) << "Variable " << name << " has already been defined. "
					<< "It will be replaced by the new value. Use the Redefine command instead to avoid this warning." << std::endl;
			} else if (oldType != ifpp::VAR_UNDEFINED) {
				ctx.errorAt(l) << "Variable " << name << " has already been defined as type " << oldType << "! "
					<< "The variable will be replaced by the new value of type " << type << "." << std::endl;
				ctx.undefineVariable(name);
			}
		}

		ctx.defineVariable(name, type, value);
		return new ifpp::Definition<T>(name, type, value);
	}

	static ifpp::ConditionInterval * magicInterval(ifpp::Context & ctx, const yy::location & l,
		const std::string & what, int from, int to) {

		clampInterval(ctx, l, what, from, to, ifpp::getLimit(what, ifpp::MIN), ifpp::getLimit(what, ifpp::MAX));
		return new ifpp::ConditionInterval(what, from, to);
	}

	static ifpp::ConditionInterval * magicInterval(ifpp::Context & ctx, const yy::location & l,
		const std::string & what, ifpp::Operator op, int value) {

		int from = INT_MIN, to = INT_MAX;
		switch(op) {
			case ifpp::OP_LT: to = value - 1; break;
			case ifpp::OP_LE: to = value; break;
			case ifpp::OP_EQ: from = to = value; break;
			case ifpp::OP_GE: from = value; break;
			case ifpp::OP_GT: from = value + 1; break;
			default: throw ifpp::UnhandledCase("Operator", __FILE__, __LINE__);
		}
		return magicInterval(ctx, l, what, from, to);
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

%token
	NEWLINE "end of line"

	CHR_LEFTBRACKET "{"
	CHR_RIGHTBRACKET "}"
	CHR_PERIOD "."
	CHR_DOTDOT ".."
	CHR_COLON ":"

	KW_RULE "Rule"
	KW_DEFAULT "Default"
	
	KW_DEFINE "Define"
	KW_REDEFINE "Redefine"

	KW_VERSION "Version"
;

%token <ifpp::VariableType>
	TYPE_NUMBER "Number"
	TYPE_COLOR "Color"
	TYPE_FILE "File"
	TYPE_LIST "List"
	TYPE_STYLE "Style"
;

%token <ifpp::Operator>
	OP_LT "<"
	OP_LE "<="
	OP_EQ "="
	OP_GE ">="
	OP_GT ">"
;

%token <ifpp::SocketGroup> SOCKETGROUP "socket group"

%token <ifpp::ModifierList> KW_OVERRIDE "Override"

%token <bool> CONST_BOOL "boolean value"

%token <std::string>
	CON_ITEMLEVEL "ItemLevel"
	CON_DROPLEVEL "DropLevel"
	CON_QUALITY "Quality"
	CON_SOCKETS "Sockets"
	CON_LINKEDSOCKETS "LinkedSockets"
	CON_HEIGHT "Height"
	CON_WIDTH "Width"
	CON_STACKSIZE "StackSize"
	CON_GEMLEVEL "GemLevel"
	CON_MAPTIER "MapTier"

	CON_RARITY "Rarity"

	CON_CLASS "Class"
	CON_BASETYPE "BaseType"
	CON_EXPLICIT "HasExplicitMod"

	CON_IDENTIFIED "Identified"
	CON_CORRUPTED "Corrupted"
	CON_ELDERITEM "ElderItem"
	CON_SHAPERITEM "ShaperItem"
	CON_SHAPEDMAP "ShapedMap"

	CON_SOCKETGROUP "SocketGroup"

	AC_FONTSIZE "SetFontSize"

	AC_BORDERCOLOR "SetBorderColor"
	AC_TEXTCOLOR "SetTextColor"
	AC_BGCOLOR "SetBackgroundColor"

	AC_SOUND "PlayAlertSound"
	AC_SOUNDPOSITIONAL "PlayAlertSoundPositional"

	AC_DISABLESOUND "DisableDropSound"
	AC_HIDDEN "Hidden"

	AC_CUSTOMSOUND "CustomAlertSound"
	AC_MINIMAPICON "MinimapIcon"
	AC_PLAYEFFECT "PlayEffect"

	AC_USESTYLE "UseStyle"

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

%token END 0 "end of file"


%type <ifpp::Instruction *> instruction
%type <ifpp::DefinitionBase *> definition

%type <ifpp::CommandList>
	ruleBody
	conditions
	actions
	actionsNotEmpty
	rules
	rulesNotEmpty
;

%type <ifpp::Condition *> condition
%type <ifpp::Action *> action
%type <ifpp::DefaultStyle *> defaultStyle
%type <ifpp::RuleIFPP *> rule

%type <ifpp::Style>
	style
	actionStyle
	exprStyle
	varStyle
;

%type <ifpp::Operator> operator

%type <ifpp::ModifierList>
	modifiers
	modifier
;

%type <std::string>
	conditionNumeric
	conditionRarity
	conditionBool
	conditionNameList
	conditionSocketGroup

	actionNumber
	actionColor
	actionBool
	actionSound

	soundId

	exprFile
	varFile
;

%type <int>
	soundVolume
	exprNumber
	exprReallyNumber
	varNumber
;

%type <ifpp::Color>
	exprColor
	varColor
;

%type <ifpp::NameList>
	exprList
	varList
;

%type <bool>
	defineOrRedefine
	exprBool
;

%printer { ifpp::print(yyoutput, ifpp::PRINT_IFPP, $$); }
	<ifpp::Condition *>
	<ifpp::Action *>
	<ifpp::CommandList>
	<ifpp::Style>

	<ifpp::DefinitionBase *>
	<ifpp::Instruction *>
	<ifpp::RuleIFPP *>
;

%printer { ifpp::operator<<(yyoutput, $$); }
	<ifpp::NameList>
;

%printer { yyoutput << $$; } <*>

%%

%start statements;

statements:
%empty { }
| statements instruction { ctx.addStatement($2); }
| statements definition { ctx.addStatement($2); }
| statements rule { ctx.addStatement($2); }
| statements NEWLINE { }
| statements error NEWLINE { }

instruction:
KW_VERSION NUMBER[vMajor] CHR_PERIOD NUMBER[vMinor] CHR_PERIOD NUMBER[vBugfix] NEWLINE {
	if (ctx.versionCheck($vMajor, $vMinor, $vBugfix)) {
		$$ = new ifpp::InstructionVersion($vMajor, $vMinor, $vBugfix);
	} else {
		// If it is impossible to continue, the context should abort parsing gracefully.
		YYABORT;
	}
}

definition:
  defineOrRedefine VARIABLE TYPE_NUMBER exprNumber NEWLINE
	{ $$ = magicDefinition(ctx, @$, $2, $3, $4, $1); }
| defineOrRedefine VARIABLE TYPE_COLOR exprColor NEWLINE
	{ $$ = magicDefinition(ctx, @$, $2, $3, $4, $1); }
| defineOrRedefine VARIABLE TYPE_FILE exprFile NEWLINE
	{ $$ = magicDefinition(ctx, @$, $2, $3, $4, $1); }
| defineOrRedefine VARIABLE TYPE_LIST exprList NEWLINE
	{ $$ = magicDefinition(ctx, @$, $2, $3, $4, $1); }
| defineOrRedefine VARIABLE TYPE_STYLE newlines CHR_LEFTBRACKET NEWLINE style CHR_RIGHTBRACKET NEWLINE
	{ $$ = magicDefinition(ctx, @$, $2, $3, $style, $1); }

defineOrRedefine:
  KW_DEFINE { $$ = false; }
| KW_REDEFINE { $$ = true; }

rule:
KW_RULE modifiers newlines CHR_LEFTBRACKET NEWLINE ruleBody CHR_RIGHTBRACKET NEWLINE {
	$$ = new ifpp::RuleIFPP($modifiers, $ruleBody);
}

ruleBody:
conditions actions rules defaultStyle {
	$$.insert($$.end(), $1.begin(), $1.end());
	$$.insert($$.end(), $2.begin(), $2.end());
	$$.insert($$.end(), $3.begin(), $3.end());
	if ($4) $$.push_back($4);
}

conditions:
%empty {}
| conditions condition { listAppend($$, $1, $2); }
| conditions NEWLINE { listCopy($$, $1); }
| conditions error NEWLINE { listCopy($$, $1); }

actions:
%empty { /* To fix ambiguity when a rule only contains newlines. */ }
| actionsNotEmpty { listCopy($$, $1); }

actionsNotEmpty:
action { listAppend($$, $$, $1); }
| actionStyle { listMerge($$, $$, $1); }
| actionsNotEmpty action { listAppend($$, $1, $2); }
| actionsNotEmpty actionStyle { listMerge($$, $1, $2); }
| actionsNotEmpty NEWLINE { listCopy($$, $1); }
| actionsNotEmpty error NEWLINE { listCopy($$, $1); }

rules:
%empty { /* To fix ambiguity when a rule only contains newlines. */ }
| rulesNotEmpty { $$ = $1; }

rulesNotEmpty:
rule { listAppend($$, $$, $1); }
| rulesNotEmpty rule { listAppend($$, $1, $2); }
| rulesNotEmpty NEWLINE { listCopy($$, $1); }
| rulesNotEmpty error NEWLINE { listCopy($$, $1); }

defaultStyle:
%empty { $$ = NULL; }
| KW_DEFAULT newlines CHR_LEFTBRACKET NEWLINE style CHR_RIGHTBRACKET newlines { $$ = new ifpp::DefaultStyle($style); }

newlines:
%empty {}
| newlines NEWLINE {}



condition:
  conditionNumeric[what] operator[op] exprNumber[value] NEWLINE
	{ $$ = magicInterval(ctx, @$, $what, $op, $value); }
| conditionNumeric[what] exprNumber[value] NEWLINE
	{ $$ = magicInterval(ctx, @$, $what, $value, $value); }
| conditionNumeric[what] exprNumber[from] CHR_DOTDOT exprNumber[to] NEWLINE
	{ $$ = magicInterval(ctx, @$, $what, $from, $to); }
| conditionRarity[what] operator[op] CONST_RARITY[value] NEWLINE
	{ $$ = magicInterval(ctx, @$, $what, $op, $value); }
| conditionRarity[what] CONST_RARITY[value] NEWLINE
	{ $$ = magicInterval(ctx, @$, $what, $value, $value); }
| conditionRarity[what] CONST_RARITY[from] CHR_DOTDOT CONST_RARITY[to] NEWLINE
	{ $$ = magicInterval(ctx, @$, $what, $from, $to); }
| conditionBool[what] exprBool[value] NEWLINE
	{ $$ = new ifpp::ConditionBool($what, $value); }
| conditionNameList[what] exprList[list] NEWLINE
	{ $$ = new ifpp::ConditionNameList($what, $list); }
| conditionSocketGroup[what] SOCKETGROUP[value] NEWLINE
	{ $$ = new ifpp::ConditionSocketGroup($what, $value); }

conditionNumeric:
  CON_ITEMLEVEL { $$ = $1; }
| CON_DROPLEVEL { $$ = $1; }
| CON_QUALITY { $$ = $1; }
| CON_SOCKETS { $$ = $1; }
| CON_LINKEDSOCKETS { $$ = $1; }
| CON_HEIGHT { $$ = $1; }
| CON_WIDTH { $$ = $1; }
| CON_STACKSIZE { $$ = $1; }
| CON_GEMLEVEL { $$ = $1; }
| CON_MAPTIER { $$ = $1; }

conditionRarity:
CON_RARITY { $$ = $1; }

conditionBool:
  CON_IDENTIFIED { $$ = $1; }
| CON_CORRUPTED { $$ = $1; }
| CON_ELDERITEM { $$ = $1; }
| CON_SHAPERITEM { $$ = $1; }
| CON_SHAPEDMAP { $$ = $1; }

conditionNameList:
  CON_CLASS { $$ = $1; }
| CON_BASETYPE { $$ = $1; }
| CON_EXPLICIT { $$ = $1; }

conditionSocketGroup:
CON_SOCKETGROUP { $$ = $1; }



action:
  modifiers actionNumber[what] exprNumber[value] NEWLINE {
	clampValue(ctx, @$, $what, $value, ifpp::getLimit($what, ifpp::MIN), ifpp::getLimit($what, ifpp::MAX));
	$$ = new ifpp::ActionNumber($modifiers, $what, $value);
}
| modifiers actionColor[what] exprColor[value] NEWLINE
	{ $$ = new ifpp::ActionColor($modifiers, $what, $value); }
| modifiers actionBool[what] exprBool[value] NEWLINE
	{ $$ = new ifpp::ActionBool($modifiers, $what, $value); }
| modifiers actionSound[what] soundId soundVolume NEWLINE
	{ $$ = new ifpp::ActionSound($modifiers, $what, $soundId, $soundVolume); }
| modifiers AC_CUSTOMSOUND[what] exprFile[file] NEWLINE
	{ $$ = new ifpp::ActionFile($modifiers, $what, $file); }
| modifiers AC_MINIMAPICON[what] exprNumber[size] CONST_COLOR[color] CONST_SHAPE[shape] NEWLINE {
	clampValue(ctx, @$, $what, $size, ifpp::getLimit($what, ifpp::MIN), ifpp::getLimit($what, ifpp::MAX));
	$$ = new ifpp::ActionMapIcon($modifiers, $what, $size, $color, $shape);
}
| modifiers AC_PLAYEFFECT[what] CONST_COLOR[color] NEWLINE
	{ $$ = new ifpp::ActionEffect($modifiers, $what, $color, ""); }
| modifiers AC_PLAYEFFECT[what] CONST_COLOR[color] CONST_TEMP[temp] NEWLINE
	{ $$ = new ifpp::ActionEffect($modifiers, $what, $color, $temp); }

actionNumber:
AC_FONTSIZE { $$ = $1; }

actionColor:
  AC_TEXTCOLOR { $$ = $1; }
| AC_BGCOLOR { $$ = $1; }
| AC_BORDERCOLOR { $$ = $1; }

actionBool:
  AC_HIDDEN { $$ = $1; }
| AC_DISABLESOUND { $$ = $1; }

actionSound:
  AC_SOUND { $$ = $1; }
| AC_SOUNDPOSITIONAL { $$ = $1; }

actionStyle:
modifiers AC_USESTYLE exprStyle[style] NEWLINE {
	listNew($$);
	for (const auto & a : $style) {
		ifpp::Action * aa = static_cast<ifpp::Action *>(a->clone());
		aa->modifiers |= $modifiers;
		$$.push_back(aa);
	}
}



modifiers:
%empty { $$ = 0; }
| modifiers modifier { $$ |= $1; }

modifier:
KW_OVERRIDE { $$ = $1; }



style:
actions {
	for (auto & a : $1) {
		$$.push_back(static_cast<ifpp::Action *>(a));
	}
}

operator:
  OP_LT	{ $$ = $1; }
| OP_LE	{ $$ = $1; }
| OP_EQ	{ $$ = $1; }
| OP_GE	{ $$ = $1; }
| OP_GT	{ $$ = $1; }

soundId:
exprNumber {
	std::stringstream ss;
	ss << $1;
	$$ = ss.str();
}
| CONST_SOUND { $$ = $1; }

soundVolume:
%empty { $$ = ifpp::getLimit("Volume", ifpp::DEFAULT); }
| exprNumber {
	clampValue(ctx, @1, "Sound volume", $1, ifpp::getLimit("Volume", ifpp::MIN), ifpp::getLimit("Volume", ifpp::MAX));
	$$ = $1;
}



exprNumber:
varNumber { $$ = $1; }
| exprReallyNumber { $$ = $1; }

exprReallyNumber:
NUMBER { $$ = $1; }

exprBool:
%empty { $$ = true; }
| CONST_BOOL { $$ = $1; }

exprColor:
HEX { $$ = ifpp::Color($1); }
| varColor { $$ = $1; }
| varColor CHR_COLON exprNumber { $$ = $1; $$.a = $3; }
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

exprFile:
varFile { $$ = $1; }
| FILENAME { $$ = $1; }

exprList:
%empty { listNew($$); }
| exprList NAME	{ listAppend($$, $1, $2); }
| exprList varList[var] { listMerge($$, $1, $2); }

exprStyle:
varStyle { $$ = $1; }



varNumber:
VARIABLE {
	if (checkVarUse(ctx, @1, $1, ifpp::VAR_NUMBER)) {
		$$ = ctx.getVarValueNumber($1);
	} else {
		YYERROR;
	}
}

varColor:
VARIABLE {
	if (checkVarUse(ctx, @1, $1, ifpp::VAR_COLOR)) {
		$$ = ctx.getVarValueColor($1);
	} else {
		YYERROR;
	}
}

varFile:
VARIABLE {
	if (checkVarUse(ctx, @1, $1, ifpp::VAR_FILE)) {
		$$ = ctx.getVarValueFile($1);
	} else {
		YYERROR;
	}
}

varList:
VARIABLE {
	if (checkVarUse(ctx, @1, $1, ifpp::VAR_LIST)) {
		$$ = ctx.getVarValueList($1);
	} else {
		listNew($$);
	}
}

varStyle:
VARIABLE {
	if (checkVarUse(ctx, @1, $1, ifpp::VAR_STYLE)) {
		$$ = ctx.getVarValueStyle($1);
	} else {
		YYERROR;
	}
}

%%
