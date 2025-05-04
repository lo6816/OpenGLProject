#ifndef PARTICLE_H
#define PARTICLE_H

#include <cmath>
#include<iostream>

//include glad before GLFW to avoid header conflict or define "#define GLFW_INCLUDE_NONE"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"
// #include "../3rdParty/stb/stb_image.h"


#include "./camera.h"
#include "./shader.h"
#include "./object.h"

#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/random.hpp>


#include <algorithm>
#include <chrono>
#include <thread>
#include <glm/gtx/color_space.hpp>

const int MaxParticles = 1000;

// Fonction pour contrôler la taille globale de la flamme/fumée
float getFlameScale()
{
    return 0.4f; // Modifier cette valeur pour agrandir ou réduire la flamme
}

struct Particle {
	glm::vec3 pos, speed;
	glm::vec4 color;
	float life;
	float size;
	float cameraDist; //Squared distance to the camera position

	Particle() : pos(0.0f), speed(0.0f), color(1.0f), life(0.0f), size(0.0f), cameraDist(0.0f) {}

	bool operator<(const Particle& otherP) const {
		//sort in reverse order, first particles that are further away
		return this->cameraDist > otherP.cameraDist;
	}
};



class ParticleSystem
{
private:
    Camera camera;
    Shader shader;
    Particle particlesContainer[MaxParticles];
    GLuint VBO_vertex, VBO_position, VBO_color, VAO;

    int lastUsedParticle = 0;
    const float vertexData[18] = {
        // vertices
        -1.0, -1.0, 0.0,
        1.0, -1.0, 0.0,
        -1.0, 1.0, 0.0,
        1.0, 1.0, 0.0,
        -1.0, 1.0, 0.0,
        1.0, -1.0, 0.0
    };
    GLfloat* g_particule_position_size_data;
    GLfloat* g_particule_color_data;
    double lastTime = 0.0;

    // ParticleSystem(Camera camera, Shader shader) : camera(camera), shader(shader) {}

    int findUnusedParticle() {
        for (int i = lastUsedParticle; i < MaxParticles; i++) {
            if (particlesContainer[i].life < 0) {
                lastUsedParticle = i;
                return i;
            }
        }

        for (int i = 0; i < lastUsedParticle; i++) {
            if (particlesContainer[i].life < 0) {
                lastUsedParticle = i;
                return i;
            }
        }

        return 0;
    }

    void addParticles(const int newParticle, glm::vec3 firePosition)
    {
        for (int i = 0; i < newParticle; i++) {
            int particleIdx = findUnusedParticle();
            particlesContainer[particleIdx].life = (15.0f + rand() % 10 / 10.0);
            // --- Hauteur de départ concentrée à la base, 10% longues ---
            float randHeight = glm::linearRand(0.0f, 1.0f);
            float flameScale = getFlameScale();
            // Base plus basse et plus large, échelle appliquée
            float height = (randHeight < 0.9f) ? glm::linearRand(0.0f, 0.2f * flameScale) : glm::linearRand(0.5f * flameScale, 2.5f * flameScale);
            // Position de la particule dépendante de firePosition
            particlesContainer[particleIdx].pos = firePosition + glm::vec3(0.0, height, 0.0) + glm::ballRand(0.12f * flameScale);

            // Probabilité que certaines particules (fumée noire) aillent plus haut
            bool isHighSmoke = (randHeight >= 0.9f && glm::linearRand(0.0f, 1.0f) > 0.7f);
            float angle = glm::linearRand(0.0f, 2.0f * 3.1415f);
            float radial = glm::linearRand(0.0f, 0.18f); // un peu plus large
            glm::vec3 dirXZ = glm::vec3(glm::cos(angle), 0.0f, glm::sin(angle)) * radial * 0.12f * flameScale;
            float vertical = isHighSmoke ? glm::linearRand(2.5f, 4.0f) * flameScale : ((randHeight < 0.9f) ? glm::linearRand(0.5f, 1.2f) * flameScale : glm::linearRand(1.8f, 2.5f) * flameScale);
            particlesContainer[particleIdx].speed = dirXZ + glm::vec3(0.0f, vertical, 0.0f);

            //use hsv color to get pretty results
            if (isHighSmoke) {
                // Fumée noire : S=0, V plus bas, alpha plus faible
                particlesContainer[particleIdx].color = glm::vec4(0.0, 0.0, glm::linearRand(0.1f, 0.35f), 0.25f + glm::linearRand(0.0f, 0.15f));
            } else {
                particlesContainer[particleIdx].color = glm::vec4(45.0, 1.0, 1.0, 0.7); // jaune vif, alpha un peu plus fort pour la base
            }

            particlesContainer[particleIdx].size = 0.08f;
        }

    }

