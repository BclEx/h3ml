#ifndef __LEXER_H__
#define __LEXER_H__

// lexer flags
typedef enum {
	LEXFL_NOERRORS = BIT(0),	// don't print any errors
	LEXFL_NOWARNINGS = BIT(1),	// don't print any warnings
	LEXFL_NOFATALERRORS = BIT(2),	// errors aren't fatal
	LEXFL_NOSTRINGCONCAT = BIT(3),	// multiple strings seperated by whitespaces are not concatenated
	LEXFL_NOSTRINGESCAPECHARS = BIT(4),	// no escape characters inside strings
	LEXFL_NODOLLARPRECOMPILE = BIT(5),	// don't use the $ sign for precompilation
	LEXFL_NOBASEINCLUDES = BIT(6),	// don't include files embraced with < >
	LEXFL_ALLOWPATHNAMES = BIT(7),	// allow path seperators in names
	LEXFL_ALLOWNUMBERNAMES = BIT(8),	// allow names to start with a number
	LEXFL_ALLOWIPADDRESSES = BIT(9),	// allow ip addresses to be parsed as numbers
	LEXFL_ALLOWFLOATEXCEPTIONS = BIT(10),	// allow float exceptions like 1.#INF or 1.#IND to be parsed
	LEXFL_ALLOWMULTICHARLITERALS = BIT(11),	// allow multi character literals
	LEXFL_ALLOWBACKSLASHSTRINGCONCAT = BIT(12),	// allow multiple strings seperated by '\' to be concatenated
	LEXFL_ONLYSTRINGS = BIT(13)	// parse as whitespace deliminated strings (quoted strings keep quotes)
} lexerFlags_t;

// punctuation ids
#define P_RSHIFT_ASSIGN				1
#define P_LSHIFT_ASSIGN				2
#define P_PARMS						3
#define P_PRECOMPMERGE				4

#define P_LOGIC_AND					5
#define P_LOGIC_OR					6
#define P_LOGIC_GEQ					7
#define P_LOGIC_LEQ					8
#define P_LOGIC_EQ					9
#define P_LOGIC_UNEQ				10

#define P_MUL_ASSIGN				11
#define P_DIV_ASSIGN				12
#define P_MOD_ASSIGN				13
#define P_ADD_ASSIGN				14
#define P_SUB_ASSIGN				15
#define P_INC						16
#define P_DEC						17

#define P_BIN_AND_ASSIGN			18
#define P_BIN_OR_ASSIGN				19
#define P_BIN_XOR_ASSIGN			20
#define P_RSHIFT					21
#define P_LSHIFT					22

#define P_POINTERREF				23
#define P_CPP1						24
#define P_CPP2						25
#define P_MUL						26
#define P_DIV						27
#define P_MOD						28
#define P_ADD						29
#define P_SUB						30
#define P_ASSIGN					31

#define P_BIN_AND					32
#define P_BIN_OR					33
#define P_BIN_XOR					34
#define P_BIN_NOT					35

#define P_LOGIC_NOT					36
#define P_LOGIC_GREATER				37
#define P_LOGIC_LESS				38

#define P_REF						39
#define P_COMMA						40
#define P_SEMICOLON					41
#define P_COLON						42
#define P_QUESTIONMARK				43

#define P_PARENTHESESOPEN			44
#define P_PARENTHESESCLOSE			45
#define P_BRACEOPEN					46
#define P_BRACECLOSE				47
#define P_SQBRACKETOPEN				48
#define P_SQBRACKETCLOSE			49
#define P_BACKSLASH					50

#define P_PRECOMP					51
#define P_DOLLAR					52

// punctuation
typedef struct punctuation_s
{
	char *p;						// punctuation character(s)
	int n;							// punctuation id
} punctuation_t;

class Lexer {
	friend class idParser;
public:
	// constructor
	Lexer();
	Lexer(int flags);
	Lexer(const char *filename, int flags = 0, bool OSPath = false);
	Lexer(const char *ptr, int length, const char *name, int flags = 0);
	~Lexer(); // destructor
	int LoadFile(const char *filename, bool OSPath = false); // load a script from the given file at the given offset with the given length
	// load a script from the given memory with the given length and a specified line offset,
	// so source strings extracted from a file can still refer to proper line numbers in the file
	// NOTE: the ptr is expected to point at a valid C string: ptr[length] == '\0'
	int	LoadMemory(const char *ptr, int length, const char *name, int startLine = 1);
	void FreeSource(); // free the script
	int IsLoaded() { return Lexer::_loaded; }; // returns true if a script is loaded
	int ReadToken(Token *token); // read a token
	int ExpectTokenString(const char *string); // expect a certain token, reads the token when available
	int ExpectTokenType(int type, int subtype, Token *token); // expect a certain token type
	int ExpectAnyToken(Token *token); // expect a token
	int CheckTokenString(const char *string); // returns true when the token is available
	int CheckTokenType(int type, int subtype, Token *token); // returns true an reads the token when a token with the given type is available
	int PeekTokenString(const char *string); // returns true if the next token equals the given string but does not remove the token from the source 
	int PeekTokenType(int type, int subtype, Token *token); // returns true if the next token equals the given type but does not remove the token from the source
	int SkipUntilString(const char *string); // skip tokens until the given token string is read
	int SkipRestOfLine(); // skip the rest of the current line
	int SkipBracedSection(bool parseFirstBrace = true); // skip the braced section
	bool SkipWhiteSpace(bool currentLine); // skips spaces, tabs, C-like comments etc. Returns false if there is no token left to read.
	void UnreadToken(const Token *token); // unread the given token
	int ReadTokenOnLine(Token *token); // read a token only if on the same line

