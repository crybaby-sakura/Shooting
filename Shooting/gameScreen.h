#pragma once

#define GAME_W 854 
#define GAME_H 480
#define GAME_AREA_X 187   // ゲームエリアの左端X座標

void backGround();
void foreGround();
void drawSidePanel();
void drawGameOverlay();
void resetStars();
void updateStars();
