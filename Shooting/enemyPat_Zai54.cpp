// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：溢れる狂乱の飛沫（ビールかけ）
static void ShotBeerSplash(sEnemyShotSet* pEnemyShotSet)
{
    // 予告動作
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 第1波：メインの飛沫（液体）
    if (pEnemyShotSet->count == 30) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        for (int i = 0; i < 30; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // プレイヤー方向を中心に ±20度のランダムな角度
            // GetRand(40) は 0〜40 を返すので、-20〜+20 のブレになる
            pEnemyShot->muki = baseMuki + (GetRand(40) - 20) / 180.0 * DX_PI;

            // 速度にランダム性を持たせる (4.5 〜 6.0)
            double spd = 4.5 + GetRand(15) / 10.0;

            // 重力を実装するため、X方向とY方向の速度成分を分けて保存する
            pEnemyShot->param_d[0] = 0.04 + GetRand(20) / 1000.0; // 重力加速度 (0.04 〜 0.06)
            pEnemyShot->param_d[1] = spd * cos(pEnemyShot->muki); // X方向速度
            pEnemyShot->param_d[2] = spd * sin(pEnemyShot->muki); // Y方向速度
            pEnemyShot->speed = 0.0; // 極座標移動を無効化するため0にする

            // ビールの液体：橙色(8) の小玉
            pEnemyShot->kind = img_enemyShotSmallBall[8];

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 第2波：拡散する泡の波
    // 32, 34, 36 フレーム目に発射
    if (pEnemyShotSet->count >= 32 && pEnemyShotSet->count <= 100 && pEnemyShotSet->count % 4 == 0) {
        if (pEnemyShotSet->count == 32) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        int numBubbles = 20;
        double speed = 1.5;

        for (int i = 0; i < numBubbles; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 全方位に均等に配置（少しずつ回転させることで渦巻き状にする）
            double offsetMuki = (pEnemyShotSet->count - 32) * 0.05;
            pEnemyShot->muki = (2.0 * DX_PI / numBubbles) * i + offsetMuki;
            pEnemyShot->speed = speed;

            // 重力なしを表現するため0にする
            pEnemyShot->param_d[0] = 0.0;

            // ビールの泡：白色(6) の中楕円弾
            pEnemyShot->kind = img_enemyShotMediumOval[6];

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_d[0] != 0.0) {
            // 重力あり（飛沫）：放物線を描く物理演算
            pShot->param_d[2] += pShot->param_d[0]; // Y方向速度に重力を加算
            pShot->x += pShot->param_d[1];          // X方向へ等速直線運動
            pShot->y += pShot->param_d[2];          // Y方向へ加速運動
        }
        else {
            // 重力なし（泡など）：通常の極座標移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_BeerSpray_Zai()
{
    static int state;
    static double targetX;
    static int shotStartCount;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = -20.0;
        enemy.maxHp = enemy.hp = 200;
        state = 0;
        targetX = 240.0;
        shotStartCount = -999;
    }
    else {
        if (state == 0) {
            // 登場：画面上部まで降りてくる
            enemy.y += 2.0;
            if (enemy.y >= 80.0) {
                enemy.y = 80.0;
                state = 1;
            }
        }
        else if (state == 1) {
            // 移動：次のビールかけ位置へ滑らかに移動
            double dx = targetX - enemy.x;
            if (fabs(dx) < 2.0) {
                enemy.x = targetX;
                state = 2; // 構え状態へ
            }
            else {
                enemy.x += dx * 0.08;
            }
        }
        else if (state == 2) {
            // 構え：ジョッキを振り上げて微かに揺れる
            enemy.x += sin(count * 0.3) * 0.3;
        }
        else if (state == 3) {
            // 叩きつけモーション中：少し震える
            enemy.x += sin(count * 1.5) * 0.5;
            // 弾幕が完了するまでの間その場に留まる
            if (count - shotStartCount > 45) {
                targetX = 60.0 + GetRand(360); // 次の位置をランダムに決定 (60 〜 420)
                state = 1;                     // 移動状態へ戻る
            }
        }
    }

    // ビールかけの発動（構え状態以降で、かつ180フレームに1回）
    if (state >= 2 && count % 150 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBeerSplash;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        shotStartCount = count;
        state = 3; // 叩きつけモーションへ移行
    }
}