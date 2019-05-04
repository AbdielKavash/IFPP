%option batch stack
%option noyywrap noinput

%{

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <string>

#include "Types.h"
#include "Parser.h"
#include "Context.h"

#define YY_DECL yy::Parser::symbol_type yylex(ifpp::Context & ctx)
YY_DECL;

static yy::location loc;
// Code run each time a pattern is matched.
#define YY_USER_ACTION { loc.step(); loc.columns(yyleng); }

// Work around an incompatibility in flex (at least versions
// 2.5.31 through 2.5.33): it generates code that does
// not conform to C89.  See Debian bug 333231
// <http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=333231>.
# undef yywrap
# define yywrap() 1

static void yyunput ( int c, char *buf_ptr  );

void unputs(const char * s) {
	if (!*s) return;
	const char * p = s;
	while (*p) ++p;
	do { unput(*--p); } while (p != s);
}

%}

%x comment
%x nameList
%x includeFile

blank [ \t\r]

idPrefix "$"
id [[:alpha:][:digit:]_]
dotdot ".."

hex [0-9a-fA-F]
socket [rgbwRGBW]

comStart "//"|"#"
garbage [^ \t\r\n.><={}/#]

%%

{blank}*\n		loc.lines(); return yy::Parser::make_NEWLINE("\n", loc);
{blank}+

<INITIAL,nameList>{blank}*{comStart}[^\n]*

<INITIAL,nameList>"/*"  yy_push_state(comment);
<comment>{
	[^*\n]*
	"*"+[^*/\n]*
	\n			loc.lines();
	"*/"        yy_pop_state();
}


Include					yy_push_state(includeFile);
<includeFile>{blank}*\"
<includeFile>[^"\r\n]* {
	yyin = fopen(yytext, "r");
	if (!yyin) {
		ctx.errorAt(loc) << "Included file \"" << yytext << "\" not found!" << std::endl;
	} else {
		yypush_buffer_state(yy_create_buffer(yyin, YY_BUF_SIZE));
		yy_push_state(INITIAL);
	}
}
<includeFile>\"{blank}*\n {
	loc.lines();
	yy_pop_state();
	return yy::Parser::make_NEWLINE("\n", loc);
}


"{"				unputs("@{@\n"); return yy::Parser::make_NEWLINE("\n", loc);
"}"				unputs("@}@\n"); return yy::Parser::make_NEWLINE("\n", loc);
"@{"			return yy::Parser::make_CHR_LEFTBRACKET("{", loc);
"@}"			return yy::Parser::make_CHR_RIGHTBRACKET("}", loc);
"@\n"			return yy::Parser::make_NEWLINE("\n", loc);

".."			return yy::Parser::make_CHR_DOTDOT("..", loc);
"."				return yy::Parser::make_CHR_PERIOD(".", loc);
":"				return yy::Parser::make_CHR_COLON(":", loc);

"<"				return yy::Parser::make_OPERATOR(ifpp::OP_LT, loc);
"<="			return yy::Parser::make_OPERATOR(ifpp::OP_LE, loc);
"="				return yy::Parser::make_OPERATOR(ifpp::OP_EQ, loc);
">="			return yy::Parser::make_OPERATOR(ifpp::OP_GE, loc);
">"				return yy::Parser::make_OPERATOR(ifpp::OP_GT, loc);

Define			return yy::Parser::make_KW_DEFINE("Define", loc);

Rule			return yy::Parser::make_KW_RULE("Rule", loc);
ConditionGroup	return yy::Parser::make_KW_CONDITIONGROUP("ConditionGroup", loc);
Modifier		return yy::Parser::make_KW_MODIFIER("Modifier", loc);
Group			return yy::Parser::make_KW_GROUP("Group", loc);
Default			return yy::Parser::make_KW_DEFAULT("Default", loc);

Override		return yy::Parser::make_TAG(ifpp::TAG_OVERRIDE, loc);
Final			return yy::Parser::make_TAG(ifpp::TAG_FINAL, loc);
NoDefault		return yy::Parser::make_TAG(ifpp::TAG_NODEFAULT, loc);
Required 		return yy::Parser::make_TAG(ifpp::TAG_REQUIRED, loc);

