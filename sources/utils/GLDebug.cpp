#include "utils/GLDebug.h"

#include <iostream>
#include <string>

void glCheckError_(const char *file, int line) {
    // glCheckFramebuffer_(file, line);
    GLenum err(glGetError());
    while (err != GL_NO_ERROR) {
        std::string error;
        switch (err) {
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
#if DEBUG_PRINT_OPENGL_ERRORS
        std::cerr << "GL_" << error.c_str() << " - " << file << ":" << line << std::endl;
#endif
        err = glGetError();
    }
}

void glCheckFramebuffer_(const char *file, int line) {
    GLint depthAttachment;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                                          &depthAttachment);
    if (depthAttachment == GL_NONE) {
        std::cerr << "Framebuffer attachment error (depth) " << file << ":" << line << std::endl;
    }
}

void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                              const GLchar *message, const void *userParam) {
    if (severity != GL_DEBUG_SEVERITY_LOW) {
        printf("OpenGL Debug Message: %s\n", message);
    }
}
