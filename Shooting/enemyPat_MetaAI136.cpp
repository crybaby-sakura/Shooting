#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 噴水弾幕 - Fountain Crown
// 使用素材:
//   中玉(7.0x7.0) 青:  img_enemyShotMediumBall[4] -> 水柱のコア
//   小玉(2.5x2.5) シアン/青/白: img_enemyShotSmallBall[3]/[4]/[6] -> 水滴・飛沫・波紋
//   中楕円弾(10.5x7.0) シアン: img_enemyShotMediumOval[3] -> 横に広がる波紋の代用
// 色: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
// ============================================================

static void ShotFountain(sEnemyShotSet* pEnemyShotSet)
{
    // 初回: 予告演出
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = 0; // 0:未爆発 1:爆発済み
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 0～15F: ボス足元に水が溜まる表現 (白い小玉が中心に集まる)
    if (pEnemyShotSet->count < 15) {
        if (pEnemyShotSet->count % 2 == 0) {
            for (int j = 0; j < 2; j++) {
                sEnemyShot* p = new sEnemyShot;
                double ang = (GetRand(360) / 180.0) * DX_PI;
                double r = GetRand(250) / 10.0; // 0～25px
                p->x = pEnemyShotSet->x + r * cos(ang);
                p->y = pEnemyShotSet->y + r * sin(ang);
                p->muki = 0; p->speed = 0;
                p->kind = img_enemyShotSmallBall[6]; // 白
                p->param_i[0] = 10; // 溜めエフェクト
                p->param_d[0] = 0;

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // 20F: 打ち上げ
    if (pEnemyShotSet->count == 20) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 水柱のコア (中玉・青)
        {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = -DX_PI / 2.0;
            p->speed = 0;
            p->kind = img_enemyShotMediumBall[4]; // 青中玉
            p->param_i[0] = 0; // 0:コア
            p->param_i[1] = 0; // 未爆発
            // GetRandは0～xを返すので、-50～50を作るには GetRand(100)-50
            p->param_d[1] = (double)(GetRand(100) - 50) / 100.0 * 0.4; // vx 微小ランダム
            p->param_d[2] = -6.0 - (double)GetRand(100) / 100.0;       // vy 上向き
            p->margin = 240;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 全弾の挙動更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 削除に備えて先に保存

        // --- 溜めエフェクト ---
        if (pShot->param_i[0] == 10) {
            // 中心に吸い込まれる
            pShot->x += (pEnemyShotSet->x - pShot->x) * 0.12;
            pShot->y += (pEnemyShotSet->y - pShot->y) * 0.12;
            if (pEnemyShotSet->count > 22) {
                // 役目終了
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }
        // --- コア (上昇する水柱) ---
        else if (pShot->param_i[0] == 0) {
            // 放物線運動
            pShot->x += pShot->param_d[1];
            pShot->y += pShot->param_d[2];
            pShot->param_d[2] += 0.085; // 重力

            // 軌跡に小玉の飛沫を残す
            if (pEnemyShotSet->count % 2 == 0) {
                sEnemyShot* t = new sEnemyShot;
                t->x = pShot->x + (GetRand(40) - 20) / 10.0;
                t->y = pShot->y + (GetRand(40) - 20) / 10.0;
                t->kind = img_enemyShotSmallBall[3]; // シアン小玉
                t->param_i[0] = 1; // 水滴
                t->param_d[1] = pShot->param_d[1] * 0.3 + (GetRand(60) - 30) / 100.0;
                t->param_d[2] = pShot->param_d[2] * 0.3;
                t->margin = 240;
                t->prev = pEnemyShotSet->pEnemyShotHead->prev;
                t->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = t;
                pEnemyShotSet->pEnemyShotHead->prev = t;
            }

            // 頂点付近 (vyが0近く) で爆発
            if (pShot->param_d[2] >= -0.2 && pShot->param_i[1] == 0) {
                pShot->param_i[1] = 1; // 爆発フラグ

                // 効果音: 弾ける
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                double baseAng = (GetRand(360) / 180.0) * DX_PI;
                const int NUM_DROP = 32;
                for (int i = 0; i < NUM_DROP; i++) {
                    sEnemyShot* d = new sEnemyShot;
                    double ang = baseAng + (2.0 * DX_PI * i / NUM_DROP) + (GetRand(20) - 10) / 180.0 * DX_PI * 0.5;
                    double sp = 1.6 + (double)GetRand(150) / 100.0; // 1.6～3.1
                    // 水の色を青とシアンで交互に
                    d->x = pShot->x;
                    d->y = pShot->y;
                    d->kind = (i % 2 == 0) ? img_enemyShotSmallBall[4] : img_enemyShotSmallBall[3];
                    d->param_i[0] = 1; // 水滴
                    d->param_d[1] = cos(ang) * sp * 0.9;
                    d->param_d[2] = sin(ang) * sp * 0.9 - 0.5; // 少し上向きに
                    d->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    d->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = d;
                    pEnemyShotSet->pEnemyShotHead->prev = d;
                }
                // 中心の白いしぶき
                for (int i = 0; i < 10; i++) {
                    sEnemyShot* s = new sEnemyShot;
                    double ang = (GetRand(360) / 180.0) * DX_PI;
                    double sp = 2.0 + GetRand(100) / 100.0;
                    s->x = pShot->x;
                    s->y = pShot->y;
                    s->kind = img_enemyShotSmallBall[6]; // 白
                    s->param_i[0] = 1;
                    s->param_d[1] = cos(ang) * sp;
                    s->param_d[2] = sin(ang) * sp;
                    s->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    s->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = s;
                    pEnemyShotSet->pEnemyShotHead->prev = s;
                }

                // コア自体は消去
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }
        // --- 水滴 (落下) ---
        else if (pShot->param_i[0] == 1) {
            pShot->x += pShot->param_d[1];
            pShot->y += pShot->param_d[2];
            pShot->param_d[2] += 0.085; // 重力落下
            pShot->param_d[1] *= 0.998; // 空気抵抗で少し横減速

            // 地面 (y=400～440の間でランダム) に到達で波紋化
            double groundY = 460.0 + GetRand(40);
            if (pShot->y >= groundY) {
                // 波紋を生成: 中心から8方向に小玉が広がる + 中楕円で横波を強調
                for (int k = 0; k < 8; k++) {
                    sEnemyShot* w = new sEnemyShot;
                    double ang = 2.0 * DX_PI * k / 8.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
                    w->x = pShot->x;
                    w->y = groundY;
                    // 交互に色を変えて波紋らしく
                    w->kind = (k % 2 == 0) ? img_enemyShotSmallBall[3] : img_enemyShotSmallBall[6];
                    w->param_i[0] = 2; // 波紋
                    double rsp = 0.8 + GetRand(80) / 100.0;
                    w->param_d[1] = cos(ang) * rsp;
                    w->param_d[2] = sin(ang) * rsp * 0.3; // 横に広がる
                    w->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    w->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = w;
                    pEnemyShotSet->pEnemyShotHead->prev = w;
                }
                // 横に広がる楕円の波紋1つ
                {
                    sEnemyShot* wo = new sEnemyShot;
                    wo->x = pShot->x;
                    wo->y = groundY;
                    wo->kind = img_enemyShotMediumOval[3]; // シアン中楕円
                    wo->param_i[0] = 2;
                    wo->param_d[1] = (GetRand(60) - 30) / 100.0;
                    wo->param_d[2] = 0;
                    wo->param_d[3] = 0.02; // 拡大演出用に使うならparam_dで管理も可 (今回は速度減衰のみ)
                    wo->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    wo->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = wo;
                    pEnemyShotSet->pEnemyShotHead->prev = wo;
                }

                // 水滴本体は消去
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }
        // --- 波紋 (地面で横に広がる) ---
        else if (pShot->param_i[0] == 2) {
            pShot->x += pShot->param_d[1];
            pShot->y += pShot->param_d[2];
            pShot->param_d[1] *= 0.97;
            pShot->param_d[2] *= 0.97;
            // 40F経過で自然消滅 (画面外削除はメイン側だが念のため寿命で消す)
            if (pShot->count > 45) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }

        pShot = pNext;
    }
}

void EnemyPat_Fountain_MetaAI()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 260.0;
        enemy.maxHp = enemy.hp = 200; // 仕様固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 噴水を見やすくするため、左右にゆっくり移動
        enemy.x += 0.6 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // 80Fごとに噴水を1本打ち上げ
    // GetRandの仕様に注意: 0～x までなので揺らぎを少し入れる
    if (count % 80 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFountain;
        // 少し横にランダムにずらして自然な噴水に
        pSet->x = enemy.x + (GetRand(80) - 40);
        pSet->y = enemy.y + 10.0;
        pSet->muki = -DX_PI / 2.0; // 真上
        pSet->kind = shot_count++;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // リストに連結 (サンプルと同じ手順)
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}