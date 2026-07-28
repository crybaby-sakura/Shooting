// enemyPat_Tmp.cpp
// アリジゴクをモチーフにした弾幕「底なし蟻地獄」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

//------------------------------------------------------------
// 弾追加用の簡易ヘルパー
//------------------------------------------------------------
static sEnemyShot* AddBullet(sEnemyShotSet* pSet,
    double x, double y,
    int    kindImg,
    double muki, double speed,
    int    type)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kindImg;
    pShot->count = 0;
    pShot->param_i[0] = type;   // 弾の種類ラベル
   
    // リスト先頭に挿入
    pShot->prev = pSet->pEnemyShotHead;
    pShot->next = pSet->pEnemyShotHead->next;
    pSet->pEnemyShotHead->next->prev = pShot;
    pSet->pEnemyShotHead->next = pShot;

    return pShot;
}

//------------------------------------------------------------
// アリジゴク弾幕のメインパターン関数
//------------------------------------------------------------
static void AntlionPitPattern(sEnemyShotSet* pSet)
{
    int    phase = pSet->count;   // 12秒周期 (60fps想定)
    double centerX = pSet->x;             // 240
    double centerY = pSet->y;             // 440（画面下中央）

    // 渦の角度をフェーズ先頭でリセット（不要な継続を防ぐ）
    if (phase == 0) {
        pSet->param_d[0] = 0.0;
    }

    // ---------- 各フェーズの弾生成 ----------
    // Phase 1 : 引き込みの渦 (0～239)
    if (phase < 240) {
        // 渦を構成する砂粒弾（黄褐色の中玉）
        if (phase % 1 == 0) {
            double angle = pSet->param_d[0];
            pSet->param_d[0] += 0.5;

            sEnemyShot* p = AddBullet(pSet, centerX, centerY,
                img_enemyShotMediumBall[1], // 黄
                angle, 0.0, 1);
            p->param_d[0] = 0.0;        // r (動径)
            p->param_d[1] = angle;      // θ
            p->param_d[2] = 4.0;        // 動径初速
            p->param_d[3] = 2.0;        // 接線速度
            p->margin = 960;
        }

        // 左右の斜面を落ちる砂弾（橙色の小玉）
        if (phase % 5 == 0) {
            sEnemyShot* pL = AddBullet(pSet, 10, -10,
                img_enemyShotSmallBall[8], // 橙
                0, 0, 5);
            pL->param_d[0] = 2.0;   // 速度
            sEnemyShot* pR = AddBullet(pSet, 470, -10,
                img_enemyShotSmallBall[8], // 橙
                0, 0, 5);
            pR->param_d[0] = 2.0;
        }
    }

    // Phase 2 : 砂かけ (240～419)
    else if (phase >= 240 && phase < 420) {
        if (phase == 240) {
            if (CheckSoundMem(sound_enemyShot_heavy))
                StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // 放物線を描く砂粒（橙の中玉）30個
            for (int i = 0; i < 30; ++i) {
                double ang = -DX_PI / 2.0 + (GetRand(140) - 70) / 100.0;
                sEnemyShot* p = AddBullet(pSet, centerX, centerY,
                    img_enemyShotMediumBall[8], // 橙
                    ang, 4.0, 2);
                p->param_d[0] = 4.0 * cos(ang);  // vx
                p->param_d[1] = 4.0 * sin(ang);  // vy
                p->param_i[1] = 0;                // 分裂済みフラグ
            }

            // ランダムに飛ぶ黒い小石（黒の大玉）6個
            for (int i = 0; i < 6; ++i) {
                double ang = GetRand(360) / 180.0 * DX_PI;
                sEnemyShot* p = AddBullet(pSet, centerX, centerY,
                    img_enemyShotLargeBall[7], // 黒
                    ang, 1.5, 4);
                p->param_d[0] = 1.5 * cos(ang);
                p->param_d[1] = 1.5 * sin(ang);
            }
        }
    }

    // Phase 3 : 捕食の顎 (420～539)
    else if (phase >= 420 && phase < 540) {
        if (phase == 420) {
            if (CheckSoundMem(sound_enemyShot_extreme))
                StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            // 縦一列に並ぶ針弾（赤レーザー）8本、下から上へ
            for (int i = 0; i < 8; ++i) {
                double yy = 480.0 - i * 30.0;   // 400, 385, 370, ...
                sEnemyShot* p = AddBullet(pSet, centerX, yy,
                    img_enemyShotLaser[0], // 赤
                    -DX_PI / 2.0, 5.0, 6); // type=6: 顎の針
                p->param_d[0] = 0.0;   // 分岐済みフラグ (0=未分岐)
                p->param_d[1] = yy;    // 生成時のy座標（分岐判定用）
                p->margin = 960;
            }
        }
    }

    // Phase 4 : 引き戻しと再構築 (540～719) … 生成は行わない

    // ---------- 全弾の移動処理 ----------
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int type = pShot->param_i[0];

        // フェーズ4での吸い込み処理（小石(type4)と横枝(type7)のみ）
        if (phase >= 540 && (type == 4 || type == 7)) {
            double dx = centerX - pShot->x;
            double dy = centerY - pShot->y;
            pShot->x += 0.05 * dx;
            pShot->y += 0.05 * dy;
            if (dx * dx + dy * dy < 100.0) {
                pShot->y = 9999.0;   // 中心到達で消去
            }
            pShot = pShot->next;
            continue;
        }

        // タイプ別の移動
        switch (type) {
        case 1: // 渦・螺旋弾
        {
            pShot->param_d[0] += pShot->param_d[2];   // r増加
            pShot->param_d[2] -= 0.015;               // 動径速度減少
            double r = pShot->param_d[0];
            if (r > 1.0)
                pShot->param_d[1] += pShot->param_d[3] / r;
            else
                pShot->param_d[1] += pShot->param_d[3];
            pShot->x = centerX + r * cos(pShot->param_d[1]);
            pShot->y = centerY + r * sin(pShot->param_d[1]);
            if (pShot->count >= 540) {
                pShot->y = 9999.0;
            }
            break;
        }
        case 2: // 砂かけ主弾（放物線）
        {
            double vx = pShot->param_d[0];
            double vy = pShot->param_d[1];
            vy += 0.08;                // 重力
            pShot->param_d[1] = vy;
            pShot->x += vx;
            pShot->y += vy;

            // 頂点付近で分裂（vy >= 0）
            if (pShot->param_i[1] == 0 && vy >= 0.0) {
                pShot->param_i[1] = 1;
                for (int i = 0; i < 6; ++i) {
                    double ang = GetRand(360) / 180.0 * DX_PI;
                    sEnemyShot* f = AddBullet(pSet, pShot->x, pShot->y,
                        img_enemyShotSmallBall[8], // 橙
                        ang, 2.0, 3); // type=3: 分裂粒
                    f->param_d[0] = 2.0 * cos(ang);
                    f->param_d[1] = 2.0 * sin(ang);
                }
                pShot->y = 9999.0;   // 元弾消去
            }
            break;
        }
        case 3: // 分裂後の小さな砂粒（等速直線）
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
            break;

        case 4: // 小石（壁で跳ね返る）
        {
            double vx = pShot->param_d[0];
            double vy = pShot->param_d[1];
            pShot->x += vx;
            pShot->y += vy;
            if (pShot->x < 0.0) { pShot->x = 0.0;   vx *= -1.0; }
            if (pShot->x > 480.0) { pShot->x = 480.0; vx *= -1.0; }
            if (pShot->y < 0.0) { pShot->y = 0.0;   vy *= -1.0; }
            if (pShot->y > 480.0) { pShot->y = 480.0; vy *= -1.0; }
            pShot->param_d[0] = vx;
            pShot->param_d[1] = vy;
            break;
        }
        case 5: // 壁の落砂弾（中心へ等速移動）
        {
            double dx = centerX - pShot->x;
            double dy = centerY - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > 0.0) {
                pShot->x += pShot->param_d[0] * dx / dist;
                pShot->y += pShot->param_d[0] * dy / dist;
            }
            if (dist < 20.0) pShot->y = 9999.0; // 中心付近で消去
            break;
        }
        case 6: // 顎の針（垂直上昇 → 分岐して消滅）
        {
            // 上へ移動
            pShot->y -= pShot->speed;   // speed = 3.0

            // 未分岐で一定ライン(y<=200)を超えたら左右に枝を出す
            if (pShot->param_d[0] == 0.0 && pShot->y <= 10.0) {
                pShot->param_d[0] = 1.0;   // 分岐済み

                // 左横枝
                sEnemyShot* left = AddBullet(pSet, pShot->x, pShot->y,
                    img_enemyShotDiamond[0], // 赤菱形
                    DX_PI, 2.5, 7); // type=7
                left->param_d[0] = -2.5;   // x速度

                // 右横枝
                sEnemyShot* right = AddBullet(pSet, pShot->x, pShot->y,
                    img_enemyShotDiamond[0], // 赤菱形
                    0.0, 2.5, 7);
                right->param_d[0] = 2.5;

                // 針自身は消滅
                pShot->y = 9999.0;
            }
            break;
        }
        case 7: // 横枝弾（Phase 3の間は水平移動、Phase 4では吸い込みへ）
            if (phase < 540) {
                pShot->x += pShot->param_d[0];
            }
            break;
        }

        pShot = pShot->next;
    }
}

//------------------------------------------------------------
// 敵本体パターン（エントリポイント）
//------------------------------------------------------------
void EnemyPat_Antlion_DeepSeek()
{
    if (count == 1) {
        // アリジゴクは画面下中央
        enemy.x = 240.0;
        enemy.y = 440.0;
        enemy.maxHp = enemy.hp = 60 * 30;

        player.y = 240;
    }

    enemy.hp--;

    if (count % 540 == 1) {
        // 弾幕セットを一つ生成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = AntlionPitPattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルリストに接続
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // プレイヤーを常に中心へ引き寄せる（蟻地獄の吸い込み）
    double dx = enemy.x - player.x;
    double dy = enemy.y - player.y;
    double dist = sqrt(dx * dx + dy * dy);
    if (dist > 10.0) {
        double force = 0.125;
        player.x += force * dx / dist;
        player.y += force * dy / dist;
        spawnForceParticles(player.x, player.y, force * dx / dist * 3, force * dy / dist * 3);
    }
}