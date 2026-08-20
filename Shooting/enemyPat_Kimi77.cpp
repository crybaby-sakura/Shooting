// enemyPat_yoyo.cpp
// ヨーヨーをモチーフにした弾幕「スリーピング・ヨーヨー」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// --------------------------------------------------
// 弾幕パターン：スリーピング・ヨーヨー
// --------------------------------------------------
// フェーズ構成
//   0:Throw  - ボスから自機方向へ核弾（大玉）を投げ、紐弾が連なる
//   1:Sleep  - 最遠点で停止、紐弾が螺旋状に回転しながら内側へ収束
//   2:Return - ボスへ戻る、戻り道に弾の壁を形成
//   3:Burst  - ボス到達時、全弾が全方位へ拡散
// --------------------------------------------------
static void ShotSleepingYoyo(sEnemyShotSet* pSet)
{
    // ---- 初期化（count == 0 のみ実行） ----
    if (pSet->count == 0) {
        // 効果音：重めの発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // パラメータ初期化
        pSet->param_i[0] = 0;           // phase: 0=Throw, 1=Sleep, 2=Return, 3=Burst
        pSet->param_i[1] = 14 * 2;          // 紐弾の数
        pSet->param_d[0] = pSet->x;     // 核弾 現在X
        pSet->param_d[1] = pSet->y;     // 核弾 現在Y
        pSet->param_d[2] = pSet->x;     // 投げ始めX（ボス位置）
        pSet->param_d[3] = pSet->y;     // 投げ始めY
        pSet->param_d[4] = pSet->muki;  // 投げる角度
        pSet->param_d[5] = 220.0;       // 投げる距離
        pSet->param_d[6] = 0.0;         // Sleep回転角度
        pSet->param_d[7] = 3.2;         // 核弾移動速度
        pSet->param_d[8] = 0.0;         // 累計移動距離

        // 核弾（大玉・白）生成
        sEnemyShot* pCore = new sEnemyShot;
        pCore->x = pSet->param_d[0];
        pCore->y = pSet->param_d[1];
        pCore->muki = 0.0;
        pCore->speed = 0.0;
        pCore->kind = img_enemyShotLargeBall[6]; // 6:白
        pCore->param_i[0] = 0; // 役割：核弾
        pCore->margin = 480.0;
        pCore->prev = pSet->pEnemyShotHead->prev;
        pCore->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pCore;
        pSet->pEnemyShotHead->prev = pCore;

        // 紐弾（小玉・青）生成
        for (int i = 0; i < pSet->param_i[1]; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->param_d[0];
            p->y = pSet->param_d[1];
            p->muki = 0.0;
            p->speed = 0.0;
            p->kind = img_enemyShotSmallBall[4]; // 4:青
            p->param_i[0] = 1; // 役割：紐弾
            p->param_i[1] = i; // 紐番号
            p->margin = 480.0;
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // ---- ローカル変数展開 ----
    int phase = pSet->param_i[0];
    int stringCount = pSet->param_i[1];
    double coreX = pSet->param_d[0];
    double coreY = pSet->param_d[1];
    double startX = pSet->param_d[2];
    double startY = pSet->param_d[3];
    double angle = pSet->param_d[4];
    double throwDist = pSet->param_d[5];
    double coreSpeed = pSet->param_d[7];
    double moved = pSet->param_d[8];

    // ---- フェーズ遷移＆核弾位置更新 ----
    if (phase == 0) { // Throw：自機方向へ直進
        coreX += coreSpeed * cos(angle);
        coreY += coreSpeed * sin(angle);
        moved += coreSpeed;
        pSet->param_d[8] = moved;

        if (moved >= throwDist) {
            pSet->param_i[0] = 1; // Sleepへ
            pSet->param_d[6] = 0.0;
        }
    }
    else if (phase == 1) { // Sleep：核弾停止、紐弾が螺旋
        pSet->param_d[6] += 0.06; // 回転進行
        if (pSet->param_d[6] >= DX_PI * 3.0) { // 1.5周でReturnへ
            pSet->param_i[0] = 2;
        }
    }
    else if (phase == 2) { // Return：ボスへ直線回帰
        double dx = startX - coreX;
        double dy = startY - coreY;
        double dist = sqrt(dx * dx + dy * dy);
        double retSpeed = coreSpeed * 1.3;

        if (dist <= retSpeed) {
            coreX = startX;
            coreY = startY;
            pSet->param_i[0] = 3; // Burstへ
            // 到達効果音
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
        else {
            coreX += retSpeed * dx / dist;
            coreY += retSpeed * dy / dist;
        }
    }
    // phase==3 は各弾個別の直進（下のループで処理）

    // 核弾座標を書き戻し
    pSet->param_d[0] = coreX;
    pSet->param_d[1] = coreY;

    // ---- 各弾の位置更新 ----
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 0) { // ====== 核弾 ======
            if (phase == 3) {
                if (p->speed == 0.0) {
                    // Burst初回：ランダムな方向へ射出
                    p->muki = GetRand(359) / 180.0 * DX_PI;
                    p->speed = 3.5;
                }
                p->x += p->speed * cos(p->muki);
                p->y += p->speed * sin(p->muki);
            }
            else {
                p->x = coreX;
                p->y = coreY;
            }
        }
        else if (p->param_i[0] == 1) { // ====== 紐弾 ======
            int idx = p->param_i[1];

            if (phase == 0) { // Throw：ボス―核弾間を等間隔で補間
                double t = (double)(idx + 1) / (double)(stringCount + 1);
                p->x = startX + (coreX - startX) * t;
                p->y = startY + (coreY - startY) * t;
            }
            else if (phase == 1) { // Sleep：核弾周りを螺旋状に回転・収束
                double baseAngle = DX_PI * 2.0 * idx / stringCount;
                double currentRot = pSet->param_d[6] / 3;
                double radius = 70.0 * (1.0 - currentRot * 3 / (DX_PI * 3.0)) * 3;
                if (radius < 5.0) radius = 5.0;
                p->x = coreX + radius * cos(baseAngle + currentRot);
                p->y = coreY + radius * sin(baseAngle + currentRot);
            }
            else if (phase == 2) { // Return：ボス―核弾間を等間隔で補間（弾の壁）
                double t = (double)(idx + 1) / (double)(stringCount + 1);
                p->x = startX + (coreX - startX) * t;
                p->y = startY + (coreY - startY) * t;
            }
            else if (phase == 3) { // Burst：全方位へ拡散
                if (p->speed == 0.0) {
                    // 均等な方向＋若干のランダムずれ
                    p->muki = (DX_PI * 2.0 * idx / stringCount)
                        + (GetRand(20) - 10) / 180.0 * DX_PI;
                    p->speed = 2.0 + GetRand(25) / 10.0;
                }
                p->x += p->speed * cos(p->muki);
                p->y += p->speed * sin(p->muki);
            }
        }
        p = p->next;
    }
}

// --------------------------------------------------
// 敵本体パターン
// --------------------------------------------------
void EnemyPat_Yoyo_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 90フレーム毎にヨーヨー弾幕を生成
    if (count % 90 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSleepingYoyo;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
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