// enemyPat_Tmp.cpp
// ヒュドラをモチーフにした弾幕パターン「多頭再生弾幕」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 通常頭部射撃：細いレーザー風の連射（緑系）
// ============================================================
static void ShotHeadLaser(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 一定間隔で発射
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 1発の短レーザーをプレイヤー方向へ
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        // 少しランダムに散らす
        pEnemyShot->muki += (GetRand(40) - 20) / 180.0 * DX_PI;
        pEnemyShot->speed = 3.5 + GetRand(10) / 10.0;
        // 短レーザー・緑
        pEnemyShot->kind = img_enemyShotLaser[2];
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 弾移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 再生の種弾：破壊時に放射状に飛び、停止後に次の弾幕の種になる
// param_i[0] : フェーズ (0=飛行, 1=停止待機)
// param_d[0] : 目標停止時間
// ============================================================
static void ShotRegenSeed(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ種弾を生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int num = 10 + GetRand(4); // 10〜14発
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 全方向放射
            pEnemyShot->muki = (DX_PI * 2.0 * i) / num + (GetRand(10) - 5) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.0 + GetRand(15) / 10.0;
            // 小玉・緑
            pEnemyShot->kind = img_enemyShotSmallBall[2];
            // フェーズ管理
            pEnemyShot->param_i[0] = 0; // 飛行中
            pEnemyShot->param_d[0] = 40.0 + GetRand(20); // 停止までのフレーム目安
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の挙動更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 飛行フェーズ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->speed *= 0.97; // 徐々に減速

            if (pShot->count >= (int)pShot->param_d[0] || pShot->speed < 0.3) {
                pShot->param_i[0] = 1; // 停止待機へ
                pShot->speed = 0.0;
                pShot->param_d[1] = pShot->count + 30.0 + GetRand(20); // 再生開始までの待機
            }
        }
        else if (pShot->param_i[0] == 1) {
            // 停止待機。時間になったら螺旋弾を生成して自身は消える準備
            if (pShot->count >= (int)pShot->param_d[1]) {
                // 螺旋弾をこの位置から生成（別ショットセットとして後で扱うため、ここでは小さな弾を追加）
                // 簡易的にここで螺旋の種を数発出す
                for (int k = 0; k < 3; k++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    pNew->muki = pShot->muki + (k - 1) * 0.4;
                    pNew->speed = 1.8;
                    pNew->kind = img_enemyShotScale[2]; // 鱗弾・緑
                    pNew->param_i[0] = 2; // 螺旋用フラグ
                    pNew->param_d[0] = 0.08 + GetRand(5) / 100.0; // 角速度
                    pNew->param_d[1] = pNew->speed;
                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;
                }
                // 種弾自体は画面外扱いにするため大きく移動させる（メインルーチンで消去）
                pShot->x = -9999.0;
                pShot->param_i[0] = 99;
            }
        }
        else if (pShot->param_i[0] == 2) {
            // 螺旋弾の動き
            pShot->muki += pShot->param_d[0];
            pShot->speed = pShot->param_d[1] + pShot->count * 0.015; // 徐々に加速
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 再生完了後の強化頭部射撃：中玉＋菱形の混合
// ============================================================
static void ShotHeadReinforced(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 3方向に中玉
        for (int i = -1; i <= 1; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x) + i * 0.25;
            pEnemyShot->speed = 2.8 + GetRand(8) / 10.0;
            pEnemyShot->kind = img_enemyShotMediumBall[2]; // 中玉・緑
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // たまに菱形弾を追加
    if (pEnemyShotSet->count == 5) {
        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x)
                + (i - 2) * 0.18;
            pEnemyShot->speed = 3.2;
            pEnemyShot->kind = img_enemyShotDiamond[2]; // 菱形・緑
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 胴体からの牽制弾（大玉・低速）
// ============================================================
static void ShotBodyWarn(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 低速大玉をプレイヤー方向と左右に
        for (int i = -1; i <= 1; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x) + i * 0.4;
            pEnemyShot->speed = 1.6 + GetRand(6) / 10.0;
            pEnemyShot->kind = img_enemyShotLargeBall[2]; // 大玉・緑
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン：多頭再生弾幕
// ============================================================
void EnemyPat_Hydra_Grok()
{
    // 頭部管理用static
    static int headNum;          // 現在の頭部数
    static double headOffsetX[8]; // 頭部の相対X
    static double headOffsetY[8]; // 頭部の相対Y
    static int headState[8];     // 0:通常 1:破壊演出中 2:強化後
    static int headTimer[8];     // 各頭部のタイマー
    static int phase;            // 全体フェーズ
    static int regenCount;       // 再生回数
    static int moveDir = 1;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;

        headNum = 3;
        phase = 0;
        regenCount = 0; 
        moveDir = 1;

        // 初期頭部位置（円弧状）
        for (int i = 0; i < 8; i++) {
            headOffsetX[i] = 0.0;
            headOffsetY[i] = 0.0;
            headState[i] = 0;
            headTimer[i] = 0;
        }
        headOffsetX[0] = -70.0; headOffsetY[0] = -20.0;
        headOffsetX[1] = 0.0; headOffsetY[1] = -35.0;
        headOffsetX[2] = 70.0; headOffsetY[2] = -20.0;
    }
    else {
        // 胴体のゆるやかな左右移動
        enemy.x += 0.6 * moveDir;
        if (enemy.x > 320.0) moveDir = -1;
        if (enemy.x < 160.0) moveDir = 1;

        // 頭部位置をわずかに揺らす
        for (int i = 0; i < headNum; i++) {
            headOffsetX[i] += sin((count + i * 40) * 0.03) * 0.15;
            headOffsetY[i] += cos((count + i * 55) * 0.025) * 0.1;
        }
    }

    // --------------------------------------------------------
    // 頭部の破壊・再生判定（時間とHPで制御）
    // --------------------------------------------------------
    // HPが減るほど再生が早く・多くなる
    int destroyInterval = 180 - (200 - enemy.hp) / 4;
    if (destroyInterval < 90) destroyInterval = 90;

    for (int i = 0; i < headNum; i++) {
        headTimer[i]++;

        // 通常状態から一定時間で破壊演出へ
        if (headState[i] == 0 && headTimer[i] >= destroyInterval + i * 20) {
            headState[i] = 1;
            headTimer[i] = 0;

            // 再生の種弾セットを生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotRegenSeed;
            pSet->x = enemy.x + headOffsetX[i];
            pSet->y = enemy.y + headOffsetY[i];
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;

            // 予告音
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // 破壊演出終了 → 強化状態へ（再生完了）
        if (headState[i] == 1 && headTimer[i] >= 90) {
            headState[i] = 2;
            headTimer[i] = 0;
            regenCount++;

            // 頭部数を増やす（最大6）
            if (headNum < 6 && regenCount % 2 == 0) {
                int newIdx = headNum;
                headNum++;
                // 新しい頭部を外側に配置
                double angle = (DX_PI * 1.2 * newIdx) / 6.0 - DX_PI * 0.6;
                headOffsetX[newIdx] = cos(angle) * (80.0 + newIdx * 8.0);
                headOffsetY[newIdx] = sin(angle) * 30.0 - 25.0;
                headState[newIdx] = 2;
                headTimer[newIdx] = 0;
            }
        }

        // 強化状態も一定時間で再び破壊可能に戻す（ループ）
        if (headState[i] == 2 && headTimer[i] >= destroyInterval + 60) {
            headState[i] = 0;
            headTimer[i] = 0;
        }
    }

    // --------------------------------------------------------
    // 各頭部からの通常／強化射撃
    // --------------------------------------------------------
    for (int i = 0; i < headNum; i++) {
        // 発射タイミングを頭部ごとにずらす
        if ((count + i * 7) % 30 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->x = enemy.x + headOffsetX[i];
            pSet->y = enemy.y + headOffsetY[i];
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
            pSet->kind = i;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;

            if (headState[i] == 2) {
                pSet->patternFunc = ShotHeadReinforced;
            }
            else {
                pSet->patternFunc = ShotHeadLaser;
            }
        }
    }

    // --------------------------------------------------------
    // 胴体からの牽制弾（低頻度）
    // --------------------------------------------------------
    if (count % 50 == 10) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBodyWarn;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 15.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}