ItemLevel		return yy::Parser::make_CON_NUMBER("ItemLevel", loc);
DropLevel		return yy::Parser::make_CON_NUMBER("DropLevel", loc);
Quality			return yy::Parser::make_CON_NUMBER("Quality", loc);
Sockets			return yy::Parser::make_CON_NUMBER("Sockets", loc);
LinkedSockets	return yy::Parser::make_CON_NUMBER("LinkedSockets", loc);
Height			return yy::Parser::make_CON_NUMBER("Height", loc);
Width			return yy::Parser::make_CON_NUMBER("Width", loc);
StackSize		return yy::Parser::make_CON_NUMBER("StackSize", loc);
GemLevel		return yy::Parser::make_CON_NUMBER("GemLevel", loc);
MapTier			return yy::Parser::make_CON_NUMBER("MapTier", loc);

Rarity			return yy::Parser::make_CON_RARITY("Rarity", loc);

Class			yy_push_state(nameList); return yy::Parser::make_CON_LIST("Class", loc);
BaseType		yy_push_state(nameList); return yy::Parser::make_CON_LIST("BaseType", loc);
Prophecy		yy_push_state(nameList); return yy::Parser::make_CON_LIST("Prophecy", loc);
HasExplicitMod	yy_push_state(nameList); return yy::Parser::make_CON_LIST("HasExplicitMod", loc);
HasEnchantment	yy_push_state(nameList); return yy::Parser::make_CON_LIST("HasEnchantment", loc);

Identified		return yy::Parser::make_CON_BOOL("Identified", loc);
Corrupted		return yy::Parser::make_CON_BOOL("Corrupted", loc);
ElderItem		return yy::Parser::make_CON_BOOL("ElderItem", loc);
ShaperItem		return yy::Parser::make_CON_BOOL("ShaperItem", loc);
ShapedMap		return yy::Parser::make_CON_BOOL("ShapedMap", loc);
FracturedItem	return yy::Parser::make_CON_BOOL("FracturedItem", loc);
SynthesisedItem	return yy::Parser::make_CON_BOOL("SynthesisedItem", loc);
AnyEnchantment	return yy::Parser::make_CON_BOOL("AnyEnchantment", loc);

SocketGroup		return yy::Parser::make_CON_SOCKETGROUP("SocketGroup", loc);

SetFontSize		return yy::Parser::make_AC_NUMBER("SetFontSize", loc);
SetTextSize		return yy::Parser::make_AC_NUMBER("SetFontSize", loc);

SetBorderColor	return yy::Parser::make_AC_COLOR("SetBorderColor", loc);
SetTextColor	return yy::Parser::make_AC_COLOR("SetTextColor", loc);
SetFontColor	return yy::Parser::make_AC_COLOR("SetTextColor", loc);
SetBackgroundColor	return yy::Parser::make_AC_COLOR("SetBackgroundColor", loc);

PlayAlertSound				return yy::Parser::make_AC_SOUND("PlayAlertSound", loc);
PlayAlertSoundPositional	return yy::Parser::make_AC_SOUND("PlayAlertSoundPositional", loc);

CustomAlertSound	return yy::Parser::make_AC_CUSTOMSOUND("CustomAlertSound", loc);
MinimapIcon			return yy::Parser::make_AC_MINIMAPICON("MinimapIcon", loc);
PlayEffect			return yy::Parser::make_AC_PLAYEFFECT("PlayEffect", loc);

DisableDropSound	return yy::Parser::make_AC_BOOL("DisableDropSound", loc);
Hidden			return yy::Parser::make_AC_BOOL("Hidden", loc);

UseMacro		return yy::Parser::make_AC_USEMACRO("UseMacro", loc);
Remove			return yy::Parser::make_AC_REMOVE("Remove", loc);

Number			return yy::Parser::make_TYPE_NUMBER(ifpp::EXPR_NUMBER, loc);
Color			return yy::Parser::make_TYPE_COLOR(ifpp::EXPR_COLOR, loc);
File			return yy::Parser::make_TYPE_FILE(ifpp::EXPR_FILE, loc);
List			yy_push_state(nameList); return yy::Parser::make_TYPE_LIST(ifpp::EXPR_LIST, loc);
Macro			return yy::Parser::make_TYPE_MACRO(ifpp::EXPR_MACRO, loc);

