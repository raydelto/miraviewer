//-----------------------------------------------------------------------------
// Models_01.cpp by Steve Jones
// Copyright (c) 2015-2019 Game Institute. All Rights Reserved.
//
// - Creates Mesh class
// - Loads and renders (3) OBJ models
//-----------------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <string>
#ifdef __APPLE__
#include <glad/glad.h>
#else
#define GLEW_STATIC
#include "GL/glew.h" // Important - this header must come before glfw3 header
#endif
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "ShaderProgram.h"
#include "Texture2D.h"
#include "Camera.h"
#include "Mesh.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"
#include "stb_image/stb_image.h"

namespace
{
    constexpr float MIN_FOV = 20.0f;
    constexpr float MAX_FOV = 90.0f;
    constexpr float MIN_ROTATION = 0.0f;
    constexpr float MAX_ROTATION = 360.0f;
    constexpr float Z_NEAR = 0.1f;
    constexpr float Z_FAR = 200.0f;
    constexpr float ZOOM_SENSITIVITY = -3.0;
    constexpr float MOVE_SPEED = 5.0f; // units per second
    constexpr float DRAG_THRESHOLD = 5.0f;

    const char *APP_TITLE = "MiraViewer v0.1";
    int gWindowWidth = 1024;
    int gWindowHeight = 768;
    GLFWwindow *gWindow = nullptr;
    bool gWireframe = false;
    bool gPerspectiveUpdated = true;
    bool gIsDragging = false;
    bool gSelectingTexture = false;

    FPSCamera gFpsCamera(glm::vec3(0.0f, 3.0f, 10.0f));

    float ginitialMouseX = 0.0;
    float gInitialMouseY = 0.0;
    float gLastMouseX = 0.0;
    float gLastMouseY = 0.0;

    float gModelRotationAngleX = 180.0;
    float gModelRotationAngleY = 180.0;
    float gMouseSensitivity = 100.0f;

    Mesh *gSelectedMesh = nullptr;
    Texture2D *gSelectedTexture = nullptr;
    bool gShowModelLoaderTool = false;

    std::string gModelPath;
    std::string gTexturePath;
}

// Function prototypes
void glfw_onKey(GLFWwindow *window, int key, int scancode, int action, int mode);
void glfw_onFramebufferSize(GLFWwindow *window, int width, int height);
void glfw_onMouseScroll(GLFWwindow *window, double deltaX, double deltaY);
void update(double elapsedTime);
void showFPS(GLFWwindow *window);
bool initOpenGL();
void initImGUI();
void renderMenuBar();

