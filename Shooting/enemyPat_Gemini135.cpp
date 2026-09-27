#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：熱圧爆砕（バックドラフト）
static void ShotBackdraft(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int t = pEnemyShotSet->count;

    // Phase 1：酸素吸引（インテーク） [0〜90フレーム]
    if (t >= 0 && t < 90) {
        if (t % 2 == 0) {
            // 吸引中の空気の音として定期的に軽い音を鳴らす
            if (t % 30 == 0) {
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            // 画面外周付近からボスの中心に向かって青い小弾を生成
            for (int i = 0; i < 3; i++) {
                pEnemyShot = new sEnemyShot;
                double r = (GetRand(359) * DX_PI) / 180.0; // 360度ランダムな方向
                double dist = 400.0 + GetRand(100);        // 画面外〜外周の距離
                pEnemyShot->margin = 120;

                pEnemyShot->x = pEnemyShotSet->x + cos(r) * dist;
                pEnemyShot->y = pEnemyShotSet->y + sin(r) * dist;

                // ボス中心へ向かう角度
                pEnemyShot->muki = atan2(pEnemyShotSet->y - pEnemyShot->y, pEnemyShotSet->x - pEnemyShot->x);
                pEnemyShot->speed = 3.0 + GetRand(300) / 100.0; // 少しバラつきを持たせた吸引速度
                pEnemyShot->kind = img_enemyShotSmallBall[4];   // 青(4)の小弾
                pEnemyShot->param_i[0] = 0; // 【管理フラグ 0】: 吸引される弾

                // リストへ追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // Phase 2：無酸素の溜め（プレッシャー） [90フレーム目]
    if (t == 90) {
        // 予告音を鳴らして静寂を作る
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0; // 停止
        pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤(0)の大玉（爆発の核）
        pEnemyShot->param_i[0] = 1; // 【管理フラグ 1】: 溜め中の大玉

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // Phase 3：爆炎噴出（バックドラフト） [150フレーム目]
    if (t == 150) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 1. 衝撃波（自機狙いを基準にした8方向の赤レーザー）
        double base_angle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int i = 0; i < 8*4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_angle + (i * DX_PI / 4.0/4);
            pEnemyShot->speed = 9.0;                      // 衝撃波らしく超高速
            pEnemyShot->kind = img_enemyShotLaser[0];     // 赤(0)のレーザー
            pEnemyShot->param_i[0] = 2;                   // 【管理フラグ 2】: 通常の飛ぶ弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 2. 全方位の不均一な爆炎（赤・黄・橙の小弾・中玉・鱗弾・菱形弾を大量に）
        for (int i = 0; i < 120*4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (GetRand(359) * DX_PI) / 180.0;
            pEnemyShot->speed = 1.0 + GetRand(600) / 100.0; // 速度に幅を持たせて波を作る

            // 色の決定 (0:赤, 1:黄, 8:橙)
            int c_rand = GetRand(2);
            int color = (c_rand == 0) ? 0 : ((c_rand == 1) ? 1 : 8);

            // 弾の形状の決定
            int type_rand = GetRand(3);
            if (type_rand == 0)      pEnemyShot->kind = img_enemyShotSmallBall[color];
            else if (type_rand == 1) pEnemyShot->kind = img_enemyShotScale[color];
            else if (type_rand == 2) pEnemyShot->kind = img_enemyShotDiamond[color];
            else                     pEnemyShot->kind = img_enemyShotMediumBall[color];

            pEnemyShot->param_i[0] = 2; // 【管理フラグ 2】: 通常の飛ぶ弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の個別移動とフラグごとの特殊処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // 【フラグ 0】吸引弾：ボスの中心付近に到達したら消去（吸収）
        if (pShot->param_i[0] == 0) {
            double dx = pEnemyShotSet->x - pShot->x;
            double dy = pEnemyShotSet->y - pShot->y;
            if (dx * dx + dy * dy < 20.0 * 20.0) { // 中心から半径20以内
                sEnemyShot* del = pShot;
                pShot = pShot->next;
                del->prev->next = del->next;
                del->next->prev = del->prev;
                delete del;
                continue; // 削除したので次の弾へ
            }
        }

        // 【フラグ 1】溜め中の大玉：ボスに追従させ、爆発タイミングで消去
        if (pShot->param_i[0] == 1) {
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            if (t >= 150) { // 爆発の瞬間に消える
                sEnemyShot* del = pShot;
                pShot = pShot->next;
                del->prev->next = del->next;
                del->next->prev = del->prev;
                delete del;
                continue;
            }
        }

        // 基本的な移動処理 (フラグ1の大玉は speed=0 なので動かない)
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }

    // セット自体の座標をボスに追従させる（ボスが動いても弾幕発生源がズレないようにする）
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
}


// 敵本体のパターン
void EnemyPat_Backdraft_Gemini()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 100.0; // 全方位の爆発が綺麗に見えるよう、やや中央寄りに配置
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ボスはゆっくり左右に漂う
        enemy.x += 0.6 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 300フレーム（約5秒）周期でバックドラフト弾幕のセットを登録
    if (count % 300 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBackdraft;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}