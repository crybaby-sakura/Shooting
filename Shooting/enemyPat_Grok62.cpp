// enemyPat_Tmp.cpp
// カオス高難易度弾幕：虚数螺旋崩壊（Imaginary Spiral Collapse）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 使用素材まとめ
// 効果音: sound_enemyShot_medium / heavy / extreme / sound_enemyCharge
// 弾種: 中玉(MediumBall), 小玉(SmallBall), 菱形弾(Diamond), 鱗弾(Scale), 中楕円弾(MediumOval), 短レーザー(Laser)
// 色: 主に 3:シアン, 4:青, 5:マゼンタ, 0:赤, 8:橙 をカオス用に混在
// ============================================================

// 螺旋弾の移動・分裂・逆流を制御する共通処理
static void UpdateSpiralShots(sEnemyShotSet* pEnemyShotSet, int phase)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // phase に応じて挙動を変える
        if (phase == 1) {
            // 通常螺旋移動（わずかに角速度を与えて曲げる）
            pShot->muki += pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (phase == 2) {
            // 二次分裂タイミング（param_i[0] に分裂予定フレームを入れてある）
            if (pShot->count == pShot->param_i[0] && pShot->param_i[1] == 0) {
                pShot->param_i[1] = 1; // 分裂済みフラグ
                // 2方向に分裂弾を生成
                for (int k = 0; k < 2; k++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    double offset = (k == 0 ? 1.0 : -1.0) * (0.6 + GetRand(40) / 100.0);
                    pNew->muki = pShot->muki + offset;
                    pNew->speed = pShot->speed * (0.85 + GetRand(30) / 100.0);
                    pNew->kind = pShot->kind;
                    pNew->param_d[0] = pShot->param_d[0] * 0.7; // 角速度も減衰
                    pNew->param_i[0] = 9999; // 再分裂しない
                    pNew->param_i[1] = 1;
                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;
                }
            }
            pShot->muki += pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (phase == 3) {
            // 逆流フェーズ：速度を半減したあと、核方向へ引き寄せ気味に向きを変える
            if (pShot->count < pShot->param_i[2] + 8) {
                // 減速区間
                pShot->speed *= 0.92;
            }
            else {
                // 逆流：中心方向へ徐々に向きを変える
                double toCenter = atan2(pEnemyShotSet->y - pShot->y, pEnemyShotSet->x - pShot->x);
                double diff = toCenter - pShot->muki;
                while (diff > DX_PI) diff -= 2.0 * DX_PI;
                while (diff < -DX_PI) diff += 2.0 * DX_PI;
                pShot->muki += diff * 0.08;
                // たまに急激な90度近い転換
                if (GetRand(200) == 0) {
                    pShot->muki += (GetRand(1) ? 1.0 : -1.0) * (1.2 + GetRand(20) / 100.0);
                }
                pShot->speed = 2.2 + GetRand(80) / 100.0;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 最終崩壊：ランダム飛散
            if (pShot->param_i[3] == 0) {
                pShot->param_i[3] = 1;
                pShot->muki = GetRand(628) / 100.0; // 0〜約2π
                pShot->speed = 2.0 + GetRand(400) / 100.0;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// メイン螺旋パターン（虚数核からの多重螺旋）※1ショットセットで1サイクル分を実行
static void ShotImaginaryCore(sEnemyShotSet* pEnemyShotSet)
{
    // 位相を param_i[0] で管理（0:初期螺旋, 1:分裂準備, 2:分裂中, 3:逆流, 4:崩壊）
    // 角度は param_d[0]/[1] で管理（static変数の代わり）
    int& phase = pEnemyShotSet->param_i[0];
    double& baseAngleCW = pEnemyShotSet->param_d[0];
    double& baseAngleCCW = pEnemyShotSet->param_d[1];

    // 初回のみ初期化と予告音
    if (pEnemyShotSet->count == 0) {
        phase = 0;
        baseAngleCW = 0.0;
        baseAngleCCW = DX_PI;
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ---- フェーズ遷移 ----
    if (pEnemyShotSet->count == 90) {
        phase = 1; // 分裂準備
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    if (pEnemyShotSet->count == 120) {
        phase = 2; // 本格分裂
    }
    if (pEnemyShotSet->count == 210) {
        phase = 3; // 逆流開始
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        // 既存弾に逆流開始フレームを記録
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            p->param_i[2] = p->count; // 逆流開始時の弾countを記録
            p = p->next;
        }
    }
    if (pEnemyShotSet->count == 300) {
        phase = 4; // 最終崩壊
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // ---- 弾生成 ----
    // 初期螺旋（多重層・時計回り＆反時計回り）
    if (phase == 0 && pEnemyShotSet->count % 3 == 0 && pEnemyShotSet->count < 90) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 時計回り3層
        for (int layer = 0; layer < 3; layer++) {
            for (int i = 0; i < 2; i++) { // 少しバラして密度を上げる
                sEnemyShot* pShot = new sEnemyShot;
                double ang = baseAngleCW + layer * 0.35 + (GetRand(20) - 10) / 100.0;
                pShot->x = pEnemyShotSet->x + cos(ang) * (8.0 + layer * 4.0);
                pShot->y = pEnemyShotSet->y + sin(ang) * (8.0 + layer * 4.0);
                pShot->muki = ang;
                pShot->speed = 1.6 + layer * 0.35 + GetRand(30) / 100.0;
                pShot->margin = 200;
                pShot->param_d[0] = 0.028 + layer * 0.006; // 角速度
                pShot->param_i[0] = 40 + GetRand(50);       // 分裂予定フレーム（後で使う）
                pShot->param_i[1] = 0;
                // 色と種類：青〜シアン系中心
                int col = 3 + GetRand(2); // 3 or 4
                if (GetRand(5) == 0) col = 5; // たまにマゼンタ
                switch (GetRand(3)) {
                case 0: pShot->kind = img_enemyShotMediumBall[col]; break;
                case 1: pShot->kind = img_enemyShotDiamond[col]; break;
                default: pShot->kind = img_enemyShotScale[col]; break;
                }
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
        // 反時計回り2層
        for (int layer = 0; layer < 2; layer++) {
            sEnemyShot* pShot = new sEnemyShot;
            double ang = baseAngleCCW - layer * 0.4 + (GetRand(16) - 8) / 100.0;
            pShot->x = pEnemyShotSet->x + cos(ang) * (10.0 + layer * 5.0);
            pShot->y = pEnemyShotSet->y + sin(ang) * (10.0 + layer * 5.0);
            pShot->muki = ang;
            pShot->speed = 1.9 + layer * 0.4 + GetRand(25) / 100.0;
            pShot->margin = 200;
            pShot->param_d[0] = -0.032 - layer * 0.007; // 逆回転
            pShot->param_i[0] = 35 + GetRand(45);
            pShot->param_i[1] = 0;
            int col = 4 + GetRand(2); // 4 or 5
            pShot->kind = (GetRand(1) ? img_enemyShotMediumBall[col] : img_enemyShotMediumOval[col]);
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        baseAngleCW += 0.22 + GetRand(8) / 100.0;
        baseAngleCCW -= 0.19 + GetRand(6) / 100.0;
    }

    // 分裂フェーズ：核が分裂したように見える追加螺旋
    if (phase == 2 && pEnemyShotSet->count % 4 == 0 && pEnemyShotSet->count < 200) {
        // 仮想分裂位置（ランダムにずらす）
        double ox = (GetRand(120) - 60);
        double oy = (GetRand(80) - 40);
        int dir = (GetRand(1) ? 1 : -1);
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double ang = baseAngleCW + i * (DX_PI * 2.0 / 5.0) + (GetRand(30) - 15) / 100.0;
            pShot->x = pEnemyShotSet->x + ox + cos(ang) * 12.0;
            pShot->y = pEnemyShotSet->y + oy + sin(ang) * 12.0;
            pShot->muki = ang;
            pShot->speed = 2.1 + GetRand(60) / 100.0;
            pShot->margin = 200;
            pShot->param_d[0] = dir * (0.04 + GetRand(20) / 1000.0);
            pShot->param_i[0] = 25 + GetRand(40);
            pShot->param_i[1] = 0;
            int col = (GetRand(3) == 0) ? 0 : (3 + GetRand(3)); // たまに赤を混ぜる
            pShot->kind = img_enemyShotSmallBall[col];
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        baseAngleCW += 0.31;
    }

    // 逆流中に短レーザーをランダム方向へ追加
    if (phase == 3 && pEnemyShotSet->count % 7 == 0 && pEnemyShotSet->count < 290) {
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = GetRand(628) / 100.0;
            pShot->speed = 4.5 + GetRand(100) / 100.0;
            pShot->margin = 200;
            pShot->param_d[0] = 0.0;
            pShot->param_i[0] = 9999;
            pShot->param_i[1] = 1;
            pShot->param_i[2] = pShot->count;
            int col = 5; // マゼンタ
            pShot->kind = img_enemyShotLaser[col];
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 最終崩壊時の弾雨
    if (phase == 4 && pEnemyShotSet->count < 340) {
        if (pEnemyShotSet->count % 2 == 0) {
            for (int i = 0; i < 8; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x + (GetRand(60) - 30);
                pShot->y = pEnemyShotSet->y + (GetRand(40) - 20);
                pShot->muki = GetRand(628) / 100.0;
                pShot->speed = 2.8 + GetRand(200) / 100.0;
                pShot->margin = 200;
                pShot->param_d[0] = (GetRand(1) ? 1.0 : -1.0) * 0.015;
                pShot->param_i[0] = 9999;
                pShot->param_i[1] = 1;
                pShot->param_i[3] = 0; // 飛散フラグ用
                int col = GetRand(8);
                // 種類をランダムに
                int type = GetRand(4);
                if (type == 0) pShot->kind = img_enemyShotMediumBall[col];
                else if (type == 1) pShot->kind = img_enemyShotDiamond[col];
                else if (type == 2) pShot->kind = img_enemyShotScale[col];
                else pShot->kind = img_enemyShotSmallBall[col];
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // ---- 全弾の移動更新 ----
    int movePhase = 1;
    if (phase == 2) movePhase = 2;
    else if (phase == 3) movePhase = 3;
    else if (phase >= 4) movePhase = 4;
    UpdateSpiralShots(pEnemyShotSet, movePhase);
}

// 敵本体パターン
void EnemyPat_TooChaotic_Grok()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    else {
        // 敵は緩やかに左右へ揺れつつ少し上下
        enemy.x = 240.0 + sin(count * 0.025) * 70.0;
        enemy.y = 80.0 + sin(count * 0.017) * 25.0;
    }

    // 虚数核（ショットセット）を周期的に発射（360フレームごと）
    // static変数は使わず、count の剰余でタイミングを管理
    if (count >= 30 && (count - 30) % 360 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotImaginaryCore;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        // param で状態を保持（static変数の代わり）
        pEnemyShotSet->param_i[0] = 0; // phase
        pEnemyShotSet->param_d[0] = 0.0; // baseAngleCW
        pEnemyShotSet->param_d[1] = 0.0; // baseAngleCCW
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}