    void simulateParticles(int &particleCount ,float delta, GLfloat* g_particule_position_size_data, GLfloat* g_particule_color_data )
    {
        //Simulate the particle
        for (int i = 0; i < MaxParticles; i++) {
            Particle& p = particlesContainer[i]; //shortcut
            float flameScale = getFlameScale();
            float h = p.pos.y;  // Hauteur de la particule
            float h_norm = glm::clamp(h / (2.5f * flameScale), 0.0f, 1.0f);

            if (p.life > 0.0) {
                //decrease life, use time since last frame
                p.life -= delta*10;

                //change of pos
                p.pos += p.speed * (float)delta;
                // Ajout d'un vortex visuel
                float adjustvortex = 0.5f;
                p.pos.x += adjustvortex * 0.006f * glm::linearRand(0.0,glm::sin(glfwGetTime() * 4.0f + p.pos.y));
                p.pos.z += adjustvortex * 0.006f * glm::linearRand(0.0,glm::cos(glfwGetTime() * 3.5f + p.pos.y));

                // Dissipation plus forte pour les particules très hautes
                if (p.pos.y > 3.0f) {
                    p.speed *= 0.95f;
                    p.color.a *= 0.96f;
                }

                // Rends les particules noires plus visibles en haut
                if (p.color.g < 0.2f && p.pos.y > 2.0f) {
                    p.size = 0.12f; // accentuer la visibilité des fumées noires hautes
                    p.color.a = glm::clamp(p.color.a + 0.02f, 0.0f, 0.6f); // éviter qu’elles disparaissent trop vite
                }

                // Atténuation latérale selon la hauteur (base se resserre plus vite)
                float attenuation = glm::clamp(1.2f - p.pos.y, 0.0f, 1.0f);
                p.speed.x *= attenuation;
                p.speed.z *= attenuation;

                // Accentue la base large et sommet fin
                p.size = glm::mix(0.12f, 0.04f, h_norm);

                // Interpolation HSV selon la hauteur
                if (p.color.g < 0.2f) {
                    // Fumée noire : ne pas changer la couleur
                } else if (h_norm < 0.4f) {
                    // Jaune → Rouge
                    float t = h_norm / 0.5f;
                    p.color.r = glm::mix(45.0f, 10.0f, t);  // Hue
                    p.color.g = 1.0f; // Saturation
                    p.color.b = 1.0f; // Valeur
                }
                else {
                    // Rouge → Gris
                    float t = (h_norm - 0.4f) / 0.6f;
                    p.color.r = 0.0f; // Hue n'importe peu quand S = 0
                    p.color.g = glm::mix(1.0f, 0.0f, t); // Saturation
                    p.color.b = glm::mix(1.0f, 0.2f, t); // Valeur (luminosité)
                }

                p.color.a -= (float)delta * 0.05;

                //update distance with the camera
                p.cameraDist = glm::length2(p.pos - camera.GetCameraPosition());
                
                
                //fill the gpu buffer
                g_particule_position_size_data[4 * particleCount] = p.pos.x;
                g_particule_position_size_data[4 * particleCount + 1] = p.pos.y;
                g_particule_position_size_data[4 * particleCount + 2] = p.pos.z;

                g_particule_position_size_data[4 * particleCount + 3] = p.size;

                glm::vec3 hsv = glm::vec3(p.color.r, p.color.g, p.color.b);
                glm::vec3 rgb = glm::rgbColor(hsv);
                g_particule_color_data[4 * particleCount + 0] = rgb.r;
                g_particule_color_data[4 * particleCount + 1] = rgb.g;
                g_particule_color_data[4 * particleCount + 2] = rgb.b;
                g_particule_color_data[4 * particleCount + 3] = p.color.a;

                particleCount++;
            }
            else {
                //make sure all dead particle will be put at the end of the list
                p.cameraDist = -1;
            }
        }
            
    }

