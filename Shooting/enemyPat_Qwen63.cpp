// enemyPat_Tmp.cpp
// サインポールをモチーフにした錯覚弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：錯覚のサインポール・ラビリンス
static void ShotSignPole(sEnemyShotSet* pEnemyShotSet)
{
    // count == 0 のときのみ弾を生成（メインルーチンで毎フレーム+1される仕様のため）
    if (pEnemyShotSet->count == 0) {
        // 効果音：中程度の発射音を使用
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int numShots = 12; // 各色12発ずつ（計36発）
        double baseAngle = pEnemyShotSet->muki;

        // 螺旋の巻き具合。発射回数(kind)に比例して位相をずらすことで、
        // 連続発射時に「螺旋が回転しながら広がっている」錯覚を生む
        double spiralOffset = pEnemyShotSet->kind * 0.25;

        for (int i = 0; i < numShots; i++) {
            double angleStep = (2.0 * DX_PI) / numShots;

            // 🔴 赤弾：高速・直進（小玉・赤）
            // 螺旋の外側へ勢いよく飛び出し、プレイヤーに圧力をかける
            sEnemyShot* pRed = new sEnemyShot;
            pRed->x = pEnemyShotSet->x;
            pRed->y = pEnemyShotSet->y;
            pRed->muki = baseAngle + i * angleStep + spiralOffset;
            pRed->speed = 3.5;
            pRed->kind = img_enemyShotSmallBall[0]; // 0:赤
            pRed->param_i[0] = 0; // 動作フラグ: 直進

            pRed->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pRed->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pRed;
            pEnemyShotSet->pEnemyShotHead->prev = pRed;

            // 🔵 青弾：スロー・曲線（中楕円弾・青）
            // 赤弾の隙間を埋めるように配置され、遅れてうねりながら迫る
            sEnemyShot* pBlue = new sEnemyShot;
            pBlue->x = pEnemyShotSet->x;
            pBlue->y = pEnemyShotSet->y;
            pBlue->muki = baseAngle + i * angleStep + spiralOffset + (angleStep / 2.0); // 赤の隙間
            pBlue->speed = 1.5;
            pBlue->kind = img_enemyShotMediumOval[4]; // 4:青
            pBlue->param_i[0] = 1; // 動作フラグ: 曲線

            pBlue->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBlue->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBlue;
            pEnemyShotSet->pEnemyShotHead->prev = pBlue;

            // ⚪ 白弾：逆回転の錯覚用（菱形弾・白）
            // 螺旋オフセットを逆方向にすることで、隙間が逆向きに動いているように見せ、
            // プレイヤーの空間認識を撹乱する（セーフティゾーンのガイド役）
            sEnemyShot* pWhite = new sEnemyShot;
            pWhite->x = pEnemyShotSet->x;
            pWhite->y = pEnemyShotSet->y;
            pWhite->muki = baseAngle + i * angleStep - spiralOffset + (angleStep / 4.0);
            pWhite->speed = 2.2;
            pWhite->kind = img_enemyShotDiamond[6]; // 6:白
            pWhite->param_i[0] = 2; // 動作フラグ: 直進

            pWhite->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pWhite->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pWhite;
            pEnemyShotSet->pEnemyShotHead->prev = pWhite;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 青弾（param_i[0] == 1）は少し旋回させ、「うねり」を強調する
        if (pShot->param_i[0] == 1) {
            pShot->muki += 0.005; // 時計回りに緩やかに曲がる
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン（ご指定の関数名）
void EnemyPat_SignPole_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480。中央やや上に配置
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // HPを少し増やしてパターン持続時間を確保
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵本体はゆっくりと左右に往復移動
        enemy.x += 1.2 * (double)muki;
        if (count % 180 == 90) {
            muki *= -1;
        }
    }

    // 15フレームごとに弾幕セットを生成（螺旋の連続性を保つ適度な間隔）
    if (count % 15 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSignPole;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;

        // 基本の狙い方向はプレイヤーへ
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // kind を発射カウンタとして使用し、螺旋の位相オフセット計算に利用する
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