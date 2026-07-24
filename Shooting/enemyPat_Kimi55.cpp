// enemyPat_tmp.cpp
// 弾幕：螺旋の花園（Spiral Garden）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：螺旋の花園（Spiral Garden）
// 4色の螺旋弾が画面を覆い、花弾を分岐させ、反射と星弾を生み、最後は一斉ホーミング
static void ShotSpiralGarden(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // フェーズ定数（60fps想定）
    const int PHASE_SPIRAL_END = 180;   // 0〜180: 螺旋発射
    const int PHASE_BLOOM_END = 300;   // 60〜300: 花弾分岐期間
    const int PHASE_STAR_END = 480;   // 180〜480: 反射・星弾期間
    // 480〜: 散華（ホーミング）

    // 4色マップ：赤(0)、黄(1)、青(4)、マゼンタ(5)
    const int COLOR_MAP[4] = { 0, 1, 4, 5 };

    // ===== フェーズ0：蕾〜開花（螺旋弾連続発射） =====
    if (pEnemyShotSet->count <= PHASE_SPIRAL_END && pEnemyShotSet->count % 3 == 0) {
        // 予告音（最初のみ）
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        // 発射音（適度に重ねる）
        if (pEnemyShotSet->count % 12 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        for (int arm = 0; arm < 4; arm++) {
            pEnemyShot = new sEnemyShot;

            // 時間経過で回転しながら速度が増す螺旋
            double rotation = pEnemyShotSet->count * 0.08;
            double angle = rotation + arm * DX_PI / 2.0;
            double speed = 1.0 + (pEnemyShotSet->count / 180.0) * 2.5;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = speed;

            // 小玉で4色の螺旋を描く
            int col = COLOR_MAP[arm];
            pEnemyShot->kind = img_enemyShotSmallBall[col];

            // パラメータ初期化
            pEnemyShot->param_i[0] = arm;   // 腕番号（0〜3）
            pEnemyShot->param_i[1] = 0;       // 状態（0:未分岐 1:花弾済 2:星弾済）
            pEnemyShot->param_i[2] = col;     // 色インデックス
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = speed;

            // リスト追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ===== 散華開始時：全螺旋弾を中楕円弾に変化させ、重い効果音 =====
    if (pEnemyShotSet->count == PHASE_STAR_END + 1) {
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] >= 0) { // 螺旋弾本体のみ
                p->kind = img_enemyShotMediumOval[p->param_i[2]];
            }
            p = p->next;
        }
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // ===== 弾の移動・特殊処理 =====
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        // --- フェーズ1：開花（花弾分岐）---
        // 螺旋弾が一定時間経過で、6方向に菱形弾を放射
        if (pEnemyShotSet->count >= 60 && pEnemyShotSet->count <= PHASE_BLOOM_END) {
            if (pShot->param_i[0] >= 0 && pShot->param_i[1] == 0 && pShot->count >= 40 && pShot->count % 40 == 0) {
                for (int i = 0; i < 6; i++) {
                    sEnemyShot* pFlower = new sEnemyShot;
                    double flowerAngle = i * DX_PI / 3.0 + pEnemyShotSet->count * 0.03;
                    pFlower->x = pShot->x;
                    pFlower->y = pShot->y;
                    pFlower->muki = flowerAngle;
                    pFlower->speed = 1.3;
                    pFlower->kind = img_enemyShotDiamond[pShot->param_i[2]];
                    pFlower->param_i[0] = -1; // 花弾識別
                    pFlower->param_i[1] = 1;  // 再分岐防止

                    pFlower->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pFlower->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pFlower;
                    pEnemyShotSet->pEnemyShotHead->prev = pFlower;
                }
                pShot->param_i[1] = 1;

                if (pEnemyShotSet->count % 6 == 1) {
                    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                }
            }
        }

        // --- フェーズ2：満開（画面端反射＆星弾生成）---
        //if (pEnemyShotSet->count > PHASE_SPIRAL_END && pEnemyShotSet->count <= PHASE_STAR_END) {
        //    bool reflected = false;

        //    if (pShot->x < 10.0) {
        //        pShot->x = 10.0;
        //        pShot->muki = DX_PI - pShot->muki;
        //        reflected = true;
        //    }
        //    else if (pShot->x > 470.0) {
        //        pShot->x = 470.0;
        //        pShot->muki = DX_PI - pShot->muki;
        //        reflected = true;
        //    }
        //    if (pShot->y < 10.0) {
        //        pShot->y = 10.0;
        //        pShot->muki = -pShot->muki;
        //        reflected = true;
        //    }
        //    else if (pShot->y > 470.0) {
        //        pShot->y = 470.0;
        //        pShot->muki = -pShot->muki;
        //        reflected = true;
        //    }

        //    // 反射時に星弾を生成（螺旋弾本体でまだ星弾を出していないもの）
        //    if (reflected && pShot->param_i[0] >= 0 && pShot->param_i[1] == 1) {
        //        for (int i = 0; i < 5; i++) {
        //            sEnemyShot* pStar = new sEnemyShot;
        //            // GetRand(360) は 0〜360 の 361 種類
        //            double starAngle = GetRand(360) / 180.0 * DX_PI;
        //            pStar->x = pShot->x;
        //            pStar->y = pShot->y;
        //            pStar->muki = starAngle;
        //            // GetRand(150) は 0〜150 の 151 種類
        //            pStar->speed = 0.7 + GetRand(150) / 200.0; // 0.7〜1.45
        //            pStar->kind = img_enemyShotMediumBall[pShot->param_i[2]];
        //            pStar->param_i[0] = -2; // 星弾識別
        //            pStar->param_i[1] = 1;

        //            pStar->prev = pEnemyShotSet->pEnemyShotHead->prev;
        //            pStar->next = pEnemyShotSet->pEnemyShotHead;
        //            pEnemyShotSet->pEnemyShotHead->prev->next = pStar;
        //            pEnemyShotSet->pEnemyShotHead->prev = pStar;
        //        }
        //        pShot->param_i[1] = 2; // 星弾生成済み
        //    }
        //}

        // --- フェーズ3：散華（ホーミング化）---
        if (pEnemyShotSet->count > PHASE_STAR_END) {
            if (pShot->param_i[0] >= 0) { // 螺旋弾本体
                double targetAngle = atan2(player.y - pShot->y, player.x - pShot->x);
                double diff = targetAngle - pShot->muki;
                while (diff > DX_PI)  diff -= 2.0 * DX_PI;
                while (diff < -DX_PI) diff += 2.0 * DX_PI;
                pShot->muki += diff * 0.05; // 5%ずつ向きを変える
                if (pShot->speed < 3.5) pShot->speed += 0.02; // 緩加速
            }
            else if (pShot->param_i[0] == -1) { // 花弾も弱ホーミング
                double targetAngle = atan2(player.y - pShot->y, player.x - pShot->x);
                double diff = targetAngle - pShot->muki;
                while (diff > DX_PI)  diff -= 2.0 * DX_PI;
                while (diff < -DX_PI) diff += 2.0 * DX_PI;
                pShot->muki += diff * 0.025;
                if (pShot->speed < 2.2) pShot->speed += 0.015;
            }
            // 星弾（-2）は直進のまま
        }

        // 基本移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_ThumbnailFriendly_Kimi()
{
    static int muki;
    static int shot_count;
    static int phase;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 90.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
        phase = 0;
    }
    else {
        // ゆるやかな左右移動
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 螺旋の花園を1回だけ発動
    if (count % 360 == 60 && phase == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpiralGarden;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        phase = 0;
    }
}