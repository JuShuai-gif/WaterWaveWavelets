#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>
#include "shader.h"

#include "ProfileBuffer.h"
#include "DirectionNumber.h"
#include "Utils.h"

struct Vertex {
	// position
	glm::vec3 Position;
	// normal
	glm::vec3 Normal;


};


class PlaneMesh
{
public:
	
	PlaneMesh(int nx,int ny) {
		std::vector<glm::vec3> pos;
		float dx = 2.0f / nx;
		float dy = 2.0f / ny;

		vertices.clear();
		indices.clear();

		for (int i = 0; i <= nx; i++) {
			for (int j = 0; j <= ny; j++) {
				Vertex v;
				v.Position = glm::vec3{ -1.0f + i * dx, 0.0f ,-1.0f + j * dy };
				v.Normal = glm::vec3{ 0.f, 1.f, 0.0f };
				vertices.push_back(v);
			}
		}
		print_vec(vertices[vertices.size() - 1].Position);
		print_vec(vertices[0].Position);


		indices.clear();
		for (int i = 0; i < nx; i++) {
			for (int j = 0; j < ny; j++) {

				const int idx = j + i * (ny + 1);
				const int J = 1;
				const int I = ny + 1;

				indices.push_back(idx);
				indices.push_back(idx + I);
				indices.push_back(idx + J);

				indices.push_back(idx + I);
				indices.push_back(idx + I + J);
				indices.push_back(idx + J);
			}
		}
		bindBuffers();
	}

	void loadProfile(WaterWavelets::ProfileBuffer const& profileBuffer) 
	{
		// 创建一个纹理对象
		GLuint textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_1D, textureID);

		// 设置纹理参数
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// 将数据绑定到纹理
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, profileBuffer.m_data.size(), 0, GL_RGBA, GL_FLOAT, profileBuffer.m_data.data());

		// 解绑纹理
		glBindTexture(GL_TEXTURE_1D, 0);

	}

	void bindBuffers()
	{
		// create buffers/arrays
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);
		// load data into vertex buffers
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		// A great thing about structs is that their memory layout is sequential for all its items.
		// The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
		// again translates to 3/2 floats which translates to a byte array.
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		// set the vertex attribute pointers
		// vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	}

	void draw() 
	{
		// draw mesh
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	void recomputeNormals()
	{
		//for (auto& d : vertices) {
		//	d = glm::vec3{ 0.f, 0.f, 0.f };
		//}

		//for (size_t i = 0; i < _indices.size();) {

		//	auto id1 = _indices[i++];
		//	auto id2 = _indices[i++];
		//	auto id3 = _indices[i++];

		//	auto v1 = _data.positions[id1];
		//	auto v2 = _data.positions[id2];
		//	auto v3 = _data.positions[id3];

		//	// This does weighted area based on triangle area
		//	auto n = cross((v2 - v1), (v3 - v1));

		//	_data.normals[id1] += n;
		//	_data.normals[id2] += n;
		//	_data.normals[id3] += n;
		//}

		//for (auto& d : _data.normals)
		//	d = glm::normalize(d);
	}

	template <class Fun> void setVertices(Fun fun) {
		std::vector<Vertex> newData = vertices;

		for (size_t i = 0; i < newData.size(); i++) {
			fun(i, newData[i]);
		}
		recomputeNormals(newData);
		bindBuffers(newData);
	}

private:
	// mesh Data
	std::vector<Vertex>       vertices;
	std::vector<unsigned int> indices;

	unsigned int VAO;
	// render data 
	unsigned int VBO, EBO;
};
