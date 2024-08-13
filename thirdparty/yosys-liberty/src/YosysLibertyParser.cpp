/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Claire Xenia Wolf <claire@yosyshq.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "YosysLibertyParser.h"

#include <istream>
#include "YosysLibertyException.h"

namespace Yosys {

LibertyAst::~LibertyAst()
{
  for (auto child : children)
    delete child;
  children.clear();
}

LibertyAst *LibertyAst::find(std::string name)
{
  for (auto child : children)
    if (child->id == name)
      return child;
  return NULL;
}

void LibertyParser::error() {
  throw naja::liberty::YosysLibertyException("LibertyParser error");
}

void LibertyParser::error(const std::string &str) {
  throw naja::liberty::YosysLibertyException(str);
}

int LibertyParser::lexer(std::string &str)
{
	int c;

	// eat whitespace
	do {
		c = f.get();
	} while (c == ' ' || c == '\t' || c == '\r');

	// search for identifiers, numbers, plus or minus.
	if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || c == '_' || c == '-' || c == '+' || c == '.') {
		str = static_cast<char>(c);
		while (1) {
			c = f.get();
			if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || c == '_' || c == '-' || c == '+' || c == '.')
				str += c;
			else
				break;
		}
		f.unget();
		if (str == "+" || str == "-") {
			/* Single operator is not an identifier */
			// fprintf(stderr, "LEX: char >>%s<<\n", str.c_str());
			return str[0];
		}
		else {
			// fprintf(stderr, "LEX: identifier >>%s<<\n", str.c_str());
			return 'v';
		}
	}

	// if it wasn't an identifer, number of array range,
	// maybe it's a string?
	if (c == '"') {
		str = "";
		while (1) {
			c = f.get();
			if (c == '\n')
				line++;
			if (c == '"')
				break;
			str += c;
		}
		// fprintf(stderr, "LEX: string >>%s<<\n", str.c_str());
		return 'v';
	}

	// if it wasn't a string, perhaps it's a comment or a forward slash?
	if (c == '/') {
		c = f.get();
		if (c == '*') {         // start of '/*' block comment
			int last_c = 0;
			while (c > 0 && (last_c != '*' || c != '/')) {
				last_c = c;
				c = f.get();
				if (c == '\n')
					line++;
			}
			return lexer(str);
		} else if (c == '/') {  // start of '//' line comment
			while (c > 0 && c != '\n')
				c = f.get();
			line++;
			return lexer(str);
		}
		f.unget();
		// fprintf(stderr, "LEX: char >>/<<\n");
		return '/';             // a single '/' charater.
	}

	// check for a backslash
	if (c == '\\') {
		c = f.get();		
		if (c == '\r')
			c = f.get();
		if (c == '\n') {
			line++;
			return lexer(str);
		}
		f.unget();
		return '\\';
	}

	// check for a new line
	if (c == '\n') {
		line++;
		return 'n';
	}

	// anything else, such as ';' will get passed
	// through as literal items.

	// if (c >= 32 && c < 255)
	// 	fprintf(stderr, "LEX: char >>%c<<\n", c);
	// else
	// 	fprintf(stderr, "LEX: char %d\n", c);
	return c;
}

LibertyAst *LibertyParser::parse()
{
	std::string str;

	int tok = lexer(str);

	// there are liberty files in the wild that
	// have superfluous ';' at the end of
	// a  { ... }. We simply ignore a ';' here.
	// and get to the next statement.

	while ((tok == 'n') || (tok == ';'))
		tok = lexer(str);

	if (tok == '}' || tok < 0)
		return NULL;

	if (tok != 'v') {
		std::string eReport;
		switch(tok)
		{
		case 'n':
			error("Unexpected newline.");
			break;
		case '[':
		case ']':
		case '}':
		case '{':
		case '\"':
		case ':':
			eReport = "Unexpected '";
			eReport += static_cast<char>(tok);
			eReport += "'.";
			error(eReport);
			break;
		default:
			error();
		}
	}

	LibertyAst *ast = new LibertyAst;
	ast->id = str;

	while (1)
	{
		tok = lexer(str);

		// allow both ';' and new lines to 
		// terminate a statement.
		if ((tok == ';') || (tok == 'n'))
			break;

		if (tok == ':' && ast->value.empty()) {
			tok = lexer(ast->value);
			if (tok == 'v') {
    				tok = lexer(str);
			}
			while (tok == '+' || tok == '-' || tok == '*' || tok == '/' || tok == '!') {
				ast->value += tok;
				tok = lexer(str);
				if (tok != 'v')
					error();
				ast->value += str;
				tok = lexer(str);
			}
			
			// In a liberty file, all key : value pairs should end in ';'
			// However, there are some liberty files in the wild that
			// just have a newline. We'll be kind and accept a newline
			// instead of the ';' too..
			if ((tok == ';') || (tok == 'n'))
				break;
			else
				error();
			continue;
		}

		if (tok == '(') {
			while (1) {
				std::string arg;
				tok = lexer(arg);
				if (tok == ',')
					continue;
				if (tok == ')')
					break;
				
				// FIXME: the AST needs to be extended to store
				//        these vector ranges.
				if (tok == '[')
				{
					// parse vector range [A] or [A:B]
					std::string arg;
					tok = lexer(arg);
					if (tok != 'v')
					{
						// expected a vector array index
						error("Expected a number.");
					}
					else
					{
						// fixme: check for number A
					}
					tok = lexer(arg);
					// optionally check for : in case of [A:B]
					// if it isn't we just expect ']'
					// as we have [A]
					if (tok == ':')
					{
						tok = lexer(arg);
						if (tok != 'v')
						{
							// expected a vector array index
							error("Expected a number.");
						}
						else
						{
							// fixme: check for number B
							tok = lexer(arg);                            
						}
					}
					// expect a closing bracket of array range
					if (tok != ']')
					{
						error("Expected ']' on array range.");
					}
					continue;           
				}
				if (tok != 'v') {
					std::string eReport;
					switch(tok)
					{
					case 'n':
					  continue;
					case '[':
					case ']':
					case '}':
					case '{':
					case '\"':
					case ':':
						eReport = "Unexpected '";
						eReport += static_cast<char>(tok);
						eReport += "'.";
						error(eReport);
						break;
					default:
						error();
					}
				}
				ast->args.push_back(arg);
			}
			continue;
		}

		if (tok == '{') {
			while (1) {
				LibertyAst *child = parse();
				if (child == NULL)
					break;
				ast->children.push_back(child);
			}
			break;
		}

		error();
	}

	return ast;
}

}