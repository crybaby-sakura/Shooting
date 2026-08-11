// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：座標接続弾幕「シャドウ・リンク」
static void ShotShadowLink(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 発射時は軽い音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // テレポート先の座標は、EnemyPat_Warp_Zai側から param_d[0], [1] に渡されている前提
        // プレイヤーに向かって、ブレのあるゆっくりした弾を5〜8発発射
        int shotNum = 5 + GetRand(3) + 100; // GetRand(3) は 0〜3 のため、5+0=5 〜 5+3=8発
        for (int i = 0; i < shotNum; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // プレイヤー方向を基準に、-30度〜+30度のランダムなブレを加える
            double baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShot->muki = baseMuki + (GetRand(240) - 120) / 180.0 * DX_PI;

            // 非常に遅い速度で発射 (1.5 〜 2.5)
            pEnemyShot->speed = 1.5 + GetRand(40) / 10.0;

            // 青の中玉で発射（落ち着いた色で油断させる）
            pEnemyShot->kind = img_enemyShotMediumBall[4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    else if (pEnemyShotSet->count == 60) {
        // テレポート直前の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else if (pEnemyShotSet->count == 61) {
        // テレポート実行（ボスの座標を強制移動）
        enemy.x = pEnemyShotSet->param_d[0];
        enemy.y = pEnemyShotSet->param_d[1];

        // 空間が歪んだような重い音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 全ての弾の軌道をテレポート先のボス位置に向けて変更
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 現在の位置から、テレポート後のボス位置への角度を再計算
            pShot->muki = atan2(enemy.y - pShot->y, enemy.x - pShot->x);
            pShot->speed = 5.0; // 速度を上げて一気に引き寄せるように見せる
            pShot->kind = img_enemyShotMediumBall[0]; // 色を赤に変更して危険を知らせる（フェアネス演出）
            pShot = pShot->next;
        }
    }

    // 弾の移動処理（毎フレーム）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Warp_Zai()
{
    // テレポート先の座標パターン（対角に飛ぶように設定し、軌道の変化を目立たせる）
    static double telePos[4][2] = {
        {100.0, 350.0},   // 左下
        {380.0, 100.0},   // 右上
        {100.0, 100.0},   // 左上
        {380.0, 350.0}    // 右下
    };
    static int teleportIndex;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        teleportIndex = GetRand(3);
    }

    // 180フレームごとに弾幕セットを生成
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotShadowLink;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 次のテレポート先の座標をパラメータとして弾幕セットに渡す
        pEnemyShotSet->param_d[0] = telePos[teleportIndex][0];
        pEnemyShotSet->param_d[1] = telePos[teleportIndex][1];
        teleportIndex = (teleportIndex + 1 + GetRand(2)) % 4; // 次回は別の場所へ

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}