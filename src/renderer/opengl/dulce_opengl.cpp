#include "dulce_opengl.h"

#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_SRGB8_ALPHA8                   0x8C43

#define GL_SHADING_LANGUAGE_VERSION       0x8B8C
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_VALIDATE_STATUS                0x8B83

#define GL_FRAMEBUFFER                    0x8D40
#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5

#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT32              0x81A7
#define GL_DEPTH_COMPONENT32F             0x8CAC

#define GL_MULTISAMPLE                    0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE       0x809E
#define GL_SAMPLE_ALPHA_TO_ONE            0x809F
#define GL_SAMPLE_COVERAGE                0x80A0
#define GL_SAMPLE_BUFFERS                 0x80A8
#define GL_SAMPLES                        0x80A9
#define GL_SAMPLE_COVERAGE_VALUE          0x80AA
#define GL_SAMPLE_COVERAGE_INVERT         0x80AB
#define GL_TEXTURE_2D_MULTISAMPLE         0x9100
#define GL_MAX_SAMPLES                    0x8D57
#define GL_MAX_COLOR_TEXTURE_SAMPLES      0x910E
#define GL_MAX_DEPTH_TEXTURE_SAMPLES      0x910F

//Windows-specific
#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002


static OpenGlInfo OpenGlGetInfo(b32 modern_context) {
    OpenGlInfo result = {};
    result.vendor = (char*)glGetString(GL_VENDOR);
    result.renderer = (char*)glGetString(GL_RENDERER);
    result.version = (char*)glGetString(GL_VERSION);
    result.shading_language_version = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    result.extensions = (char*)glGetString(GL_EXTENSIONS);

    char* at = result.extensions;
    while(*at) {
        while(*at == ' ') {at++;}
        char* end = at;
        while (*end && (*end != ' ')) {end++;}
        i32 count = end - at;

        //TODO: set bools based on extensions
        if (0) {}
        else if (StringsEqual(at, "GL_ARB_framebuffer_sRGB")) {result.GL_ARB_framebuffer_sRGB = true;}
        else if (StringsEqual(at, "GL_EXT_texture_sRGB")) {result.GL_EXT_texture_sRGB = true;}
        else if (StringsEqual(at, "GL_ARB_framebuffer_object")) {result.GL_ARB_framebuffer_object = true;}
        at = end;
    }
    return result;
}

inline void OpenGlRectangle(Vec4 min, Vec4 max, Vec4 color) {
    glBegin(GL_TRIANGLES);

    glColor4f(color.r, color.g, color.b, color.a);

    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(min.x, min.y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(max.x, min.y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(max.x, max.y);

    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(min.x, min.y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(max.x, max.y);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(min.x, max.y);

    glEnd();

}

static void OpenGlRenderCommands() {
    //glViewport(0, 0, window_width, window_height)
    /*
    switch(type to render) {
        case clear:
            glclear stuff
        case bitmap:
            gl textured render call
        case rectangle:
            gl rectangle call
    }
    */
}

static GLuint OpenGlCreateProgram(char* header_code, char* header, char* vertex_code, char* fragment_code) {
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLchar* vertex_shader_code[] = {
       header_code,
       vertex_code 
    };
    glShaderSource(vertex_shader_id, ArrayCount(vertex_shader_code), vertex_shader_code, 0);
    glCompileShader(vertex_shader_id);

    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    GLchar* fragment_shader_code[] = {
        header_code,
        fragment_code
    };
    glShaderSource(fragment_shader_id, ArrayCount(fragment_shader_code), fragment_shader_code, 0);
    glCompileShader(fragment_shader_id);

    GLuint program_id = glCreateProgram(); //when shaders come together to make the whole pipeline
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);

    glValidateProgram(program_id);
    GLint validated = false;
    glGetProgramiv(program_id, GL_VALIDATE_STATUS, &validated);
    if (!validated) {
        u32 buff_size = 4096;
        GLsizei length;
        char vertex_errors[buff_size];
        char fragment_errors[buff_size];
        char program_errors[buff_size];
        glGetShaderInfoLog(vertex_shader_id, buff_size, &length, vertex_errors);
        glGetShaderInfoLog(fragment_shader_id, buff_size, &length, fragment_errors);
        glGetProgramInfoLog(program_id, buff_size, &length, program_errors);
        DASSERT(!"Shader validation failed");
    }

    return program_id;
}

static OpenGlInfo OpenGlInit(OpenGlInfo info) {
    //glGenTextures(1, &open_gl.reserved_blit_texture);
    g_open_gl.default_internal_texture_format = GL_RGBA8;
    if (0) {
        g_open_gl.default_internal_texture_format = GL_SRGB8_ALPHA8;
        glEnable(GL_FRAMEBUFFER_SRGB);
    }
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    char* header_code = R"(
        // Header code\n

    )";
    char* vertex_code = R"(
        // Vertex code\n
    )";
    char* fragment_code = R"(
        // Fragment code\n
    )";

    return info;
}

