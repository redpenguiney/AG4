#include "gui.hpp"
#include "rendergraph_node.hpp"
#include "shader_program.hpp"

static std::shared_ptr<GuiContainer> ScreenGui() {
    auto pass = std::make_shared<DrawPass>();
    pass->outputs.push_back(WINDOW_RESOURCE_NAME);
    pass->name = "SCREEN_GUI_PRESENTATION";
    pass->renderTarget = WindowRenderTargetDescriptor{.loadPolicy = AttachmentLoadPolicy::Load, .clearDepth = false};
    pass->params.shader = ShaderProgram::New("gui_vertex.glsl", "gui_fragment.glsl");
    
    auto container = std::make_shared<GuiContainer>();
    container->pass = pass;

    return container;
}

std::shared_ptr<GuiContainer> GetScreenGuiContainer()
{
    static auto container = ScreenGui();
    return container;
}

std::shared_ptr<GuiElement> GuiElement::New(GuiContainer& storeIn)
{
    auto ptr = std::shared_ptr<GuiElement>(new GuiElement(storeIn));
    storeIn.contents.push_back(ptr);
    return ptr;
}

GuiElement::GuiElement(GuiContainer& storeIn) {

}
