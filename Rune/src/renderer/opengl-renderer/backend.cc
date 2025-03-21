// clang-format off
#include "glad/glad.h"
#include "GLFW/glfw3.h"
// clang-format on

#include "backend.hh"

#include <GL/gl.h>
#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <glm/vec3.hpp>
#include <print>
#include <vector>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "core/logger/logger.hh"
#include "imgui.h"

// FIXME: check for errors

namespace fs = std::filesystem;

namespace Rune::GL
{
    namespace
    {
        constexpr std::array dummy{
            glm::vec3{ -0.5f, -0.5f, 0.f },
            glm::vec3{ 0.5f, -0.5f, 0.f },
            glm::vec3{ 0.f, 0.5f, 0.f },
        };

        void debug_callback(GLenum, GLenum type, GLuint, GLenum sev, GLsizei,
                            const char* msg, const void*)
        {
            if (sev == GL_DEBUG_SEVERITY_NOTIFICATION)
            {
                return;
            }

            const bool is_error = type == GL_DEBUG_TYPE_ERROR;
            Logger::log(is_error ? Logger::ERROR : Logger::INFO,
                        (sev == GL_DEBUG_SEVERITY_HIGH ? "[GL][HIGH]" : "[GL]"),
                        msg);
        }

        u32 compile_shader_file(const fs::path& shader_file, GLenum type,
                                bool fatal = false)
        {
            auto shader = glCreateShader(type);

            auto file = std::ifstream(shader_file, std::ios::ate);

            if (!file)
                Logger::error(
                    std::format("Invalid file at {}", shader_file.c_str()),
                    true);

            int size = file.tellg();

            file.seekg(0);

            // + null byte
            std::vector<char> source(size + 1);

            file.read(source.data(), size);
            auto cstrsource = source.data();

            glShaderSource(shader, 1, &cstrsource, nullptr);
            glCompileShader(shader);

            int res = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &res);

            if (!res)
            {
                constexpr i32 len = 1024;
                std::array<char, len> log;
                glGetShaderInfoLog(shader, sizeof(log), const_cast<int*>(&len),
                                   log.data());

                Logger::error(
                    std::format("Compilation for {} failed with message '{}'",
                                shader_file.c_str(),
                                std::string(log.begin(), log.end())),
                    fatal);
            }

            Logger::log(Logger::INFO, "Successfully compiled",
                        shader_file.c_str());

            return shader;
        }
    } // namespace

    void Backend::init(Window* window, std::string_view, i32 width, i32 height)
    {
        window_ = window;
        viewport_ = { width, height };

        Logger::log(Logger::INFO, "Initializing OpenGL renderer");

        init_gl();
        init_debug_callbacks();
        init_imgui();
        init_default_program();
    }

    void Backend::init_gl()
    {
        assert(window_);
        glfwMakeContextCurrent(window_->get());
        glfwSwapInterval(1);

        if (!gladLoadGLLoader((GLADloadproc)(glfwGetProcAddress)))
            Logger::abort("Failed to initialize glad");

        Logger::log(Logger::INFO, "OpenGL", glGetString(GL_VERSION),
                    "initialized on", glGetString(GL_VENDOR),
                    glGetString(GL_RENDERER), "using GLSL",
                    glGetString(GL_SHADING_LANGUAGE_VERSION));

        glViewport(0, 0, viewport_.width, viewport_.height);
        glClearColor(0.5f, 0.7f, 0.8f, 0.0f);
    }

    void Backend::init_debug_callbacks()
    {
        glDebugMessageCallback(&debug_callback, nullptr);
        glEnable(GL_DEBUG_OUTPUT);

        Logger::log(Logger::INFO, "Initialized debug callbacks");
    }

    void Backend::init_imgui()
    {
        ImGui_ImplGlfw_InitForOpenGL(window_->get(), true);
        ImGui_ImplOpenGL3_Init();
        imgui_initialized_ = true;
        Logger::log(Logger::INFO, "Initialized imgui for OpenGl backend");
    }

    void Backend::init_default_program()
    {
        default_program_ = glCreateProgram();

        auto vertex =
            compile_shader_file(RUNE_GL_DEFAULT_SHADER_PATH "/default.vert",
                                GL_VERTEX_SHADER, true);
        glAttachShader(default_program_, vertex);

        auto fragment =
            compile_shader_file(RUNE_GL_DEFAULT_SHADER_PATH "/default.frag",
                                GL_FRAGMENT_SHADER, true);
        glAttachShader(default_program_, fragment);

        glLinkProgram(default_program_);

        int res = 0;
        glGetProgramiv(default_program_, GL_LINK_STATUS, &res);

        if (!res)
        {
            constexpr i32 len = 1024;
            std::array<char, len> log;
            glGetShaderInfoLog(default_program_, sizeof(log),
                               const_cast<int*>(&len), log.data());

            Logger::abort(std::format("message '{}'",
                                      std::string(log.begin(), log.end())));
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);

        glCreateVertexArrays(1, &vao_);
        glEnableVertexArrayAttrib(vao_, 0);
        glVertexArrayAttribFormat(vao_, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao_, 0, 0);

        glCreateBuffers(1, &vbo_);
        glNamedBufferStorage(vbo_, sizeof(dummy), dummy.data(),
                             GL_DYNAMIC_STORAGE_BIT);
        glVertexArrayVertexBuffer(vao_, 0, vbo_, 0, sizeof(glm::vec3));

        Logger::log(Logger::INFO, "Initialized default program");
    }

    bool Backend::is_imgui_initialized()
    {
        return imgui_initialized_;
    }

    void Backend::imgui_frame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void Backend::draw_frame()
    {
        glBindVertexArray(vao_);
        glUseProgram(default_program_);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        imgui_frame();

        glfwSwapBuffers(window_->get());

        glClear(GL_COLOR_BUFFER_BIT);
    }

    void Backend::cleanup()
    {
        Logger::log(Logger::INFO, "Cleaning up OpenGl renderer");
        glDeleteProgram(default_program_);
        ImGui_ImplOpenGL3_Shutdown();
    }
} // namespace Rune::GL