static HGLRC g_opengl_rc;
#define WGL_DRAW_TO_WINDOW_ARB           0x2001
#define WGL_ACCELERATION_ARB             0x2003
#define WGL_SUPPORT_OPENGL_ARB           0x2010
#define WGL_DOUBLE_BUFFER_ARB            0x2011
#define WGL_PIXEL_TYPE_ARB               0x2013
#define WGL_FULL_ACCELERATION_ARB        0x2027
#define WGL_TYPE_RGBA_ARB                0x202B
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20A9
#define WGL_RED_BITS_ARB                 0x2015
#define WGL_GREEN_BITS_ARB               0x2017
#define WGL_BLUE_BITS_ARB                0x2019
#define WGL_ALPHA_BITS_ARB               0x201B
#define WGL_DEPTH_BITS_ARB               0x2022

typedef BOOL WglSwapIntervalExt(i32 interval);
typedef HGLRC WglCreateContextAttribsARB(HDC hDC, HGLRC hShareContext, const int* attribList);
typedef BOOL WglGetPixelFormatAttribivARB(HDC hDC, i32 iPixelFormat, i32 iLayerPlane, u32 nAttributes, const int* piAttributes, i32* piValues);
typedef BOOL WglGetPixelFormatAttribifARB(HDC hDC, i32 iPixelFormat, i32 iLayerPlane, u32 nAttributes, const int* piAttributes, f32* piValues);
typedef BOOL WglChoosePixelFormatARB(HDC hDC, const i32* piAttribIList, const f32* pfAttribFList, u32 nMaxFormats, int* piFormats, u32* nNumFormats);
typedef const char* WglGetExtensionsStringEXT(void);

