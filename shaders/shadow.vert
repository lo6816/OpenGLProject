
#version 330 core
layout(location=0) in vec3 position; 
out vec4 frag_coord; 
uniform mat4 MVP; 
void main(){ 
frag_coord = MVP*vec4(position, 1.0); 
gl_Position = frag_coord; 
}