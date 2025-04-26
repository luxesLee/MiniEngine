#version 430 core
#extension GL_ARB_shader_image_load_store : require

layout(rgba8, binding = 0) uniform readonly image3D visualVoxel;

// Uniform
uniform int VoxelSize;

// In Out
out vec4 color;

void main()
{
    float fVoxelSize = float(VoxelSize);

    vec3 position = vec3(gl_VertexID % VoxelSize,
                        (gl_VertexID / VoxelSize) % VoxelSize,
                        gl_VertexID / (VoxelSize * VoxelSize));

    ivec3 voxelPos = ivec3(position);
    color = imageLoad(visualVoxel, voxelPos);

    gl_Position = vec4(position, 1.0f);
}
