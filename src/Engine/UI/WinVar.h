#ifndef __WINVAR_H__
#define __WINVAR_H__

#include "Rectangle.h"

static const char *VAR_GUIPREFIX = "gui::";
static const int VAR_GUIPREFIX_LEN = strlen(VAR_GUIPREFIX);

class Window;
class WinVar {
public:
	WinVar();
	virtual ~WinVar();

	void SetGuiInfo(Dict *dict, const char *name);
	const char *GetName() const {
		if (_name) {
			if (_dict && *_name == '*')
				return _dict->GetString(&_name[1]);
			return _name;
		}
		return "";
	}
	void SetName(const char *name) {
		delete[] _name;
		_name = nullptr;
		if (name) {
			_name = new char[strlen(name) + 1];
			strcpy(_name, name);
		}
	}

	WinVar &operator=(const WinVar &other) {
		_dict = other._dict;
		SetName(other._name);
		return *this;
	}

	Dict *GetDict() const { return _dict; }
	bool NeedsUpdate() { return (_dict != nullptr); }

	virtual void Init(const char *name, Window *win) = 0;
	virtual void Set(const char *val) = 0;
	virtual void Update() = 0;
	virtual const char *c_str() const = 0;
	virtual size_t Size() { size_t sz = (_name ? strlen(_name) : 0); return sz + sizeof(*this); }

	virtual float x() const = 0;

	void SetEval(bool b) {
		_eval = b;
	}
	bool GetEval() {
		return _eval;
	}

protected:
	Dict *_dict;
	char *_name;
	bool _eval;
};

class WinBool : public WinVar {
public:
	WinBool() : WinVar() {};
	~WinBool() {};
	virtual void Init(const char *_name, Window *win) {
		WinVar::Init(_name, win);
		if (_dict)
			_data = _dict->GetBool(GetName());
	}
	int	operator==(const bool &other) { return (other == _data); }
	bool &operator=(const bool &other) {
		_data = other;
		if (_dict)
			_dict->SetBool(GetName(), _data);
		return _data;
	}
	WinBool &operator=(const WinBool &other) {
		WinVar::operator=(other);
		_data = other._data;
		return *this;
	}

	operator bool() const { return _data; }

	virtual void Set(const char *val) {
		_data = (atoi(val) != 0);
		if (_dict)
			_dict->SetBool(GetName(), _data);
	}

	virtual void Update() {
		const char *s = GetName();
		if (_dict && s[0] != '\0')
			_data = _dict->GetBool(s);
	}

	virtual const char *c_str() const { return va("%i", _data); }

	virtual float x() const { return _data ? 1.0f : 0.0f; };

protected:
	bool _data;
};

class WinStr : public WinVar {
public:
	WinStr() : WinVar() {};
	~WinStr() {};
	virtual void Init(const char *name, Window *win) {
		WinVar::Init(name, win);
		if (_dict) {
			const char *name2 = GetName();
			_data = (!name2[0] ? "" : _dict->GetString(name2));
		}
	}
	int	operator==(const string &other) const {
		return (other == _data);
	}
	int	operator==(const char *other) const {
		return (_data == other);
	}
	string &operator=(const string &other) {
		_data = other;
		if (_dict)
			_dict->Set(GetName(), _data.c_str());
		return _data;
	}
	WinStr &operator=(const WinStr &other) {
		WinVar::operator=(other);
		_data = other._data;
		return *this;
	}
	operator const char *() const {
		return _data.c_str();
	}
	operator const string &() const {
		return _data;
	}
	//int LengthWithoutColors() {
	//	if (_dict && _name && *_name)
	//		_data = _dict->GetString(GetName());
	//	return _data.LengthWithoutColors();
	//}
	int Length() {
		if (_dict && _name && *_name)
			_data = _dict->GetString(GetName());
		return _data.size();
	}
	//void RemoveColors() {
	//	if (_dict && _name && *_name)
	//		_data = _dict->GetString(GetName());
	//	_data.RemoveColors();
	//}
	virtual const char *c_str() const {
		return _data.c_str();
	}

