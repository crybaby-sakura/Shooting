#include "DxLib.h"
#include "gv.h"
#include "enemy.h"
#include "imgSoundLoad.h"
#include "stateManager.h"
#include "player.h"
#include "effectRand.h"
#include "tasController.h"


#define MAX_SPARKS 480   // 同時表示最大数

// ---------- 火花パーティクル ----------
struct Spark {
    bool active;         // 使用中フラグ
    double x, y;
    double vx, vy;
    int life;
    int maxLife;
    unsigned int color;
};

static Spark sparkPool[MAX_SPARKS];   // 固定長配列
static int sparkCount = 0;            // 現在のアクティブ数

// 火花を発生させる
void addExplosion(double x, double y) {
    if (g_isTasMode) return;

    const int NUM_SPARKS = 12 + effectRandInt(8);
    for (int i = 0; i < NUM_SPARKS; ++i) {
        // 満杯なら最も古いものを探して上書き（または単にスキップ）
        if (sparkCount >= MAX_SPARKS) {
            // 先頭から寿命0を探して再利用
            bool found = false;
            for (int j = 0; j < MAX_SPARKS; ++j) {
                if (!sparkPool[j].active || sparkPool[j].life <= 0) {
                    sparkPool[j].active = false;
                    sparkCount--;
                    // そのインデックスを再利用
                    found = true;
                    // ループを抜けてそのjを使う（下の処理に持っていく）
                    // 面倒なので、新しく生成するときに空きを探す方式に切り替え
                }
            }
            // 見つからなければ生成しない
            if (!found) break;
        }

        // 空きスロットを探す
        int idx = -1;
        for (int j = 0; j < MAX_SPARKS; ++j) {
            if (!sparkPool[j].active || sparkPool[j].life <= 0) {
                idx = j;
                break;
            }
        }
        if (idx == -1) continue;  // 念のため

        double angle = 2.0 * 3.14159265 * effectRandDouble(); // 0～2π
        double speed = 1.0 + effectRandDouble() * 2.0;  // 1.0～3.0 (連続的に)
        Spark* s = &sparkPool[idx];
        s->active = true;
        s->x = x;
        s->y = y;
        s->vx = cos(angle) * speed;
        s->vy = sin(angle) * speed - 1.5;
        s->maxLife = 15 + effectRandInt(10);             // 15～24
        s->life = s->maxLife;
        int r = 255;
        int g = 200 + effectRandInt(56);                 // 200～255
        int b = effectRandInt(51);                       // 0～50
        s->color = GetColor(r, g, b);
        sparkCount++;
    }
}

