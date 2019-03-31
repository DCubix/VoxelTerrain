#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <vector>
#include <algorithm>
#include "integer.h"
#include "stb_image.h"

struct Color {
	f32 r, g, b, a;

	inline Color(f32 r, f32 g, f32 b, f32 a=1.0f) : r(r), g(g), b(b), a(a) {}
	inline Color() : r(0), g(0), b(0), a(0) {}

	inline Color lerp(const Color& to, f32 fac) const {
		return (*this) * (1.0f - fac) + to * fac;
	}

	inline Color operator +(const Color& o) const { return Color(r + o.r, g + o.g, b + o.b, a); }
	inline Color operator -(const Color& o) const { return Color(r - o.r, g - o.g, b - o.b, a); }
	inline Color operator *(const Color& o) const { return Color(r * o.r, g * o.g, b * o.b, a * o.a); }
	inline Color operator *(f32 o) const { return Color(r * o, g * o, b * o, a * o); }
	inline Color operator /(f32 o) const { return Color(r / o, g / o, b / o, a / o); }

	inline void clamp() {
		r = std::clamp(r, 0.0f, 1.0f);
		g = std::clamp(g, 0.0f, 1.0f);
		b = std::clamp(b, 0.0f, 1.0f);
		a = std::clamp(a, 0.0f, 1.0f);
	}
};

class Texture {
public:
	Texture() = default;
	~Texture() = default;

	inline Texture(const std::string& fileName) {
		i32 w, h, comp;
		u8* data = stbi_load(fileName.c_str(), &w, &h, &comp, 4);
		if (data) {
			m_width = w;
			m_height = h;
			m_pixels = std::vector<u8>(data, data + (w * h * 4));
			stbi_image_free(data);
		}
	}

	inline Texture(u32 width, u32 height) {
		m_pixels.resize(width * height * 4);
		std::fill(m_pixels.begin(), m_pixels.end(), 0u);
		m_width = width;
		m_height = height;
	}

	inline Color sample(f32 u, f32 v) {
		u = u * m_width;
		v = v * m_height;

		u32 x = ::floor(u);
		u32 y = ::floor(v);

		f32 ur = u - x;
		f32 vr = v - y;
		f32 uo = 1.0f - ur;
		f32 vo = 1.0f - vr;

		Color res =
			(get(x, y) * uo + get(x + 1, y) * ur) * vo +
			(get(x, y + 1) * uo + get(x + 1, y + 1) * ur) * vr;
		return res;
	}

	inline void set(i32 x, i32 y, Color color) {
		if (m_width == 0 || m_height == 0) return;
		color.clamp();

		x = x % m_width;
		y = y % m_height;
		u32 uvi = (x + y * m_width) * 4;
		m_pixels[uvi + 0] = u8(color.r * 255.0f);
		m_pixels[uvi + 1] = u8(color.g * 255.0f);
		m_pixels[uvi + 2] = u8(color.b * 255.0f);
		m_pixels[uvi + 3] = u8(color.a * 255.0f);
	}

	inline Color get(i32 x, i32 y) {
		if (m_width == 0 || m_height == 0) return Color(0.0f, 0.0f, 0.0f);

		x = x % m_width;
		y = y % m_height;
		u32 uvi = (x + y * m_width) * 4;
		f32 r = f32(m_pixels[uvi + 0]) / 255.0f;
		f32 g = f32(m_pixels[uvi + 1]) / 255.0f;
		f32 b = f32(m_pixels[uvi + 2]) / 255.0f;
		f32 a = f32(m_pixels[uvi + 3]) / 255.0f;
		return Color(r, g, b, a);
	}

	u32 width() const { return m_width; }
	u32 height() const { return m_height; }

private:
	u32 m_width{ 0 }, m_height{ 0 };
	std::vector<u8> m_pixels;
};

#endif // TEXTURE_H