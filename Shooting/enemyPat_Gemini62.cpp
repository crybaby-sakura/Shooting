// enemyPat_deterministicChaos.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：確定性混沌（デターミニスティック・カオス）
static void ShotDeterministicChaos(sEnemyShotSet* pEnemyShotSet)
{
    // フェーズの切り替えタイミング（フレーム数）
    const int PHASE1_END = 240; // 約4秒間射出
    const int PHASE2_END = 264; // +0.4秒間（24F）静止

    int phase = 0;
    if (pEnemyShotSet->count < PHASE1_END) phase = 1;
    else if (pEnemyShotSet->count < PHASE2_END) phase = 2;
    else phase = 3;

    // Phase 1中はボスの位置に発生源を追従させる
    if (phase == 1) {
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
    }

    // ==========================================
    // Phase 1: 揺らぎの渦（低速・高密度の射出）
    // ==========================================
    if (phase == 1) {
        // シャッフルリズム（12フレーム周期の中で0と8のタイミングで射出）
        if (pEnemyShotSet->count % 12 == 0 || pEnemyShotSet->count % 12 == 8) {

            // 射出音（うるさくなりすぎないよう12フレーム周期の頭だけ鳴らす）
            if (pEnemyShotSet->count % 12 == 0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
            else {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }

            // 5方向の螺旋
            int ways = 7;
            for (int w = 0; w < ways; w++) {

                // --- 赤弾 (時計回り) ---
                sEnemyShot* pRed = new sEnemyShot;
                pRed->x = pEnemyShotSet->x;
                pRed->y = pEnemyShotSet->y;
                // GetRand(10) は 0~10 を返すので 0.0~0.5 の揺らぎになる
                pRed->speed = 2.5 + GetRand(10) / 20.0;
                pRed->muki = pEnemyShotSet->param_d[0] + w * (DX_PI * 2.0 / ways);
                pRed->kind = img_enemyShotSmallBall[0]; // 0:赤
                pRed->param_i[0] = 0; // 色判定用フラグ

                pRed->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pRed->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pRed;
                pEnemyShotSet->pEnemyShotHead->prev = pRed;

                // --- 青弾 (反時計回り) ---
                sEnemyShot* pBlue = new sEnemyShot;
                pBlue->x = pEnemyShotSet->x;
                pBlue->y = pEnemyShotSet->y;
                pBlue->speed = 2.5 + GetRand(10) / 20.0;
                pBlue->muki = pEnemyShotSet->param_d[1] + w * (DX_PI * 2.0 / ways);
                pBlue->kind = img_enemyShotSmallBall[4]; // 4:青
                pBlue->param_i[0] = 1; // 色判定用フラグ

                pBlue->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pBlue->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pBlue;
                pEnemyShotSet->pEnemyShotHead->prev = pBlue;
            }

            // 螺旋の角度を更新
            pEnemyShotSet->param_d[0] += 0.12; // 時計回り
            pEnemyShotSet->param_d[1] -= 0.12; // 反時計回り
        }
    }

    // ==========================================
    // Phase 2: 思考停止（全停止の瞬間）
    // ==========================================
    if (pEnemyShotSet->count == PHASE1_END) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ==========================================
    // Phase 3: ベクトル反転＆拡散（再加速の瞬間）
    // ==========================================
    if (pEnemyShotSet->count == PHASE2_END) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 全弾のベクトルを自機基準で再計算
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                // 赤弾: 自機から放射状に遠ざかるベクトル
                pShot->muki = atan2(pShot->y - player.y, pShot->x - player.x);
                pShot->speed = 5.0 + GetRand(20) / 10.0; // 5.0 〜 7.0 の超高速拡散
            }
            else {
                // 青弾: 自機に吸い込まれるベクトル
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->speed = 3.5 + GetRand(15) / 10.0; // 3.5 〜 5.0 で迫る
            }
            pShot = pShot->next;
        }
    }

    // ==========================================
    // 移動処理（Phase 2の静止中はスキップ）
    // ==========================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (phase == 1 || phase == 3) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_TooChaotic_Gemini()
{
    static int mode_count = 0;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 120.0; // 弾幕が映えるよう中央上部に配置
        enemy.maxHp = enemy.hp = 200;
        mode_count = 0;
    }

    // フワフワと漂う不気味な動き
    enemy.x = 240.0 + 40.0 * sin(count * DX_PI / 150.0);
    enemy.y = 120.0 + 15.0 * sin(count * DX_PI / 110.0);

    // 400フレーム（約6.6秒）周期で弾幕を展開
    if (mode_count % 400 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDeterministicChaos;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // 自由パラメータに各螺旋の初期角度を保存
        pEnemyShotSet->param_d[0] = 0.0;   // 赤弾用
        pEnemyShotSet->param_d[1] = DX_PI; // 青弾用

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    mode_count++;
}