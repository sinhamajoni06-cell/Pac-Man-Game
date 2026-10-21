#pragma once
#include "ui_panel.h"

// What each screen looks like. The ids are what Panel::mousePressed returns.
namespace Menus {
enum Id { BtnPlay = 1, BtnQuit, BtnResume, BtnMenu, BtnMute, BtnRetry, BtnNext, BtnPause, BtnInfo, BtnBack };

void buildTitle(Panel& p);                                   // PLAY / QUIT on the home page
void buildHudButtons(Panel& p);                              // the little pause button while playing
void buildPause(Panel& p, float volume01, bool muted);
void buildDeath(Panel& p, int score, int highScore, int level);
void buildPass(Panel& p, int level, int score);
void buildInfo(Panel& p);                                    // controls and credits
}
