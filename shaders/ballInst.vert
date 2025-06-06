#version 330 core
/*========= attributs du maillage =========*/
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 tex_coord;   // ← non-utilisé mais laissé pour compatibilité VAO
layout (location = 2) in vec3 normal;

/*========= attributs d’instance =========*/
layout (location = 3) in vec4 iModel0;     // colonne 0 de la mat4
layout (location = 4) in vec4 iModel1;     // colonne 1
layout (location = 5) in vec4 iModel2;     // colonne 2
layout (location = 6) in vec4 iModel3;     // colonne 3

layout (location = 7) in vec3 iColor;   // nouvelle entrée

/*========= uniforms communs =========*/
uniform mat4 V;          // vue
uniform mat4 P;          // projection
/* (optionnel) uniform mat4 U = mat4(1); // transfo globale éventuelle */
// matrice‑modèle globale (translation verticale + éventuelle échelle)
uniform mat4 M;
// inverse‑transpose de M, au cas où l’on voudrait la composer avec Minst
uniform mat4 itM;

/*========= sorties vers le fragment shader =========*/
out vec3 v_frag_coord;    // position monde
out vec3 v_normal;        // normale monde

out vec3 v_color;                       // vers le FS

void main()
{
    mat4 Minst  = mat4(iModel0, iModel1, iModel2, iModel3);   // matrice‑modèle propre à l’instance
    // instance * modèle global (évite la mise à l’échelle des translations)
    mat4 Mworld = Minst * M;                                  // instance * modèle global (évite la mise à l’échelle des translations)

    vec4 worldPos = Mworld * vec4(position, 1.0);
    v_frag_coord  = worldPos.xyz;

    // matrice inverse-transpose pour la normale
    v_normal = mat3(transpose(inverse(Mworld))) * normal;
    v_color = iColor;

    gl_Position = P * V * worldPos;
}