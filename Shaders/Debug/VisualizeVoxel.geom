#version 430 core

layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

layout(binding = 0) uniform CommonUBO
{
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 projectionView;
    mat4 invProjection;
    mat4 invViewProjection;
    vec4 screenAndInvScreen;
    vec4 cameraPosition;
    int lightNum;
};

// Uniform
uniform vec4 frustumPlanes[6];
uniform mat4 voxelProjection;
uniform mat4 voxelInvProjection;
uniform int VoxelSize;
uniform float CellSize;

// In Out
in vec4 color[];
out vec4 voxelColor;

bool VoxelInFrustum(vec3 center, vec3 extent)
{
    for(int i = 0; i < 6; i++)
    {
        // 体素沿每个坐标轴与平面法向的投影长度的绝对值之和，表示体素在当前平面法向上最大扩展长度
        float d = dot(extent, abs(frustumPlanes[i].xyz));
        // 体素中心相对于平面的位置，带符号
        float r = dot(center, frustumPlanes[i].xyz) + frustumPlanes[i].w;
        if(d + r > 0 == false)
        {
            return false;
        }
    }
    return true;
}

vec3 VoxelToWorld(vec3 voxelPos)
{
    vec3 ndc = (voxelPos / float(VoxelSize) * 2.0 - 1.0);
    return (voxelInvProjection * vec4(ndc, 1.0f)).xyz;
}

void main()
{
	const vec4 cubeVertices[8] = vec4[8] 
	(
		vec4( 0.5f,  0.5f,  0.5f, 0.0f),
		vec4( 0.5f,  0.5f, -0.5f, 0.0f),
		vec4( 0.5f, -0.5f,  0.5f, 0.0f),
		vec4( 0.5f, -0.5f, -0.5f, 0.0f),
		vec4(-0.5f,  0.5f,  0.5f, 0.0f),
		vec4(-0.5f,  0.5f, -0.5f, 0.0f),
		vec4(-0.5f, -0.5f,  0.5f, 0.0f),
		vec4(-0.5f, -0.5f, -0.5f, 0.0f)
	);

	const int cubeIndices[24]  = int[24] 
	(
		0, 2, 1, 3, // right
		6, 4, 7, 5, // left
		5, 4, 1, 0, // up
		6, 7, 2, 3, // down
		4, 6, 0, 2, // front
		1, 3, 5, 7  // back
	);

    if(color[0].a == 0)
    {
        return;
    }

    vec3 center = VoxelToWorld(gl_in[0].gl_Position.xyz);
    vec3 extent = vec3(CellSize);
    if(!VoxelInFrustum(center, extent))
    {
        return;
    }

    vec4 projectedVertices[8];
    for(int i = 0; i < 8; i++)
    {
        vec4 vertex = gl_in[0].gl_Position + cubeVertices[i];
        projectedVertices[i] = (projectionView * vec4(VoxelToWorld(vertex.xyz), 1.0f));
    }

    for(int face = 0; face < 6; face++)
    {
        for(int vertex = 0; vertex < 4; vertex++)
        {
            gl_Position = projectedVertices[cubeIndices[face * 4 + vertex]];
            voxelColor = color[0];
            EmitVertex();
        }
        EndPrimitive();
    }
}