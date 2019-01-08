%{

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <string>

#include "Parser.h"

#include "Types.h"
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

%}

%option noyywrap nounput batch noinput debug

%x comment
%x nameList

blank [ \t\r]

idPrefix "$"
id [[:alpha:][:digit:]_]

hex [0-9a-fA-F]
socket [rgbwRGBW]

comStart "//"|"#"
garbage [^ \t\r\n.><={}/#]

%%

{blank}*\n		loc.lines(); return yy::Parser::make_NEWLINE(loc);
{blank}+

<INITIAL,nameList>{blank}*{comStart}[^\n]*		BEGIN(INITIAL);
<INITIAL,nameList>{blank}*{comStart}[^\n]*\n {
	// For error reporting, the \n should still be a part of the last line.
	yy::location l = loc;
	loc.lines();
	BEGIN(INITIAL);
	return yy::Parser::make_NEWLINE(l);
}

<INITIAL,nameList>"/*"  BEGIN(comment);
<comment>[^*\n]*
<comment>"*"+[^*/\n]*
<comment>\n				loc.lines();
<comment>"*"+"/"        BEGIN(INITIAL);


"{"				return yy::Parser::make_CHR_LEFTBRACKET(loc);
"}"				return yy::Parser::make_CHR_RIGHTBRACKET(loc);
"."				return yy::Parser::make_CHR_PERIOD(loc);
".."			return yy::Parser::make_CHR_DOTDOT(loc);
":"				return yy::Parser::make_CHR_COLON(loc);

"<"				return yy::Parser::make_OP_LT(ifpp::OP_LT, loc);
"<="			return yy::Parser::make_OP_LE(ifpp::OP_LE, loc);
"="				return yy::Parser::make_OP_EQ(ifpp::OP_EQ, loc);
">="			return yy::Parser::make_OP_GE(ifpp::OP_GE, loc);
">"				return yy::Parser::make_OP_GT(ifpp::OP_GT, loc);

Rule			return yy::Parser::make_KW_RULE(loc);
Default			return yy::Parser::make_KW_DEFAULT(loc);

Override		return yy::Parser::make_KW_OVERRIDE(ifpp::MOD_OVERRIDE, loc);

Define			return yy::Parser::make_KW_DEFINE(loc);
Redefine		return yy::Parser::make_KW_REDEFINE(loc);

Version			return yy::Parser::make_KW_VERSION(loc);

ItemLevel		return yy::Parser::make_CON_ITEMLEVEL("ItemLevel", loc);
DropLevel		return yy::Parser::make_CON_DROPLEVEL("DropLevel", loc);
Quality			return yy::Parser::make_CON_QUALITY("Quality", loc);
Sockets			return yy::Parser::make_CON_SOCKETS("Sockets", loc);
LinkedSockets	return yy::Parser::make_CON_LINKEDSOCKETS("LinkedSockets", loc);
Height			return yy::Parser::make_CON_HEIGHT("Height", loc);
Width			return yy::Parser::make_CON_WIDTH("Width", loc);
StackSize		return yy::Parser::make_CON_STACKSIZE("StackSize", loc);
GemLevel		return yy::Parser::make_CON_GEMLEVEL("GemLevel", loc);
MapTier			return yy::Parser::make_CON_MAPTIER("MapTier", loc);

Rarity			return yy::Parser::make_CON_RARITY("Rarity", loc);

Identified		return yy::Parser::make_CON_IDENTIFIED("Identified", loc);
Corrupted		return yy::Parser::make_CON_CORRUPTED("Corrupted", loc);
ElderItem		return yy::Parser::make_CON_ELDERITEM("ElderItem", loc);
ShaperItem		return yy::Parser::make_CON_SHAPERITEM("ShaperItem", loc);
ShapedMap		return yy::Parser::make_CON_SHAPEDMAP("ShapedMap", loc);

Class			BEGIN(nameList); return yy::Parser::make_CON_CLASS("Class", loc);
BaseType		BEGIN(nameList); return yy::Parser::make_CON_BASETYPE("BaseType", loc);
HasExplicitMod	BEGIN(nameList); return yy::Parser::make_CON_EXPLICIT("HasExplicitMod", loc);

SocketGroup		return yy::Parser::make_CON_SOCKETGROUP("SocketGroup", loc);

SetFontSize		return yy::Parser::make_AC_FONTSIZE("SetFontSize", loc);
SetTextSize		return yy::Parser::make_AC_FONTSIZE("SetFontSize", loc);

SetBorderColor	return yy::Parser::make_AC_BORDERCOLOR("SetBorderColor", loc);
SetTextColor	return yy::Parser::make_AC_TEXTCOLOR("SetTextColor", loc);
SetFontColor	return yy::Parser::make_AC_TEXTCOLOR("SetTextColor", loc);
SetBackgroundColor	return yy::Parser::make_AC_BGCOLOR("SetBackgroundColor", loc);

PlayAlertSound				return yy::Parser::make_AC_SOUND("PlayAlertSound", loc);
PlayAlertSoundPositional	return yy::Parser::make_AC_SOUNDPOSITIONAL("PlayAlertSoundPositional", loc);

CustomAlertSound	return yy::Parser::make_AC_CUSTOMSOUND("CustomAlertSound", loc);
MinimapIcon			return yy::Parser::make_AC_MINIMAPICON("MinimapIcon", loc);
PlayEffect			return yy::Parser::make_AC_PLAYEFFECT("PlayEffect", loc);

DisableDropSound	return yy::Parser::make_AC_DISABLESOUND("DisableDropSound", loc);
Hidden			return yy::Parser::make_AC_HIDDEN("Hidden", loc);

UseStyle		return yy::Parser::make_AC_USESTYLE("UseStyle", loc);

Number			return yy::Parser::make_TYPE_NUMBER(ifpp::VAR_NUMBER, loc);
Color			return yy::Parser::make_TYPE_COLOR(ifpp::VAR_COLOR, loc);
File			return yy::Parser::make_TYPE_FILE(ifpp::VAR_FILE, loc);
List			BEGIN(nameList); return yy::Parser::make_TYPE_LIST(ifpp::VAR_LIST, loc);
Style			return yy::Parser::make_TYPE_STYLE(ifpp::VAR_STYLE, loc);

true			return yy::Parser::make_CONST_BOOL(true, loc);
false			return yy::Parser::make_CONST_BOOL(false, loc);

Normal	return yy::Parser::make_CONST_RARITY(ifpp::Normal, loc);
Magic	return yy::Parser::make_CONST_RARITY(ifpp::Magic, loc);
Rare	return yy::Parser::make_CONST_RARITY(ifpp::Rare, loc);
Unique	return yy::Parser::make_CONST_RARITY(ifpp::Unique, loc);

ItemFilterAlert10	|
ItemFilterAlert11	|
ItemFilterAlert12	|
ItemFilterAlert13	|
ItemFilterAlert14	|
ItemFilterAlert15	|
ItemFilterAlert16	|
ShAlchemy			|
ShBlessed			|
ShChaos				|
ShDivine			|
ShExalted			|
ShFusing			|
ShGeneral			|
ShMirror			|
ShRegal				|
ShVaal				return yy::Parser::make_CONST_SOUND(yytext, loc);

Red		return yy::Parser::make_CONST_COLOR("Red", loc);
Green	return yy::Parser::make_CONST_COLOR("Green", loc);
Blue	return yy::Parser::make_CONST_COLOR("Blue", loc);
Brown	return yy::Parser::make_CONST_COLOR("Brown", loc);
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

<nameList>{blank}+  
<nameList>\n		loc.lines(); BEGIN(INITIAL); return yy::Parser::make_NEWLINE(loc);
<nameList>{idPrefix}{id}+	return yy::Parser::make_VARIABLE(yytext, loc);

<nameList>\"[^"\r\n]*\" {
	std::string s(yytext + 1, yyleng - 2);
	return yy::Parser::make_NAME(s, loc);
}

<nameList>[^ \r\n"]+	return yy::Parser::make_NAME(yytext, loc);

{garbage}*		ctx.criticalAt(loc) << "String \"" << yytext << "\" not recognized!" << std::endl;

<<EOF>> {
	static bool first = true;
	if (first) {
		first = false;
		return yy::Parser::make_NEWLINE(loc);		
	}
	return yy::Parser::make_END(loc);	
}

%%
