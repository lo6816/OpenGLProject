#version 330 core
out vec4 FragColor;
precision mediump float; 
in vec4 frag_coord; 
uniform vec3 color; 
void main(){ 
FragColor = vec4(color, 0.2); 
}