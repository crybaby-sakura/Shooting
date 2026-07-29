// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：神輿乱舞・三三七拍子
// ============================================================
static void ShotMikoSanSanNana(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // フェーズに応じた効果音の再生
        if (pEnemyShotSet->kind == 0) {
            // 前半：太鼓のような重低音
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        else if (pEnemyShotSet->kind == 1) {
            // 中盤：鉦（かね）のような高く鋭い金属音
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        else if (pEnemyShotSet->kind == 2) {
            // 終盤：手拍子のような鋭く乾いた連続音
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // フェーズに応じた弾の生成
        if (pEnemyShotSet->kind == 0) {
            // 【前半：三拍子】赤色の「大玉」、ゆっくりした狙い撃ち
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = pEnemyShotSet->muki;
            pShot->speed = 3.0; // ゆったり
            // 弾の色: 0=赤, 種類: 大玉
            pShot->kind = img_enemyShotLargeBall[0];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        else if (pEnemyShotSet->kind == 1) {
            // 【中盤：三拍子】青色の「中玉」、3way拡散
            for (int i = 0; i < 3; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                // 中心から左右に0.3ラジアンずつずらす
                pShot->muki = pEnemyShotSet->muki + (i - 1) * 0.3;
                pShot->speed = 4.5;
                // 弾の色: 4=青, 種類: 中玉
                pShot->kind = img_enemyShotMediumBall[4];

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
        else if (pEnemyShotSet->kind == 2) {
            // 【終盤：七拍子】白色の「短レーザー」、高速かつ少しばらついた狙い撃ち
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            // GetRand(20) は 0〜20 を返すので、-10〜+10 の範囲になる。
            // 100.0で割って -0.1〜+0.1 ラジアンのばらつきを与え、光の檻を表現
            pShot->muki = pEnemyShotSet->muki + (GetRand(20) - 10) / 100.0;
            pShot->speed = 8.0; // 高速
            // 弾の色: 6=白, 種類: 短レーザー
            pShot->kind = img_enemyShotLaser[6];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン：三三七拍子（新規作成）
// ============================================================
void EnemyPat_337Beat_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // リズムパターンを数回繰り返せるHP
    }
    else {
        // 神輿を担ぐような、リズミカルな揺れ動き
        enemy.x = 240.0 + sin(count * 0.05) * 120.0;
        enemy.y = 80.0 + sin(count * 0.10) * 20.0;
    }

    // 1サイクル = 240フレーム (約4秒 @60FPS)
    int cycle = count % 240;

    // サイクル開始時に予告音で「間」を作る
    if (cycle == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    bool shouldShoot = false;
    int shotKind = -1;

    // 【前半：三拍子】ゆったりとした間隔 (30, 45, 60フレーム)
    if (cycle == 30 || cycle == 45 || cycle == 60) {
        shouldShoot = true;
        shotKind = 0;
    }
    // 【中盤：三拍子】少し速い間隔で緊張感を上昇 (90, 105, 120フレーム)
    else if (cycle == 90 || cycle == 105 || cycle == 120) {
        shouldShoot = true;
        shotKind = 1;
    }
    // 【終盤：七拍子】最高速の連射で包囲 (150, 160, 170, 180, 190, 200, 210フレーム)
    else if (cycle >= 150 && cycle <= 210 && (cycle - 150) % 10 == 0) {
        shouldShoot = true;
        shotKind = 2;
    }

    if (shouldShoot) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMikoSanSanNana;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shotKind;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
