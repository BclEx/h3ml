#ifndef __PARSER_H__
#define __PARSER_H__

#define DEFINE_FIXED			0x0001

#define BUILTIN_LINE			1
#define BUILTIN_FILE			2
#define BUILTIN_DATE			3
#define BUILTIN_TIME			4
#define BUILTIN_STDC			5

#define INDENT_IF				0x0001
#define INDENT_ELSE				0x0002
#define INDENT_ELIF				0x0004
#define INDENT_IFDEF			0x0008
#define INDENT_IFNDEF			0x0010

// macro definitions
typedef struct define_s {
	char *name;					// define name
	int	flags;					// define flags
	int	builtin;				// > 0 if builtin define
	int	numparms;				// number of define parameters
	Token *parms;				// define parameters
	Token *tokens;				// macro tokens (possibly containing parm tokens)
	struct define_s	*next;		// next defined macro in a list
	struct define_s	*hashnext;	// next define in the hash chain
} define_t;

// indents used for conditional compilation directives:
// #if, #else, #elif, #ifdef, #ifndef
typedef struct indent_s {
	int type;					// indent type
	int skip;					// true if skipping current indent
	Lexer *script;				// script the indent was in
	struct indent_s	*next;		// next indent on the indent stack
} indent_t;

class Parser {
public:
	// constructor
	Parser();
	Parser(int flags);
	Parser(const char *filename, int flags = 0, bool OSPath = false);
	Parser(const char *ptr, int length, const char *name, int flags = 0);
	~Parser(); // destructor

	int LoadFile(const char *filename, bool OSPath = false); // load a source file
	int LoadMemory(const char *ptr, int length, const char *name); // load a source from the given memory with the given length, NOTE: the ptr is expected to point at a valid C string: ptr[length] == '\0'
	void FreeSource(bool keepDefines = false); // free the current source
	int IsLoaded() const { return Parser::loaded; } // returns true if a source is loaded
	int ReadToken(Token *token); // read a token from the source
	int ExpectTokenString(const char *string); // expect a certain token, reads the token when available
	int	ExpectTokenType(int type, int subtype, Token *token); // expect a certain token type
	int	ExpectAnyToken(Token *token); // expect a token
	int CheckTokenString(const char *string); // returns true if the next token equals the given string and removes the token from the source
	int	CheckTokenType(int type, int subtype, Token *token); // returns true if the next token equals the given type and removes the token from the source
	int	PeekTokenString(const char *string); // returns true if the next token equals the given string but does not remove the token from the source
	int	PeekTokenType(int type, int subtype, Token *token); // returns true if the next token equals the given type but does not remove the token from the source
	int	SkipUntilString(const char *string); // skip tokens until the given token string is read
	int	SkipRestOfLine(); // skip the rest of the current line
	int	SkipBracedSection(bool parseFirstBrace = true); // skip the braced section
	const char *ParseBracedSection(string &out, int tabs, bool parseFirstBrace, char intro, char outro); // parse a braced section into a string
	const char *ParseBracedSectionExact(string &out, int tabs = -1); // parse a braced section into a string, maintaining indents and newlines
	const char *ParseRestOfLine(string &out); // parse the rest of the line
	void UnreadToken(Token *token); // unread the given token
	int ReadTokenOnLine(Token *token); // read a token only if on the current line
	int ParseInt(); // read a signed integer
	bool ParseBool(); // read a boolean
	float ParseFloat(); // read a floating point number
	// parse matrices with floats
	int Parse1DMatrix(int x, float *m);
	int Parse2DMatrix(int y, int x, float *m);
	int Parse3DMatrix(int z, int y, int x, float *m);
	int GetLastWhiteSpace(string &whiteSpace) const; // get the white space before the last read token
	void SetMarker(); // Set a marker in the source file (there is only one marker)
	void GetStringFromMarker(string &out, bool clean = false); // Get the string from the marker to the current position
	int AddDefine(const char *string); // add a define to the source
	void AddBuiltinDefines(); // add builtin defines
	void SetIncludePath(const char *path); // set the source include path
	void SetPunctuations(const punctuation_t *p); // set the punctuation set
	const char *GetPunctuationFromId(int id); // returns a pointer to the punctuation with the given id
	int	GetPunctuationId(const char *p); // get the id for the given punctuation
	void SetFlags(int flags); // set lexer flags
	int	GetFlags() const; // get lexer flags
	const char *GetFileName() const; // returns the current filename
	const int GetFileOffset() const; // get current offset in current script
	const int64 GetFileTime() const; // get file time for current script
	const int GetLineNum() const; // returns the current line number
	void Error(const char *str, ...) const; // print an error message
	void Warning(const char *str, ...) const; // print a warning message

