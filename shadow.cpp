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


const int width = 500;
const int height = 500;


GLuint compileShader(std::string shaderCode, GLenum shaderType);
GLuint compileProgram(GLuint vertexShader, GLuint fragmentShader);
void processInput(GLFWwindow* window);

#ifndef NDEBUG
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

Camera camera(glm::vec3(0.0, 0.0, 0.1));

// — paramètres ----------------------------------------------------------------

// Plan récepteur  :  n·x + d = 0  → plan (0,1,0) à y = 0
const glm::vec3 planeN(0.0f, 1.0f, 0.0f);
const float     planeD = 0.0f;

// -----------------------------------------------------------------------------
// Calcule la matrice de projection d’ombre (Eq. 7.4 du PDF)
glm::mat4 shadowMatrix(const glm::vec3& L,const glm::vec3& n,float d)
{
float nDotL = glm::dot(n, L) + d;        // n·l + d
glm::mat4 M(0.0f);
std::cout << "Parameters: " << std::endl;
std::cout << "L: " << L.x << " " << L.y << " " << L.z << std::endl;
std::cout << "n: " << n.x << " " << n.y << " " << n.z << std::endl;
std::cout << "d: " << d << std::endl;
std::cout << "End of parameters" << std::endl;

M[0][0] = nDotL - L.x * n.x;  M[0][1] = -L.x * n.y;      M[0][2] = -L.x * n.z;      M[0][3] = -L.x * d;
M[1][0] = -L.y * n.x;         M[1][1] = nDotL - L.y*n.y; M[1][2] = -L.y * n.z;      M[1][3] = -L.y * d;
M[2][0] = -L.z * n.x;         M[2][1] = -L.z * n.y;      M[2][2] = nDotL - L.z*n.z; M[2][3] = -L.z * d;
M[3][0] = -n.x;               M[3][1] = -n.y;            M[3][2] = -n.z;            M[3][3] =  nDotL;

return M;
}

glm::mat4 shadowMatrix2(const glm::vec3& L,const glm::vec3& n,float d)
{
// float nDotL = glm::dot(n, L) + d;        // n·l + d
glm::mat4 M(0.0f);

M[0][0] = L.y;
    M[2][2] = L.y;
    M[3][0] = -L.x;
    M[3][1] = -L.y;
    M[3][2] = -L.z;
    M[3][3] = L.y;

return M;
}

glm::mat4 shadowMatrix3(const glm::vec3& L,const glm::vec3& n,float d)
{
float nDotL = glm::dot(n, L) + d;        // n·l + d
glm::mat4 M(0.0f);
// std::cout << "Parameters: " << std::endl;
// std::cout << "L: " << L.x << " " << L.y << " " << L.z << std::endl;
// std::cout << "n: " << n.x << " " << n.y << " " << n.z << std::endl;
// std::cout << "d: " << d << std::endl;
// std::cout << "End of parameters" << std::endl;

M[0][0] = nDotL - L.x * n.x;  M[1][0] = -L.x * n.y;      M[2][0] = -L.x * n.z;      M[3][0] = -L.x * d;
M[0][1] = -L.y * n.x;         M[1][1] = nDotL - L.y*n.y; M[2][1] = -L.y * n.z;      M[3][1] = -L.y * d;
M[0][2] = -L.z * n.x;         M[1][2] = -L.z * n.y;      M[2][2] = nDotL - L.z*n.z; M[3][2] = -L.z * d;
M[0][3] = -n.x;               M[1][3] = -n.y;            M[2][3] = -n.z;            M[3][3] =  nDotL;

return M;
}

glm::mat4 shadowMatrix4(const glm::vec3& L,const glm::vec3& n,float d)
{
// float nDotL = glm::dot(n, L) + d;        // n·l + d
glm::mat4 M(0.0f);

M[0][0] = L.y;    			  M[1][0] = -L.x      ;      M[2][0] = 0;       		M[3][0] = 0;
M[0][1] = 0;         		  M[1][1] = 0; 				 M[2][1] = 0;      			M[3][1] = 0;
M[0][2] = 0;         		  M[1][2] = -L.z;      		 M[2][2] = L.y; 			M[3][2] = 0;
M[0][3] = 0;                  M[1][3] = -1;              M[2][3] = 0;            	M[3][3] = L.y;

return M;
}

