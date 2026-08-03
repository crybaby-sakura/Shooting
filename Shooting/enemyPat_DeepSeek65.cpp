// enemyPat_Tmp_hydra.cpp
// 不死結界「生首の庭」ヒュドラ弾幕パターン実装（修正版）
// 敵本体関数名: void EnemyPat_Hydra_DeepSeek()

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//-----------------------------------------------------------
// 前方宣言
//-----------------------------------------------------------
static void BigHeadPattern(sEnemyShotSet* pSet);
static void SmallHeadPattern(sEnemyShotSet* pSet);

//-----------------------------------------------------------
// 大首（破壊対象）のパターン
//-----------------------------------------------------------
static void BigHeadPattern(sEnemyShotSet* pSet)
{
    // 初回フレーム：頭部を生成
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShot* head = new sEnemyShot;
        head->x = pSet->x;
        head->y = pSet->y;
        head->kind = img_enemyShotLargeBall[2]; // 緑大玉（頭部）
        head->muki = 0.0;
        head->speed = 0.0;
        head->margin = 100.0;
        head->param_i[0] = 1; // 1:アクティブ, 0:破壊済み

        head->prev = pSet->pEnemyShotHead->prev;
        head->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = head;
        pSet->pEnemyShotHead->prev = head;

        // うねり基準座標を保存
        pSet->param_d[0] = pSet->x;
        pSet->param_d[1] = pSet->y;
    }

    // 毎フレーム：頭部の動き・弾発射・全弾の移動
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 頭部（大玉）の場合：蛇行移動
        if (pShot->kind == img_enemyShotLargeBall[2]) {
            double baseX = pSet->param_d[0];
            double baseY = pSet->param_d[1];

            pShot->x = baseX + 50.0 * sin(pSet->count * 0.04 + baseX * 0.01);
            pShot->y = baseY + 20.0 * sin(pSet->count * 0.03);

            // 画面外にはみ出さないよう軽くクランプ
            if (pShot->x < 10.0)  pShot->x = 10.0;
            if (pShot->x > 470.0) pShot->x = 470.0;
            if (pShot->y < 10.0)  pShot->y = 10.0;
            if (pShot->y > 470.0) pShot->y = 470.0;

            // アクティブなら弾を発射
            if (pShot->param_i[0] == 1 && pSet->count % 30 == 0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                sEnemyShot* bullet = new sEnemyShot;
                bullet->x = pShot->x;
                bullet->y = pShot->y;
                bullet->muki = atan2(player.y - bullet->y, player.x - bullet->x);
                bullet->speed = 1.8;               // 低速
                bullet->kind = img_enemyShotSmallBall[4]; // 青小玉
                bullet->margin = 20.0;

                bullet->prev = pSet->pEnemyShotHead->prev;
                bullet->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = bullet;
                pSet->pEnemyShotHead->prev = bullet;
            }
        }
        // それ以外の弾（青小玉）は等速直線移動
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

//-----------------------------------------------------------
// 小首（増殖した首）のパターン
//-----------------------------------------------------------
static void SmallHeadPattern(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShot* head = new sEnemyShot;
        head->x = pSet->x;
        head->y = pSet->y;
        head->kind = img_enemyShotMediumBall[0]; // 赤中玉（頭部）
        head->muki = 0.0;
        head->speed = 0.0;
        head->margin = 100.0;
        head->param_i[0] = 1; // 常時アクティブ（破壊不可）

        head->prev = pSet->pEnemyShotHead->prev;
        head->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = head;
        pSet->pEnemyShotHead->prev = head;

        pSet->param_d[0] = pSet->x;
        pSet->param_d[1] = pSet->y;
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 頭部（赤中玉）の動き
        if (pShot->kind == img_enemyShotMediumBall[0]) {
            double baseX = pSet->param_d[0];
            double baseY = pSet->param_d[1];

            pShot->x = baseX + 40.0 * sin(pSet->count * 0.12 + baseY * 0.02);
            pShot->y = baseY + 30.0 * cos(pSet->count * 0.15 + baseX * 0.02);

            if (pShot->x < 15.0)  pShot->x = 15.0;
            if (pShot->x > 465.0) pShot->x = 465.0;
            if (pShot->y < 15.0)  pShot->y = 15.0;
            if (pShot->y > 465.0) pShot->y = 465.0;

            // 約0.25秒ごとに全方位赤針弾
            if (pSet->count % 15 == 0) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                int way = 5 + (pSet->count / 20) % 3; // 5～7way
                double base_angle = GetRand(359) / 360.0 * DX_PI * 2.0;

                for (int i = 0; i < way; ++i) {
                    double angle = base_angle + DX_PI * 2.0 * i / way;
                    sEnemyShot* bullet = new sEnemyShot;
                    bullet->x = pShot->x;
                    bullet->y = pShot->y;
                    bullet->muki = angle;
                    bullet->speed = 5.5;
                    bullet->kind = img_enemyShotBullet[0]; // 赤銃弾
                    bullet->margin = 20.0;

                    bullet->prev = pSet->pEnemyShotHead->prev;
                    bullet->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = bullet;
                    pSet->pEnemyShotHead->prev = bullet;
                }
            }
        }
        // 弾の移動
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

