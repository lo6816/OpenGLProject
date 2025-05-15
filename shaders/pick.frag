#version 330

layout(location = 0) out vec3 FragColor;
uniform float ObjectID;
void main(){
    FragColor = vec3(ObjectID, 0.0, 0.0);
}