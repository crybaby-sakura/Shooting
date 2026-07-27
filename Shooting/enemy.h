#pragma once
void enemyDisp();
void enemyHit();
void enemyControl();

void addExplosion(double x, double y);
void drawExplosion();

// 敵機エンジン炎（色・サイズを指定可能）
void spawnEnemyEngineFlame(double x, double y, double vx, double vy,
    int baseR, int baseG, int baseB, double radius);
void updateEnemyEngineFlame();
void drawEnemyEngineFlame(int blendAlpha);