// glm::mat4 shadowMatrix4(const glm::vec3& L,const glm::vec3& n,float d)
// {
// // float nDotL = glm::dot(n, L) + d;        // n·l + d
// glm::mat4 M(0.0f);

// M[0][0] = L.y;    			  M[1][0] = -L.x      ;      M[2][0] = 0;       		M[3][0] = 0;
// M[0][1] = 0;         		  M[1][1] = 0; 				 M[2][1] = 0;      			M[3][1] = 0;
// M[0][2] = 0;         		  M[1][2] = -L.z;      		 M[2][2] = L.y; 			M[3][2] = 0;
// M[0][3] = 0;                  M[1][2] = -1;              M[2][3] = 0;            	M[3][3] = L.y;

// return M;
// }

glm::mat4 shadowMatrix5(const glm::vec3& L,const glm::vec3& n,float d)
{
// float nDotL = glm::dot(n, L) + d;        // n·l + d
glm::mat4 M(0.0f);

M[0][0] = L.y;    			  M[0][1] = -L.x      ;      M[0][2] = 0;       		M[0][3] = 0;
M[1][0] = 0;         		  M[1][1] = 0; 				 M[1][2] = 0;      			M[1][3] = 0;
M[2][0] = 0;         		  M[2][1] = -L.z;      		 M[2][2] = L.y; 			M[2][3] = 0;
M[3][0] = 0;                  M[3][1] = -1;              M[3][2] = 0;            	M[3][3] = L.y;

return M;
}
// -----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	std::cout << "Welcome to exercice 7: " << std::endl;
	std::cout << "Complete light equation and attenuation factor\n"
		"Implement the complete light equation.\n"
		"Make the light move with time closer and further of the model.\n"
		"Put the attenuation factor into the calculations\n"
		"\n";


	//Boilerplate
	//Create the OpenGL context 
	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialise GLFW \n");
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
	//create a debug context to help with Debugging
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif


	//Create the window
	GLFWwindow* window = glfwCreateWindow(width, height, "Exercise 07", nullptr, nullptr);
	if (window == NULL)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window\n");
	}

	glfwMakeContextCurrent(window);

	//load openGL function
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		throw std::runtime_error("Failed to initialize GLAD");
	}

	glEnable(GL_DEPTH_TEST);

