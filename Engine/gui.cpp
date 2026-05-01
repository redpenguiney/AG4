#include "gui.hpp"
#include "rendergraph_node.hpp"
#include "shader_program.hpp"
#include "gameobject.hpp"
#include "mesh.hpp"
#include <string>
#include "assert.hpp"
#include "window.hpp"

static void SupplyTextUniforms(std::shared_ptr<BaseShaderProgram> shader) {
    shader->Uniform("fontMappingEnabled", true);
}

static std::shared_ptr<GuiContainer> ScreenGui() {
    auto pass = std::shared_ptr<DrawPass>(new DrawPass());
    pass->outputs.push_back(WINDOW_RESOURCE_NAME);
    pass->outputs.push_back("FRAMES_DRAWN");
    pass->dependencies.push_back("POST_PROC");
    pass->name = "SCREEN_GUI_PRESENTATION_UNTEXTURED";
    pass->params.cullMode = FaceCulling::Backface;
    pass->renderTarget = WindowRenderTargetDescriptor{
        .loadPolicy = AttachmentLoadPolicy::Load, 
        .clearDepth = false, 
        //.blendingSrcFactor = BlendFactorMode::OneMinusSrcAlpha,
        //.blendingDstFactor = BlendFactorMode::SrcAlpha,
        //.blendFunc = BlendingEquation::Addition,
    };
    //pass->uniformSupplier = std::nullopt;
    //pass->params.depthTestMode = DepthTestMode::Disabled;
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

void GuiElement::InitGuiEvents() {

    static auto connection1 = Window::Get().onWindowResize.Connect([](Window*, glm::uvec2, glm::uvec2) {
        for (auto& ui : elementsList) {
            if (ui->font) ui->RefreshText();
            else ui->RefreshTransform();
        }
        });

    static auto connection2 = Window::Get().postInputProccessing.Connect([](Window* w) {
        glm::ivec2 cursorPos(w->MOUSE_POS);
        cursorPos.y = w->height - cursorPos.y;
        for (auto& ui : elementsList) {
            auto uiPos = ui->GetPixelCenterPosition();
            auto uiSize = ui->GetPixelSize();
            bool isHovering = cursorPos.x > uiPos.x - uiSize.x / 2 && cursorPos.y > uiPos.y - uiSize.y / 2 && cursorPos.x < uiPos.x + uiSize.x / 2 && cursorPos.y < uiPos.y + uiSize.y / 2;
            if (isHovering && !ui->hover) {
                elementsBeingHoveredOn.push_back(ui);
                ui->hover = true;
                onHoverBegin.Fire(ui);
            }
            else if (!isHovering && ui->hover) {
                for (unsigned i = 0; i < elementsBeingHoveredOn.size(); i++) {
                    if (elementsBeingHoveredOn[i] == ui) {
                        elementsBeingHoveredOn[i] = elementsBeingHoveredOn.back();
                        elementsBeingHoveredOn.pop_back();
                        break;
                    }
                }
                ui->hover = false;
                onHoverEnd.Fire(ui);
            }
        }

        for (auto& p : w->PRESS_BEGAN_KEYS) {
            for (auto& ui : elementsBeingHoveredOn) {
                onInputBegin.Fire(ui, p);
            }
        }
        for (auto& p : w->PRESS_ENDED_KEYS) {
            for (auto& ui : elementsBeingHoveredOn) {
                onInputEnd.Fire(ui, p);
            }
        }
    });
}

std::shared_ptr<GuiElement> GuiElement::New(GuiContainer& storeIn, std::shared_ptr<Texture> background, std::shared_ptr<Texture> font)
{
    auto ptr = std::shared_ptr<GuiElement>(new GuiElement(storeIn, background, font));
    //storeIn.contents.push_back(ptr.get());
    return ptr;
}

GuiElement::~GuiElement() {
    for (unsigned i = 0; i < elementsList.size(); i++) {
        if (elementsList[i] == this) {
            elementsList[i] = elementsList.back();
            elementsList.pop_back();
        }
    }

    if (hover) {
        for (unsigned i = 0; i < elementsBeingHoveredOn.size(); i++) {
            if (elementsBeingHoveredOn[i] == this) {
                elementsBeingHoveredOn[i] = elementsBeingHoveredOn.back();
                elementsBeingHoveredOn.pop_back();
            }
        }
    }
}

void GuiElement::RefreshTransform() {
    auto& window = Window::Get();
    glm::ivec2 objectCenterPosition = GetPixelCenterPosition();
    glm::ivec2 objectSize = GetPixelSize();

    gameobject->SetPosition(glm::dvec3(objectCenterPosition, depth));
    gameobject->SetScale(glm::dvec3(objectSize, 1.0));
    gameobject->SetRotation(glm::angleAxis(rotation, glm::vec3(0, 0, 1)));

    if (textobject) {
        //DebugLogInfo("Text placed at ", objectCenterPosition);
        textobject->SetPosition(glm::dvec3(objectCenterPosition, depth - 0.001));
        textobject->SetRotation(glm::angleAxis(rotation, glm::vec3(0, 0, 1)));
    }

    for (auto& child : children) {
        child->RefreshTransform();
    }
}

void GuiElement::RefreshGraphics() {
    gameobject->SetInstanceAttribute(*Mesh::GuiQuad()->format.GetAttribute("color"), backgroundColor);
    gameobject->SetInstanceAttribute(*Mesh::GuiQuad()->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);
    if (textobject) {
        textobject->SetInstanceAttribute(*Mesh::GuiQuad()->format.GetAttribute("color"), textColor);
        textobject->SetInstanceAttribute(*Mesh::GuiQuad()->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), 0.0f);
    }
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
    glm::vec2 screenResolution = GetParentBounds();
    glm::ivec2 finalPixelSize = glm::ivec2(screenResolution * percentageSize) + pixelSize;
    return finalPixelSize;
}

