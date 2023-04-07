#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {

    glm::vec3 position;
    glm::vec3 direction;

    float cutOff;
    float outerCutOff;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = false;
    glm::vec3 initPosition = glm::vec3(10.0f, -125.0f, 0.0f);
    float initScale = 3.0f;
    PointLight pointLight;
    SpotLight spotLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // build and compile shaders
    // -------------------------
    Shader objShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader blendShader("resources/shaders/blending.vs", "resources/shaders/blending.fs");

    // load models
    // -----------
    Model floorModel("resources/objects/floor/stone_floor.obj");
    floorModel.SetShaderTextureNamePrefix("material.");

    Model sceneModel("resources/objects/scene/bank.obj");
    sceneModel.SetShaderTextureNamePrefix("material.");

//    Model tvModel("resources/objects/tv/Old_Tv.obj");
//    tvModel.SetShaderTextureNamePrefix("material.");
//
//    Model chairModel("resources/objects/chair/Chair2.obj");
//    chairModel.SetShaderTextureNamePrefix("material.");
//
//    Model gunModel("resources/objects/gun/m1911pistol.obj");
//    gunModel.SetShaderTextureNamePrefix("material.");
//
//    Model smokegModel1("resources/objects/smokeg/smokeg.obj");
//    smokegModel1.SetShaderTextureNamePrefix("material.");
//    Model smokegModel2("resources/objects/smokeg/smokeg.obj");
//    smokegModel2.SetShaderTextureNamePrefix("material.");
//
//    Model arModel("resources/objects/ar/Reference.obj");
//    arModel.SetShaderTextureNamePrefix("material.");
//
//    Model sniperModel("resources/objects/sniper/Model.obj");
//    sniperModel.SetShaderTextureNamePrefix("material.");

    Model picModel("resources/objects/picture/picture.obj");
    picModel.SetShaderTextureNamePrefix("material.");

    Model glassModel("resources/objects/glass/glass.obj");
    glassModel.SetShaderTextureNamePrefix("material.");



    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(10.0f, -120.0f, 0.0);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);
    pointLight.constant = 1.0f;
    pointLight.linear = 0.00f;
    pointLight.quadratic = 0.0f;


    SpotLight& spotLight = programState->spotLight;
    spotLight.position = glm::vec3(9.75f, -118.0f, 4.0);
    spotLight.direction = glm::vec3(0.0f, -125.0f, 0.0);
    spotLight.cutOff = glm::cos(glm::radians(10.0f));
    spotLight.outerCutOff = glm::cos(glm::radians(15.0f));
    spotLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    spotLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    spotLight.specular = glm::vec3(1.0, 1.0, 1.0);
    spotLight.constant = 1.0f;
    spotLight.linear = 0.0f;
    spotLight.quadratic = 0.0f;




    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        objShader.use();
        pointLight.position = glm::vec3(7.0 * cos(currentFrame), -120.0f, 7.0 * sin(currentFrame));
        objShader.setVec3("pointLight.position", pointLight.position);
        objShader.setVec3("pointLight.ambient", pointLight.ambient);
        objShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        objShader.setVec3("pointLight.specular", pointLight.specular);
        objShader.setFloat("pointLight.constant", pointLight.constant);
        objShader.setFloat("pointLight.linear", pointLight.linear);
        objShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        objShader.setVec3("viewPosition", programState->camera.Position);

        objShader.setVec3("spotLight.position", spotLight.position);
        objShader.setVec3("spotLight.direction", spotLight.direction);
        objShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        objShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);
        objShader.setVec3("spotLight.ambient", spotLight.ambient);
        objShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        objShader.setVec3("spotLight.specular", spotLight.specular);
        objShader.setFloat("spotLight.constant", spotLight.constant);
        objShader.setFloat("spotLight.linear", spotLight.linear);
        objShader.setFloat("spotLight.quadratic", spotLight.quadratic);


        objShader.setFloat("material.shininess", 32.0f);
        //objShader.setVec3("material.ambient", 1.0f, 0.5f, 0.3f);
        //objShader.setVec3("material.diffuse", 1.0f, 0.5f, 0.3f);
        //objShader.setVec3("material.specular", 0.5f, 0.5f, 0.5f);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        objShader.setMat4("projection", projection);
        objShader.setMat4("view", view);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(10.0f, -125.0f, 0.0f));
        model = glm::scale(model, glm::vec3(3.0f));
        objShader.setMat4("model", model);
        floorModel.Draw(objShader);

        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(10.0f, -124.9f, 1.0f));
        model = glm::scale(model, glm::vec3(0.35f));
        objShader.setMat4("model", model);
        sceneModel.Draw(objShader);

