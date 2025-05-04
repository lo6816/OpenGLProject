#include <cmath>
#include<iostream>

//include glad before GLFW to avoid header conflict or define "#define GLFW_INCLUDE_NONE"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"



#include "./utils/camera.h"
#include "./utils/shader.h"
#include "./utils/object.h"
#include "./utils/particles.h"

#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/random.hpp>


#include <algorithm>
#include <chrono>
#include <thread>
#include <glm/gtx/color_space.hpp>


const int width = 500;
const int height = 500;


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

#ifndef NDEBUG
//source: https://learnopengl.com/In-Practice/Debugging
void APIENTRY glDebugOutput(GLenum source,
	GLenum type,
	unsigned int id,
	GLenum severity,
	GLsizei length,
	const char* message,
	const void* userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
	if (id == 1282) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}
#endif


Camera camera(glm::vec3(0.0, 1.0, 0.0));

// struct Particle {
// 	glm::vec3 pos, speed;
// 	glm::vec4 color;
// 	float life;
// 	float size;
// 	float cameraDist; //Squared distance to the camera position

// 	Particle() : pos(0.0f), speed(0.0f), color(1.0f), life(0.0f), size(0.0f), cameraDist(0.0f) {}

// 	bool operator<(const Particle& otherP) const {
// 		//sort in reverse order, first particles that are further away
// 		return this->cameraDist > otherP.cameraDist;
// 	}
// };

// const int MaxParticles = 1000;
// const int MaxParticles = 1000000;
Particle particlesContainer[MaxParticles];

int lastUsedParticle = 0;

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


int main(int argc, char* argv[])
{
	std::cout << "Welcome to the fourth exercise session for VR" << std::endl;
	std::cout << "Try to make a particle system\n"
		;

	//Boilerplate
	//Create the OpenGL context 
	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialise GLFW \n");
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
	//A1. Request a debug context with glfw 
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif


	//Create the window
	GLFWwindow* window = glfwCreateWindow(width, height, "Labo 04", nullptr, nullptr);
	if (window == NULL)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window\n");
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	//load openGL function
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		throw std::runtime_error("Failed to initialize GLAD");
	}
	glEnable(GL_DEPTH_TEST);
