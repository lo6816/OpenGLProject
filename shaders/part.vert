#version 330 core
in vec3 vertex;
in vec4 center;
in vec4 col;

uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform mat4 V;
uniform mat4 P;


out vec4 color;

void main(){
	//center is x,y,z pos of center + w for the size of the particle
	float scale = center.w;

	vec3 particleCenter = center.xyz;

	//BILLBOARDING
	//replace the usage of model matrice with just a translation for the position 
	vec3 vertexPosition_worldspace = particleCenter + cameraRight * vertex.x * scale + cameraUp * vertex.y * scale;
	

	gl_Position = P*V*vec4(vertexPosition_worldspace, 1.0);
	//gl_Position /= gl_Position.w;
	//color = col;
	color = col;
}