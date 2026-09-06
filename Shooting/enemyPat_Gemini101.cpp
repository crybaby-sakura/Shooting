// enemyPat_phalanx.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ファランクス・チャージ（重装方陣の突撃）
static void ShotPhalanx(sEnemyShotSet* pEnemyShotSet)
{
    // param_d[0] に保存した方陣の基本進行速度
    double speed = pEnemyShotSet->param_d[0];
    double angle = pEnemyShotSet->muki;

    // 見えない基準点（方陣の中心）をゆっくり前進させる
    pEnemyShotSet->x += speed * cos(angle);
    pEnemyShotSet->y += speed * sin(angle);

    // 1. 大楯の壁（盾陣の展開）
    if (pEnemyShotSet->count == 0) {
        // 鈍く重い音で盾を展開
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int numShields = 13;   // 盾の枚数
        double spacing = 16.0; // 盾の間隔（中玉の直径14よりわずかに広い程度）

        for (int i = 0; i < numShields; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 中央を基準にしたインデックス（-6 〜 +6）
            int offsetIdx = i - numShields / 2;

            // 進行方向に対して90度（真横）のオフセット位置を計算
            double offsetX = cos(angle + DX_PI / 2.0) * offsetIdx * spacing;
            double offsetY = sin(angle + DX_PI / 2.0) * offsetIdx * spacing;

            pEnemyShot->x = pEnemyShotSet->x + offsetX;
            pEnemyShot->y = pEnemyShotSet->y + offsetY;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = speed;
            pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青の中玉を盾に見立てる

            // 双方向循環リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 2. 槍の突き出し（スラスト）
    // 盾の展開後、一定間隔（40フレーム毎）で盾のスキマから槍を突き出す
    if (pEnemyShotSet->count >= 40 && pEnemyShotSet->count <= 160 && pEnemyShotSet->count % 40 == 0) {
        // 鋭い発射音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int numGaps = 12; // 13枚の盾の間にあるスキマの数
        double spacing = 16.0;

        // 奇数番目と偶数番目のスキマから交互に突き出し、兵士が息を合わせて突いている感を出す
        int phase = (pEnemyShotSet->count / 40) % 2;

        for (int i = 0; i < numGaps; i++) {
            if (i % 2 == phase) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                // スキマの位置（-5.5 〜 +5.5）
                double offset = (i - 5.5) * spacing;

                double offsetX = cos(angle + DX_PI / 2.0) * offset;
                double offsetY = sin(angle + DX_PI / 2.0) * offset;

                // 槍（短レーザー）は長いので、盾の少し後ろから発射して隙間を通り抜けるようにする
                double backX = cos(angle + DX_PI) * 30.0;
                double backY = sin(angle + DX_PI) * 30.0;

                pEnemyShot->x = pEnemyShotSet->x + offsetX + backX;
                pEnemyShot->y = pEnemyShotSet->y + offsetY + backY;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = speed + 6.0; // 盾を素早く追い抜く高速な突き
                pEnemyShot->kind = img_enemyShotLaser[1]; // 黄色の短レーザーを槍に見立てる

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 既存の弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Phalanx_Gemini()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆっくり左右に移動し、方陣を展開する開始位置を散らす
        enemy.x += 0.8 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 3. 方陣の旋回・押し込み
    // 一定間隔（120フレーム）で新たな方陣を編成し、自機方向へ押し出す
    if (count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPhalanx;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // 自機を正確に狙う角度（方陣ごとに少しずつ旋回して追いつめる形になる）
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // param_d[0] にこの方陣の進行速度を記憶させておく
        pEnemyShotSet->param_d[0] = 1.2;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 兵士が号令をかけるようなイメージで予告音を鳴らす
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}