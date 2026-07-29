// enemyPat_hakyouRanhansha.cpp
// 鏡面反撃「破鏡乱反射」
//
// ボスは見えない守護鏡を纏っている、という設定。
// プレイヤー弾の命中によってHPが減少した瞬間をトリガーとし、
// プレイヤー方向へ反射弾（撃ち返し弾）を放つ。
// HPが減るほど鏡にヒビが入り、反射角度のブレ（乱反射）と弾速・弾数が増していく。
//
// 注意：sPlayerShotには座標のみで向き情報が無いため、
// 「入射ベクトルの厳密な反射」ではなく、
// 「鏡は打ち込んできた相手（プレイヤー）へ跳ね返す」という近似で
// 反射方向 = ボスからプレイヤーへの方向、として扱っている。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 反射弾１セット分の弾幕（撃ち返し弾本体）
static void ShotMirrorReflect(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (enemy.hp % 50 == 49) {
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
        else {
            if (enemy.hp % 2 == 0) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }
        }

        int    shotNum = pEnemyShotSet->param_i[0]; // 発射数（コンボ・ダメージ量で変動）
        double spread = pEnemyShotSet->param_d[0]; // 扇の半幅（ラジアン）＝乱反射の強さ
        double speed = pEnemyShotSet->param_d[1]; // 弾速
        int    colorIdx = pEnemyShotSet->param_i[1]; // 鏡のヒビ具合を表す色

        for (int i = 0; i < shotNum; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 中心角(muki)を基準に扇状へ配置し、さらに乱反射のブレを加える
            double angle_offset = (shotNum == 1) ? 0.0
                : spread * 2.0 * ((double)i / (shotNum - 1) - 0.5);
            double rand_offset = (GetRand(2000) - 1000) / 1000.0 * spread * 0.3;
            pEnemyShot->muki = pEnemyShotSet->muki + angle_offset + rand_offset;

            pEnemyShot->speed = speed + GetRand(20) / 100.0;

            // 弾の種類一覧: 菱形弾(4.5x2.5) ＝ 割れた鏡の破片をイメージ
            // 色一覧: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
            pEnemyShot->kind = img_enemyShotDiamond[colorIdx];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン：鏡面反撃「破鏡乱反射」
void EnemyPat_Counter_Claude()
{
    static int muki;
    static int prevHp;       // 前フレームのHP（被弾検知用）
    static int comboCount;   // 短時間の連続被弾コンボ
    static int lastHitFrame; // 直近の被弾フレーム(count)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        prevHp = enemy.hp;
        comboCount = 0;
        lastHitFrame = -9999;
    }
    else {
        // 守護鏡を構えたまま、ゆったりと左右にスウェイ
        enemy.x += 0.5 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // HP比率から「鏡のヒビ具合＝フェーズ」を決定
    double hpRatio = (double)enemy.hp / (double)enemy.maxHp;
    double spreadDeg, speedBonus;
    int colorIdx;
    if (hpRatio > 0.75) { spreadDeg = 3.0;  speedBonus = 0.0; colorIdx = 3; } // 無傷：シアン
    else if (hpRatio > 0.50) { spreadDeg = 10.0; speedBonus = 0.2; colorIdx = 6; } // 小ヒビ：白
    else if (hpRatio > 0.25) { spreadDeg = 20.0; speedBonus = 0.4; colorIdx = 8; } // 大ヒビ：橙
    else { spreadDeg = 35.0; speedBonus = 0.6; colorIdx = 0; } // 破鏡寸前：赤

    // 被弾検知：このフレームでHPが減っていれば命中があったとみなす
    int dmg = prevHp - enemy.hp;
    if (dmg > 0 && count > 1) {
        // 直近30フレーム以内の被弾ならコンボ継続、そうでなければリセット
        if (count - lastHitFrame <= 30) comboCount++;
        else                              comboCount = 1;
        lastHitFrame = count;

        // コンボとダメージ量に応じて反射弾数を決定（過剰生成を防ぐため上限あり）
        int shotNum = 3 + (comboCount - 1) + dmg / 5;
        if (shotNum > 16) shotNum = 16;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMirrorReflect;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        // 反射方向＝鏡はプレイヤーへ跳ね返す、という近似
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->param_i[0] = shotNum;
        pEnemyShotSet->param_d[0] = spreadDeg / 180.0 * DX_PI;
        pEnemyShotSet->param_d[1] = 2.0 + speedBonus + comboCount * 0.05;
        pEnemyShotSet->param_i[1] = colorIdx;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    prevHp = enemy.hp;
}