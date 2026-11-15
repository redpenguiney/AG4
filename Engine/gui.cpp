#include "gui.hpp"
#include "rendergraph_node.hpp"
#include "shader_program.hpp"
#include "gameobject.hpp"
#include "mesh.hpp"
#include <string>
#include "assert.hpp"
#include "window.hpp"

static constexpr int MAX_STENCIL_DEPTH = 8;

static void SupplyTextUniforms(std::shared_ptr<BaseShaderProgram> shader) {
    shader->Uniform("fontMappingEnabled", true);
}

// not static bc friend (well actually bc mingw says so)
std::shared_ptr<GuiContainer> ScreenGui() {
    // declare attachments for rendering screengui
    GraphicsEngine::Get().AddAttachment(FramebufferAttachmentFormatDescriptor{
        .resourceName = "SCREEN_GUI_COLOR",
        .format = Texture::RGBA_8Bit,
        .windowSize = {1, 1}
        });
    GraphicsEngine::Get().AddAttachment(FramebufferAttachmentFormatDescriptor{
        .resourceName = "SCREEN_GUI_DEPTH_STENCIL",
        .format = Texture::DEPTH24_STENCIL8,
        .windowSize = {1, 1}
        });

    // setup drawpass for blitting screengui to window
    auto blitPass = std::make_shared<DrawPass>();
    blitPass->name = "blit_screengui";
    auto rt = WindowRenderTargetDescriptor();
    rt.loadPolicy = AttachmentLoadPolicy::Load;
    rt.clearDepth = false;
    blitPass->renderTarget = rt;
    blitPass->dependencies.push_back("POST_PROC");
    blitPass->dependencies.push_back("SCREEN_GUI_COLOR");
    blitPass->outputs.push_back("FRAMES_DRAWN");
    blitPass->boundTextures.push_back(TextureUsageDescriptor{
        .texture = "SCREEN_GUI_COLOR",
        .textureUsageLocation = "guiTextureColor"
        });
    blitPass->outputs.push_back(WINDOW_RESOURCE_NAME);
    blitPass->params.depthTestMode = DepthTestMode::Disabled;
    blitPass->params.writeDepthBuffer = false;
    blitPass->params.cullMode = FaceCulling::Backface;
    blitPass->params.shader = ShaderProgram::New("../shaders/gui_postproc_vertex.glsl", "../shaders/gui_postproc_fragment.glsl");
    blitPass->params.blending = true;

    // setup quad used to do so
    GameobjectCreateParams quadParams;
    quadParams.mesh = Mesh::Quad();
    quadParams.renderPasses = { blitPass, };
    static std::shared_ptr<Gameobject> blitQuad(Gameobject::New(quadParams));

    // drawpass for clearing gui buffer
    auto clearPass = std::make_shared<DrawPass>();
    clearPass->name = "clear_gui";
    clearPass->outputs.push_back("GUI_BUFF_CLEAR");
    clearPass->outputs.push_back("GUI_STENCIL_READY");
    clearPass->outputs.push_back("SCREEN_GUI_COLOR");
    clearPass->outputs.push_back("SCREEN_GUI_DEPTH_STENCIL");
    clearPass->renderTarget = FramebufferRenderTargetDescriptor{
        .colorAttachments = {
            FramebufferAttachmentUsageDescriptor {
                .attachmentName = "SCREEN_GUI_COLOR",
                .loadPolicy = AttachmentLoadPolicy::Clear,
            },
        },
        .depthStencilAttachment = FramebufferAttachmentUsageDescriptor {
            .attachmentName = "SCREEN_GUI_DEPTH_STENCIL",
            .loadPolicy = AttachmentLoadPolicy::Clear,
            .clearColor = {1, 0, 0, 0}
        }
    };
    for (int i = 0; i < MAX_STENCIL_DEPTH; i++) {
        clearPass->outputs.push_back("GUI_STENCIL_" + std::to_string(i) + "_WRITTEN_BIT");
    }

    GraphicsEngine::Get().ForceAddDrawPass(clearPass);

    // create default drawpass for gui elements
    auto pass = std::shared_ptr<DrawPass>(new DrawPass());
    pass->dependencies.push_back("GUI_BUFF_CLEAR");
    pass->dependencies.push_back("GUI_STENCIL_READY");
    pass->outputs.push_back("SCREEN_GUI_COLOR");
    pass->outputs.push_back("SCREEN_GUI_DEPTH_STENCIL");
    pass->outputs.push_back("GUI_STENCIL_" + std::to_string(0) + "_WRITTEN_BIT");
    pass->name = "SCREEN_GUI_PRESENTATION_UNTEXTURED";
    pass->params.cullMode = FaceCulling::Backface;
    pass->renderTarget = FramebufferRenderTargetDescriptor {
        .colorAttachments = {
            FramebufferAttachmentUsageDescriptor {
                .attachmentName = "SCREEN_GUI_COLOR",
                .loadPolicy = AttachmentLoadPolicy::Load
            },
        },
        .depthStencilAttachment = FramebufferAttachmentUsageDescriptor {
            .attachmentName = "SCREEN_GUI_DEPTH_STENCIL",
            .loadPolicy = AttachmentLoadPolicy::Load
        }
    };
    pass->params.stencilTestComparisonValue = 0xFF;
    pass->params.stencilTestMode = DepthTestMode::LEqual;
    pass->params.stencilTestSuccessDepthTestSuccessAction = StencilOperation::Set;
    pass->params.stencilTestMask.reset();
    pass->params.stencilWriteMask.reset().set(0, true);
    //pass->params.stencilWriteMask = 0xFF;
    //pass->uniformSupplier = std::nullopt;
    //pass->params.depthTestMode = DepthTestMode::Disabled;
    pass->params.shader = ShaderProgram::New("../shaders/gui_vertex.glsl", "../shaders/gui_fragment.glsl");
    
    pass->params.blending = true;

    auto container = std::make_shared<GuiContainer>();
    container->elementPasses[GuiPassRequirements(nullptr, 0)] = pass;
    GuiContainer::Containers().push_back(container);
    return container;
}

