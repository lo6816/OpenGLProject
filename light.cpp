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

Camera camera(glm::vec3(0.0, 0.0, 0.0));

// — paramètres ----------------------------------------------------------------

// Lumière ponctuelle
glm::vec3 lightShadow(2.0f, 6.0f, 2.0f);

// Plan récepteur  :  n·x + d = 0  → plan (0,1,0) à y = 0
const glm::vec3 planeN(0.0f, 1.0f, 0.0f);
float planeD = 0.0f;

// -----------------------------------------------------------------------------
// Calcule la matrice de projection d’ombre (Eq. 7.4 du PDF)
glm::mat4 shadowMatrix(const glm::vec3& L,
	const glm::vec3& n,
	float d)
{
float nDotL = glm::dot(n, L) + d;        // n·l + d
glm::mat4 M(0.0f);

M[0][0] = nDotL - L.x * n.x;  M[0][1] = -L.x * n.y;      M[0][2] = -L.x * n.z;      M[0][3] = -L.x * d;
M[1][0] = -L.y * n.x;         M[1][1] = nDotL - L.y*n.y; M[1][2] = -L.y * n.z;      M[1][3] = -L.y * d;
M[2][0] = -L.z * n.x;         M[2][1] = -L.z * n.y;      M[2][2] = nDotL - L.z*n.z; M[2][3] = -L.z * d;
M[3][0] = -n.x;               M[3][1] = -n.y;            M[3][2] = -n.z;            M[3][3] =  nDotL;

return M;
}

glm::mat4 shadowMatrix2(const glm::vec3& L,
	const glm::vec3& n,
	float d)
{
float nDotL = glm::dot(n, L) + d;        // n·l + d
glm::mat4 M(0.0f);

M[0][0] = L.y;    			  M[0][1] = -L.x      ;      M[0][2] = 0;       		M[0][3] = 0;
M[1][0] = 0;         		  M[1][1] = 0; 				 M[1][2] = 0;      			M[1][3] = 0;
M[2][0] = 0;         		  M[2][1] = -L.z;      		 M[2][2] = -L.y; 			M[2][3] = 0;
M[3][0] = 0;                  M[3][1] = -1;              M[3][2] = 0;            	M[3][3] = L.y;

return M;
}

