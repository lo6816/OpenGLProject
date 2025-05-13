#ifndef OBJECT_H
#define OBJECT_H

#include<iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>

#include<glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
// #include "../3rdParty/stb/stb_image.h""




struct Vertex {
	glm::vec3 Position;
	glm::vec2 Texture;
	glm::vec3 Normal;
};


class Object
{
public:
	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> textures;
	std::vector<glm::vec3> normals;
	std::vector<Vertex> vertices;

	// const unsigned int INSTANCE_COUNT = 20;
	// glm::mat4 modelMatrices[20];

	int numVertices;

	GLuint texture;
	// glGenTextures(1, &texture2);
	// glActiveTexture(GL_TEXTURE0);
	// glBindTexture(GL_TEXTURE_2D, texture2);

	GLuint VBO, VAO;
	// --- instancing ---
	GLuint   instanceVBO  = 0;   // VBO contenant les matrices modèle
	GLsizei  instanceCount = 0;  // nombre d’instances à dessiner
	GLuint colorVBO = 0;

	glm::mat4 model = glm::mat4(1.0);


	Object(const std::vector<Vertex>& vertices) {
        this->vertices = vertices;
        this->numVertices = vertices.size();
    }

	Object(const char* path) {

		std::ifstream infile(path);
		//TODO Error management
		std::string line;
		while (std::getline(infile, line))
		{
			std::istringstream iss(line);
			std::string indice;
			iss >> indice;
			//std::cout << "indice : " << indice << std::endl;
			if (indice == "v") {
				float x, y, z;
				iss >> x >> y >> z;
				positions.push_back(glm::vec3(x, y, z));

			}
			else if (indice == "vn") {
				float x, y, z;
				iss >> x >> y >> z;
				normals.push_back(glm::vec3(x, y, z));
			}
			else if (indice == "vt") {
				float u, v;
				iss >> u >> v;
				textures.push_back(glm::vec2(u, v));
			}
			else if (indice == "f") {
				std::string f1, f2, f3;
				iss >> f1 >> f2 >> f3;

				std::string p, t, n;

				//for vertex 1
				Vertex v1;

				p = f1.substr(0, f1.find("/"));
				f1.erase(0, f1.find("/") + 1);

				t = f1.substr(0, f1.find("/"));
				f1.erase(0, f1.find("/") + 1);

				n = f1.substr(0, f1.find("/"));


				v1.Position = positions.at(std::stof(p) - 1);
				v1.Normal = normals.at(std::stof(n) - 1);
				v1.Texture = textures.at(std::stof(t) - 1);
				vertices.push_back(v1);

				//for vertex 2
				Vertex v2;

				p = f2.substr(0, f2.find("/"));
				f2.erase(0, f2.find("/") + 1);

				t = f2.substr(0, f2.find("/"));
				f2.erase(0, f2.find("/") + 1);

				n = f2.substr(0, f2.find("/"));


				v2.Position = positions.at(std::stof(p) - 1);
				v2.Normal = normals.at(std::stof(n) - 1);
				v2.Texture = textures.at(std::stof(t) - 1);
				vertices.push_back(v2);

				//for vertex 3
				Vertex v3;

				p = f3.substr(0, f3.find("/"));
				f3.erase(0, f3.find("/") + 1);

				t = f3.substr(0, f3.find("/"));
				f3.erase(0, f3.find("/") + 1);

				n = f3.substr(0, f3.find("/"));


				v3.Position = positions.at(std::stof(p) - 1);
				v3.Normal = normals.at(std::stof(n) - 1);
				v3.Texture = textures.at(std::stof(t) - 1);
				vertices.push_back(v3);
			}
		}
		//std::cout << positions.size() << std::endl;
		//std::cout << normals.size() << std::endl;
		std::cout << textures.size() << std::endl;
		std::cout << "Load model with " << vertices.size() << " vertices" << std::endl;

		infile.close();

		numVertices = vertices.size();
	}

	
	void makeObject(Shader shader, bool texture = true) {
		/* This is a working but not perfect solution, you can improve it if you need/want
		* What happens if you call this function twice on an Model ?
		* What happens when a shader doesn't have a position, tex_coord or normal attribute ?
		*/

		float* data = new float[8 * numVertices];
		for (int i = 0; i < numVertices; i++) {
			Vertex v = vertices.at(i);
			data[i * 8] = v.Position.x;
			data[i * 8 + 1] = v.Position.y;
			data[i * 8 + 2] = v.Position.z;

			data[i * 8 + 3] = v.Texture.x;
			data[i * 8 + 4] = v.Texture.y;

			data[i * 8 + 5] = v.Normal.x;
			data[i * 8 + 6] = v.Normal.y;
			data[i * 8 + 7] = v.Normal.z;
		}

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		//define VBO and VAO as active buffer and active vertex array
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * numVertices, data, GL_STATIC_DRAW);

