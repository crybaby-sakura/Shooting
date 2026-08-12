// enemyPat_Tmp.cpp
// 五芒星弾幕「星辰の呪縛」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 五芒星の頂点座標を計算するヘルパー
static void GetPentagramVertex(double cx, double cy, double r, double baseAng, int idx, double* outX, double* outY)
{
    // 上向きを基準に 72度間隔。星形接続は (i -> i+2)
    double a = baseAng + idx * (2.0 * DX_PI / 5.0) - DX_PI / 2.0;
    *outX = cx + r * cos(a);
    *outY = cy + r * sin(a);
}

// 弾幕パターン本体
static void ShotPentagram(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 毎フレーム中心を敵に追従させる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    //--------------------------------------------------
    // 初期化（count == 0）
    //--------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // param_i[0] : フェーズ管理
        // 0=形成, 1=拘束(回転+往復), 2=加速, 3=解放
        pEnemyShotSet->param_i[0] = 0;

        // param_d[0] : 現在の回転角
        // param_d[1] : 半径
        // param_d[2] : 回転速度
        pEnemyShotSet->param_d[0] = 0.0;
        pEnemyShotSet->param_d[1] = 95.0;
        pEnemyShotSet->param_d[2] = 0.012;

        // 頂点弾を5つ生成（視覚的な五芒星の角）
        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[5]; // マゼンタ
            pEnemyShot->param_i[0] = i;          // 頂点番号 0-4
            pEnemyShot->param_i[1] = 0;          // 種類: 0=頂点
            pEnemyShot->param_d[0] = 0.0;
            pEnemyShot->param_d[1] = 0.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    //--------------------------------------------------
    // フェーズ遷移
    //--------------------------------------------------
    int phase = pEnemyShotSet->param_i[0];

    // 形成完了 → 辺弾生成
    if (phase == 0 && pEnemyShotSet->count == 45) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 5辺 × 7個の小弾で星の辺を構成
        for (int e = 0; e < 5; e++) {
            for (int k = 1; k <= 7; k++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン
                pEnemyShot->param_i[0] = e;               // 辺番号
                pEnemyShot->param_i[1] = 1;               // 種類: 1=辺弾
                pEnemyShot->param_d[0] = k / 8.0;         // 辺上の位置 t (0〜1)
                pEnemyShot->param_d[1] = 0.018;           // tの変化速度（往復用）

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        pEnemyShotSet->param_i[0] = 1; // 拘束フェーズへ
    }

    // 拘束中に扇状弾を定期発射
    if (phase == 1 && pEnemyShotSet->count >= 90 && (pEnemyShotSet->count % 22) == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double r = pEnemyShotSet->param_d[1];
        double ang = pEnemyShotSet->param_d[0];

        for (int i = 0; i < 5; i++) {
            double vx, vy;
            GetPentagramVertex(pEnemyShotSet->x, pEnemyShotSet->y, r, ang, i, &vx, &vy);

            // 頂点からプレイヤー方向へ3-way扇
            double baseMuki = atan2(player.y - vy, player.x - vx);
            for (int s = -1; s <= 1; s++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = vx;
                pEnemyShot->y = vy;
                pEnemyShot->muki = baseMuki + s * 0.18;
                pEnemyShot->speed = 2.4;
                pEnemyShot->kind = img_enemyShotBullet[5]; // マゼンタの銃弾
                pEnemyShot->param_i[1] = 2; // 種類: 2=扇弾（以降は通常移動のみ）

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 加速フェーズへ
    if (phase == 1 && pEnemyShotSet->count == 210) {
        pEnemyShotSet->param_i[0] = 2;
        pEnemyShotSet->param_d[2] = 0.045; // 回転高速化
    }

    // 解放フェーズ（弾ける）
    if (phase == 2 && pEnemyShotSet->count == 250) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = 3;

        // 既存の辺弾を放射状に飛ばす準備は更新ループで行う
        // 頂点からも二次的な小五芒星を生成
        double r = pEnemyShotSet->param_d[1];
        double ang = pEnemyShotSet->param_d[0];
        for (int i = 0; i < 5; i++) {
            double vx, vy;
            GetPentagramVertex(pEnemyShotSet->x, pEnemyShotSet->y, r, ang, i, &vx, &vy);

            for (int j = 0; j < 5; j++) {
                pEnemyShot = new sEnemyShot;
                double a = ang + j * (2.0 * DX_PI / 5.0) - DX_PI / 2.0;
                pEnemyShot->x = vx;
                pEnemyShot->y = vy;
                pEnemyShot->muki = a;
                pEnemyShot->speed = 3.1;
                pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤
                pEnemyShot->param_i[1] = 2; // 通常弾扱い

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    //--------------------------------------------------
    // 全弾の位置更新
    //--------------------------------------------------
    double cx = pEnemyShotSet->x;
    double cy = pEnemyShotSet->y;
    double r = pEnemyShotSet->param_d[1];
    double ang = pEnemyShotSet->param_d[0];

    // 回転角を進める（解放後も少し回す）
    if (phase < 3) {
        pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[2];
    }
    else {
        pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[2] * 0.4;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int type = pShot->param_i[1];

        if (type == 0) {
            // 頂点弾：円周上を回転
            int idx = pShot->param_i[0];
            double vx, vy;
            GetPentagramVertex(cx, cy, r, ang, idx, &vx, &vy);
            pShot->x = vx;
            pShot->y = vy;

            // 解放時は外側へ押し出す
            if (phase >= 3 && pShot->speed == 0.0) {
                pShot->muki = atan2(vy - cy, vx - cx);
                pShot->speed = 1.8;
            }
        }
        else if (type == 1) {
            // 辺弾
            if (phase < 3) {
                // 拘束中：辺上を往復
                int e = pShot->param_i[0];
                double t = pShot->param_d[0];
                double dt = pShot->param_d[1];

                double x1, y1, x2, y2;
                GetPentagramVertex(cx, cy, r, ang, e, &x1, &y1);
                GetPentagramVertex(cx, cy, r, ang, (e + 2) % 5, &x2, &y2);

                pShot->x = x1 + (x2 - x1) * t;
                pShot->y = y1 + (y2 - y1) * t;

                // 往復
                t += dt;
                if (t >= 1.0) {
                    t = 1.0;
                    pShot->param_d[1] = -fabs(dt);
                }
                else if (t <= 0.0) {
                    t = 0.0;
                    pShot->param_d[1] = fabs(dt);
                }
                pShot->param_d[0] = t;
            }
            else {
                // 解放：放射状に飛び出す（初回のみ向きを設定）
                if (pShot->speed == 0.0) {
                    pShot->muki = atan2(pShot->y - cy, pShot->x - cx);
                    pShot->speed = 2.6 + (pShot->param_d[0] * 0.8); // 辺の位置で少し差をつける
                }
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else {
            // 扇弾・二次弾など通常弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        // 頂点弾が解放後に速度を持った場合の移動
        if (type == 0 && pShot->speed > 0.0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_Pentagram_Grok()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 170.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 緩やかな左右移動
        enemy.x += 0.65 * (double)muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;
    }

    if (count % 300 == 1) {
        // 五芒星弾幕セットを1つ生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPentagram;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
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