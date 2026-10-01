#include "gv.h"
#include <cmath>

// 外部定義されている前提の定数（必要に応じて調整してください）
#ifndef DX_PI
#define DX_PI 3.14159265358979323846
#endif
#ifndef DX_PLAYTYPE_BACK
#define DX_PLAYTYPE_BACK 0
#endif

// 画像とサウンドの変数は他ファイルで定義されている前提とします
extern int sound_enemyShot_light;
extern int sound_enemyShot_medium;
extern int sound_enemyShot_heavy;
extern int sound_enemyCharge;
extern int img_enemyShotSmallBall[9];
extern int img_enemyShotMediumBall[9];
extern int img_enemyShotLargeBall[9];
extern int img_enemyShotScale[9];
extern int img_enemyShotDiamond[9];

// 弾をリストに追加するヘルパー関数
static void AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind, double base_speed = 0.0) {
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->param_d[0] = base_speed; // 速度変化計算用のベース速度を保存

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 弾幕：カーペット（地弾）の制御
static void PatternCarpet(sEnemyShotSet* pSet) {
    // === 1. 弾の発射 ===
    if (pSet->count % 12 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 発射角の揺れ幅（サイン波で波打たせる）
        double swing = 0.6 * sin(pSet->count * DX_PI / 70.0);

        // 青の菱形弾（右へうねる層）
        double angle1 = DX_PI / 2.0 + swing;
        for (int i = -3; i <= 3; i++) {
            AddShot(pSet, enemy.x, enemy.y, angle1 + i * (DX_PI / 8.0), 1.8, img_enemyShotDiamond[4], 1.8);
        }

        // マゼンタの菱形弾（左へうねる層、逆位相で交差させる）
        double angle2 = DX_PI / 2.0 - swing;
        for (int i = -3; i <= 3; i++) {
            AddShot(pSet, enemy.x, enemy.y, angle2 + i * (DX_PI / 8.0), 1.8, img_enemyShotDiamond[5], 1.8);
        }
    }

    // アクセント弾（シアンの鱗弾、模様の骨組み）
    if (pSet->count % 6 == 0) {
        for (int i = -2; i <= 2; i++) {
            AddShot(pSet, enemy.x, enemy.y, DX_PI / 2.0 + i * (DX_PI / 5.0), 2.2, img_enemyShotScale[3], 2.2);
        }
    }

    // === 2. 弾の移動 ===
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // カーペット全体の呼吸（グローバル変数 count を使うことで全弾が同期して脈動する）
        double speed_factor = 0.6 + 0.4 * sin(count * DX_PI / 90.0);

        // 弾個別の横揺れ（布のひだを表現）
        double sway_speed = 0.4 * cos(pShot->count * DX_PI / 50.0 + pShot->muki);

        // 速度を合成
        double current_speed = pShot->param_d[0] * speed_factor;

        // 進行方向への移動
        pShot->x += current_speed * cos(pShot->muki);
        pShot->y += current_speed * sin(pShot->muki);

        // 横揺れ方向への移動（進行方向に対して垂直なベクトル）
        pShot->x += sway_speed * cos(pShot->muki + DX_PI / 2.0);
        pShot->y += sway_speed * sin(pShot->muki + DX_PI / 2.0);

        pShot = pShot->next;
    }
}

// 弾幕：流星（主攻撃）の制御
static void PatternMeteor(sEnemyShotSet* pSet) {
    // 予告音
    if (pSet->count % 120 == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // === 1. 弾の発射 ===
    if (pSet->count % 120 == 40) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // プレイヤーへの基本角度
        double angle_to_player = atan2(player.y - enemy.y, player.x - enemy.x);

        // V字交差する2本の流星
        for (int i = -1; i <= 1; i += 2) {
            // 自機狙いから少し角度をずらし、画面を交差するように逃がす
            double base_angle = angle_to_player + i * (DX_PI / 6.0);
            double meteor_speed = 4.5;

            // 先頭の大玉（黄色）
            AddShot(pSet, enemy.x, enemy.y, base_angle, meteor_speed, img_enemyShotLargeBall[1]);

            // 尾を引く中玉・小玉（黄色）
            for (int j = 1; j <= 6; j++) {
                int kind = (j < 4) ? img_enemyShotMediumBall[1] : img_enemyShotSmallBall[1];
                // 角度に合わせて後ろにずらして配置する
                double offset_x = -j * 15.0 * cos(base_angle);
                double offset_y = -j * 15.0 * sin(base_angle);
                AddShot(pSet, enemy.x + offset_x, enemy.y + offset_y, base_angle, meteor_speed, kind);
            }
        }
    }

    // === 2. 弾の移動 ===
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 流星はカーペットを切り裂くように直線で高速移動する
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン（指定関数名）
void EnemyPat_NightCarpet_Gemini()
{
    if (count == 1) {
        // ボス初期設定
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // スペルカード用に高めに設定

        // ----------------------------------------------------
        // カーペット用 ShotSet の登録
        // ----------------------------------------------------
        sEnemyShotSet* pCarpetSet = new sEnemyShotSet;
        pCarpetSet->count = 0;
        pCarpetSet->patternFunc = PatternCarpet;

        pCarpetSet->pEnemyShotHead = new sEnemyShot;
        pCarpetSet->pEnemyShotHead->prev = pCarpetSet->pEnemyShotHead;
        pCarpetSet->pEnemyShotHead->next = pCarpetSet->pEnemyShotHead;

        pCarpetSet->prev = enemyShotSetHead.prev;
        pCarpetSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pCarpetSet;
        enemyShotSetHead.prev = pCarpetSet;

        // ----------------------------------------------------
        // 流星用 ShotSet の登録
        // ----------------------------------------------------
        sEnemyShotSet* pMeteorSet = new sEnemyShotSet;
        pMeteorSet->count = 0;
        pMeteorSet->patternFunc = PatternMeteor;
        pMeteorSet->alive = 99999;

        pMeteorSet->pEnemyShotHead = new sEnemyShot;
        pMeteorSet->pEnemyShotHead->prev = pMeteorSet->pEnemyShotHead;
        pMeteorSet->pEnemyShotHead->next = pMeteorSet->pEnemyShotHead;

        pMeteorSet->prev = enemyShotSetHead.prev;
        pMeteorSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMeteorSet;
        enemyShotSetHead.prev = pMeteorSet;
    }

    // ボスの移動: 画面中央上部で優雅に8の字（または左右）に揺れ続ける
    enemy.x = 240.0 + 70.0 * sin(count * DX_PI / 150.0);
    enemy.y = 70.0 + 15.0 * sin(count * DX_PI / 75.0);
}