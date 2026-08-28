// enemyPat_slitherLoop.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：追尾蛇の捕食円陣（スリザー・ループ）
static void ShotSlither(sEnemyShotSet* pEnemyShotSet)
{
    int color = pEnemyShotSet->kind % 8;

    // 初期化処理
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = 0; // フェーズ (0:ウネウネ前進, 1:囲い込み, 2:ペレット化・静止, 3:収束)

        // param_d[0], param_d[1] はヘビの先頭（頭）の現在のX, Y座標
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;

        // プレイヤーの方向を初期の基準角度とする
        pEnemyShotSet->param_d[2] = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
    }

    int phase = pEnemyShotSet->param_i[0];

    // フェーズ0・1：ヘビの移動と弾の生成
    if (phase == 0 || phase == 1) {
        double head_speed = 5.0; // 頭が進むスピード
        double angle = 0.0;

        if (phase == 0) {
            // --- フェーズ0：ウネウネ移動 ---
            // 基準角度に対してサイン波を加算し、波打つ動きを作る
            angle = pEnemyShotSet->param_d[2] + sin(pEnemyShotSet->count * 0.1) * 0.8;

            pEnemyShotSet->param_d[0] += head_speed * cos(angle);
            pEnemyShotSet->param_d[1] += head_speed * sin(angle);

            // 60フレーム前進したら囲い込みフェーズへ移行
            if (pEnemyShotSet->count == 60) {
                pEnemyShotSet->param_i[0] = 1;
                pEnemyShotSet->param_i[1] = pEnemyShotSet->count; // 移行時のカウントを記録
                pEnemyShotSet->param_d[3] = angle;                // 囲い込み開始時の角度

                // プレイヤーが現在どちらの方向にいるかを計算し、巻き付く回転方向を決定
                double to_player = atan2(player.y - pEnemyShotSet->param_d[1], player.x - pEnemyShotSet->param_d[0]);
                double diff = to_player - angle;
                // 角度を -PI ～ PI に正規化
                while (diff > DX_PI) diff -= DX_PI * 2.0;
                while (diff < -DX_PI) diff += DX_PI * 2.0;

                pEnemyShotSet->param_d[4] = (diff > 0) ? 1.0 : -1.0; // 1.0なら右回り、-1.0なら左回り
            }
        }
        else if (phase == 1) {
            // --- フェーズ1：囲い込み（コイリング） ---
            // 一定の角速度で進行方向を曲げ、円を描く
            double angular_velocity = 0.05 * pEnemyShotSet->param_d[4];
            pEnemyShotSet->param_d[3] += angular_velocity;
            angle = pEnemyShotSet->param_d[3];

            pEnemyShotSet->param_d[0] += head_speed * cos(angle);
            pEnemyShotSet->param_d[1] += head_speed * sin(angle);

            // 1周（約125フレーム）したらペレット化フェーズへ
            if (pEnemyShotSet->count - pEnemyShotSet->param_i[1] >= (int)(DX_PI * 2.0 / 0.05)) {
                pEnemyShotSet->param_i[0] = 2;
                pEnemyShotSet->param_i[2] = pEnemyShotSet->count; // 移行時のカウントを記録
            }
        }

        // --- 置き弾による蛇体の形成 ---
        // 前のフレームで生成した先頭の弾（＝1フレーム前の頭）を「中玉（胴体）」に変更する
        if (pEnemyShotSet->pEnemyShotHead->prev != pEnemyShotSet->pEnemyShotHead) {
            pEnemyShotSet->pEnemyShotHead->prev->kind = img_enemyShotMediumBall[color];
        }

        // 現在の頭の座標に新しい弾を生成
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->param_d[0];
        pEnemyShot->y = pEnemyShotSet->param_d[1];
        pEnemyShot->speed = 0.0; // この段階では弾自体は動かさず、空間に固定（置き弾）
        pEnemyShot->muki = angle;
        pEnemyShot->kind = img_enemyShotLargeBall[color]; // 最先端は「大玉（頭）」として描画
        pEnemyShot->margin = 120;

        // リストの末尾（直前）に追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }
    else if (phase == 2) {
        // --- フェーズ2：ペレット化・静止 ---
        // フェーズ移行した瞬間のみ実行
        if (pEnemyShotSet->count == pEnemyShotSet->param_i[2] + 1) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                // すべての弾（頭と胴体）を「小玉（ペレット）」に変更
                pShot->kind = img_enemyShotSmallBall[color];

                // slither.ioの死亡時のように、少しだけ位置をランダムにずらして崩れた感じを出す
                // GetRand(4) は 0,1,2,3,4 を返すので、-2 して -2～+2 の範囲の揺らぎにする
                pShot->x += GetRand(4) - 2;
                pShot->y += GetRand(4) - 2;

                pShot = pShot->next;
            }
        }

        // 60フレーム（約1秒）静止して溜めを作り、収束フェーズへ
        if (pEnemyShotSet->count - pEnemyShotSet->param_i[2] >= 60) {
            pEnemyShotSet->param_i[0] = 3;

            // 動き出す合図の音
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                // プレイヤーの方向を計算
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                // 1.0 ～ 2.0 の間でランダムな速度を与え、うじゃうじゃと向かってくるようにする
                pShot->speed = (100 + GetRand(100)) / 100.0;
                pShot = pShot->next;
            }
        }
    }
    // phase == 3 (収束) の時は新たにやることはなく、各弾が設定された速度で動き続ける

    // --- 弾自体の移動処理（共通） ---
    // speedが0の間（フェーズ0～2）は座標は変わらず、フェーズ3で動き出す
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Slitherio_Gemini()
{
    static int muki;
    static int shot_count;

    // 敵の初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 画面上部を左右にうろうろする
        enemy.x += 1.5 * (double)muki;
        if (enemy.x > 380.0) muki = -1;
        if (enemy.x < 100.0) muki = 1;
    }

    // 300フレーム（5秒）周期でヘビを放つ
    if (count % 150 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSlither;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->kind = shot_count++; // 発射ごとに色を変える

        // リストのダミーヘッド初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 大本のセットリストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}