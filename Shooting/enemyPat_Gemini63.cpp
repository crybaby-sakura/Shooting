// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：トリコロール・ヘリックス（サインポール錯視）
static void ShotTricolorHelix(sEnemyShotSet* pEnemyShotSet)
{
    // ボス本体の移動に合わせてエミッターの座標を追従させる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // --- 弾の生成 ---
    // 錯視ギミック（ポール錯視）：回転方向（角速度）の反転と弾速を連動させる
    // 周期的に sin / cos で揺らすことで、ヌルヌルと流れ落ちるような錯覚を生み出す
    double omega = 0.07 * sin(pEnemyShotSet->count * DX_PI / 300.0);
    pEnemyShotSet->param_d[0] += omega; // 発射ベース角度の更新

    double speed = 2.0 + 0.5 * cos(pEnemyShotSet->count * DX_PI / 300.0);

    // 主弾：トリコロール・スパイラル (3フレームに1回発射)
    if (pEnemyShotSet->count % 2 == 0) {
        // 軽い発射音
        if (pEnemyShotSet->count % 6 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 3方向(120度ずつ)にずらして発射
            double angle = pEnemyShotSet->param_d[0] + (DX_PI * 2.0 / 3.0) * i;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = speed;

            // 弾の種類と色を設定 (赤:0, 白:6, 青:4)
            if (i == 0) pEnemyShot->kind = img_enemyShotMediumBall[0];
            else if (i == 1) pEnemyShot->kind = img_enemyShotMediumBall[6];
            else if (i == 2) pEnemyShot->kind = img_enemyShotMediumBall[4];

            // 循環リストの末尾に追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // アクセント弾：キャップ・リング (150フレーム周期で全方位発射)
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 150 == 0) {
        // 重い発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int way = 36;
        for (int i = 0; i < way; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 24方位へリング状に展開
            pEnemyShot->muki = (DX_PI * 2.0 / way) * i;
            pEnemyShot->speed = 1.8;
            pEnemyShot->kind = img_enemyShotLargeBall[1]; // 黄色の大玉（金属キャップ風）

            // 循環リストの末尾に追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 既存弾の移動処理 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_SignPole_Gemini()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 100.0; // 弾幕が綺麗に見えるよう少し高めに配置
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ボスはゆっくりと左右に往復移動する
        enemy.x += 0.4 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // カウント60で、弾を生成し続ける「エミッター」としての弾セットを1つだけ登録
    if (count == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTricolorHelix;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->param_d[0] = 0.0; // 発射角度のベース値を初期化

        // 弾リストのダミーヘッドを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットリストへ登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}