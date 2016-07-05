#pragma once

//typedef vec3 angles;
struct cmp_str { __inline bool operator()(char const *a, char const *b) { return strcmp(a, b) < 0; } };
class Dict : public map<const char *, const char *, cmp_str> {
public:
	bool GetFloat(const char *key, const char *defaultString, float &out) const;
	bool GetInt(const char *key, const char *defaultString, int &out) const;
	bool GetBool(const char *key, const char *defaultString, bool &out) const;
	bool GetFloat(const char *key, const float defaultFloat, float &out) const;
	bool GetInt(const char *key, const int defaultInt, int &out) const;
	bool GetBool(const char *key, const bool defaultBool, bool &out) const;
	bool GetVector(const char *key, const char *defaultString, vec3 &out) const;
	bool GetVec2(const char *key, const char *defaultString, vec2 &out) const;
	bool GetVec4(const char *key, const char *defaultString, vec4 &out) const;
	//bool GetAngles(const char *key, const char *defaultString, angles &out) const;
	bool GetMatrix(const char *key, const char *defaultString, mat3 &out) const;

	__inline void Set(const char *key, const char *value) {
		insert_or_assign(key, value);
	}
	__inline void SetFloat(const char *key, float val) {
		insert_or_assign(key, va("%f", val));
	}

	__inline void SetInt(const char *key, int val) {
		insert_or_assign(key, va("%i", val));
	}

	__inline void SetBool(const char *key, bool val) {
		insert_or_assign(key, va("%i", val));
	}

	__inline void SetVector(const char *key, const vec3 &val) {
		insert_or_assign(key, va("%f %f %f", val.x, val.y, val.z));
	}

	__inline void SetVec4(const char *key, const vec4 &val) {
		insert_or_assign(key, va("%f %f %f %f", val.x, val.y, val.z, val.w));
	}

	__inline void SetVec2(const char *key, const vec2 &val) {
		insert_or_assign(key, va("%f %f", val.x, val.y));
	}

	//__inline void SetAngles(const char *key, const angles &val) {
	//	insert_or_assign(key, va("%f %f %f", val.x, val.y, val.z));
	//}

	__inline void SetMatrix(const char *key, const mat3 &val) {
		insert_or_assign(key, va("%f %f %f %f %f %f %f %f %f", val[0].x, val[0].y, val[0].z, val[1].x, val[1].y, val[1].z, val[2].x, val[2].y, val[2].z));
	}

	__inline bool GetString(const char *key, const char *defaultString, const char **out) const {
		const_iterator kv = find(key);
		if (kv != end()) {
			*out = kv->second;
			return true;
		}
		*out = defaultString;
		return false;
	}

	__inline bool GetString(const char *key, const char *defaultString, string &out) const {
		const_iterator kv = find(key);
		if (kv != end()) {
			out = kv->second;
			return true;
		}
		out = defaultString;
		return false;
	}

	__inline const char *GetString(const char *key, const char *defaultString = "") const {
		const_iterator kv = find(key);
		if (kv != end())
			return kv->second;
		return defaultString;
	}

	__inline float GetFloat(const char *key, const char *defaultString) const {
		return (float)atof(GetString(key, defaultString));
	}

	__inline int GetInt(const char *key, const char *defaultString) const {
		return atoi(GetString(key, defaultString));
	}

	__inline bool GetBool(const char *key, const char *defaultString) const {
		return (atoi(GetString(key, defaultString)) != 0);
	}

	__inline float GetFloat(const char *key, const float defaultFloat = 0.0f) const {
		const_iterator kv = find(key);
		if (kv != end())
			return (float)atof(kv->second);
		return defaultFloat;
	}

	__inline int GetInt(const char *key, int defaultInt = 0) const {
		const_iterator kv = find(key);
		if (kv != end())
			return atoi(kv->second);
		return defaultInt;
	}

	__inline bool GetBool(const char *key, const bool defaultBool = false) const {
		const_iterator kv = find(key);
		if (kv != end())
			return (atoi(kv->second) != 0);
		return defaultBool;
	}

	__inline vec3 GetVector(const char *key, const char *defaultString = nullptr) const {
		vec3 out;
		GetVector(key, defaultString, out);
		return out;
	}

	__inline vec2 GetVec2(const char *key, const char *defaultString = nullptr) const {
		vec2 out;
		GetVec2(key, defaultString, out);
		return out;
	}

	__inline vec4 GetVec4(const char *key, const char *defaultString = nullptr) const {
		vec4 out;
		GetVec4(key, defaultString, out);
		return out;
	}

	//__inline angles GetAngles(const char *key, const char *defaultString = nullptr) const {
	//	angles out;
	//	GetAngles(key, defaultString, out);
	//	return out;
	//}

	__inline mat3 GetMatrix(const char *key, const char *defaultString = nullptr) const {
		mat3 out;
		GetMatrix(key, defaultString, out);
		return out;
	}
};
