// enemyPat_Tmp.cpp
// 極彩螺旋華（Gokusaiseisenka）用パターン
// サムネ映えする派手で美しい螺旋弾幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ========================
// 極彩螺旋華 本体パターン関数
// ========================
static void ShotSpiralRainbow(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 出現直後（count == 0）に大量の弾を生成
    if (pEnemyShotSet->count == 0)
    {
        // 効果音（派手め）
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int spiralCount = 6;      // 6方向の螺旋
        const int bulletsPerSpiral = 48; // 1螺旋あたりの弾数

        for (int s = 0; s < spiralCount; s++)
        {
            double baseAngle = pEnemyShotSet->muki + (double)s / spiralCount * DX_PI * 2.0;

            for (int i = 0; i < bulletsPerSpiral; i++)
            {
                pEnemyShot = new sEnemyShot;

                // 螺旋の開始位置（中心から少しずらす）
                double offset = i * 4.0;
                pEnemyShot->x = pEnemyShotSet->x + cos(baseAngle) * offset;
                pEnemyShot->y = pEnemyShotSet->y + sin(baseAngle) * offset;

                // 進行方向（徐々に角度をずらして螺旋感を出す）
                pEnemyShot->muki = baseAngle + (i * 0.08);

                // 速度（内側は遅め、外側は速め）
                pEnemyShot->speed = 1.8 + i * 0.035;

                // 色と種類のバリエーション（虹色寄りに）
                int colorType = (i % 6); // 0~5で鮮やかな色を回す
                switch (colorType)
                {
                case 0: pEnemyShot->kind = img_enemyShotScale[0]; break; // 赤
                case 1: pEnemyShot->kind = img_enemyShotScale[4]; break; // 青
                case 2: pEnemyShot->kind = img_enemyShotScale[2]; break; // 緑
                case 3: pEnemyShot->kind = img_enemyShotScale[5]; break; // マゼンタ
                case 4: pEnemyShot->kind = img_enemyShotScale[1]; break; // 黄
                default: pEnemyShot->kind = img_enemyShotScale[3]; break; // シアン
                }

                // 少しのランダム性で自然に
                pEnemyShot->muki += (GetRand(20) - 10) / 180.0 * DX_PI;

                // 双方向リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 毎フレームの移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead)
    {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 軽いカーブを付けてより美しい螺旋に（徐々に角度を変化）
        if (pShot->count > 20)
        {
            pShot->muki += 0.008; // ゆるやかな外側への広がり
        }

        pShot = pShot->next;
    }
}

// ========================
// 敵本体パターン（指定された関数名）
// ========================
void EnemyPat_ThumbnailFriendly_Grok()
{
    static int muki = 1;
    static int shot_count = 0;

    if (count == 1)
    {
        // 初期配置（ゲーム画面 480x480）
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;

        muki = 1;
        shot_count = 0;
    }
    else
    {
        // 左右緩やかに移動
        enemy.x += 1.2 * (double)muki;
        if (count % 140 == 70) muki *= -1;

        // 軽く上下移動で動きを追加
        enemy.y = 60.0 + sin(count / 40.0) * 15.0;
    }

    // 定期的に螺旋弾幕を発射（少し間隔を空けてサムネ映え重視）
    if (count % 60 == 3)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpiralRainbow;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // プレイヤー方向をベースに少しランダム性を加える
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x)
            + (GetRand(40) - 20) / 180.0 * DX_PI * 0.6;

        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 双方向リストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}