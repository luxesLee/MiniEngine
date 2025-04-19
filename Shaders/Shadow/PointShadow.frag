#version 430 core
in vec4 FragPos;

uniform vec3 lightPos;
// uniform float far_plane;

void main()
{
    float lightDistance = length(FragPos.xyz - lightPos);
    
    lightDistance = lightDistance / 5.0f;

    gl_FragDepth = lightDistance;
}