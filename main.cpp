#include "tgaimage.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdio>

#include <glm/glm.hpp>

#define WIDTH 500
#define HEIGHT 500

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
int *zbuffer;

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

void fill_triangle(TGAImage &image,int x0, int y0, int x1, int y1, int x2, int y2,TGAColor &color){
	// boites englobantes
	int minx = glm::min(x0, glm::min(x1, x2));
	int miny = glm::min(y0, glm::min(y1, y2));

	int maxx = glm::max(x0, glm::max(x1, x2));
	int maxy = glm::max(y0, glm::max(y1, y2));

	glm::mat3 m(x0, y0,1, x1, y1, 1, x2, y2, 1);
	m = glm::inverse(m);


	// on parcours les pixels qui sont dans la boite englobante
	for (int x = minx; x <= maxx; x++)
	{
		for (int y = miny; y <= maxy; y++)
		{
			glm::vec3 p(x, y, 1);
			glm::vec3 bary = m * p;

			// si les coordonnées barycentriques sont positives
			if (bary.x >= 0 && bary.y >= 0 && bary.z >= 0)
			{
				if (zbuffer[int(p.x + p.y * WIDTH)] < p.z)
				{
					zbuffer[int(p.x + p.y * WIDTH)] = p.z;
					image.set(p.x, p.y, color);
				}
			}
		}
	}
}

struct Vertex
{
	float x;
	float y;
	float z;
};
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
	zbuffer = new int[WIDTH*HEIGHT];
	std::fstream objfile;
	objfile.open("obj/african_head/african_head.obj");
	if (!objfile.is_open())
	{
		std::cout << "error while opening obj file " << std::endl;
	}
	std::string line;
	std::vector<Vertex> vertices;
	std::vector<Face> faces;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> textures;
	while (getline(objfile, line))
	{
		std::istringstream iss(line.c_str());
		char v;
		if (!line.compare(0, 2, "v "))
		{
			Vertex vertex;
			iss >> v;
			iss >> vertex.x;
			iss >> vertex.y;
			iss >> vertex.z;
			vertices.push_back(vertex);
		}
		else if (!line.compare(0, 3, "vt "))
		{
			glm::vec3 texture;
			iss >> v;
			iss >> texture.x;
			iss >> texture.y;
			iss >> texture.z;
			textures.push_back(texture);
		}
		else if (!line.compare(0, 3, "vn "))
		{
			glm::vec3 normal;
			iss >> v;
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

	// rotate the model by 90 degrees
	for (auto &face : faces)
	{
		Vertex v0 = vertices[face.v0 - 1];
		Vertex v1 = vertices.at(face.v1 - 1);
		Vertex v2 = vertices.at(face.v2 - 1);
		//backface culling
		glm::vec3 u = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
		glm::vec3 v = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
		glm::vec3 normal = glm::cross(u, v);
		if (normal.z < 0)
		{
			continue;
		}

		// v0 = {v0.x, v0.z, v0.y};
		// v1 = {v1.x, v1.z, v1.y};
		// v2 = {v2.x, v2.z, v2.y};
		TGAColor randomcolor = TGAColor(rand() % 255, rand() % 255, rand() % 255, 255);
		fill_triangle(image, (v0.x + 1) * half_width, (v0.y + 1) * half_height, (v1.x + 1) * half_width, (v1.y + 1) * half_height, (v2.x + 1) * half_width, (v2.y + 1) * half_height,randomcolor);
	}
	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	return 0;
}