		auto att_pos = glGetAttribLocation(shader.ID, "position");
		glEnableVertexAttribArray(att_pos);
		glVertexAttribPointer(att_pos, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)0);

		
		if (texture) {
			auto att_tex = glGetAttribLocation(shader.ID, "tex_coord");
			glEnableVertexAttribArray(att_tex);
			glVertexAttribPointer(att_tex, 2, GL_FLOAT, false, 8 * sizeof(float), (void*)(3 * sizeof(float)));
			
		}
		
		auto att_col = glGetAttribLocation(shader.ID, "normal");
		glEnableVertexAttribArray(att_col);
		glVertexAttribPointer(att_col, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)(5 * sizeof(float)));
		
		//desactive the buffer
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		delete[] data;

	}

	void makeObject2(Shader shader, bool texture = true) {
		/* This is a working but not perfect solution, you can improve it if you need/want
		* What happens if you call this function twice on an Model ?
		* What happens when a shader doesn't have a position, tex_coord or normal attribute ?
		*/

		float* data = new float[8 * numVertices];
		for (int i = 0; i < numVertices; i++) {
			Vertex v = vertices.at(i);
			data[i * 8] = v.Position.x;
			data[i * 8 + 1] = v.Position.y;
			data[i * 8 + 2] = v.Position.z;

			data[i * 8 + 3] = v.Texture.x;
			data[i * 8 + 4] = v.Texture.y;

			data[i * 8 + 5] = v.Normal.x;
			data[i * 8 + 6] = v.Normal.y;
			data[i * 8 + 7] = v.Normal.z;
		}

		glGenVertexArrays(2, &VAO);
		glGenBuffers(2, &VBO);

		//define VBO and VAO as active buffer and active vertex array
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * numVertices, data, GL_STATIC_DRAW);

		auto att_pos = glGetAttribLocation(shader.ID, "position");
		glEnableVertexAttribArray(att_pos);
		glVertexAttribPointer(att_pos, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)0);

		
		if (texture) {
			auto att_tex = glGetAttribLocation(shader.ID, "tex_coord");
			glEnableVertexAttribArray(att_tex);
			glVertexAttribPointer(att_tex, 2, GL_FLOAT, false, 8 * sizeof(float), (void*)(3 * sizeof(float)));
			
		}
		
		auto att_col = glGetAttribLocation(shader.ID, "normal");
		glEnableVertexAttribArray(att_col);
		glVertexAttribPointer(att_col, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)(5 * sizeof(float)));
		
		//desactive the buffer
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		delete[] data;

	}

	// void setModelMatrix() {
	// 	float* data = new float[8 * numVertices];
	// 	for (int i = 0; i < numVertices; i++) {
	// 		Vertex v = vertices.at(i);
	// 		data[i * 8] = v.Position.x;
	// 		data[i * 8 + 1] = v.Position.y;
	// 		data[i * 8 + 2] = v.Position.z;
	// 		data[i * 8 + 3] = v.Texture.x;
	// 		data[i * 8 + 4] = v.Texture.y;
	// 		data[i * 8 + 5] = v.Normal.x;
	// 		data[i * 8 + 6] = v.Normal.y;
	// 		data[i * 8 + 7] = v.Normal.z;
	// 	}
	// 	for (unsigned int i = 0; i < INSTANCE_COUNT; ++i)
	// 	{
	// 		float angle = i * 360.0f / INSTANCE_COUNT;
	// 		float radius = 10.0f;                 // cercle de 10 m de rayon
	// 		float x = cos(glm::radians(angle)) * radius;
	// 		float z = sin(glm::radians(angle)) * radius;
	// 		modelMatrices[i] = glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.0f, z));
	// 		modelMatrices[i] = glm::scale(modelMatrices[i], glm::vec3(0.5f)); // même échelle que ta boule unique		
	// 	}
	// 	GLuint instanceVBO;
	// 	glGenBuffers(1, &instanceVBO);
	// 	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	// 	glBufferData(GL_ARRAY_BUFFER,
	// 				INSTANCE_COUNT * sizeof(Vertex) * numVertices,
	// 				data,
	// 				GL_STATIC_DRAW);
	// }

	// --------------------------------------------------------------------
	// 1) Charge un tableau de matrices modèle dans un VBO et configure le VAO
	void setupInstancing(const std::vector<glm::mat4>& matrices)
	{
		instanceCount = static_cast<GLsizei>(matrices.size());

		if (instanceVBO == 0)
			glGenBuffers(1, &instanceVBO);

		glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
		glBufferData(GL_ARRAY_BUFFER,
					instanceCount * sizeof(glm::mat4),
					matrices.data(),
					GL_STATIC_DRAW);

		glBindVertexArray(VAO);

		std::size_t vec4Size = sizeof(glm::vec4);
		for (int i = 0; i < 4; ++i) {
			glEnableVertexAttribArray(3 + i);
			glVertexAttribPointer(3 + i,
								4,
								GL_FLOAT,
								GL_FALSE,
								sizeof(glm::mat4),
								(void*)(i * vec4Size));
			glVertexAttribDivisor(3 + i, 1);   // une valeur par instance
		}

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	// 2) Exemple : génère `count` instances disposées sur un cercle horizontal,
	//    puis appelle setupInstancing().
	void generateCircleInstances(unsigned int count, float radius = 10.0f, float decalage = 45.0f, float angle = 360.0f)
	{
		std::vector<glm::mat4> mats(count);

		for (unsigned int i = 0; i < count; ++i) {
			// decalage = 45.0f; // Décalage de 45° 
			float angle_i = i * angle / count;
			float x = -cos(glm::radians(angle_i + decalage)) * radius;
			float z = -sin(glm::radians(angle_i + decalage)) * radius;

			glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.0f, z));
			M = glm::scale(M, glm::vec3(0.5f));
			mats[i] = M;
		}

		setupInstancing(mats);
	}

	// 3) Dessin instancié (retombe sur draw() si aucune instance n’est définie)
	void drawInstanced()
	{
		if (instanceCount == 0) {
			draw();
			return;
		}

		glBindVertexArray(VAO);
		glDrawArraysInstanced(GL_TRIANGLES, 0, numVertices, instanceCount);
	}
	// --------------------------------------------------------------------

	void setupInstanceColors(const std::vector<glm::vec3>& colors)
	{
		if (colors.size() != instanceCount) {
			std::cerr << "[Object] instanceCount != colors.size()\n";
			return;
		}
		if (colorVBO == 0) glGenBuffers(1, &colorVBO);

		glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
		glBufferData(GL_ARRAY_BUFFER,
					colors.size() * sizeof(glm::vec3),
					colors.data(),
					GL_STATIC_DRAW);

		glBindVertexArray(VAO);
		// location 7 libre (0-2: géo, 3-6: matrices)
		glEnableVertexAttribArray(7);
		glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
		glVertexAttribDivisor(7, 1);     // ► une couleur par instance
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void draw() {

		glBindVertexArray(this->VAO);
		glDrawArrays(GL_TRIANGLES, 0, numVertices);

	}

	void createText(char file[128]) {
		
		glGenTextures(1, &texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture);

		// //3. Define the parameters for the texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//4. Load the image
		//Carefull depending on where your executable is, the relative path might be different from what you think it is
		//Try to use an absolute path
		//image usually have thei 0.0 at the top of the vertical axis and not the bottom like opengl expects
		stbi_set_flip_vertically_on_load(true);
		int imWidth, imHeight, imNrChannels;
		// char file[128] = "./../textures/moai_text.jpg";
		
		unsigned char* data = stbi_load(file, &imWidth, &imHeight, &imNrChannels, 0);
		if (data)
		{
			printf("image loaded\n imNrChannels: %d\n", imNrChannels);
			if (imNrChannels == 4) {
				printf("image has 4 channels\n");
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imWidth, imHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			} else {
				printf("image has 3 channels\n");
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imWidth, imHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			}
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else {
			std::cout << "Failed to Load texture" << std::endl;
			const char* reason = stbi_failure_reason();
			std::cout << reason << std::endl;
		}

		stbi_image_free(data);

	}

	void drawText() {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
	}

	void centerModel() {
		glm::vec3 center(0.0f);
		for (const auto& vertex : vertices) {
			center += vertex.Position;
		}
		center /= vertices.size(); // Moyenne des positions des sommets
	
		for (auto& vertex : vertices) {
			vertex.Position -= center; // Recentrer les sommets
		}
	}
};
#endif