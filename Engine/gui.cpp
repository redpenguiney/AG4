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
    pass->renderTarget = WindowRenderTargetDescriptor{.loadPolicy = AttachmentLoadPolicy::Load, .clearDepth = false};
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
    glm::vec2 screenResolution = { window.width, window.height };
    glm::ivec2 anchorPointPixelPosition = glm::ivec2(screenResolution * percentagePosition) + pixelPosition;
    glm::ivec2 finalPixelSize = glm::ivec2(screenResolution * percentageSize) + pixelSize;

    glm::ivec2 anchorPointOffset = anchorPoint * glm::vec2(finalPixelSize);
    glm::ivec2 objectCenterPosition = anchorPointPixelPosition - anchorPointOffset;

    gameobject->SetPosition(glm::dvec3(objectCenterPosition, depth));
    gameobject->SetScale(glm::dvec3(finalPixelSize, 1.0));
    gameobject->SetRotation(glm::angleAxis(rotation, glm::vec3(0, 0, 1)));

    for (auto& child : children) {
        child->RefreshTransform();
    }
}

void GuiElement::RefreshGraphics() {
    gameobject->SetInstanceAttribute(*Mesh::GuiQuad()->format.GetAttribute("color"), backgroundColor);
    //gameobject->SetInstanceAttribute(*Mesh::GuiQuad()->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);
}

void GuiElement::RefreshText() {

}

void GuiElement::SetParent(GuiElement* newParent) {
    Orphan();
    parent = newParent;
    parent->children.push_back(shared_from_this());
}

void GuiElement::MakeTextobject() {

}

GuiElement::GuiElement(GuiContainer& storeIn, std::shared_ptr<Texture> background, std::shared_ptr<Texture> font):
texture(background),
font(font),
container(&storeIn)
{
    if (font) {
        MakeTextobject();
    }
    
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
