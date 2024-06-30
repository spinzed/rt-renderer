#include "Shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

std::string Shader::_baseDir;
std::unordered_map<std::string, Shader *> Shader::loadedShaders;

void Shader::setBaseDirectory(std::string baseDir) { _baseDir = baseDir; };

void Shader::checkCompilerErrors(unsigned int shader, std::string type) {
    int success;
    char infolog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infolog);
            fprintf(stderr,
                    "ERROR::SHADER_COMPILATION_ERROR of type: "
                    "%s\n%s\n-----------------------------------------------------\n",
                    type.c_str(), infolog);
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infolog);
            fprintf(stderr,
                    "ERROR::PROGRAM_LINKING_ERROR of type: "
                    "%s\n%s\n-------------------------------------------------------\n",
                    type.c_str(), infolog);
        }
    }
}

Shader::Shader(std::vector<ShaderData> shaders) {
    ID = glCreateProgram();

    for (auto &shader : shaders) {
        std::string code;
        std::ifstream shaderFile;
        shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            shaderFile.open(shader.path);
            std::stringstream shaderStream;
            shaderStream << shaderFile.rdbuf();
            shaderFile.close();
            code = shaderStream.str();
        } catch (std::ifstream::failure &e) {
            if (shader.optional)
                continue;
            fprintf(stderr, "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ\n");
        }
        const char *shaderCode = code.c_str();

        GLuint shaderID = glCreateShader(shader.type);
        glShaderSource(shaderID, 1, &shaderCode, NULL);
        glCompileShader(shaderID);
        checkCompilerErrors(shaderID, shader.path);

        glAttachShader(ID, shaderID);
        glDeleteShader(shaderID);
    }
    glLinkProgram(ID);
    checkCompilerErrors(ID, "PROGRAM");

    // cache uniform variable positions
    int i = 0;
    for (std::string uniformName : SHADER_UNIFORMS) {
        uniformPositions[i++] = getUniformLocation(uniformName);
    }
}

Shader *Shader::load(std::string naziv) {
    if (loadedShaders.find(naziv) != loadedShaders.end()) {
        return loadedShaders[naziv];
    }

    std::string base = Shader::_baseDir + "/" + naziv;
    std::vector<ShaderData> shaders = {
        {GL_VERTEX_SHADER, base + ".vert", false},
        {GL_FRAGMENT_SHADER, base + ".frag", false},
        {GL_GEOMETRY_SHADER, base + ".geom", true},
    };
    Shader *s = new Shader(shaders);
    loadedShaders[naziv] = s;
    return s;
}

Shader::~Shader() { glDeleteProgram(ID); }

void Shader::use() { glUseProgram(ID); }

GLint Shader::getUniformLocation(const std::string &name) { return glGetUniformLocation(ID, name.c_str()); }

void Shader::setUniformByLocation(int location, int size, int *val) const { glUniform1iv(location, size, val); }

void Shader::setUniform(int index, int val) const { glUniform1i(uniformPositions[index], val); }

void Shader::setUniform(int index, float val) const { glUniform1f(uniformPositions[index], val); }

void Shader::setUniform(int index, int size, float array[3]) const {
    glUniform3fv(uniformPositions[index], size, array);
}

void Shader::setUniform(int index, int size, glm::vec3 vector) const {
    glUniform3fv(uniformPositions[index], size, glm::value_ptr(vector));
}

void Shader::setUniform(int index, int size, std::vector<float> vector) const {
    glUniform3fv(uniformPositions[index], size, &vector[0]);
}

void Shader::setUniform(int index, int size, glm::mat4 matrix) const {
    glUniformMatrix4fv(uniformPositions[index], size, GL_FALSE, glm::value_ptr(matrix));
}
