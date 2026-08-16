// enemyPat_Tmp.cpp
// リサジュー曲線をモチーフにした弾幕「交差する無限軌道（Lissajous Weave）」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：リサジュー曲線上を滑りながら徐々に拡大する弾
// 複数の位相差・周波数比を重ねて交差する「織り」を作る
// ============================================================
static void ShotLissajousWeave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // セット自身の中心を敵の現在位置に追従させる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // 一定間隔で新しい弾を生成（密度調整）
    // count のインクリメントはメインルーチン側で行われる
    if (pEnemyShotSet->count % 4 == 0) {
        // 発射音（軽めの連続音）
        if (pEnemyShotSet->count % 12 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 同時に走らせる曲線の本数
        const int numCurves = 5;

        // 周波数比のバリエーション（a:b）
        // 3:2, 5:3, 4:3, 5:4, 7:5 などを混ぜると交差が豊かになる
        static const double freqA[5] = { 3.0, 5.0, 4.0, 5.0, 7.0 };
        static const double freqB[5] = { 2.0, 3.0, 3.0, 4.0, 5.0 };

        for (int c = 0; c < numCurves; c++) {
            pEnemyShot = new sEnemyShot;

            // パラメータ格納
            // param_d[0] : 初期半径係数 A0
            // param_d[1] : 初期半径係数 B0
            // param_d[2] : 周波数 a
            // param_d[3] : 周波数 b
            // param_d[4] : 位相差 δ
            // param_d[5] : t の進行速度（角速度）
            // param_d[6] : 半径の拡大速度
            pEnemyShot->param_d[0] = 40.0;                          // A0
            pEnemyShot->param_d[1] = 40.0;                          // B0
            pEnemyShot->param_d[2] = freqA[c];                      // a
            pEnemyShot->param_d[3] = freqB[c];                      // b
            pEnemyShot->param_d[4] = c * (2.0 * DX_PI / numCurves); // δ
            pEnemyShot->param_d[5] = 0.055;                         // ω
            pEnemyShot->param_d[6] = 0.55;                          // 拡大速度

            // 初期位置（t=0 の時点）
            double t = 0.0;
            double radiusA = pEnemyShot->param_d[0];
            double radiusB = pEnemyShot->param_d[1];
            pEnemyShot->x = pEnemyShotSet->x + radiusA * sin(pEnemyShot->param_d[2] * t + pEnemyShot->param_d[4]);
            pEnemyShot->y = pEnemyShotSet->y + radiusB * sin(pEnemyShot->param_d[3] * t);

            // muki / speed は線形移動には使わないが、初期化しておく
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;

            // 弾種・色
            // 小玉を使用し、曲線ごとに色を変えて視覚的に区別しやすくする
            // 色一覧: 0赤 1黄 2緑 3シアン 4青 5マゼンタ 6白 7黒 8橙
            int color = (c * 2) % 9;   // 0,2,4,6,8 を循環
            pEnemyShot->kind = img_enemyShotSmallBall[color];
            pEnemyShot->margin = 200;

            // リストへ挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾の位置更新（リサジュー曲線 + 半径拡大）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 弾自身の count はメインルーチンで毎フレーム +1 される
        double t = pShot->count * pShot->param_d[5];

        // 時間とともに半径を拡大させて画面外へ逃がす
        double radiusA = pShot->param_d[0] + pShot->param_d[6] * pShot->count;
        double radiusB = pShot->param_d[1] + pShot->param_d[6] * pShot->count;

        pShot->x = pEnemyShotSet->x + radiusA * sin(pShot->param_d[2] * t * 0.3 + pShot->param_d[4]);
        pShot->y = pEnemyShotSet->y + radiusB * sin(pShot->param_d[3] * t * 0.3);

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// 関数名は指定どおり void EnemyPat_Lissajous_Grok()
// ============================================================
void EnemyPat_Lissajous_Grok()
{
    // 敵の初期化（最初の1フレームだけ）
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 200.0;
        enemy.maxHp = enemy.hp = 200;   // 200で固定

        // リサジュー弾幕用のショットセットを1つだけ生成して継続運用
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLissajousWeave;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵の緩やかな左右移動（弾幕の中心も追従する）
        static int dir = 1;
        enemy.x += 0.6 * (double)dir;
        if (enemy.x > 320.0) dir = -1;
        if (enemy.x < 160.0) dir = 1;

        // 上下にもわずかに揺らすと曲線の重心が動いて回避が難しくなる
        enemy.y = 200.0 + 18.0 * sin(count * 0.025);
    }
}