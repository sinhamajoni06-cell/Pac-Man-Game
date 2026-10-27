#pragma once
#include "ui_panel.h"

// What each screen looks like. The ids are what Panel::mousePressed returns.
namespace Menus {
enum Id { BtnPlay = 1, BtnQuit, BtnResume, BtnMenu, BtnMute, BtnRetry, BtnNext, BtnPause, BtnInfo, BtnBack, BtnClassic, BtnWordMode, BtnSpin, BtnPlayWord, BtnSkip, BtnQuitYes, BtnQuitCancel };

void buildTitle(Panel& p);                                   // PLAY / QUIT on the home page
void buildHudButtons(Panel& p);                              // the little pause button while playing
void buildPause(Panel& p, float volume01, bool muted);
void buildDeath(Panel& p, int score, int highScore, int level);
void buildPass(Panel& p, int level, int score);
void buildModeSelect(Panel& p, bool comingSoon);             // Classic / Word Finding
void buildWordSlot(Panel& p, bool finished, int letters);
void buildWordSpin(Panel& p, bool finished, int letters, int level);   // in-game spin: only BACK
void buildWordWin(Panel& p, const std::string& word, int level, int score, float seconds, int bonus);   // "WORD FOUND!" screen   // text and buttons under the slot machine
void buildQuitConfirm(Panel& p);                             // "Do you want to exit?"
void buildInfo(Panel& p);                                    // controls and credits
}
