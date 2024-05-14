#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>

struct Face
{
    int v0;
    int v1;
    int v2;
    int vt0, vt1, vt2;
    int vn0, vn1, vn2;
};

struct Model
{
    std::vector<glm::vec3> vertices;
    std::vector<Face> faces;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> textures;
    TGAImage diffuse, normal, specular;
    void parse(std::string &model_name)
    {
        if (!this->diffuse.read_tga_file(std::string("obj/" + model_name + "/" + model_name + "_diffuse.tga").c_str()))
        {
            std::cout << "Failed to load diffuse texture" << std::endl;
        }
        else
        {
            std::cout << "Diffuse texture load successfully" << std::endl;
        }
        if (!this->normal.read_tga_file(std::string("obj/" + model_name + "/" + model_name + "_nm.tga").c_str()))
        {
            std::cout << "Failed to load normal texture" << std::endl;
        }
        else
        {
            std::cout << "Normal texture load successfully" << std::endl;
        }
        if (!this->specular.read_tga_file(std::string("obj/" + model_name + "/" + model_name + "_spec.tga").c_str()))
        {
            std::cout << "Failed to load specular texture" << std::endl;
        }
        else
        {
            std::cout << "Specular texture load successfully" << std::endl;
        }
        std::fstream objfile;
        objfile.open(std::string("obj/" + model_name + "/" + model_name + ".obj").c_str());
        if (!objfile.is_open())
        {
            std::cout << "error while opening obj file " << std::endl;
        }
        std::string line;
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
                this->vertices.push_back(vertex);
            }
            else if (!line.compare(0, 3, "vt "))
            {
                glm::vec3 texture;
                iss >> trash;
                iss >> texture.x;
                iss >> texture.y;
                texture.y = 1 - texture.y;
                iss >> texture.z; // TRASH
                this->textures.push_back(texture);
            }
            else if (!line.compare(0, 3, "vn "))
            {
                glm::vec3 normal;
                iss >> trash;
                iss >> normal.x;
                iss >> normal.y;
                iss >> normal.z;
                this->normals.push_back(normal);
            }
            else if (!line.compare(0, 2, "f "))
            {
                // f 1210/1260/1210 1090/1259/1090 1212/1277/1212
                Face face;
                sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &face.v0, &face.vt0, &face.vn0, &face.v1, &face.vt1, &face.vn1, &face.v2, &face.vt2, &face.vn2);
                this->faces.push_back(face);
            }
        }

        std::cout << this->vertices.size() << " vertices" << std::endl;
        std::cout << this->faces.size() << " faces" << std::endl;
    }
};
