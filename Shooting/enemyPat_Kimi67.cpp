// enemyPat_tmp.cpp
// 弾幕：無限分岐のカオス・フラクタル

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const double MARGIN = 240.0;

// 弾幕パターン：無限分岐のカオス・フラクタル
static void ShotFractal(sEnemyShotSet* pEnemyShotSet)
{
    const int MAX_GENERATION = 3;      // 親(0)→子(1)→孫(2)→曾孫(3)
    const int SPLIT_INTERVAL = 45;     // 親弾の分裂までのフレーム
    const int HOMING_START = 140;    // ホーミング開始フレーム（セット生成後）
    const int FINAL_RUSH = 230;    // 最終加速開始フレーム
    const int LASER_TIMING = 110;    // レーザー壁生成タイミング

    sEnemyShot* pShot;
    sEnemyShot* pNext;

    // === 初期化：32発の親弾を「自機周囲包囲」形で発射 ===
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        double px = player.x;
        double py = player.y;
        double radius = 300.0; // 自機周囲の包囲半径

        for (int i = 0; i < 32; i++) {
            pShot = new sEnemyShot;

            // 自機を中心とした円周上32等分点を目指す角度
            double targetAngle = (2.0 * DX_PI * i) / 32.0;
            double tx = px + radius * cos(targetAngle);
            double ty = py + radius * sin(targetAngle);
            double angle = atan2(ty - pEnemyShotSet->y, tx - pEnemyShotSet->x);

            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = angle;
            pShot->speed = 2.0;
            pShot->count = 0;
            pShot->kind = img_enemyShotMediumBall[i % 8]; // 中玉、8色
            pShot->param_i[0] = 0;        // 世代
            pShot->param_i[1] = SPLIT_INTERVAL; // 分裂カウントダウン
            pShot->param_i[2] = 0;        // 予約
            pShot->param_i[3] = 0;        // 0:通常弾, 1:ghost(当たり判定なし推奨)
            pShot->margin = MARGIN;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // セットパラメータ初期化
        pEnemyShotSet->param_i[0] = MAX_GENERATION;
        pEnemyShotSet->param_i[1] = 0; // レーザー生成済みフラグ
    }

    // === レーザー壁生成（四隅から自機へ） ===
    if (pEnemyShotSet->count == LASER_TIMING && pEnemyShotSet->param_i[1] == 0) {
        pEnemyShotSet->param_i[1] = 1;

        // 効果音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double corners[4][2] = { {0.0, 0.0}, {480.0, 0.0}, {0.0, 480.0}, {480.0, 480.0} };
        for (int i = 0; i < 4; i++) {
            pShot = new sEnemyShot;
            pShot->x = corners[i][0];
            pShot->y = corners[i][1];
            pShot->muki = atan2(player.y - corners[i][1], player.x - corners[i][0]);
            pShot->speed = 1.0; // ゆっくり侵食
            pShot->count = 0;
            pShot->kind = img_enemyShotLaser[6]; // 白レーザー
            pShot->param_i[0] = 99; // レーザー識別用
            pShot->param_i[1] = 0;
            pShot->param_i[2] = 0;
            pShot->param_i[3] = 0;
            pShot->margin = MARGIN; // しっかり画面外まで消える

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // === 弾更新ループ ===
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pNext = pShot->next;

        // --- レーザー壁の更新 ---
        if (pShot->param_i[0] == 99) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pNext;
            continue;
        }

        // --- ghost（残像弾）の寿命管理 ---
        if (pShot->param_i[3] == 1) {
            pShot->param_i[1]--;
            if (pShot->param_i[1] <= 0) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                pShot = pNext;
                continue;
            }
        }

        // --- 分裂処理 ---
        // param_i[1] が 0 以下、かつ未分裂、かつ最大世代未満
        if (pShot->param_i[1] <= 0 && pShot->param_i[3] == 0 && pShot->param_i[0] < MAX_GENERATION) {
            int gen = pShot->param_i[0] + 1;
            double baseAngle = pShot->muki;
            double baseSpeed = pShot->speed * 1.3;

            for (int j = 0; j < 4; j++) {
                sEnemyShot* pChild = new sEnemyShot;

                // ±15°のランダム偏差。GetRand(100)は0～100なので、-50～50に正規化
                double randOffset = (GetRand(100) - 50) / 50.0 * (15.0 / 180.0 * DX_PI);
                double childAngle = baseAngle + randOffset;

                // 自機方向への微修正（現在角度と自機方向のブレンド）
                double toPlayer = atan2(player.y - pShot->y, player.x - pShot->x);
                // 角度差を -PI～+PI に正規化してブレンド
                double diff = toPlayer - childAngle;
                while (diff > DX_PI) diff -= 2.0 * DX_PI;
                while (diff < -DX_PI) diff += 2.0 * DX_PI;
                childAngle += diff * 0.25; // 25%だけ自機方向へ引き寄せ

                pChild->x = pShot->x;
                pChild->y = pShot->y;
                pChild->muki = childAngle;
                pChild->speed = baseSpeed;
                pChild->count = 0;

                // 世代に応じた見た目
                if (gen == 1)      pChild->kind = img_enemyShotSmallBall[j % 8];
                else if (gen == 2) pChild->kind = img_enemyShotScale[j % 8];
                else               pChild->kind = img_enemyShotDiamond[j % 8];

                pChild->param_i[0] = gen;
                // 次の分裂までのカウント（世代が深いほど短く）
                pChild->param_i[1] = (gen < MAX_GENERATION) ? (SPLIT_INTERVAL - gen * 8) : 0;
                pChild->param_i[2] = 0;
                pChild->param_i[3] = 0; // 通常弾
                pChild->margin = MARGIN;

                // 親の直後に挿入（リストの整合性維持）
                pChild->prev = pShot;
                pChild->next = pShot->next;
                pShot->next->prev = pChild;
                pShot->next = pChild;
            }

            // 親弾を削除
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
            pShot = pNext;
            continue;
        }

        // --- ホーミング（全弾が自機方向へゆるやかに曲がる） ---
        if (pEnemyShotSet->count >= HOMING_START) {
            double toPlayer = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = toPlayer - pShot->muki;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;
            pShot->muki += diff * 0.025; // 1フレームあたり2.5%方向修正
            pShot->speed *= 1.004;       // 微加速
        }

        // --- 最終加速（カオス収束） ---
        if (pEnemyShotSet->count >= FINAL_RUSH) {
            pShot->speed *= 1.015; // 急加速

            // 残像弾生成：曾孫弾のみ、一定間隔で
            if (pShot->param_i[0] == MAX_GENERATION && pEnemyShotSet->count % 4 == 0) {
                if (GetRand(100) < 10) { // 40%確率
                    sEnemyShot* pGhost = new sEnemyShot;
                    pGhost->x = pShot->x - pShot->speed * cos(pShot->muki) * 1.5;
                    pGhost->y = pShot->y - pShot->speed * sin(pShot->muki) * 1.5;
                    pGhost->muki = pShot->muki + (GetRand(20) - 10) / 180.0 * DX_PI;
                    pGhost->speed = pShot->speed * 0.7;
                    pGhost->count = 0;
                    pGhost->kind = img_enemyShotSmallBall[7]; // 黒小玉
                    pGhost->param_i[0] = MAX_GENERATION;
                    pGhost->param_i[1] = 12; // 12フレームの寿命
                    pGhost->param_i[2] = 0;
                    pGhost->param_i[3] = 1;    // ghostフラグ
                    pGhost->margin = MARGIN;

                    pGhost->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pGhost->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pGhost;
                    pEnemyShotSet->pEnemyShotHead->prev = pGhost;
                }
            }
        }

        // 分裂カウントダウン
        if (pShot->param_i[1] > 0) pShot->param_i[1]--;

        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_TheHardest_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 中央付近で微小に揺動
        enemy.x += 0.6 * (double)muki;
        if (count % 200 == 100) muki *= -1;
    }

    // メイン弾幕セットを生成（320フレーム周期）
    if (count % 320 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFractal;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 12.0;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}