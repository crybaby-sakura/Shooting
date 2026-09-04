// enemyPat_tmp.cpp
// 弾幕：覗き込む者（The Watcher）- ジャンプスケアをモチーフにした4フェーズ弾幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// --------------------------------------------------
// 第1フェーズ：気配（予兆）
// 画面端に黒い小弾が12個出現し、ゆらゆらと蠢く
// --------------------------------------------------
static void ShotWatcher_Phase1(sEnemyShotSet* pSet)
{
    sEnemyShot* p;
    if (pSet->count == 0) {
        // 画面四隅と上下左右の端に、不気味な小弾（黒）を生成
        for (int i = 0; i < 12; i++) {
            p = new sEnemyShot;
            if (i < 3) {
                p->x = GetRand(60);
                p->y = GetRand(480);
            }
            else if (i < 6) {
                p->x = 420 + GetRand(60);
                p->y = GetRand(480);
            }
            else if (i < 9) {
                p->x = GetRand(480);
                p->y = GetRand(60);
            }
            else {
                p->x = GetRand(480);
                p->y = 420 + GetRand(60);
            }
            p->muki = GetRand(360) / 180.0 * DX_PI;
            p->speed = 0.3 + GetRand(50) / 100.0;
            p->kind = img_enemyShotSmallBall[7]; // 黒
            p->param_i[0] = 0; // 気配弾
            p->margin = 240;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // 更新：ゆらゆらと蠢く（正弦波＋わずかな向き変化）
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot->muki += 0.03;
        pShot = pShot->next;
    }

    // 90フレームでフェーズ終了、弾を画面外へ追い出して消去
    if (pSet->count == 90) {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x = -1000.0;
            pShot->speed = 0.0;
            pShot = pShot->next;
        }
    }
}

// --------------------------------------------------
// 第2フェーズ：凝視
// 中央に白い大玉（目玉）が出現し、赤い小弾（血管）が周回
// --------------------------------------------------
static void ShotWatcher_Phase2(sEnemyShotSet* pSet)
{
    sEnemyShot* p;
    if (pSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 目玉（大玉・白）を画面中央に
        p = new sEnemyShot;
        p->x = 240.0;
        p->y = 240.0;
        p->muki = 0.0;
        p->speed = 0.0;
        p->kind = img_enemyShotLargeBall[6]; // 白
        p->param_i[0] = 1; // 目玉
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;

        // 血管（小弾・赤）を目玉周囲に8個
        for (int i = 0; i < 8; i++) {
            p = new sEnemyShot;
            p->x = 240.0;
            p->y = 240.0;
            p->muki = 0.0;
            p->speed = 0.0;
            p->kind = img_enemyShotSmallBall[0]; // 赤
            p->param_i[0] = 2; // 血管
            p->param_d[0] = i * DX_PI / 4.0; // 初期角度
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // 更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 目玉：脈動（微動）とプレイヤー方向への向き
            pShot->x = 240.0 + sin(pSet->count * 0.1) * 3.0;
            pShot->y = 240.0 + cos(pSet->count * 0.1) * 3.0;
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
        }
        else if (pShot->param_i[0] == 2) {
            // 血管：目玉周囲を周回
            double angle = pShot->param_d[0] + pSet->count * 0.04;
            pShot->x = 240.0 + cos(angle) * 38.0;
            pShot->y = 240.0 + sin(angle) * 38.0;
        }
        pShot = pShot->next;
    }

    // 120フレームで瞬き（全弾を画面外へ）
    if (pSet->count == 120) {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x = -1000.0;
            pShot->speed = 0.0;
            pShot = pShot->next;
        }
    }
}

// --------------------------------------------------
// 第3フェーズ：ジャンプスケア
// 目玉がプレイヤー位置に飛び出し、針と影の手が襲う
// --------------------------------------------------
static void ShotWatcher_Phase3(sEnemyShotSet* pSet)
{
    sEnemyShot* p;
    if (pSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        pSet->param_d[0] = player.x;
        pSet->param_d[1] = player.y;

        // 針状弾（銃弾・赤）：全方位24発
        for (int i = 0; i < 24; i++) {
            p = new sEnemyShot;
            p->muki = i * DX_PI / 12.0;
            p->x = player.x - cos(p->muki) * 150.0;
            p->y = player.y - sin(p->muki) * 150.0;
            p->speed = 5.0;
            p->kind = img_enemyShotBullet[0]; // 赤
            p->param_i[0] = 11; // 針
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }

        // 影の手（短レーザー・黒）：上下左右から中央へ
        for (int i = 0; i < 4; i++) {
            p = new sEnemyShot;
            if (i == 0) {       // 上から下へ
                p->x = 240.0; p->y = 0.0; p->muki = DX_PI / 2.0;
            }
            else if (i == 1) { // 下から上へ
                p->x = 240.0; p->y = 480.0; p->muki = -DX_PI / 2.0;
            }
            else if (i == 2) { // 左から右へ
                p->x = 0.0; p->y = 240.0; p->muki = 0.0;
            }
            else {             // 右から左へ
                p->x = 480.0; p->y = 240.0; p->muki = DX_PI;
            }
            p->speed = 10.0;
            p->kind = img_enemyShotLaser[7]; // 黒
            p->param_i[0] = 12; // 影の手
            p->margin = 120;
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    if (pSet->count == 10) {
        // 目玉：プレイヤー位置に巨大化（大玉・白）で出現
        p = new sEnemyShot;
        p->x = pSet->param_d[0];
        p->y = pSet->param_d[1];
        p->muki = 0.0;
        p->speed = 0.0;
        p->kind = img_enemyShotLargeBall[6]; // 白
        p->param_i[0] = 10; // ジャンプ目玉
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }

    // 更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 10) {
            // 目玉：プレイヤーを超高速で追尾（出現位置がプレイヤーなので、動けば避けられる）
            double dx = player.x - pShot->x;
            double dy = player.y - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > 0.1) {
                pShot->x += (dx / dist) * 10.0 / 3;
                pShot->y += (dy / dist) * 10.0 / 3;
            }
        }
        else if (pShot->param_i[0] == 11) {
            // 針：直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[0] == 12) {
            // 影の手：直進（中央を貫通して画面外へ）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// --------------------------------------------------
// 敵本体のパターン
// --------------------------------------------------
void EnemyPat_JumpScare_Kimi()
{
    static int shot_count = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;
    }

    // 第1フェーズ：気配（予兆）- count 1
    if (count % 400 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotWatcher_Phase1;
        pSet->x = 240.0;
        pSet->y = 240.0;
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

    // 第2フェーズ：凝視 - count 90（気配が終了した直後）
    if (count % 400 == 90) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotWatcher_Phase2;
        pSet->x = 240.0;
        pSet->y = 240.0;
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

    // 第3フェーズ：ジャンプスケア - count 215（凝視終了後、15フレームの暗転間を置いて）
    if (count % 400 == 215) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotWatcher_Phase3;
        pSet->x = player.x;
        pSet->y = player.y;
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