typedef void GlBindFramebuffer(GLenum target, GLuint framebuffer);
typedef void GlDeleteFramebuffers(GLsizei n, const GLuint* framebuffer);
typedef void GlGenFramebuffers(GLsizei n, GLuint* framebuffer);
typedef void GlFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum GlCheckFramebufferStatus(GLenum target);
typedef void GlAttachShader (GLuint program, GLuint shader);
typedef void GlBindAttribLocation (GLuint program, GLuint index, const GLchar *name);
typedef void GlCompileShader (GLuint shader);
typedef GLuint GlCreateProgram (void);
typedef GLuint GlCreateShader (GLenum type);
typedef void GlDeleteProgram (GLuint program);
typedef void GlDeleteShader (GLuint shader);
typedef void GlDetachShader (GLuint program, GLuint shader);
typedef void GlGetAttachedShaders (GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders);
typedef void GlGetProgramiv (GLuint program, GLenum pname, GLint *params);
typedef void GlGetProgramInfoLog (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void GlGetShaderiv (GLuint shader, GLenum pname, GLint *params);
typedef void GlGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void GlGetShaderSource (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source);
typedef void GlLinkProgram (GLuint program);
typedef void GlShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void GlUseProgram (GLuint program);
typedef void GlValidateProgram (GLuint program);

static GlBindFramebuffer* glBindFramebuffer;
static GlDeleteFramebuffers* glDeleteFramebuffers;
static GlGenFramebuffers* glGenFramebuffers;
static GlFramebufferTexture2D* glFramebufferTexture2D;

static GlAttachShader* glAttachShader;
static GlBindAttribLocation* glBindAttribLocation;
static GlCompileShader* glCompileShader;
static GlCreateProgram* glCreateProgram;
static GlCreateShader* glCreateShader;
static GlDeleteProgram* glDeleteProgram;
static GlDeleteShader* glDeleteShader;
static GlDetachShader* glDetachShader;
static GlGetAttachedShaders* glGetAttachedShaders;
static GlGetProgramiv* glGetProgramiv;
static GlGetProgramInfoLog* glGetProgramInfoLog;
static GlGetShaderiv* glGetShaderiv;
static GlGetShaderInfoLog* glGetShaderInfoLog;
static GlGetShaderSource* glGetShaderSource;
static GlLinkProgram* glLinkProgram;
static GlShaderSource* glShaderSource;
static GlUseProgram*  glUseProgram;
static GlValidateProgram* glValidateProgram;

static WglSwapIntervalExt* wglSwapInterval;
static WglCreateContextAttribsARB* wglCreateContextAttribsARB;
static WglGetPixelFormatAttribivARB* wglGetPixelFormatAttribivARB;
static WglGetPixelFormatAttribifARB* wglGetPixelFormatAttribifARB;
static WglChoosePixelFormatARB* wglChoosePixelFormatARB;
static WglGetExtensionsStringEXT* wglGetExtensionsStringEXT;

#include "dulce_opengl.h"
static OpenGl g_open_gl;

static void Win32SetPixelFormat(HDC window_dc) {
    i32 suggested_pixel_format_index = 0;
    GLuint extended_pick = 0;
    
    if (wglChoosePixelFormatARB) {
            i32 int_attrib_list[] = {
                WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
                WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
                WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
                WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
                WGL_RED_BITS_ARB, 8,
                WGL_GREEN_BITS_ARB, 8,
                WGL_BLUE_BITS_ARB, 8,
                WGL_ALPHA_BITS_ARB, 8,
                WGL_DEPTH_BITS_ARB, 32,
                //srgb stuff WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
                0
            };
            f32 float_attrib_list[] = {0};
            wglChoosePixelFormatARB(window_dc, int_attrib_list, float_attrib_list, 1, &suggested_pixel_format_index, &extended_pick);

    }
    if (!extended_pick) {
        PIXELFORMATDESCRIPTOR desired_pixel_format = {};
        desired_pixel_format.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        desired_pixel_format.nVersion = 1;
        desired_pixel_format.iPixelType = PFD_TYPE_RGBA;
        desired_pixel_format.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
        desired_pixel_format.cColorBits = 32;
        desired_pixel_format.cAlphaBits = 8;
        desired_pixel_format.cDepthBits = 32;
        desired_pixel_format.iLayerType = PFD_MAIN_PLANE;

        suggested_pixel_format_index = ChoosePixelFormat(window_dc, &desired_pixel_format);
    }
    PIXELFORMATDESCRIPTOR suggested_pixel_format = {};
    DescribePixelFormat(window_dc, suggested_pixel_format_index, sizeof(suggested_pixel_format), &suggested_pixel_format);
    SetPixelFormat(window_dc, suggested_pixel_format_index, &suggested_pixel_format);
}

static void Win32LoadWglExtensions() {
    WNDCLASSA window_class = {};

    window_class.lpfnWndProc = DefWindowProcA;
    window_class.hInstance = GetModuleHandle(0);
    window_class.lpszClassName = "WglLoader";

    if (RegisterClassA(&window_class)) {
        HWND window = CreateWindowExA(0, window_class.lpszClassName, "Name", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, window_class.hInstance, 0);
        HDC window_dc = GetDC(window);
        Win32SetPixelFormat(window_dc);
        HGLRC opengl_rc = wglCreateContext(window_dc);
        if (wglMakeCurrent(window_dc, opengl_rc)) {
            wglChoosePixelFormatARB = (WglChoosePixelFormatARB*)wglGetProcAddress("wglChoosePixelFormatARB");
            wglCreateContextAttribsARB = (WglCreateContextAttribsARB*)wglGetProcAddress("wglCreateContextAttribsARB");
            wglSwapInterval = (WglSwapIntervalExt*)wglGetProcAddress("wglSwapIntervalEXT");
            wglMakeCurrent(0,0);
        }
        wglDeleteContext(opengl_rc);
        ReleaseDC(window, window_dc);
        DestroyWindow(window);
    }
}

static HGLRC Win32InitOpenGl(HDC window_dc) {
    Win32LoadWglExtensions();

    b32 modern_context = true;
    HGLRC opengl_rc = 0;
    Win32SetPixelFormat(window_dc);
    if (wglCreateContextAttribsARB) {
        i32 attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 0,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
            0
        };
        Win32SetPixelFormat(window_dc);
        opengl_rc = wglCreateContextAttribsARB(window_dc, 0, attribs);
    }

    if (!opengl_rc) {
        modern_context = false;
        opengl_rc = wglCreateContext(window_dc);
    }

    if(wglMakeCurrent(window_dc, opengl_rc)) {
        OpenGlInfo info = OpenGlGetInfo(modern_context);
        if (info.GL_ARB_framebuffer_object) {
            glBindFramebuffer = (GlBindFramebuffer*)wglGetProcAddress("glBindFramebuffer");
            glDeleteFramebuffers = (GlDeleteFramebuffers*)wglGetProcAddress("glDeleteFramebuffers");
            glGenFramebuffers = (GlGenFramebuffers*)wglGetProcAddress("glGenFramebuffers");
            glFramebufferTexture2D = (GlFramebufferTexture2D*)wglGetProcAddress("glFramebufferTexture2D");
            glAttachShader = (GlAttachShader*)wglGetProcAddress("glAttachShader");
            glBindAttribLocation = (GlBindAttribLocation*)wglGetProcAddress("glBindAttribLocation");
            glCompileShader = (GlCompileShader*)wglGetProcAddress("glCompileShader");
            glCreateProgram = (GlCreateProgram*)wglGetProcAddress("glCreateProgram");
            glCreateShader = (GlCreateShader*)wglGetProcAddress("glCreateShader");
            glDeleteProgram = (GlDeleteProgram*)wglGetProcAddress("glDeleteProgram");
            glDeleteShader = (GlDeleteShader*)wglGetProcAddress("glDeleteShader");
            glDetachShader = (GlDetachShader*)wglGetProcAddress("glDetachShader");
            glGetAttachedShaders = (GlGetAttachedShaders*)wglGetProcAddress("glGetAttachedShaders");
            glGetProgramiv = (GlGetProgramiv*)wglGetProcAddress("glGetProgramiv");
            glGetProgramInfoLog = (GlGetProgramInfoLog*)wglGetProcAddress("glGetProgramInfoLog");
            glGetShaderiv = (GlGetShaderiv*)wglGetProcAddress("glGetShaderiv");
            glGetShaderInfoLog = (GlGetShaderInfoLog*)wglGetProcAddress("glGetShaderInfoLog");
            glGetShaderSource = (GlGetShaderSource*)wglGetProcAddress("glGetShaderSource");
            glLinkProgram = (GlLinkProgram*)wglGetProcAddress("glLinkProgram");
            glShaderSource = (GlShaderSource*)wglGetProcAddress("glShaderSource");
            glUseProgram = (GlUseProgram*)wglGetProcAddress("glUseProgram");
            glValidateProgram = (GlValidateProgram*)wglGetProcAddress("glValidateProgram");
        }

        if (wglSwapInterval) {
            wglSwapInterval(1);
        }
        OpenGlInit(info);
    }

    return opengl_rc;
}

/*
    glViewport(0, 0, window_width, window_height);

    glBindTexture(GL_TEXTURE_2D, texture_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, buffer->width, buffer->height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, buffer->memory);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);

    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW_MATRIX);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glBegin(GL_TRIANGLES);

    f32 p = 0.9f;
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-p, -p);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(p, -p);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(p, p);

    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-p, -p);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(p, p);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-p, p);

    glEnd();

    OpenGlRenderCommands();
*/

    //SwapBuffers(device_context);