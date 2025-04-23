#version 450 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 Normal[];
layout(location = 1) in vec2 TexCoords[];
layout(location = 2) flat in int MatID[];

out GS_OUT {
    vec3 VoxelPosition;
    vec3 Normal;
    vec2 TexCoords;
    flat int MatID;
} gs_out;

uniform int VoxelSize;
uniform mat4 project;

// World
//Y|  /-Z
// | /
// |/
//O — — — —> X 
vec4 GetProjectPoint(vec4 pos, int axis)
{
    if(axis == 0)
    {
        // X --> -Z
        // Y --> Y
        // Z --> X
        pos.z = pos.z;
        return pos.zyxw;
    }
    else if(axis == 1)
    {
        // X --> X
        // Y --> -Z
        // Z --> Y
        pos.z = pos.z;
        return pos.xzyw;
    }
    else if(axis == 2)
    {
        // X --> X
        // Y --> Y
        // Z --> Z
        return pos.xyzw;
    }
    return pos;
}

void main()
{
    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;

    // 确定当前三角形应该在X、Y、Z哪个面上投影
    // 叉乘得到三角形面的垂线，分量越长对应着投影信息越多
    vec3 norm = abs(cross(p1 - p0, p2 - p0));
    int majorAxis = 0;
    if(norm.y >= norm.x && norm.y >= norm.z)
    {
        majorAxis = 1;
    }
    else if(norm.z >= norm.x && norm.z >= norm.y)
    {
        majorAxis = 2;
    }

    for(int i = 0; i < 3; i++)
    {
        // 相当于直接将点投影到Voxel立方体（视锥）中，因而此处无需计算投影轴
        vec4 ndc = project * gl_in[i].gl_Position;
        gs_out.VoxelPosition = (ndc.xyz + vec3(1)) * 0.5f * VoxelSize;

        // 更改
        gl_Position = project * GetProjectPoint(gl_in[i].gl_Position, majorAxis);

        gs_out.Normal = Normal[i];
        gs_out.TexCoords = TexCoords[i];
        gs_out.MatID = MatID[i];
        EmitVertex();
    }

    EndPrimitive();
}
