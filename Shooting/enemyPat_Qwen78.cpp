// enemyPat_BreathCorona.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 定数定義
static const double PI2 = DX_PI * 2.0;
static const double DEG_TO_RAD = DX_PI / 180.0;
static const double RAD_TO_DEG = 180.0 / DX_PI;

// 回転角速度 (18度/秒 -> ラジアン/フレーム @60fps)
static const double ROT_SPEED_RAD = (18.0 * DEG_TO_RAD) / 60.0 * 2;
// 呼吸周期 (4秒)
static const double BREATH_FREQ = DX_PI / (4.0 * 60.0) / 2;

// ---------------------------------------------------------
// ヘルパー: 弾をリストに追加
// ---------------------------------------------------------
static void AddShot(sEnemyShotSet* pSet, sEnemyShot* pShot) {
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// ---------------------------------------------------------
// 1. 花びら弾 (Petal Bullet)
// 回転する安全地帯を作る
// ---------------------------------------------------------
static void ShotPetal(sEnemyShotSet* pEnemyShotSet)
{
    // 生成処理 (約0.12秒 = 7フレームごと)
    if (pEnemyShotSet->count % 3 == 0) {
        if (pEnemyShotSet->count % 12 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // グローバルcountから現在の回転角と隙間を計算
        double t = count / 60.0; // 秒
        double baseAngle = DX_PI / 2.0 + (count * ROT_SPEED_RAD);

        // 隙間の幅 (18度 ± 6度)
        double gapDeg = 30.0 + 18.0 * sin(BREATH_FREQ * count);
        double gapRad = gapDeg * DEG_TO_RAD;

        double speed = 1.66; // 100px/s @60fps
        double angVel = ROT_SPEED_RAD; // 旋回速度

        for (int i = 0; i < 6; i++) {
            double armCenter = baseAngle + i * (DX_PI / 3.0);

            // 隙間の両端に弾を配置
            double angleL = armCenter - gapRad / 2.0;
            double angleR = armCenter + gapRad / 2.0;

            // 左端の弾
            sEnemyShot* pShotL = new sEnemyShot;
            pShotL->x = pEnemyShotSet->x;
            pShotL->y = pEnemyShotSet->y;
            pShotL->muki = angleL;
            pShotL->speed = speed;
            pShotL->kind = img_enemyShotScale[5]; // マゼンタ
            pShotL->param_d[0] = angVel; // 角速度を保存
            AddShot(pEnemyShotSet, pShotL);

            // 右端の弾
            sEnemyShot* pShotR = new sEnemyShot;
            pShotR->x = pEnemyShotSet->x;
            pShotR->y = pEnemyShotSet->y;
            pShotR->muki = angleR;
            pShotR->speed = speed;
            pShotR->kind = img_enemyShotScale[5]; // マゼンタ
            pShotR->param_d[0] = angVel;
            AddShot(pEnemyShotSet, pShotR);
        }
    }

    // 更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 旋回 (mukiを更新)
        pShot->muki += pShot->param_d[0] / 2;

        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 2. リング弾 (Ring with Notches)
// 安全地帯を強調する抜け穴付きリング
// ---------------------------------------------------------
static void ShotRing(sEnemyShotSet* pEnemyShotSet)
{
    // 生成処理 (2.0秒 = 120フレームごと)
    if (pEnemyShotSet->count % 120 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double t = count / 60.0;
        double baseAngle = DX_PI / 2.0 + (count * ROT_SPEED_RAD);

        // 花びらと同じ隙間計算
        double gapDeg = 18.0 + 6.0 * sin(BREATH_FREQ * count);
        double gapRad = gapDeg * DEG_TO_RAD;

        // 抜け穴の幅は花びらより少し広め (+8度) にとり、安心感を出す
        double notchHalfWidth = gapRad / 2.0 + (8.0 * DEG_TO_RAD);

        double speed = 1.33; // 80px/s
        double spawnRadius = 32.0;

        // 5度刻みでリング生成 (72発)
        for (int deg = 0; deg < 360; deg += 5) {
            double a = deg * DEG_TO_RAD;
            bool isNotch = false;

            // 6方向の抜け穴と一致するかチェック
            for (int i = 0; i < 6; i++) {
                double notchCenter = baseAngle + i * (DX_PI / 3.0);
                // 角度の差分を正規化して比較
                double diff = a - notchCenter;
                while (diff > DX_PI) diff -= PI2;
                while (diff < -DX_PI) diff += PI2;

                if (fabs(diff) < notchHalfWidth) {
                    isNotch = true;
                    break;
                }
            }

            if (!isNotch) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x + spawnRadius * cos(a);
                pShot->y = pEnemyShotSet->y + spawnRadius * sin(a);
                pShot->muki = a;
                pShot->speed = speed;
                pShot->kind = img_enemyShotSmallBall[3]; // シアン

                // 加速パラメータ
                // param_i[0]: 加速するタイミング (Setのcount)
                // param_d[0]: 加速後の速度
                pShot->param_i[0] = pEnemyShotSet->count + 48; // 0.8秒後(48f)に加速
                pShot->param_d[0] = 1.83; // 110px/s

                AddShot(pEnemyShotSet, pShot);
            }
        }
    }

    // 更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 加速処理
        if (pShot->param_i[0] > 0 && pEnemyShotSet->count >= pShot->param_i[0]) {
            pShot->speed = pShot->param_d[0];
            pShot->param_i[0] = 0; // フラグクリア
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 3. ニードル弾 (Graze Needle)
// グレイズを誘う狙い弾
// ---------------------------------------------------------
static void ShotNeedle(sEnemyShotSet* pEnemyShotSet)
{
    // 生成処理 (2.0秒ごと、リングと位相をずらすため 60f オフセット)
    if (pEnemyShotSet->count % 120 == 60) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double aim = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double speed = 3.66; // 220px/s

        // 3発散布 (-10, 0, +10 度)
        double offsets[] = { -10.0 * DEG_TO_RAD, 0.0, 10.0 * DEG_TO_RAD };

        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = aim + offsets[i];
            pShot->speed = speed;
            pShot->kind = img_enemyShotBullet[1]; // 黄
            AddShot(pEnemyShotSet, pShot);
        }
    }

    // 更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 敵本体パターン
// ---------------------------------------------------------
void EnemyPat_TheMostFun_Qwen()
{
    static sEnemyShotSet* pSetPetal = nullptr;
    static sEnemyShotSet* pSetRing = nullptr;
    static sEnemyShotSet* pSetNeedle = nullptr;

    if (count == 1) {
        // ボス初期化
        enemy.x = 240.0;
        enemy.y = 190.0; // 画面上部中央
        enemy.maxHp = enemy.hp = 100;

        // --- 花びら弾セット初期化 ---
        pSetPetal = new sEnemyShotSet;
        pSetPetal->count = 0;
        pSetPetal->patternFunc = ShotPetal;
        pSetPetal->x = enemy.x;
        pSetPetal->y = enemy.y;
        pSetPetal->muki = 0;
        pSetPetal->kind = 0;
        pSetPetal->pEnemyShotHead = new sEnemyShot;
        pSetPetal->pEnemyShotHead->prev = pSetPetal->pEnemyShotHead;
        pSetPetal->pEnemyShotHead->next = pSetPetal->pEnemyShotHead;

        pSetPetal->prev = enemyShotSetHead.prev;
        pSetPetal->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetPetal;
        enemyShotSetHead.prev = pSetPetal;

        // --- リング弾セット初期化 ---
        pSetRing = new sEnemyShotSet;
        pSetRing->count = 0;
        pSetRing->patternFunc = ShotRing;
        pSetRing->x = enemy.x;
        pSetRing->y = enemy.y;
        pSetRing->muki = 0;
        pSetRing->kind = 0;
        pSetRing->pEnemyShotHead = new sEnemyShot;
        pSetRing->pEnemyShotHead->prev = pSetRing->pEnemyShotHead;
        pSetRing->pEnemyShotHead->next = pSetRing->pEnemyShotHead;

        pSetRing->prev = enemyShotSetHead.prev;
        pSetRing->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetRing;
        enemyShotSetHead.prev = pSetRing;

        // --- ニードル弾セット初期化 ---
        pSetNeedle = new sEnemyShotSet;
        pSetNeedle->count = 0;
        pSetNeedle->patternFunc = ShotNeedle;
        pSetNeedle->x = enemy.x;
        pSetNeedle->y = enemy.y;
        pSetNeedle->muki = 0;
        pSetNeedle->kind = 0;
        pSetNeedle->alive = 99999;
        pSetNeedle->pEnemyShotHead = new sEnemyShot;
        pSetNeedle->pEnemyShotHead->prev = pSetNeedle->pEnemyShotHead;
        pSetNeedle->pEnemyShotHead->next = pSetNeedle->pEnemyShotHead;

        pSetNeedle->prev = enemyShotSetHead.prev;
        pSetNeedle->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetNeedle;
        enemyShotSetHead.prev = pSetNeedle;
    }
    else {
        // ボスの移動 (ゆっくり左右に揺れる)
        enemy.x = 240.0 + 120.0 * sin(count * 0.008);

        // 各ShotSetの座標をボスに追従させる
        if (pSetPetal) { pSetPetal->x = enemy.x; pSetPetal->y = enemy.y; }
        if (pSetRing) { pSetRing->x = enemy.x;  pSetRing->y = enemy.y; }
        if (pSetNeedle) { pSetNeedle->x = enemy.x; pSetNeedle->y = enemy.y; }
    }
}