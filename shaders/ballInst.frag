#version 330 core
out vec4 FragColor;

/*========= entrées venues du vertex shader =========*/
in vec3 v_frag_coord;
in vec3 v_normal;
in vec3 v_color;              // reçu du VS

/*========= uniforms =========*/
uniform vec3       u_view_pos;      // position de la caméra en espace monde
uniform samplerCube cubemapSampler; // cubemap déjà bindé sur l’unité 0

void main()
{
    vec3 N = normalize(v_normal);
    vec3 V = normalize(u_view_pos - v_frag_coord);  // direction vue → fragment
    vec3 R = reflect(-V, N);                        // vecteur réfléchi

    // FragColor = texture(cubemapSampler, R);
    FragColor = vec4(v_color, 1.0); 
}