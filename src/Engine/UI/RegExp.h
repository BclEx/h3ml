#ifndef __REGEXP_H__
#define __REGEXP_H__

class TokenParser;
class Window;
class WinVar;
class Register
{
public:
	Register();
	Register(const char *p, int t);

	enum REGTYPE { VEC4 = 0, FLOAT, BOOL, INT, STRING, VEC2, VEC3, RECTANGLE, NUMTYPES };
	static int REGCOUNT[NUMTYPES];

	bool enabled;
	short type;
	string name;
	int regCount;
	unsigned short regs[4];
	WinVar *var;

	void SetToRegs(float *registers);
	void GetFromRegs(float *registers);
	void CopyRegs(Register *src);
	void Enable(bool b) { enabled = b; }
};

__inline Register::Register()
{
}

__inline Register::Register(const char *p, int t)
{
	name = p;
	type = t;
	assert(t >= 0 && t < NUMTYPES);
	regCount = REGCOUNT[t];
	enabled = (type == STRING ? false : true);
	var = nullptr;
};

__inline void Register::CopyRegs(Register *src)
{
	regs[0] = src->regs[0];
	regs[1] = src->regs[1];
	regs[2] = src->regs[2];
	regs[3] = src->regs[3];
}

class RegisterList
{
public:
	RegisterList();
	~RegisterList();

	void AddReg(const char *name, int type, TokenParser *src, Window *win, WinVar *var);
	void AddReg(const char *name, int type, vec4 data, Window *win, WinVar *var);

	Register *FindReg(const char *name);
	void SetToRegs(float *registers);
	void GetFromRegs(float *registers);
	void Reset();

private:
	vector<Register *> _regs;
	map<int, int> _regHash;
};

__inline RegisterList::RegisterList()
{
	//_regs.SetGranularity(4);
	//_regHash.SetGranularity(4);
	//_regHash.Clear(32, 4);
}

__inline RegisterList::~RegisterList() {
}

#endif /* !__REGEXP_H__ */