	const char *ReadRestOfLine(string &out); //Returns the rest of the current line
	int ParseInt(); // read a signed integer
	bool ParseBool(); // read a boolean
	float ParseFloat(bool *errorFlag = nullptr); // read a floating point number.  If errorFlag is NULL, a non-numeric token will issue an Error().  If it isn't NULL, it will issue a Warning() and set *errorFlag = true
	// parse matrices with floats
	int Parse1DMatrix(int x, float *m);
	int Parse2DMatrix(int y, int x, float *m);
	int Parse3DMatrix(int z, int y, int x, float *m);
	const char *ParseBracedSection(string &out); // parse a braced section into a string
	const char *ParseBracedSectionExact(string &out, int tabs = -1); // parse a braced section into a string, maintaining indents and newlines
	const char *ParseRestOfLine(string &out); // parse the rest of the line
	const char *ParseCompleteLine(string &out); // pulls the entire line, including the \n at the end
	int GetLastWhiteSpace(string &whiteSpace) const; // retrieves the white space characters before the last read token
	int GetLastWhiteSpaceStart() const; // returns start index into text buffer of last white space
	int GetLastWhiteSpaceEnd() const; // returns end index into text buffer of last white space
	void SetPunctuations(const punctuation_t *p); // set an array with punctuations, NULL restores default C/C++ set, see default_punctuations for an example

	const char *GetPunctuationFromId(int id); // returns a pointer to the punctuation with the given id
	int GetPunctuationId(const char *p); // get the id for the given punctuation
	void SetFlags(int flags); // set lexer flags
	int GetFlags(); // get lexer flags
	void Reset(); // reset the lexer
	bool EndOfFile(); // returns true if at the end of the file
	const char *GetFileName(); // returns the current filename
	const int GetFileOffset(); // get offset in script
	const int64 GetFileTime(); // get file time
	const int GetLineNum(); // returns the current line number
	void Error(const char *str, ...); // print an error message
	void Warning(const char *str, ...); // print a warning message
	bool HadError() const; // returns true if Error() was called with LEXFL_NOFATALERRORS or LEXFL_NOERRORS set

	static void SetBaseFolder(const char *path); // set the base folder to load files from

private:
	int _loaded;					// set when a script file is loaded from file or memory
	string _filename;				// file name of the script
	int _allocated;				// true if buffer memory was allocated
	const char *_buffer;					// buffer containing the script
	const char *_script_p;				// current pointer in the script
	const char *_end_p;					// pointer to the end of the script
	const char *_lastScript_p;			// script pointer before reading token
	const char *_whiteSpaceStart_p;		// start of last white space
	const char *_whiteSpaceEnd_p;		// end of last white space
	int64 _fileTime;				// file time
	int _length;					// length of the script in bytes
	int _line;					// current line in script
	int _lastline;				// line before reading token
	int _tokenavailable;			// set by unreadToken
	int _flags;					// several script flags
	const punctuation_t *_punctuations;		// the punctuations used in the script
	int *_punctuationtable;		// ASCII table with punctuations
	int *_nextpunctuation;		// next punctuation in chain
	Token _token;					// available token
	Lexer *_next;					// next script in a chain
	bool _hadError;				// set by idLexer::Error, even if the error is supressed

	static char _baseFolder[256];		// base folder to load files from

private:
	void CreatePunctuationTable(const punctuation_t *punctuations);
	int ReadWhiteSpace();
	int ReadEscapeCharacter(char *ch);
	int ReadString(Token *token, int quote);
	int ReadName(Token *token);
	int ReadNumber(Token *token);
	int ReadPunctuation(Token *token);
	int ReadPrimitive(Token *token);
	int CheckString(const char *str) const;
	int NumLinesCrossed();
};

__inline const char *Lexer::GetFileName() {
	return Lexer::_filename.c_str();
}

__inline const int Lexer::GetFileOffset() {
	return Lexer::_script_p - Lexer::_buffer;
}

__inline const int64 Lexer::GetFileTime() {
	return Lexer::_fileTime;
}

__inline const int Lexer::GetLineNum() {
	return Lexer::_line;
}

__inline void Lexer::SetFlags(int flags) {
	Lexer::_flags = flags;
}

__inline int Lexer::GetFlags() {
	return Lexer::_flags;
}

#endif /* !__LEXER_H__ */

