#ifndef __TOKEN_H__
#define __TOKEN_H__

// token types
#define TT_STRING					1		// string
#define TT_LITERAL					2		// literal
#define TT_NUMBER					3		// number
#define TT_NAME						4		// name
#define TT_PUNCTUATION				5		// punctuation

// number sub types
#define TT_INTEGER					0x00001		// integer
#define TT_DECIMAL					0x00002		// decimal number
#define TT_HEX						0x00004		// hexadecimal number
#define TT_OCTAL					0x00008		// octal number
#define TT_BINARY					0x00010		// binary number
#define TT_LONG						0x00020		// long int
#define TT_UNSIGNED					0x00040		// unsigned int
#define TT_FLOAT					0x00080		// floating point number
#define TT_SINGLE_PRECISION			0x00100		// float
#define TT_DOUBLE_PRECISION			0x00200		// double
#define TT_EXTENDED_PRECISION		0x00400		// long double
#define TT_INFINITE					0x00800		// infinite 1.#INF
#define TT_INDEFINITE				0x01000		// indefinite 1.#IND
#define TT_NAN						0x02000		// NaN
#define TT_IPADDRESS				0x04000		// ip address
#define TT_IPPORT					0x08000		// ip port
#define TT_VALUESVALID				0x10000		// set if intvalue and floatvalue are valid

// string sub type is the length of the string
// literal sub type is the ASCII code
// punctuation sub type is the punctuation id
// name sub type is the length of the name

class Token : public string {
	friend class Parser;
	friend class Lexer;

public:
	int type;								// token type
	int subtype;							// token sub type
	int line;								// line in script the token was on
	int linesCrossed;						// number of lines crossed in white space before token
	int flags;								// token flags, used for recursive defines

public:
	Token();
	Token(const Token *token);
	~Token();

	void operator=(const string &text);
	void operator=(const char *text);

	double GetDoubleValue();			// double value of TT_NUMBER
	float GetFloatValue();				// float value of TT_NUMBER
	unsigned long GetUnsignedLongValue();// unsigned long value of TT_NUMBER
	int GetIntValue();					// int value of TT_NUMBER
	int WhiteSpaceBeforeToken() const;	// returns length of whitespace before token
	void ClearTokenWhiteSpace();		// forget whitespace before token

	void NumberValue();					// calculate values for a TT_NUMBER

private:
	unsigned long _intvalue;			// integer value
	double _floatvalue;					// floating point value
	const char *_whiteSpaceStart_p;		// start of white space before token, only used by idLexer
	const char *_whiteSpaceEnd_p;		// end of white space before token, only used by idLexer
	Token *_next;						// next token in chain, only used by idParser

	void AppendDirty(const char a);		// append character without adding trailing zero
};

__inline Token::Token() : type(), subtype(), line(), linesCrossed(), flags() {
}

__inline Token::Token(const Token *token) {
	*this = *token;
}

__inline Token::~Token() {
}

__inline void Token::operator=(const char *text) {
	*static_cast<string *>(this) = text;
}

__inline void Token::operator=(const string &text) {
	*static_cast<string *>(this) = text;
}

__inline double Token::GetDoubleValue() {
	if (type != TT_NUMBER)
		return 0.0;
	if (!(subtype & TT_VALUESVALID))
		NumberValue();
	return _floatvalue;
}

__inline float Token::GetFloatValue() {
	return (float)GetDoubleValue();
}

__inline unsigned long Token::GetUnsignedLongValue() {
	if (type != TT_NUMBER)
		return 0;
	if (!(subtype & TT_VALUESVALID))
		NumberValue();
	return _intvalue;
}

__inline int Token::GetIntValue() {
	return (int)GetUnsignedLongValue();
}

__inline int Token::WhiteSpaceBeforeToken() const {
	return (_whiteSpaceEnd_p > _whiteSpaceStart_p);
}

__inline void Token::AppendDirty(const char a) {
	//EnsureAlloced(length() + 2, true);
	append((const char *)a, 1);
}

#endif /* !__TOKEN_H__ */
