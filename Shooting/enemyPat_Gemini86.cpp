// enemyPat_IssenKagenui.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：一閃・影縫い
static void ShotIssenKagenui(sEnemyShotSet* pSet)
{
    // 状態管理: pSet->count でフェーズ進行
    // pSet->muki はダッシュ方向（ターゲットへの角度）を保持する

    // 1. 予兆と誘導（ロックオン）
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 自機への角度を計算して保持
        pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
    }

    // 予兆線（赤レーザー）: 細く真っ直ぐ飛ばす
    if (pSet->count > 0 && pSet->count <= 50 && pSet->count % 2 == 0) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = enemy.x;
        pShot->y = enemy.y;
        pShot->muki = pSet->muki;
        pShot->speed = 20.0;
        pShot->kind = img_enemyShotLaser[0]; // 0:赤
        pShot->param_i[0] = 1; // 動くフラグ

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    // 全方位リング展開（青小玉）
    if (pSet->count == 20) {
        for (int i = 0; i < 24; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = enemy.x;
            pShot->y = enemy.y;
            pShot->muki = i * DX_PI * 2.0 / 24.0;
            pShot->speed = 1.5;
            pShot->kind = img_enemyShotSmallBall[4]; // 4:青
            pShot->param_i[0] = 1; // 動くフラグ

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 2. 超高速ダッシュと残留弾設置（一閃）
    if (pSet->count >= 60 && pSet->count <= 65) {
        // 敵の移動（6フレームで超高速移動）
        double dash_speed = 35.0;
        enemy.x += dash_speed * cos(pSet->muki);
        enemy.y += dash_speed * sin(pSet->muki);

        // 軌跡に弾を置く（シアンの中楕円弾）
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = enemy.x;
        pShot->y = enemy.y;
        pShot->muki = pSet->muki;
        pShot->speed = 0.0; // 停滞
        pShot->kind = img_enemyShotMediumOval[3]; // 3:シアン
        pShot->param_i[0] = 0; // 0:停滞状態フラグ

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    // 3. 近接判定＋時間差クロス爆発（影縫い）
    if (pSet->count == 80) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 近接斬撃（マゼンタの大玉を扇状に広範囲へ）
        for (int i = -5; i <= 5; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = enemy.x;
            pShot->y = enemy.y;
            pShot->muki = pSet->muki + i * DX_PI / 15.0;
            pShot->speed = 12.0;
            pShot->kind = img_enemyShotLargeBall[5]; // 5:マゼンタ
            pShot->param_i[0] = 1; // 動くフラグ

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        // 軌跡弾（静止光弾）の左右発射
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) { // 軌跡弾のみ対象
                pShot->param_i[0] = 1; // 動く状態へ変更
                pShot->muki = pSet->muki + DX_PI / 2.0; // ダッシュ軸から右へ90度
                pShot->speed = 6.0;

                // 左へ向かう弾を複製
                sEnemyShot* pNewShot = new sEnemyShot;
                pNewShot->x = pShot->x;
                pNewShot->y = pShot->y;
                pNewShot->muki = pSet->muki - DX_PI / 2.0; // ダッシュ軸から左へ90度
                pNewShot->speed = 6.0;
                pNewShot->kind = img_enemyShotMediumOval[3]; // 3:シアン
                pNewShot->param_i[0] = 1; // 動くフラグ

                pNewShot->prev = pSet->pEnemyShotHead->prev;
                pNewShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pNewShot;
                pSet->pEnemyShotHead->prev = pNewShot;
            }
            pShot = pShot->next;
        }
    }

    // 4. 離脱と制限
    if (pSet->count >= 110 && pSet->count < 140) {
        // 初期位置付近(240, 80)へバックステップ
        double back_angle = atan2(80.0 - enemy.y, 240.0 - enemy.x);
        enemy.x += 4.0 * cos(back_angle);
        enemy.y += 4.0 * sin(back_angle);
    }

    // 離脱中の自機狙い3連射
    if (pSet->count == 110 || pSet->count == 120 || pSet->count == 130) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double angle = atan2(player.y - enemy.y, player.x - enemy.x);
        for (int i = -1; i <= 1; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = enemy.x;
            pShot->y = enemy.y;
            pShot->muki = angle + i * 0.15;
            pShot->speed = 4.0;
            pShot->kind = img_enemyShotDiamond[6]; // 6:白
            pShot->param_i[0] = 1; // 動くフラグ

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // === 弾の移動処理 ===
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) { // 軌跡用の停滞弾(0)以外は移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_CloseCombat_Gemini()
{
    if (count == 1) {
        // ゲーム画面の中央上部付近に配置
        enemy.x = 240.0;
        enemy.y = 80.0;

        // 事前の指示通り、勝手にHPは増やさず200のままとする
        enemy.maxHp = enemy.hp = 200;
    }

    // 200フレーム周期で行動を開始
    if (count % 200 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotIssenKagenui;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0; // 弾幕関数内で自機を狙って計算するため仮置き

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}