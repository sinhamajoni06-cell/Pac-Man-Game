#include "ui_panel.h"

#include <algorithm>
#include <iostream>

namespace {
sf::Font g_font;
bool     g_loaded = false;
float    g_top = 0.f, g_h = 8.f;

const sf::Color kLavender(222, 222, 255);
const sf::Color kYellow(255, 255, 0);
const sf::Color kBlue(33, 33, 255);
const sf::Color kRed(255, 0, 0);
}

bool Panel::loadFont(const std::string& path) {
    if (g_loaded) return true;
    if (!g_font.openFromFile(path)) {
        std::cerr << "[Panel] Cannot load font: " << path << "\n";
        return false;
    }
    g_font.setSmooth(false);
    sf::Text ref(g_font, "H", 8);
    sf::FloatRect b = ref.getLocalBounds();
    g_top = b.position.y;
    g_h   = b.size.y;
    g_loaded = true;
    return true;
}

void Panel::clear() {
    m_texts.clear();
    m_buttons.clear();
    m_slider = SliderItem{};
    m_dim = 0;
}

void Panel::addText(const std::string& text, sf::Vector2f mapPos, sf::Color color, float size, Align align) {
    m_texts.push_back({text, mapPos, color, size, align});
}

void Panel::addButton(int id, const std::string& label, sf::Vector2f center, sf::Vector2f size, float textSize) {
    ButtonItem b;
    b.id = id;
    b.textSize = textSize;
    b.label = label;
    b.rect = sf::FloatRect({center.x - size.x / 2.f, center.y - size.y / 2.f}, size);
    m_buttons.push_back(b);
}

void Panel::setSlider(sf::Vector2f center, float width, float value01) {
    m_slider.active = true;
    m_slider.center = center;
    m_slider.width = width;
    m_slider.value = std::clamp(value01, 0.f, 1.f);
    m_slider.dragging = false;
}

void Panel::setSliderFromMouse(sf::Vector2f mapPos) {
    float left = m_slider.center.x - m_slider.width / 2.f;
    m_slider.value = std::clamp((mapPos.x - left) / m_slider.width, 0.f, 1.f);
}

// ---------------------------------------------------------------- mouse

void Panel::mouseMoved(sf::Vector2f world) {
    sf::Vector2f m = toMap(world);
    int hoveredId = 0;
    for (ButtonItem& b : m_buttons) {
        b.hover = b.rect.contains(m);
        if (b.hover) hoveredId = b.id;
    }
    if (hoveredId > 0)         { m_focusId = hoveredId; m_focusFromMouse = true; }
    else if (m_focusFromMouse) { m_focusId = 0;         m_focusFromMouse = false; }
    if (m_slider.dragging) setSliderFromMouse(m);
}

int Panel::mousePressed(sf::Vector2f world) {
    sf::Vector2f m = toMap(world);

    if (m_slider.active) {
        sf::FloatRect hit({m_slider.center.x - m_slider.width / 2.f - 4.f, m_slider.center.y - 8.f},
                          {m_slider.width + 8.f, 16.f});
        if (hit.contains(m)) {
            m_slider.dragging = true;
            setSliderFromMouse(m);
            return 0;
        }
    }
    for (const ButtonItem& b : m_buttons)
        if (b.rect.contains(m)) return b.id;
    return 0;
}

void Panel::mouseReleased() { m_slider.dragging = false; }

// ---------------------------------------------------------------- drawing

void Panel::drawText(sf::RenderTarget& t, const std::string& s, sf::Vector2f pos,
                     sf::Color color, float size, Align align) {
    if (!g_loaded) return;
    sf::Text text(g_font, s, 8);
    text.setFillColor(color);
    float w = 8.f * static_cast<float>(s.size());
    float ox = (align == Align::Center) ? w / 2.f : (align == Align::Right ? w : 0.f);
    text.setOrigin({ox, g_top + g_h / 2.f});
    float k = m_scale * size / 8.f;
    text.setScale({k, k});
    text.setPosition(m_origin + pos * m_scale);
    t.draw(text);
}