void renderMenuBar()
{
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            auto selectedFiles = ImGuiFileDialog::Instance()->GetSelection();

            for (const auto &file : selectedFiles)
            {
                if (gSelectingTexture)
                {
                    gTexturePath = file.second;
                }
                else
                {
                    gModelPath = file.second;
                }
            }
            gSelectingTexture = false;
        }

        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load", "Ctrl+L"))
            {
                gShowModelLoaderTool = true;
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
                glfwSetWindowShouldClose(gWindow, true);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (gShowModelLoaderTool)
    {
        ImGui::Begin("Model loader", &gShowModelLoaderTool);

        ImGui::Text("3D Model");
        ImGui::SameLine();
        if (ImGui::Button("...##3D"))
        {
            IGFD::FileDialogConfig config;
            config.path = ".";
            const char *filters = "Model files (*.obj *.md2 *.md3 *.md5 *.fbx *.dae *.stl *.ply){.obj,.md2,.md3,.md5,.fbx,.dae,.stl,.ply}";

            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose files", filters, config);
        }

        ImGui::Text("Texture");
        ImGui::SameLine();
        if (ImGui::Button("...##Texture"))
        {
            gSelectingTexture = true;
            IGFD::FileDialogConfig config;
            config.path = ".";
            const char *filters = "Image files (*.png *.gif *.jpg *.jpeg *.tga *.bmp){.png,.gif,.jpg,.jpeg,.tga,.bmp}";
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose files", filters, config);
        }

        if (ImGui::Button("Load"))
        {
            if (!gModelPath.empty() && !gTexturePath.empty())
            {
                gSelectedMesh = new Mesh();
                gSelectedMesh->loadModel(gModelPath);

                gSelectedTexture = new Texture2D();
                gSelectedTexture->loadTexture(gTexturePath);

                gModelPath.clear();
                gTexturePath.clear();
                gShowModelLoaderTool = false;
            }
            else
            {
                ImGui::OpenPopup("Validation Error");
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            gModelPath.clear();
            gTexturePath.clear();
            gShowModelLoaderTool = false;
        }

        if (ImGui::BeginPopupModal("Validation Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Both model and texture files must be selected.\n\n");
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }
}

//-----------------------------------------------------------------------------
// Main Application Entry Point
//-----------------------------------------------------------------------------
int main()
{
    if (!initOpenGL())
    {
        std::cerr << "GLFW initialization failed" << std::endl;
        return -1;
    }

    initImGUI();

    ShaderProgram shaderProgram;
    shaderProgram.loadShaders("shaders/basic.vert", "shaders/basic.frag");

    double lastTime = glfwGetTime();

    // Create the projection matrix
    glm::mat4 projection = glm::mat4(1.0f);
    ImVec4 clearColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    // Rendering loop
    while (!glfwWindowShouldClose(gWindow))
    {
        showFPS(gWindow);

        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;

        // Poll for and process events
        glfwPollEvents();
        update(deltaTime);

        // Clear the screen
        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = gFpsCamera.getViewMatrix();

        // Must be called BEFORE setting uniforms because setting uniforms is done
        // on the currently active shader program.
        shaderProgram.use();

        // Pass the matrices to the shader
        shaderProgram.setUniform("view", view);

        // Render the scene
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(gModelRotationAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, -glm::radians(gModelRotationAngleY), glm::vec3(0.0f, 1.0f, 0.0f));

        shaderProgram.setUniform("model", model);

        if (gSelectedTexture != nullptr)
        {
            gSelectedTexture->bind();
        }

        if (gSelectedMesh != nullptr)
        {
            gSelectedMesh->draw();
        }

        if (gSelectedTexture != nullptr)
        {
            gSelectedTexture->unbind(0);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderMenuBar();

        if (gSelectedMesh != nullptr)
        {
            float fov = gFpsCamera.getFOV();

            ImGui::Begin("Controls");

            ImGui::SliderFloat("X Axis Rotation", &gModelRotationAngleX, MIN_ROTATION, MAX_ROTATION);
            ImGui::SliderFloat("Y Axis Rotation", &gModelRotationAngleY, MIN_ROTATION, MAX_ROTATION);
            ImGui::SliderFloat("Mouse rotation sensitivity", &gMouseSensitivity, 100.0f, 1000.0f);

            if (ImGui::SliderFloat("Field of View (FOV)", &fov, MIN_FOV, MAX_FOV))
            {

                gFpsCamera.setFOV(glm::clamp(fov, MIN_FOV, MAX_FOV));
                gPerspectiveUpdated = true;
            }

            if (gPerspectiveUpdated)
            {
                projection = glm::perspective(glm::radians(gFpsCamera.getFOV()), static_cast<float>(gWindowWidth) / static_cast<float>(gWindowHeight), Z_NEAR, Z_FAR);
                gPerspectiveUpdated = false;
            }

            shaderProgram.setUniform("projection", projection);

            ImGui::ColorEdit3("Background color", (float *)&clearColor);

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap front and back buffers
        glfwSwapBuffers(gWindow);

        lastTime = currentTime;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();

    return 0;
}

//-----------------------------------------------------------------------------
// Initialize GLFW and OpenGL
//-----------------------------------------------------------------------------
bool initOpenGL()
{
    if (!glfwInit())
    {
        std::cerr << "GLFW initialization failed" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // forward compatible with newer versions of OpenGL as they become available but not backward compatible (it will not run on devices that do not support OpenGL 3.3

    // Create an OpenGL 3.3 core, forward compatible context window
    gWindow = glfwCreateWindow(gWindowWidth, gWindowHeight, APP_TITLE, nullptr, nullptr);
    if (gWindow == nullptr)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    // Set the window's icon
    GLFWimage images[1];
    images[0].pixels = stbi_load("resources/miraviewer.png", &images[0].width, &images[0].height, 0, 4);
    glfwSetWindowIcon(gWindow, 1, images);
    stbi_image_free(images[0].pixels);

    // Make the window's context the current one
    glfwMakeContextCurrent(gWindow);

#ifdef __APPLE__
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
#else
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }
#endif

    // Set the required callback functions
    glfwSetKeyCallback(gWindow, glfw_onKey);
    glfwSetFramebufferSizeCallback(gWindow, glfw_onFramebufferSize);
    glfwSetScrollCallback(gWindow, glfw_onMouseScroll);

    // Hides and grabs cursor, unlimited movement
    // glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // glfwSetCursorPos(gWindow, gWindowWidth / 2.0, gWindowHeight / 2.0);

    // Define the viewport dimensions
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    glEnable(GL_DEPTH_TEST);

    return true;
}

//-----------------------------------------------------------------------------
// Is called whenever a key is pressed/released via GLFW
//-----------------------------------------------------------------------------
void glfw_onKey(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
    {
        gWireframe = !gWireframe;
        if (gWireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

//-----------------------------------------------------------------------------
// Is called when the window is resized
//-----------------------------------------------------------------------------
void glfw_onFramebufferSize(GLFWwindow *window, int width, int height)
{
    gWindowWidth = width;
    gWindowHeight = height;
    gPerspectiveUpdated = true;
    glViewport(0, 0, static_cast<float>(gWindowWidth), static_cast<float>(gWindowHeight));
}

//-----------------------------------------------------------------------------
// Called by GLFW when the mouse wheel is rotated
//-----------------------------------------------------------------------------
void glfw_onMouseScroll(GLFWwindow *window, double deltaX, double deltaY)
{
    float fov = gFpsCamera.getFOV() + deltaY * ZOOM_SENSITIVITY;
    fov = glm::clamp(fov, MIN_FOV, MAX_FOV);
    gFpsCamera.setFOV(fov);
    gPerspectiveUpdated = true;
}

//-----------------------------------------------------------------------------
// Update stuff every frame
//-----------------------------------------------------------------------------
void update(double elapsedTime)
{

    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return; // Skip processing mouse input in GLFW
    }

    double mouseX, mouseY;
    glfwGetCursorPos(gWindow, &mouseX, &mouseY);

    if (glfwGetMouseButton(gWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        if (!gIsDragging)
        {
            // First click: store initial position
            ginitialMouseX = mouseX;
            gInitialMouseY = mouseY;
            gLastMouseX = mouseX;
            gLastMouseY = mouseY;
            gIsDragging = true;
        }

        // Check if movement is above threshold
        float deltaX = static_cast<float>(mouseX - ginitialMouseX);
        float deltaY = static_cast<float>(mouseY - gInitialMouseY);
        float distance = sqrt(deltaX * deltaX + deltaY * deltaY);

        if (distance > DRAG_THRESHOLD)
        {
            // Apply rotation only if the mouse has moved enough
            gModelRotationAngleX += static_cast<float>(mouseY - gLastMouseY) * gMouseSensitivity * static_cast<float>(elapsedTime);
            gModelRotationAngleY += static_cast<float>(mouseX - gLastMouseX) * gMouseSensitivity * static_cast<float>(elapsedTime);
            gModelRotationAngleX = glm::clamp(gModelRotationAngleX, MIN_ROTATION, MAX_ROTATION);
            gModelRotationAngleY = glm::clamp(gModelRotationAngleY, MIN_ROTATION, MAX_ROTATION);
        }

        gLastMouseX = mouseX;
        gLastMouseY = mouseY;
    }
    else
    {
        gIsDragging = false;
    }

    // Clamp mouse cursor to center of screen
    // glfwSetCursorPos(gWindow, gWindowWidth / 2.0, gWindowHeight / 2.0);

    // Camera FPS movement

    // Forward/backward
    if (glfwGetKey(gWindow, GLFW_KEY_W) == GLFW_PRESS)
        gFpsCamera.move(MOVE_SPEED * static_cast<float>(elapsedTime) * gFpsCamera.getLook());
    else if (glfwGetKey(gWindow, GLFW_KEY_S) == GLFW_PRESS)
        gFpsCamera.move(MOVE_SPEED * static_cast<float>(elapsedTime) * -gFpsCamera.getLook());

    // Strafe left/right
    if (glfwGetKey(gWindow, GLFW_KEY_A) == GLFW_PRESS)
        gFpsCamera.move(MOVE_SPEED * static_cast<float>(elapsedTime) * -gFpsCamera.getRight());
    else if (glfwGetKey(gWindow, GLFW_KEY_D) == GLFW_PRESS)
        gFpsCamera.move(MOVE_SPEED * static_cast<float>(elapsedTime) * gFpsCamera.getRight());

    // Up/down
    if (glfwGetKey(gWindow, GLFW_KEY_Z) == GLFW_PRESS)
        gFpsCamera.move(MOVE_SPEED * static_cast<float>(elapsedTime) * gFpsCamera.getUp());
    else if (glfwGetKey(gWindow, GLFW_KEY_X) == GLFW_PRESS)
        gFpsCamera.move(MOVE_SPEED * static_cast<float>(elapsedTime) * -gFpsCamera.getUp());
}

//-----------------------------------------------------------------------------
// Code computes the average frames per second, and also the average time it takes
// to render one frame.  These stats are appended to the window caption bar.
//-----------------------------------------------------------------------------
void showFPS(GLFWwindow *window)
{
    static double previousSeconds = 0.0;
    static int frameCount = 0;
    double elapsedSeconds;
    double currentSeconds = glfwGetTime(); // returns number of seconds since GLFW started, as double float

    elapsedSeconds = currentSeconds - previousSeconds;

    // Limit text updates to 4 times per second
    if (elapsedSeconds > 0.25)
    {
        previousSeconds = currentSeconds;
        double fps = static_cast<double>(frameCount) / elapsedSeconds;
        double msPerFrame = 1000.0 / fps;

        // The C++ way of setting the window title
        std::ostringstream outs;
        outs.precision(3); // decimal places
        outs << std::fixed
             << APP_TITLE << "    "
             << "FPS: " << fps << "    "
             << "Frame Time: " << msPerFrame << " (ms)";
        glfwSetWindowTitle(window, outs.str().c_str());

        // Reset for next average.
        frameCount = 0;
    }

    frameCount++;
}

void initImGUI()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(gWindow, true);
    ImGui_ImplOpenGL3_Init();
}
