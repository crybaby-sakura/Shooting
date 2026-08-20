#pragma once
void enemyDisp();
void enemyHit();
void enemyControl();

void addExplosion(double x, double y);
void drawExplosion();
void clearAllSparks();

// 敵機エンジン炎（色・サイズを指定可能）
void spawnEnemyEngineFlame(double x, double y, double vx, double vy,
    int baseR, int baseG, int baseB, double radius, int id);
void updateEnemyEngineFlame();
void drawEnemyEngineFlame(int blendAlpha, int blendAlpha2);
void clearAllEnemyEngineFlames();
