#ifndef TEXT_H
#define TEXT_H

// utils/text.cpp  — rendu texte OpenGL + FreeType
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include <string>
#include <iostream>
#include "shader.h"


// GLuint text_VBO, text_VAO;   // VAO/VBO pour le texte

/* ──────────────  STRUCT & DATA  ────────────── */
struct Character {
    unsigned int TextureID;      // texture ID du glyphe
    glm::ivec2   Size;           // largeur / hauteur
    glm::ivec2   Bearing;        // offset baseline → coin haut-gauche
    unsigned int Advance;        // avance horizontale (1/64 px)
};

static std::map<char, Character> Characters;   // ASCII → glyphes

/* ──────────────  LOAD FONT / CREATE GLYPH TEXTURES  ────────────── */
void LoadFontCharacters(const char* fontPath, unsigned int pixelSize /* =48 */)
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) { std::cerr << "[text] FreeType init failed\n"; return; }

    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face))   { std::cerr << "[text] Font load failed: " << fontPath << '\n'; FT_Done_FreeType(ft); return; }
    FT_Set_Pixel_Sizes(face, 0, pixelSize);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);     // bitmap 1 octet / pixel

    for (unsigned char c = 0; c < 128; ++c)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        { std::cerr << "[text] glyph " << int(c) << " failed\n"; continue; }

        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                     face->glyph->bitmap.width,
                     face->glyph->bitmap.rows,
                     0, GL_RED, GL_UNSIGNED_BYTE,
                     face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Characters.emplace(c, Character{
            tex,
            { face->glyph->bitmap.width,  face->glyph->bitmap.rows },
            { face->glyph->bitmap_left,   face->glyph->bitmap_top  },
            static_cast<unsigned int>(face->glyph->advance.x)
        });
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

/* ──────────────  CREATE VAO/VBO  ────────────── */
void SetupTextVAO(GLuint& VAO, GLuint& VBO)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

/* ──────────────  RENDER TEXT  ────────────── */
void RenderText(Shader& shader, const std::string& text, float x, float y, float scale,glm::vec3 color,GLuint VAO, GLuint VBO)
{
    shader.use();
    bool depthOn = glIsEnabled(GL_DEPTH_TEST);
    if (depthOn) glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader.setVector3f("textColor", color);
    shader.setMatrix4("projection", glm::ortho(0.f, 800.f, 0.f, 600.f));
    shader.setInteger("text", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    for (char c : text)
    {
        const Character& ch = Characters[c];
        if (ch.Size.x == 0 || ch.Size.y == 0) { x += (ch.Advance >> 6) * scale; continue; }

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float verts[6][4] = {
            { xpos,     ypos + h,   0.f, 0.f },
            { xpos,     ypos,       0.f, 1.f },
            { xpos+w,   ypos,       1.f, 1.f },
            { xpos,     ypos + h,   0.f, 0.f },
            { xpos+w,   ypos,       1.f, 1.f },
            { xpos+w,   ypos + h,   1.f, 0.f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.Advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (depthOn) glEnable(GL_DEPTH_TEST);
}

#endif