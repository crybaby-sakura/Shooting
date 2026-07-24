// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：欺瞞の円環（エビングハウス錯視モチーフ）
static void ShotEbbinghaus(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // Aパターン：巨大弾の輪（錯視を誘発する威圧的な壁）
        // 大玉(20.0x20.0)を24個配置（15度間隔）
        // 黒(7)で描画し、背景に溶け込ませることで「不気味な影」として隙間を極端に狭く見せます
        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 基準の向きを中心に、-172.5度〜+172.5度の範囲で等間隔に放射
            pEnemyShot->muki = pEnemyShotSet->muki + (i * 15.0 - 172.5) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.8; // ゆっくりとした不気味な進行
            pEnemyShot->kind = img_enemyShotLargeBall[7]; // 黒の大玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // Bパターン：極小弾の輪（見た目の隙間を埋め尽くす光の雨）
        // 中玉(7.0x7.0)を使用。大玉の「隙間」にだけ、びっしりと弾を詰めます。
        // 白(6)で描画し、過剰に光っているように見せることで隙間を広く（安全に）錯覚させます。
        for (int i = 0; i < 24; i++) {
            // 大玉と大玉の間の角度（7.5度ずらした位置）を中心とする
            double centerAngle = pEnemyShotSet->muki + (i * 15.0 + 7.5 - 172.5) / 180.0 * DX_PI;

            // 中心角度から ±3度の範囲を、0.5度間隔で計13発ずつ発射
            // これにより、プレイヤー目の前では中玉同士が完全に重なり合い「物理的に絶対に通れない白い壁」になります
            for (int j = -9; j <= 9; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = centerAngle + j * 0.5 / 180.0 * DX_PI;
                pEnemyShot->speed = 1.95; // 大玉より少し速く、先に迫ってくる恐怖を演出
                pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白の中玉

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        // 合計 24 + (24 * 13) = 336発
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Ebbinghaus_Zai()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 弾幕発射の30フレーム前に予告音を鳴らす
    //if (count % 90 == 60) {
    //    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
    //    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    //}

    // 90フレームごとに弾幕を生成
    if (count % 60 == 1) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotEbbinghaus;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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