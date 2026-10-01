// ============================================================
//  陰蜂風オリジナル弾幕：『冥蜂輪廻葬』
//  enemyPat_Tmp.cpp
//  敵本体関数：void EnemyPat_Inbachi_DeepSeek()
//
//  構成：
//   ・常時    ：逆回転する二重螺旋（黄／赤の小玉）
//   ・1.5秒毎 ：蜂卵弾（低速で飛来→停止→16分裂の針弾）
//   ・2.8秒毎 ：羽根レーザー（短レーザーの扇状掃引）
//   ・4秒毎   ：蜂の舞踏（ダッシュ＋軌跡の自機狙い針弾）
//   ・終盤    ：螺旋3系統化／卵の分裂前自機ドリフト
//
//  使用素材：
//   画像：img_enemyShotSmallBall[0/1/8]
//         img_enemyShotMediumBall[8]
//         img_enemyShotBullet[1]
//         img_enemyShotDiamond[1]
//         img_enemyShotLaser[5]
//   音  ：sound_enemyShot_heavy（蜂卵）
//         sound_enemyCharge    （羽根レーザー予告）
// ============================================================

#include "gv.h"
#include <cmath>


// ============================================================
//  ヘルパー
// ============================================================

// 弾をセットに1つ追加し、そのポインタを返す
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// 弾セットを新規作成してリストに追加
static sEnemyShotSet* CreateShotSet(double x, double y, double muki, int kind,
    sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// 弾を等速直線移動させる共通処理
static void MoveShots(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// ============================================================
//  弾幕1：二重螺旋（常時）
//   敵の左右の羽から、赤と黄色の小玉が逆方向に回転しながら発射
//   param_i[0] : 回転方向 (+1 / -1)
//   param_i[1] : 弾の色 (0=赤, 1=黄, 8=橙)
// ============================================================
static void ShotSpiral(sEnemyShotSet* pEnemyShotSet)
{
    // 敵本体に追従（螺旋の中心がボスの移動に合わせてずれる）
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    if (pEnemyShotSet->count % 3 == 0) {
        int dir = pEnemyShotSet->param_i[0];
        int color = pEnemyShotSet->param_i[1];

        // 時間と共に回転する角度
        double baseAngle = pEnemyShotSet->muki
            + pEnemyShotSet->count * 0.13 * dir;

        // 羽の位置から発射するイメージで僅かに左右へオフセット
        double wingOffset = (dir > 0) ? 14.0 : -14.0;

        sEnemyShot* pShot = AddShot(pEnemyShotSet);
        pShot->x = pEnemyShotSet->x + wingOffset;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = baseAngle;
        pShot->speed = 2.2;
        pShot->kind = img_enemyShotSmallBall[color];
        pShot->param_i[0] = 0;
    }

    MoveShots(pEnemyShotSet);
}


// ============================================================
//  弾幕2：蜂卵弾（1.5秒ごと）
//   低速の卵が飛び、一定時間後に停止→明滅→16発の針弾に分裂
//   param_i[0] : 0=通常弾, 1=卵飛行中, 2=卵停止中
// ============================================================
static void ShotEgg(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 5つの卵を扇状に発射
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pShot = AddShot(pEnemyShotSet);
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = pEnemyShotSet->muki + (i - 2) * 0.22;
            pShot->speed = 1.8;
            pShot->kind = img_enemyShotMediumBall[8]; // 橙の中玉
            pShot->param_i[0] = 1; // 卵・飛行中
        }
    }

    // 終盤（HP半分以下）では分裂前に自機方向へ少しドリフト
    bool isLastPhase = (enemy.hp <= enemy.maxHp / 2);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;

        if (pShot->param_i[0] == 1) {
            // 卵：飛行中
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            if (pShot->count >= 45) { // 0.75秒で停止
                pShot->speed = 0.0;
                pShot->param_i[0] = 2;
            }
        }
        else if (pShot->param_i[0] == 2) {
            // 卵：停止中
            if (isLastPhase && pShot->count == 60) {
                // 分裂直前に一度だけ自機方向へドリフト
                double ang = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->x += cos(ang) * 8.0;
                pShot->y += sin(ang) * 8.0;
            }

            if (pShot->count >= 70) { // 停止から約0.4秒後に分裂
                double px = pShot->x;
                double py = pShot->y;
                double baseAng = atan2(player.y - py, player.x - px);

                // 16発の針弾に分裂（扇状＋わずかなランダム角）
                for (int i = 0; i < 16; i++) {
                    sEnemyShot* pNew = AddShot(pEnemyShotSet);
                    pNew->x = px;
                    pNew->y = py;
                    pNew->muki = baseAng
                        + i * (2.0 * DX_PI / 16.0)
                        + GetRand(6) * 0.01;
                    pNew->speed = 2.8;
                    pNew->kind = img_enemyShotBullet[1]; // 黄色の銃弾
                    pNew->param_i[0] = 0;
                }

                // 卵本体を削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }
        else {
            // 分裂後の針弾：直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = next;
    }
}


// ============================================================
//  弾幕3：羽根レーザー（2.8秒ごと）
//   左右の羽から細い短レーザーが扇状に掃引される
// ============================================================
static void ShotLaserSweep(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count < 72 && pEnemyShotSet->count % 6 == 0) {
        int    step = pEnemyShotSet->count / 6; // 0..11
        double t = step / 12.0;              // 0..1

        // 左羽：下向き＋右から左へ掃引
        {
            sEnemyShot* pShot = AddShot(pEnemyShotSet);
            pShot->x = pEnemyShotSet->x - 40.0;
            pShot->y = pEnemyShotSet->y + 10.0;
            pShot->muki = DX_PI * (0.40 + 0.40 * t);
            pShot->speed = 6.0;
            pShot->kind = img_enemyShotLaser[5]; // マゼンタの短レーザー
            pShot->param_i[0] = 0;
        }
        // 右羽：下向き＋左から右へ掃引
        {
            sEnemyShot* pShot = AddShot(pEnemyShotSet);
            pShot->x = pEnemyShotSet->x + 40.0;
            pShot->y = pEnemyShotSet->y + 10.0;
            pShot->muki = DX_PI * (0.60 - 0.40 * t);
            pShot->speed = 6.0;
            pShot->kind = img_enemyShotLaser[5];
            pShot->param_i[0] = 0;
        }
    }

    MoveShots(pEnemyShotSet);
}


// ============================================================
//  弾幕4：蜂の舞踏（ダッシュの軌跡）
//   その場に停滞→0.7秒後に自機狙いの針弾へ変化
//   param_i[0] : 0=停滞中, 1=発射済み
// ============================================================
static void ShotDashTrail(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pShot = AddShot(pEnemyShotSet);
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = 0.0;
        pShot->speed = 0.0;
        pShot->kind = img_enemyShotDiamond[1]; // 黄色の菱形弾
        pShot->param_i[0] = 0;                 // 停滞中
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 停滞中
            if (pShot->count >= 42) { // 0.7秒
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->speed = 4.5;
                pShot->kind = img_enemyShotBullet[1]; // 黄色の針弾
                pShot->param_i[0] = 1;
            }
        }
        else {
            // 発射後
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}


// ============================================================
//  敵本体パターン：陰蜂風『冥蜂輪廻葬』
// ============================================================
void EnemyPat_Inbachi_DeepSeek()
{
    static int    moveDir;
    static int    shotCount;
    static bool   isDashing;
    static double dashTargetX;
    static bool   lastPhaseSpiralAdded;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        moveDir = 1;
        shotCount = 0;
        isDashing = false;
        dashTargetX = 240.0;
        lastPhaseSpiralAdded = false;

        // 常時：逆回転する二重螺旋（左羽=時計回り／右羽=反時計回り）
        {
            sEnemyShotSet* s = CreateShotSet(enemy.x, enemy.y, 0.0, 0, ShotSpiral);
            s->param_i[0] = +1;
            s->param_i[1] = 1;  // 黄
        }
        {
            sEnemyShotSet* s = CreateShotSet(enemy.x, enemy.y, DX_PI, 0, ShotSpiral);
            s->param_i[0] = -1;
            s->param_i[1] = 0;  // 赤
        }
    }
    else {
        // ---- 移動：左右往復 + 4秒ごとのダッシュ ----
        if (isDashing) {
            double dx = dashTargetX - enemy.x;
            enemy.x += dx * 0.15;
            if (dx < 1.0 && dx > -1.0) {
                enemy.x = dashTargetX;
                isDashing = false;
            }
        }
        else {
            enemy.x += 0.9 * moveDir;
            if (enemy.x < 80.0) moveDir = 1;
            if (enemy.x > 400.0) moveDir = -1;

            if (count % 240 == 120) {
                isDashing = true;
                dashTargetX = (enemy.x < 240.0) ? 380.0 : 100.0;
            }
        }

        // ---- 蜂の舞踏：ダッシュ中、軌跡に停滞弾を設置 ----
        if (isDashing && count % 6 == 0) {
            CreateShotSet(enemy.x, enemy.y, 0.0, shotCount++, ShotDashTrail);
        }

        // ---- 蜂卵弾：1.5秒ごと ----
        if (count % 90 == 30) {
            CreateShotSet(enemy.x, enemy.y + 10.0,
                atan2(player.y - enemy.y, player.x - enemy.x),
                shotCount++, ShotEgg);
        }

        // ---- 羽根レーザー：2.8秒ごと ----
        if (count % 168 == 60) {
            CreateShotSet(enemy.x, enemy.y + 10.0, 0.0,
                shotCount++, ShotLaserSweep);
        }

        // ---- 終盤：螺旋弾を3系統に増やす ----
        if (!lastPhaseSpiralAdded && enemy.hp <= enemy.maxHp / 2) {
            lastPhaseSpiralAdded = true;
            sEnemyShotSet* s = CreateShotSet(enemy.x, enemy.y,
                DX_PI * 0.5, 0, ShotSpiral);
            s->param_i[0] = +1;
            s->param_i[1] = 8;  // 橙
        }
    }
}