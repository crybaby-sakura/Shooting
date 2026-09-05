// enemyPat_Hilbert.cpp
// ヒルベルト曲線をモチーフにした弾幕「空間充填の螺旋」実装（半整数補間対応版）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ヒルベルト曲線 座標計算ヘルパー (整数版)
// ============================================================
static void rot(int n, int* x, int* y, int rx, int ry) {
    if (ry == 0) {
        if (rx == 1) {
            *x = n - 1 - *x;
            *y = n - 1 - *y;
        }
        int t = *x;
        *x = *y;
        *y = t;
    }
}

static void d2xy(int n, int d, int* x, int* y) {
    int rx, ry, s, t = d;
    *x = *y = 0;
    for (s = 1; s < n; s *= 2) {
        rx = 1 & (t / 2);
        ry = 1 & (t ^ rx);
        rot(s, x, y, rx, ry);
        *x += s * rx;
        *y += s * ry;
        t /= 4;
    }
}

// ============================================================
// ヒルベルト曲線 座標計算ヘルパー (半整数・連続値対応版)
// ============================================================
// d に半整数を許容し、隣接する整数点間を線形補間する
static void d2xy_double(int n, double d, double* x, double* y) {
    int int_d = (int)d;
    double frac = d - int_d;

    int x0 = 0, y0 = 0;
    d2xy(n, int_d, &x0, &y0);

    int x1 = 0, y1 = 0;
    if (int_d + 1 < n * n) {
        d2xy(n, int_d + 1, &x1, &y1);
    }
    else {
        // 終端に達した場合は同じ座標を参照
        x1 = x0;
        y1 = y0;
    }

    // 線形補間により、半整数ステップでの滑らかな座標を算出
    *x = (double)x0 + ((double)x1 - (double)x0) * frac;
    *y = (double)y0 + ((double)y1 - (double)y0) * frac;
}