#ifndef NDEBUG
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

	// const std::string shadowV = "#version 330 core\n"
	// 	"layout(location=0) in vec3 position; \n"
	// 	"out vec4 frag_coord; \n"
	// 	"uniform mat4 MVP; \n"
	// 	"void main(){ \n"
	// 	"frag_coord = MVP*vec4(position, 1.0); \n"
	// 	"gl_Position = frag_coord; \n"
	// 	"}\n";
	
	// const std::string shadowF = "#version 330 core\n"
	// 	"out vec4 FragColor;\n"
	// 	"precision mediump float; \n"
	// 	"in vec4 frag_coord; \n"
	// 	"uniform vec3 color; \n"
	// 	"void main(){ \n"
	// 	"FragColor = vec4(color, 1.0); \n"
	// 	"}\n";

    char pathGroundV[] = PATH_TO_SHADERS "/simpleVert.txt";
    char pathGroundF[] = PATH_TO_SHADERS "/simpleFrag.txt";
    char pathSphereF[] = PATH_TO_SHADERS "/sphFrag.txt";
    char pathSphereV[] = PATH_TO_SHADERS "/sphVert.txt";

	Shader shader(pathSphereV, pathSphereF);
	Shader shadowShader(PATH_TO_SHADERS "/shadow.vert", PATH_TO_SHADERS "/shadow.frag");
    Shader shadGround(pathGroundV, pathGroundF);

	

	char path[] = PATH_TO_OBJECTS "/campfire.obj";
	char path1[] = PATH_TO_OBJECTS "/sphere_smooth.obj";


	Object sphere1(path);
	sphere1.makeObject(shader, false);

	Object sphere2(path);
	sphere2.makeObject(shadowShader, false);

    Object sphere3(path1);
    sphere3.makeObject(shader, false);

    // Initialisation des sommets du triangle
    float triangleV = 100.0f;
    std::vector<Vertex> triangleVertices = {
        {{-triangleV,0.0f,  -triangleV}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}, // Sommet 1
        {{triangleV, 0.0f, -triangleV}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},  // Sommet 2
        {{0.0f,  0.0f, triangleV}, {triangleV, 1.0f}, {0.0f, 0.0f, 1.0f}}   // Sommet 3
    };

    // Création d'un objet
    Object triangle(triangleVertices);
    triangle.makeObject(shadGround, false);

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
			std::cout.flush();
		}
	};


	

	glm::mat4 model = glm::mat4(1.0);
	model = glm::translate(model, glm::vec3(0.0, 1.0, 0.0));
	model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
	glm::mat4 inverseModel = glm::transpose( glm::inverse(model));

    // glm::mat4 modelsun = glm::mat4(1.0);
	// modelsun = glm::translate(modelsun, glm::vec3(0.0, 10.0, 0.0));
	// modelsun = glm::scale(modelsun, glm::vec3(0.1, 0.1, 0.1));
	// glm::mat4 inverseModel = glm::transpose( glm::inverse(modelsun));



	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 perspective = camera.GetProjectionMatrix();

	float ambient = 0.1;
	float diffuse = 1.0;
	float specular = 0.8;

	glm::vec3 materialColour = glm::vec3(0.5f,0.6,0.8);

	// glm::vec3 light_pos = glm::vec3(1.0, 2.0, 1.5);
	// glm::vec3 light_pos = glm::vec3(2.0, 10.0, 2.0);
	// glm::vec3 light_direction = glm::vec3(-0.2f, -1.0f, -0.3f);

	// auto u_time = glGetUniformLocation(shader, "time");

	//Rendering

	shader.use();
	shader.setFloat("shininess", 32.0f);
	shader.setVector3f("materialColour", materialColour);
	shader.setFloat("light.ambient_strength", ambient);
	shader.setFloat("light.diffuse_strength", diffuse);
	shader.setFloat("light.specular_strength", specular);
	shader.setFloat("light.constant", 1.0);
	shader.setFloat("light.linear", 0.14);
	shader.setFloat("light.quadratic", 0.07);
	shader.setFloat("sunIntensity", 1.0f);
	

    glm::mat4 modelsun = glm::mat4(1.0);
	modelsun = glm::translate(modelsun, glm::vec3(0.0, 5.0, 0.0));
	modelsun = glm::scale(modelsun, glm::vec3(0.1, 0.1, 0.1));

    glm::vec3 light_pos = glm::vec3(0.0, 10.0, 0.0);
	glm::vec3 light_direction = glm::vec3(-0.2f, -1.0f, -0.3f);

	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		view = camera.GetViewMatrix();
		glfwPollEvents();
		double now = glfwGetTime();
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// auto delta = light_pos ;//+ glm::vec3(0.0,0.0,2 * std::sin(now));
		// auto delta = light_pos + glm::vec3(0.0,0.0,2 * std::sin(now));
		float damptime = 0.2;
		auto delta = glm::vec3(10.0f * std::cos(damptime * now), 10.0f * std::sin(damptime *  now), 0.0f);
		//std::cout << delta.z <<std::endl;
		shader.setVector3f("light.light_pos", delta);
		shader.setFloat("now", 0.1 * (now));

        shadGround.use();
        shadGround.setMatrix4("V", view);
        shadGround.setMatrix4("P", perspective);
        triangle.draw();

        // glm::vec3 lightProjected = glm::vec3(light_pos.x, 0.0f, light_pos.z); // Projeter la lumière sur le plan y = 0
        // glm::mat4 S = shadowMatrix(lightProjected, planeN, planeD);
        float amplitude = 20.0f; // Amplitude du mouvement
        float frequency = 0.5f; // Fréquence du mouvement
        float offset = amplitude * std::sin(frequency * now); // Déplacement symétrique
        float offset2 = amplitude * std::cos(frequency * now); // Déplacement symétrique
        light_pos = glm::vec3(offset, 10.0f, offset2);
        model = glm::translate(model, glm::vec3(0.0, 1.0, 0.0));
        // model = glm::translate(glm::mat4(1.0), glm::vec3(amplitude * std::cos(frequency * now), 2.0f, amplitude * std::sin(frequency * now)));
        model = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0f, 0.0));
        model = glm::rotate(model, (float)now, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 S  = shadowMatrix3(light_pos, planeN, 0.0);

		shadowShader.use();
		shadowShader.setMatrix4("MVP", perspective*view* S * model);
		shadowShader.setVector3f("color", glm::vec3(0.0, 0.0, 0.0));
        glDisable(GL_DEPTH_TEST);
		sphere2.draw();
        glEnable(GL_DEPTH_TEST);


		shader.use();
        

        // Mettre à jour la matrice modèle
        glm::mat4 dynamicModel = glm::rotate(modelsun, (float) (0.0001*glfwGetTime()),glm::vec3(0.0f, 1.0f, 0.0f));
        // glm::mat4 model = glm::rotate(model, (float) (0.0001*glfwGetTime()),glm::vec3(0.0f, 1.0f, 0.0f));
        

        // Passer la matrice mise à jour au shader
        shader.setMatrix4("M", model);
        shader.setMatrix4("itM", glm::transpose(glm::inverse(model)));


		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		// shader.setMatrix4("S", S);
		shader.setVector3f("u_view_pos", camera.Position);
		shader.setVector3f("u_light_direction", light_direction);
		sphere1.draw();

        shader.use();
        shader.setMatrix4("M", dynamicModel);
        shader.setMatrix4("itM", glm::transpose(glm::inverse(dynamicModel)));
		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		// shader.setMatrix4("S", S);
		shader.setVector3f("u_view_pos", camera.Position);
		shader.setVector3f("u_light_direction", light_direction);
		sphere3.draw();
        // // Passer la matrice mise à jour au shader
        // shader.setMatrix4("M", dynamicModel);
        // shader.setMatrix4("itM", glm::transpose(glm::inverse(dynamicModel)));

        

        

        // shadowShader.use();
        // S  = shadowMatrix(light_pos, planeN, 0.0);
        // dynamicModel = glm::translate(model, glm::vec3(offset+0.1, 0.0f, offset2+0.1));
		// shadowShader.setMatrix4("MVP", perspective*view* S * dynamicModel);
		// shadowShader.setVector3f("color", glm::vec3(0.0, 1.0, 0.0));
		// sphere2.draw();


        // S  = shadowMatrix2(light_pos, planeN, 0.0);
		// shader.use();
		// shader.setMatrix4("M", model);
		// shader.setMatrix4("itM", inverseModel);
		// shader.setMatrix4("V", view);
		// shader.setMatrix4("P", perspective);
		// // shader.setMatrix4("S", S);
		// shader.setVector3f("u_view_pos", camera.Position);
		// shader.setVector3f("u_light_direction", light_direction);
		// glDisable(GL_DEPTH_TEST);


		fps(now);
		glfwSwapBuffers(window);
	}

	//clean up ressource
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}


void processInput(GLFWwindow* window) {
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
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(UP, 0.1);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(DOWN, 0.1);

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(1, 0.0,1);
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(-1, 0.0,1);

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(0.0, 1.0, 1);
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(0.0, -1.0, 1);


}
