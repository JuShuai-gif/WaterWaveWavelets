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


class Plane
{
public:

	Plane(int nx, int ny) {
		float dx = 2.0f / nx;
		float dy = 2.0f / ny;

		vertices.clear();

		for (int i = 0; i <= nx; i++) {
			for (int j = 0; j <= ny; j++) {
				Vertex v;
				v.Position = glm::vec3{ -1.0f + i * dx ,-1.0f + j * dy, 0.0f };
				v.Normal = glm::vec3{ 0.f, 0.f, 1.0f };
				vertices.push_back(v);
			}
		}
		//print_vec(vertices[vertices.size() - 1].Position);
		//print_vec(vertices[0].Position);


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
		for (auto& d : vertices) {
			d.Normal = glm::vec3{ 0.f, 0.f, 0.f };
		}

		for (size_t i = 0; i < indices.size();) {

			auto id1 = indices[i++];
			auto id2 = indices[i++];
			auto id3 = indices[i++];

			auto v1 = vertices[id1].Position;
			auto v2 = vertices[id2].Position;
			auto v3 = vertices[id3].Position;

			// This does weighted area based on triangle area
			auto n = cross((v2 - v1), (v3 - v1));

			vertices[id1].Normal += n;
			vertices[id2].Normal += n;
			vertices[id3].Normal += n;
		}

		for (auto& d : vertices)
			d.Normal = glm::normalize(d.Normal);
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
