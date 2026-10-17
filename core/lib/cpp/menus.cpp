#include "menus.h"

#include <string>

namespace {
const sf::Color kYellow(255, 255, 0);
const sf::Color kCyan(0, 255, 255);
const sf::Color kRed(255, 0, 0);
const sf::Color kOrange(255, 184, 81);
const sf::Color kWhite(222, 222, 255);
const sf::Vector2f kBtn(104.f, 16.f);
}

void Menus::buildTitle(Panel& p) {
    p.clear();
    p.addButton(BtnPlay, "PLAY", {64.f, 256.f}, {64.f, 14.f});
    p.addButton(BtnQuit, "QUIT", {160.f, 256.f}, {64.f, 14.f});
}

void Menus::buildHudButtons(Panel& p) {
    p.clear();
    p.addButton(BtnPause, "II", {204.f, -16.f}, {24.f, 12.f});
}

void Menus::buildPause(Panel& p, float volume01, bool muted) {
    p.clear();
    p.setDim(190);
    p.addText("PAUSED", {112.f, 56.f}, kYellow, 16.f);
    p.addText("VOLUME", {112.f, 92.f}, kCyan);
    p.setSlider({112.f, 108.f}, 120.f, volume01);
    p.addButton(BtnMute,   muted ? "SOUND: OFF" : "SOUND: ON", {112.f, 150.f}, kBtn);
    p.addButton(BtnResume, "RESUME",                            {112.f, 174.f}, kBtn);
    p.addButton(BtnMenu,   "MENU",                              {112.f, 198.f}, kBtn);
}

void Menus::buildDeath(Panel& p, int score, int highScore, int level) {
    p.clear();
    p.setDim(215);
    p.addText("GAME OVER", {112.f, 56.f}, kRed, 16.f);
    if (score > 0 && score >= highScore)
        p.addText("NEW HIGH SCORE!", {112.f, 78.f}, kYellow);
    p.addText("SCORE",      {112.f, 96.f},  kCyan);
    p.addText(std::to_string(score),     {112.f, 108.f}, kWhite);
    p.addText("HIGH SCORE", {112.f, 124.f}, kCyan);
    p.addText(std::to_string(highScore), {112.f, 136.f}, kWhite);
    p.addText("LEVEL " + std::to_string(level), {112.f, 152.f}, kOrange);
    p.addButton(BtnRetry, "RETRY", {112.f, 178.f}, kBtn);
    p.addButton(BtnMenu,  "MENU",  {112.f, 200.f}, kBtn);
    p.addButton(BtnQuit,  "QUIT",  {112.f, 222.f}, kBtn);
}

void Menus::buildPass(Panel& p, int level, int score) {
    p.clear();
    p.setDim(215);
    p.addText("LEVEL CLEAR!", {112.f, 60.f}, kYellow, 16.f);
    p.addText("LEVEL " + std::to_string(level), {112.f, 92.f}, kOrange);
    p.addText("SCORE", {112.f, 116.f}, kCyan);
    p.addText(std::to_string(score), {112.f, 128.f}, kWhite);
    p.addButton(BtnNext, "NEXT LEVEL", {112.f, 172.f}, kBtn);
    p.addButton(BtnMenu, "MENU",       {112.f, 196.f}, kBtn);
}