#ifndef NDEBUG
	//2. Tell OpenGL to enable debug output and to call the glDebugOutput function(defined above in this code)\n
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
#endif

	char fileVert[128] = PATH_TO_SHADERS"/part.vert";
	char fileFrag[128] = PATH_TO_SHADERS"/part.frag";

    char pathSphereF[] = PATH_TO_SHADERS "/fragSrc.txt";
    char pathSphereV[] = PATH_TO_SHADERS "/vertSrc.txt";

	Shader shader(fileVert, fileFrag);
    Shader shader2(pathSphereV, pathSphereF);

    char pathCub[] = PATH_TO_OBJECTS "/campfire.obj"; 
	Object sun(pathCub);
	sun.makeObject(shader2, true);
    char file3[128] = "./../textures/plage.png";
	sun.createText(file3);

    glm::mat4 modelSun = glm::mat4(1.0f);
    modelSun = glm::translate(modelSun, glm::vec3(0.0, 0.0, 0.0));
	glm::mat4 inverseModelSun= glm::transpose(glm::inverse(modelSun));

    ParticleSystem particleSystem(camera, shader);

	// // First object!
	// const float vertexData[18] = {
	// 	// vertices
	// 	-1.0, -1.0, 0.0,
	// 	1.0, -1.0, 0.0,
	// 	-1.0, 1.0, 0.0,
	// 	1.0, 1.0, 0.0,
	// 	-1.0, 1.0, 0.0,
	// 	1.0, -1.0, 0.0
	// };


	// static GLfloat* g_particule_position_size_data = new GLfloat[MaxParticles * 4];
	// static GLfloat* g_particule_color_data = new GLfloat[MaxParticles * 4];

	// for (int i = 0; i < MaxParticles; i++) {
	// 	particlesContainer[i].life = -1.0;
	// }

	// //Create the vertex buffer objects for the quad used as a particle, 
	// //and the positions and colors of all particle
	// //the same vertex array object is used for all 3 VBOs
	// GLuint VBO_vertex, VBO_position, VBO_color, VAO;
	// glGenVertexArrays(1, &VAO);
	// glGenBuffers(1, &VBO_vertex);
	// glGenBuffers(1, &VBO_position);
	// glGenBuffers(1, &VBO_color);

	// //define VBO and VAO as active buffer and active vertex array
	// glBindVertexArray(VAO);

	// glBindBuffer(GL_ARRAY_BUFFER, VBO_vertex);
	// glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	// auto att_vertex = glGetAttribLocation(shader.ID, "vertex");
	// glEnableVertexAttribArray(att_vertex);
	// glVertexAttribPointer(att_vertex, 3, GL_FLOAT, false, 0, 0);
	// glVertexAttribDivisor(att_vertex, 0);


	// glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
	// glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GL_FLOAT), NULL, GL_STREAM_DRAW);

	// auto att_center = glGetAttribLocation(shader.ID, "center");
	// glEnableVertexAttribArray(att_center);
	// glVertexAttribPointer(att_center, 4, GL_FLOAT, false, 0, 0);
	// glVertexAttribDivisor(att_center, 1);
	

	// glBindBuffer(GL_ARRAY_BUFFER, VBO_color);
	// glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

	// auto att_col = glGetAttribLocation(shader.ID, "col");
	// glEnableVertexAttribArray(att_col);
	// glVertexAttribPointer(att_col, 4, GL_FLOAT, true, 0, 0);
	// glVertexAttribDivisor(att_col, 1);

	// //desactive the buffer
	// glBindBuffer(GL_ARRAY_BUFFER, 0);
	// glBindVertexArray(0);


	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 perspective = camera.GetProjectionMatrix();

	glm::vec3 cameraRight = camera.GetCameraRight();
	glm::vec3 cameraUp = camera.GetCameraUp();
	glm::vec3 cameraPosition = camera.GetCameraPosition();
	
	
	// glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int window_width, window_height;
	glfwGetFramebufferSize(window, &window_width, &window_height);

	double prev = 0;
	int deltaFrame = 0;
	//fps function
	auto fps = [&](double now) {
		double deltaTime = now - prev;
		deltaFrame++;
		if (deltaTime > 0.5) {
			prev = now;
			const double fpsCount = (double)deltaFrame / deltaTime;
			deltaFrame = 0;
			std::cout << "\r FPS: " << fpsCount;
		}
	};


	//sync with the screen refresh rate
	glfwSwapInterval(1);
	double lastTime = glfwGetTime();

    float ambient = 0.1;
	float diffuse = 1.0;
	float specular = 0.8;


    glm::vec3 light_pos = glm::vec3(1.0, 2.0, 1.5);
	glm::vec3 light_direction = glm::vec3(-0.2f, -1.0f, -0.3f);

    // Position du feu (modifiez cette variable pour déplacer la source du feu/fumée)
    glm::vec3 firePosition = glm::vec3(0.0, 0.2, 0.0);
    // Exemple de déplacement :
    // glm::vec3 firePosition = glm::vec3(-1.5, 0.0, 2.0); // position décalée dans la scène
	shader.use();
	shader.setFloat("shininess", 256.0f);
	shader.setFloat("light.ambient_strength", ambient);
	shader.setFloat("light.diffuse_strength", diffuse);
	shader.setFloat("light.specular_strength", specular);
	shader.setFloat("light.constant", 1.0);
	shader.setFloat("light.linear", 0.14);
	shader.setFloat("light.quadratic", 0.07);
	shader.setFloat("sunIntensity", 1.0f);
	
    particleSystem.initParticleSystem();

    // glm::vec3 light_pos = glm::vec3(0.0, 10.0, 0.0);
	// glm::vec3 light_direction = glm::vec3(-0.2f, -1.0f, -0.3f);
	//Rendering
	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		glfwGetFramebufferSize(window, &window_width, &window_height);
		view = camera.GetViewMatrix();
		perspective = camera.GetProjectionMatrix(45.0, (float)window_width / (float)window_height, 0.01, 100.0);
		cameraRight = camera.GetCameraRight();
		cameraUp = camera.GetCameraUp();
		cameraPosition = camera.GetCameraPosition();
		glfwPollEvents();
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader2.use();
        shader2.setVector3f("light.light_pos", light_pos);
		shader2.setFloat("now", 0.1 * glfwGetTime());
        shader2.setMatrix4("M", modelSun);
        shader2.setMatrix4("itM", glm::transpose(glm::inverse(modelSun)));
		shader2.setMatrix4("V", view);
		shader2.setMatrix4("P", perspective);
        shader2.setVector3f("u_view_pos", camera.Position);
		shader2.setVector3f("u_light_direction", light_direction);

		shader2.setInteger("ourTexture", 0);	
		shader2.setFloat("intensity", 1.0f);
		glDepthFunc(GL_LEQUAL);
		sun.drawText();
		sun.draw();

        /*===================================================================*/
        particleSystem.drawParticles(view, perspective, cameraRight, cameraUp);
		// double currentTime = glfwGetTime();
		// double delta = currentTime - lastTime;
		// lastTime = currentTime;
		// int newParticle = delta*1000.0f;
		// if (newParticle > (int)(0.032f * 1000.0)) newParticle = (int)(0.032f * 1000.0);

        // addParticles(newParticle, firePosition);
        // int particleCount = 0;
        // simulateParticles(particleCount,delta, g_particule_position_size_data, g_particule_color_data);
		// sortParticles();
		
		// glBindVertexArray(VAO);
		// glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
		// glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GL_FLOAT), NULL, GL_STREAM_DRAW);;
		// glBufferSubData(GL_ARRAY_BUFFER, 0, particleCount * sizeof(GLfloat) * 4, g_particule_position_size_data);

		// glBindBuffer(GL_ARRAY_BUFFER, VBO_color);
		// glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);;
		// glBufferSubData(GL_ARRAY_BUFFER, 0, particleCount * sizeof(GLfloat) * 4, g_particule_color_data);


		// shader.use();

		// shader.setMatrix4("V", view);
		// shader.setMatrix4("P", perspective);

		// shader.setVector3f("cameraRight", cameraRight);
		// shader.setVector3f("cameraUp", cameraUp);

		// glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, particleCount);

		// fps(currentTime);
		glfwSwapBuffers(window);
	}

	// //clean up ressource
	// delete[] g_particule_color_data, g_particule_position_size_data;

	// //clean up VBO and VAO
	// glDeleteBuffers(1, &VBO_vertex);
	// glDeleteBuffers(1, &VBO_position);
	// glDeleteBuffers(1, &VBO_color);

	// glDeleteVertexArrays(1, &VAO);

    particleSystem.cleanUp();

	glDeleteProgram(shader.ID);


	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
	//3. Use the cameras class to change the parameters of the camera
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(LEFT, 0.1);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(RIGHT, 0.1);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(FORWARD, 0.1);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(BACKWARD, 0.1);

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(1, 0.0, 1);
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(-1, 0.0, 1);

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(0.0, 1.0, 1);
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(0.0, -1.0, 1);


}
