// enemyPat_SanSanNana.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 1回目の「3」：自機狙い大弾3連
static void Shot_SanSanNana_Part1(sEnemyShotSet* pSet) {
    if (pSet->count == 0 || pSet->count == 24 || pSet->count == 48) {
        // 重厚な発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pShot->speed = 5.0;
        pShot->kind = img_enemyShotLargeBall[0]; // 赤い大玉

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    // 弾の移動処理
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 2回目の「3」：自機狙い5Way弾3連
static void Shot_SanSanNana_Part2(sEnemyShotSet* pSet) {
    if (pSet->count == 0 || pSet->count == 24 || pSet->count == 48) {
        // 重厚な発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double base_muki = atan2(player.y - pSet->y, player.x - pSet->x);
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = base_muki + (i - 2) * (15.0 * DX_PI / 180.0);
            pShot->speed = 4.0;
            pShot->kind = img_enemyShotMediumOval[4]; // 青い中楕円弾

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 最後の「7」：全方位弾7連
static void Shot_SanSanNana_Part3(sEnemyShotSet* pSet) {
    // 24フレームごとに7回発射（0, 24, 48, 72, 96, 120, 144）
    if (pSet->count % 24 == 0 && pSet->count <= 24 * 6) {
        int step = pSet->count / 24;

        if (step < 6) {
            // 1〜6発目は中音
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            // 奇数・偶数回で回転方向を逆にする
            double base_angle = (step % 2 == 0) ? (pSet->count * 0.015) : (-pSet->count * 0.015);
            int ways = 36;
            for (int i = 0; i < ways; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = base_angle + i * (DX_PI * 2.0 / ways);
                pShot->speed = 2.5;
                // 色を交互に変更（黄と緑）
                pShot->kind = img_enemyShotMediumBall[(step % 2 == 0) ? 1 : 2];

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
        else {
            // 最後の1発（7発目）は派手な極大音と大玉
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            int ways = 48;
            for (int i = 0; i < ways; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = i * (DX_PI * 2.0 / ways);
                pShot->speed = 4.5;
                pShot->kind = img_enemyShotLargeBall[6]; // 白い大玉

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン（三三七拍子）
void EnemyPat_337Beat_Gemini()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 500フレーム周期でループ
    int t = count % 500;

    // --- 敵の移動処理（「ウン」の溜めでの移動） ---
    if (t >= 80 && t < 110) {
        enemy.x -= 4.0; // 最初の3の後に左へサッと移動
    }
    else if (t >= 180 && t < 210) {
        enemy.x += 4.0; // 次の3の後に中央へ戻る
    }

    // --- 予告音（最後の7連への溜め） ---
    if (t == 190) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --- 弾幕セットの登録 ---
    // 1回目の「3」
    if (t == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Shot_SanSanNana_Part1;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 2回目の「3」
    if (t == 126) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Shot_SanSanNana_Part2;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 最後の「7」
    if (t == 222) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Shot_SanSanNana_Part3;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}