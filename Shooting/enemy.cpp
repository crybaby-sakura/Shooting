#include "DxLib.h"
#include "gv.h"
#include "enemy.h"
#include "imgSoundLoad.h"
#include "stateManager.h"
#include "player.h"


#define MAX_SPARKS 500   // 同時表示最大数

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
    const int NUM_SPARKS = 12 + GetRand(8);
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

        double angle = (2.0 * 3.14159265) * GetRand(10000) / 10000.0;
        double speed = 1.0 + (double)(GetRand(100)) / 50.0;
        Spark* s = &sparkPool[idx];
        s->active = true;
        s->x = x;
        s->y = y;
        s->vx = cos(angle) * speed;
        s->vy = sin(angle) * speed - 1.5;
        s->maxLife = 15 + GetRand(10);
        s->life = s->maxLife;
        int r = 255;
        int g = 200 + GetRand(55);
        int b = 0 + GetRand(50);
        s->color = GetColor(r, g, b);
        sparkCount++;
    }
}

void enemyDisp() {
    // 表示に使うハンドルを先に決める
    int handle = (StateManager::GetState() != Joutai::Win)
        ? imageData[img_enemy[0]].handle
        : imageData[img_enemy[1]].handle;

    // 画像サイズを取得
    int w, h;
    GetGraphSize(handle, &w, &h);

    // 自機と敵機の距離を計算
    double dx = player.x - enemy.x;
    double dy = player.y - enemy.y;
    double distance = sqrt(dx * dx + dy * dy);

    // 透明度のパラメータ（お好みで調整）
    const double FAR_DIST = 80.0;     // この距離以遠は完全不透明
    const double NEAR_DIST = 40.0;    // この距離以内は最も透明
    const int    MIN_ALPHA = 64;      // 最も近いときのアルファ値（0～255）
    const int    MAX_ALPHA = 255;     // 遠いときのアルファ値（完全不透明）

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

    // ---------- 火花パーティクルの更新と描画 ----------
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

void enemyHit() {
    if (isMuteki) return;

    double dx = player.x - enemy.x;
    double dy = player.y - enemy.y;
    double r = imageData[img_player].radiusX + imageData[img_enemy[0]].radiusX;
    if (dx * dx + dy * dy < r * r) {
        if (CheckSoundMem(sound_playerDestroyed)) StopSoundMem(sound_playerDestroyed);
        PlaySoundMem(sound_playerDestroyed, DX_PLAYTYPE_BACK);
        StateManager::ChangeState(Joutai::Lose);
    }
}