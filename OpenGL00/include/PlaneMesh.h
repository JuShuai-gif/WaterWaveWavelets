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
	glm::vec3 position;
	//glm::vec3 normal;
	glm::vec4 index;
	//float p_index;
};

class PlaneMesh
{
public:
	PlaneMesh(int nx,int ny) {
		float dx = 2.0f / nx;
		float dy = 2.0f / ny;

		vertices.clear();
		indices.clear();
		int in = 0;
		for (int i = 0; i <= nx; i++) {
			for (int j = 0; j <= ny; j++) {
				Vertex v;
				v.position = glm::vec3{ -1.0f + i * dx,-1.0f + j * dy ,0.0f };
				//v.p_index = in;
				//v.p_index = float(in/484.0f);
				//std::cout << "in / 120: " << v.p_index << std::endl;
				//in++;
				float ff = 1 / 484.0f;
				for (size_t k = 0; k < 4; ++k) {
					v.index[k] = float(in / 484.0f);
					in++;
					glm::vec4 vv(0.21f, 0.45f, 0.76f, 0);
					amplitudeData.push_back(vv);
				}
				vertices.push_back(v);
			}
		}
		TexTureWidth = amplitudeData.size();
		//std::cout << "in: " << in << std::endl;
		//std::cout << "�ܹ��ж��ٸ����㣺" << vertices.size() << std::endl;
		//std::cout << "���㣺" << vertices.size() * 16 << std::endl;
		//std::cout << "�ܹ��ж��ٸ�����ֵ��" << amplitudeData.size() << std::endl;
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
		// ����һ���������
		glGenTextures(1, &profileBuffer_TexID);
		glBindTexture(GL_TEXTURE_1D, profileBuffer_TexID);

		// �����������
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// �����ݰ󶨵�����
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, profileBuffer.m_data.size(), 0, GL_RGBA, GL_FLOAT, profileBuffer.m_data.data());

		// �������
		glBindTexture(GL_TEXTURE_1D, 0);


		// �����ݸ��Ƶ�Ŀ���ڴ���
		std::vector<float> flatData(amplitudeData.size() * 4); // �������ڴ洢��������Ŀ������
		memcpy(flatData.data(), amplitudeData.data(), amplitudeData.size() * sizeof(glm::vec4));

		glGenTextures(1, &amplitude_TexID);
		glBindTexture(GL_TEXTURE_1D, amplitude_TexID);

		// �����������С�ͷŴ���˷�ʽ
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// ��������Ļ��Ʒ�ʽ������ʹ�� GL_CLAMP_TO_EDGE ����ֹ�߽��ظ���
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

		// �� std::vector<glm::vec4> �������ϴ���������
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, flatData.size(), 0, GL_RGBA, GL_FLOAT, flatData.data());

		// ���������󣨿�ѡ�������������
		glBindTexture(GL_TEXTURE_1D, 0);
	}

	void testTex(WaterWavelets::ProfileBuffer const& profileBuffer)
{
		//std::vector<float> data = {
		//			0.3f, 0.1f, 0.2f, 1.0f,
		//			0.4f, 0.4f, 0.6f, 1.0f,
		//			0.12f, 0.15f, 0.18f, 1.0f,
		//			0.78f, 0.46f, 0.47f, 1.0f
		//};
		std::vector<float> profileData(profileBuffer.m_data.size() * 4); // �������ڴ洢��������Ŀ������
		memcpy(profileData.data(), profileBuffer.m_data.data(), profileBuffer.m_data.size() * sizeof(float) * 4);
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_1D, texture);

		// �����������С�ͷŴ���˷�ʽ
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// ��������Ļ��Ʒ�ʽ������ʹ�� GL_CLAMP_TO_EDGE ����ֹ�߽��ظ���
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

		// �� std::vector<float> �������ϴ���������
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, profileBuffer.m_data.size() / 4, 0, GL_RGBA, GL_FLOAT, profileBuffer.m_data.data());

		// ���������󣨿�ѡ�������������
		glBindTexture(GL_TEXTURE_1D, 0);
		
		
		//// �����ݸ��Ƶ�Ŀ���ڴ���
		std::vector<float> flatData(amplitudeData.size() * 4); // �������ڴ洢��������Ŀ������
		memcpy(flatData.data(), amplitudeData.data(), amplitudeData.size() * sizeof(glm::vec4));
		glGenTextures(1, &texture1);
		glBindTexture(GL_TEXTURE_1D, texture1);

		// �����������С�ͷŴ���˷�ʽ
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// ��������Ļ��Ʒ�ʽ������ʹ�� GL_CLAMP_TO_EDGE ����ֹ�߽��ظ���
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

		// �� std::vector<float> �������ϴ���������
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, flatData.size() / 4, 0, GL_RGBA, GL_FLOAT, flatData.data());

		// ���������󣨿�ѡ�������������
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

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, index));
		std::cout << "sizeof(Vertex): " << sizeof(Vertex) << "offsetof(Vertex, id): " << offsetof(Vertex, index) << std::endl;
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

public:
	// mesh Data
	std::vector<Vertex>       vertices;
	std::vector<unsigned int> indices;

	std::vector<glm::vec4> amplitudeData;
	int TexTureWidth;

	GLuint profileBuffer_TexID;
	GLuint amplitude_TexID;

	unsigned int texture;
	unsigned int texture1;

	unsigned int VAO;
	// render data 
	unsigned int VBO, EBO;
};