	virtual void Set(const char *val) {
		_data = val;
		if (_dict)
			_dict->Set(GetName(), _data.c_str());
	}

	virtual void Update() {
		const char *s = GetName();
		if (_dict && s[0] != '\0')
			_data = _dict->GetString(s);
	}

	virtual size_t Size() {
		size_t sz = WinVar::Size();
		return sz + _data.capacity();
	}

	virtual float x() const { return _data[0] ? 1.0f : 0.0f; };

protected:
	string _data;
};

class WinInt : public WinVar {
public:
	WinInt() : WinVar() {};
	~WinInt() {};
	virtual void Init(const char *name, Window *win) {
		WinVar::Init(name, win);
		if (_dict)
			_data = _dict->GetInt(GetName());
	}
	int &operator=(const int &other) {
		_data = other;
		if (_dict)
			_dict->SetInt(GetName(), _data);
		return _data;
	}
	WinInt &operator=(const WinInt &other) {
		WinVar::operator=(other);
		_data = other._data;
		return *this;
	}
	operator int() const {
		return _data;
	}
	virtual void Set(const char *val) {
		_data = atoi(val);
		if (_dict)
			_dict->SetInt(GetName(), _data);
	}

	virtual void Update() {
		const char *s = GetName();
		if (_dict && s[0] != '\0')
			_data = _dict->GetInt(s);
	}
	virtual const char *c_str() const {
		return va("%i", _data);
	}

	// no suitable conversion
	virtual float x() const { assert(false); return 0.0f; };

protected:
	int _data;
};

class WinFloat : public WinVar {
public:
	WinFloat() : WinVar() {};
	~WinFloat() {};
	virtual void Init(const char *name, Window *win) {
		WinVar::Init(name, win);
		if (_dict)
			_data = _dict->GetFloat(GetName());
	}
	WinFloat &operator=(const WinFloat &other) {
		WinVar::operator=(other);
		_data = other._data;
		return *this;
	}
	float &operator=(const float &other) {
		_data = other;
		if (_dict)
			_dict->SetFloat(GetName(), _data);
		return _data;
	}
	operator float() const {
		return _data;
	}
	virtual void Set(const char *val) {
		_data = (float)atof(val);
		if (_dict)
			_dict->SetFloat(GetName(), _data);
	}
	virtual void Update() {
		const char *s = GetName();
		if (_dict && s[0] != '\0')
			_data = _dict->GetFloat(s);
	}
	virtual const char *c_str() const {
		return va("%f", _data);
	}

	virtual float x() const { return _data; };
protected:
	float _data;
};

class WinRectangle : public WinVar {
public:
	WinRectangle() : WinVar() {};
	~WinRectangle() {};
	virtual void Init(const char *name, Window *win) {
		WinVar::Init(_name, win);
		if (_dict) {
			vec4 v = _dict->GetVec4(GetName());
			_data.x = v.x;
			_data.y = v.y;
			_data.w = v.z;
			_data.h = v.w;
		}
	}

	int	operator==(const Rectangle &other) const {
		return (other == _data);
	}

	WinRectangle &operator=(const WinRectangle &other) {
		WinVar::operator=(other);
		_data = other._data;
		return *this;
	}
	Rectangle &operator=(const vec4 &other) {
		_data = other;
		if (_dict)
			_dict->SetVec4(GetName(), other);
		return _data;
	}

	Rectangle &operator=(const Rectangle &other) {
		_data = other;
		if (_dict) {
			vec4 v = _data.ToVec4();
			_dict->SetVec4(GetName(), v);
		}
		return _data;
	}

	operator const Rectangle&() const {
		return _data;
	}

