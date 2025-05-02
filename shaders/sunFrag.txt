#version 330 core
out vec4 FragColor;
uniform sampler2D ourTexture; 
precision mediump float; 
in vec4 v_col; 
in vec2 v_t; 
uniform float intensity;
void main() { 
// FragColor = v_col*(1.0-v_t.y); 
// FragColor = v_col*(1.0); 
FragColor = texture(ourTexture, v_t) * intensity; 
} 
