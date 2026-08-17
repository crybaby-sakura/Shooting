// enemyPat_Tmp.cpp
// 処理落ちモチーフ弾幕「フレームスキップ・バラージュ」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾追加ヘルパー
// afterimage = true なら残像（当たり判定なし想定）
// 残像は param_i[0] = 1 で識別する
// ------------------------------------------------------------
static void AddEnemyShot(sEnemyShotSet* pSet,
    double x, double y,
    double muki, double speed,
    int kind,
    bool afterimage)
{
    sEnemyShot* p = new sEnemyShot;

    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;

    if (afterimage) {
        p->param_i[0] = 1;   // 残像フラグ
        p->param_i[1] = 18;  // 寿命（約0.3秒）
    }
    else {
        p->param_i[0] = 0;
        p->param_i[1] = 0;
    }

    // リスト末尾（pEnemyShotHead の直前）へ挿入
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// ------------------------------------------------------------
// 32方向渦巻き弾
// ------------------------------------------------------------
static void SpawnSpiral(sEnemyShotSet* pSet)
{
    if (CheckSoundMem(sound_enemyShot_medium))
        StopSoundMem(sound_enemyShot_medium);
    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

    double baseAngle = atan2(player.y - pSet->y, player.x - pSet->x);

    for (int i = 0; i < 32 * 2; i++) {
        double angle = baseAngle + (2.0 * DX_PI * i) / 32.0 / 2;

        // 4フレームに1回動くため、1回の移動量を少し大きめに
        double speed = 6.0 + GetRand(20) / 10.0;  // 6.0 〜 8.0

        // 実体は青い小玉
        AddEnemyShot(pSet,
            pSet->x, pSet->y,
            angle, speed,
            img_enemyShotSmallBall[4],
            false);
    }
}

// ------------------------------------------------------------
// 処理落ち明けのポップイン弾（24方向）
// ------------------------------------------------------------
static void SpawnBurst(sEnemyShotSet* pSet, int n)
{
    if (CheckSoundMem(sound_enemyShot_heavy))
        StopSoundMem(sound_enemyShot_heavy);
    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

    double baseAngle = atan2(player.y - pSet->y, player.x - pSet->x);

    for (int i = 0; i < n; i++) {
        double angle = baseAngle + (2.0 * DX_PI * i) / n;
        double speed = 7.0 + GetRand(20) / 10.0; // 7.0 〜 9.0

        AddEnemyShot(pSet,
            pSet->x, pSet->y,
            angle, speed,
            img_enemyShotSmallBall[0],
            false);
    }
}

// ------------------------------------------------------------
// メインパターン関数
// ------------------------------------------------------------
static void FrameSkipBarrage(sEnemyShotSet* pSet)
{
    // 敵本体の現在位置を弾源として使う
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    // --- 処理落ち中 ------------------------------------------------
    if (pSet->param_i[0] > 0) {
        pSet->param_i[0]--;

        // 処理落ち終了の瞬間
        if (pSet->param_i[0] == 0) {
            // 既存の実弾を通常の3倍距離ワープさせる
            sEnemyShot* p = pSet->pEnemyShotHead->next;
            while (p != pSet->pEnemyShotHead) {
                if (p->param_i[0] == 0) { // 残像ではない実弾のみ
                    p->x += p->speed * 3.0 * cos(p->muki);
                    p->y += p->speed * 3.0 * sin(p->muki);
                }
                p = p->next;
            }

            // ポップイン弾を出現させる
            SpawnBurst(pSet, 24 * 2);
        }

        // 処理落ち中は通常移動・発射を行わない
        return;
    }

    // --- 一定周期で処理落ち開始 ------------------------------------
    if (pSet->count % 300 == 240) {
        pSet->param_i[0] = 24; // 0.4秒（24フレーム）

        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        return;
    }

    // --- 通常時の渦巻き弾発射 --------------------------------------
    if (pSet->count % 52 == 1) {
        SpawnSpiral(pSet);
    }

    // --- 弾移動・残像生成 ------------------------------------------
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* next = p->next; // 残像挿入に備えて次を保存

        if (p->param_i[0] == 1) {
            // 残像：寿命を減らし、尽きたら画面外へ飛ばしてメイン処理に消してもらう
            p->param_i[1]--;
            if (p->param_i[1] <= 0) {
                p->x = 10000.0;
                p->y = 10000.0;
                p->speed = 0.0;
            }
        }
        else {
            // 実弾：4フレームに1回だけ座標更新（低FPS再現）
            if (p->count % 4 == 0) {
                double oldX = p->x;
                double oldY = p->y;

                p->x += p->speed * cos(p->muki);
                p->y += p->speed * sin(p->muki);

                // 移動前の位置へ白い残像を置く
                AddEnemyShot(pSet,
                    oldX, oldY,
                    p->muki, 0.0,
                    img_enemyShotSmallBall[6],
                    true);
            }
        }

        p = next;
    }
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_Lag_DeepSeek()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // この敵パターン専用のショットセットを1つだけ作成
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = FrameSkipBarrage;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 0;

        // パラメータ初期化
        pSet->param_i[0] = 0;  // 処理落ちタイマー
        pSet->param_i[1] = 0;  // 予備
        pSet->param_i[2] = 0;  // 予備

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // ショットセットをグローバルリストへ登録
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        // 敵本体の左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60)
            muki *= -1;
    }
}