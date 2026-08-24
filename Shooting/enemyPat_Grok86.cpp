// enemyPat_Tmp.cpp
// 近接弾幕パターン：「突進斬撃＋拡散弾」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン関数群
// ============================================================

// 予兆用：ボス周囲からゆっくり放射状に小さな弾を出す
static void ShotWarning(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 周囲に低速小玉を放射（威嚇）
        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * (2.0 * DX_PI / 12.0) + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.2 + GetRand(8) / 10.0;  // 1.2〜2.0
            pEnemyShot->kind = img_enemyShotSmallBall[0];  // 赤の小玉
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動のみ（画面外消去はメインルーチン担当）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 突進中：進行方向に対して左右扇状に弾を連続発射
static void ShotChargeFan(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 一定間隔で扇状弾を追加生成（countはメインでインクリメント済み）
    if (pEnemyShotSet->count % 4 == 0 && pEnemyShotSet->count <= 120) {
        if (pEnemyShotSet->count % 8 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 進行方向（param_d[0]に保存）を基準に左右に広げる
        double baseMuki = pEnemyShotSet->param_d[0];
        int num = 5;  // 左右に広がる本数
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(10) - 5);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(10) - 5);
            // 中心から左右に扇状（±40度程度）
            double offset = (i - (num - 1) / 2.0) * (20.0 / 180.0 * DX_PI);
            pEnemyShot->muki = baseMuki + offset + (GetRand(10) - 5) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.5 + GetRand(15) / 10.0;  // 3.5〜5.0
            // 赤〜橙の菱形弾で攻撃感を出す
            pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotDiamond[0] : img_enemyShotDiamond[8];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // セット自体の位置をボスに追従させる（突進中）
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 斬撃時：前方扇形に高速弾を一気に放出
static void ShotSlash(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 斬撃の弧に沿って扇状高速弾（約120度）
        double baseMuki = pEnemyShotSet->muki;
        int num = 15;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            double offset = (i - (num - 1) / 2.0) * (8.0 / 180.0 * DX_PI);  // ±約56度
            pEnemyShot->muki = baseMuki + offset;
            pEnemyShot->speed = 5.5 + GetRand(20) / 10.0;  // 5.5〜7.5
            // 中楕円弾（赤）で斬撃感を強調
            pEnemyShot->kind = img_enemyShotMediumOval[0];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 斬撃本体の「当たり判定」として大きめの弾も少し出す（近接感）
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + cos(baseMuki) * (15.0 + i * 12.0);
            pEnemyShot->y = pEnemyShotSet->y + sin(baseMuki) * (15.0 + i * 12.0);
            pEnemyShot->muki = baseMuki;
            pEnemyShot->speed = 2.0;
            pEnemyShot->kind = img_enemyShotLargeBall[0];  // 大玉で近接ヒットボックス風
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 余波：後退しながら低速の軽い誘導弾を発射
static void ShotResidual(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // プレイヤー方向に少しずれた低速弾を数発
        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(30) - 15);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(20) - 10);
            double toPlayer = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            pEnemyShot->muki = toPlayer + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.8 + GetRand(12) / 10.0;  // 1.8〜3.0
            pEnemyShot->kind = img_enemyShotMediumBall[0];  // 赤中玉
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動（簡易誘導は初期方向のみ。必要ならここを拡張）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_CloseCombat_Grok()
{
    // 状態管理用（staticで保持）
    static int phase = 0;          // 0:待機, 1:予兆, 2:突進, 3:斬撃, 4:後退
    static int phaseTimer = 0;     // 各フェーズの経過フレーム
    static double chargeMuki = 0.0; // 突進方向
    static double startX = 0.0, startY = 0.0;

    // 初期化（1フレーム目）
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        phaseTimer = 0;
    }

    // フェーズ進行
    phaseTimer++;

    switch (phase) {
    case 0: // 待機（少し左右に揺れる）
        enemy.x += 0.6 * sin(count * 0.05);
        if (phaseTimer >= 90) {  // 約1.5秒待機後に予兆へ
            phase = 1;
            phaseTimer = 0;

            // 予兆ショットセット生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotWarning;
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
        break;

    case 1: // 予兆（約0.8秒）
        // ボスはその場で微振動
        enemy.x += (GetRand(4) - 2) * 0.3;
        enemy.y += (GetRand(4) - 2) * 0.2;
        if (phaseTimer >= 48) {
            phase = 2;
            phaseTimer = 0;
            // 突進方向をプレイヤーへ固定
            chargeMuki = atan2(player.y - enemy.y, player.x - enemy.x);
            startX = enemy.x;
            startY = enemy.y;

            // 突進中ファンショットセット生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotChargeFan;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = chargeMuki;
            pSet->param_d[0] = chargeMuki;  // 進行方向を保存
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        break;

    case 2: // 高速突進
    {
        const double chargeSpeed = 9.0;
        enemy.x += chargeSpeed * cos(chargeMuki);
        enemy.y += chargeSpeed * sin(chargeMuki);

        // プレイヤーに十分近づいた、または一定時間経過、または画面端で斬撃へ
        double dist = hypot(player.x - enemy.x, player.y - enemy.y);
        if (dist < 55.0 || phaseTimer >= 35 ||
            enemy.x < 20.0 || enemy.x > 460.0 || enemy.y < 20.0 || enemy.y > 460.0) {
            phase = 3;
            phaseTimer = 0;

            // 斬撃ショットセット生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSlash;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = chargeMuki;  // 突進してきた方向に斬る
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        break;
    }

    case 3: // 斬撃（短時間）
        // 斬撃中はほぼ停止
        if (phaseTimer >= 12) {
            phase = 4;
            phaseTimer = 0;

            // 余波ショットセット生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotResidual;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        break;

    case 4: // 後退
    {
        // 突進してきた方向の逆へ後退
        const double retreatSpeed = 4.5;
        enemy.x -= retreatSpeed * cos(chargeMuki);
        enemy.y -= retreatSpeed * sin(chargeMuki);

        // 画面上部付近に戻ったら待機へ
        if (enemy.y < 100.0 || phaseTimer >= 40) {
            // 位置を画面内にクランプ
            if (enemy.x < 40.0) enemy.x = 40.0;
            if (enemy.x > 440.0) enemy.x = 440.0;
            if (enemy.y < 40.0) enemy.y = 40.0;
            phase = 0;
            phaseTimer = 0;
        }
        break;
    }
    }

    // 画面外に出ないよう簡易クランプ（突進中以外）
    if (phase != 2) {
        if (enemy.x < 30.0) enemy.x = 30.0;
        if (enemy.x > 450.0) enemy.x = 450.0;
        if (enemy.y < 30.0) enemy.y = 30.0;
        if (enemy.y > 200.0) enemy.y = 200.0;  // ボスは上半分寄りに留める
    }
}