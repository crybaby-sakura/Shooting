// enemyPat_Tmp.cpp
// 高難易度弾幕パターン：次元崩壊の万華鏡（カレイド・アポカリプス）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// レイヤー1：凍結する幾何学（固定弾の迷宮）
// ============================================================
static void ShotKaleidoWall(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // リサージュ曲線風にばら撒く
        for (int i = 0; i < 12; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double angle = pEnemyShotSet->muki + (i * DX_PI * 2.0 / 12.0);
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 3.0 + GetRand(100) / 100.0; // 3.0 ~ 4.0

            // 移動中はシアンの中楕円弾
            pEnemyShot->kind = img_enemyShotMediumOval[3];

            // param_i[0]: 状態 (0:移動中, 1:固定中)
            pEnemyShot->param_i[0] = 0;
            // param_i[1]: 固定化するまでのカウント
            pEnemyShot->param_i[1] = 60 + GetRand(60); // 60~120フレームで固定

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 移動中
            pShot->speed = pShot->param_i[1] * 0.05;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->param_i[1]--;

            // 一定時間経過で固定化
            if (pShot->param_i[1] <= 0) {
                pShot->param_i[0] = 1; // 固定状態へ
                pShot->param_i[1] = 240;
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotLargeBall[6]; // 固定化したら白い大玉に変化（壁として強調）
            }
        }
        else if (pShot->param_i[0] == 1) {
            pShot->param_i[1]--;
            if (pShot->param_i[1] <= 0) {
                pShot->margin = -9999;
            }
        }
        pShot = pShot->next;
    }
}

// ============================================================
// レイヤー2：重力に歪む光線（加速・軌道変化弾）
// ============================================================
static void ShotGravityBeam(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        // プレイヤー方向へ発射（少しばらつきを持たせる）
        pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(40) - 20) / 100.0 * DX_PI;
        pEnemyShot->speed = 3.0 + GetRand(200) / 100.0; // 6.0 ~ 8.0

        // 極細の青い銃弾（レーザー状の視覚効果）
        pEnemyShot->kind = img_enemyShotLaser[4];
        pEnemyShot->margin = 40;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double gravityX = 0.0, gravityY = 0.0;

        // 画面内の「固定弾」(param_i[0] == 1) を重力源として探索
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            sEnemyShot* pTarget = pSet->pEnemyShotHead->next;
            while (pTarget != pSet->pEnemyShotHead) {
                if (pTarget->param_i[0] == 1) { // 固定弾のみを対象
                    double dx = pTarget->x - pShot->x;
                    double dy = pTarget->y - pShot->y;
                    double dist = sqrt(dx * dx + dy * dy);

                    // 距離が1〜150ドットの範囲で引力を計算
                    if (dist > 1.0 && dist < 150.0) {
                        double force = 80.0 / (dist * dist); // 距離の二乗に反比例する力
                        gravityX += (dx / dist) * force;
                        gravityY += (dy / dist) * force;
                    }
                }
                pTarget = pTarget->next;
            }
            pSet = pSet->next;
        }

        // 速度ベクトルに重力を加算して向きを補正
        double targetMuki = atan2(pShot->speed * sin(pShot->muki) + gravityY,
            pShot->speed * cos(pShot->muki) + gravityX);

        // 急激な変化を防ぎ、滑らかに曲がるように補間
        double diff = targetMuki - pShot->muki;
        while (diff > DX_PI) diff -= DX_PI * 2.0;
        while (diff < -DX_PI) diff += DX_PI * 2.0;
        pShot->muki += diff * 0.15;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
// レイヤー3：時空の残響（遅延・擬態弾）
// ============================================================
static void ShotPhantomEcho(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(100) - 50;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(100) - 50;

            // プレイヤー方向へゆっくり進む（少しばらつき）
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x) + (GetRand(60) - 30) / 100.0 * DX_PI;
            pEnemyShot->speed = 1.5 + GetRand(100) / 100.0;

            // 擬態中は白い鱗弾
            pEnemyShot->kind = img_enemyShotScale[6];
            pEnemyShot->param_i[0] = 0; // 0:擬態中

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 擬態中：プレイヤーに近づくと実弾化
            double dx = player.x - pShot->x;
            double dy = player.y - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist < 120.0) {
                pShot->param_i[0] = 1; // 実弾化フラグ
                pShot->kind = img_enemyShotDiamond[0]; // 赤い菱形弾に変化
                pShot->speed = 5.0; // 加速して突進
                pShot->muki = atan2(dy, dx); // プレイヤー方向へ向きを修正
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else {
            // 実弾化後：直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン制御
// ============================================================
void EnemyPat_TooChaotic_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200; // 高難易度のためHPを多めに設定

        // 予告音を鳴らす
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // リサージュ曲線移動で、予測不能かつ美しい軌道を描く
        enemy.x = 240.0 + 150.0 * sin(count * 0.02);
        enemy.y = 100.0 + 80.0 * cos(count * 0.03);
    }

    // レイヤー1：固定弾の迷宮（40フレーム周期）
    if (count % 40 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKaleidoWall;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = count * 0.05; // 発射角度を時間とともに回転させる
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // レイヤー2：重力に歪む光線（15フレーム周期）
    if (count % 30 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGravityBeam;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 1;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // レイヤー3：時空の残響（60フレーム周期）
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPhantomEcho;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 2;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}