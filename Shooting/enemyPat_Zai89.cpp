// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// プロトタイプ宣言
static void ShotHypnoticSpiral(sEnemyShotSet* pEnemyShotSet);
static void ShotJumpScare(sEnemyShotSet* pEnemyShotSet);

// 弾幕：催眠螺旋（導入）
static void ShotHypnoticSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 呼び出し元から渡されたフレーム数をもとに、ゆっくり回転する角度を算出
        double baseAngle = pEnemyShotSet->param_i[0] * 0.15;

        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + (DX_PI * 2 / 5) * i;
            pEnemyShot->speed = 1.5;

            // 赤い小玉で視界を狭め、集中させる
            pEnemyShot->kind = img_enemyShotSmallBall[0];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 弾幕：ジャンプスケア本体（睨みつける巨顔）
static void ShotJumpScare(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;
    if (pEnemyShotSet->count == 0) {

        // ==========================================
        // 左目の生成 (白い大玉を中心に、赤い大玉で瞳を形成)
        // ==========================================
        double eyeL_cx = 100.0, eyeL_cy = 120.0;

        // 中心の白玉
        pShot = new sEnemyShot;
        pShot->param_d[0] = 0.0; pShot->param_d[1] = 0.0; // 描画オフセット
        pShot->param_d[2] = eyeL_cx; pShot->param_d[3] = eyeL_cy; // 目の中心座標
        pShot->x = eyeL_cx; pShot->y = eyeL_cy;
        pShot->muki = atan2(player.y - eyeL_cy, player.x - eyeL_cx);
        pShot->speed = 0.3; // ゆっくり漂う
        pShot->param_i[0] = 1; // 追尾フラグ
        pShot->kind = img_enemyShotLargeBall[6]; // 白大玉
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;

        // 周囲の赤玉 (8個で囲む)
        for (int i = 0; i < 8; i++) {
            pShot = new sEnemyShot;
            pShot->param_d[0] = cos(DX_PI * 2 / 8 * i) * 45.0;
            pShot->param_d[1] = sin(DX_PI * 2 / 8 * i) * 45.0;
            pShot->param_d[2] = eyeL_cx; pShot->param_d[3] = eyeL_cy;
            pShot->x = eyeL_cx + pShot->param_d[0];
            pShot->y = eyeL_cy + pShot->param_d[1];
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 0.3;
            pShot->param_i[0] = 1; // 追尾フラグ
            pShot->kind = img_enemyShotLargeBall[0]; // 赤大玉
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }


        // ==========================================
        // 右目の生成
        // ==========================================
        double eyeR_cx = 380.0, eyeR_cy = 120.0;

        pShot = new sEnemyShot;
        pShot->param_d[0] = 0.0; pShot->param_d[1] = 0.0;
        pShot->param_d[2] = eyeR_cx; pShot->param_d[3] = eyeR_cy;
        pShot->x = eyeR_cx; pShot->y = eyeR_cy;
        pShot->muki = atan2(player.y - eyeR_cy, player.x - eyeR_cx);
        pShot->speed = 0.3;
        pShot->param_i[0] = 1;
        pShot->kind = img_enemyShotLargeBall[6]; // 白大玉
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;

        for (int i = 0; i < 8; i++) {
            pShot = new sEnemyShot;
            pShot->param_d[0] = cos(DX_PI * 2 / 8 * i) * 45.0;
            pShot->param_d[1] = sin(DX_PI * 2 / 8 * i) * 45.0;
            pShot->param_d[2] = eyeR_cx; pShot->param_d[3] = eyeR_cy;
            pShot->x = eyeR_cx + pShot->param_d[0];
            pShot->y = eyeR_cy + pShot->param_d[1];
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 0.3;
            pShot->param_i[0] = 1;
            pShot->kind = img_enemyShotLargeBall[0]; // 赤大玉
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }


        // ==========================================
        // 口の生成 (赤い中玉・大玉で喰らいつくような扇状弾幕)
        // ==========================================
        for (int i = 0; i < 40; i++) {
            pShot = new sEnemyShot;
            pShot->x = 240.0 + GetRand(40) - 20;
            pShot->y = 60.0 + GetRand(20);
            // 真下を中心に ±40度のランダムな角度
            pShot->muki = DX_PI / 2 + (GetRand(80) - 40) / 180.0 * DX_PI;
            pShot->speed = 1.5 + GetRand(20) / 10.0;
            // 中玉と大玉をランダムに混ぜて「牙」や「肉」の不規則さを出す
            pShot->kind = (GetRand(1) == 0) ? img_enemyShotMediumBall[0] : img_enemyShotLargeBall[0];
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }


        // ==========================================
        // 視界を奪う壁 (左右から迫る青い銃弾)
        // ==========================================
        for (int i = 0; i < 60; i += 2) {
            // 左から右へ
            pShot = new sEnemyShot;
            pShot->x = -30.0;
            pShot->y = (double)(i * 8);
            pShot->muki = 0.0;
            pShot->speed = 4.0 + GetRand(20) / 10.0;
            pShot->kind = img_enemyShotBullet[4]; // 青銃弾
            pShot->margin = 40;
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;

            // 右から左へ
            pShot = new sEnemyShot;
            pShot->x = 510.0;
            pShot->y = (double)(i * 8) + 4.0;
            pShot->muki = DX_PI;
            pShot->speed = 4.0 + GetRand(20) / 10.0;
            pShot->kind = img_enemyShotBullet[4]; // 青銃弾
            pShot->margin = 40;
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // ==========================================
    // 移動処理
    // ==========================================
    sEnemyShot* pCur = pEnemyShotSet->pEnemyShotHead->next;
    while (pCur != pEnemyShotSet->pEnemyShotHead) {
        if (pCur->param_i[0] == 1) {
            // 目玉の追尾処理（ギョロリと動く演出）
            double targetAngle = atan2(player.y - pCur->param_d[3], player.x - pCur->param_d[2]);
            double diff = targetAngle - pCur->muki;
            // 角度を -PI ~ PI に正規化
            while (diff > DX_PI) diff -= DX_PI * 2;
            while (diff < -DX_PI) diff += DX_PI * 2;
            pCur->muki += diff * 0.03; // ゆっくりとプレイヤーの方を向く

            // 目の中心座標を更新
            pCur->param_d[2] += pCur->speed * cos(pCur->muki);
            pCur->param_d[3] += pCur->speed * sin(pCur->muki);

            // 実際の描画座標にオフセットを適用（形を保ったまま追尾する）
            pCur->x = pCur->param_d[2] + pCur->param_d[0];
            pCur->y = pCur->param_d[3] + pCur->param_d[1];
        }
        else {
            // 通常の弾の移動
            pCur->x += pCur->speed * cos(pCur->muki);
            pCur->y += pCur->speed * sin(pCur->muki);
        }
        pCur = pCur->next;
    }
}


// 敵本体のパターン
void EnemyPat_JumpScare_Zai()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }

    const int T = 400;
    int countT = count % T;

    // --- 導入：催眠螺旋 (約3秒間) ---
    if (countT >= 60 && countT <= 239) {
        if (countT % 3 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotHypnoticSpiral;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;
            pEnemyShotSet->param_i[0] = countT; // 螺旋の角度計算用にフレーム数を保存

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }

    // --- 静寂：不気味な停止 (約0.3秒間) ---
    // ここでは何もしません。プレイヤーに「止まった」と認識させます。

    // --- 予告音 ---
    if (countT == 250) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --- ジャンプスケア発動 ---
    if (countT == 300) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotJumpScare;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}