glm::mat4 shadowMatrix3(const glm::vec3& L,
	const glm::vec3& n,
	float d, float yprime)
{
float nDotL = glm::dot(n, L) + d;        // n·l + d
glm::mat4 M(0.0f);

M[0][0] = L.y + yprime;    	  M[0][1] = -L.x      ;      M[0][2] = 0;       		M[0][3] = -yprime*L.x;
M[1][0] = 0;         		  M[1][1] = 0; 				 M[1][2] = 0;      			M[1][3] = 0;
M[2][0] = 0;         		  M[2][1] = -L.z;      		 M[2][2] = -L.y+yprime; 	M[2][3] = -yprime*L.z;
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

	const std::string sourceV = "#version 330 core\n"
		"in vec3 position; \n"
		"in vec2 tex_coords; \n"
		"in vec3 normal; \n"

		"out vec3 v_frag_coord; \n"
		"out vec3 v_normal; \n"

		"uniform mat4 M; \n"
		"uniform mat4 itM; \n"
		"uniform mat4 V; \n"
		"uniform mat4 P; \n"
		"uniform mat4 S; \n"


		" void main(){ \n"
		"vec4 frag_coord = M*vec4(position, 1.0); \n"
		// "vec4 frag_coord = S*M*vec4(position, 1.0); \n"
		"gl_Position = P*V*frag_coord; \n"
		//4. transfomr correctly the normals
		"v_normal = vec3(itM * vec4(normal, 1.0)); \n"
		"v_frag_coord = frag_coord.xyz; \n"
		"\n" //same component in every direction
		"}\n";

	const std::string sourceF = "#version 330 core\n"
		"out vec4 FragColor;\n"
		"precision mediump float; \n"

		"in vec3 v_frag_coord; \n"
		"in vec3 v_normal; \n"

		"uniform vec3 u_view_pos; \n"
		"uniform vec3 u_light_direction; \n"

		//In GLSL you can use structures to better organize your code
		//light
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
		"uniform vec3 materialColour; \n"
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
			"float orbitAngle = atan(light.light_pos.y, light.light_pos.x);\n"
			"float dynamicIntensity = sunIntensity * clamp(0.5 + 0.5 * cos(orbitAngle), 0.0, 1.0);\n"
			"float res = light.ambient_strength + dynamicIntensity * attenuation * (diffuse); \n"
			"return res; \n"
		"}\n"

		"void main() { \n"
		// "Vec3 light_direction = (-0.2f, -1.0f, -0.3f); \n"

		"vec3 N = normalize(v_normal);\n"
		"vec3 V = normalize(u_view_pos - v_frag_coord); \n"

		"float result = CalcDirectionalLight(light, N, V, normalize(-u_light_direction)); \n"
		"result += CalcPointLight(light, N, V, normalize(light.light_pos - v_frag_coord)); \n"

		"FragColor = vec4(materialColour * vec3(result), 1.0); \n"
		"} \n";

	const std::string shadowV = "#version 330 core\n"
		"layout(location=0) in vec3 position; \n"
		"out vec4 frag_coord; \n"
		"uniform mat4 MVP; \n"
		"void main(){ \n"
		"frag_coord = MVP*vec4(position, 1.0); \n"
		"gl_Position = frag_coord; \n"
		"}\n";
	
	const std::string shadowF = "#version 330 core\n"
		"out vec4 FragColor;\n"
		"precision mediump float; \n"
		"in vec4 frag_coord; \n"
		"uniform vec3 color; \n"
		"void main(){ \n"
		"FragColor = vec4(color, 1.0); \n"
		"}\n";

	Shader shader(sourceV, sourceF);
	Shader shadowShader(shadowV, shadowF);

	char shaderSunV[128] = PATH_TO_SHADERS "/sunVert.txt";
	char shaderSunF[128] = PATH_TO_SHADERS "/sunFrag.txt";
	Shader shader2(shaderSunV, shaderSunF);


	

	// char path[] = PATH_TO_OBJECTS "/sphere_smooth.obj";
	// char path[] = PATH_TO_OBJECTS "/moai5.obj";
	char path[] = PATH_TO_OBJECTS "/cube.obj";

	Object sphere1(path);
	sphere1.makeObject(shader, false);

	Object sphere2(path);
	sphere2.makeObject(shadowShader, false);

	char pathCub[] = PATH_TO_OBJECTS "/sphere_smooth.obj"; 
	Object sun(pathCub);
	sun.makeObject(shader2, true);
	char file3[128] = "./../textures/2k_sun.jpg";
	sun.createText(file3);

	


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
	model = glm::translate(model, glm::vec3(0.0, 0.0, 2.0));
	// model = glm::translate(model, glm::vec3(1.0, 1.0, -2.0));
	model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
	glm::mat4 inverseModel = glm::transpose( glm::inverse(model));

	glm::mat4 modelSun = glm::translate(modelSun, glm::vec3(0.0, 0.0, 0.0));
	modelSun = glm::scale(modelSun, glm::vec3(0.5, 0.5, 0.5));
	glm::mat4 inverseModelSun= glm::transpose(glm::inverse(modelSun));



	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 perspective = camera.GetProjectionMatrix();

	float ambient = 0.1;
	float diffuse = 1.0;
	float specular = 0.8;

	glm::vec3 materialColour = glm::vec3(0.5f,0.6,0.8);

	glm::vec3 light_pos = glm::vec3(1.0, 2.0, 1.5);
	// glm::vec3 light_pos = glm::vec3(0.0, 4.0, 0.0);
	glm::vec3 light_direction = glm::vec3(-0.2f, -1.0f, -0.3f);

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

			// Un grand carré centré sur l'origine dans le plan XZ (y = 0)
		float floorVertices[] = {
			// positions            // texture coords
		-10.0f, 0.0f, -10.0f,     0.0f,  0.0f,
			10.0f, 0.0f, -10.0f,     1.0f,  0.0f,
			10.0f, 0.0f,  10.0f,     1.0f,  1.0f,
		-10.0f, 0.0f,  10.0f,     0.0f,  1.0f
		};

		unsigned int floorIndices[] = {
			0, 1, 2,
			2, 3, 0
		};
	


	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		view = camera.GetViewMatrix();
		glfwPollEvents();
		double now = glfwGetTime();
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// float damptime = -0.04;
		// float distSun = -50.0f;
		// auto delta = glm::vec3(distSun * std::cos(damptime * now), distSun * std::sin(damptime *  now), -80.0f);
		float damptime = 1.0f;
		// float damptime = 0.2;
		auto delta = glm::vec3(10.0f * std::cos(damptime * now), 10.0f * std::sin(damptime *  now), 0.0f);

		

		// planeD = -1.0f;
		// planeD = -1.5f;
		// glm::mat4 S  = shadowMatrix(delta, planeN, planeD);
		// glm::mat4 S  = shadowMatrix(glm::vec3(0.0,5.0,0.0), planeN, planeD);
		// glm::vec3 poslightshad = glm::vec3(std::cos(damptime * now),5.0, 3.0);
		glm::vec3 poslightshad = glm::vec3(std::cos(damptime * now),5.0, std::sin(damptime * now));
		// planeD = -1.0f;
		// glm::mat4 S  = shadowMatrix3(poslightshad, planeN, planeD, -planeD);
		// glm::mat4 S  = shadowMatrix(poslightshad, planeN, -1);
		glm::mat4 S  = shadowMatrix(poslightshad, planeN, 0.0f);

		// shader.setVector3f("light.light_pos", delta);
		shader.setVector3f("light.light_pos", poslightshad);
		shader.setFloat("now", 0.1 * (now));


		shader.use();

		shader.setMatrix4("M", model);
		shader.setMatrix4("itM", inverseModel);


		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		shader.setMatrix4("S", S);
		shader.setVector3f("u_view_pos", camera.Position);
		shader.setVector3f("u_light_direction", light_direction);

		
		

		sphere1.draw();

		shadowShader.use();
		shadowShader.setMatrix4("MVP", perspective*view* S * model);
		shadowShader.setVector3f("color", glm::vec3(0.0, 0.0, 0.0));
		sphere2.draw();



		shader2.use();
		modelSun = glm::mat4(1.0f);
		modelSun = glm::translate(modelSun, poslightshad);
		// modelCub = glm::scale(modelCub, glm::vec3(0.5f));
		// glm::mat4 inverseModelCub = glm::transpose(glm::inverse(modelCub));
		// shader2.setMatrix4("M", modelCub);
		// shader2.setMatrix4("itM", inverseModelCub);

		shader2.setMatrix4("M", modelSun);
		shader2.setMatrix4("V", view);
		shader2.setMatrix4("P", perspective);

		shader2.setInteger("ourTexture", 0);	
		float intensityMap = fmin(fmax(std::sin(-damptime * now), 0.0f) + 0.1f, 1.0f);
		shader2.setFloat("intensity", 1.0f + 10*intensityMap);
		sun.drawText();
		sun.draw();


		
		// glDisable(GL_DEPTH_TEST);


		// fps(now);
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


