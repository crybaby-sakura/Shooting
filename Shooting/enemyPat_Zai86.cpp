// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：近接式・無双閃剣
static void ShotMeleeBlade(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    double baseX = enemy.x;
    double baseY = enemy.y;

    // ボスの中心からどれだけ離れた位置を「刃の発生点」とするか
    // この距離の内側には弾が飛んでこない（＝近接の安全地帯）
    const double offsetDist = 40.0;

    // 予備動作（振りかぶり）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 第1斬撃：右から左への薙ぎ払い（赤）
    if (pEnemyShotSet->count == 30) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int shotNum = 50; // 密度を高めて極太の壁にする
        double baseAngle = DX_PI; // 左向き

        for (int i = 0; i < shotNum; i++) {
            pEnemyShot = new sEnemyShot;

            // GetRand(40) - 20 は -20 〜 +20 の範囲。
            // 500.0で割ることで、-0.04 〜 +0.04 ラジアンの僅かなバラつきを作る
            double angle = baseAngle + (GetRand(40) - 20) / 500.0 * 10;

            // ボス中心から offsetDist だけ離れた位置を発射起点とする
            pEnemyShot->x = baseX + offsetDist * cos(angle);
            pEnemyShot->y = baseY + offsetDist * sin(angle);
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 10.0; // 速めに設定して「斬撃」の速さを表現

            // 赤い中玉を使用
            pEnemyShot->kind = img_enemyShotMediumBall[0];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 第2斬撃：左から右への逆袈裟（青）
    if (pEnemyShotSet->count == 50) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int shotNum = 50;
        double baseAngle = 0.0; // 右向き

        for (int i = 0; i < shotNum; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = baseAngle + (GetRand(40) - 20) / 500.0 * 10;
            pEnemyShot->x = baseX + offsetDist * cos(angle);
            pEnemyShot->y = baseY + offsetDist * sin(angle);
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 10.0;

            // 青い中玉を使用
            pEnemyShot->kind = img_enemyShotMediumBall[4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // フィニッシュ：八方全周ドーナツ状の波（黄）
    if (pEnemyShotSet->count == 70) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int divNum = 36*10; // 36方向に分割
        // ドーナツ状（極太の輪）に見せるため、2重に発射する
        double offsets[2] = { 30.0*2, 45.0*3 };

        for (int o = 0; o < 2; o++) {
            for (int i = 0; i < divNum; i++) {
                pEnemyShot = new sEnemyShot;

                double angle = (2.0 * DX_PI / divNum) * i;
                // 中心から離れた位置を発射起点とする（これにより中心は安全地帯）
                pEnemyShot->x = baseX + offsets[o] * cos(angle);
                pEnemyShot->y = baseY + offsets[o] * sin(angle);
                pEnemyShot->muki = angle;
                pEnemyShot->speed = 7.0;

                // 黄色い中玉を使用
                pEnemyShot->kind = img_enemyShotMediumBall[1];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_CloseCombat_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // リサジュー曲線で上下左右に揺らす
    double t = count * 0.01;
    enemy.x = 240.0 + 100.0 * sin(3.0 * t);
    enemy.y = 120.0 + 40.0 * sin(2.0 * t);

    // 150フレーム周期で斬撃弾幕を起動
    // (70フレームで攻撃完了 + 80フレームの硬直時間)
    if (count % 150 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMeleeBlade;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->alive = 120;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}