#pragma once
void playerControl();
void playerDisp();

// 力場エフェクト（引力・斥力演出）
void spawnForceParticles(double x, double y, double forceX, double forceY, double radius = 30.0);
void updateForceParticles();
void drawForceParticles();

void spawnPlayerEngineFlame(double x, double y, double vx, double vy);
void updatePlayerEngineFlame();
void drawPlayerEngineFlame();

extern bool isMuteki;