std::shared_ptr<GuiContainer> GetScreenGuiContainer()
{
    static auto container = ScreenGui();
    return container;
}

void GuiElement::InitGui() {
    // setup connections
    static auto connection1 = Window::Get().onWindowResize.Connect([](Window*, glm::uvec2 old, glm::uvec2 neww) {
        DebugLogInfo("Resizing window from ", old, " ", neww, ": updating ui");
        for (auto& ui : elementsList) {
            if (ui->font) ui->RefreshText();
        }

        for (auto& ui : elementsList) {
            if (ui->parent == nullptr) ui->RefreshTransform();
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

        bool leftClick = false;

        for (auto& p : w->PRESS_BEGAN_EVENTS) {
            if (p.input == InputType::LMB) leftClick = true;
        }

        if (leftClick) {
            //DebugLogInfo("LMB");
            for (auto& e : elementsBeingHoveredOn) {
                //DebugLogInfo("ITS ", e->text);
                if (auto textbox = dynamic_cast<TextboxElement*>(e)) {
                    if (currentlyFocused == textbox) {
                        goto textboxFocusDone;
                    }
                    else {
                        textbox->Focus();
                        goto textboxFocusDone;
                    }
                }
            }
            currentlyFocused->Unfocus();
        }
        textboxFocusDone:;

    });

    static auto inputContext = InputStack::Get().PushContext(InputStack::GUI_INPUT_PRIORITY);
    static auto connection3 = InputStack::onInput.Connect(inputContext.get(), [](InputContext* ctx, InputObject input) {
        for (auto& ui : elementsBeingHoveredOn) {
            onInput.Fire(ui, input);
        }

        if (currentlyFocused) {
            if (input.state == InputState::Up) return;
            bool mod = false;

            if (auto s = InputToString(input)) {
                currentlyFocused->text += s.value();
                mod = true;
            }
            else if (input.input == InputType::Backspace) {
                if (!currentlyFocused->text.empty()) {
                    currentlyFocused->text.pop_back();
                    mod = true;
                }
            }

            // todo: will wastefully refresh multiple times if multiple keypresses in a frame
            if (mod) {
                currentlyFocused->RefreshText();
                currentlyFocused->onTextChanged.FireNow(currentlyFocused);
            }
        }
        else {
            ctx->Fallthrough(input);
        }
    });
}

std::shared_ptr<GuiElement> GuiElement::New(GuiContainer& storeIn, const GuiCreateParams params)
{
    auto ptr = std::shared_ptr<GuiElement>(new GuiElement(storeIn, params));
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
        onHoverEnd.FireNow(this);

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

    gameobject->SetPosition(glm::dvec3(objectCenterPosition, -depth));
    gameobject->SetScale(glm::dvec3(objectSize, 1.0));
    gameobject->SetRotation(glm::angleAxis(rotation, glm::vec3(0, 0, 1)));

    if (textobject) {
        //DebugLogInfo("Text placed at ", objectCenterPosition);
        glm::dvec3 textPos = glm::dvec3(objectCenterPosition, -(depth - 0.001));

        // ensure text is on integer pixel coordinates
        //if (std::fmod(textobject->Scale().x, 2.0) == 0.0) {
            //textPos.x += 0.5;
        //}
        //if (std::fmod(textobject->Scale().y, 2.0) == 0.0) {
            //textPos.y += 0.5;
        //}
        textobject->SetPosition(textPos);
        textobject->SetRotation(glm::angleAxis(rotation, glm::vec3(0, 0, 1)));
    }

    if (childrenLayout) {
        childrenLayout->Apply(*this);
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
    MakeTextobject(GetStencilDepth());
}

int GuiElement::TextHeight() {
    return textHeightPixels;
}

void GuiElement::SetParent(GuiElement* newParent) {
    Orphan();
    parent = newParent;
    parent->children.push_back(shared_from_this());

    int newDepth = GetStencilDepth();
    UpdateStencilDepth(newDepth);
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

//void GuiElement::LayoutChildrenList(ListLayout params) {
    
//}

void GuiElement::MakeTextobject(int stencilDepth) {
    textobject = nullptr;

    if (!text.empty()) {
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

        auto result = textmeshparams.LoadText(*font, text, format);

        if (result.success && !textmeshparams.vertices.empty()) { // checking text.empty() is insufficient bc whitespace characters/etc., and Mesh does not allow empty meshes
            textmeshparams.normalizeSize = false;
            auto textmesh = Mesh::New(std::move(textmeshparams));
            GameobjectCreateParams textobjectparams;
            textobjectparams.mesh = textmesh;

            auto key = GuiPassRequirements(font.get(), stencilDepth);
            if (!container->elementPasses.contains(key)) {
                Assert(font);
                static int i = 0;
                std::shared_ptr<DrawPass> newPass = DrawPass::FromTemplate(*container->elementPasses.at(GuiPassRequirements(nullptr, 0)));
                newPass->drawnObjects.clear();
                std::erase_if(newPass->outputs, [stencilDepth](const std::string& s) { return s.find("_WRITTEN_BIT") != std::string::npos; });
                newPass->outputs. push_back("GUI_STENCIL_" + std::to_string(stencilDepth) + "_WRITTEN_BIT");
                std::erase_if(newPass->dependencies, [stencilDepth](const std::string& s) { return s.find("_WRITTEN_BIT") != std::string::npos; });
                if (stencilDepth != 0) {
                    newPass->dependencies.push_back("GUI_STENCIL_" + std::to_string(stencilDepth - 1) + "_WRITTEN_BIT");
                }
                newPass->name = "SCREEN_GUI_PRESENTATION_FONT_" + std::to_string(i++);
                newPass->params.stencilTestMask.reset();
                if (stencilDepth != 0) newPass->params.stencilTestMask.set(stencilDepth - 1, true);
                newPass->params.stencilWriteMask.reset().set(stencilDepth, true);
                //newPass->outputs.pop_back();
                newPass->uniformSupplier = SupplyTextUniforms;
                newPass->boundTextures.push_back(TextureUsageDescriptor{
                    .texture = font,
                    .textureUsageLocation = "fontMap",
                    .willRead = true
                    });
                container->elementPasses.emplace(key, newPass);
            }
            textobjectparams.renderPasses = { container->elementPasses[key], };
            textobject = std::unique_ptr<Gameobject>(Gameobject::New(textobjectparams));
            RefreshGraphics();
            RefreshTransform();

            textHeightPixels = result.heightPixels;
        }
        else {
            textHeightPixels = 0;
        }
    }
}

GuiElement::GuiElement(GuiContainer& storeIn, const GuiCreateParams uiparams):
clipChildren(uiparams.clipChildren),
texture(uiparams.background),
font(uiparams.font),
container(&storeIn)
{
    elementsList.push_back(this);

    GameobjectCreateParams params;
    params.mesh = Mesh::GuiQuad();
    auto key = GuiPassRequirements(uiparams.background.get(), 0);
    if (!storeIn.elementPasses.contains(key)) {
        Assert(uiparams.background);
        static int i = 0;
        std::shared_ptr<DrawPass> newPass = DrawPass::FromTemplate(*storeIn.elementPasses[GuiPassRequirements(nullptr, 0)]);
        newPass->name = "SCREEN_GUI_PRESENTATION_TEXTURED_" + std::to_string(i++);
        newPass->boundTextures.push_back(TextureUsageDescriptor{
                    .texture = uiparams.background,
                    .textureUsageLocation = "colorMap",
                    .willRead = true
            });
        container->elementPasses.emplace(key, newPass);
        
    }
    params.renderPasses = { storeIn.elementPasses[key],};
    params.primitiveType = GL_TRIANGLES;
    gameobject = std::unique_ptr<Gameobject>(Gameobject::New(params));

    RefreshGraphics();
    RefreshTransform();

    if (font) {
        MakeTextobject(0);
    }
}

int GuiElement::GetStencilDepth() {
    if (parent) return std::min(MAX_STENCIL_DEPTH - 1, (parent->clipChildren ? 1 : 0) + parent->GetStencilDepth());
    return 0;
}

void GuiElement::UpdateStencilDepth(int stencilDepth) {
    auto key = GuiPassRequirements(texture.get(), stencilDepth);
    if (!container->elementPasses.contains(key)) {
        static int i = 0;
        std::shared_ptr<DrawPass> newPass = DrawPass::FromTemplate(*container->elementPasses[GuiPassRequirements(nullptr, 0)]);
        std::erase_if(newPass->outputs, [stencilDepth](const std::string& s) { return s.find("_WRITTEN_BIT") != std::string::npos; });
        newPass->outputs.push_back("GUI_STENCIL_" + std::to_string(stencilDepth) + "_WRITTEN_BIT");

        std::erase_if(newPass->outputs, [stencilDepth](const std::string& s) { return s.find("_WRITTEN_BIT") != std::string::npos; });
        if (stencilDepth != 0) {
            newPass->dependencies.push_back("GUI_STENCIL_" + std::to_string(stencilDepth - 1) + "_WRITTEN_BIT");
        }

        //newPass->params.stencilTestMask.reset().set(depth - 1, true);
        newPass->params.stencilTestMask.reset();
        if (stencilDepth != 0) newPass->params.stencilTestMask.set(stencilDepth - 1, true);
        newPass->params.stencilWriteMask.reset().set(stencilDepth, true);
        newPass->name = "SCREEN_GUI_PRESENTATION_STENCILED_" + std::to_string(i++) + "_D" + std::to_string(stencilDepth);
        if (texture) {
            newPass->boundTextures.push_back(TextureUsageDescriptor{
                        .texture = texture,
                        .textureUsageLocation = "colorMap",
                        .willRead = true
                });
        }
        container->elementPasses.emplace(key, newPass);
    }
    std::vector<std::shared_ptr<DrawPass>> mainObjPasses = { container->elementPasses[key],};
    gameobject->ChangeDrawPasses(mainObjPasses);

    if (textobject) {
        //std::vector<std::shared_ptr<DrawPass>> mainObjPasses = { container->elementPasses[GuiPassRequirements(font.get(), depth)], };
        //textobject->ChangeDrawPasses(mainObjPasses);
        MakeTextobject(stencilDepth);
    }

    if (clipChildren) stencilDepth += 1;
    for (auto& c : children) {
        c->UpdateStencilDepth(stencilDepth);
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

std::shared_ptr<TextboxElement> TextboxElement::New(GuiContainer& storeIn, GuiCreateParams params)
{
    Assert(params.font);
    auto ptr = std::shared_ptr<TextboxElement>(new TextboxElement(storeIn, params));
    return ptr;
}

void TextboxElement::SetEmptyText(std::string txt) {
    emptyText = txt;
    if (isEmpty) {
        text = txt;
        RefreshText();
    }
}

bool TextboxElement::IsEmpty() {
    return isEmpty;
}

TextboxElement::~TextboxElement() {
    if (currentlyFocused == this) {
        Unfocus();
    }
}

void TextboxElement::ApplyText(std::string txt) {
    text = txt;
    isEmpty = text.empty();
	onTextChanged.FireNow(this);
    RefreshText();
}

void TextboxElement::Focus() {
    if (currentlyFocused) currentlyFocused->Unfocus();
    currentlyFocused = this;

    if (clearOnFocus) {
        text.clear();
        isEmpty = true;
    }
    else if (isEmpty) {
        text.clear(); // get rid of the empty info text
    }
    RefreshText();
}

void TextboxElement::Unfocus() {
    Assert(currentlyFocused == this);

    currentlyFocused = nullptr;
}

TextboxElement::TextboxElement(GuiContainer& storeIn, const GuiCreateParams params)
: GuiElement(storeIn, params) {
    text = emptyText;
    RefreshText();
}

std::shared_ptr<ScrollableElement> ScrollableElement::New(GuiContainer& storeIn, ScrollableGuiCreateParams uiparams)
{
    auto ptr = std::shared_ptr<ScrollableElement>(new ScrollableElement(storeIn, uiparams));

    return ptr;
}

void ScrollableElement::RefreshGraphics() {
    GuiElement::RefreshGraphics();
    canvas->RefreshGraphics();
    if (horizontalScrollbar) horizontalScrollbar->RefreshGraphics();
    if (verticalScrollbar) verticalScrollbar->RefreshGraphics();
}

ScrollableElement::ScrollableElement(GuiContainer& storeIn, ScrollableGuiCreateParams uiparams) :
	GuiElement(storeIn, uiparams)
{
    clippingMask = GuiElement::New(storeIn, GuiCreateParams{ .background = nullptr, .clipChildren = true });
    clippingMask->SetParent(this);
    clippingMask->backgroundColor = { 0, 0, 0, 0 };
    clippingMask->pixelPosition = { 0, 0 };
    clippingMask->percentagePosition = { 0, 1 };
    clippingMask->anchorPoint = { -0.5, 0.5 };
    clippingMask->percentageSize = { 1, 1 };
    clippingMask->pixelSize = { uiparams.orientation == ScrollbarOrientation::Horizontal ? 0 : -uiparams.scrollbarSizePixels, uiparams.orientation == ScrollbarOrientation::Vertical ? 0 : -uiparams.scrollbarSizePixels };
    clippingMask->RefreshGraphics();

    canvas = GuiElement::New(storeIn, GuiCreateParams{ .background = uiparams.background, .clipChildren = false});
    canvas->SetParent(clippingMask.get());
    canvas->pixelPosition = { 0, 0 };
    canvas->percentagePosition = { 0, 1 };
    canvas->anchorPoint = { -0.5, 0.5 };
    canvas->percentageSize = { 1, 1 };
    canvas->pixelSize = { 0, 0 };
}

void ListLayout::Apply(GuiElement& target) {
    // sort children
    std::sort(target.children.begin(), target.children.end(), [](const std::shared_ptr<GuiElement>& a, const std::shared_ptr<GuiElement>& b) -> bool { return a->sortOrder < b->sortOrder; });

    glm::ivec2 pixelPos = pixelStart;
    glm::vec2 percentPos = rootPercentStart;
    for (auto& c : target.children) {
        c->pixelPosition = pixelPos;
        pixelPos += pixelStride + glm::ivec2(glm::vec2(c->GetPixelSize()) * childPercentStride);
        c->percentagePosition = percentPos;
        percentPos += rootPercentStride;
        //c->RefreshTransform();
    }
}
