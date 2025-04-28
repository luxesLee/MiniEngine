#pragma once
#include <optional>
#include <array>
#include "glad/glad.h"
#include "util/Types.h"

enum class CompareOp : GLenum
{
    Never = GL_NEVER,
    Less = GL_LESS,
    Equal = GL_EQUAL,
    LessOrEqual = GL_LEQUAL,
    Greater = GL_GREATER,
    NotEqual = GL_NOTEQUAL,
    GreaterOrEqual = GL_GEQUAL,
    Always = GL_ALWAYS
};
struct DepthStencilState
{
    Bool depthTest{false};
    Bool depthWrite{true};
    CompareOp depthCompOp{CompareOp::LessOrEqual};
};

enum class BlendOp : GLenum
{
    Add = GL_FUNC_ADD,
    Subtract = GL_FUNC_SUBTRACT,
    ReverseSubtract = GL_FUNC_REVERSE_SUBTRACT,
    Min = GL_MIN,
    Max = GL_MAX
};
enum class BlendFactor : GLenum
{
    Zero = GL_ZERO,
    One = GL_ONE,
    SrcColor = GL_SRC_COLOR,
    OneMinusSrcColor = GL_ONE_MINUS_SRC_COLOR,
    DstColor = GL_DST_COLOR,
    OneMinusDstColor = GL_ONE_MINUS_DST_COLOR,
    SrcAlpha = GL_SRC_ALPHA,
    OneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA,
    DstAlpha = GL_DST_ALPHA,
    OneMinusDstAlpha = GL_ONE_MINUS_DST_ALPHA,
    ConstantColor = GL_CONSTANT_COLOR,
    OneMinusConstantColor = GL_ONE_MINUS_CONSTANT_COLOR,
    ConstantAlpha = GL_CONSTANT_ALPHA,
    OneMinusConstantAlpha = GL_ONE_MINUS_CONSTANT_ALPHA,
    SrcAlphaSaturate = GL_SRC_ALPHA_SATURATE,
    Src1Color = GL_SRC1_COLOR,
    OneMinusSrc1Color = GL_ONE_MINUS_SRC1_COLOR,
    Src1Alpha = GL_SRC1_ALPHA,
    OneMinusSrc1Alpha = GL_ONE_MINUS_SRC1_ALPHA
};
struct BlendState
{
    Bool bEnabled{false};

    BlendFactor srcColor{BlendFactor::One};
    BlendFactor desColor{BlendFactor::Zero};
    BlendOp     colorOp{BlendOp::Add};

    BlendFactor srcAlpha{BlendFactor::One};
    BlendFactor desAlpha{BlendFactor::Zero};
    BlendOp     alphaOp{BlendOp::Add};
};

enum class PolygonMode : GLenum
{
    Point = GL_POINT,
    Line = GL_LINE,
    Fill = GL_FILL
};
enum class CullMode : GLenum
{
    None = GL_NONE,
    Back = GL_BACK,
    Front = GL_FRONT
};
struct PolygonOffset
{
    float factor{0.0};
    float units{0.0f};
};
struct RasterizerState
{
    RasterizerState() {}

    RasterizerState(PolygonMode _polygonMode, CullMode _cullMode, Bool _bDepthClamp, Bool _bScissor)
        : polygonMode{_polygonMode}, cullMode{_cullMode}, bDepthClamp{_bDepthClamp}, bScissor{_bScissor}
    {
    }

    PolygonMode polygonMode{PolygonMode::Fill};
    CullMode cullMode{CullMode::Back};
    std::optional<PolygonOffset> polygonOffset;
    Bool bDepthClamp{false};
    Bool bScissor{false};
};

constexpr auto kMaxNumBlendStates = 4;
class PipelineStateObject
{

public:
    PipelineStateObject() = default;
    ~PipelineStateObject() {}

    PipelineStateObject(const PipelineStateObject&) = delete;
    PipelineStateObject& operator=(const PipelineStateObject&) = delete;

    PipelineStateObject(PipelineStateObject&& other)
        : program{other.program},
          depthStencilState{std::move(other.depthStencilState)},
          rasterState{std::move(other.rasterState)},
          blendStates{std::move(other.blendStates)}
    {
        memset(&other, 0, sizeof(PipelineStateObject));
    }
    // PipelineStateObject& operator=(PipelineStateObject&& rhs)
    // {

    // }

    void setState();

    class PSOBuilder
    {
    public:
        PSOBuilder() {}
        ~PSOBuilder() {}

        PSOBuilder& setShaderProgram(GLuint program);
        PSOBuilder& setDepthStencil(const DepthStencilState& depthStencilState);
        PSOBuilder& setRasterState(const RasterizerState& rasterState);
        PSOBuilder& setBlendState(Uint attachNum, const BlendState& blendState);

        PipelineStateObject build();

    private:
        GLuint program{GL_NONE};

        DepthStencilState depthStencilState;
        RasterizerState rasterState;
        std::array<BlendState, kMaxNumBlendStates> blendStates;
    };

private:

    GLuint program{GL_NONE};

    DepthStencilState depthStencilState;
    RasterizerState rasterState;
    std::array<BlendState, kMaxNumBlendStates> blendStates;
};
