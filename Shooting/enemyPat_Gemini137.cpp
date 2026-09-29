#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------------------------------------------------------
// フェーズ1：事象の地平（黒色重力巨弾 ＋ 影からの超高速追撃）
// ---------------------------------------------------------------------------
static void ShotPhase1(sEnemyShotSet* pEnemyShotSet)
{
    // 発生処理（初回フレームのみ）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // ボスを中心に黒色の大玉（重力源）を展開
        for (int i = 0; i < 8; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = pEnemyShotSet->muki + (i * DX_PI / 4.0);
            pShot->speed = 1.2 + (GetRand(100) / 200.0);
            pShot->kind = img_enemyShotLargeBall[7]; // 7:黒色の大玉
            pShot->param_i[0] = 0; // 0: 黒大玉, 1: 影からの針弾

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 毎フレームの弾挙動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 途中で弾を追加する可能性に備える

        if (pShot->param_i[0] == 0) { // 黒色大玉の処理
            // 自機方向にわずかに誘惑・重力曲折
            double angleToPlayer = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->muki += (angleToPlayer - pShot->muki) * 0.015;

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 15フレームごとに大玉の影から極小の超高速自機狙い針弾を生成
            if (pShot->count % 15 == 0 && pShot->count > 0) {
                if (!CheckSoundMem(sound_enemyShot_light)) {
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }
                sEnemyShot* pSubShot = new sEnemyShot;
                pSubShot->x = pShot->x;
                pSubShot->y = pShot->y;
                // 1ピクセルのランダムブレを加えた自機狙い
                double baseAngle = atan2(player.y - pShot->y, player.x - pShot->x);
                pSubShot->muki = baseAngle + ((GetRand(200) - 100) / 1000.0);
                pSubShot->speed = 6.5 + (GetRand(150) / 100.0);
                pSubShot->kind = img_enemyShotBullet[0]; // 0:赤色の銃弾/針弾
                pSubShot->param_i[0] = 1; // 1: 影からの針弾

                pSubShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSubShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSubShot;
                pEnemyShotSet->pEnemyShotHead->prev = pSubShot;
            }
        }
        else if (pShot->param_i[0] == 1) { // 影からの高速針弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// ---------------------------------------------------------------------------
// フェーズ2：量子重畳（青色観測待ち弾 ＋ 運動検知で赤色高速拡散弾化）
// ---------------------------------------------------------------------------
static void ShotPhase2(sEnemyShotSet* pEnemyShotSet)
{
    static double lastPlayerX = 240.0, lastPlayerY = 400.0;

    // 初回生成：画面内に浮遊する青色小玉（判定のない概念状態の演出）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 24; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = GetRand(440) + 20;
            pShot->y = GetRand(300) + 20;
            pShot->muki = (GetRand(3600) / 1000.0);
            pShot->speed = 0.3 + (GetRand(100) / 200.0);
            pShot->kind = img_enemyShotSmallBall[4]; // 4:青色小玉
            pShot->param_i[0] = 0; // 0: 未観測(青), 1: 観測後(赤)

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        lastPlayerX = player.x;
        lastPlayerY = player.y;
    }

    // 自機の移動量を検知
    double playerMoveDist = sqrt((player.x - lastPlayerX) * (player.x - lastPlayerX) +
        (player.y - lastPlayerY) * (player.y - lastPlayerY));

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) { // 青色弾（未観測）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 自機が大きく移動した際、自機周辺または未来位置にある青弾が即死赤弾へ変異
            if (playerMoveDist > 1.2) {
                double distToPlayer = sqrt((player.x - pShot->x) * (player.x - pShot->x) +
                    (player.y - pShot->y) * (player.y - pShot->y));
                if (distToPlayer < 120.0 && GetRand(100) < 35) {
                    pShot->param_i[0] = 1; // 赤化
                    pShot->kind = img_enemyShotDiamond[0]; // 0:赤色菱形弾
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x) + ((GetRand(60) - 30) / 180.0 * DX_PI);
                    pShot->speed = 4.8 + (GetRand(200) / 100.0);

                    if (!CheckSoundMem(sound_enemyShot_light)) {
                        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                    }
                }
            }
        }
        else { // 赤色弾（拡散移動）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    lastPlayerX = player.x;
    lastPlayerY = player.y;
}

// ---------------------------------------------------------------------------
// フェーズ3：特異点崩壊（全画面レーザー網 ＋ 1ピクセル移動安全地帯）
// ---------------------------------------------------------------------------
static void ShotPhase3(sEnemyShotSet* pEnemyShotSet)
{
    // 静的変数で動的な安全地帯（安地）の中心座標を管理
    static double safeX = 240.0;
    static double safeY = 300.0;
    static double safeTargetX = 240.0;
    static double safeTargetY = 300.0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        safeX = 240.0;
        safeY = 300.0;
        safeTargetX = 240.0;
        safeTargetY = 300.0;
    }

    // 20フレームごとに安地の目標地点を超高速移動（画面全体を不規則に駆け回る）
    if (pEnemyShotSet->count % 20 == 0) {
        safeTargetX = 40.0 + GetRand(400);
        safeTargetY = 100.0 + GetRand(320);
    }

    // 安地座標のなめらかな超高速移動
    safeX += (safeTargetX - safeX) * 0.15;
    safeY += (safeTargetY - safeY) * 0.15;

    // 毎フレーム、全方位・格子状に短レーザー網を配置（安地中心の僅かな半径のみ回避生成）
    if (pEnemyShotSet->count % 3 == 0) {
        for (int i = 0; i < 16; i++) {
            double spawnX = GetRand(480);
            double spawnY = GetRand(480);

            // 安地半径（約8ピクセル）内部には弾を生成しないことで「1ピクセル安地」を構築
            double distToSafe = sqrt((spawnX - safeX) * (spawnX - safeX) + (spawnY - safeY) * (spawnY - safeY));
            if (distToSafe > 8.0) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = spawnX;
                pShot->y = spawnY;
                pShot->muki = (i * DX_PI / 8.0) + ((GetRand(100) - 50) / 1000.0);
                pShot->speed = 3.5 + (GetRand(100) / 100.0);
                pShot->kind = img_enemyShotLaser[0]; // 0:赤色短レーザー

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 弾の進行処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------------------
// 敵本体のメイン制御パターン関数
// ---------------------------------------------------------------------------
void EnemyPat_TenYears_Gemini()
{
    static int muki;
    static int phase;

    // 初期化処理（count == 1）
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = 200;
        enemy.hp = 200; // 仕様により200で初期化
        muki = 1;
        phase = 1;

        // 予告音の再生
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // 敵本体の8の字ゆらぎ移動
        enemy.x += 1.2 * sin(count * 0.03);
        enemy.y = 100.0 + 20.0 * cos(count * 0.05);
    }

    // HP残量に応じた段階的フェーズ移行
    if (enemy.hp > 140) {
        phase = 1; // フェーズ1：黒色巨弾＆重力歪曲
    }
    else if (enemy.hp > 70) {
        phase = 2; // フェーズ2：量子重畳＆移動検知拡散
    }
    else {
        phase = 3; // フェーズ3：特異点崩壊＆超高速移動安地
    }

    // 周期的な ShotSet の生成
    bool shouldSpawn = false;
    sEnemyShotSet::PatternFunc targetFunc = nullptr;

    if (phase == 1 && count % 45 == 1) {
        shouldSpawn = true;
        targetFunc = ShotPhase1;
    }
    else if (phase == 2 && count % 30 == 1) {
        shouldSpawn = true;
        targetFunc = ShotPhase2;
    }
    else if (phase == 3 && count % 10 == 1) {
        shouldSpawn = true;
        targetFunc = ShotPhase3;
    }

    if (shouldSpawn && targetFunc != nullptr) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = targetFunc;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = phase;

        // 双方向ダミーヘッドの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リスト末尾への挿入
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}