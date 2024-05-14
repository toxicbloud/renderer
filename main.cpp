#include "tgaimage.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <chrono>

#define GLM_FORCE_SSE2 // pour SSE2
#define GLM_FORCE_SSE3 // pour SSE3
#define GLM_FORCE_SSSE3 // pour SSSE3
#define GLM_FORCE_SSE41 // pour SSE4.1
#define GLM_FORCE_SSE42 // pour SSE4.2
#define GLM_FORCE_AVX // pour AVX
#define GLM_FORCE_AVX2 // pour AVX2
#define GLM_FORCE_AVX512 // pour AVX-512
#define GLM_FORCE_ALIGNED // pour les types alignés

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>

#define WIDTH 2000
#define HEIGHT 2000
#define DEBUG_BUFFER 0
#define BACKFACE_CULLING 1
#define GENERATE_ANIMATION 0
#define NORMAL_MAPPING 1
#define GOURAUD_SHADING 1

#include "Model.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
#if DEBUG_BUFFER
TGAImage normalImage(WIDTH, HEIGHT, TGAImage::RGB);
#endif
float *zbuffer;

struct Shader
{
	glm::mat4 mvp;
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

struct Light
{
	glm::vec3 dir;
	glm::vec3 color;
	float intensity;
};

void draw_line(TGAImage &image, int x0, int y0, int x1, int y1)
{
	for (float t = 0.; t < 1.; t += .01)
	{
		int x = x0 + (x1 - x0) * t;
		int y = y0 + (y1 - y0) * t;
		image.set(x, y, red);
	}
}
void draw_triangle(TGAImage &image, int x0, int y0, int x1, int y1, int x2, int y2)
{
	draw_line(image, x0, y0, x1, y1);
	draw_line(image, x1, y1, x2, y2);
	draw_line(image, x2, y2, x0, y0);
}
glm::vec3 barycentric(const glm::vec3 *pts, const glm::vec3 &P)
{
	glm::mat3 m(pts[0].x, pts[0].y, 1,
				pts[1].x, pts[1].y, 1,
				pts[2].x, pts[2].y, 1);
	m = glm::inverse(m);
	glm::vec3 bary = m * P;
	return bary;
}
void fill_triangle(TGAImage &image, glm::vec3 *pts, glm::vec3 *world_pts, glm::vec3 *vns, Model &model, glm::vec3 &vt0, glm::vec3 &vt1, glm::vec3 &vt2, glm::mat4 &projection)
{
	TGAImage& diffuse = model.diffuse;
	TGAImage& normal = model.normal;
	TGAImage& spec = model.specular;
	for (size_t i = 0; i < 3; i++)
	{
		pts[i].x = (pts[i].x + 1) * WIDTH / 2;
		pts[i].y = (pts[i].y + 1) * HEIGHT / 2;
	}
	// boites englobantes
	int minx = glm::min(pts[0].x, glm::min(pts[1].x, pts[2].x));
	int miny = glm::min(pts[0].y, glm::min(pts[1].y, pts[2].y));

	int maxx = glm::max(pts[0].x, glm::max(pts[1].x, pts[2].x));
	int maxy = glm::max(pts[0].y, glm::max(pts[1].y, pts[2].y));

	glm::mat3 m(pts[0].x, pts[0].y,1, pts[1].x, pts[1].y, 1, pts[2].x, pts[2].y, 1);
	m = glm::inverse(m);

	#pragma omp for schedule(dynamic)
	// on parcours les pixels qui sont dans la boite englobante
	for (int x = minx; x <= maxx; x++)
	{
		for (int y = miny; y <= maxy; y++)
		{
			glm::vec3 p(x, y, 1);
			// glm::vec3 bary = barycentric(pts, p);
			glm::vec3 bary = m * p;

			// si les coordonnées barycentriques sont positives
			if (bary.x >= -1e-2 && bary.y >= -1e-2 && bary.z >= -1e-2)
			{
				glm::vec2 uv = bary.x * vt0 + bary.y * vt1 + bary.z * vt2;
				glm::vec2 uvPixel = glm::vec2(uv.x * float(diffuse.get_width()), uv.y * float(diffuse.get_height()));
				TGAColor texColor = diffuse.get(uvPixel.x, uvPixel.y);
				TGAColor normalColor = normal.get(uvPixel.x, uvPixel.y);
				// update Z
				float z_clip = world_pts[0].z * bary.x + world_pts[1].z * bary.y + world_pts[2].z * bary.z;

				float z_ndc = z_clip * 2.0f - 1.0f; // Convertir à l'intervalle [-1, 1]
				float near = 0.1f;
				float far = 100.0f;
				float z_linear = 2.0f * near * far / (far + near - z_ndc * (far - near));

				p.z = z_linear;

				if ((p.x + p.y * WIDTH) >= WIDTH * HEIGHT || (p.x + p.y * WIDTH) < 0)
				{
					continue;
				}
				if (zbuffer[int(p.x + p.y * WIDTH)] < p.z)
				{
					#if GOURAUD_SHADING
					// Smooth normal  Gouraud shading
					glm::vec3 vn = glm::normalize(bary.x * vns[0] + bary.y * vns[1] + bary.z * vns[2]);
					#else
					// Flat normal
					glm::vec3 vn = glm::normalize(glm::cross(world_pts[1] - world_pts[0], world_pts[2] - world_pts[0]));
					#endif
					glm::vec3 normalFromTex = glm::normalize(glm::vec3(normalColor.r, normalColor.g, normalColor.b) * 2.0f - glm::vec3(1, 1, 1));
					// normal mapping
					#if NORMAL_MAPPING
						vn = glm::normalize(vn + normalFromTex);
					#endif
#if DEBUG_BUFFER
					normalImage.set(p.x, p.y, TGAColor(vn.x * 255, vn.y * 255, vn.z * 255, 255));
#endif
					glm::vec3 lightDir = glm::normalize(glm::vec3(1, 0, 0.f) - glm::vec3(0, 0.5f, 0));
					glm::vec3 lightDir2 = glm::normalize(glm::vec3(0, 1, 0.f) - glm::vec3(0, 0.5f, 0));
					float intensity = glm::clamp(glm::dot(vn, glm::normalize(lightDir)), 0.0f, 1.0f);
					float intensity2 = glm::clamp(glm::dot(vn, glm::normalize(lightDir2)), 0.0f, 1.0f);
					intensity = glm::clamp(intensity + intensity2, 0.0f, 1.0f);
					zbuffer[int(p.x + p.y * WIDTH)] = p.z; // ZBUFFER
					TGAColor specColor = spec.get(uvPixel.x, uvPixel.y);
					glm::vec3 reflect = glm::reflect(-lightDir, vn);
					// float specIntensity = glm::pow(glm::clamp(glm::dot(reflect, glm::normalize(glm::vec3(0, 0, 1))), 0.0f, 1.0f), 5.0f);
			
					float specIntensity = std::pow(glm::max(-reflect.z, 0.f), 5.f + specColor.r);
					TGAColor finalColor = TGAColor((specIntensity + intensity) * texColor.r,
												   (specIntensity + intensity) * texColor.g,
												   (specIntensity + intensity) * texColor.b, 255);
					image.set(p.x, p.y, finalColor);
				}
			}
		}
	}
}

int main(int argc, char **argv)
{
	Model model3d;
	TGAImage image(WIDTH, HEIGHT, TGAImage::RGB);
#if DEBUG_BUFFER
	TGAImage zbufferImage(WIDTH, HEIGHT, TGAImage::GRAYSCALE);
#endif
	std::string model_name = "african_head";
	model3d.parse(model_name);

	// initialize to lowest possible value
	zbuffer = new float[WIDTH * HEIGHT];
	#pragma omp parallel for
	for (int i = 0; i < WIDTH * HEIGHT; i++)
	{
		zbuffer[i] = -std::numeric_limits<float>::max();
	}
	double start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	#pragma omp parallel
	for (auto &face : model3d.faces)
	{


		glm::vec3& v0 = model3d.vertices[face.v0 - 1];
		glm::vec3& v1 = model3d.vertices.at(face.v1 - 1);
		glm::vec3& v2 = model3d.vertices.at(face.v2 - 1);

		glm::vec3& vn0 = model3d.normals[face.vn0 - 1];
		glm::vec3& vn1 = model3d.normals[face.vn1 - 1];
		glm::vec3& vn2 = model3d.normals[face.vn2 - 1];

		glm::vec3& vt0 = model3d.textures[face.vt0 - 1];
		glm::vec3& vt1 = model3d.textures[face.vt1 - 1];
		glm::vec3& vt2 = model3d.textures[face.vt2 - 1];

		glm::vec3 vns[3] = {vn0, vn1, vn2};

		glm::vec3 cameraPos = glm::vec3(0, 0, 2);
		glm::mat4 view = glm::lookAt(cameraPos,
									 glm::vec3(0, 0, 0),
									 glm::vec3(0, 1, 0));

		glm::mat4 projection = glm::perspective(glm::radians(90.0f),
												float(WIDTH) / float(HEIGHT), 0.1f, 100.0f);
		glm::mat4 model = glm::mat4(1.0f);
		// rotate y axis
		// model = glm::rotate(model, glm::radians(270.0f), glm::vec3(0.5f, 0.5f, 0.f));
		glm::mat4 mvp = projection * view * model;
		glm::mat4x4 vertices = glm::mat4x4(
			glm::vec4(v0, 1.0f),
			glm::vec4(v1, 1.0f),
			glm::vec4(v2, 1.0f),
			glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

		glm::mat4x4 pts4 = mvp * vertices;

		glm::vec3 pts[3] = {
			glm::vec3(pts4[0] / pts4[0].w),
			glm::vec3(pts4[1] / pts4[1].w),
			glm::vec3(pts4[2] / pts4[2].w)};
		glm::mat4x4 world_pts4 = model * vertices;

		glm::vec3 world_pts[3] = {
			glm::vec3(world_pts4[0]),
			glm::vec3(world_pts4[1]),
			glm::vec3(world_pts4[2])};
		// backface culling real normal in world
		glm::vec3 u = pts[1] - pts[0];
		glm::vec3 v = pts[2] - pts[0];
		glm::vec3 normal = glm::normalize(glm::cross(u, v));
#if BACKFACE_CULLING
		if (normal.z < 0)
		{
			continue;
		}
#endif
		fill_triangle(image, pts, world_pts, vns, model3d, vt0, vt1, vt2, projection);
	}
	double end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	std::cout << "Time elapsed: " << end - start << " ms" << std::endl;

	image.flip_vertically();
	image.write_tga_file("output.tga");
// fill zbuffer image
#if DEBUG_BUFFER
	for (int i = 0; i < WIDTH * HEIGHT; i++)
	{
		zbufferImage.set(i % WIDTH, i / WIDTH, TGAColor(zbuffer[i] * 255, zbuffer[i] * 255, zbuffer[i] * 255, 255));
	}
	zbufferImage.flip_vertically();
	zbufferImage.write_tga_file("zbuffer.tga");
	normalImage.flip_vertically();
	normalImage.write_tga_file("normal.tga");
#endif
	return 0;
}
