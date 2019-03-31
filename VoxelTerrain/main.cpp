#include <iostream>

#include <algorithm>
#include <array>

#include "game_canvas.h"
#include "texture.hpp"

#define xlerp(a, b, t) ((1-t)*a+b*t)
#define rad(x) (x * 0.0174533f)
#define E 2.71828f

struct Vec3 {
	f32 x, y, z;

	Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}
	Vec3() : x(0.0f), y(0.0f), z(0.0f) {}

	f32 dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }

	Vec3 cross(const Vec3& o) const {
		return Vec3(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x);
	}

	f32 length() const { return std::sqrtf(dot(*this)); }
	Vec3 normalized() const { return (*this) / length(); }
	f32 angleZ() const { return std::atan2f(y, x); }

	Vec3 rotateX(f32 angle) const {
		const float s = std::sinf(angle), c = std::cosf(angle);
		f32 ry = y * c - z * s;
		f32 rz = y * s + z * c;
		return Vec3(x, ry, rz);
	}

	Vec3 rotateY(f32 angle) const {
		const float s = std::sinf(angle), c = std::cosf(angle);
		f32 rx = y * c + z * s;
		f32 rz = y * -s + z * c;
		return Vec3(rx, y, rz);
	}

	Vec3 rotateZ(f32 angle) const {
		const float s = std::sinf(angle), c = std::cosf(angle);
		f32 rx = x * c - y * s;
		f32 ry = x * s + y * c;
		return Vec3(rx, ry, z);
	}

	Vec3 lerp(const Vec3& to, f32 fac) const {
		return (*this) * (1.0f - fac) + to * fac;
	}

	Vec3 operator +(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
	Vec3 operator -(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
	Vec3 operator *(const Vec3& o) const { return Vec3(x * o.x, y * o.y, z * o.z); }
	Vec3 operator *(f32 o) const { return Vec3(x * o, y * o, z * o); }
	Vec3 operator /(f32 o) const { return Vec3(x / o, y / o, z / o); }
};

struct Mat3 {
	union {
		std::array<f32, 9> m;
		Vec3 rows[3];
	};

	Mat3() {
		m[0] = 1.0f; m[1] = 0.0f; m[2] = 0.0f;
		m[3] = 0.0f; m[4] = 1.0f; m[5] = 0.0f;
		m[6] = 0.0f; m[7] = 0.0f; m[8] = 1.0f;
	}

	Mat3(const std::array<f32, 9> m) : m(m) {}

	Mat3 transposed() const {
		return Mat3({
			m[0], m[3], m[6],
			m[1], m[4], m[7],
			m[2], m[5], m[8]
		});
	}

	static Mat3 rotationX(f32 angle) {
		const f32 s = ::sinf(angle), c = ::cosf(angle);
		return Mat3({
			1.0f, 0.0f, 0.0f,
			0.0f,    c,   -s,
			0.0f,    s,    c
					});
	}

	static Mat3 rotationY(f32 angle) {
		const f32 s = ::sinf(angle), c = ::cosf(angle);
		return Mat3({
			   c, 0.0f,    s,
			0.0f, 1.0f, 0.0f,
			  -s, 0.0f,    c
					});
	}

	static Mat3 rotationZ(f32 angle) {
		const f32 s = ::sinf(angle), c = ::cosf(angle);
		return Mat3({
			   c,   -s, 0.0f,
			   s,    c, 0.0f,
			0.0f, 0.0f, 1.0f
					});
	}

	const f32& operator [](u32 i) const { return m[i]; }
	f32& operator [](u32 i) { return m[i]; }

	Mat3 operator *(const Mat3& v) const {
		Mat3 ret{};
		for (u32 y = 0; y < 3; y++) {
			for (u32 x = 0; x < 3; x++) {
				u32 i = x + y * 3;
				ret[i] = 0.0f;
				for (u32 k = 0; k < 3; k++) {
					ret[i] += m[k + y * 3] * v[x + k * 3];
				}
			}
		}
		return ret;
	}

	Vec3 operator *(const Vec3& v) const {
		Vec3 ret{};
		ret.x = m[0] * v.x + m[1] * v.y + m[2] * v.z;
		ret.y = m[3] * v.x + m[4] * v.y + m[5] * v.z;
		ret.z = m[6] * v.x + m[7] * v.y + m[8] * v.z;
		return ret;
	}
};

class VoxelTerrain : public GameAdapter {
public:
	virtual void onSetup(GameCanvas *canvas) {
		heightmap = Texture("terrain.png");
		color = Texture("color.png");

		// Generate normals
		/*const Vec3 size{ 0.25f, 0.0f, 0.0f };
		normalmap = Texture(heightmap.width(), heightmap.height());
		for (u32 y = 0; y < heightmap.height(); y++) {
			for (u32 x = 0; x < heightmap.width(); x++) {
				float s11 = heightmap.get(x, y).r;
				float s01 = heightmap.get(x - 1, y).r;
				float s21 = heightmap.get(x + 1, y).r;
				float s10 = heightmap.get(x, y - 1).r;
				float s12 = heightmap.get(x, y + 1).r;
				Vec3 va = Vec3(size.x, size.y, s21 - s01).normalized(),
					 vb = Vec3(size.y, size.x, s12 - s10).normalized();
				Vec3 n = va.cross(vb) * 0.5f + Vec3(0.5f, 0.5f, 0.5f);
				Color bump{ n.x, n.y, n.z, s11 };
				normalmap.set(x, y, Color(n.x, n.y, n.z));
			}
		}*/
	}

	virtual void onUpdate(GameCanvas *canvas, f32 dt) {
		f32 rx = ((camRot.x / 2.0f) * 2.0f - 1.0f) * M_PI * 0.25f;
		Mat3 rot = Mat3::rotationZ(-camRot.y) * Mat3::rotationX(rx);
		Vec3 dir = rot * Vec3(0.0f, 1.0f, 0.0f);

		if (canvas->isHeld(SDLK_LEFT)) {
			turning.y = xlerp(turning.y, 1.0f, 0.1f);
			turning.z = xlerp(turning.z, 70.0f, dt);
		} else if (canvas->isHeld(SDLK_RIGHT)) {
			turning.y = xlerp(turning.y, -1.0f, 0.1f);
			turning.z = xlerp(turning.z, -70.0f, dt);
		}

		if (canvas->isHeld(SDLK_DOWN)) {
			turning.x = xlerp(turning.y, 2.0f, 0.2f);
		} else if (canvas->isHeld(SDLK_UP)) {
			turning.x = xlerp(turning.y, -2.0f, 0.2f);
		}

		camPos = camPos + dir * dt * moveSpeed;
		camRot.x += turning.x * dt;
		camRot.x = std::clamp(camRot.x, 0.0f, 2.0f);
		camRot.y += turning.y * dt;
		camRot.z = turning.z;

		turning = turning * 0.9f;
	}

	virtual void onDraw(GameCanvas *canvas) {
		canvas->clear(bg.r, bg.g, bg.b);

		const f32 w2 = canvas->width() / 2;
		const f32 h2 = canvas->height() / 2;

		const f32 thf = ::tanf(camFOV / 2.0f);
		const f32 planeDist = w2 / thf;
		Vec3 plane(
			0.0f,
			thf,
			0.0f
		);
		plane = plane.rotateZ(-camRot.y);
		Vec3 perp(-plane.y, plane.x, 0.0f);

		std::vector<f32> ybuffer;
		ybuffer.resize(canvas->width());
		std::fill(ybuffer.begin(), ybuffer.end(), f32(canvas->height()));

		f32 s = ::sinf(camRot.y), c = ::cosf(camRot.y);
		Vec3 dir(s, c, 0.0f);

		f32 horizon = camRot.x * h2;

		// Draw from front to back
		f32 dz = 1.0f;
		for (f32 z = 1.0f; z < distance; z += dz) {
			f32 sz = z / thf;
			f32 lz = (sz - 1.0f) / distance * dz;
			f32 invz = 1.0f / z * scaleHeight;

			Vec3 pleft = camPos + dir * sz - perp * sz;
			Vec3 pright = camPos + dir * sz + perp * sz;

			for (u32 x = 0; x < canvas->width(); x++) {
				const f32 xf = (f32(x) / canvas->width());
				Vec3 pos = pleft.lerp(pright, xf);
				/*f32 u = pos.x / f32(heightmap.width());
				f32 v = pos.y / f32(heightmap.height());*/

				Color c = heightmap.get(pos.x, pos.y);
				f32 val = c.r;
				f32 h = (camPos.z - val * 255) * invz + horizon;
				h += camRot.z * (xf*2.0f-1.0f);
				if (h < 0.0f || h > canvas->height()) continue;

				f32 fogD = 1.0f;
				f32 fog = 1.0f / ::exp((lz * fogD) * (lz * fogD));
				fog = std::clamp(fog, 0.0f, 1.0f);

				/*Color nc = normalmap.get(pos.x, pos.y);
				Vec3 n = Vec3(nc.r, nc.g, nc.b) * 2.0f - Vec3(1.0f, 1.0f, 1.0f);
				f32 nl = std::clamp(n.dot(L), 0.0f, 1.0f);
				
				Color col = bg.lerp(Color(nl, nl, nl), fog);*/
				Color col = bg.lerp(color.get(pos.x, pos.y), fog);

				if (h < ybuffer[x]) {
					canvas->line(x, h, x, ybuffer[x], col.r, col.g, col.b);
					ybuffer[x] = h;
				}
			}
			dz += 0.001f;
		}

		canvas->str("X: " + std::to_string(camPos.x), 10, 10);
		canvas->str("Y: " + std::to_string(camPos.y), 10, 20);
	}

	Texture heightmap, color, normalmap;

	Color bg{ 0.45f, 0.1f, 0.05f };

	Vec3 L{ -1.0f, -1.0f, 1.0f };

	Vec3 camPos{ 160, 120, 120 };
	Vec3 camRot{ 1.0f, 0.0f, 0.0f };
	f32 camFOV{ rad(90.0f) };

	Vec3 turning{ 0.0f, 0.0f, 0.0f };
	f32 turnSpeed{ 20.0f };
	f32 moveSpeed{ 60.0f };

	f32 distance{ 800.0f }, scaleHeight{ 120.0f };
};

int main(int argc, char** argv) {
	std::cout << argv[0] << std::endl;
	GameCanvas gc{ new VoxelTerrain(), 800, 600 };
	return gc.run();
}