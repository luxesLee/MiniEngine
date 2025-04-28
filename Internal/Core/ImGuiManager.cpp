#include "ImGuiManager.h"
#include "Config.h"
#include "Camera.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

ImGuiManager::ImGuiManager(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 460";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

ImGuiManager::~ImGuiManager()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

bool show_demo_window = true;
bool show_another_window = false;
void ImGuiManager::Render()
{
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    RenderMainMenuBar();
    RenderSettingWindow();

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiManager::RenderMainMenuBar()
{
    if(ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("File"))
        {
            if(ImGui::MenuItem("Open", "Ctrl+O"))
            {
                RenderOpenDialog();
            }
            if(ImGui::MenuItem("Dump"))
            {

            }

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void ImGuiManager::RenderOpenDialog()
{
    
}

void ImGuiManager::RenderSettingWindow()
{
    ImGui::SetNextWindowPos(ImVec2(g_Config->screenWidth, 20));
    ImGui::SetNextWindowSize(ImVec2(g_Config->imguiWidth, g_Config->screenHeight - 20));
    if(ImGui::Begin("Setting", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        if(ImGui::CollapsingHeader("Basic", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // FPS
            ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("Per Frame cost: %.2f ms", 1000.0f / io.Framerate);

            // Renderdoc Button
            ImGui::Text("debug (RenderDoc):");
            ImGui::SameLine();
            if (ImGui::SmallButton("capture")) 
            {
                g_Config->bRenderdocCapture = true;
            }
        }

        // render
        if(ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Current rendering accum %d frames", g_Config->accumulateFrames);
            ImGui::Checkbox("Render BaseColor", &g_Config->bShadeBaseColor);

            // Denoiser
            const char* Denoises[] = {"None", "SVGF", "OIDN", "OPTIX"};
            if(ImGui::BeginCombo("Current denoiser :", Denoises[Int(g_Config->curDenoise)]))
            {
                for(int i = 0; i < 4; i++)
                {
                    bool isSelected = Int(g_Config->curDenoise) == i;
                    if(ImGui::Selectable(Denoises[i], isSelected))
                    {
                        g_Config->curDenoise = (DenoiseType)i;
                    }
                    if(isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // ToneMapping
            const char* ToneMappings[] = {"None", "Linear", "ACES", "TonyMcMapface"};
            if(ImGui::BeginCombo("Current ToneMapping :", ToneMappings[Int(g_Config->curToneMapping)]))
            {
                for(int i = 0; i < 4; i++)
                {
                    bool isSelected = Int(g_Config->curToneMapping) == i;
                    if(ImGui::Selectable(ToneMappings[i], isSelected))
                    {
                        g_Config->curToneMapping = (ToneMappingType)i;
                    }
                    if(isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SliderInt("PathTracing Depth", &g_Config->maxRayTracingDepth, 1.0f, 8.0f);
        }

        // shadow
        if(ImGui::CollapsingHeader("Shadow", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Render Shadow", &g_Config->bShadeShadow);
            ImGui::Checkbox("Render PCF", &g_Config->bPCF);
            ImGui::Checkbox("Use CascadeShadow", &g_Config->bCascadeShadow);
            ImGui::SliderInt("Cascade Level", &g_Config->cascadeLevel, 1, 6);
            ImGui::Text("ShadowMap Resolution:(%d x %d)", g_Config->shadowDepthWidth, g_Config->shadowDepthHeight);
        }

        // Indirect Lighting
        if(ImGui::CollapsingHeader("Indirect Lighting", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("SSAO", &g_Config->bSSAO);
            ImGui::SliderInt("SSAO Kernel Size", &g_Config->SSAOKernelSize, 0, 64);
            ImGui::SliderFloat("SSAO Radius", &g_Config->SSAORadius, 0.05f, 1.0f);
            ImGui::SliderFloat("SSAO Bias", &g_Config->SSAOBias, 0.01f, 0.1f);

            ImGui::Checkbox("VXGI", &g_Config->bVXGI);
            if(ImGui::RadioButton("Voxel Size: 16", &g_Config->VoxelSize, 16))
            {
                g_Config->VoxelMipmapLvel = 1;
            }
            if(ImGui::RadioButton("Voxel Size: 64", &g_Config->VoxelSize, 64))
            {
                g_Config->VoxelMipmapLvel = 3;
            }
            if(ImGui::RadioButton("Voxel Size: 256", &g_Config->VoxelSize, 256))
            {
                g_Config->VoxelMipmapLvel = 6;
            }
        }

        // Debug
        auto highestBitPosition = [](unsigned int x)
        {
            if(x == 0) return -1;
            int pos = 0;
            while(x != 0)
            {
                x >>= 1;
                pos++;
            }
            return pos - 1;
        };
        const char* DebugModes[] = {"None", "Shadow", "VXGI"};
        if(ImGui::BeginCombo("Current DebugMode :", DebugModes[highestBitPosition(Uint(g_Config->debugMode))]))
        {
            for(int i = 0; i < 3; i++)
            {
                bool isSelected = (Int(g_Config->debugMode) == (1 << i));
                if(ImGui::Selectable(DebugModes[i], isSelected))
                {
                    g_Config->debugMode = DebugMode(1 << i);
                }
                if(isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }


        // Camera
        if(ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Camera Position:(%.5f, %.5f, %.5f)", g_Camera->Position.x, g_Camera->Position.y, g_Camera->Position.z);
            ImGui::Text("Camera Front:(%.5f, %.5f, %.5f)", g_Camera->Front.x, g_Camera->Front.y, g_Camera->Front.z);

            ImGui::SliderFloat("Camera MoveSpeed", &g_Config->cameraMoveSpeed, 0.0f, 30.0f);
            ImGui::SliderFloat("Camera RotSensitivity", &g_Config->cameraRotSensitivity, 0.0f, 0.2f);
            ImGui::SliderFloat("Camera zNear", &g_Config->cameraZFar, 0.1f, 10.0f);
            ImGui::SliderFloat("Camera zFar", &g_Config->cameraZNear, 100.0f, 1000.0f);
        }

        ImGui::End();
    }
}
