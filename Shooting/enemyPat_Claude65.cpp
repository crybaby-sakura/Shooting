// enemyPat_hydra.cpp
// ヒュドラモチーフ弾幕：「双首再生」
// 1本の首が伸びて毒弾（3way）を放った後、その先端付近から2本の首が
// 分岐して再生する。世代を重ねるごとに 1→2→4 本と扇状に増殖していき、
// PATTERN_PERIODフレームごとに世代0からパターン全体が反復する。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾幕：首の成長・毒弾発射・退場（再生準備）までを一括管理
// ------------------------------------------------------------
static const int    SEG_NUM = 20;   // 1本の首を構成する体節（鱗）の数
static const int    GROW_INTERVAL = 3;    // 体節が1つ伸びる間隔（フレーム）
static const int    FIRE_COUNTS[3] = { 60, 68, 76 }; // 毒弾（3way）を放つタイミング
static const int    FADE_START = 120;  // 首が外側へ溶けるように退場を始めるタイミング
static const double DRIFT_SPEED = 3.0;  // 退場時に外側へ流れる速さ
static const double WAVE_FREQ = 1.5;  // 首1本あたりの蛇行の波数
static const int    PATTERN_PERIOD = 240; // パターン全体（世代0〜2）を反復させる周期（フレーム）