void Panel::drawRect(sf::RenderTarget& t, sf::FloatRect rect, sf::Color fill,
                     sf::Color outline, float thickness) {
    sf::RectangleShape r({rect.size.x * m_scale, rect.size.y * m_scale});
    r.setPosition(m_origin + rect.position * m_scale);
    r.setFillColor(fill);
    r.setOutlineColor(outline);
    r.setOutlineThickness(-thickness * m_scale);          // drawn inside the rectangle
    t.draw(r);
}

void Panel::draw(sf::RenderTarget& target) {
    if (m_dim > 0)                                         // darken the whole game area
        drawRect(target, sf::FloatRect({0.f, -24.f}, {224.f, 288.f}),
                 sf::Color(0, 0, 0, static_cast<std::uint8_t>(m_dim)), sf::Color::Transparent, 0.f);

    for (const TextItem& t : m_texts)
        drawText(target, t.text, t.pos, t.color, t.size, t.align);

    for (const ButtonItem& b : m_buttons) {
        drawRect(target, b.rect, (b.id == m_focusId) ? kBlue : sf::Color::Black,
                 (b.id == m_focusId) ? kYellow : kLavender, 1.f);
        drawText(target, b.label, b.rect.position + b.rect.size / 2.f,
                 (b.id == m_focusId) ? kYellow : sf::Color::White, b.textSize, Align::Center);
    }

    if (m_slider.active) {
        float left = m_slider.center.x - m_slider.width / 2.f;
        float cy = m_slider.center.y;
        drawRect(target, sf::FloatRect({left, cy - 2.f}, {m_slider.width, 4.f}),
                 sf::Color(40, 40, 90), kLavender, 1.f);
        drawRect(target, sf::FloatRect({left, cy - 2.f}, {m_slider.width * m_slider.value, 4.f}),
                 kYellow, sf::Color::Transparent, 0.f);
        drawRect(target, sf::FloatRect({left + m_slider.width * m_slider.value - 2.f, cy - 5.f}, {4.f, 10.f}),
                 sf::Color::White, sf::Color::Transparent, 0.f);
        drawText(target, std::to_string(static_cast<int>(m_slider.value * 100.f + 0.5f)) + "%",
                 {m_slider.center.x, cy + 14.f}, sf::Color::White, 8.f, Align::Center);
    }
}

// Small box in the middle of the maze that shows the volume for a moment
void Panel::drawVolumeToast(sf::RenderTarget& target, float volume01, bool muted) {
    drawRect(target, sf::FloatRect({64.f, 112.f}, {96.f, 28.f}),
             sf::Color::Black, kLavender, 1.f);
    drawText(target, muted ? "MUTED" : "VOLUME", {112.f, 121.f},
             muted ? kRed : sf::Color::White, 8.f, Align::Center);

    int filled = static_cast<int>(volume01 * 10.f + 0.5f);
    for (int i = 0; i < 10; ++i) {
        bool on = (i < filled) && !muted;
        drawRect(target, sf::FloatRect({73.f + 8.f * static_cast<float>(i), 130.f}, {6.f, 4.f}),
                 on ? kYellow : sf::Color(60, 60, 100), sf::Color::Transparent, 0.f);
    }
}

void Panel::keyNavigate(int delta) {
    if (m_buttons.empty()) return;
    const int n = static_cast<int>(m_buttons.size());

    int cur = -1;
    for (int i = 0; i < n; ++i)
        if (m_buttons[static_cast<size_t>(i)].id == m_focusId) cur = i;

    int next;
    if (cur < 0) next = (delta > 0) ? 0 : n - 1;       // first press: start at the first (or last) button
    else         next = (cur + delta + n) % n;          // wraps around

    m_focusId = m_buttons[static_cast<size_t>(next)].id;
    m_focusFromMouse = false;
}

int Panel::focusedId() const {
    for (const ButtonItem& b : m_buttons)
        if (b.id == m_focusId) return m_focusId;
    return 0;
}
