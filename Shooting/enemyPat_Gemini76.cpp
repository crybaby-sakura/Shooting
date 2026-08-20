// enemyPat_BipolarResonance.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ボスB（青・コントローラー）：静止する全方位リング弾
// ============================================================
static void ShotController(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 0, 10, 20フレーム目に、速度の異なる全方位弾を展開して「厚みのある壁」を作る
    if (pEnemyShotSet->count <= 20 && pEnemyShotSet->count % 5 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int ways = 32;
        // 発射のたびに少しずつ角度をずらし、網目を複雑にする
        double baseMuki = (pEnemyShotSet->count / 5) * (DX_PI * 2.0 / ways / 3.0);
        // 発射タイミングが遅いものほど速くし、静止時に重なる層を作る
        double speed = 2.0 + (pEnemyShotSet->count / 5) * 1.5;

        for (int i = 0; i < ways; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseMuki + (DX_PI * 2.0 / ways) * i;
            pEnemyShot->speed = speed;
            pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青の中玉

            pEnemyShot->param_d[0] = speed; // 元の速度を記憶
            pEnemyShot->param_i[0] = 0;     // 状態フラグ (0:移動中, 1:停止中, 2:再移動中)

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の挙動更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 発射から60フレーム後（網目が広がった状態）で全弾を空中で静止させる
        if (pEnemyShotSet->count == 60 && pShot->param_i[0] == 0) {
            pShot->speed = 0.0;
            pShot->param_i[0] = 1;
        }
        // 発射から180フレーム後（赤ボスの攻撃終了後）に元の速度で再稼働し散っていく
        if (pEnemyShotSet->count == 180 && pShot->param_i[0] == 1) {
            pShot->speed = pShot->param_d[0];
            pShot->param_i[0] = 2;
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// ボスA（赤・アタッカー）：自機狙いの屈折レーザー
// ============================================================
static void ShotAttacker(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // セット起動時に予告音を鳴らし、自機の位置をロックオン
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
    }

    // count が 30〜42 の間、3フレームおきに5連射（長めのレーザーを表現）
    if (pEnemyShotSet->count >= 30 && pEnemyShotSet->count <= 52 && pEnemyShotSet->count % 3 == 0) {
        if (pEnemyShotSet->count == 30) {
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        int ways = 5;
        double targetMuki = pEnemyShotSet->muki;

        for (int i = 0; i < ways; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 扇状にわずかに角度を広げる
            pEnemyShot->muki = targetMuki + (i - ways / 2) * 0.15;
            pEnemyShot->speed = 6.0 * 2.5;
            pEnemyShot->kind = img_enemyShotLaser[0]; // 赤の短レーザー
            pEnemyShot->margin = 480;

            // 曲がる方向を決定（外側に向かって90度曲がるようにする）
            int turnDir = 1;
            if (i < ways / 2) turnDir = -1;
            else if (i > ways / 2) turnDir = 1;
            else turnDir = (GetRand(1) == 0) ? 1 : -1; // 中央の弾はランダム

            pEnemyShot->param_i[0] = 0; // 状態フラグ (0:屈折前, 1:1回屈折, 2:2回屈折)
            pEnemyShot->param_i[1] = turnDir;
            pEnemyShot->param_i[2] = 20; // 1回目の屈折までのフレーム数 (個別弾のcount基準)
            pEnemyShot->param_i[3] = 20; // 2回目の屈折までのフレーム数

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の挙動更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 弾の個別の count で屈折タイミングを測る
        if (pShot->param_i[0] == 0 && pShot->count == pShot->param_i[2]) {
            pShot->param_i[0] = 1;
            pShot->muki += pShot->param_i[1] * (DX_PI / 2.0); // 90度屈折
            pShot->speed *= 0.75;
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        else if (pShot->param_i[0] == 1 && pShot->count == pShot->param_i[2] + pShot->param_i[3]) {
            pShot->param_i[0] = 2;
            pShot->muki += pShot->param_i[1] * (DX_PI / 2.0); // さらに90度屈折
            pShot->speed *= 0.75;
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン管理
// ============================================================
void EnemyPat_TwoBoss_Gemini()
{
    if (count == 1) {
        // ボスA(赤)を左、ボスB(青)を右に配置
        enemy.x = 120.0;
        enemy.y = 180.0;
        enemy.x2 = 360.0;
        enemy.y2 = 180.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 2体のボスの動き（サイン波でゆるやかに8の字・円軌道を描く）
    enemy.x = 120.0 + 40.0 * sin(count * DX_PI / 180.0);
    enemy.y = 180.0 + 20.0 * cos(count * DX_PI / 180.0);

    enemy.x2 = 360.0 - 40.0 * sin(count * DX_PI / 180.0);
    enemy.y2 = 180.0 + 20.0 * cos(count * DX_PI / 180.0);

    // 周期240フレームで連携攻撃を実行
    int cycle = count % 240;

    // ボスB(青)：安地となる格子網を展開
    if (cycle == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotController;
        pSet->x = enemy.x2;
        pSet->y = enemy.y2;
        pSet->muki = 0.0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ボスA(赤)：青の網目が停止するタイミング(cycle==60)で自機狙い屈折レーザーの準備開始
    if (cycle == 60) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotAttacker;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}