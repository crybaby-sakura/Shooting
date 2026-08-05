// enemyPat_Tmp.cpp
// 超巨大弾「巨弾の呼吸・潮汐弾幕」実装
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾幕パターン：巨弾の呼吸・潮汐弾幕
// 既存の大玉(img_enemyShotLargeBall)を円環状に多数配置して
// 超巨大弾を擬似表現する。param / count で状態管理。
// ------------------------------------------------------------
static void ShotBreathGiant(sEnemyShotSet* pEnemyShotSet)
{
    // param_d[0] : 現在の半径
    // param_d[1] : 回転角度（rad）
    // param_d[2] : 中心X（敵位置を追従）
    // param_d[3] : 中心Y
    // param_i[0] : フェーズ 0=出現 1=呼吸 2=崩落
    // param_i[1] : 呼吸回数カウンタ
    // param_i[2] : 表面弾の色インデックス

    const int SURFACE_NUM = 36;          // 巨大弾を構成する大玉の数
    const double MAX_RADIUS = 160.0;     // 最大半径（画面の約2/3）
    const double MIN_RADIUS = 40.0;      // 最小半径
    const double APPEAR_SPEED = 2.2;     // 出現時の膨張速度
    const int BREATH_CYCLE = 90;         // 1回の呼吸に要するフレーム
    const int BREATH_TIMES = 4;          // 呼吸回数

    sEnemyShot* pEnemyShot;

    // ---------- 初回生成 ----------
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_d[0] = 8.0;               // 初期半径
        pEnemyShotSet->param_d[1] = 0.0;               // 回転
        pEnemyShotSet->param_d[2] = pEnemyShotSet->x;  // 中心X
        pEnemyShotSet->param_d[3] = pEnemyShotSet->y;  // 中心Y
        pEnemyShotSet->param_i[0] = 0;                 // 出現フェーズ
        pEnemyShotSet->param_i[1] = 0;                 // 呼吸回数
        pEnemyShotSet->param_i[2] = 0;                 // 色（赤スタート）

        // 表面を構成する大玉を円環状に配置（速度0で固定し、毎フレーム位置を上書き）
        for (int i = 0; i < SURFACE_NUM; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = (DX_PI * 2.0 / SURFACE_NUM) * i;
            pEnemyShot->x = pEnemyShotSet->param_d[2] + pEnemyShotSet->param_d[0] * cos(ang);
            pEnemyShot->y = pEnemyShotSet->param_d[3] + pEnemyShotSet->param_d[0] * sin(ang);
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤の大玉
            pEnemyShot->param_i[0] = i;                   // 自身のインデックス
            pEnemyShot->param_d[0] = ang;                 // 基準角度
            pEnemyShot->margin = 240;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        return;
    }

    // 中心を敵位置に緩やかに追従
    pEnemyShotSet->param_d[2] += (enemy.x - pEnemyShotSet->param_d[2]) * 0.08;
    pEnemyShotSet->param_d[3] += (enemy.y + 30.0 - pEnemyShotSet->param_d[3]) * 0.08;

    double& radius = pEnemyShotSet->param_d[0];
    double& rot = pEnemyShotSet->param_d[1];
    int& phase = pEnemyShotSet->param_i[0];
    int& breathCnt = pEnemyShotSet->param_i[1];
    int& colorIdx = pEnemyShotSet->param_i[2];

    // ---------- フェーズ処理 ----------
    if (phase == 0) {
        // 出現：半径を最大まで伸ばす
        radius += APPEAR_SPEED;
        if (radius >= MAX_RADIUS) {
            radius = MAX_RADIUS;
            phase = 1;
            breathCnt = 0;
            // 出現完了音
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }
    else if (phase == 1) {
        // 呼吸：サイン波で膨張・収縮を繰り返す
        int local = pEnemyShotSet->count % BREATH_CYCLE;
        double t = (double)local / BREATH_CYCLE; // 0〜1
        // 0.0〜0.5 で膨張、0.5〜1.0 で収縮
        if (t < 0.5) {
            radius = MIN_RADIUS + (MAX_RADIUS - MIN_RADIUS) * (t * 2.0);
        }
        else {
            radius = MAX_RADIUS - (MAX_RADIUS - MIN_RADIUS) * ((t - 0.5) * 2.0);
        }

        // 1サイクル終了ごとに呼吸回数を加算し、色を変更
        if (local == 0 && pEnemyShotSet->count > 0) {
            breathCnt++;
            colorIdx = (colorIdx + 1) % 9; // 0〜8を循環
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 規定回数で崩落へ
        if (breathCnt >= BREATH_TIMES) {
            phase = 2;
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        // ---- 表面からの弾放射 ----
        // 膨張時：細いレーザー状弾を放射
        if (t < 0.48 && local % 4 == 0) {
            for (int i = 0; i < 12; i++) {
                pEnemyShot = new sEnemyShot;
                double ang = rot + (DX_PI * 2.0 / 12.0) * i + (GetRand(20) - 10) * 0.01;
                pEnemyShot->x = pEnemyShotSet->param_d[2] + radius * cos(ang);
                pEnemyShot->y = pEnemyShotSet->param_d[3] + radius * sin(ang);
                pEnemyShot->muki = ang;
                pEnemyShot->speed = 3.8 + GetRand(15) * 0.1;
                pEnemyShot->kind = img_enemyShotLaser[colorIdx % 9];
                pEnemyShot->margin = 40;
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        // 収縮時：螺旋状に中玉を噴出
        if (t >= 0.52 && local % 3 == 0) {
            for (int i = 0; i < 8; i++) {
                pEnemyShot = new sEnemyShot;
                double ang = rot + (DX_PI * 2.0 / 8.0) * i + t * 6.0;
                pEnemyShot->x = pEnemyShotSet->param_d[2] + radius * 0.6 * cos(ang);
                pEnemyShot->y = pEnemyShotSet->param_d[3] + radius * 0.6 * sin(ang);
                pEnemyShot->muki = ang + DX_PI * 0.5; // 接線方向寄り
                pEnemyShot->speed = 2.2 + GetRand(10) * 0.1;
                pEnemyShot->kind = img_enemyShotMediumBall[(colorIdx + 3) % 9];
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        // 収縮の最中に波紋状の小玉
        if (t >= 0.55 && t < 0.75 && local % 5 == 0) {
            for (int i = 0; i < 18; i++) {
                pEnemyShot = new sEnemyShot;
                double ang = (DX_PI * 2.0 / 18.0) * i + rot * 0.5;
                pEnemyShot->x = pEnemyShotSet->param_d[2] + radius * cos(ang);
                pEnemyShot->y = pEnemyShotSet->param_d[3] + radius * sin(ang);
                pEnemyShot->muki = ang;
                pEnemyShot->speed = 1.6 + GetRand(8) * 0.1;
                pEnemyShot->kind = img_enemyShotSmallBall[(colorIdx + 6) % 9];
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    else if (phase == 2) {
        // 崩落：急激に膨張して消滅し、破片を撒く
        radius += 6.5;
        if (radius > MAX_RADIUS + 80.0) {
            // 破片弾を一斉放射して終了
            for (int i = 0; i < 48; i++) {
                pEnemyShot = new sEnemyShot;
                double ang = (DX_PI * 2.0 / 48.0) * i + GetRand(30) * 0.02;
                pEnemyShot->x = pEnemyShotSet->param_d[2];
                pEnemyShot->y = pEnemyShotSet->param_d[3];
                pEnemyShot->muki = ang;
                pEnemyShot->speed = 2.5 + GetRand(25) * 0.1;
                // 大玉・中玉・菱形を混ぜる
                int r = GetRand(2);
                if (r == 0) pEnemyShot->kind = img_enemyShotLargeBall[GetRand(8)];
                else if (r == 1) pEnemyShot->kind = img_enemyShotMediumBall[GetRand(8)];
                else pEnemyShot->kind = img_enemyShotDiamond[GetRand(8)];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
            // 表面の大玉をすべて高速で外側へ飛ばす
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->speed == 0.0) { // 表面構成弾
                    pShot->speed = 4.0 + GetRand(20) * 0.1;
                    pShot->muki = pShot->param_d[0] + rot;
                }
                pShot = pShot->next;
            }
            // このショットセットは以降何もしない（自然消滅待ち）
            phase = 3;
        }
    }

    // ---------- 表面大玉の位置更新（毎フレーム） ----------
    rot += 0.025; // ゆっくり回転
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 速度0の弾だけを表面構成弾として扱う
        if (pShot->speed == 0.0 && phase < 3) {
            double ang = pShot->param_d[0] + rot;
            pShot->x = pEnemyShotSet->param_d[2] + radius * cos(ang);
            pShot->y = pEnemyShotSet->param_d[3] + radius * sin(ang);
            // 色も呼吸に合わせて変化
            pShot->kind = img_enemyShotLargeBall[colorIdx % 9];
        }
        else {
            // 通常弾の移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_HugeBullet_Grok()
{
    static int moveDir;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;
    }
    else {
        // ゆっくり左右に揺れる
        enemy.x += 0.55 * (double)moveDir;
        if (enemy.x < 120.0) moveDir = 1;
        if (enemy.x > 360.0) moveDir = -1;
    }

    // 開始直後に一度だけ巨大弾ショットセットを生成
    if (count % 540 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBreathGiant;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;
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