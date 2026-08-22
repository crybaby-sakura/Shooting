// enemyPat_Tmp.cpp
// 虹色の息継ぎ（レインボー・ブレス）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：虹色の息継ぎ（拡張→収縮→分裂拡散）
static void ShotRainbowBreath(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // count==0 のときだけリングを生成（1回の呼吸）
    if (pEnemyShotSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 中心位置を記録（収縮の目標点）
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;

        // 12発の大玉を円形に配置（隙間がトンネルになる）
        const int num = 12;
        const double baseAngle = (pEnemyShotSet->kind % 12) * (DX_PI / 6.0); // セットごとに少し回転させる

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 放射方向
            pEnemyShot->muki = baseAngle + (DX_PI * 2.0 * i) / num;
            // 初期速度（少しバラつき）
            pEnemyShot->speed = 1.6 + GetRand(8) / 10.0 + 1; // 1.6～2.4

            // 虹色（0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ）を順番に
            int color = (pEnemyShotSet->kind + i) % 6;
            pEnemyShot->kind = img_enemyShotLargeBall[color];
            pEnemyShot->margin = 240;

            // フェーズ管理用
            // param_i[0] : 0=拡張中, 1=収縮中, 2=分裂後拡散
            pEnemyShot->param_i[0] = 0;
            // 元の速度を保存
            pEnemyShot->param_d[0] = pEnemyShot->speed;
            // 色番号を保存（分裂時に使用）
            pEnemyShot->param_i[1] = color;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム全弾の挙動更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // フェーズ遷移
        if (pShot->param_i[0] == 0 && pShot->count >= 55 * 2) {
            // 拡張 → 収縮へ
            pShot->param_i[0] = 1;
            // 中心に向かう方向を計算
            double dx = pEnemyShotSet->param_d[0] - pShot->x;
            double dy = pEnemyShotSet->param_d[1] - pShot->y;
            pShot->muki = atan2(dy, dx);
            pShot->speed = pShot->param_d[0] * 0.85; // 少しゆっくり収縮
        }
        else if (pShot->param_i[0] == 1 && pShot->count >= 110 * 2) {
            // 収縮 → 分裂拡散へ
            pShot->param_i[0] = 2;

            // 自分自身を小玉に変化させて外側へ
            int color = pShot->param_i[1];
            pShot->kind = img_enemyShotSmallBall[color];
            // 外側方向（中心から離れる）
            double dx = pShot->x - pEnemyShotSet->param_d[0];
            double dy = pShot->y - pEnemyShotSet->param_d[1];
            pShot->muki = atan2(dy, dx);
            pShot->speed = 3.2 + GetRand(10) / 10.0; // 3.2～4.2

            // 追加で左右に2発の小玉を分裂させる（計3発になる）
            for (int k = -1; k <= 1; k += 2) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = pShot->x;
                pNew->y = pShot->y;
                pNew->muki = pShot->muki + k * (20.0 + GetRand(15)) / 180.0 * DX_PI;
                pNew->speed = pShot->speed * (0.9 + GetRand(3) / 10.0);
                pNew->kind = img_enemyShotSmallBall[color];
                pNew->param_i[0] = 2; // 既に拡散フェーズ
                pNew->param_i[1] = color;

                // リストに追加
                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
        }

        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_TheMostFun_Grok()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 200.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // ゆっくり左右に移動（呼吸に合わせて揺れる感じ）
        enemy.x += 0.7 * (double)muki;
        if (count % 150 == 75) muki *= -1;

        // わずかに上下にも揺れる
        enemy.y = 200.0 + 8.0 * sin(count / 40.0);
    }

    // 約1.2秒ごとに新しいリング（呼吸）を生成
    // 重なり合うことで「息を吸う・吐く」が連続して感じられる
    if (count % 72 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRainbowBreath;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = 0.0; // 使わない
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}