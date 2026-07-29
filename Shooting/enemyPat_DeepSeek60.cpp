// enemyPat_Tmp.cpp
// 「散華の呪詛針」パターン
// ボスが被弾した瞬間に低速散布→停滞→高速自機狙いの三段階で弾を放つ

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//---------------------------------------------------
// 弾幕パターン: 散華の呪詛針（三段階：低速散布 → 停滞 → 高速自機狙い）
//---------------------------------------------------
static void ReturnFireNeedle(sEnemyShotSet* pEnemyShotSet)
{
    // 初回のみ弾を生成
    if (pEnemyShotSet->count == 0) {
        if (enemy.hp % 2 == 1) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        constexpr double SPREAD_FRAMES = 90.0;   // 低速散布の継続フレーム数
        constexpr double STOP_FRAMES = 72.0;   // 停滞時間（1.2秒）
        constexpr double SLOW_SPEED = 240.0 / SPREAD_FRAMES;    // 低速散布時の速度
        constexpr double FAST_SPEED = 6.0;    // 高速加速時の速度
        constexpr double OFFSET_RAD = 0.05;   // 束の角度差(約2.86度)
        const int BASE_COUNT = 8;                // 8方向
        const int BULLETS_PER_DIR = 3;           // 各方向に3発

        for (int i = 0; i < BASE_COUNT; ++i) {
            double baseAngle = (DX_PI * 2.0 / BASE_COUNT) * i;
            double offsets[BULLETS_PER_DIR] = { -OFFSET_RAD, 0.0, OFFSET_RAD };
            for (int j = 0; j < BULLETS_PER_DIR; ++j) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = baseAngle + offsets[j];
                pShot->speed = SLOW_SPEED;
                // 銃弾タイプ・くすんだ紫(マゼンタ)を使用
                pShot->kind = img_enemyShotBullet[5];
                // フェーズ管理: param_i[0]=フェーズ, param_i[1]=残りフレーム
                pShot->param_i[0] = 0;               // 0:低速散布
                pShot->param_i[1] = (int)SPREAD_FRAMES;
                pShot->margin = 200;

                // リンクリストに追加
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 毎フレームの弾更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case 0: // 低速散布フェーズ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (--pShot->param_i[1] <= 0) {
                // 停滞フェーズへ移行
                pShot->param_i[0] = 1;
                pShot->param_i[1] = 72;  // 1.2秒(60fps想定)
                pShot->speed = 0.0;
            }
            break;

        case 1: // 停滞フェーズ
            if (--pShot->param_i[1] <= 0) {
                // 高速自機狙いへ移行
                pShot->param_i[0] = 2;
                pShot->speed = 6.0;
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            }
            break;

        case 2: // 高速加速フェーズ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }
        pShot = pShot->next;
    }
}

//---------------------------------------------------
// 敵本体パターン（ボス）
//---------------------------------------------------
void EnemyPat_Counter_DeepSeek()
{
    static int prevHp;
    static int muki;

    // 初回初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;
        prevHp = enemy.hp;
        muki = 1;
    }
    else {
        // 横移動（参考用）
        enemy.x += 0.70 * (double)muki;
        if (count % 160 == 80) muki *= -1;

        // 被弾検出（HPが減っていたら差分だけ撃ち返しを発生）
        int damage = prevHp - enemy.hp;
        for (int d = 0; d < damage; ++d) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ReturnFireNeedle;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;   // 未使用
            pSet->kind = 0;
            // 弾リストの初期化
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            // 全体リストに追加
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        prevHp = enemy.hp;
    }
}