	float x() const {
		return _data.x;
	}
	float y() const {
		return _data.y;
	}
	float w() const {
		return _data.w;
	}
	float h() const {
		return _data.h;
	}
	float Right() const {
		return _data.Right();
	}
	float Bottom() const {
		return _data.Bottom();
	}
	vec4 &ToVec4() {
		static vec4 ret;
		ret = _data.ToVec4();
		return ret;
	}
	virtual void Set(const char *val) {
		if (strchr(val, ','))
			sscanf(val, "%f,%f,%f,%f", &_data.x, &_data.y, &_data.w, &_data.h);
		else
			sscanf(val, "%f %f %f %f", &_data.x, &_data.y, &_data.w, &_data.h);
		if (_dict) {
			vec4 v = _data.ToVec4();
			_dict->SetVec4(GetName(), v);
		}
	}
	virtual void Update() {
		const char *s = GetName();
		if (_dict && s[0] != '\0') {
			vec4 v = _dict->GetVec4(s);
			_data.x = v.x;
			_data.y = v.y;
			_data.w = v.z;
			_data.h = v.w;
		}
	}

	virtual const char *c_str() const {
		vec4 ret = _data.ToVec4();
		return va("%f %f %f %f", ret.x, ret.y, ret.z, ret.w);
	}

protected:
	Rectangle _data;
};

class WinVec2 : public WinVar {
public:
	WinVec2() : WinVar() {};
	~WinVec2() {};
	virtual void Init(const char *name, Window *win) {
		WinVar::Init(name, win);
		if (_dict)
			_data = _dict->GetVec2(GetName());
	}
	int	operator==(const vec2 &other) const {
		return (other == _data);
	}
	WinVec2 &operator=(const WinVec2 &other) {
		WinVar::operator=(other);
		_data = other._data;
		return *this;
	}

	vec2 &operator=(const vec2 &other) {
		_data = other;
		if (_dict)
			_dict->SetVec2(GetName(), _data);
		return _data;
	}
	float x() const {
		return _data.x;
	}
	float y() const {
		return _data.y;
	}
	virtual void Set(const char *val) {
		if (strchr(val, ','))
			sscanf(val, "%f,%f", &_data.x, &_data.y);
		else
			sscanf(val, "%f %f", &_data.x, &_data.y);
		if (_dict)
			_dict->SetVec2(GetName(), _data);
	}
	operator const vec2&() const {
		return _data;
	}
	virtual void Update() {
		const char *s = GetName();
		if (_dict && s[0] != '\0')
			_data = _dict->GetVec2(s);
	}
	virtual const char *c_str() const {
		return va("%f %f", _data.x, _data.y);
	}
	void Zero() {
		_data.x = _data.y = 0;
	}

protected:
	vec2 _data;
};

class WinVec4 : public WinVar {
public:
	WinVec4() : WinVar() {};
	~WinVec4() {};
	virtual void Init(const char *name, Window *win) {
		WinVar::Init(name, win);
		if (_dict)
			_data = _dict->GetVec4(GetName());
	}
	int	operator==(const vec4 &other) const {
		return (other == _data);
	}
	WinVec4 &operator=(const WinVec4 &other) {
		WinVar::operator=(other);
		_data = other._data;
		return *this;
	}
	vec4 &operator=(const vec4 &other) {
		_data = other;
		if (_dict)
			_dict->SetVec4(GetName(), _data);
		return _data;
	}
	operator const vec4&() const {
		return _data;
	}

	float x() const {
		return _data.x;
	}

	float y() const {
		return _data.y;
	}

	float z() const {
		return _data.z;
	}

	float w() const {
		return _data.w;
	}
	virtual void Set(const char *val) {
		if (strchr(val, ','))
			sscanf(val, "%f,%f,%f,%f", &_data.x, &_data.y, &_data.z, &_data.w);
		else
			sscanf(val, "%f %f %f %f", &_data.x, &_data.y, &_data.z, &_data.w);
		if (_dict)
			_dict->SetVec4(GetName(), _data);
	}
	virtual void Update() {
		const char *s = GetName();
		if (_dict && s[0] != '\0') {
			_data = _dict->GetVec4(s);
		}
	}
	virtual const char *c_str() const {
		return va("%f %f %f %f", _data.x, _data.y, _data.z, _data.w);
	}

