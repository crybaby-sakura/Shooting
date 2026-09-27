#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  禁忌「カゴメカゴメ」 実装
//  緑の小玉で網目（籠目）の囲いを作り、黄の大玉でラインを崩して拡散させる
//  素材選択:
//    - 緑小玉: img_enemyShotSmallBall[2]  (判定小さめの囲い弾)
//    - 黄大玉: img_enemyShotLargeBall[1]  (破壊・拡散用)
//    - 効果音: light(緑設置), heavy(大玉), charge(予告)
// ============================================================

// 緑の籠目弾（設置→一定時間後に拡散）
// param_i[0]: 拡散開始フラグ用（セット側で管理）
// param_d[0]: 初期x, param_d[1]: 初期y（拡散中心計算用）
static void ShotKagomeGreen(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // count==0 で網目状に緑小玉を配置（静止）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 籠目（格子）を作る。横方向・縦方向のラインで囲いを形成
        // 画面中央寄りに 横7本 × 縦6本 程度の格子
        const int gridX = 7;
        const int gridY = 6;
        const double startX = 60.0;
        const double startY = 80.0;
        const double stepX = 60.0;
        const double stepY = 55.0;

        for (int gy = 0; gy < gridY; gy++) {
            for (int gx = 0; gx < gridX; gx++) {
                // 完全な格子ではなく、少しジグザグさせて籠目感を出す
                double offsetX = (gy % 2 == 1) ? (stepX * 0.5) : 0.0;
                double bx = startX + gx * stepX + offsetX;
                double by = startY + gy * stepY;

                // 画面内に収まるものだけ生成
                if (bx < 20.0 || bx > 460.0 || by < 30.0 || by > 420.0) continue;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = bx;
                pEnemyShot->y = by;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;          // 最初は静止
                pEnemyShot->kind = img_enemyShotSmallBall[2]; // 緑小玉
                pEnemyShot->param_d[0] = bx;      // 初期位置保存
                pEnemyShot->param_d[1] = by;
                pEnemyShot->param_i[0] = 0;        // 拡散済みフラグ

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 一定時間後、または大玉が近づいたタイミングで拡散開始
    // ここではセットのcountで管理（大玉発射に合わせて崩す）
    const int scatterStart = 90;  // 大玉が飛んでくる頃に崩す

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->count >= scatterStart && pShot->param_i[0] == 0) {
            // 拡散開始：中心から外側へ、またはランダム方向へ弾を飛ばす
            double cx = 240.0;
            double cy = 200.0;
            double dx = pShot->param_d[0] - cx;
            double dy = pShot->param_d[1] - cy;
            double len = sqrt(dx * dx + dy * dy);
            if (len < 1.0) len = 1.0;

            // 外側方向＋少しランダムを混ぜて自然に崩す
            double baseMuki = atan2(dy, dx);
            double randOffset = (GetRand(40) - 20) / 180.0 * DX_PI;
            pShot->muki = baseMuki + randOffset;
            pShot->speed = 1.2 + GetRand(120) / 100.0; // 1.2〜2.4
            pShot->param_i[0] = 1;
        }

        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 黄の大玉（破壊弾）
// 6発で1ループ。1,2,3,5発目はランダム寄り、4,6発目は自機狙い
static void ShotKagomeYellow(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 大玉を1発だけ生成（呼び出し側でタイミングを制御）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->kind = img_enemyShotLargeBall[1]; // 黄大玉

        // kind を発射番号として利用（0〜5）
        int shotIndex = pEnemyShotSet->kind % 6;

        if (shotIndex == 3 || shotIndex == 5) {
            // 4発目・6発目（0-indexで3,5）は自機狙い
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
        }
        else {
            // それ以外はランダム寄り（自機方向を中心に±幅を持たせる）
            double base = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            // GetRand(90) で -45〜+45度程度の振れ
            double spread = (GetRand(90) - 45) / 180.0 * DX_PI;
            pEnemyShot->muki = base + spread;
        }

        // 大玉は比較的速め
        pEnemyShot->speed = 3.0 + GetRand(80) / 100.0; // 3.0〜3.8

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_KagomeKagome_Grok()
{
    static int phase;          // 0: 準備, 1: 緑設置中, 2: 大玉連射中
    static int yellowIndex;    // 大玉の発射番号 0〜5
    static int cycleCount;     // サイクル管理用

    if (count == 1) {
        // 初期化（HPは200固定）
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        yellowIndex = 0;
        cycleCount = 0;
    }
    else {
        // 敵はゆっくり左右に揺れる（過度に動かない）
        enemy.x = 240.0 + 40.0 * sin(count * 0.02);
        // yはほぼ固定（少し上下）
        enemy.y = 60.0 + 8.0 * sin(count * 0.015);
    }

    // ----- 弾幕サイクル -----
    // 1サイクル roughly 240フレーム程度
    //   0〜20  : チャージ音＋待機
    //  20     : 緑の籠目設置
    //  50〜   : 大玉を6発（間隔を変えて）発射
    // その後ループ

    int local = count % 260;

    // チャージ予告
    if (local == 5) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 緑の籠目を設置
    if (local == 25) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotKagomeGreen;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 大玉6発をタイミングをずらして発射
    // 間隔: 短めのところと長めのところを混ぜて「3→4, 5→6が短い」雰囲気を出す
    // 発射タイミング例: 70, 95, 120, 135, 160, 175
    static const int yellowTiming[6] = { 70, 95, 120, 135, 160, 175 };

    for (int i = 0; i < 6; i++) {
        if (local == yellowTiming[i]) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotKagomeYellow;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 15.0;
            pSet->muki = 0.0;
            pSet->kind = i;   // 発射番号として利用
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}