//-----------------------------------------------------------
// 敵本体：ヒュドラ
//-----------------------------------------------------------
void EnemyPat_Hydra_DeepSeek()
{
    static int  moveDir;
    static bool thresholds[3];
    static int  regenTimer;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = 200;      // HPを200に固定
        enemy.hp = 200;
        moveDir = 1;

        for (int i = 0; i < 3; ++i) thresholds[i] = false;
        regenTimer = 0;

        // 最初の大首を3体生成
        const double offsetX[3] = { -80.0, 0.0, 80.0 };
        const double offsetY = -20.0;

        for (int i = 0; i < 3; ++i) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = BigHeadPattern;
            pSet->x = enemy.x + offsetX[i];
            pSet->y = enemy.y + offsetY;
            pSet->muki = 0.0;
            pSet->kind = 0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 本体の横移動
    enemy.x += 0.4 * moveDir;
    if (count % 180 == 0) moveDir *= -1;
    if (enemy.x < 50.0) { enemy.x = 50.0;  moveDir = 1; }
    if (enemy.x > 430.0) { enemy.x = 430.0; moveDir = -1; }

    // HPしきい値：150, 100, 50 で小首を増殖
    const int hpThresholds[3] = { 150, 100, 50 };
    for (int i = 0; i < 3; ++i) {
        if (!thresholds[i] && enemy.hp <= hpThresholds[i]) {
            thresholds[i] = true;

            // アクティブな大首を一つ無効化
            {
                sEnemyShotSet* pSet = enemyShotSetHead.next;
                while (pSet != &enemyShotSetHead) {
                    if (pSet->patternFunc == BigHeadPattern) {
                        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
                        while (pShot != pSet->pEnemyShotHead) {
                            if (pShot->kind == img_enemyShotLargeBall[2] && pShot->param_i[0] == 1) {
                                pShot->param_i[0] = 0;
                                pSet = &enemyShotSetHead; // 二重ループ脱出の目印
                                break;
                            }
                            pShot = pShot->next;
                        }
                    }
                    if (pSet != &enemyShotSetHead) pSet = pSet->next;
                }
            }

            // 小首を2体追加
            for (int j = 0; j < 2; ++j) {
                double spawnX = enemy.x + (GetRand(120) - 60);
                double spawnY = enemy.y - 30.0 + (GetRand(20) - 10);

                sEnemyShotSet* pSmall = new sEnemyShotSet;
                pSmall->count = 0;
                pSmall->patternFunc = SmallHeadPattern;
                pSmall->x = spawnX;
                pSmall->y = spawnY;
                pSmall->muki = 0.0;
                pSmall->kind = 0;

                pSmall->pEnemyShotHead = new sEnemyShot;
                pSmall->pEnemyShotHead->prev = pSmall->pEnemyShotHead;
                pSmall->pEnemyShotHead->next = pSmall->pEnemyShotHead;

                pSmall->prev = enemyShotSetHead.prev;
                pSmall->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSmall;
                enemyShotSetHead.prev = pSmall;
            }
        }
    }

    // 一定時間ごとに大首が再生（増える）
    ++regenTimer;
    if (regenTimer >= 480) {
        regenTimer = 0;

        double newX = enemy.x + (GetRand(100) - 50);
        double newY = enemy.y - 20.0;

        sEnemyShotSet* pNewBig = new sEnemyShotSet;
        pNewBig->count = 0;
        pNewBig->patternFunc = BigHeadPattern;
        pNewBig->x = newX;
        pNewBig->y = newY;
        pNewBig->muki = 0.0;
        pNewBig->kind = 0;

        pNewBig->pEnemyShotHead = new sEnemyShot;
        pNewBig->pEnemyShotHead->prev = pNewBig->pEnemyShotHead;
        pNewBig->pEnemyShotHead->next = pNewBig->pEnemyShotHead;

        pNewBig->prev = enemyShotSetHead.prev;
        pNewBig->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pNewBig;
        enemyShotSetHead.prev = pNewBig;

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
}