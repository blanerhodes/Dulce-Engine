#pragma once

struct OpenGlInfo {
    char* vendor;
    char* renderer;
    char* version;
    char* shading_language_version;
    char* extensions;
    b32 GL_EXT_texture_sRGB;
    b32 GL_ARB_framebuffer_sRGB;
    b32 GL_ARB_framebuffer_object;
};

struct OpenGl {
    //b32 supports_srgb_framebuffer;
    //GLuint reserved_blit_texture;
    GLuint default_internal_texture_format;
    GLuint default_program;
};