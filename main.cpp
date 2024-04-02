#include "tgaimage.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdio>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>

#define WIDTH 4000
#define HEIGHT 4000

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
float *zbuffer;

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

void fill_triangle(TGAImage &image, glm::vec3 *pts,glm::vec3 *vns, TGAColor &color, TGAImage diffuse, glm::vec3 vt0, glm::vec3 vt1, glm::vec3 vt2)
{
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

	// on parcours les pixels qui sont dans la boite englobante
	for (int x = minx; x <= maxx; x++)
	{
		for (int y = miny; y <= maxy; y++)
		{
			glm::vec3 p(x, y, 1);
			glm::vec3 bary = m * p;

			// si les coordonnées barycentriques sont positives
			if (bary.x >= -1e-2 && bary.y >= -1e-2 && bary.z >= -1e-2)
			{
				glm::vec2 uv = bary.x * vt0 + bary.y * vt1 + bary.z * vt2;
				glm::vec2 uvPixel = glm::vec2(uv.x*float(diffuse.get_width()),uv.y*float(diffuse.get_height()));
				TGAColor texColor = diffuse.get(uvPixel.x,uvPixel.y);
				// update Z
				p.z = pts[0].z * bary.x + pts[1].z * bary.y + pts[2].z * bary.z;
				if((p.x +p.y*WIDTH )>= WIDTH*HEIGHT || (p.x +p.y*WIDTH )<0)
				{
					continue;
				}
				if (zbuffer[int(p.x + p.y * WIDTH)] < p.z)
				{
					// Smooth normal  Gouraud shading
					glm::vec3 vn = glm::normalize(bary.x * vns[0] + bary.y * vns[1] + bary.z * vns[2]);
					float intensity = glm::clamp(glm::dot(vn,glm::normalize(glm::vec3(0,0,1))),0.0f,1.0f);
					zbuffer[int(p.x + p.y * WIDTH)] = p.z;
					image.set(p.x, p.y, TGAColor (intensity*texColor.r,intensity*texColor.g,intensity*texColor.b,255));
				}
			}
		}
	}
}
struct Face
{
	int v0;
	int v1;
	int v2;
	int vt0, vt1, vt2;
	int vn0, vn1, vn2;
};

int main(int argc, char **argv)
{
	TGAImage image(WIDTH, HEIGHT, TGAImage::RGB);
	TGAImage diffuse;
	if (!diffuse.read_tga_file("obj/african_head/african_head_diffuse.tga"))
	{
		std::cout << "Failed to load diffuse texture" << std::endl;
	}
	else
	{
		std::cout << "Diffuse texture load successfully" << std::endl;
	}
	// initialize to lowest possible value
	zbuffer = new float[WIDTH*HEIGHT];
	for (int i = 0; i < WIDTH*HEIGHT; i++)
	{
		zbuffer[i] = -std::numeric_limits<float>::max();
	}
	std::fstream objfile;
	objfile.open("obj/african_head/african_head.obj");
	if (!objfile.is_open())
	{
		std::cout << "error while opening obj file " << std::endl;
	}
	std::string line;
	std::vector<glm::vec3> vertices;
	std::vector<Face> faces;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> textures;
	while (getline(objfile, line))
	{
		std::istringstream iss(line.c_str());
		char v;
		// vt trash
		std::string trash;
		if (!line.compare(0, 2, "v "))
		{
			glm::vec3 vertex;
			iss >> v;
			iss >> vertex.x;
			iss >> vertex.y;
			iss >> vertex.z;
			vertices.push_back(vertex);
		}
		else if (!line.compare(0, 3, "vt "))
		{
			glm::vec3 texture;
			iss >> trash;
			iss >> texture.x;
			iss >> texture.y;
			texture.y = 1 - texture.y;
			iss >> texture.z; // TRASH
			textures.push_back(texture);
		}
		else if (!line.compare(0, 3, "vn "))
		{
			glm::vec3 normal;
			iss >> trash;
			iss >> normal.x;
			iss >> normal.y;
			iss >> normal.z;
			normals.push_back(normal);
		}
		else if (!line.compare(0, 2, "f "))
		{
			// f 1210/1260/1210 1090/1259/1090 1212/1277/1212
			Face face;
			sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &face.v0, &face.vt0, &face.vn0, &face.v1, &face.vt1, &face.vn1, &face.v2, &face.vt2, &face.vn2);
			faces.push_back(face);
		}
	}
	for (auto &vertex : vertices)
	{
		// std::cout << vertex.x << " ";
		// std::cout << vertex.y << " ";
		// std::cout << vertex.z << std::endl;

		// image 100 x 100 , coordinates -1 to 1
		// image.set((vertex.x + 1) * 50, (vertex.y + 1) * 50, red);
	}
	std::cout << vertices.size() << " vertices" << std::endl;
	std::cout << faces.size() << " faces" << std::endl;

	int half_width = image.get_width() / 2;
	int half_height = image.get_height() / 2;
	glm::vec3 light_dir(0.f,0.f,1.f);
	// rotate the model by 90 degrees
	for (auto &face : faces)
	{
		glm::vec3 v0 = vertices[face.v0 - 1];
		glm::vec3 v1 = vertices.at(face.v1 - 1);
		glm::vec3 v2 = vertices.at(face.v2 - 1);
		//backface culling
		glm::vec3 u = v1 - v0;
		glm::vec3 v = v2 - v0;
		glm::vec3 normal = glm::normalize(glm::cross(u,v));

		glm::vec3 vn0 = normals[face.vn0 -1];
		glm::vec3 vn1 = normals[face.vn1 -1];
		glm::vec3 vn2 = normals[face.vn2 -1];

		if (normal.z < 0)
		{
			continue;
		}

		glm::vec3 vt0 = textures[face.vt0 -1];
		glm::vec3 vt1 = textures[face.vt1 -1];
		glm::vec3 vt2 = textures[face.vt2 -1];

		// v0 = {v0.x, v0.z, v0.y};
		// v1 = {v1.x, v1.z, v1.y};
		// v2 = {v2.x, v2.z, v2.y};
		TGAColor randomcolor = TGAColor(rand() % 255, rand() % 255, rand() % 255, 255);
		glm::vec3 vns[3] = {vn0, vn1, vn2};
		
		float cameraDistance = 2.0f;
		glm::vec3 pts[3] = {
			glm::vec3(v0.x / (1-v0.z/cameraDistance),v0.y / (1-v0.z/cameraDistance),v0.z),
			glm::vec3(v1.x / (1-v1.z/cameraDistance),v1.y / (1-v1.z/cameraDistance),v1.z),
			glm::vec3(v2.x / (1-v2.z/cameraDistance),v2.y / (1-v2.z/cameraDistance),v2.z)
		};
		fill_triangle(image,pts,vns,randomcolor,diffuse,vt0,vt1,vt2);
	}
	image.flip_vertically();
	image.write_tga_file("output.tga");
	return 0;
}
