#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ----------------------------------------------------
// 弾幕：緑小弾による籠目（格子）結界
// ----------------------------------------------------
static void ShotGrid(sEnemyShotSet* pEnemyShotSet)
{
    // 設置時（1フレーム目）に格子状に弾を配置
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // param_i[0] に保存されたタイプ(0〜2)でグリッドの傾きを変える
        int type = pEnemyShotSet->param_i[0];
        double angle = 0.0;
        if (type == 1) angle = DX_PI / 6.0;  // 右斜め (30度)
        if (type == 2) angle = -DX_PI / 6.0; // 左斜め (-30度)

        // 画面全体を覆うように余裕を持って配置
        int step = 50; // 網目の大きさ（大きめにすることで回避スペースを確保）
        for (int ix = -600; ix <= 600; ix += step) {
            for (int iy = -600; iy <= 600; iy += step) {
                // 回転行列を用いて配置座標を計算（中心を240, 240とする）
                double rx = ix * cos(angle) - iy * sin(angle) + 240.0;
                double ry = ix * sin(angle) + iy * cos(angle) + 240.0;

                // 画面内外の適度な範囲のものだけ実体化
                if (rx >= -30 && rx <= 510 && ry >= -30 && ry <= 510) {
                    sEnemyShot* pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = rx;
                    pEnemyShot->y = ry;
                    pEnemyShot->speed = 0.0; // 最初は静止
                    pEnemyShot->muki = 0.0;
                    pEnemyShot->kind = img_enemyShotSmallBall[2]; // 緑色の小弾

                    // リストへの追加
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
    }

    // 時間経過（140フレーム）で崩壊開始
    if (pEnemyShotSet->count == 140) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 下方向にバラバラと落下させる
            pShot->speed = 1.0 + GetRand(150) / 100.0; // 1.0 〜 2.5 のランダム速度
            pShot->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI; // 真下を中心に±20度のばらつき
            pShot = pShot->next;
        }
    }

    // 毎フレームの移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ----------------------------------------------------
// 弾幕：黄色大弾による破壊（6発1セットのループ）
// ----------------------------------------------------
static void ShotLargeBalls(sEnemyShotSet* pEnemyShotSet)
{
    // 240フレーム（約4秒）周期で動作
    int t = pEnemyShotSet->count % 240;

    // 1ループにつき合計6発を発射
    // 0, 40, 80: 広範囲ランダム
    // 100: 自機狙い（80から間隔が短い）
    // 140: 広範囲ランダム
    // 160: 自機狙い（140から間隔が短い）
    if (t == 0 || t == 40 || t == 80 || t == 100 || t == 140 || t == 160) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = enemy.x;
        pEnemyShot->y = enemy.y;
        pEnemyShot->speed = 3.5; // 大弾は少し速め
        pEnemyShot->kind = img_enemyShotLargeBall[1]; // 黄色の大玉

        if (t == 100 || t == 160) {
            // 自機狙い
            pEnemyShot->muki = atan2(player.y - enemy.y, player.x - enemy.x);
        }
        else {
            // ランダム広範囲（真上以外の下方向扇状）
            // -20度 〜 200度 の範囲で射出
            pEnemyShot->muki = (-20 + GetRand(220)) / 180.0 * DX_PI;
        }

        // リストへの追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 毎フレームの移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ----------------------------------------------------
// 敵本体のパターン
// ----------------------------------------------------
void EnemyPat_KagomeKagome_Gemini()
{
    static int grid_type; // 次に展開する格子の傾き種類を保持

    // 出現時（1フレーム目）の初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0; // 画面上部中央付近に陣取る
        enemy.maxHp = enemy.hp = 200;
        grid_type = 0;

        // ----------------------------------------
        // 黄色大弾マネージャーを1つ生成（常時稼働）
        // ----------------------------------------
        sEnemyShotSet* pSetLarge = new sEnemyShotSet;
        pSetLarge->count = 0;
        pSetLarge->patternFunc = ShotLargeBalls;
        pSetLarge->pEnemyShotHead = new sEnemyShot;
        pSetLarge->pEnemyShotHead->prev = pSetLarge->pEnemyShotHead;
        pSetLarge->pEnemyShotHead->next = pSetLarge->pEnemyShotHead;
        pSetLarge->prev = enemyShotSetHead.prev;
        pSetLarge->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetLarge;
        enemyShotSetHead.prev = pSetLarge;
    }

    // 敵本体はわずかに左右に揺れる（不気味な浮遊感）
    enemy.x = 240.0 + 30.0 * sin(count / 60.0);

    // ----------------------------------------
    // 240フレーム周期で新たな「緑小弾の籠目」を展開
    // ----------------------------------------
    if (count % 240 == 1) {
        sEnemyShotSet* pSetGrid = new sEnemyShotSet;
        pSetGrid->count = 0;
        pSetGrid->patternFunc = ShotGrid;

        // 生成するたびに格子の傾きを変える (0:水平垂直, 1:右斜め, 2:左斜め)
        pSetGrid->param_i[0] = grid_type % 3;
        grid_type++;

        pSetGrid->pEnemyShotHead = new sEnemyShot;
        pSetGrid->pEnemyShotHead->prev = pSetGrid->pEnemyShotHead;
        pSetGrid->pEnemyShotHead->next = pSetGrid->pEnemyShotHead;
        pSetGrid->prev = enemyShotSetHead.prev;
        pSetGrid->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetGrid;
        enemyShotSetHead.prev = pSetGrid;
    }
}