// enemyPat_Tmp.cpp
// 「くるくるくるりん」モチーフ：回転する迷宮の螺旋

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：回転する迷宮の螺旋
static void ShotRotatingMaze(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;
    int phase = pEnemyShotSet->kind;
    double center_x = pEnemyShotSet->x;
    double center_y = pEnemyShotSet->y;

    // 弾の生成間隔（フレーム数）。値が小さいほど壁が密になる
    int interval = 400;

    if (t % interval == 0) {
        if (phase == 1) {
            // フェーズ1: 十字回転壁
            // 4本のアームが時計回りに回転しながら外側へ広がる
            double base_angle = t * 0.03;
            int color = 4; // 4:青

            for (int k = 0; k < 4; k++) {
                double angle = base_angle + k * (DX_PI / 2.0);
                // 中心から外側へ、間隔12で弾を配置して壁を形成
                for (double r = 20.0; r < 320.0; r += 12.0) {
                    double ex = center_x + r * cos(angle);
                    double ey = center_y + r * sin(angle);

                    // 画面端付近まで生成（marginによる自動消去に任せる）
                 //   if (ex > -40 && ex < 520 && ey > -40 && ey < 520) {
                        sEnemyShot* pShot = new sEnemyShot;
                        pShot->x = ex;
                        pShot->y = ey;
                        pShot->muki = angle;
                        pShot->speed = 0.0; // 移動はparam_dで独自管理

                        // 回転・螺旋移動用のパラメータ
                        pShot->param_d[0] = center_x;       // 中心X
                        pShot->param_d[1] = center_y;       // 中心Y
                        pShot->param_d[2] = angle;          // 現在の角度
                        pShot->param_d[3] = r;              // 現在の半径
                        pShot->param_d[4] = 0.03 / 5;           // 角速度 (rad/frame)
                        pShot->param_d[5] = 1.0;            // 半径増加速度 (pixel/frame)

                        // 短レーザーを半径方向に向けることで「棒」のように見せる
                        pShot->kind = img_enemyShotLaser[color];
                        pShot->margin = 600.0;

                        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pShot->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                        pEnemyShotSet->pEnemyShotHead->prev = pShot;
                //    }
                }
            }
        }
        else if (phase == 2) {
            // フェーズ2: 二重螺旋
            // 内側と外側で回転方向が逆。通過には高度なタイミング判断が必要

            // 内側: 6本のアーム、時計回り、シアンの中玉
            double angle_in_base = t * 0.05;
            for (int k = 0; k < 6; k++) {
                double angle = angle_in_base + k * (DX_PI / 3.0);
                for (double r = 30.0; r < 160.0; r += 12.0) {
                    double ex = center_x + r * cos(angle);
                    double ey = center_y + r * sin(angle);

                  //  if (ex > -40 && ex < 520 && ey > -40 && ey < 520) {
                        sEnemyShot* pShot = new sEnemyShot;
                        pShot->x = ex;
                        pShot->y = ey;
                        pShot->muki = angle;
                        pShot->speed = 0.0;

                        pShot->param_d[0] = center_x;
                        pShot->param_d[1] = center_y;
                        pShot->param_d[2] = angle;
                        pShot->param_d[3] = r;
                        pShot->param_d[4] = 0.05 / 5;   // 時計回り
                        pShot->param_d[5] = 1.2;    // 半径増加

                        pShot->kind = img_enemyShotMediumBall[3]; // 3:シアン
                        pShot->margin = 600.0;

                        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pShot->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                        pEnemyShotSet->pEnemyShotHead->prev = pShot;
                 //   }
                }
            }

            // 外側: 4本のアーム、反時計回り、マゼンタのレーザー
            double angle_out_base = -t * 0.04;
            for (int k = 0; k < 4; k++) {
                double angle = angle_out_base + k * (DX_PI / 2.0);
                for (double r = 160.0; r < 320.0; r += 12.0) {
                    double ex = center_x + r * cos(angle);
                    double ey = center_y + r * sin(angle);

                 //   if (ex > -40 && ex < 520 && ey > -40 && ey < 520) {
                        sEnemyShot* pShot = new sEnemyShot;
                        pShot->x = ex;
                        pShot->y = ey;
                        pShot->muki = angle;
                        pShot->speed = 0.0;

                        pShot->param_d[0] = center_x;
                        pShot->param_d[1] = center_y;
                        pShot->param_d[2] = angle;
                        pShot->param_d[3] = r;
                        pShot->param_d[4] = -0.04 / 5;  // 反時計回り
                        pShot->param_d[5] = 1.2;    // 半径増加

                        pShot->kind = img_enemyShotLaser[5]; // 5:マゼンタ
                        pShot->margin = 600.0;

                        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pShot->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                        pEnemyShotSet->pEnemyShotHead->prev = pShot;
                 //   }
                }
            }
        }
        else if (phase == 3) {
            // フェーズ3: 可変回転ミラー
            // 120フレーム(約2秒)ごとに回転方向と速度が変化する。予測が要求される。
            int cycle = t / 120;
            double dir = (cycle % 2 == 0) ? 1.0 : -1.0;
            double speed = (0.03 + (cycle % 4) * 0.015) / 5; // サイクルごとに徐々に高速化

            double base_angle = t * speed * dir;
            int color = (cycle % 2 == 0) ? 4 : 1; // 4:青 と 1:黄 で切り替え

            for (int k = 0; k < 4; k++) {
                double angle = base_angle + k * (DX_PI / 2.0);
                for (double r = 20.0; r < 320.0; r += 12.0) {
                    double ex = center_x + r * cos(angle);
                    double ey = center_y + r * sin(angle);

                 //   if (ex > -40 && ex < 520 && ey > -40 && ey < 520) {
                        sEnemyShot* pShot = new sEnemyShot;
                        pShot->x = ex;
                        pShot->y = ey;
                        pShot->muki = angle;
                        pShot->speed = 0.0;

                        pShot->param_d[0] = center_x;
                        pShot->param_d[1] = center_y;
                        pShot->param_d[2] = angle;
                        pShot->param_d[3] = r;
                        pShot->param_d[4] = speed * dir;
                        pShot->param_d[5] = 1.0;

                        pShot->kind = img_enemyShotLaser[color];
                        pShot->margin = 600.0;

                        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pShot->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                        pEnemyShotSet->pEnemyShotHead->prev = pShot;
                  //  }
                }
            }
        }
    }

    // 既存の弾の移動処理 (回転・螺旋移動)
    // メインルーチンの直進移動処理を上書きする形で、独自座標計算を行う
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_d[4] != 0.0) {
            // 角度更新
            pShot->param_d[2] += pShot->param_d[4];
            // 半径更新 (螺旋状に外側へ広げる)
            pShot->param_d[3] += pShot->param_d[5];

            // 位置再計算
            pShot->x = pShot->param_d[0] + pShot->param_d[3] * cos(pShot->param_d[2]);
            pShot->y = pShot->param_d[1] + pShot->param_d[3] * sin(pShot->param_d[2]);
            // レーザー等の向きも回転に追従させることで、常に中心から外を向く「棒」を維持
            pShot->muki = pShot->param_d[2];
        }
        else {
            // フォールバック: 通常の直進移動（param_d[4]が0の場合）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_KuruKuruKururin_Qwen()
{
    static int muki;
    static int phase;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 160.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        phase = 1;
    }
    else {
        // 敵の移動: 左右にゆっくり往復
        enemy.x += 1.5 * (double)muki;
        if (enemy.x > 400.0 || enemy.x < 80.0) {
            muki *= -1;
        }
    }

    // 600フレーム(約10秒)を1サイクルとして弾幕パターンを切り替える
    // countの直接操作はメインルーチンと衝突するため、剰余演算でサイクル管理する
    int cycle_count = count % 300;

    if (cycle_count == 60) {
        // 新しい弾幕セットの生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRotatingMaze;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0;

        // サイクル内の経過時間に応じてフェーズを決定
        //if (cycle_count < 240) {
        //    pEnemyShotSet->kind = 1; // フェーズ1: 十字回転壁
        //}
        //else if (cycle_count < 480) {
        //    pEnemyShotSet->kind = 2; // フェーズ2: 二重螺旋
        //}
        //else {
        //    pEnemyShotSet->kind = 3; // フェーズ3: 可変回転
        //}
        pEnemyShotSet->kind = phase;
        phase++;
        if (phase >= 4) phase = 1;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 弾幕展開の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}