    void sortParticles() {
        std::sort(&particlesContainer[0], &particlesContainer[MaxParticles]);
    }

public:

    ParticleSystem(Camera camera, Shader shader) : camera(camera), shader(shader) {}

    void initParticleSystem()
    {
        g_particule_position_size_data = new GLfloat[MaxParticles * 4];
        g_particule_color_data = new GLfloat[MaxParticles * 4];

        for (int i = 0; i < MaxParticles; i++) {
            particlesContainer[i].life = -1.0;
        }

        //Create the vertex buffer objects for the quad used as a particle, 
        //and the positions and colors of all particle
        //the same vertex array object is used for all 3 VBOs
       
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO_vertex);
        glGenBuffers(1, &VBO_position);
        glGenBuffers(1, &VBO_color);

        //define VBO and VAO as active buffer and active vertex array
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO_vertex);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

        auto att_vertex = glGetAttribLocation(shader.ID, "vertex");
        glEnableVertexAttribArray(att_vertex);
        glVertexAttribPointer(att_vertex, 3, GL_FLOAT, false, 0, 0);
        glVertexAttribDivisor(att_vertex, 0);


        glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
        glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GL_FLOAT), NULL, GL_STREAM_DRAW);

        auto att_center = glGetAttribLocation(shader.ID, "center");
        glEnableVertexAttribArray(att_center);
        glVertexAttribPointer(att_center, 4, GL_FLOAT, false, 0, 0);
        glVertexAttribDivisor(att_center, 1);
        

        glBindBuffer(GL_ARRAY_BUFFER, VBO_color);
        glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

        auto att_col = glGetAttribLocation(shader.ID, "col");
        glEnableVertexAttribArray(att_col);
        glVertexAttribPointer(att_col, 4, GL_FLOAT, true, 0, 0);
        glVertexAttribDivisor(att_col, 1);

        //desactive the buffer
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        //We want our particle semi-transparents to actually look like smoke
        glEnable(GL_BLEND);
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    }

    void drawParticles(glm::mat4 view, glm::mat4 perspective, glm::vec3 cameraRight, glm::vec3 cameraUp)
    {
        double currentTime = glfwGetTime();
        double delta = currentTime - lastTime;
        lastTime = currentTime;
        int newParticle = delta*1000.0f;
        if (newParticle > (int)(0.032f * 1000.0)) newParticle = (int)(0.032f * 1000.0);
        addParticles(newParticle, glm::vec3(0.0f, 0.0f, 0.0f));
        int particleCount = 0;
        simulateParticles(particleCount, delta, g_particule_position_size_data, g_particule_color_data);
        sortParticles();

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
        glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GL_FLOAT), NULL, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particleCount * sizeof(GLfloat) * 4, g_particule_position_size_data);

        glBindBuffer(GL_ARRAY_BUFFER, VBO_color);
        glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particleCount * sizeof(GLfloat) * 4, g_particule_color_data);
    
        shader.use();
		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		shader.setVector3f("cameraRight", cameraRight);
		shader.setVector3f("cameraUp", cameraUp);

		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, particleCount);
        
    }

    void cleanUp()
    {
        delete[] g_particule_color_data;
        delete[] g_particule_position_size_data;

        //clean up VBO and VAO
        glDeleteBuffers(1, &VBO_vertex);
        glDeleteBuffers(1, &VBO_position);
        glDeleteBuffers(1, &VBO_color);

        glDeleteVertexArrays(1, &VAO);

        glDeleteProgram(shader.ID);
    }
};

#endif