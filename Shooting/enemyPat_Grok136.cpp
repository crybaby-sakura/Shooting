#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：噴水 cascade
// 上方向に扇状に吹き上げた後、疑似重力で弧を描いて降り注ぐ
static void ShotFountain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 一定間隔で噴出（噴水の連続感）
    // count はメインルーチンで毎フレーム +1 される
    if (pEnemyShotSet->count % 8 == 0) {
        // 効果音（中程度）
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 扇状に複数発（本流＋左右に広がる水しぶき）
        const int bulletNum = 11;
        for (int i = 0; i < bulletNum; i++) {
            pEnemyShot = new sEnemyShot;

            // 発射位置（敵付近に少し揺らぎ）
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(20) - 10);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(10) - 5);

            // 上方向を中心に扇状（-55度〜+55度程度）
            // 画面座標系は y 下向きなので、真上は -PI/2
            double baseMuki = -DX_PI / 2.0;
            double spread = (i - (bulletNum - 1) / 2.0) * (110.0 / (bulletNum - 1)) / 180.0 * DX_PI;
            double angle = baseMuki + spread + (GetRand(10) - 5) / 180.0 * DX_PI; // わずかなブレ

            // 初期速度（上向き成分を強めに）
            double initSpeed = 4.2 + GetRand(120) / 100.0; // 4.2〜5.4
            pEnemyShot->param_d[0] = initSpeed * cos(angle); // vx
            pEnemyShot->param_d[1] = initSpeed * sin(angle); // vy

            // 見た目：水らしくシアン・青・白を中心に、小玉と中玉を混ぜる
            // 色: 0赤 1黄 2緑 3シアン 4青 5マゼンタ 6白 7黒 8橙
            int color;
            if (i == bulletNum / 2) {
                // 中央の本流は少し大きめ（中玉）＋白寄り
                color = 6; // 白
                pEnemyShot->kind = img_enemyShotMediumBall[color];
            }
            else if (abs(i - bulletNum / 2) <= 2) {
                // 本流近くはシアン中玉
                color = 3 + GetRand(1); // 3 or 4
                pEnemyShot->kind = img_enemyShotMediumBall[color];
            }
            else {
                // 外側は細かい小玉（シアン〜青〜白）
                int r = GetRand(2);
                if (r == 0) color = 3;
                else if (r == 1) color = 4;
                else color = 6;
                pEnemyShot->kind = img_enemyShotSmallBall[color];
            }

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾の移動（疑似重力付き）
    const double gravity = 0.085; // y下向き正なので + で落下
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 重力適用
        pShot->param_d[1] += gravity;

        // 位置更新
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];

        pShot = pShot->next;
    }
}

// 敵本体のパターン（噴水 cascade）
void EnemyPat_Fountain_Grok()
{
    static int muki;
    static int shotPhase;

    if (count == 1) {
        // ゲーム画面は 480x480
        // 噴水なので敵をやや下寄りに配置（上に吹き上げてから落とす余裕を確保）
        enemy.x = 240.0;
        enemy.y = 160.0;
        enemy.maxHp = enemy.hp = 200; // 変更禁止
        muki = 1;
        shotPhase = 0;
    }
    else {
        // ゆっくり左右移動
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 80.0 || enemy.x > 400.0) {
            muki *= -1;
        }
        // 念のため範囲内にクランプ
        if (enemy.x < 60.0) enemy.x = 60.0;
        if (enemy.x > 420.0) enemy.x = 420.0;
    }

    // 一定間隔で噴水セットを生成（連続感を出すため短め）
    if (count % 480 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFountain;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
        pEnemyShotSet->muki = -DX_PI / 2.0; // 基本は上向き
        pEnemyShotSet->kind = shotPhase++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}