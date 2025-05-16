#version 330 core
in vec3 position; 
in vec2 tex_coord; 
in vec3 normal; 

out vec4 v_col; 
out vec2 v_t; 

out vec3 v_frag_coord; 
out vec3 v_normal; 

uniform mat4 itM; 
		
uniform mat4 M; 
uniform mat4 V; 
uniform mat4 P; 

void main(){ 


vec4 frag_coord = M*vec4(position, 1.0); 
// Correct order: Projection * View * Model * position
gl_Position = P * V * frag_coord;

v_col = vec4(normal*0.5 + 0.5, 1.0);
v_t = tex_coord; 
v_normal = vec3(itM * vec4(normal, 1.0)); 
v_frag_coord = frag_coord.xyz; 
}