	bool EndOfFile(); // returns true if at the end of the file
	static int AddGlobalDefine(const char *string); // add a global define that will be added to all opened sources
	static int RemoveGlobalDefine(const char *name); // remove the given global define

	static void	RemoveAllGlobalDefines(); // remove all global defines
	static void SetBaseFolder(const char *path); // set the base folder to load files from

private:
	int loaded;						// set when a source file is loaded from file or memory
	string	filename;					// file name of the script
	string 	includepath;				// path to include files
	bool OSPath;						// true if the file was loaded from an OS path
	const punctuation_t *punctuations;			// punctuations to use
	int flags;						// flags used for script parsing
	Lexer *scriptstack;				// stack with scripts of the source
	Token *tokens;						// tokens to read first
	define_t *defines;					// list with macro definitions
	define_t **definehash;					// hash chain with defines
	indent_t *indentstack;				// stack with indents
	int skip;						// > 0 if skipping conditional code
	const char *marker_p;

	static define_t *globaldefines;				// list with global defines added to every source loaded

private:
	void PushIndent(int type, int skip);
	void PopIndent(int *type, int *skip);
	void PushScript(Lexer *script);
	int ReadSourceToken(Token *token);
	int ReadLine(Token *token);
	int UnreadSourceToken(Token *token);
	int ReadDefineParms(define_t *define, Token **parms, int maxparms);
	int StringizeTokens(Token *tokens, Token *token);
	int MergeTokens(Token *t1, Token *t2);
	int ExpandBuiltinDefine(Token *deftoken, define_t *define, Token **firsttoken, Token **lasttoken);
	int ExpandDefine(Token *deftoken, define_t *define, Token **firsttoken, Token **lasttoken);
	int ExpandDefineIntoSource(Token *deftoken, define_t *define);
	void AddGlobalDefinesToSource();
	define_t *CopyDefine(define_t *define);
	define_t *FindHashedDefine(define_t **definehash, const char *name);
	int FindDefineParm(define_t *define, const char *name);
	void AddDefineToHash(define_t *define, define_t **definehash);
	static void PrintDefine(define_t *define);
	static void FreeDefine(define_t *define);
	static define_t *FindDefine(define_t *defines, const char *name);
	static define_t *DefineFromString(const char *string);
	define_t *CopyFirstDefine();
	int Directive_include();
	int Directive_undef();
	int Directive_if_def(int type);
	int Directive_ifdef();
	int Directive_ifndef();
	int Directive_else();
	int Directive_endif();
	int EvaluateTokens(Token *tokens, signed long int *intvalue, double *floatvalue, int integer);
	int Evaluate(signed long int *intvalue, double *floatvalue, int integer);
	int DollarEvaluate(signed long int *intvalue, double *floatvalue, int integer);
	int Directive_define();
	int Directive_elif();
	int Directive_if();
	int Directive_line();
	int Directive_error();
	int Directive_warning();
	int Directive_pragma();
	void UnreadSignToken();
	int Directive_eval();
	int Directive_evalfloat();
	int ReadDirective();
	int DollarDirective_evalint();
	int DollarDirective_evalfloat();
	int ReadDollarDirective();
};

__inline const char *Parser::GetFileName() const {
	return (Parser::scriptstack ? Parser::scriptstack->GetFileName() : 0);
}

__inline const int Parser::GetFileOffset() const {
	return (Parser::scriptstack ? Parser::scriptstack->GetFileOffset() : 0);
}

__inline const int64 Parser::GetFileTime() const {
	return (Parser::scriptstack ? Parser::scriptstack->GetFileTime() : 0);
}

__inline const int Parser::GetLineNum() const {
	return (Parser::scriptstack ? Parser::scriptstack->GetLineNum() : 0);
}

#endif /* !__PARSER_H__ */
