#version 330

uniform uint gDrawIndex;
uniform uint gObjectIndex;

out vec3 FragColor;

void main()
{
    FragColor = vec3(ObjectID, 0.0, 0.0);
}