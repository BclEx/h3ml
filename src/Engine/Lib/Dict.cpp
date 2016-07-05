#include "..\Global.h"

bool Dict::GetFloat(const char *key, const char *defaultString, float &out) const {
	const char *s;
	bool found = GetString(key, defaultString, &s);
	out = atof(s);
	return found;
}

bool Dict::GetInt(const char *key, const char *defaultString, int &out) const {
	const char *s;
	bool found = GetString(key, defaultString, &s);
	out = atoi(s);
	return found;
}

bool Dict::GetBool(const char *key, const char *defaultString, bool &out) const {
	const char *s;
	bool found = GetString(key, defaultString, &s);
	out = (atoi(s) != 0);
	return found;
}

bool Dict::GetFloat(const char *key, const float defaultFloat, float &out) const {
	const_iterator kv = find(key);
	if (kv != end()) {
		out = atof(kv->second);
		return true;
	}
	out = defaultFloat;
	return false;
}

bool Dict::GetInt(const char *key, const int defaultInt, int &out) const {
	const_iterator kv = find(key);
	if (kv != end()) {
		out = atoi(kv->second);
		return true;
	}
	out = defaultInt;
	return false;
}

bool Dict::GetBool(const char *key, const bool defaultBool, bool &out) const {
	const_iterator kv = find(key);
	if (kv != end()) {
		out = (atoi(kv->second) != 0);
		return true;
	}
	out = defaultBool;
	return false;
}

//bool Dict::GetAngles(const char *key, const char *defaultString, angles &out) const {
//	if (!defaultString)
//		defaultString = "0 0 0";
//	const char *s;
//	bool found = GetString(key, defaultString, &s);
//	out.pitch = out.yaw = out.roll = 0;
//	sscanf(s, "%f %f %f", &out.pitch, &out.yaw, &out.roll);
//	return found;
//}

bool Dict::GetVector(const char *key, const char *defaultString, vec3 &out) const {
	if (!defaultString)
		defaultString = "0 0 0";
	const char *s;
	bool found = GetString(key, defaultString, &s);
	out.x = out.y = out.z;
	sscanf(s, "%f %f %f", &out.x, &out.y, &out.z);
	return found;
}

bool Dict::GetVec2(const char *key, const char *defaultString, vec2 &out) const {
	if (!defaultString)
		defaultString = "0 0";
	const char *s;
	bool found = GetString(key, defaultString, &s);
	out.x = out.y = 0;
	sscanf(s, "%f %f", &out.x, &out.y);
	return found;
}

bool Dict::GetVec4(const char *key, const char *defaultString, vec4 &out) const {
	if (!defaultString)
		defaultString = "0 0 0 0";
	const char *s;
	bool found = GetString(key, defaultString, &s);
	out.x = out.y = out.z = out.w = 0;
	sscanf(s, "%f %f %f %f", &out.x, &out.y, &out.z, &out.w);
	return found;
}

bool Dict::GetMatrix(const char *key, const char *defaultString, mat3 &out) const {
	if (!defaultString)
		defaultString = "1 0 0 0 1 0 0 0 1";
	const char *s;
	bool found = GetString(key, defaultString, &s);
	out[0].x = out[0].y = out[0].z = 0;
	out[1].x = out[1].y = out[1].z = 0;
	out[2].x = out[2].y = out[2].z = 0;
	sscanf(s, "%f %f %f %f %f %f %f %f %f", &out[0].x, &out[0].y, &out[0].z, &out[1].x, &out[1].y, &out[1].z, &out[2].x, &out[2].y, &out[2].z);
	return found;
}