//        model = glm::mat4(1.0f);
//        model = glm::translate(model, glm::vec3(12.6f, -122.92f, -0.7f));
//        model = glm::scale(model, glm::vec3(0.3f));
//        model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(0,1,0));
//        objShader.setMat4("model", model);
//        tvModel.Draw(objShader);
//
//        model = glm::mat4(1.0f);
//        model= glm::translate(model, glm::vec3(9.3f, -124.0f, 4.7f));
//        model = glm::scale(model, glm::vec3(0.0025f));
//        model = glm::rotate(model, glm::radians(3.0f), glm::vec3(1,0,0));
//        model = glm::rotate(model, glm::radians(3.0f), glm::vec3(0,0,1));
//        model = glm::rotate(model, glm::radians(135.0f), glm::vec3(0,1,0));
//        objShader.setMat4("model", model);
//        chairModel.Draw(objShader);
//
//        model = glm::mat4(1.0f);
//        model= glm::translate(model, glm::vec3(12.5f, -123.373f, 1.7f));
//        model = glm::scale(model, glm::vec3(0.045));
//        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0,0,1));
//        model = glm::rotate(model, glm::radians(-135.0f), glm::vec3(1,0,0));
//        objShader.setMat4("model", model);
//        gunModel.Draw(objShader);
//
//        model = glm::mat4(1.0f);
//        model= glm::translate(model, glm::vec3(12.6f, -123.222f, 0.9f));
//        model = glm::scale(model, glm::vec3(0.22f));
//        objShader.setMat4("model", model);
//        smokegModel1.Draw(objShader);

//        model = glm::mat4(1.0f);
//        model= glm::translate(model, glm::vec3(12.8f, -123.222f, 1.1f));
//        model = glm::scale(model, glm::vec3(0.22f));
//        objShader.setMat4("model", model);
//        smokegModel2.Draw(objShader);
//
//        model = glm::mat4(1.0f);
//        model= glm::translate(model, glm::vec3(10.1f, -123.22f, 4.3f));
//        model = glm::scale(model, glm::vec3(0.1f));
//        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1,0,0));
//        model = glm::rotate(model, glm::radians(225.0f), glm::vec3(0,0,1));
//        model = glm::rotate(model, glm::radians(-15.0f), glm::vec3(0,1,0));
//        model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0,0,1));
//        objShader.setMat4("model", model);
//        arModel.Draw(objShader);
//
//        model = glm::mat4(1.0f);
//        model= glm::translate(model, glm::vec3(8.5f, -124.393f, 2.415f));
//        model = glm::scale(model, glm::vec3(0.06f));
//        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1,0,0));
//        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0,0,1));
//        model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0,1,0));
//        objShader.setMat4("model", model);
//        sniperModel.Draw(objShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(10.0f, -120.0f, -5.0f));
        model = glm::scale(model, glm::vec3(5.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        picModel.Draw(objShader);


        // Blending
        blendShader.use();
        blendShader.setMat4("projection", projection);
        blendShader.setMat4("view", view);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(15.0f, -117.5f, 2.5f));
        model = glm::scale(model, glm::vec3(7.5));
        blendShader.setMat4("model", model);
        glassModel.Draw(blendShader);







        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Test object");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Object position", (float*)&programState->initPosition);
        ImGui::DragFloat("Object scale", &programState->initScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);

        ImGui::Text("Spotlight");
        ImGui::DragFloat3("Spotlight position", (float*)&programState->spotLight.position);
        ImGui::DragFloat("spotLight.constant", &programState->spotLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("spotLight.linear", &programState->spotLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("spotLight.quadratic", &programState->spotLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}
