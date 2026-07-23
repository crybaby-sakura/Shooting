// enemyPat_beer.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：黄金の雫（重力あり・落下後に跳ねる飛沫へ分裂）
// ============================================================
static void ShotBeerDrop(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 5列の黄金の雫を生成
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (i - 2) * 40.0;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI / 2;

            // param_d[0] = vy (Y方向速度), param_d[1] = gravity (重力), param_d[2] = vx (X方向速度)
            pEnemyShot->param_d[0] = 1.5;
            pEnemyShot->param_d[1] = 0.08;
            pEnemyShot->param_d[2] = 0.0;

            pEnemyShot->kind = img_enemyShotMediumOval[8]; // 橙の中楕円弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;

        // 重力挙動の適用
        pShot->param_d[0] += pShot->param_d[1]; // vy に重力を加算
        pShot->y += pShot->param_d[0];
        pShot->x += pShot->param_d[2];

        // 画面下部(y > 380)に達したら「ビールかけ！」として跳ねる飛沫弾に分裂
        if (pShot->y > 380.0 && pShot->kind == img_enemyShotMediumOval[8]) {
            // 飛沫弾を放射状かつ跳ねる軌道で生成
            for (int i = 0; i < 6; i++) {
                sEnemyShot* pSplash = new sEnemyShot;
                pSplash->x = pShot->x;
                pSplash->y = pShot->y;

                // X方向の速度：左右に広がる (-3.0 〜 3.0)
                pSplash->param_d[2] = (GetRand(60) - 30) / 10.0;
                // Y方向の初速：上向きに跳ねる (-2.0 〜 -5.0)
                pSplash->param_d[0] = -(2.0 + GetRand(30) / 10.0);
                // 重力：飛沫なのでやや強め
                pSplash->param_d[1] = 0.15;

                pSplash->kind = img_enemyShotSmallBall[6]; // 白の小玉

                pSplash->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSplash->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSplash;
                pEnemyShotSet->pEnemyShotHead->prev = pSplash;
            }

            // 元の雫弾は役目を終えたのでリストから外して削除
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }

        pShot = nextShot;
    }
}

// ============================================================
// 弾幕パターン：もこもこの泡（速度がバラバラで上昇して視界を遮る）
// ============================================================
static void ShotBeerFoam(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 画面下部に泡を敷き詰める
        for (int x = 20; x < 480; x += 40) {
            for (int y = 400; y < 480; y += 40) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = (double)x + GetRand(20) - 10; // 位置のゆらぎ
                pEnemyShot->y = (double)y + GetRand(20) - 10;

                // スピードをランダムにバラバラにする (0.3 〜 0.8)
                pEnemyShot->speed = 0.3 + GetRand(50) / 100.0;
                pEnemyShot->kind = img_enemyShotLargeBall[6]; // 白の大玉

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 各自のスピードでゆっくり上昇
        pShot->y -= pShot->speed;
        // 泡らしいゆらぎ運動
        pShot->x += sin(pShot->count * 0.1 + pShot->x) * 0.3;

        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕パターン：オーバーフロー・シャワー（泡と液体が溢れ落ちて降り注ぐ）
// ============================================================
static void ShotBeerOverflow(sEnemyShotSet* pEnemyShotSet)
{
    // 継続的に新しい弾を生成し続ける「シャワー」型パターン
    // count が特定の範囲内のときのみ生成する

    // 例: count 0〜120 の間、2フレームごとに生成
    if (pEnemyShotSet->count < 120 && pEnemyShotSet->count % 2 == 0) {
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        // 1回に複数発生成して「ドバッ」と感を出す
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            // 画面上部から、プレイヤーのX座標付近を中心にランダムに降り注ぐ
            pEnemyShot->x = player.x + (GetRand(200) - 100);
            // 画面外の上からスタート
            pEnemyShot->y = -20.0 - GetRand(40);

            // 下向きの高速移動
            pEnemyShot->param_d[0] = 4.0 + GetRand(30) / 10.0; // vy (4.0 〜 7.0)
            pEnemyShot->param_d[1] = 0.0; // 重力はなし（勢いよく飛ぶので）
            pEnemyShot->param_d[2] = (GetRand(40) - 20) / 10.0; // vx (-2.0 〜 2.0) のわずかなブレ
            pEnemyShot->muki = atan2(pEnemyShot->param_d[0], pEnemyShot->param_d[2]);

            // 泡(白)と液体(橙)をランダムに混ぜる
            if (GetRand(1) == 0) {
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白の小玉（泡）
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumOval[8]; // 橙の中楕円弾（液体）
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;

        // 設定された速度で移動
        pShot->y += pShot->param_d[0];
        pShot->x += pShot->param_d[2];

        pShot = nextShot;
    }
}

// ============================================================
// 敵本体のパターン（反復型）
// ============================================================
void EnemyPat_BeerSpray_Qwen()
{
    // 600フレーム(約10秒)周期でパターンを反復させる
    int cycle = count % 600;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 反復パターンなのでHPは減らない設定（またはメイン側で管理）
    }

    if (cycle == 1) {
        // パターン開始の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 敵のちょっとした動き（左右に揺れる）
    enemy.x = 240.0 + sin(count * 0.02) * 80.0;

    // 【フェーズ1】豪快な注ぎ (cycle 60 から開始)
    if (cycle >= 30 && cycle <= 180 && cycle % 10 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBeerDrop;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 【フェーズ2】もこもこの泡 (cycle 240 から開始)
    if (cycle == 240) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBeerFoam;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 480.0;
        pEnemyShotSet->muki = -DX_PI / 2.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 【フェーズ3】オーバーフロー・シャワー (cycle 420 から開始)
    // 泡が画面を覆い尽くした頃合いに、ジョッキから溢れ落ちるシャワーでフィニッシュ
    if (cycle == 420) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBeerOverflow;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 0.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
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