glm::ivec2 GuiElement::GetPixelCenterPosition() {
    glm::vec2 screenResolution = GetParentBounds();
    glm::ivec2 anchorPointPixelPosition = glm::ivec2(screenResolution * percentagePosition) + pixelPosition;
    glm::ivec2 anchorPointOffset = anchorPoint * glm::vec2(GetPixelSize());
    glm::ivec2 objectCenterPosition = anchorPointPixelPosition - anchorPointOffset;
    return objectCenterPosition + GetParentOffset();
}

glm::ivec2 GuiElement::GetParentBounds() { 
    if (parent) {
        return parent->GetPixelSize();
    }
    else {
        return glm::ivec2(Window::Get().width, Window::Get().height);
    }
}

glm::ivec2 GuiElement::GetParentOffset() {
    if (parent) {
        return parent->GetPixelCenterPosition() - parent->GetPixelSize() / 2;
    }
    else {
        return glm::ivec2(0, 0);
    }
}

void GuiElement::LayoutChildrenList(ListLayout params) {
    // sort children
    std::sort(children.begin(), children.end(), [](const std::shared_ptr<GuiElement>& a, const std::shared_ptr<GuiElement>& b) -> bool { return a->sortOrder < b->sortOrder; });

    glm::ivec2 pixelPos = params.pixelStart;
    glm::vec2 percentPos = params.percentStart;
    for (auto& c : children) {
        c->pixelPosition = pixelPos;
        pixelPos += params.pixelStride;
        c->percentagePosition += percentPos;
        percentPos += params.percentStride;
        c->RefreshTransform();
    }
}

void GuiElement::MakeTextobject() {
    Assert(font);
    MeshCreateParams textmeshparams = MeshCreateParams::DefaultText();
    TextFormatting format;
    format.horizontalAlignment = hAlign;
    format.verticalAlignment = vAlign;
    auto size = GetPixelSize();
    auto pos = GetPixelCenterPosition();
    format.leftMargin = -size.x / 2;
    format.rightMargin = size.x / 2;
    format.bottomMargin = -size.y / 2;
    format.topMargin = size.y / 2;
    format.wrapping = wrapText;

    //DebugLogInfo("Size ", size, " margins ", format.topMargin, " ", format.bottomMargin);

    textmeshparams.LoadText(*font, text, format);
    textmeshparams.normalizeSize = false;
    auto textmesh = Mesh::New(std::move(textmeshparams));
    GameobjectCreateParams textobjectparams;
    textobjectparams.mesh = textmesh;
    if (!container->elementPasses.contains(font.get())) {
        Assert(font);
        static int i = 0;
        std::shared_ptr<DrawPass> newPass(new DrawPass(*container->elementPasses[nullptr]));
        newPass->name = "SCREEN_GUI_PRESENTATION_FONT " + std::to_string(i++);
        newPass->dependencies.push_back("FRAMES_DRAWN");
        newPass->outputs.pop_back();
        newPass->uniformSupplier = SupplyTextUniforms;
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
    elementsList.push_back(this);

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

std::shared_ptr<TextboxElement> TextboxElement::New(GuiContainer& storeIn, std::shared_ptr<Texture> background, std::shared_ptr<Texture> font)
{
    auto ptr = std::shared_ptr<TextboxElement>(new TextboxElement(storeIn, background, font));
    return ptr;
}

TextboxElement::TextboxElement(GuiContainer& storeIn, std::shared_ptr<Texture> background, std::shared_ptr<Texture> font)
: GuiElement(storeIn, background, font) {

}
