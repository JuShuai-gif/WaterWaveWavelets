#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include "../include/ProfileBuffer.h"
#include "../include/DirectionNumber.h"

struct VertexData {
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec4> colors;
};


class PlaneMesh
{
public:
	
	PlaneMesh(int nx,int ny) {
		float dx = 2.0f / nx;
		float dy = 2.0f / ny;

		_data.positions.clear();
		_data.normals.clear();
		_data.colors.clear();
		for (int i = 0; i <= nx; i++) {
			for (int j = 0; j <= ny; j++) {
				VertexData vertex;
				_data.positions.push_back(glm::vec3{ -1.0f + i * dx, -1.0f + j * dy, 0.0f });
				_data.normals.push_back(glm::vec3{ 0.f, 0.f, 1.0f });
				_data.colors.push_back(glm::vec4{ 1.f, 1.f, 1.f, 1.f });
			}
		}

		_indices.clear();
		for (int i = 0; i < nx; i++) {
			for (int j = 0; j < ny; j++) {

				const int idx = j + i * (ny + 1);
				const int J = 1;
				const int I = ny + 1;

				_indices.push_back(idx);
				_indices.push_back(idx + I);
				_indices.push_back(idx + J);

				_indices.push_back(idx + I);
				_indices.push_back(idx + I + J);
				_indices.push_back(idx + J);
			}
		}

	}

	void CreatMesh() 
	{

	}

	void loadProfile(WaterWavelets::ProfileBuffer const& profileBuffer) 
	{

	}

	void bindBuffers(std::vector<VertexData> const& data)
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(_data.), vertices, GL_STATIC_DRAW);

		glBindVertexArray(VAO);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
	}

	void draw() 
	{

	}

	void recomputeNormals(VertexData& data)
	{
		for (auto& d : data.normals) {
			d = glm::vec3{ 0.f, 0.f, 0.f };
		}

		for (size_t i = 0; i < _indices.size();) {

			auto id1 = _indices[i++];
			auto id2 = _indices[i++];
			auto id3 = _indices[i++];

			auto v1 = data.positions[id1];
			auto v2 = data.positions[id2];
			auto v3 = data.positions[id3];

			// This does weighted area based on triangle area
			auto n = cross((v2 - v1), (v3 - v1));

			data.normals[id1] += n;
			data.normals[id2] += n;
			data.normals[id3] += n;
		}

		for (auto& d : data.normals)
			d = glm::normalize(d);
	}

	template <class Fun> void setVertices(Fun fun) {
		std::vector<VertexData> newData = _data;

		for (size_t i = 0; i < newData.size(); i++) {
			fun(i, newData[i]);
		}
		recomputeNormals(newData);
		bindBuffers(newData);
	}

private:
	GLuint VAO, VBO;


	VertexData _data;
	std::vector<unsigned int> _indices;
};
