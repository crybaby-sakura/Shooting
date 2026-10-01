// enemyPat_Tmp.cpp
// 弾幕:霧箱 - 電離の軌跡 -
//
// 放射線源(中玉)が漂い、そこから
//  ・短命ですぐ减速して消える「ノイズ霧滴」(小弾・白)
//  ・プレイヤーを狙う「飛跡」= 先頭の短レーザー + 後続する霧滴(小弾・白)の点列
//  ・3回に1回は磁場中の電子のように減速しながら巻いていく「螺旋飛跡」
// を放射する。

#include "DxLib.h"
#include "gv.h"

// ============================================================
// 放射線源(霧箱)の挙動
// ============================================================
// sEnemyShotSet のパラメータ使用箇所
//   param_i[1] : 飛跡の残り放射フレーム数(0 = 非アクティブ)
//   param_i[2] : 螺旋フラグ(1 = 螺旋)
//   param_i[3] : 飛跡の発生回数
//   param_d[0] : 現在の飛跡の向き
//   param_d[1] : 放射線源の漂流向き
//   param_d[2] : 現在の飛跡の速度
//   param_d[3] : 螺旋の回転方向
// sEnemyShot のパラメータ使用箇所
//   param_i[0] : 0=ノイズ霧滴 1=飛跡の先頭レーザー 2=軌跡の霧滴 3=放射線源本体
//   param_i[1] : 寿命(フレーム)
static void ShotCloudChamber(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ---- 初回:放射線源本体(見た目用の中玉)を生成 ----
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_d[1] = GetRand(360) / 180.0 * DX_PI; // 漂流の初期向き

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン:放射線源
        pEnemyShot->param_i[0] = 3;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // ---- 寿命を過ぎたら放射を止めて源を消す(弾が全部出ればセットは空になる) ----
    if (pEnemyShotSet->count >= 880) {
        if (pEnemyShotSet->count == 880) {
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                sEnemyShot* pNext = pShot->next;
                if (pShot->param_i[0] == 3) {
                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                }
                pShot = pNext;
            }
        }
        return;
    }

    // ---- 放射線源の漂流(ランダムウォーク) ----
    pEnemyShotSet->param_d[1] += (GetRand(20) - 10) / 300.0;
    pEnemyShotSet->x += 0.4 * cos(pEnemyShotSet->param_d[1]);
    pEnemyShotSet->y += 0.4 * sin(pEnemyShotSet->param_d[1]);

    // 徘徊範囲の制限(壁で向きを反射)
    if (pEnemyShotSet->x < 40.0) { pEnemyShotSet->x = 40.0;  pEnemyShotSet->param_d[1] = DX_PI - pEnemyShotSet->param_d[1]; }
    if (pEnemyShotSet->x > 440.0) { pEnemyShotSet->x = 440.0; pEnemyShotSet->param_d[1] = DX_PI - pEnemyShotSet->param_d[1]; }
    if (pEnemyShotSet->y < 50.0) { pEnemyShotSet->y = 50.0;  pEnemyShotSet->param_d[1] = -pEnemyShotSet->param_d[1]; }
    if (pEnemyShotSet->y > 230.0) { pEnemyShotSet->y = 230.0; pEnemyShotSet->param_d[1] = -pEnemyShotSet->param_d[1]; }

    // ---- 飛跡の発射(先頭:短レーザー) ----
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 50 == 0 && pEnemyShotSet->param_i[1] == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[3]++;                       // 発生回数
        bool spiral = (pEnemyShotSet->param_i[3] % 3 == 0); // 3回に1回は螺旋

        // プレイヤー狙い + ぶれ
        double baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        baseMuki += (GetRand(80) - 40) / 180.0 * DX_PI;

        pEnemyShotSet->param_i[1] = spiral ? 80 : 30; // 飛跡を放射するフレーム数
        pEnemyShotSet->param_i[2] = spiral ? 1 : 0;
        pEnemyShotSet->param_d[0] = baseMuki;         // 飛跡の現在向き
        pEnemyShotSet->param_d[2] = 3.0;              // 飛跡の現在速度
        pEnemyShotSet->param_d[3] = (GetRand(1) == 0) ? 1.0 : -1.0; // 螺旋の回転方向

        // 先頭の短レーザー(β線の軌跡)
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = baseMuki;
        pEnemyShot->speed = 3.0;
        pEnemyShot->kind = img_enemyShotLaser[3]; // シアン
        pEnemyShot->param_i[0] = 1;
        pEnemyShot->param_i[1] = 90; // 寿命

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // ---- 飛跡の放射中:霧滴(小弾)を1個ずつ連射して「点の列 = 粒子の飛跡」を作る ----
    if (pEnemyShotSet->param_i[1] > 0) {
        pEnemyShotSet->param_i[1]--;

        // 螺旋:速度が落ちるほど曲がりが急になる(磁場中の電子の軌跡)
        if (pEnemyShotSet->param_i[2] == 1) {
            double sp = pEnemyShotSet->param_d[2];
            if (sp < 0.9) sp = 0.9;
            pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[3] * 0.075 * (3.0 / sp);
            pEnemyShotSet->param_d[2] *= 0.988;
        }

        // 霧滴(小弾・白)を追加
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->param_d[0];
        pEnemyShot->speed = pEnemyShotSet->param_d[2];
        pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白:霧滴
        pEnemyShot->param_i[0] = 2;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 先頭レーザーに向き・速度を同期( spiral のとき一緒に曲がる )
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {
                pShot->muki = pEnemyShotSet->param_d[0];
                pShot->speed = pEnemyShotSet->param_d[2];
            }
            pShot = pShot->next;
        }
    }

    // ---- ノイズ:ランダム方向に飛び出し、すぐ减速して消える霧の粒子 ----
    if (pEnemyShotSet->count > 30 && GetRand(100) < 30) {
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
        pEnemyShot->speed = 1.0 + GetRand(150) / 100.0;
        pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
        pEnemyShot->param_i[0] = 0;
        pEnemyShot->param_i[1] = 30 + GetRand(30); // 寿命

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // ---- 各弾の更新と寿命弾の消去 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        bool remove = false;

        switch (pShot->param_i[0]) {
        case 0: // ノイズ霧滴:减速しながら飛び、寿命で消える
            pShot->speed *= 0.95;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->count >= pShot->param_i[1]) remove = true;
            break;
        case 1: // 飛跡の先頭レーザー
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->count >= pShot->param_i[1]) remove = true;
            break;
        case 2: // 軌跡の霧滴(直進。点列が曲線を描く)
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        case 3: // 放射線源本体:源の位置に追従
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            break;
        }

        if (remove) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        pShot = pNext;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_CloudChamber_Zai()
{
    static int sourceCount;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        sourceCount = 0;
    }
    else {
        // ゆっくり上下に揺れながら左右を往復する
        enemy.x = 240.0 + 110.0 * sin((count - 1) * 0.006);
        enemy.y = 40.0 + 15.0 * sin((count - 1) * 0.02);
    }

    // 一定間隔で放射線源(霧箱)をフィールドに設置
    if (count % 240 == 30 && sourceCount < 5) {
        sourceCount++;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCloudChamber;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = sourceCount;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}