// ============================================================
// 弾幕パターン：空間充填の螺旋 (ヒルベルト曲線・連続補間版)
// ============================================================
static void ShotHilbert(sEnemyShotSet* pSet)
{
    int phase = pSet->param_i[2]; // 0:展開, 1:待機(脈動), 2:収束, 3:最終突撃

    // 【第1段階：初期生成と展開】
    if (phase == 0) {
        if (pSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        // フレームあたり最大4発ずつ生成して負荷を分散
        // d は 0.0 から 255.5 まで 0.5 刻みで進む (合計 512 発)
        int spawned = 0;
        while (pSet->param_d[0] <= 255.5 && spawned < 4) {
            double d = pSet->param_d[0];
            double hx = 0.0, hy = 0.0;

            // 半整数を含む d で座標を計算し、曲線上の中間点を埋める
            d2xy_double(16, d, &hx, &hy);

            // 画面480x480に収まるように座標変換 (1セル=30px, 余白15px)
            double targetX = 15.0 + hx * 30.0;
            double targetY = 15.0 + hy * 30.0;

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = enemy.x;
            pShot->y = enemy.y;

            double dx = targetX - pShot->x;
            double dy = targetY - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);

            pShot->muki = atan2(dy, dx);
            pShot->speed = dist / 60.0; // 60フレームかけて目標地点に到達

            pShot->param_d[0] = targetX; // 目標X
            pShot->param_d[1] = targetY; // 目標Y
            pShot->param_i[0] = 0;       // 弾の状態: 0=移動中

            // 曲線の軌跡を強調するため、方向性のある「鱗弾」を採用
            pShot->kind = img_enemyShotScale[3]; // シアン

            // 連結リストに追加
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;

            // d を 0.5 進める (全ての半整数をわたる)
            pSet->param_d[0] += 0.5;
            spawned++;
        }

        // 全発射完了 (d > 255.5) したら待機フェーズへ
        if (pSet->param_d[0] > 255.5) {
            pSet->param_i[2] = 1;
            pSet->param_i[3] = 0; // 待機タイマーリセット
        }
    }
    // 【第2段階：充填完了と脈動】
    else if (phase == 1) {
        pSet->param_i[3]++;

        // 3秒(180フレーム)待機した後、収束フェーズへ
        if (pSet->param_i[3] > 180) {
            pSet->param_i[2] = 2;
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        // 弾の個別更新
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                // 目標への移動処理
                double dx = pShot->param_d[0] - pShot->x;
                double dy = pShot->param_d[1] - pShot->y;
                double dist = sqrt(dx * dx + dy * dy);

                if (dist > 1.0) {
                    pShot->x += pShot->speed * cos(pShot->muki);
                    pShot->y += pShot->speed * sin(pShot->muki);
                }
                else {
                    // 到達したら「中楕円弾」に変化して曲線の線分を強調
                    pShot->x = pShot->param_d[0];
                    pShot->y = pShot->param_d[1];
                    pShot->param_i[0] = 1;
                    pShot->kind = img_enemyShotMediumOval[4]; // 青
                }
            }
            else if (pShot->param_i[0] == 1) {
                // 脈動演出: 曲線の法線方向に近い微細な位置ブレで圧迫感を出す
                double wobble = sin(pSet->count * 0.15 + pShot->param_d[0] * 0.5) * 2.0;
                pShot->x = pShot->param_d[0] + cos(pShot->muki + DX_PI / 2.0) * wobble;
                pShot->y = pShot->param_d[1] + sin(pShot->muki + DX_PI / 2.0) * wobble;
            }
            pShot = pShot->next;
        }
    }
    // 【第3段階：収束】
    else if (phase == 2) {
        bool allReached = true;
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;

        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {
                double dx = 240.0 - pShot->x;
                double dy = 240.0 - pShot->y;
                double dist = sqrt(dx * dx + dy * dy);

                if (dist > 6.0) {
                    allReached = false;
                    pShot->muki = atan2(dy, dx);
                    pShot->speed = 6.0; // 高速で中央へ収束
                    pShot->x += pShot->speed * cos(pShot->muki);
                    pShot->y += pShot->speed * sin(pShot->muki);
                }
                else {
                    // 中央到達: 白い「短レーザー」に変化し、一点に集束したエネルギーを演出
                    pShot->x = 240.0;
                    pShot->y = 240.0;
                    pShot->param_i[0] = 2;
                    pShot->kind = img_enemyShotLaser[6]; // 白
                }
            }
            pShot = pShot->next;
        }

        // 全弾が中央に集まったら最終突撃フェーズへ
        if (allReached) {
            pSet->param_i[2] = 3;
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
    }
    // 【第4段階：最終突撃】
    else if (phase == 3) {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 2) {
                // プレイヤー方向へ狙い定め、赤い「大玉」に変化して突撃
                pShot->param_i[0] = 3;
                double dx = player.x - pShot->x;
                double dy = player.y - pShot->y;
                pShot->muki = atan2(dy, dx);

                // GetRand(10) は 0~10 の11種類を返すため、-5 して -5.0 ~ +5.0 の揺らぎを与える
                double speedVariation = (GetRand(10) - 5) * 0.2;
                pShot->speed = 8.0 + speedVariation;

                pShot->kind = img_enemyShotLargeBall[0]; // 赤
            }

            if (pShot->param_i[0] == 3) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_HilbertCurve_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        if (count < 120) {
            // 画面中央へスムーズに移動
            enemy.y += (240.0 - enemy.y) * 0.05;
        }
        else {
            // 中央で浮遊するような動き
            enemy.x = 240.0 + sin((count - 120) * 0.05) * 50.0;
            enemy.y = 240.0 + cos((count - 120) * 0.07) * 30.0;
        }
    }

    // 予告演出
    if (count % 400 == 120) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // パターン開始
    if (count % 400 == 180) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHilbert;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0;
        pEnemyShotSet->kind = 0;

        // ヒルベルト曲線専用パラメータ初期化
        pEnemyShotSet->param_d[0] = 0.0; // 現在の d (0.0 から 0.5 刻みで 255.5 まで進む)
        pEnemyShotSet->param_i[2] = 0;   // フェーズ (0:展開, 1:待機, 2:収束, 3:突撃)
        pEnemyShotSet->param_i[3] = 0;   // フェーズ内タイマー

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}