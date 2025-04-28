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

Camera camera(glm::vec3(0.0, 0.0, 0.2));


int main(int argc, char* argv[])
{
	std::cout << "Welcome to exercice GNOMGNUM: " << std::endl;
	std::cout << "Object reader\n"
		"Make an object reader and load one of the .obj file\n"
		"You can also make your own object in Blender then save it as a .obj \n"
		"You will need to: \n"
		"	- Understand what a Wavefront .obj file contain\n"
		"	- Open the .obj and read the relevant data \n"
		"	- Stock the data in the relevant format  \n"
		"	- Use the relevant buffer (VAO & VBO) to be able to draw your object  \n"
		"We give you the the object.h file in the LAB03/exercises/ to help you start \n";

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
	GLFWwindow* window = glfwCreateWindow(width, height, "Escape the GNOMGNUM", nullptr, nullptr);
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

	const std::string sourceV = "#version 330 core\n"
		"in vec3 position; \n"
		"in vec2 tex_coord; \n"
		"in vec3 normal; \n"

		"out vec4 v_col; \n"
		"out vec2 v_t; \n"

		"out vec3 v_frag_coord; \n"
		"out vec3 v_normal; \n"

		"uniform mat4 itM; \n"
		
		"uniform mat4 M; \n"
		"uniform mat4 V; \n"
		"uniform mat4 P; \n"
		" void main(){ \n"

		"vec4 frag_coord = M*vec4(position, 1.0); \n"
		"gl_Position = P*M*V*frag_coord;\n"

		"v_col = vec4(normal*0.5 + 0.5, 1.0);\n"
		"v_t = tex_coord; \n"

		"v_normal = vec3(itM * vec4(normal, 1.0)); \n"
		"v_frag_coord = frag_coord.xyz; \n"
		"}\n"; 
	const std::string sourceF = "#version 330 core\n"
		"out vec4 FragColor;"
		"precision mediump float; \n"
		"in vec4 v_col; \n"
		"in vec2 v_t; \n"

		"in vec3 v_frag_coord; \n"
		"in vec3 v_normal; \n"

		"uniform sampler2D ourTexture; \n"

		"uniform vec3 u_view_pos; \n"
		"uniform vec3 u_light_direction; \n"

		"struct Light{\n" 
		"vec3 light_pos; \n"
		"float ambient_strength; \n"
		"float diffuse_strength; \n"
		"float specular_strength; \n"
		//attenuation factor
		"float constant;\n"
		"float linear;\n"
		"float quadratic;\n"
		"};\n"
		"uniform Light light;"

		"uniform float shininess; \n"
		"uniform float now;\n"
		"uniform float sunIntensity;\n"


		"float specularCalculation(vec3 N, vec3 L, vec3 V ){ \n"
			"vec3 R = reflect (-L,N);  \n " //reflect (-L,N) is  equivalent to //max (2 * dot(N,L) * N - L , 0.0) ;
			"float cosTheta = dot(R , V); \n"
			"float spec = pow(max(cosTheta,0.0), 32.0); \n"
			"return light.specular_strength * spec;\n"
		"}\n"

		"float CalcPointLight(Light light, vec3 N, vec3 V, vec3 L)\n"
			"{\n"
			"float specular = specularCalculation( N, L, V); \n"
			"float diffuse = light.diffuse_strength * max(dot(N,L),0.0);\n"
			"float distance = length(light.light_pos - v_frag_coord);"
			"float attenuation = 1 / (light.constant + light.linear * distance + light.quadratic * distance * distance);"
			"float res = light.ambient_strength + attenuation * (diffuse + specular); \n"
			"return res; \n"
		"}\n"

		"float CalcDirectionalLight(Light light, vec3 N, vec3 V, vec3 L)\n"
			"{\n"
			// "float specular = specularCalculation( N, L, V); \n"
			"float diffuse = light.diffuse_strength * max(dot(N,L),0.0);\n"
			"float attenuation = 1; \n"
			"float orbitAngle = atan(light.light_pos.x, light.light_pos.y);\n"
			"float dynamicIntensity = sunIntensity * clamp(0.5 + 0.5 * cos(orbitAngle), 0.0, 1.0);\n"
			"float res = light.ambient_strength + dynamicIntensity * attenuation * (diffuse); \n"
			"return res; \n"
		"}\n"

		"void main() { \n"
		"vec3 N = normalize(v_normal);\n"
		"vec3 V = normalize(u_view_pos - v_frag_coord); \n"
		"float result = CalcDirectionalLight(light, N, V, normalize(-u_light_direction)); \n"
		"result += CalcPointLight(light, N, V, normalize(light.light_pos - v_frag_coord)); \n"

		// Récupération de la couleur de texture
		"vec3 texColor = texture(ourTexture, v_t).rgb; \n"

		// "FragColor = v_col*(1.0-v_t.y); \n" //for the sake of the exercise, we will color the result using texture and color coordinated present in the file, feel free to change the shaders as you prefer
		// "FragColor = texture(ourTexture, v_t); \n"
		"FragColor = vec4(texColor * result, 1.0); \n"
		"} \n";

	
	Shader shader(sourceV, sourceF);

	//Here for this exercise session, we use a macro to indicate the folder containing the .obj files
	//Check the file CMakeLists.txt in the LAB03 folder to see where it is specified
	//Start the exercise with the cube object then try to test your implementation with more complex one
	char path[] = PATH_TO_OBJECTS "/moai5.obj";
	
	//Model is the class present in the object.h file
	//Change the constructor to read the file
	Object cube(path);
	//Modify the makeObject function to create and setup your VBO and VAO
	cube.makeObject(shader);

	char file[128] = "./../textures/uii.png";
	cube.createText(file);


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


	glm::mat4 model = glm::mat4(1.0);
	model = glm::translate(model, glm::vec3(0.0, 0.0, 0.0));
	// model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
	glm::mat4 inverseModel = glm::transpose( glm::inverse(model));


	char path2[] = PATH_TO_OBJECTS "/plage.obj";
	Object secondObj(path2);
	secondObj.makeObject(shader);
	char file2[128] = "./../textures/plage.png";
	secondObj.createText(file2);	
	glm::mat4 secondModel = glm::translate(glm::mat4(1.0f),glm::vec3(0.0, 0.0, 0.0));
	glm::mat4 inverseModel2 = glm::transpose( glm::inverse(secondModel));


	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 perspective = camera.GetProjectionMatrix();

	glm::vec3 light_pos = glm::vec3(1.0, 2.0, 1.5);
	glm::vec3 light_direction = glm::vec3(-0.2f, -1.0f, -0.3f);


	glfwSwapInterval(1);
	//Rendering

	float ambient = 0.1;
	float diffuse = 1.0;
	float specular = 0.8;

	shader.use();
	shader.setFloat("shininess", 32.0f);
	shader.setFloat("light.ambient_strength", ambient);
	shader.setFloat("light.diffuse_strength", diffuse);
	shader.setFloat("light.specular_strength", specular);
	shader.setFloat("light.constant", 1.0);
	shader.setFloat("light.linear", 0.14);
	shader.setFloat("light.quadratic", 0.07);
	shader.setFloat("sunIntensity", 1.0f);

	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		view = camera.GetViewMatrix();
		glfwPollEvents();
		double now = glfwGetTime();
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		//Use the shader Class to send the uniform
		shader.use();

		shader.setMatrix4("M", model);
		shader.setMatrix4("itM", inverseModel);
		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);

		shader.setVector3f("u_view_pos", camera.Position);
		shader.setVector3f("u_light_direction", light_direction);

		float damptime = 0.2;
		auto delta = glm::vec3(10.0f * std::cos(damptime * now), 10.0f * std::sin(damptime *  now), 0.0f);
		//std::cout << delta.z <<std::endl;
		shader.setVector3f("light.light_pos", delta);
		shader.setFloat("now", 0.1 * (now));


		shader.setInteger("ourTexture", 0);		
		cube.drawText();
		//you will have to change the implementation of this function
		cube.draw();

		// glm::mat4 secondModel = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		shader.setMatrix4("M", secondModel);
		shader.setMatrix4("itM", inverseModel2);
		secondObj.drawText();
		secondObj.draw();
		
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


