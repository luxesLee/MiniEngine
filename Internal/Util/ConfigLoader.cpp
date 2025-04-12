#include "ConfigLoader.h"
#include "json.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"
#include <fstream>

using json = nlohmann::json;

void ParseConfig(const std::string &configFile, SceneConfig &config)
{
    json params = json::parse(std::ifstream(configFile));

    json cameraJson = params.value("camera", json::object());
    json modelJsons = params.value("models", json::array());
    json lightJsons = params.value("lights", json::array());
    json envMapJson = params.value("envMap", json::object());
    json matsJson = params.value("materials", json::array());

    auto getValue = [&](const json jsonMap, const std::string& key, std::vector<float>& output)
    {
        if(jsonMap.contains(key))
        {
            int m = jsonMap[key].size();
            for(int i = 0; i < m; i++)
            {
                output[i] = jsonMap[key][i];
            }
        }
        else
        {
            for(int i = 0; i < output.size(); i++)
            {
                output[i] = 0;
            }
        }
    };
    std::vector<float> vecTmp(4);

    for(const auto& modelJson : modelJsons)
    {
        if(modelJson.contains("path"))
        {
            ModelConfig modelConfig;
            glm::mat4 transform = glm::mat4(1.0f);

            modelConfig.modelPath = modelJson["path"];

            if(modelJson.contains("translation"))
            {
                getValue(modelJson, "translation", vecTmp);
                // transform[3] = glm::make_vec4(vecTmp.data());
                transform[3][0] = vecTmp[0];
                transform[3][1] = vecTmp[1];
                transform[3][2] = vecTmp[2];
            }
            if(modelJson.contains("rotation"))
            {
                getValue(modelJson, "rotation", vecTmp);
                transform = glm::mat4_cast(glm::make_quat(vecTmp.data())) * transform;
            }
            if(modelJson.contains("scale"))
            {
                getValue(modelJson, "scale", vecTmp);
                transform[0][0] *= vecTmp[0];
                transform[1][1] *= vecTmp[1];
                transform[2][2] *= vecTmp[2];
            }
            modelConfig.transform = transform;

            modelConfig.materialName = modelJson.value("material", "");

            config.modelConfigs.push_back(modelConfig);
        }
    }

    {
        getValue(cameraJson, "position", vecTmp);
        config.cameraConfig.position = glm::vec3(vecTmp[0], vecTmp[1], vecTmp[2]);
        getValue(cameraJson, "look_at", vecTmp);
        config.cameraConfig.lookAt = glm::vec3(vecTmp[0], vecTmp[1], vecTmp[2]);
        
        config.cameraConfig.zFar = cameraJson.value("far", 100.0);
        config.cameraConfig.zNear = cameraJson.value("near", 1.0);
        config.cameraConfig.zoom = cameraJson.value("zoom", 45.0);
    }

    for(const auto& lightJson : lightJsons)
    {
        LightConfig lightconfig;

        std::string lightType = lightJson.value("type", "");
        if(lightType == "")
        {
            continue;
        }
        else if(lightType == "directional")
        {
            lightconfig.type = DIRECTIONAL_LIGHT;
        }
        else if(lightType == "point")
        {
            lightconfig.type = POINT_LIGHT;
        }
        else if(lightType == "spot")
        {
            lightconfig.type = SPOT_LIGHT;
        }
        else if(lightType == "quad")
        {
            lightconfig.type = QUAD_LIGHT;

            if(!lightJson.contains("vertex"))
            {
                continue;
            }
            auto vertexJson = lightJson["vertex"];

            lightconfig.u = glm::vec3(vertexJson[0][0], vertexJson[0][1], vertexJson[0][2]);
            lightconfig.v = glm::vec3(vertexJson[1][0], vertexJson[1][1], vertexJson[1][2]);
        }

        getValue(lightJson, "position", vecTmp);
        lightconfig.position = glm::vec4(vecTmp[0], vecTmp[1], vecTmp[2], vecTmp[3]);

        getValue(lightJson, "direction", vecTmp);
        lightconfig.direction = glm::vec4(vecTmp[0], vecTmp[1], vecTmp[2], vecTmp[3]);

        getValue(lightJson, "color", vecTmp);
        lightconfig.color = glm::vec4(vecTmp[0], vecTmp[1], vecTmp[2], vecTmp[3]);

        lightconfig.active = lightJson.value("active", false);
        lightconfig.range = lightJson.value("range", 1.0f);
        lightconfig.outerCosine = lightJson.value("outerCosine", 1.0f);
        lightconfig.innerCosine = lightJson.value("innerCosine", 1.0f);

        config.lightConfigs.push_back(lightconfig);
    }

    if(envMapJson.contains("envMap"))
    {
        config.envMapConfig.envMapPath = envMapJson["envMap"];
    }

    for(const auto& matJson : matsJson.items())
    {
        MaterialConfig matConfig;
        const auto& matJsonVal = matJson.value();
        if(matJsonVal.contains("path"))
        {
            matConfig.bNotLoad = false;
            matConfig.matPath = matJsonVal.value("path", "");
        }
        else
        {
            matConfig.bNotLoad = true;
            getValue(matJsonVal, "color", vecTmp);
            matConfig.mat.baseColor = glm::vec3(vecTmp[0], vecTmp[1], vecTmp[2]);
            // todo: other attributes
        }

        config.matConfigMap[matJson.key()] = matConfig;
    }
}

bool DumpConfig(const std::string &configFile, SceneConfig &config)
{
    return false;
}
