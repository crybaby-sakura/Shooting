// enemyPat_Tmp.cpp
// slither.ioモチーフ弾幕：「成長する蛇の軌跡（Slither Trail）」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：成長する蛇の軌跡
// 頭（中玉）がくねくねと進み、その軌跡上に体（小玉）を一定間隔で生成して長い蛇状の弾列を作る
static void ShotSlither(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初期化（countはメインルーチンで自動インクリメントされる）
    if (pEnemyShotSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // パラメータ初期化
        // param_i[0] : 生成した体の数
        // param_i[1] : 次に体を生成するまでの残りフレーム
        // param_i[2] : 最大体長
        // param_d[0] : 頭の現在の向き（ラジアン）
        // param_d[1] : 頭の速さ
        // param_d[2] : くねりの位相
        // param_d[3] : くねりの振幅（角度）
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0;          // すぐに最初の体を出せるように
        pEnemyShotSet->param_i[2] = 28 + GetRand(12); // 最大体長 28〜40
        pEnemyShotSet->param_d[0] = pEnemyShotSet->muki; // 初期向き（プレイヤー方向）
        pEnemyShotSet->param_d[1] = 2.2 + GetRand(80) / 100.0; // 速さ 2.2〜3.0
        pEnemyShotSet->param_d[2] = 0.0;        // 位相
        pEnemyShotSet->param_d[3] = (12.0 + GetRand(10)) / 180.0 * DX_PI; // 振幅 12〜22度

        // 頭弾を生成（中玉・緑寄りで目立つように）
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->param_d[0];
        pEnemyShot->speed = pEnemyShotSet->param_d[1];
        // 色: 2=緑 を基本に、バリエーションで 3=シアン も混ぜる
        int col = (pEnemyShotSet->kind % 2 == 0) ? 2 : 3;
        pEnemyShot->kind = img_enemyShotMediumBall[col];
        pEnemyShot->param_i[0] = 1; // 1=頭フラグ
        pEnemyShot->margin = 120;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 頭の位置・向きを更新するための変数（毎フレーム計算）
    double headX = pEnemyShotSet->x;
    double headY = pEnemyShotSet->y;
    double headMuki = pEnemyShotSet->param_d[0];
    double headSpeed = pEnemyShotSet->param_d[1];

    // 既存の弾を動かす + 頭の位置を追跡
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 頭弾だけ特別にくねくね制御
        if (pShot->param_i[0] == 1) {
            // くねりの位相を進める
            pEnemyShotSet->param_d[2] += 0.09 + (pEnemyShotSet->kind % 5) * 0.01;

            // 基本向きをプレイヤー方向へ弱く寄せる（弱いホーミング）
            double toPlayer = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = toPlayer - pShot->muki;
            // 角度差を -π〜π に正規化
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;
            pShot->muki += diff * 0.025; // 弱い追従

            // くねりを加算
            double wave = sin(pEnemyShotSet->param_d[2]) * pEnemyShotSet->param_d[3];
            pShot->muki += wave * 0.15; // くねりの強さ

            // 速さを少し変化させて有機的に
            pShot->speed = headSpeed + sin(pEnemyShotSet->param_d[2] * 0.7) * 0.25;

            // 位置更新
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // セット側にも頭情報を反映（次の体生成用）
            headX = pShot->x;
            headY = pShot->y;
            headMuki = pShot->muki;
            pEnemyShotSet->param_d[0] = headMuki;
            pEnemyShotSet->x = headX; // セットの基準位置も更新
            pEnemyShotSet->y = headY;
        }
        else {
            // 体弾は生成時の向き・速さで等速直線運動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // 体の追加判定（頭が存在し、まだ最大長に達していない場合）
    if (pEnemyShotSet->param_i[0] < pEnemyShotSet->param_i[2]) {
        if (pEnemyShotSet->param_i[1] <= 0) {
            // 体弾生成（頭の少し後ろに配置）
            pEnemyShot = new sEnemyShot;
            // 頭の進行方向の逆側に少しオフセット
            double backDist = 6.0 + GetRand(3); // 6〜9ピクセル後ろ
            pEnemyShot->x = headX - cos(headMuki) * backDist;
            pEnemyShot->y = headY - sin(headMuki) * backDist;
            pEnemyShot->muki = headMuki;
            pEnemyShot->speed = headSpeed * (0.32 + GetRand(10) / 100.0); // わずかに遅めにして密度を出す
            // 体は小玉、色は頭に合わせる
            int col = (pEnemyShotSet->kind % 2 == 0) ? 2 : 3;
            // たまに色を変えてアクセント
            if (GetRand(7) == 0) col = 1; // 黄
            pEnemyShot->kind = img_enemyShotSmallBall[col];
            pEnemyShot->param_i[0] = 0; // 体フラグ
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            pEnemyShotSet->param_i[0]++; // 体の数を増やす
            // 次の体生成までの間隔（フレーム）。短いほど密な蛇になる
            pEnemyShotSet->param_i[1] = 3 + GetRand(2); // 3〜5フレーム
        }
        else {
            pEnemyShotSet->param_i[1]--;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Slitherio_Grok()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.85 * (double)muki;
        if (enemy.x < 80.0) {
            enemy.x = 80.0;
            muki = 1;
        }
        if (enemy.x > 400.0) {
            enemy.x = 400.0;
            muki = -1;
        }
        // 一定間隔で方向転換のゆらぎ
        if (count % 150 == 75) {
            if (GetRand(1) == 0) muki *= -1;
        }
    }

    // 一定間隔で蛇を生成
    // 最初は早め、その後は間隔を空ける
    int interval = 90;
    if (count < 180) interval = 70;
    if (count > 600) interval = 110;

    if (count % interval == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSlither;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        // 初期向きはプレイヤー方向に少しランダムを加える
        double baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->muki = baseMuki + (GetRand(40) - 20) / 180.0 * DX_PI;
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}