// ---------- 火花パーティクルの更新と描画 ----------
void drawExplosion() {
    SetDrawBlendMode(DX_BLENDMODE_ADD, 200);   // お好みで
    for (int i = 0; i < MAX_SPARKS; ++i) {
        if (!sparkPool[i].active) continue;

        Spark* s = &sparkPool[i];
        s->x += s->vx;
        s->y += s->vy;
        s->vy += 0.05;          // 重力
        s->life--;

        if (s->life <= 0) {
            s->active = false;
            sparkCount--;
            continue;
        }

        double ratio = (double)s->life / s->maxLife;
        int alpha = (int)(255 * ratio);
        if (alpha < 0) alpha = 0;

        unsigned int baseR = (s->color >> 16) & 0xFF;
        unsigned int baseG = (s->color >> 8) & 0xFF;
        unsigned int baseB = s->color & 0xFF;
        unsigned int r = baseR;
        unsigned int g = (unsigned int)(baseG * ratio);
        unsigned int b = (unsigned int)(baseB * ratio * 0.5);
        int finalColor = GetColor(r, g, b);

        DrawCircleAA((float)s->x, (float)s->y, 2, 8, finalColor, TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 火花パーティクルを全消去
void clearAllSparks() {
    for (int i = 0; i < MAX_SPARKS; ++i) {
        sparkPool[i].active = false;
    }
    sparkCount = 0;
}



#define MAX_ENEMY_ENGINE_FLAMES 1200   // 5エンジン分 x 2機

struct EnemyEngineFlame {
    bool active;
    double x, y;
    double vx, vy;
    int life;
    int maxLife;
    int baseR, baseG, baseB;
    double drawRadius;   // 描画時の半径（エンジンごとに異なる）
    int id;
};

static EnemyEngineFlame enemyFlamePool[MAX_ENEMY_ENGINE_FLAMES];

void spawnEnemyEngineFlame(double x, double y, double vx, double vy,
    int baseR, int baseG, int baseB, double radius, int id)
{
    const int count = 8;   // 1エンジンあたりの発生数（必要に応じて引数化も可）

    // 推力方向：基本は下向き(0,1)に、移動の反動を少し加える
    double dirX = 0.0 - vx * 0.3;
    double dirY = 1.0 - vy * 0.3;
    double length = sqrt(dirX * dirX + dirY * dirY);
    if (length > 0.0) {
        dirX /= length;
        dirY /= length;
    }
    else {
        dirX = 0.0; dirY = 1.0;
    }

    for (int i = 0; i < count; ++i) {
        // 空きスロット検索
        int idx = -1;
        for (int j = 0; j < MAX_ENEMY_ENGINE_FLAMES; ++j) {
            if (!enemyFlamePool[j].active) {
                idx = j;
                break;
            }
        }
        if (idx == -1) continue;

        EnemyEngineFlame* p = &enemyFlamePool[idx];
        p->active = true;
        // 発生位置：±2.5ドットのばらつき
        p->x = x + (effectRandDouble() - 0.5) * 5.0;
        p->y = y + (effectRandDouble() - 0.5) * 5.0;

        // 速度：基本方向にやや拡散
        double speed = 2.5 + effectRandDouble() * 1.0;   // 2.5～3.5
        double spread = 0.15;   // 赤オレンジは少し広がっても良い
        p->vx = dirX * speed + (effectRandDouble() - 0.5) * spread;
        p->vy = dirY * speed + (effectRandDouble() - 0.5) * spread;

        // 寿命
        p->maxLife = 10 + effectRandInt(6);   // 10～15
        p->life = p->maxLife;

        // 色（指定基本色 ±10程度）
        p->baseR = baseR + effectRandInt(21) - 10;
        p->baseG = baseG + effectRandInt(21) - 10;
        p->baseB = baseB + effectRandInt(21) - 10;
        if (p->baseR < 0) p->baseR = 0; else if (p->baseR > 255) p->baseR = 255;
        if (p->baseG < 0) p->baseG = 0; else if (p->baseG > 255) p->baseG = 255;
        if (p->baseB < 0) p->baseB = 0; else if (p->baseB > 255) p->baseB = 255;

        p->drawRadius = radius;   // 指定された半径を保持
        p->id = id;
    }
}

void updateEnemyEngineFlame() {
    for (int i = 0; i < MAX_ENEMY_ENGINE_FLAMES; ++i) {
        if (!enemyFlamePool[i].active) continue;
        EnemyEngineFlame* p = &enemyFlamePool[i];
        p->x += p->vx;
        p->y += p->vy;
        p->life--;
        if (p->life <= 0) p->active = false;
    }
}

void drawEnemyEngineFlame(int blendAlpha, int blendAlpha2) {
    SetDrawBlendMode(DX_BLENDMODE_ADD, blendAlpha);
    for (int i = 0; i < MAX_ENEMY_ENGINE_FLAMES; ++i) {
        if (!enemyFlamePool[i].active) continue;
        if (enemyFlamePool[i].id != 0) continue;
        EnemyEngineFlame* p = &enemyFlamePool[i];
        double ratio = (double)p->life / p->maxLife;
        int r = (int)(p->baseR * ratio);
        int g = (int)(p->baseG * ratio);
        int b = (int)(p->baseB * ratio);
        if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
        // エンジンごとに異なる半径で描画
        DrawCircleAA((float)p->x, (float)p->y, (float)p->drawRadius, 8, GetColor(r, g, b), TRUE);
    }
    if (enemy.x2 > -999.0) {
        SetDrawBlendMode(DX_BLENDMODE_ADD, blendAlpha2);
        for (int i = 0; i < MAX_ENEMY_ENGINE_FLAMES; ++i) {
            if (!enemyFlamePool[i].active) continue;
            if (enemyFlamePool[i].id != 1) continue;
            EnemyEngineFlame* p = &enemyFlamePool[i];
            double ratio = (double)p->life / p->maxLife;
            int r = (int)(p->baseR * ratio);
            int g = (int)(p->baseG * ratio);
            int b = (int)(p->baseB * ratio);
            if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
            // エンジンごとに異なる半径で描画
            DrawCircleAA((float)p->x, (float)p->y, (float)p->drawRadius, 8, GetColor(r, g, b), TRUE);
        }
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 敵のエンジン炎パーティクルを全消去
void clearAllEnemyEngineFlames() {
    for (int i = 0; i < MAX_ENEMY_ENGINE_FLAMES; ++i) {
        enemyFlamePool[i].active = false;
    }
}



void enemyControl() {
    if (g_isTasMode) return;

    // 5つのエンジン位置（相対座標）を定義。実際の画像に合わせて調整してください。
    // 形式：{ offsetX, offsetY, R, G, B, radius }
    struct EnginePos {
        double ox, oy;
        int r, g, b;
        double radius;
    };

    // 仮の配置例（画像サイズに応じて変更）
    // 赤オレンジ系4つ
    const EnginePos engines[] = {
        { -68.0, 29.0, 200, 60, 15, 2 },   // 左下：赤みを落とし暗く
        {  68.0, 29.0, 200, 60, 15, 2 },   // 右下
        { -23.0, 55.0, 200, 70, 20, 2 },   // 左中
        {  20.0, 55.0, 200, 70, 20, 2 },   // 右中
        {  -1.0, 58.0,  30, 100, 200, 4 }  // 中央下：青も暗めに
    };
    const int numEngines = sizeof(engines) / sizeof(engines[0]);

    static double prevX = enemy.x;
    static double prevY = enemy.y;

    double vx = enemy.x - prevX;
    double vy = enemy.y - prevY;
    prevX = enemy.x;
    prevY = enemy.y;

    for (int i = 0; i < numEngines; ++i) {
        double nozzleX = enemy.x + engines[i].ox;
        double nozzleY = enemy.y + engines[i].oy;
        spawnEnemyEngineFlame(nozzleX, nozzleY, vx, vy,
            engines[i].r, engines[i].g, engines[i].b,
            engines[i].radius, 0);
    }

    static double prevX2 = enemy.x2;
    static double prevY2 = enemy.y2;

    if (enemy.x2 > -999.0) {
        double vx2 = enemy.x2 - prevX2;
        double vy2 = enemy.y2 - prevY2;
        prevX2 = enemy.x2;
        prevY2 = enemy.y2;

        for (int i = 0; i < numEngines; ++i) {
            double nozzleX = enemy.x2 + engines[i].ox;
            double nozzleY = enemy.y2 + engines[i].oy;
            spawnEnemyEngineFlame(nozzleX, nozzleY, vx2, vy2,
                engines[i].r, engines[i].g, engines[i].b,
                engines[i].radius, 1);
        }
    }

    updateEnemyEngineFlame();
}

void enemyDisp() {
    // 表示に使うハンドルを先に決める
    int handle = (StateManager::GetState() != Joutai::Win)
        ? imageData[img_enemy].handle
        : imageData[img_enemyDestroyed].handle;

    // 画像サイズを取得
    int w, h;
    GetGraphSize(handle, &w, &h);

    // 透明度のパラメータ（お好みで調整）
    const double FAR_DIST = 120.0;     // この距離以遠は完全不透明
    const double NEAR_DIST = 40.0;    // この距離以内は最も透明
    const int    MIN_ALPHA = 64;      // 最も近いときのアルファ値（0～255）
    const int    MAX_ALPHA = 255;     // 遠いときのアルファ値（完全不透明）


    // 2 体目が居るなら下に描画
    int alpha2 = 255;
    if (enemy.x2 > -999.0) {
        int handle2 = (StateManager::GetState() != Joutai::Win)
            ? imageData[img_enemy2].handle
            : imageData[img_enemyDestroyed2].handle;

        // 自機と敵機の距離を計算
        double dx2 = player.x - enemy.x2;
        double dy2 = player.y - enemy.y2;
        double distance2 = sqrt(dx2 * dx2 + dy2 * dy2);

        // 敵機のアルファ値を計算
        if (distance2 >= FAR_DIST) {
            alpha2 = MAX_ALPHA;
        }
        else if (distance2 <= NEAR_DIST) {
            alpha2 = MIN_ALPHA;
        }
        else {
            // 距離に比例してアルファ値を線形補間（近いほど小さい値＝透明）
            double t = (distance2 - NEAR_DIST) / (FAR_DIST - NEAR_DIST); // 0.0～1.0
            alpha2 = (int)(MIN_ALPHA + t * (MAX_ALPHA - MIN_ALPHA));
        }

        // アルファが255未満のときだけ半透明描画を設定
        if (alpha2 < 255) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha2);
        }

        // 中心座標になるように描画
        DrawGraph((int)(enemy.x2 - w / 2), (int)(enemy.y2 - h / 2), handle2, TRUE);

        // 描画モードを元に戻す
        if (alpha2 < 255) {
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }


    // 自機と敵機の距離を計算
    double dx = player.x - enemy.x;
    double dy = player.y - enemy.y;
    double distance = sqrt(dx * dx + dy * dy);

    // 敵機のアルファ値を計算
    int alpha;
    if (distance >= FAR_DIST) {
        alpha = MAX_ALPHA;
    }
    else if (distance <= NEAR_DIST) {
        alpha = MIN_ALPHA;
    }
    else {
        // 距離に比例してアルファ値を線形補間（近いほど小さい値＝透明）
        double t = (distance - NEAR_DIST) / (FAR_DIST - NEAR_DIST); // 0.0～1.0
        alpha = (int)(MIN_ALPHA + t * (MAX_ALPHA - MIN_ALPHA));
    }

    // アルファが255未満のときだけ半透明描画を設定
    if (alpha < 255) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    }

    // 中心座標になるように描画
    DrawGraph((int)(enemy.x - w / 2), (int)(enemy.y - h / 2), handle, TRUE);

    // 描画モードを元に戻す
    if (alpha < 255) {
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
   

    // 炎の加算強度を距離で二乗減衰させる
    double ratio = alpha / 255.0;                     // 1.0（遠）～ 約0.25（近）
    int flameBlend = (int)(100.0 * ratio * ratio);    // 100（遠）～ 約6（近）
    double ratio2 = alpha2 / 255.0;                   // 1.0（遠）～ 約0.25（近）
    int flameBlend2 = (int)(100.0 * ratio2 * ratio2); // 100（遠）～ 約6（近）
    drawEnemyEngineFlame(flameBlend, flameBlend2);    // 引数を追加した関数に渡す

    drawExplosion();
}

void enemyHit() {
    if (isMuteki) return;

    double dx = player.x - enemy.x;
    double dy = player.y - enemy.y;
    double r = imageData[img_player].radiusX + imageData[img_enemy].radiusX;
    if (dx * dx + dy * dy < r * r) {
        if (CheckSoundMem(sound_playerDestroyed)) StopSoundMem(sound_playerDestroyed);
        PlaySoundMem(sound_playerDestroyed, DX_PLAYTYPE_BACK);
        StateManager::ChangeState(Joutai::Lose);
        return;
    }

    double dx2 = player.x - enemy.x2;
    double dy2 = player.y - enemy.y2;
    if (dx2 * dx2 + dy2 * dy2 < r * r) {
        if (CheckSoundMem(sound_playerDestroyed)) StopSoundMem(sound_playerDestroyed);
        PlaySoundMem(sound_playerDestroyed, DX_PLAYTYPE_BACK);
        StateManager::ChangeState(Joutai::Lose);
    }
}