	void Zero() {
		_data.x = _data.y = _data.z = _data.w = 0;
		if (_dict)
			_dict->SetVec4(GetName(), _data);
	}

	const vec3 &ToVec3() const {
		return vec3(_data.x, _data.y, _data.z);
	}

protected:
	vec4 _data;
};

class WinVec3 : public WinVar {
public:
	WinVec3() : WinVar() {};
	~WinVec3() {};
	virtual void Init(const char *name, Window *win) {
		WinVar::Init(name, win);
		if (_dict)
			_data = _dict->GetVector(GetName());
	}
	int	operator==(const vec3 &other) const {
		return (other == _data);
	}
	WinVec3 &operator=(const WinVec3 &other) {
		WinVar::operator=(other);
		_data = other._data;
		return *this;
	}
	vec3 &operator=(const vec3 &other) {
		_data = other;
		if (_dict)
			_dict->SetVector(GetName(), _data);
		return _data;
	}
	operator const vec3&() const {
		return _data;
	}

	float x() const {
		return _data.x;
	}

	float y() const {
		return _data.y;
	}

	float z() const {
		return _data.z;
	}

	virtual void Set(const char *val) {
		sscanf(val, "%f %f %f", &_data.x, &_data.y, &_data.z);
		if (_dict)
			_dict->SetVector(GetName(), _data);
	}
	virtual void Update() {
		const char *s = GetName();
		if (_dict && s[0] != '\0')
			_data = _dict->GetVector(s);
	}
	virtual const char *c_str() const {
		return va("%f %f %f", _data.x, _data.y, _data.z);
	}

	void Zero() {
		_data.x = _data.y = _data.z = 0;
		if (_dict)
			_dict->SetVector(GetName(), _data);
	}

protected:
	vec3 _data;
};

class WinBackground : public WinStr {
public:
	WinBackground() : WinStr() {
		_mat = NULL;
	};
	~WinBackground() {};
	virtual void Init(const char *name, Window *win) {
		WinStr::Init(name, win);
		if (_dict)
			_data = _dict->GetString(GetName());
	}
	int	operator==(const string &other) const {
		return (other == _data);
	}
	int	operator==(const char *other) const {
		return (_data == other);
	}
	string &operator=(const string &other) {
		_data = other;
		if (_dict)
			_dict->Set(GetName(), _data.c_str());
		if (_mat)
			(*_mat) = (_data == "" ? nullptr : g_declManager->FindMaterial(_data.c_str()));
		return _data;
	}
	WinBackground &operator=(const WinBackground &other) {
		WinVar::operator=(other);
		_data = other._data;
		_mat = other._mat;
		if (_mat)
			(*_mat) = (_data == "" ? nullptr : g_declManager->FindMaterial(_data.c_str()));
		return *this;
	}
	operator const char *() const {
		return _data.c_str();
	}
	operator const string &() const {
		return _data;
	}
	int Length() {
		if (_dict)
			_data = _dict->GetString(GetName());
		return _data.size();
	}
	virtual const char *c_str() const {
		return _data.c_str();
	}
	virtual void Set(const char *val) {
		_data = val;
		if (_dict)
			_dict->Set(GetName(), _data.c_str());
		if (_mat)
			(*_mat) = (_data == "" ? nullptr : g_declManager->FindMaterial(_data.c_str()));
	}
	virtual void Update() {
		const char *s = GetName();
		if (_dict && s[0] != '\0') {
			_data = _dict->GetString(s);
			if (_mat)
				(*_mat) = (_data == "" ? nullptr : g_declManager->FindMaterial(_data.c_str()));
		}
	}
	virtual size_t Size() {
		size_t sz = WinVar::Size();
		return sz + _data.capacity();
	}
	void SetMaterialPtr(const Material **mat) {
		_mat = mat;
	}

protected:
	string _data;
	const Material **_mat;
};

class MultiWinVar : public vector<WinVar *> {
public:
	void Set(const char *val);
	void Update();
	void SetGuiInfo(Dict *dict);
};

#endif /* !__WINVAR_H__ */

