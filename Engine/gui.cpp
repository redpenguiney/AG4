#include "gui.hpp"
#include "rendergraph_node.hpp"
#include "shader_program.hpp"
#include "gameobject.hpp"
#include "mesh.hpp"
#include <string>
#include "assert.hpp"
#include "window.hpp"

static std::shared_ptr<GuiContainer> ScreenGui() {
    auto pass = std::shared_ptr<DrawPass>(new DrawPass());
    pass->outputs.push_back(WINDOW_RESOURCE_NAME);
    pass->dependencies.push_back("POST_PROC");
    pass->name = "SCREEN_GUI_PRESENTATION_UNTEXTURED";
    pass->params.cullMode = FaceCulling::None;
    pass->renderTarget = WindowRenderTargetDescriptor{
        .loadPolicy = AttachmentLoadPolicy::Load, 
        .clearDepth = false, 
        .blendingSrcFactor = BlendFactorMode::OneMinusSrcAlpha,
        .blendingDstFactor = BlendFactorMode::SrcAlpha,
        .blendFunc = BlendingEquation::Addition,
    };
    pass->params.depthTestMode = DepthTestMode::Disabled;
    pass->params.shader = ShaderProgram::New("../shaders/gui_vertex.glsl", "../shaders/gui_fragment.glsl");
    
    auto container = std::make_shared<GuiContainer>();
    container->elementPasses[nullptr] = pass;
    GuiContainer::Containers().push_back(container);
    return container;
}

std::shared_ptr<GuiContainer> GetScreenGuiContainer()
{
    static auto container = ScreenGui();
    return container;
}

std::shared_ptr<GuiElement> GuiElement::New(GuiContainer& storeIn, std::shared_ptr<Texture> background, std::shared_ptr<Texture> font)
{
    auto ptr = std::shared_ptr<GuiElement>(new GuiElement(storeIn, background, font));
    storeIn.contents.push_back(ptr);
    return ptr;
}

GuiElement::~GuiElement() {

}

void GuiElement::RefreshTransform() {
    auto& window = Window::Get();
    glm::ivec2 objectCenterPosition = GetPixelCenterPosition();
    glm::ivec2 objectSize = GetPixelSize();

    gameobject->SetPosition(glm::dvec3(objectCenterPosition, depth));
    gameobject->SetScale(glm::dvec3(objectSize, 1.0));
    gameobject->SetRotation(glm::angleAxis(rotation, glm::vec3(0, 0, 1)));

    if (textobject) {
        textobject->SetPosition(glm::dvec3(objectCenterPosition, depth + 0.001));
        textobject->SetRotation(glm::angleAxis(rotation, glm::vec3(0, 0, 1)));
    }

    for (auto& child : children) {
        child->RefreshTransform();
    }
}

void GuiElement::RefreshGraphics() {
    gameobject->SetInstanceAttribute(*Mesh::GuiQuad()->format.GetAttribute("color"), backgroundColor);
    if (textobject) {
        textobject->SetInstanceAttribute(*Mesh::GuiQuad()->format.GetAttribute("color"), textColor);
    }
    //gameobject->SetInstanceAttribute(*Mesh::GuiQuad()->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);
}

void GuiElement::RefreshText() {
    Assert(font);
    textobject = nullptr;
    MakeTextobject();
}

void GuiElement::SetParent(GuiElement* newParent) {
    Orphan();
    parent = newParent;
    parent->children.push_back(shared_from_this());
}

glm::ivec2 GuiElement::GetPixelSize() {
    auto& window = Window::Get();
    glm::vec2 screenResolution = { window.width, window.height };
    glm::ivec2 finalPixelSize = glm::ivec2(screenResolution * percentageSize) + pixelSize;
    return finalPixelSize;
}

glm::ivec2 GuiElement::GetPixelCenterPosition() {
    auto& window = Window::Get();
    glm::vec2 screenResolution = { window.width, window.height };
    glm::ivec2 anchorPointPixelPosition = glm::ivec2(screenResolution * percentagePosition) + pixelPosition;
    glm::ivec2 anchorPointOffset = anchorPoint * glm::vec2(GetPixelSize());
    glm::ivec2 objectCenterPosition = anchorPointPixelPosition - anchorPointOffset;
    return objectCenterPosition;
}

void GuiElement::MakeTextobject() {
    Assert(font);
    MeshCreateParams textmeshparams = MeshCreateParams::DefaultText();
    TextFormatting format;
    format.horizontalAlignment = hAlign;
    format.verticalAlignment = vAlign;
    auto size = GetPixelSize();
    auto pos = GetPixelCenterPosition();
    format.leftMargin = pos.x - size.x / 2;
    format.rightMargin = pos.x + size.x / 2;
    format.bottomMargin = pos.y - size.y / 2;
    format.topMargin = pos.y + size.y / 2;
    format.wrapping = wrapText;
    textmeshparams.LoadText(*font, text, format);
    auto textmesh = Mesh::New(std::move(textmeshparams));
    GameobjectCreateParams textobjectparams;
    textobjectparams.mesh = textmesh;
    if (!container->elementPasses.contains(font.get())) {
        Assert(font);
        static int i = 0;
        std::shared_ptr<DrawPass> newPass(new DrawPass(*container->elementPasses[nullptr]));
        newPass->name = "SCREEN_GUI_PRESENTATION_FONT " + std::to_string(i++);
        newPass->boundTextures.push_back(TextureUsageDescriptor{
            .texture = font,
            .textureUsageLocation = "fontMap",
            .willRead = true
            });
        container->elementPasses.emplace(font.get(), newPass);
    }
    textobjectparams.renderPasses = { container->elementPasses[font.get()], };
    textobject = std::unique_ptr<Gameobject>(Gameobject::New(textobjectparams));
    RefreshGraphics();
    RefreshTransform();
}

GuiElement::GuiElement(GuiContainer& storeIn, std::shared_ptr<Texture> background, std::shared_ptr<Texture> font):
texture(background),
font(font),
container(&storeIn)
{
    
    GameobjectCreateParams params;
    params.mesh = Mesh::GuiQuad();
    if (!storeIn.elementPasses.contains(background.get())) {
        Assert(background);
        static int i = 0;
        std::shared_ptr<DrawPass> newPass(new DrawPass(*storeIn.elementPasses[nullptr]));
        newPass->name = "SCREEN_GUI_PRESENTATION_TEXTURED " + std::to_string(i++);
        container->elementPasses.emplace(background.get(), newPass);
    }
    params.renderPasses = { storeIn.elementPasses[background.get()],};
    params.primitiveType = GL_TRIANGLES;
    gameobject = std::unique_ptr<Gameobject>(Gameobject::New(params));

    RefreshGraphics();
    RefreshTransform();

    if (font) {
        MakeTextobject();
    }
}

void GuiElement::Orphan() {
    if (parent) {
        for (unsigned i = 0; i < parent->children.size(); i++) {
            if (parent->children[i].get() == this) {
                parent->children[i] = parent->children.back();
                parent->children.pop_back();
                break;
            }
        }
        parent = nullptr;
    }
}

std::vector<std::shared_ptr<GuiContainer>>& GuiContainer::Containers() {
    static std::vector<std::shared_ptr<GuiContainer>> c;
    return c;
}
