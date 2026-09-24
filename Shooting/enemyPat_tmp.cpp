#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {
    constexpr int INPUT_N = 4;
    constexpr int HIDDEN_N = 7;
    constexpr int OUTPUT_N = 4;
    constexpr int EDGE_POINTS = 7;

    static void AddShot(sEnemyShotSet* pSet, double x, double y, int kind, double muki = 0.0, double speed = 0.0)
    {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = x;
        pShot->y = y;
        pShot->kind = kind;
        pShot->muki = muki;
        pShot->speed = speed;
        pShot->param_i[0] = 3; // 3: 固定表示用のニューロン／シナプス

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    static double NodeX(double centerX, int layer, int index, int variation)
    {
        if (layer == 0) {
            return centerX + (index - 1.5) * 62.0;
        }
        if (layer == 1) {
            static const double hiddenOffset[3][HIDDEN_N] = {
                {-126.0, -84.0, -42.0, 0.0, 42.0, 84.0, 126.0},
                {-108.0, -72.0, -36.0, 0.0, 36.0, 72.0, 108.0},
                {-120.0, -78.0, -26.0, 20.0, 62.0, 104.0, 136.0}
            };
            return centerX + hiddenOffset[variation % 3][index];
        }
        return centerX + (index - 1.5) * 62.0;
    }

    static double NodeY(int layer)
    {
        if (layer == 0) return 135.0;
        if (layer == 1) return 255.0;
        return 375.0;
    }

    static void AddConnectionDots(sEnemyShotSet* pSet, double sx, double sy, double tx, double ty)
    {
        for (int i = 1; i <= EDGE_POINTS; ++i) {
            double t = (double)i / (EDGE_POINTS + 1);
            AddShot(
                pSet,
                sx + (tx - sx) * t,
                sy + (ty - sy) * t,
                img_enemyShotSmallBall[6],
                atan2(ty - sy, tx - sx),
                0.0
            );
        }
    }

    static void AddPulse(sEnemyShotSet* pSet, int fromLayer, int fromIndex, int toLayer, int toIndex, int color)
    {
        double centerX = pSet->param_d[0];
        int variation = pSet->param_i[1];

        double sx = NodeX(centerX, fromLayer, fromIndex, variation);
        double sy = NodeY(fromLayer);
        double tx = NodeX(centerX, toLayer, toIndex, variation);
        double ty = NodeY(toLayer);

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = sx;
        pShot->y = sy;
        pShot->kind = color;
        pShot->muki = atan2(ty - sy, tx - sx);
        pShot->speed = 4.4;
        pShot->param_i[0] = 0;          // 0: input->hidden, 1: hidden->output, 2: output projectile
        pShot->param_i[1] = toIndex;    // destination index
        pShot->param_i[2] = fromIndex;  // source index
        pShot->param_d[0] = sx;
        pShot->param_d[1] = sy;
        pShot->param_d[2] = tx;
        pShot->param_d[3] = ty;
        pShot->param_d[4] = 0.0;        // transition start count

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    static void ShotNeuralNetwork(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            const double centerX = pSet->param_d[0];
            const int variation = pSet->param_i[1];

            // ニューロン本体：大玉で層を表現
            for (int i = 0; i < INPUT_N; ++i) {
                AddShot(pSet, NodeX(centerX, 0, i, variation), NodeY(0), img_enemyShotLargeBall[3]);
            }
            for (int i = 0; i < HIDDEN_N; ++i) {
                AddShot(pSet, NodeX(centerX, 1, i, variation), NodeY(1), img_enemyShotLargeBall[1]);
            }
            for (int i = 0; i < OUTPUT_N; ++i) {
                AddShot(pSet, NodeX(centerX, 2, i, variation), NodeY(2), img_enemyShotLargeBall[5]);
            }

            // シナプス：小玉を連ねて全結合を表現
            for (int i = 0; i < INPUT_N; ++i) {
                for (int j = 0; j < HIDDEN_N; ++j) {
                    AddConnectionDots(
                        pSet,
                        NodeX(centerX, 0, i, variation), NodeY(0),
                        NodeX(centerX, 1, j, variation), NodeY(1)
                    );
                }
            }
            for (int i = 0; i < HIDDEN_N; ++i) {
                for (int j = 0; j < OUTPUT_N; ++j) {
                    AddConnectionDots(
                        pSet,
                        NodeX(centerX, 1, i, variation), NodeY(1),
                        NodeX(centerX, 2, j, variation), NodeY(2)
                    );
                }
            }

            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // 一定間隔で入力層から全結合を通じて信号を流す
        if (pSet->count > 20 && pSet->count <= 330 && pSet->count % 78 == 18) {
            for (int i = 0; i < INPUT_N; ++i) {
                for (int j = 0; j < HIDDEN_N; ++j) {
                    AddPulse(pSet, 0, i, 1, j, img_enemyShotDiamond[8]);
                }
            }

            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 活性化信号を更新。到達すると次の層へ伝播し、出力層では弾幕として放出する。
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
                const double sx = pShot->param_d[0];
                const double sy = pShot->param_d[1];
                const double tx = pShot->param_d[2];
                const double ty = pShot->param_d[3];
                const double dx = tx - sx;
                const double dy = ty - sy;
                const double distance = sqrt(dx * dx + dy * dy);
                const double localT = (pShot->count - pShot->param_d[4]) * pShot->speed / distance;

                if (localT >= 1.0) {
                    if (pShot->param_i[0] == 0) {
                        // hidden層へ到達したら、対応する出力ニューロンへ1本だけ伝播
                        const int hiddenIndex = pShot->param_i[1];
                        const int inputIndex = pShot->param_i[2];
                        const int outputIndex = (hiddenIndex * 2 + inputIndex) % OUTPUT_N;

                        pShot->param_i[0] = 1;
                        pShot->param_i[1] = outputIndex;
                        pShot->param_i[2] = hiddenIndex;
                        pShot->param_d[0] = tx;
                        pShot->param_d[1] = ty;
                        pShot->param_d[2] = NodeX(pSet->param_d[0], 2, outputIndex, pSet->param_i[1]);
                        pShot->param_d[3] = NodeY(2);
                        pShot->param_d[4] = pShot->count;
                        pShot->muki = atan2(pShot->param_d[3] - pShot->param_d[1], pShot->param_d[2] - pShot->param_d[0]);
                    }
                    else {
                        // 出力ニューロンからプレイヤー方向へ「推論結果」を発射
                        pShot->param_i[0] = 2;
                        pShot->kind = img_enemyShotMediumOval[5];
                        pShot->x = tx;
                        pShot->y = ty;
                        pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                        pShot->speed = 2.6;
                    }
                }
                else {
                    pShot->x = sx + dx * localT;
                    pShot->y = sy + dy * localT;
                    pShot->muki = atan2(dy, dx);
                }
            }
            else if (pShot->param_i[0] == 2) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }

            pShot = pShot->next;
        }

        // ニューロンとシナプスだけをまとめて画面下へ送り、1ネットワークを終了させる。
        if (pSet->count > 420) {
            pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                if (pShot->param_i[0] != 2) {
                    pShot->y += 1.8;
                }
                pShot = pShot->next;
            }
        }
    }
}

// 敵本体のパターン
void EnemyPat_Tmp()
{
    static int muki;
    static int network_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 45.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        network_count = 0;
    }
    else {
        enemy.x += 1.05 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // ニューラルネットワークを一定周期で再構築。
    if (count % 520 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNeuralNetwork;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = network_count;
        pEnemyShotSet->param_d[0] = 240.0 + (network_count % 3 - 1) * 18.0;
        pEnemyShotSet->param_i[1] = network_count % 3;
        ++network_count;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}