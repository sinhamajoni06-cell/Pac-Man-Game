#include "menus.h"

#include <string>
#include "version.h"

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
    p.addButton(BtnPlay, "PLAY", {40.f, 256.f}, {56.f, 14.f});
    p.addButton(BtnInfo, "INFO", {112.f, 256.f}, {56.f, 14.f});
    p.addButton(BtnQuit, "QUIT", {184.f, 256.f}, {56.f, 14.f});
    p.addIconButton(BtnSettings, Panel::Icon::Gear, {204.f, -16.f}, {24.f, 12.f});   // where the pause button is in the game
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

void Menus::buildInfo(Panel& p) {
    p.clear();
    p.setDim(235);
    p.addText("HOW TO PLAY",                  {112.f, 18.f},  kYellow);

    p.addText("MOVE: ARROWS / WASD",          {112.f, 36.f},  kWhite);
    p.addText("GAMEPAD: STICK / D-PAD",       {112.f, 48.f},  kWhite);
    p.addText("PAUSE: P OR ESC",              {112.f, 60.f},  kWhite);
    p.addText("MUTE: M",                      {112.f, 72.f},  kWhite);
    p.addText("VOLUME: - / = / WHEEL",        {112.f, 84.f},  kWhite);
    p.addText("FULLSCREEN: F11/ALT+ENTER",    {112.f, 96.f},  kWhite);
    p.addText("MENUS: ARROWS + ENTER",        {112.f, 108.f}, kWhite);
    p.addText("MOUSE: CLICK BUTTONS",         {112.f, 120.f}, kWhite);

    p.addText("CLASSIC: EAT ALL DOTS",        {112.f, 140.f}, kCyan);
    p.addText("WORD MODE: COLLECT THE",       {112.f, 152.f}, kCyan);
    p.addText("LETTERS, FAST = BONUS",        {112.f, 164.f}, kCyan);
    p.addText("WRONG LETTER = -50 PTS",       {112.f, 176.f}, kCyan);

    p.addText(std::string("WORD-MAN  V") + WORDMAN_VERSION, {112.f, 194.f}, kYellow);
    p.addText("FONT: PRESS START 2P",         {112.f, 206.f}, kWhite, 6.f);
    p.addText("FAN-MADE PROJECT",             {112.f, 216.f}, kWhite, 6.f);
    p.addText("NOT AFFILIATED WITH BANDAI NAMCO", {112.f, 226.f}, kWhite, 6.f);

    p.addButton(BtnBack, "BACK", {112.f, 242.f}, {64.f, 14.f});
}

void Menus::buildModeSelect(Panel& p, bool comingSoon) {
    p.clear();
    p.addText("SELECT MODE", {112.f, 52.f}, kYellow, 16.f);

    p.addButton(BtnClassic, "CLASSIC", {112.f, 104.f}, {120.f, 18.f});
    p.addText("THE ARCADE GAME", {112.f, 122.f}, kCyan, 6.f);

    p.addButton(BtnWordMode, "WORD FINDING", {112.f, 150.f}, {120.f, 18.f});
    if (comingSoon) p.addText("COMING SOON!", {112.f, 170.f}, kRed, 8.f);
    else            p.addText("COLLECT THE SECRET WORD", {112.f, 168.f}, kCyan, 6.f);

    p.addButton(BtnBack, "BACK", {112.f, 214.f}, {64.f, 14.f});
}

void Menus::buildWordSlot(Panel& p, bool finished, int letters) {
    p.clear();
    if (finished) {
        p.addText("YOUR WORD HAS " + std::to_string(letters) + " LETTERS", {112.f, 168.f}, kCyan);
        p.addButton(BtnSpin,     "SPIN AGAIN", {60.f, 196.f},  {96.f, 16.f});
        p.addButton(BtnPlayWord, "PLAY",       {160.f, 196.f}, {64.f, 16.f});
        p.addButton(BtnBack,     "BACK",       {112.f, 222.f}, {64.f, 14.f});
    } else {
        p.addText("SPINNING...", {112.f, 168.f}, kYellow);
        p.addButton(BtnBack, "BACK", {112.f, 196.f}, {64.f, 14.f});
    }
}

void Menus::buildWordWin(Panel& p, const std::string& word, int level, int score, float seconds, int bonus) {
    p.clear();
    p.setDim(215);

    int total = static_cast<int>(seconds);
    std::string timeText = std::to_string(total / 60) + ":" + (total % 60 < 10 ? "0" : "") + std::to_string(total % 60);

    p.addText("WORD FOUND!", {112.f, 50.f}, kYellow, 16.f);
    p.addText(word,          {112.f, 80.f}, kCyan,   16.f);
    p.addText("LEVEL " + std::to_string(level), {112.f, 104.f}, kOrange);
    p.addText("TIME " + timeText,               {112.f, 122.f}, kWhite);
    if (bonus > 0) p.addText("TIME BONUS +" + std::to_string(bonus), {112.f, 134.f}, kYellow);
    else           p.addText("NO TIME BONUS",                          {112.f, 134.f}, kWhite);
    p.addText("SCORE", {112.f, 154.f}, kCyan);
    p.addText(std::to_string(score), {112.f, 166.f}, kWhite);
    p.addButton(BtnNext, "NEXT WORD", {112.f, 194.f}, kBtn);
    p.addButton(BtnMenu, "MENU",      {112.f, 216.f}, kBtn);
}

void Menus::buildWordSpin(Panel& p, bool finished, int letters, int level) {
    p.clear();
    p.addText("LEVEL " + std::to_string(level), {112.f, 76.f}, kOrange);
    if (finished) p.addText("YOUR WORD HAS " + std::to_string(letters) + " LETTERS", {112.f, 168.f}, kCyan);
    else          p.addText("SPINNING...", {112.f, 168.f}, kYellow);
    p.addButton(BtnSkip, "BACK", {112.f, 200.f}, {64.f, 14.f});     // skips the animation
}

void Menus::buildQuitConfirm(Panel& p) {
    p.clear();
    p.setDim(240);
    p.addText("DO YOU WANT", {112.f, 80.f},  kYellow, 16.f);
    p.addText("TO EXIT?",    {112.f, 100.f}, kYellow, 16.f);

    // CANCEL is added first, so the arrow keys land on it first (the safe choice)
    p.addButton(BtnQuitCancel, "CANCEL", {152.f, 150.f}, {72.f, 18.f});
    p.addButton(BtnQuitYes,    "YES",    {72.f, 150.f},  {72.f, 18.f});
}

void Menus::buildSettings(Panel& p, float volume01, bool muted) {
    p.clear();
    p.setDim(235);
    p.addText("SETTINGS", {112.f, 48.f},  kYellow, 16.f);
    p.addText("SOUND",    {112.f, 82.f},  kCyan);
    p.addText("VOLUME",   {112.f, 104.f}, kWhite);
    p.setSlider({112.f, 120.f}, 120.f, volume01);
    p.addButton(BtnMute, muted ? "SOUND: OFF" : "SOUND: ON", {112.f, 164.f}, kBtn);
    p.addButton(BtnBack, "BACK", {112.f, 200.f}, kBtn);
}
