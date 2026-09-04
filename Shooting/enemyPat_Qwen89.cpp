// enemyPat_Tmp.cpp
// ジャンプスケアモチーフ弾幕『暗闇からの顔面突進』実装

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ジャンプスケア『暗闇からの顔面突進』
static void ShotFaceRush(sEnemyShotSet* pEnemyShotSet)
{
    int c = pEnemyShotSet->count;

    // ============================================================
    // Phase 0: 前準備（緊張と油断） c: 0 ～ 119
    // ============================================================
    if (c < 120) {
        // 60フレームごとに「目」のような弾を生成し、ゆっくり自機を追跡させる
        if (c % 60 == 0) {
            // 予告音で緊張感を高める
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            for (int i = 0; i < 2; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                // 敵の左右に配置して「目」を表現
                pShot->x = pEnemyShotSet->x + (i == 0 ? -40.0 : 40.0);
                pShot->y = pEnemyShotSet->y;
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->speed = 1.0; // ゆっくりとした不気味な速度
                pShot->kind = img_enemyShotLargeBall[6]; // 白の大玉

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }
    // ============================================================
    // Phase 1: トリガー（ジャンプスケア発動） c == 120
    // ============================================================
    else if (c == 120) {
        // 極大の効果音で視覚的衝撃を補完
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double base_angle = GetRand(100) / 100.0 * DX_PI * 2.0;

        // 1. 視界の破壊：画面中央から白と黒の大玉を爆発的にばら撒く
        //    白と黒が瞬時に交差することで、専用エフェクトなしで「フラッシュ（明滅）」錯覚を起こす
        for (int i = 0; i < 16; i++) {
            double angle = i * (DX_PI * 2.0 / 16.0);
            int color = (i % 2 == 0) ? 6 : 7; // 白(6)と黒(7)を交互に

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = 240.0; // 画面中央
            pShot->y = 240.0;
            pShot->muki = angle;
            pShot->speed = 8.0; // 超高速で視界を横切る
            pShot->kind = img_enemyShotLargeBall[color];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 2. 個人空間への侵入：自機判定枠のギリギリ外側から赤い鱗弾を8方向に超高速拡散
        //    「外から来る」のではなく「自分の顔の前で爆発した」ような近接恐怖を演出
        for (int i = 0; i < 8*2; i++) {
            double angle = base_angle + i * (DX_PI * 2.0 / 8.0/2);

            sEnemyShot* pShot = new sEnemyShot;
            // 自機の中心から半径15ピクセルの地点から発生させる
            pShot->x = player.x + cos(angle) * 15.0;
            pShot->y = player.y + sin(angle) * 15.0;
            pShot->muki = angle;
            pShot->speed = 10.0; // 非常に高速な拡散
            pShot->kind = img_enemyShotScale[0]; // 赤(0)の鱗弾（トゲトゲした恐怖感）

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 3. 捕食：画面の最上部と最下部から白の短レーザーの壁を迫らせる
        //    ただし、自機から見た角度が「赤鱗弾が飛ぶ8方向」に近い場合は壁を生成しない
        //    これにより、「赤い弾の軌道（隙間）に逃げ込む」ことが唯一の生存手段となる
        for (int x = 0; x <= 480; x += 10) {
            // --- 上部からの壁 ---
            double angleFromPlayerTop = atan2(-40.0 - player.y, (double)x - player.x);
            bool isSafeTop = false;
            for (int i = 0; i < 8*2; i++) {
                double safeAngle = base_angle + i * (DX_PI * 2.0 / 8.0/2);
                double diff = fmod(angleFromPlayerTop - safeAngle, DX_PI * 2.0);
                if (diff > DX_PI) diff -= DX_PI * 2.0; // 角度の最短距離を計算
                if (fabs(diff) < DX_PI / 12.0 /2) { // 15度以内なら安全地帯とみなして生成しない
                    isSafeTop = true;
                    break;
                }
            }
            if (!isSafeTop) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = (double)x;
                pShot->y = -40.0;
                pShot->muki = DX_PI / 2.0; // 下向き
                pShot->speed = 6.0 - 1;
                pShot->kind = img_enemyShotLaser[6]; // 白(6)の短レーザー
                pShot->margin = 70;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }

            // --- 下部からの壁 ---
            double angleFromPlayerBottom = atan2(520.0 - player.y, (double)x - player.x);
            bool isSafeBottom = false;
            for (int i = 0; i < 8*2; i++) {
                double safeAngle = base_angle + i * (DX_PI * 2.0 / 8.0/2);
                double diff = fmod(angleFromPlayerBottom - safeAngle, DX_PI * 2.0);
                if (diff > DX_PI) diff -= DX_PI * 2.0;
                if (fabs(diff) < DX_PI / 12.0 /2) {
                    isSafeBottom = true;
                    break;
                }
            }
            if (!isSafeBottom) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = (double)x;
                pShot->y = 520.0;
                pShot->muki = -DX_PI / 2.0; // 上向き
                pShot->speed = 6.0 - 1;
                pShot->kind = img_enemyShotLaser[6]; // 白(6)の短レーザー
                pShot->margin = 70;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // ============================================================
    // 弾の移動処理 (全フェーズで実行)
    // ============================================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_JumpScare_Qwen()
{
    static int muki = 1;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 敵の動き（サンプルに合わせて左右移動）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    if (count % 400 == 1) {
        // 弾幕セットの生成は count == 1 のときだけ行い、
        // 以降は ShotFaceRush 内の pEnemyShotSet->count に応じて処理が分岐する
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFaceRush;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}