#ifndef IFPP_LOGGER_H
#define IFPP_LOGGER_H

#include <iostream>

namespace ifpp {

class Logger {
public:
	Logger(std::ostream & out) :
		numWarnings(0), numErrors(0), numCritical(0), log(out) {}
	
	std::ostream & message() { return log; }
	std::ostream & messageAppend() { return log; }

	std::ostream & warning() { ++numWarnings; return log << "Warning: ";}
	std::ostream & warningAppend() { return log; }

	std::ostream & error() { ++numErrors; return log << "Error: "; }
	std::ostream & errorAppend() { return log; }

	std::ostream & critical() { ++numCritical; return log << "CRITICAL ERROR: "; }
	std::ostream & criticalAppend() { return log; }

	int numWarnings;
	int numErrors;
	int numCritical;

private:
	std::ostream & log;
};

}

#endif