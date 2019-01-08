extern const int IFPP_VERSION_MAJOR = 2;
extern const int IFPP_VERSION_MINOR = 0;
extern const int IFPP_VERSION_PATCH = 0;
const char * POE_VERSION = "3.4";
// TODO make this more portable.
// %HOMEPATH% has the preceding slash
const char * DOCS_FOLDER = "C:%HOMEPATH%\\Documents\\My Games\\Path of Exile\\";

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>

#include <windows.h>
#include <tchar.h>

#include "Types.h"
#include "Logger.h"
#include "Context.h"
#include "ParserWrapper.h"
//#include "Compiler.h"

int main(int argc, char ** argv) {
	std::string inFile(""), outFile(""), logFile("");

	bool dLex = false;
	bool dParse = false;
	bool dPartial = false;
	bool dParseOnly = false;
	bool documents = false;

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-Dlex")) dLex = true;
		else if (!strcmp(argv[i], "-c")) logFile = "-";
		else if (!strcmp(argv[i], "-d")) documents = true;
		else if (!strcmp(argv[i], "-Dparse")) dParse = true;
		else if (!strcmp(argv[i], "-Dpartial")) dPartial = true;
		else if (!strcmp(argv[i], "-DparseOnly")) dParseOnly = true;
		else if (inFile == "") inFile = argv[i];
		else if (outFile == "") outFile = argv[i];
		else if (logFile == "") logFile = argv[i];
	}

	if (inFile == "") {
		std::cerr << "Error: No input file specified. Nothing to do." << std::endl;
		std::cerr << "Use: ifpp <input file> [output file] [log file]." << std::endl;
		return EXIT_FAILURE;
	}

	std::string baseName = inFile.substr(0, inFile.find_last_of('.'));
	if (outFile == "") outFile = baseName + ".filter";
	if (logFile == "") logFile = baseName + ".log";

	// Exception handling depends on logStream to be open.
	std::ofstream logStream;
	if (logFile != "-") {
		logStream.open(logFile, std::ios_base::out);
		if (!logStream.is_open()) {
			std::cerr << "Error: Unable to open log file \"" << logFile << "\"!" << std::endl;
			std::cerr << "Processing aborted." << std::endl;
			return EXIT_FAILURE;
		}
	}
	ifpp::Logger log(logFile == "-" ? std::cerr : logStream);
	
	try {
		log.message() << "Item Filter PreProcessor, version "
			<< IFPP_VERSION_MAJOR << '.' << IFPP_VERSION_MINOR << '.' << IFPP_VERSION_PATCH << std::endl;
		log.message() << "Designed for compatibility with Path of Exile version "
			<< POE_VERSION << ", but should work with any later releases." << std::endl;
		log.message() << "Input file \"" << inFile << "\", output file \"" << outFile << "\", log file \"" << logFile << "\"." << std::endl;
		log.message() << std::endl;
		
		ifpp::FilterIFPP inFilter;
		ifpp::Context ctx(inFile, inFilter, log);
		
		ifpp::parse(ctx, log, dLex, dParse);
		
		log.message() << "Parsing finished." << std::endl;
		log.message() << "\t" << ctx.countDef << " definitions" << std::endl;
		log.message() << "\t" << ctx.countIns << " instructions" << std::endl;
		log.message() << "\t" << ctx.countRule << " rules" << std::endl << std::endl;
		
		if (dPartial) {
			std::ofstream parsedStream(baseName + ".parsed.ifpp", std::ios_base::out);
			ifpp::print(parsedStream, ifpp::PRINT_IFPP, inFilter);
			parsedStream.close();
		}
		
		if (dParseOnly) {
			logStream.close();
			return EXIT_SUCCESS;
		}
		
		ifpp::FilterNative outFilter;
		
		std::ofstream partialStream;
		if (dPartial) partialStream.open(baseName + ".partial.ifpp", std::ios_base::out);
		
		ifpp::Compiler c(log, dPartial, partialStream);
		
		log.message() << "Compiler initialized." << std::endl;
		log.message() << "Compiling filter..." << std::endl;
		c.Compile(inFilter, outFilter);
		log.message() << "Compiling done." << std::endl;
		log.message() << '\t' << outFilter.size() << " native rules" << std::endl << std::endl;
		
		if (dPartial) partialStream.close();
		
		log.message() << "Writing native filter to \"" << outFile << "\"..." << std::endl;
		std::ofstream outStream(outFile, std::ios_base::out);
		ifpp::print(outStream, ifpp::PRINT_NATIVE, outFilter);
		outStream.close();
		
		/*
		
		if (documents) {
			size_t pos = baseName.find_last_of("/\\");
			std::string filename;
			if (pos != std::string::npos) {
				filename = baseName.substr(pos + 1);
			} else {
				filename = baseName;
			}
			
			std::stringstream loc;
			loc << DOCS_FOLDER << filename << ".filter";
			
			TCHAR bufExpanded[256];
			ExpandEnvironmentStrings(loc.str().c_str(), bufExpanded, 256);
			
			log.message() << "Writing filter to \"" << bufExpanded << "\"..." << std::endl;
			outStream.open(bufExpanded, std::ios_base::out);
			if (!outStream.is_open()) {
				log.message() << "Failed to open file!" << std::endl;
			}
			ifpp::print(outStream, ifpp::PRINT_NATIVE, outFilter);
			outStream.close();
		}
		
		log.message() << "Done." << std::endl << std::endl;
		
		log.message() << "IFPP finished." << std::endl;
		log.message() << "\t" << log.numWarnings << " warnings" << std::endl;
		log.message() << "\t" << log.numErrors << " errors" << std::endl;
		log.message() << "\t" << log.numCritical << " critical errors" << std::endl;
		*/
		logStream.close();
		return EXIT_SUCCESS;
	}
	catch (ifpp::InternalError & e) {
		log.message() << "INTERNAL ERROR: " << e.what() << std::endl;
		log.message() << "File: " << e.file << " line: " << e.line << std::endl;
		log.message() << "Processing aborted. Please report this issue at [link]." << std::endl;
		logStream.close();
		return EXIT_FAILURE;
	}
	catch (std::runtime_error & e) {
		log.error() << e.what() << std::endl;
		log.message() << "Processing aborted.";
		logStream.close();
		return EXIT_FAILURE;
	}
}