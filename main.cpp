#include "tgaimage.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdio>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);

void draw_line(TGAImage &image, int x0, int y0, int x1, int y1)
{
	for (float t = 0.; t < 1.; t += .01)
	{
		int x = x0 + (x1 - x0) * t;
		int y = y0 + (y1 - y0) * t;
		image.set(x, y, red);
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
};

int main(int argc, char **argv)
{
	TGAImage image(500, 500, TGAImage::RGB);
	std::fstream objfile;
	objfile.open("obj/african_head/african_head.obj");
	if (!objfile.is_open())
	{
		std::cout << "error while opening obj file " << std::endl;
	}
	std::string line;
	std::vector<Vertex> vertices;
	std::vector<Face> faces;
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
		else if (!line.compare(0, 2, "f "))
		{
			// f 1210/1260/1210 1090/1259/1090 1212/1277/1212
			Face face;
			sscanf(line.c_str(), "f %d/%*d/%*d %d/%*d/%*d %d/%*d/%*d", &face.v0, &face.v1, &face.v2);
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

	for (auto &face : faces)
	{
		Vertex v0 = vertices[face.v0 - 1];
		Vertex v1 = vertices.at(face.v1 - 1);
		Vertex v2 = vertices.at(face.v2 - 1);

		draw_line(image, (v0.x + 1) * half_width, (v0.y + 1) * half_height, (v1.x + 1) * half_width, (v1.y + 1) * half_height);
		draw_line(image, (v1.x + 1) * half_width, (v1.y + 1) * half_height, (v2.x + 1) * half_width, (v2.y + 1) * half_height);
		draw_line(image, (v2.x + 1) * half_width, (v2.y + 1) * half_height, (v0.x + 1) * half_width, (v0.y + 1) * half_height);
	}
	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	return 0;
}
