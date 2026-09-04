// enemyPat_Tmp.cpp (または既存ファイルに追記)

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：熱運動する粒子雲（ブラウン・ミスト）
// ============================================================
static void ShotBrownianMist(sEnemyShotSet* pEnemyShotSet)
{
    // 生成処理 (count == 0 のときのみ実行)
    if (pEnemyShotSet->count == 0) {
        // 微細な粒子の放出音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 1回につき少数ずつ生成し、頻繁に呼ばれることで「雲」を形成する
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 初期位置にわずかなばらつきを持たせる
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(40) - 20);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(40) - 20);

            // 初期方向：プレイヤー方向をベースにしつつ、大きくばらけさせる
            double base_muki = pEnemyShotSet->muki + (GetRand(120) - 60) / 180.0 * DX_PI;
            // 初期速度：遅め (1.0 〜 2.0) に設定し、その場に留まりながら拡散する質感を出す
            double speed = 1.0 + GetRand(100) / 100.0;

            // 速度ベクトルを計算してパラメータに保存
            pEnemyShot->param_d[0] = cos(base_muki) * speed; // vx
            pEnemyShot->param_d[1] = sin(base_muki) * speed; // vy

            // 弾の種類：小玉 (2.5x2.5)
            // 色：シアン(3) と 白(6) をランダムに混ぜることで、粒子の濃淡や奥行きを表現
            int color = (GetRand(1) == 0) ? 3 : 6;
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            // 連結リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動・更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double vx = pShot->param_d[0];
        double vy = pShot->param_d[1];

        // ブラウン運動的なランダムな加速度（揺らぎ）を加える
        // GetRand(20) は 0〜20 を返すので、-10 して -10〜10 の範囲にする
        // 0.15 を掛けることで、1フレームあたり -1.5 〜 +1.5 の速度変化を与える
        vx += (GetRand(20) - 10) * 0.15;
        vy += (GetRand(20) - 10) * 0.15;

        // 速度の正規化（暴走防止と、最低速度の保証）
        double current_speed = sqrt(vx * vx + vy * vy);
        if (current_speed > 2.5) {
            // 上限速度 2.5 に制限
            vx = (vx / current_speed) * 2.5;
            vy = (vy / current_speed) * 2.5;
        }
        else if (current_speed < 0.5 && current_speed > 0.001) {
            // 下限速度 0.5 に制限（止まりすぎて見えなくなるのを防ぐ）
            vx = (vx / current_speed) * 0.5;
            vy = (vy / current_speed) * 0.5;
        }

        // 更新した速度ベクトルを保存
        pShot->param_d[0] = vx;
        pShot->param_d[1] = vy;

        // 位置更新
        pShot->x += vx;
        pShot->y += vy;

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン：ブラウン・ミスト
// ============================================================
void EnemyPat_BrownianMotion_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 180.0;       // やや上めに配置し、弾が広がる余地を作る
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵本体はゆっくりと左右に往復移動
        enemy.x += 0.8 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 頻繁に少量ずつ生成して「雲」や「霧」の連続性を表現する
    // 20フレームに1回程度が、粒子が途切れず、かつ重くなりすぎない目安
    if (count % 20 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBrownianMist;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // プレイヤー方向をベース角度として渡す（内部でばらけさせる）
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        // 弾リストのヘッダ初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}