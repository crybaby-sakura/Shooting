// enemyPat_teleport.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// -----------------------------------------------------------------------------
// 共通の弾更新処理（生成後の移動のみを行うダミー関数）
// -----------------------------------------------------------------------------
static void ShotUpdateOnly(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// -----------------------------------------------------------------------------
// 残像の弾幕（時間差で全方位弾を発射）
// -----------------------------------------------------------------------------
static void ShotAfterimage(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotLargeBall[4];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 約0.6秒後(36フレーム後)に発射
    if (pEnemyShotSet->count == 36) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShotSet->pEnemyShotHead->next->margin = -9999;

        for (int i = 0; i < 12 * 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * (2.0 * DX_PI / 12.0 / 3);
            pEnemyShot->speed = 2.5;
            // 青色中玉 (残像らしい冷色系を選択)
            pEnemyShot->kind = img_enemyShotMediumBall[4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        // 以降は更新のみ行うように関数ポインタを書き換え
        pEnemyShotSet->patternFunc = ShotUpdateOnly;
    }
}

// -----------------------------------------------------------------------------
// ボス本体の弾幕（自機狙い扇状弾）
// -----------------------------------------------------------------------------
static void ShotBossFan(sEnemyShotSet* pEnemyShotSet)
{
    // ワープ直後(10フレーム後)に発射
    if (pEnemyShotSet->count == 10) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        int way = 5;
        double spread = 40.0 * DX_PI / 180.0;
        for (int i = 0; i < way; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle - spread / 2.0 + (spread * i / (way - 1));
            pEnemyShot->speed = 3.5;
            // 赤色大玉 (ボスのメイン攻撃として威圧感を出す)
            pEnemyShot->kind = img_enemyShotLargeBall[0];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        pEnemyShotSet->patternFunc = ShotUpdateOnly;
    }
}

// -----------------------------------------------------------------------------
// 通常攻撃（ワープ合間の自機狙いばら撒き）
// -----------------------------------------------------------------------------
static void ShotNormal(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int i = 0; i < 3 * 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.0 + GetRand(10) / 10.0;
            // 黄色銃弾 (スピード感のある雑魚弾)
            pEnemyShot->kind = img_enemyShotBullet[1];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        pEnemyShotSet->patternFunc = ShotUpdateOnly;
    }
}

// -----------------------------------------------------------------------------
// 敵本体のパターン
// -----------------------------------------------------------------------------
void EnemyPat_Warp_Qwen()
{
    static int pidx;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        pidx = -1;
    }

    // ワープの予告音（ワープの1秒前）
    if (count % 180 == 120) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 180フレームごとに瞬間移動
    if (count % 180 == 0) {
        double prevX = enemy.x;
        double prevY = enemy.y;

        // ワープ元の座標に残像用 ShotSet を生成
        sEnemyShotSet* pAfterimageSet = new sEnemyShotSet;
        pAfterimageSet->count = 0;
        pAfterimageSet->patternFunc = ShotAfterimage;
        pAfterimageSet->x = prevX;
        pAfterimageSet->y = prevY;
        pAfterimageSet->muki = 0.0;
        pAfterimageSet->kind = 0;

        pAfterimageSet->pEnemyShotHead = new sEnemyShot;
        pAfterimageSet->pEnemyShotHead->prev = pAfterimageSet->pEnemyShotHead;
        pAfterimageSet->pEnemyShotHead->next = pAfterimageSet->pEnemyShotHead;

        pAfterimageSet->prev = enemyShotSetHead.prev;
        pAfterimageSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pAfterimageSet;
        enemyShotSetHead.prev = pAfterimageSet;

        // ワープ先候補
        double candidates[][2] = {
            {80.0, 100.0}, {400.0, 100.0}, {240.0, 150.0}, {120.0, 250.0}, {360.0, 250.0}
        };
		int numCandidates = sizeof(candidates) / sizeof(candidates[0]);

		int idx = pidx;
		while (idx == pidx) idx = GetRand(numCandidates - 1);
		pidx = idx;
		enemy.x = candidates[idx][0];
		enemy.y = candidates[idx][1];

        // ワープ先に本体用 ShotSet を生成
        sEnemyShotSet* pBossSet = new sEnemyShotSet;
        pBossSet->count = 0;
        pBossSet->patternFunc = ShotBossFan;
        pBossSet->x = enemy.x;
        pBossSet->y = enemy.y;
        pBossSet->muki = 0.0;
        pBossSet->kind = 0;

        pBossSet->pEnemyShotHead = new sEnemyShot;
        pBossSet->pEnemyShotHead->prev = pBossSet->pEnemyShotHead;
        pBossSet->pEnemyShotHead->next = pBossSet->pEnemyShotHead;

        pBossSet->prev = enemyShotSetHead.prev;
        pBossSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBossSet;
        enemyShotSetHead.prev = pBossSet;
    }

    // ワープの合間の通常攻撃
    if (count % 45 == 0 && count % 180 != 0) {
        sEnemyShotSet* pNormalSet = new sEnemyShotSet;
        pNormalSet->count = 0;
        pNormalSet->patternFunc = ShotNormal;
        pNormalSet->x = enemy.x;
        pNormalSet->y = enemy.y;
        pNormalSet->muki = 0.0;
        pNormalSet->kind = 0;

        pNormalSet->pEnemyShotHead = new sEnemyShot;
        pNormalSet->pEnemyShotHead->prev = pNormalSet->pEnemyShotHead;
        pNormalSet->pEnemyShotHead->next = pNormalSet->pEnemyShotHead;

        pNormalSet->prev = enemyShotSetHead.prev;
        pNormalSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pNormalSet;
        enemyShotSetHead.prev = pNormalSet;
    }

    // ボスの微小なホバリング
    enemy.y += sin(count * 0.05) * 0.3;
}