true			return yy::Parser::make_CONST_BOOL(true, loc);
false			return yy::Parser::make_CONST_BOOL(false, loc);

Normal	return yy::Parser::make_CONST_RARITY(ifpp::Normal, loc);
Magic	return yy::Parser::make_CONST_RARITY(ifpp::Magic, loc);
Rare	return yy::Parser::make_CONST_RARITY(ifpp::Rare, loc);
Unique	return yy::Parser::make_CONST_RARITY(ifpp::Unique, loc);

ShAlchemy		return yy::Parser::make_CONST_SOUND("ShAlchemy", loc);
ShBlessed		return yy::Parser::make_CONST_SOUND("ShBlessed", loc);
ShChaos			return yy::Parser::make_CONST_SOUND("ShChaos", loc);
ShDivine		return yy::Parser::make_CONST_SOUND("ShDivine", loc);
ShExalted		return yy::Parser::make_CONST_SOUND("ShExalted", loc);
ShFusing		return yy::Parser::make_CONST_SOUND("ShFusing", loc);
ShGeneral		return yy::Parser::make_CONST_SOUND("ShGeneral", loc);
ShMirror		return yy::Parser::make_CONST_SOUND("ShMirror", loc);
ShRegal			return yy::Parser::make_CONST_SOUND("ShRegal", loc);
ShVaal			return yy::Parser::make_CONST_SOUND("ShVaal", loc);

Blue	return yy::Parser::make_CONST_COLOR("Blue", loc);
Brown	return yy::Parser::make_CONST_COLOR("Brown", loc);
Green	return yy::Parser::make_CONST_COLOR("Green", loc);
Red		return yy::Parser::make_CONST_COLOR("Red", loc);
White	return yy::Parser::make_CONST_COLOR("White", loc);
Yellow	return yy::Parser::make_CONST_COLOR("Yellow", loc);

Circle		return yy::Parser::make_CONST_SHAPE("Circle", loc);
Diamond		return yy::Parser::make_CONST_SHAPE("Diamond", loc);
Hexagon		return yy::Parser::make_CONST_SHAPE("Hexagon", loc);
Square		return yy::Parser::make_CONST_SHAPE("Square", loc);
Star		return yy::Parser::make_CONST_SHAPE("Star", loc);
Triangle	return yy::Parser::make_CONST_SHAPE("Triangle", loc);

Temp	return yy::Parser::make_CONST_TEMP("Temp", loc);

[[:digit:]]+ {
	errno = 0;
	long n = strtol(yytext, NULL, 10);
	if (errno == ERANGE || n > INT_MAX) {
		ctx.errorAt(loc) << "Integer constant too large! Using a really big number, this is probably not what you want." << std::endl;
		n = INT_MAX;
	}
	return yy::Parser::make_NUMBER(n, loc);
}

{idPrefix}{id}+		return yy::Parser::make_VARIABLE(yytext, loc);
x{hex}+				return yy::Parser::make_HEX(yytext + 1, loc);
{socket}+			return yy::Parser::make_SOCKETGROUP(std::string(yytext), loc);
\"[^"\r\n]*\"		return yy::Parser::make_FILENAME(yytext, loc);


<nameList>{
	{blank}+
	\\{blank}*\n	loc.lines();
	\n			loc.lines(); yy_pop_state(); return yy::Parser::make_NEWLINE("\n", loc);
	"}"			unputs("@}@\n"); yy_pop_state(); return yy::Parser::make_NEWLINE("\n", loc);
	\"[^"\r\n]*\" 	{ std::string s(yytext + 1, yyleng - 2);
						return yy::Parser::make_NAME(s, loc); }
	{idPrefix}{id}+		return yy::Parser::make_VARIABLE(yytext, loc);
	[^\}\\ \t\r\n"]+	return yy::Parser::make_NAME(yytext, loc);
}

{garbage}*		ctx.criticalAt(loc) << "String \"" << yytext << "\" not recognized!" << std::endl;

<<EOF>>	{
	yypop_buffer_state();
	if (YY_CURRENT_BUFFER) {
		// Finished reading an included file, go back to main file.
		yy_pop_state(); // pops INITIAL from the included file, continue parsing
	} else {
		// End of actual input file.
		return yy::Parser::make_END(loc);
	}
}

%%

