#ifndef IFPP_PARSER_WRAPPER_H
#define IFPP_PARSER_WRAPPER_H

#include "Context.h"
#include "Logger.h"

// This allows us to call generated lexer and parser while not having them to compile with strict GCC options.

namespace ifpp {

void parse(Context & ctx, Logger & log, bool debugLex, bool debugParse);

}

#endif