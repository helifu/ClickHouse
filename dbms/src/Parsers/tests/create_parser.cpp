#include <iostream>

#include <mysqlxx/mysqlxx.h>

#include <DB/Parsers/ASTCreateQuery.h>
#include <DB/Parsers/ParserCreateQuery.h>
#include <DB/Parsers/formatAST.h>


int main(int argc, char ** argv)
{
	DB::ParserCreateQuery parser;
	DB::ASTPtr ast;
	std::string input = "CREATE TABLE hits (URL String, UserAgentMinor2 FixedString(2), EventTime DateTime) ENGINE = Log";
	const char * expected = "";

	const char * begin = input.data();
	const char * end = begin + input.size();
	const char * pos = begin;

	if (parser.parse(pos, end, ast, expected))
	{
		std::cout << "Success." << std::endl;
		DB::formatAST(*ast, std::cout);
		std::cout << std::endl;

		std::cout << std::endl << ast->getTreeID() << std::endl;
	}
	else
	{
		std::cout << "Failed at position " << (pos - begin) << ": "
			<< mysqlxx::quote << input.substr(pos - begin, 10)
			<< ", expected " << expected << "." << std::endl;
	}

	return 0;
}
