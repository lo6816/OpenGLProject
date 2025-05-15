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
#include "./utils/picking.h"


const int width = 500;
const int height = 500;


GLuint compileShader(std::string shaderCode, GLenum shaderType);
GLuint compileProgram(GLuint vertexShader, GLuint fragmentShader);
void processInput(GLFWwindow* window);
static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

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
// Picking buffer for object selection
PickingTexture gPickingTexture;
// ------------------------------------------------------------
// Selection state (for temporary highlight on picked objects)
int   gSelectedObjectID    = 0;
double gSelectionTimestamp = -10.0;               // time of last pick
const glm::vec3 gHighlightColour(1.0f, 0.2f, 0.2f); // flashy red-orange
// ------------------------------------------------------------

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
	// Register mouse button callback for picking
	glfwSetMouseButtonCallback(window, MouseButtonCallback);

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

	// Initialise picking FBO
	if (!gPickingTexture.Init(width, height)) {
		std::cerr << "Failed to initialise picking FBO\n";
		return -1;
	}


	const std::string sourceV = "#version 330 core\n"
	    "in  vec3 position;      \n"
	    "in  vec2 tex_coord;    \n"
	    "in  vec3 normal;        \n"
	    "\n"
	    "out vec3 v_frag_coord;  \n"
	    "out vec3 v_normal;      \n"
	    "out vec2 v_t;           \n"
	    "\n"
	    "uniform mat4 M;         \n"
	    "uniform mat4 itM;       \n"
	    "uniform mat4 V;         \n"
	    "uniform mat4 P;         \n"
	    "\n"
	    "void main(){                                       \n"
	    "    vec4 frag_coord = M * vec4(position, 1.0);     \n"
	    "    gl_Position  = P * V * frag_coord;             \n"
	    "    v_normal     = vec3(itM * vec4(normal, 1.0));  \n"
	    "    v_frag_coord = frag_coord.xyz;                 \n"
	    "    v_t          = tex_coord;                     \n"
	    "}\n";

	const std::string sourceF = "#version 330 core\n"
	    "out vec4 FragColor;                 \n"
	    "precision mediump float;            \n"
	    "\n"
	    "in vec3 v_frag_coord;               \n"
	    "in vec3 v_normal;                   \n"
	    "in vec2 v_t;                        \n"
	    "\n"
	    "uniform vec3  u_view_pos;           \n"
	    "uniform vec3  u_light_direction;    \n"
	    "uniform sampler2D ourTexture;       \n"
	    "\n"
	    "struct Light{                       \n"
	    "  vec3  light_pos;                  \n"
	    "  float ambient_strength;           \n"
	    "  float diffuse_strength;           \n"
	    "  float specular_strength;          \n"
	    "  float constant;                   \n"
	    "  float linear;                     \n"
	    "  float quadratic;                  \n"
	    "};                                  \n"
	    "uniform Light light;                \n"
	    "\n"
	    "uniform float shininess;            \n"
	    "uniform float sunIntensity;         \n"
		"uniform vec3  materialColour;        \n"
	    "\n"
	    "float specularCalculation(vec3 N, vec3 L, vec3 V){\n"
	    "  vec3 R = reflect(-L, N);                          \n"
	    "  float cosTheta = dot(R, V);                      \n"
	    "  return light.specular_strength *                 \n"
	    "         pow(max(cosTheta,0.0), shininess);        \n"
	    "}                                                  \n"
	    "\n"
	    "float CalcPointLight(Light Lgt, vec3 N, vec3 V){   \n"
	    "  vec3  L  = normalize(Lgt.light_pos - v_frag_coord);\n"
	    "  float diff = Lgt.diffuse_strength * max(dot(N,L),0.0);\n"
	    "  float spec = specularCalculation(N,L,V);          \n"
	    "  float dist = length(Lgt.light_pos - v_frag_coord);\n"
	    "  float atten = 1.0 / (Lgt.constant + Lgt.linear*dist + Lgt.quadratic*dist*dist);\n"
	    "  return Lgt.ambient_strength + atten*(diff+spec);  \n"
	    "}                                                  \n"
	    "\n"
	    "float CalcDirectionalLight(Light Lgt, vec3 N, vec3 V){\n"
	    "  vec3  L = normalize(-u_light_direction);          \n"
	    "  float diff = Lgt.diffuse_strength * max(dot(N,L),0.0);\n"
	    "  float orbit = atan(Lgt.light_pos.x, Lgt.light_pos.y);\n"
	    "  float dynInt = sunIntensity * clamp(0.5+0.5*cos(orbit),0.0,1.0);\n"
	    "  return Lgt.ambient_strength + dynInt * diff;      \n"
	    "}                                                  \n"
	    "\n"
	    "void main(){                                       \n"
	    "  vec3 N = normalize(v_normal);                    \n"
	    "  vec3 V = normalize(u_view_pos - v_frag_coord);   \n"
	    "  float lighting = CalcDirectionalLight(light,N,V) +\n"
	    "                   CalcPointLight     (light,N,V);  \n"
		"vec3  texColor = texture(ourTexture, v_t).rgb;"
		"vec3  baseColor = texColor * materialColour;\n"
		"FragColor = vec4(baseColor * lighting, 1.0);\n"
	    // "  vec3  texColor = texture(ourTexture, v_t).rgb;   \n"
	    // "  FragColor = vec4(texColor * lighting, 1.0);      \n"
	    "}                                                  \n";
	
	const std::string sourceV2 = "#version 330 core\n"
		"in vec3 position; \n"
		"in vec2 tex_coord; \n"
		"in vec3 normal; \n"
		"out vec4 v_col; \n"
		"out vec2 v_t; \n"
		"uniform mat4 M; \n"
		"uniform mat4 V; \n"
		"uniform mat4 P; \n"
		" void main(){ \n"
		"gl_Position = P*V*M*vec4(position, 1.0);\n"
		"v_col = vec4(normal*0.5 + 0.5, 1.0);\n"
		// "v_col = vec4(normal*0.5 + 0.5, 1.0);\n"
		"v_t = tex_coord; \n"
		"}\n"; 
	const std::string sourceF2 = "#version 330 core\n"
		"out vec4 FragColor;"
		"uniform sampler2D ourTexture; \n"
		"precision mediump float; \n"
		"in vec4 v_col; \n"
		"in vec2 v_t; \n"
		"void main() { \n"
		// "FragColor = v_col*(1.0-v_t.y); \n"
		// "FragColor = v_col*(1.0); \n"
		"FragColor = texture(ourTexture, v_t); \n"
		"} \n";

	Shader shader(sourceV, sourceF);
	Shader shader2(sourceV2, sourceF2);

	// ---------- Picking shader (encodes an object ID in the red channel) ----------
	const std::string pickV = "#version 330 core\n"
		"in vec3 position;\n"
		"uniform mat4 M;\n"
		"uniform mat4 V;\n"
		"uniform mat4 P;\n"
		"void main(){\n"
		"    gl_Position = P * V * M * vec4(position, 1.0);\n"
		"}\n";

	const std::string pickF = "#version 330 core\n"
		"layout(location = 0) out vec3 FragColor;\n"
		"uniform float ObjectID;\n"
		"void main(){\n"
		"    FragColor = vec3(ObjectID, 0.0, 0.0);\n"
		"}\n";

	Shader pickShader(pickV, pickF);

	

	char path[] = PATH_TO_OBJECTS "/sphere_smooth.obj";
	char path2[] = PATH_TO_OBJECTS "/moai5.obj";

	Object sphere1(path2);
	// sphere1.makeObject(shader, false);
	sphere1.makeObject(shader);
	sphere1.createText("./../textures/uii.png");
	sphere1.centerModel();

	// Hitbox invisible : même mesh « sphere_smooth », sans texture
	Object hitSphere(PATH_TO_OBJECTS "/sphere_smooth.obj");
	hitSphere.makeObject(shader, true);   // VAO minimal (pas de textures)

	Object sphere2(path);
	sphere2.makeObject(shader, true);
	sphere2.createText("./../textures/sable.jpg");

	Object sphere3(path);
	sphere3.makeObject(shader, true);
	sphere3.createText("./../textures/sable.jpg");

	char pathCub[] = PATH_TO_OBJECTS "/cube.obj"; 
	Object cub(path);
	cub.makeObject(shader2, true);
	char file[128] = "./../textures/sable.jpg";
	cub.createText(file);

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
	model = glm::translate(model, glm::vec3(0.0, 0.0, -2.0));
	model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
	glm::mat4 inverseModel = glm::transpose( glm::inverse(model));

	// // --- invisible hit-sphere (20 % plus grande) autour de sphere1 ---
	// glm::mat4 modelHit = glm::scale(model, glm::vec3(1.2f));   // 20 % larger, same center


	glm::mat4 model2 = glm::mat4(1.0);
	model2 = glm::translate(model2, glm::vec3(-3.0, 0.0, -2.0));
	model2 = glm::scale(model2, glm::vec3(0.5, 0.5, 0.5));
	glm::mat4 inverseModel2 = glm::transpose( glm::inverse(model2));

	glm::mat4 model3 = glm::mat4(1.0);
	model3 = glm::translate(model3, glm::vec3(3.0, 0.0, -2.0));
	model3 = glm::scale(model3, glm::vec3(0.5, 0.5, 0.5));
	glm::mat4 inverseModel3 = glm::transpose( glm::inverse(model3));

	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 perspective = camera.GetProjectionMatrix();

	float ambient = 0.1;
	float diffuse = 1.0;
	float specular = 0.8;

	glm::vec3 materialColour = glm::vec3(0.5f,0.6,0.8);

	glm::vec3 light_pos = glm::vec3(1.0, 2.0, 1.5);
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
	shader.setInteger("ourTexture", 0);   // sampler → texture unit 0
	shader.setVector3f("materialColour", glm::vec3(1.0f));   // par défaut
	

	glm::mat4 modelCub = glm::mat4(1.0);
	// modelCub = glm::translate(modelCub, glm::vec3(0.0, 0.0, -2.0));
	modelCub = glm::translate(modelCub, glm::vec3(0.0, 2.0, -2.0));
	modelCub = glm::scale(modelCub, glm::vec3(0.5, 0.5, 0.5));
	glm::mat4 inverseModelCub = glm::transpose(glm::inverse(modelCub));

	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		view = camera.GetViewMatrix();
		glfwPollEvents();
		double now = glfwGetTime();

		// =========================================================
		// 1st pass : render IDs into the off‑screen picking buffer
		// =========================================================
		gPickingTexture.EnableWriting();

		/* ---- correctifs ---- */
		glDisable(GL_MULTISAMPLE);         // pas de moyenne d’échantillons
		glDisable(GL_BLEND);               // pas de transparence
		glDisable(GL_CULL_FACE);           // faces avant / arrière toutes conservées
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);   // ID « vide » = 0,0,0
		/* -------------------- */

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		pickShader.use();
		pickShader.setMatrix4("V", view);
		pickShader.setMatrix4("P", perspective);

		// // ---- sphere 1 ----
		// pickShader.setFloat("ObjectID", 1.0f);
		// pickShader.setMatrix4("M", model);
		// sphere1.draw(true);
		// ---- hit-sphere 1 (20 % plus grande) ----
		pickShader.setFloat("ObjectID", 1.0f);
		// --- invisible hit-sphere (20 % plus grande) autour de sphere1 ---
		glm::mat4 modelHit = glm::scale(model, glm::vec3(1.2f));   // 20 % larger, same center
		modelHit = glm::translate(modelHit, glm::vec3(-0.000288667, 0.749567, -4.76613));
		pickShader.setMatrix4("M", modelHit);   // même ID = 1
		hitSphere.draw(true);

		// ---- sphere 2 ----
		pickShader.setFloat("ObjectID", 2.0f);
		pickShader.setMatrix4("M", model2);
		sphere2.draw(true);

		// ---- sphere 3 ----
		pickShader.setFloat("ObjectID", 3.0f);
		pickShader.setMatrix4("M", model3);
		sphere3.draw(true);


		gPickingTexture.DisableWriting();
		// =========================================================
		// End picking pass – back to normal forward rendering
		// =========================================================

		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();

		// ----- handle temporary highlight after picking -----
		double elapsed         = now - gSelectionTimestamp;
		bool   highlightActive = elapsed < 2.0;   // 2-second window


		shader.setMatrix4("M", model);
		shader.setMatrix4("itM", inverseModel);
		shader.setVector3f("materialColour",
			(highlightActive && gSelectedObjectID == 1)
				? gHighlightColour
				: glm::vec3(1.0f));
		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		shader.setVector3f("u_view_pos", camera.Position);
		shader.setVector3f("u_light_direction", light_direction);

		float damptime = 0.2;
		auto delta = glm::vec3(10.0f * std::cos(damptime * now), 10.0f * std::sin(damptime *  now), -5.0f);
		shader.setVector3f("light.light_pos", delta);
		shader.setFloat("now", 0.1 * (now));
		
		sphere1.drawText();
		sphere1.draw();
		// --- draw the hit‑sphere as thin wireframe to visualise its size ---
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		shader.setMatrix4("M",   modelHit);
		shader.setMatrix4("itM", glm::transpose(glm::inverse(modelHit)));
		shader.setVector3f("materialColour", glm::vec3(0.0, 1.0, 0.0));  // bright green
		hitSphere.draw();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		shader.setMatrix4("M", model2);
		shader.setMatrix4("itM", inverseModel2);
		shader.setVector3f("materialColour",
			(highlightActive && gSelectedObjectID == 2)
				? gHighlightColour
				: glm::vec3(1.0f));
		sphere2.draw();

		shader.setMatrix4("M", model3);
		shader.setMatrix4("itM", inverseModel3);
		shader.setVector3f("materialColour",
			(highlightActive && gSelectedObjectID == 3)
				? gHighlightColour
				: glm::vec3(1.0f));
		sphere3.draw();

		shader2.use();
		modelCub = glm::mat4(1.0f);
		modelCub = glm::translate(modelCub, delta);
		shader2.setMatrix4("M", modelCub);
		shader2.setMatrix4("V", view);
		shader2.setMatrix4("P", perspective);
		shader2.setInteger("ourTexture", 0);	
		cub.draw();

		fps(now);
		glfwSwapBuffers(window);
	}

	//clean up ressource
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}



// Mouse button callback for picking
static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int winW, winH;
        glfwGetWindowSize(window, &winW, &winH);

        // OpenGL’s origin is at the lower‑left
        PickingTexture::PixelInfo info = gPickingTexture.ReadPixel(static_cast<unsigned int>(xpos),
                                                                  static_cast<unsigned int>(winH - ypos - 1));

        std::cout << "\nPicked Object ID: " << info.ObjectID << std::endl;

		gSelectedObjectID    = static_cast<int>(info.ObjectID);
        gSelectionTimestamp  = glfwGetTime();
    }
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

