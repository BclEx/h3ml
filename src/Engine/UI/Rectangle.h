#ifndef __RECTANGLE_H__
#define __RECTANGLE_H__

extern void RotateVector(vec3 &v, vec3 origin, float a, float c, float s);
class Rectangle {
public:
	float x; // horiz position
	float y; // vert position
	float w; // width
	float h; // height;
	Rectangle() { x = y = w = h = 0.0; }
	Rectangle(float ix, float iy, float iw, float ih) { x = ix; y = iy; w = iw; h = ih; }
	float Bottom() const { return y + h; }
	float Right() const { return x + w; }
	void Offset(float x, float y) {
		this->x += x;
		this->y += y;
	}
	bool Contains(float xt, float yt) {
		if (w == 0.0 && h == 0.0)
			return false;
		if (xt >= x && xt <= Right() && yt >= y && yt <= Bottom())
			return true;
		return false;
	}
	void Empty() { x = y = w = h = 0.0; };

	void ClipAgainst(Rectangle r, bool sizeOnly) {
		if (!sizeOnly) {
			if (x < r.x) {
				w -= r.x - x;
				x = r.x;
			}
			if (y < r.y) {
				h -= r.y - y;
				y = r.y;
			}
		}
		if (x + w > r.x + r.w)
			w = (r.x + r.w) - x;
		if (y + h > r.y + r.h)
			h = (r.y + r.h) - y;
	}

	void Rotate(float a, Rectangle &out) {
		vec3 center = vec3((x + w) / 2.0, (y + h) / 2.0, 0);
		vec3 p1 = vec3(x, y, 0);
		vec3 p2 = vec3(Right(), y, 0);
		vec3 p4 = vec3(x, Bottom(), 0);
		float c, s;
		if (a) {
			s = sin(radians(a));
			c = cos(radians(a));
		}
		else
			s = c = 0;
		RotateVector(p1, center, a, c, s);
		RotateVector(p2, center, a, c, s);
		RotateVector(p4, center, a, c, s);
		out.x = p1.x;
		out.y = p1.y;
		out.w = length(p2 - p1);
		out.h = length(p4 - p1);
	}

	Rectangle &operator+=(const Rectangle &a);
	Rectangle &operator-=(const Rectangle &a);
	Rectangle &operator/=(const Rectangle &a);
	Rectangle &operator/=(const float a);
	Rectangle &operator*=(const float a);
	Rectangle &operator=(const vec4 v);
	int operator==(const Rectangle &a) const;
	float &operator[](const int index);
	char *String() const;
	const vec4 &ToVec4() const;
};

__inline const vec4 &Rectangle::ToVec4() const {
	return *reinterpret_cast<const vec4 *>(&x);
}

__inline Rectangle &Rectangle::operator+=(const Rectangle &a) {
	x += a.x;
	y += a.y;
	w += a.w;
	h += a.h;
	return *this;
}

__inline Rectangle &Rectangle::operator/=(const Rectangle &a) {
	x /= a.x;
	y /= a.y;
	w /= a.w;
	h /= a.h;
	return *this;
}

__inline Rectangle &Rectangle::operator/=(const float a) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;
	w *= inva;
	h *= inva;
	return *this;
}

__inline Rectangle &Rectangle::operator-=(const Rectangle &a) {
	x -= a.x;
	y -= a.y;
	w -= a.w;
	h -= a.h;
	return *this;
}

__inline Rectangle &Rectangle::operator*=(const float a) {
	x *= a;
	y *= a;
	w *= a;
	h *= a;
	return *this;
}

__inline Rectangle &Rectangle::operator=(const vec4 v) {
	x = v.x;
	y = v.y;
	w = v.z;
	h = v.w;
	return *this;
}

__inline int Rectangle::operator==(const Rectangle &a) const {
	return (x == a.x && y == a.y && w == a.w && a.h);
}

__inline float &Rectangle::operator[](int index) {
	return (&x)[index];
}

class Region {
public:
	Region() { };

	void Empty() {
		_rects.clear();
	}

	bool Contains(float xt, float yt) {
		int c = _rects.size();
		for (int i = 0; i < c; i++)
			if (_rects[i].Contains(xt, yt))
				return true;
		return false;
	}

	void AddRect(float x, float y, float w, float h) {
		_rects.push_back(Rectangle(x, y, w, h));
	}

	int GetRectCount() {
		return _rects.size();
	}

	Rectangle *GetRect(int index) {
		if (index >= 0 && index < _rects.size())
			return &_rects[index];
		return nullptr;
	}

protected:
	vector<Rectangle> _rects;
};


#endif