static void ShotNeck(sEnemyShotSet* pEnemyShotSet)
{
    double rootX = pEnemyShotSet->x;
    double rootY = pEnemyShotSet->y;
    double baseAngle = pEnemyShotSet->muki;
    double L = pEnemyShotSet->param_d[0]; // 首の長さ
    double amplitude = pEnemyShotSet->param_d[1]; // 蛇行の振幅
    double phase = pEnemyShotSet->param_d[2]; // 蛇行の位相（個体差）
    int    colorIdx = pEnemyShotSet->param_i[1]; // 世代ごとの色（緑→シアン→青）

    // 体節（鱗）を一定間隔で根元から先端へ伸ばしていく
    if (pEnemyShotSet->count % GROW_INTERVAL == 0) {
        int segIdx = pEnemyShotSet->count / GROW_INTERVAL;
        if (segIdx < SEG_NUM) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->param_i[0] = 0; // 0:体節  1:毒弾
            pShot->param_d[0] = (double)segIdx / (double)(SEG_NUM - 1); // 首上の位置 s(0〜1)
            pShot->param_d[3] = (double)pEnemyShotSet->count;           // 生成時のセットカウント
            // 弾の色一覧: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
            pShot->kind = img_enemyShotScale[colorIdx];
            pShot->x = rootX;
            pShot->y = rootY;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 頭（首の先端）から自機狙いの毒弾を3wayで放つ
    for (int k = 0; k < 3; k++) {
        if (pEnemyShotSet->count == FIRE_COUNTS[k]) {
            double s = 1.0;
            double wave = amplitude * sin(2.0 * DX_PI * WAVE_FREQ * s + pEnemyShotSet->count * 0.05 + phase);
            double tipX = rootX + cos(baseAngle) * s * L - sin(baseAngle) * wave;
            double tipY = rootY + sin(baseAngle) * s * L + cos(baseAngle) * wave;
            double aimAngle = atan2(player.y - tipY, player.x - tipX);

            // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            for (int j = -1; j <= 1; j++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->param_i[0] = 1; // 毒弾
                pShot->param_d[0] = aimAngle + j * 0.18; // 発射角（3way）
                pShot->param_d[1] = tipX;                // 発射位置X
                pShot->param_d[2] = tipY;                // 発射位置Y
                pShot->kind = img_enemyShotMediumBall[2]; // 緑＝毒
                pShot->x = tipX;
                pShot->y = tipY;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 既存の弾（体節・毒弾）を毎フレーム式から再計算して更新（速度積分はしない）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 毒弾：発射位置から直進。countから毎フレーム位置を算出する。
            double aimAngle = pShot->param_d[0];
            double fireX = pShot->param_d[1];
            double fireY = pShot->param_d[2];
            const double speed = 2.2;
            pShot->x = fireX + speed * cos(aimAngle) * pShot->count;
            pShot->y = fireY + speed * sin(aimAngle) * pShot->count;
            pShot->muki = aimAngle; // 向き＝実際の飛翔方向
        }
        else {
            // 体節：胴体の直線＋蛇行カーブ＋（一定時間経過後は）外側への退場
            double s = pShot->param_d[0];
            double effCount = pShot->param_d[3] + pShot->count; // このセット基準の経過フレーム

            double wave = amplitude * sin(2.0 * DX_PI * WAVE_FREQ * s + effCount * 0.05 + phase);
            double px = rootX + cos(baseAngle) * s * L - sin(baseAngle) * wave;
            double py = rootY + sin(baseAngle) * s * L + cos(baseAngle) * wave;

            // 向き：sをわずかに先端側へ進めた位置との差分から、首に沿った接線方向を求める
            const double ds = 0.01;
            double waveNext = amplitude * sin(2.0 * DX_PI * WAVE_FREQ * (s + ds) + effCount * 0.05 + phase);
            double pxNext = rootX + cos(baseAngle) * (s + ds) * L - sin(baseAngle) * waveNext;
            double pyNext = rootY + sin(baseAngle) * (s + ds) * L + cos(baseAngle) * waveNext;
            pShot->muki = atan2(pyNext - py, pxNext - px);

            if (effCount > FADE_START) {
                double fadeT = effCount - FADE_START;
                px += cos(baseAngle) * fadeT * DRIFT_SPEED;
                py += sin(baseAngle) * fadeT * DRIFT_SPEED;
            }

            pShot->x = px;
            pShot->y = py;
        }

        pShot = pShot->next;
    }
}

// 首の分岐予定を保持する構造体（発生タイミング・根元座標・角度・世代）
struct sNeckSpawn {
    int    spawnCount;
    double rootX, rootY;
    double baseAngle;
    int    generation;
};

// ------------------------------------------------------------
// 敵本体のパターン：「双首再生」
// ヒュドラの首を1本→2本→4本と再生・分岐させながら増殖させる。
// ------------------------------------------------------------
void EnemyPat_Hydra_Claude()
{
    static sNeckSpawn schedule[7];
    static int scheduleNum;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    else {
        // ヒュドラの胴体を左右にゆるく揺らす（式のみで駆動、速度積分はしない）
        enemy.x = 240.0 + 15.0 * sin(count * 0.02);
    }

    // PATTERN_PERIODフレームごとにスケジュールを再構築し、パターン全体を反復させる
    if ((count - 1) % PATTERN_PERIOD == 0) {
        const double L = 150.0;
        const double baseAngle0 = DX_PI / 2.0;             // 画面下方向
        const double split1 = 35.0 * DX_PI / 180.0;    // 第1分岐の開き角
        const double split2 = 22.0 * DX_PI / 180.0;    // 第2分岐の開き角
        const int    childDelay = 80;                     // 分岐（再生）までの間隔

        // 各世代の首の発生スケジュールを、周期の先頭からの相対フレーム数で計算しておく
        scheduleNum = 0;
        double rootX0 = enemy.x;
        double rootY0 = enemy.y + 10.0;
        schedule[scheduleNum++] = { 0, rootX0, rootY0, baseAngle0, 0 };

        double tipX0 = rootX0 + L * cos(baseAngle0);
        double tipY0 = rootY0 + L * sin(baseAngle0);
        double angleL1 = baseAngle0 - split1;
        double angleR1 = baseAngle0 + split1;
        schedule[scheduleNum++] = { childDelay, tipX0, tipY0, angleL1, 1 };
        schedule[scheduleNum++] = { childDelay, tipX0, tipY0, angleR1, 1 };

        double tipXL1 = tipX0 + L * cos(angleL1);
        double tipYL1 = tipY0 + L * sin(angleL1);
        double tipXR1 = tipX0 + L * cos(angleR1);
        double tipYR1 = tipY0 + L * sin(angleR1);
        int gen2Count = childDelay * 2;
        schedule[scheduleNum++] = { gen2Count, tipXL1, tipYL1, angleL1 - split2, 2 };
        schedule[scheduleNum++] = { gen2Count, tipXL1, tipYL1, angleL1 + split2, 2 };
        schedule[scheduleNum++] = { gen2Count, tipXR1, tipYR1, angleR1 - split2, 2 };
        schedule[scheduleNum++] = { gen2Count, tipXR1, tipYR1, angleR1 + split2, 2 };
    }

    int cycleLocalCount = (count - 1) % PATTERN_PERIOD;
    for (int i = 0; i < scheduleNum; i++) {
        if (cycleLocalCount == schedule[i].spawnCount) {
            // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotNeck;
            pEnemyShotSet->x = schedule[i].rootX;
            pEnemyShotSet->y = schedule[i].rootY;
            pEnemyShotSet->muki = schedule[i].baseAngle;
            pEnemyShotSet->param_i[0] = schedule[i].generation;
            pEnemyShotSet->param_i[1] = (2 + schedule[i].generation) % 8; // 世代ごとの色：緑→シアン→青
            pEnemyShotSet->param_d[0] = 150.0;                // 首の長さ
            // GetRand(x) は 0〜x の x+1 種類を返すので、範囲の意図に注意して使用
            pEnemyShotSet->param_d[1] = 16.0 + GetRand(8);    // 蛇行の振幅（個体差）
            pEnemyShotSet->param_d[2] = GetRand(1000) / 1000.0 * 2.0 * DX_PI; // 蛇行の位相（個体差）

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}