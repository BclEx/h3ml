#ifndef __TOKENPARSER_H__
#define __TOKENPARSER_H__

class BinaryToken {
public:
	BinaryToken() {
		tokenType = 0;
		tokenSubType = 0;
	}
	BinaryToken(const Token &tok) {
		token = tok.c_str();
		tokenType = tok.type;
		tokenSubType = tok.subtype;
	}
	bool operator==(const BinaryToken &b) const {
		return (tokenType == b.tokenType && tokenSubType == b.tokenSubType && !token.compare(b.token));
	}
	//void Read(File *inFile) {
	//	inFile->ReadString(token);
	//	inFile->ReadBig(tokenType);
	//	inFile->ReadBig(tokenSubType);
	//}
	//void Write(File *inFile) {
	//	inFile->WriteString(token);
	//	inFile->WriteBig(tokenType);
	//	inFile->WriteBig(tokenSubType);
	//}
	string token;
	int8 tokenType;
	short tokenSubType;
};

class TokenIndexes {
public:
	TokenIndexes() {}
	void Clear() {
		_tokenIndexes.clear();
	}
	void Append(short sdx) {
		return _tokenIndexes.push_back(sdx);
	}
	int Num() {
		return _tokenIndexes.size();
	}
	void SetNum(int num) {
		_tokenIndexes.resize(num);
	}
	short &operator[](const int index) {
		return _tokenIndexes[index];
	}
	void SetName(const char *name) {
		_fileName = name;
	}
	const char *GetName() {
		return _fileName.c_str();
	}
	//void Write(File *outFile) {
	//	outFile->WriteString(_fileName);
	//	outFile->WriteBig((int)_tokenIndexes.size());
	//	outFile->WriteBigArray(_tokenIndexes.begin(), _tokenIndexes.size());
	//}
	//void Read(File *inFile) {
	//	inFile->ReadString(_fileName);
	//	int num;
	//	inFile->ReadBig(num);
	//	_tokenIndexes.resize(num);
	//	inFile->ReadBigArray(_tokenIndexes.Ptr(), num);
	//}
private:
	vector<short> _tokenIndexes;
	string _fileName;
};

class TokenParser {
public:
	TokenParser() {
		_timeStamp = -1; // FILE_NOT_FOUND_TIMESTAMP;
		_preloaded = false;
		_currentToken = 0;
		_currentTokenList = 0;
	}
	~TokenParser() {
		Clear();
	}
	void Clear() {
		_tokens.clear();
		_guiTokenIndexes.clear();
		_currentToken = 0;
		_currentTokenList = -1;
		_preloaded = false;
	}
	void LoadFromFile(const char *filename);
	void WriteToFile(const char *filename);
	void LoadFromParser(Parser &parser, const char *guiName);

	bool StartParsing(const char *fileName);
	void DoneParsing() { _currentTokenList = -1; }

	bool IsLoaded() { return _tokens.size() > 0; }
	bool ReadToken(Token * tok);
	int	ExpectTokenString(const char *string);
	int	ExpectTokenType(int type, int subtype, Token *token);
	int ExpectAnyToken(Token *token);
	void SetMarker() {}
	void UnreadToken(const Token *token);
	void Error(const char *str, ...);
	void Warning(const char *str, ...);
	int ParseInt();
	bool ParseBool();
	float ParseFloat(bool *errorFlag = NULL);
	void UpdateTimeStamp(int64 &t) {
		if (t > _timeStamp)
			_timeStamp = t;
	}
private:
	vector<BinaryToken> _tokens;
	vector<TokenIndexes> _guiTokenIndexes;
	int _currentToken;
	int _currentTokenList;
	int64 _timeStamp;
	bool _preloaded;
};

#endif /* !__TOKENPARSER_H__ */
