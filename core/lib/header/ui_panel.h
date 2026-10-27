#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// A simple menu screen: dim background, text, clickable buttons and one slider.
// Positions are in MAP pixels (the maze is 224x248, the HUD rows sit above and below it).
class Panel {
public:
    enum class Align { Left, Center, Right };
    enum class Icon { None, Gear };

    static bool loadFont(const std::string& path);      // once, shared by every panel

    void setTransform(sf::Vector2f origin, float scale) { m_origin = origin; m_scale = scale; }
    void clear();
    void setDim(int alpha) { m_dim = alpha; }           // 0..255 black layer over the whole game

    void addText(const std::string& text, sf::Vector2f mapPos, sf::Color color,
                 float size = 8.f, Align align = Align::Center);
    void addButton(int id, const std::string& label, sf::Vector2f center, sf::Vector2f size,
                   float textSize = 8.f);
    void setSlider(sf::Vector2f center, float width, float value01);
    void addIconButton(int id, Icon icon, sf::Vector2f center, sf::Vector2f size);

    float sliderValue()    const { return m_slider.value; }      // 0..1
    bool  sliderDragging() const { return m_slider.dragging; }

    // `world` = mouse position in window coordinates (window.mapPixelToCoords)
    void mouseMoved(sf::Vector2f world);
    int  mousePressed(sf::Vector2f world);              // id of the clicked button, or 0
    void mouseReleased();

    // Keyboard: arrows move the highlight, Enter uses it
    void keyNavigate(int delta);        // -1 = previous button, +1 = next (wraps around)
    int  focusedId() const;             // id of the highlighted button, or 0 if none
    void resetFocus() { m_focusId = 0; m_focusFromMouse = false; }

    void draw(sf::RenderTarget& target);
    void drawVolumeToast(sf::RenderTarget& target, float volume01, bool muted);

private:
    struct TextItem   { std::string text; sf::Vector2f pos; sf::Color color; float size; Align align; };
    struct ButtonItem { int id; std::string label; sf::FloatRect rect; bool hover = false; float textSize = 8.f; Icon icon = Icon::None; };
    struct SliderItem { bool active = false; sf::Vector2f center; float width = 0.f;
                        float value = 0.f; bool dragging = false; };

    sf::Vector2f toMap(sf::Vector2f world) const { return (world - m_origin) / m_scale; }
    void drawText(sf::RenderTarget& t, const std::string& s, sf::Vector2f pos,
                  sf::Color color, float size, Align align);
    void drawRect(sf::RenderTarget& t, sf::FloatRect rect, sf::Color fill,
                  sf::Color outline, float thickness);
    void setSliderFromMouse(sf::Vector2f mapPos);
    void drawGear(sf::RenderTarget& t, sf::Vector2f center, sf::Color color);

    std::vector<TextItem>   m_texts;
    std::vector<ButtonItem> m_buttons;
    SliderItem m_slider;
    int  m_focusId = 0;                 // highlighted button (0 = none); kept when a screen is rebuilt
    bool m_focusFromMouse = false;
    int   m_dim = 0;

    sf::Vector2f m_origin{0.f, 0.f};
    float m_scale = 1.f;
};
