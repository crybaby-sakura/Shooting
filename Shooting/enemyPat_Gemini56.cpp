// enemyPat_Ebbinghaus.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// エビングハウス錯視弾幕用のセット更新関数
static void ShotEbbinghaus(sEnemyShotSet* pEnemyShotSet)
{
    // 初期化（初回呼び出し時）
    if (pEnemyShotSet->count == 0) {
        int type = pEnemyShotSet->param_i[0]; // 0: 外周が大玉, 1: 外周が小玉

        // 中心となるコア弾（中玉・赤）
        // ※当たり判定は同じなのに、周囲の弾の大きさによってサイズが違って見える
        sEnemyShot* pCore = new sEnemyShot;
        pCore->kind = img_enemyShotMediumBall[0]; // 0:赤
        pCore->param_i[0] = 0; // 0はコア弾であることを意味する

        pCore->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pCore->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pCore;
        pEnemyShotSet->pEnemyShotHead->prev = pCore;

        // 周囲を回るサテライト弾（青）
        // 左(type=0)は大玉少数、右(type=1)は小玉多数で錯視を生む
        int satelliteNum = (type == 0) ? 8 : 16;
        int satKind = (type == 0) ? img_enemyShotLargeBall[4] : img_enemyShotSmallBall[4]; // 4:青
        double radius = 45.0; // 中心からの距離は同じにするのが錯視の基本条件

        for (int i = 0; i < satelliteNum; i++) {
            sEnemyShot* pSat = new sEnemyShot;
            pSat->kind = satKind;
            pSat->param_i[0] = (type == 0) ? 1 : 2; // 1:初期大玉, 2:初期小玉
            pSat->param_d[0] = radius; // 旋回半径
            pSat->param_d[1] = DX_PI * 2.0 / satelliteNum * i; // 初期角度
            pSat->param_d[2] = 0.02; // 角速度
            pSat->margin = 100;

            pSat->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pSat->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pSat;
            pEnemyShotSet->pEnemyShotHead->prev = pSat;
        }
    }

    // クラスタ全体の移動（ゆっくり下へ）
    pEnemyShotSet->y += 1.2;

    // トラップ発動：一定時間経過後（自機に迫った頃）に弾の大小が反転する
    int reverseTime = 200;
    if (pEnemyShotSet->count == reverseTime) {
        // 重複して鳴らないよう、片方のセット（左側配置）でのみ効果音を再生
        if (pEnemyShotSet->param_i[1] == 0) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
    }

    // 属する各弾の座標更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // コア弾は常にクラスタの中心
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
        }
        else {
            // トラップ発動で見た目のサイズを反転させる
            if (pEnemyShotSet->count == reverseTime) {
                if (pShot->param_i[0] == 1) {
                    pShot->kind = img_enemyShotSmallBall[4]; // 大玉 -> 小玉
                }
                else if (pShot->param_i[0] == 2) {
                    pShot->kind = img_enemyShotLargeBall[4]; // 小玉 -> 大玉
                }
            }

            // サテライト弾は中心を旋回
            pShot->param_d[1] += pShot->param_d[2]; // 角度更新
            pShot->x = pEnemyShotSet->x + pShot->param_d[0] * cos(pShot->param_d[1]);
            pShot->y = pEnemyShotSet->y + pShot->param_d[0] * sin(pShot->param_d[1]);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Ebbinghaus_Gemini()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 敵はゆったりと左右に揺れる
        enemy.x = 240.0 + 30.0 * sin(count * 0.02);
    }

    // 発射の30フレーム前にチャージ音（予告）を鳴らす
    if (count % 110 == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 定期的に錯視弾幕のペアを発射
    if (count % 110 == 31) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 左右どちらが大玉になるかを毎回ランダムに決定 (GetRand(1)は0か1を返す)
        int swapType = GetRand(1);

        // 左右2つのクラスタを発射
        for (int i = 0; i < 7; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotEbbinghaus;

            // 左側(i=0)と右側(i=1)に離して配置
            pEnemyShotSet->x = enemy.x + (i - 3) * 90.0;
            pEnemyShotSet->y = enemy.y + 20.0;

            // i=0とi=1に、swapTypeを足して2で割った余りを入れることで、0と1を割り当てる
            pEnemyShotSet->param_i[0] = (i + swapType) % 2; // 0:大玉ベース, 1:小玉ベース
            pEnemyShotSet->param_i[1] = i; // 音の重複再生防止用フラグとして自身の位置(左右)を記録

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}