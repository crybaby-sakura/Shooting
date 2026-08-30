// enemyPat_SingularityCollapse.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：不可逆量子特異点（Singularity Collapse）
static void Shot_SingularityCollapse(sEnemyShotSet* pSet)
{
    // pSet->count : 弾幕の経過フレーム
    // pSet->param_d[0] : 自機のX移動ベクトル（EnemyPat_TAS_Geminiから渡される）
    // pSet->param_d[1] : 自機のY移動ベクトル（EnemyPat_TAS_Geminiから渡される）

    int local_t = pSet->count % 400; // 400フレーム周期でループ
    bool playExtremeSound = false;

    // =========================================================
    // 【第1波】フレーム依存誘導（0～119F）
    // =========================================================
    if (local_t < 120) {
        if (local_t % 10 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 毎フレーム10発の微小弾を射出 (600発/秒)
        for (int i = 0; i < 10; i++) {
            sEnemyShot* pNew = new sEnemyShot;
            pNew->x = pSet->x;
            pNew->y = pSet->y;

            // ベースは全方位。毎フレーム少しずつ回転を加える
            double base_angle = (i * DX_PI * 2.0 / 10.0) + (local_t * 0.05);
            pNew->muki = base_angle;
            pNew->speed = 2.0 + (GetRand(100) / 100.0);
            pNew->kind = img_enemyShotSmallBall[5]; // マゼンタ小玉
            pNew->param_i[0] = 0; // 状態0: 誘導フェーズ

            pNew->prev = pSet->pEnemyShotHead->prev;
            pNew->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pNew;
            pSet->pEnemyShotHead->prev = pNew;
        }
    }

    // =========================================================
    // 【第2波】絶対貫通レーザー格子 & 特異点マーカー配置（0F目のみ）
    // =========================================================
    if (local_t == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 画面全体(480x480)に60px間隔でレーザー格子を展開
        // 交差点は x,y 共に 30, 90, 150, 210, 270, 330, 390, 450
        for (int i = 30; i <= 450; i += 60) {
            // 縦レーザー
            for (int j = -32; j <= 512; j += 64) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = i;  pNew->y = j;
                pNew->muki = DX_PI / 2.0; pNew->speed = 0.0;
                pNew->kind = img_enemyShotLaser[4]; // 青レーザー
                pNew->param_i[0] = 100; pNew->param_i[1] = 0; // 状態100: レーザー

                pNew->prev = pSet->pEnemyShotHead->prev; pNew->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pNew; pSet->pEnemyShotHead->prev = pNew;
            }
            // 横レーザー
            for (int j = -32; j <= 512; j += 64) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = j;  pNew->y = i;
                pNew->muki = 0.0; pNew->speed = 0.0;
                pNew->kind = img_enemyShotLaser[4]; // 青レーザー
                pNew->param_i[0] = 100; pNew->param_i[1] = 0; // 状態100: レーザー

                pNew->prev = pSet->pEnemyShotHead->prev; pNew->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pNew; pSet->pEnemyShotHead->prev = pNew;
            }
        }

        // 交差点に「特異点マーカー（ブラックホール）」を設置
        // ここに微小弾が吸い込まれ、後に炸裂する
        for (int x = 30; x <= 450; x += 60) {
            for (int y = 30; y <= 450; y += 60) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = x; pNew->y = y;
                pNew->muki = 0.0; pNew->speed = 0.0;
                pNew->kind = img_enemyShotMediumBall[7]; // 黒中玉
                pNew->param_i[0] = 200; pNew->param_i[1] = 0; // 状態200: マーカー

                pNew->prev = pSet->pEnemyShotHead->prev; pNew->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pNew; pSet->pEnemyShotHead->prev = pNew;
            }
        }
    }

    // =========================================================
    // 登録された全弾の更新処理
    // =========================================================
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 先に次のポインタを保持

        if (pShot->param_i[0] == 0) {
            // --- 状態0：誘導フェーズ ---
            double vx = pSet->param_d[0];
            double vy = pSet->param_d[1];
            double v_mag = hypot(vx, vy);

            // 自機の移動ベクトルが存在すれば、そのベクトル方向へ弾道が追従カーブする
            if (v_mag > 0.5) {
                double target_angle = atan2(vy, vx);
                double diff = target_angle - pShot->muki;
                while (diff > DX_PI) diff -= DX_PI * 2;
                while (diff < -DX_PI) diff += DX_PI * 2;
                pShot->muki += diff * 0.03;
            }

            // 一定時間（ランダム）経過で吸着フェーズへ移行
            if (pShot->count > 100 + GetRand(40)) {
                pShot->param_i[0] = 1;
                // 最寄りの交差点を算出
                double target_x = floor((pShot->x - 30.0) / 60.0 + 0.5) * 60.0 + 30.0;
                double target_y = floor((pShot->y - 30.0) / 60.0 + 0.5) * 60.0 + 30.0;
                if (target_x < 30) target_x = 30; if (target_x > 450) target_x = 450;
                if (target_y < 30) target_y = 30; if (target_y > 450) target_y = 450;
                pShot->param_d[1] = target_x;
                pShot->param_d[2] = target_y;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[0] == 1) {
            // --- 状態1：吸着フェーズ ---
            double tx = pShot->param_d[1];
            double ty = pShot->param_d[2];
            double dist = hypot(ty - pShot->y, tx - pShot->x);

            if (dist < 8.0) {
                // 特異点に吸収され、自身は消滅する（プール節約）
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                pShot = pNext;
                continue;
            }
            else {
                // 特異点へ鋭く向かう
                double angle = atan2(ty - pShot->y, tx - pShot->x);
                double diff = angle - pShot->muki;
                while (diff > DX_PI) diff -= DX_PI * 2;
                while (diff < -DX_PI) diff += DX_PI * 2;
                pShot->muki += diff * 0.15;
                pShot->speed = 6.0;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else if (pShot->param_i[0] == 200) {
            // --- 状態200：特異点マーカー（第3波：時限拡散の乱数爆発） ---
            pShot->param_i[1]++;
            if (pShot->param_i[1] == 260) { // 260フレーム目に一斉炸裂
                playExtremeSound = true;

                // 32方向へ炸裂。交差点64個×32発＝2048発 が可視領域を埋め尽くす
                for (int i = 0; i < 32; i++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    // 【TAS要素】乱数によるサブピクセル単位のズレを生じさせ、回避可能な1pxの隙間を生成
                    pNew->muki = (i * DX_PI * 2.0 / 32.0) + ((GetRand(200) - 100) / 10000.0);
                    pNew->speed = 1.2 + (GetRand(50) / 100.0);
                    pNew->kind = img_enemyShotSmallBall[0]; // 赤小玉
                    pNew->param_i[0] = 99; // 炸裂後の通常直進弾

                    pNew->prev = pSet->pEnemyShotHead->prev; pNew->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pNew; pSet->pEnemyShotHead->prev = pNew;
                }
                // 炸裂後、特異点マーカー自身は消滅
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                pShot = pNext;
                continue;
            }
        }
        else if (pShot->param_i[0] == 100) {
            // --- 状態100：レーザー格子 ---
            pShot->param_i[1]++;
            if (pShot->param_i[1] > 399) { // 周期の終わりに消滅
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                pShot = pNext;
                continue;
            }
        }
        else if (pShot->param_i[0] == 99) {
            // --- 状態99：炸裂後の通常直進弾 ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        if (pShot->count >= 400) pShot->margin = -9999;

        pShot = pNext;
    }

    if (playExtremeSound) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }
}


// 敵本体のパターン（TAS専用：不可逆量子特異点）
void EnemyPat_TAS_Gemini()
{
    static double p_hist_x[3];
    static double p_hist_y[3];
    static sEnemyShotSet* pMasterSet = nullptr;

    if (count == 1) {
        // ゲーム画面は 480x480。圧倒的絶望感を出すためボスは中央上部から動かない。
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;

        p_hist_x[0] = p_hist_x[1] = p_hist_x[2] = player.x;
        p_hist_y[0] = p_hist_y[1] = p_hist_y[2] = player.y;

        // 弾幕管理用のSetを1つだけ生成して永続化する
        pMasterSet = new sEnemyShotSet;
        pMasterSet->count = 0;
        pMasterSet->patternFunc = Shot_SingularityCollapse;
        pMasterSet->x = enemy.x;
        pMasterSet->y = enemy.y;

        pMasterSet->pEnemyShotHead = new sEnemyShot;
        pMasterSet->pEnemyShotHead->prev = pMasterSet->pEnemyShotHead;
        pMasterSet->pEnemyShotHead->next = pMasterSet->pEnemyShotHead;

        pMasterSet->prev = enemyShotSetHead.prev;
        pMasterSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMasterSet;
        enemyShotSetHead.prev = pMasterSet;
    }

    // 自機座標の履歴を更新 (過去3フレーム分を保存)
    p_hist_x[2] = p_hist_x[1]; p_hist_x[1] = p_hist_x[0]; p_hist_x[0] = player.x;
    p_hist_y[2] = p_hist_y[1]; p_hist_y[1] = p_hist_y[0]; p_hist_y[0] = player.y;

    // 自機の3フレーム間の移動ベクトルを計算し、弾幕側にパラメータとして伝達
    double dx = p_hist_x[0] - p_hist_x[2];
    double dy = p_hist_y[0] - p_hist_y[2];

    if (pMasterSet != nullptr) {
        pMasterSet->param_d[0] = dx;
        pMasterSet->param_d[1] = dy;
        pMasterSet->x = enemy.x;
        pMasterSet->y = enemy.y;
    }
}