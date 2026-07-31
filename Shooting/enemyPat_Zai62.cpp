// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾の状態管理用定数 (param_i[0] に格納)
#define STATE_NOISE 0       // 基礎ノイズ弾
#define STATE_LASER 1       // 干渉レーザー
#define STATE_MUTATE_WAIT 2 // 変異待機（グリッチ遅延）
#define STATE_MUTATE_FIRE 3 // 変異発射済み（シアン狙い撃ち）
#define STATE_BIT 4         // 跳弾オブジェクト

// 弾幕パターン関数
static void ShotGlitchCascade(sEnemyShotSet* pEnemyShotSet)
{
    // --- 初期化処理 ---
    if (pEnemyShotSet->count == 0) {
        // 予告音を再生
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // ① 基礎ノイズ弾を150発生成 (小玉・白)
        for (int i = 0; i < 150; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            // GetRand(440) + 20 で 20〜460 の範囲に配置し、即消去を防ぐ
            while (true) {
                pEnemyShot->x = (double)(GetRand(440) + 20);
                pEnemyShot->y = (double)(GetRand(440) + 20);
                if (hypot(pEnemyShot->x - player.x, pEnemyShot->y - player.y) > 40) break;
            }
            pEnemyShot->muki = GetRand(3141) / 1000.0; // 0.000 ~ 3.141 (ラジアン)
            pEnemyShot->speed = (10 + GetRand(20)) / 100.0; // 0.1 ~ 0.3 の超低速
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 小玉、白(6)
            pEnemyShot->margin = 10;
            pEnemyShot->param_i[0] = STATE_NOISE;
            pEnemyShot->param_i[2] = 0;

            // リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ④ 跳弾オブジェクト(ビット)を4つ生成 (中楕円弾・緑)
        for (int i = 0; i < 4; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            // 四隅からスタート
            if (i == 0) { pEnemyShot->x = 30.0; pEnemyShot->y = 30.0; }
            else if (i == 1) { pEnemyShot->x = 450.0; pEnemyShot->y = 30.0; }
            else if (i == 2) { pEnemyShot->x = 30.0; pEnemyShot->y = 450.0; }
            else { pEnemyShot->x = 450.0; pEnemyShot->y = 450.0; }

            pEnemyShot->muki = atan2(240.0 - pEnemyShot->y, 240.0 - pEnemyShot->x);
            pEnemyShot->speed = 0.25;
            pEnemyShot->kind = img_enemyShotLargeBall[2]; // 中楕円弾、緑(2)
            pEnemyShot->margin = 40;
            pEnemyShot->param_i[0] = STATE_BIT;
            // margin はデフォルト(20.0)のまま。画面内で跳ね返るため画面外には出ない

            // リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- ② 干渉レーザーの生成 (30フレームごとに2本) ---
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 30 == 15) {
        // レーザー発射音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            int startPos = GetRand(2); // 0~3の4方向

            if (startPos == 0) { pEnemyShot->x = -60.0; pEnemyShot->y = 90.0 + (double)GetRand(300); pEnemyShot->muki = 0.0; }
            else if (startPos == 1) { pEnemyShot->x = 540.0; pEnemyShot->y = 90.0 + (double)GetRand(300); pEnemyShot->muki = DX_PI; }
            else if (startPos == 2) { pEnemyShot->x = 90.0 + (double)GetRand(300); pEnemyShot->y = -60.0; pEnemyShot->muki = DX_PI / 2.0; }
            else { pEnemyShot->x = 90.0 + (double)GetRand(300); pEnemyShot->y = 540.0; pEnemyShot->muki = -DX_PI / 2.0; }

            pEnemyShot->speed = 4.0; // 超高速で一瞬で画面を横断する
            pEnemyShot->kind = img_enemyShotLaser[5]; // 短レーザー、マゼンタ(5)
            pEnemyShot->margin = 100;
            pEnemyShot->param_i[0] = STATE_LASER;

            // リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 全弾の移動と干渉処理 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 消去保険

        if (pEnemyShotSet->count >= 1200 - 60) {
            pShot->margin = -9999;
            pShot = pNext;
            continue;
        }

        if (pShot->param_i[0] == STATE_NOISE) {
            // ノイズ弾：ゆっくり漂う
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[0] == STATE_LASER) {
            // レーザー：高速移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // ③ カオス連鎖：レーザーがノイズ弾を変異させる
            double lx = cos(pShot->muki);
            double ly = sin(pShot->muki);

            sEnemyShot* pNoise = pEnemyShotSet->pEnemyShotHead->next;
            while (pNoise != pEnemyShotSet->pEnemyShotHead) {
                if (pNoise->param_i[0] == STATE_NOISE) {
                    double rx = pNoise->x - pShot->x;
                    double ry = pNoise->y - pShot->y;

                    // レーザーの進行方向の距離
                    double dot = rx * lx + ry * ly;
                    // レーザーの直交方向の距離
                    double cross = rx * ly - ry * lx;

                    // 短レーザー(64x4)と小玉を加味した矩形判定近似
                    if (dot > -32.0 && dot < 32.0 && cross * cross < 25.0) {
                        pNoise->param_i[0] = STATE_MUTATE_WAIT;
                        pNoise->param_i[1] = 60; // 12フレームのグリッチ遅延
                        pNoise->speed = 0.0;
                        pNoise->kind = img_enemyShotSmallBall[3]; // シアン(3)に変色
                    }
                }
                pNoise = pNoise->next;
            }
        }
        else if (pShot->param_i[0] == STATE_MUTATE_WAIT) {
            // 変異待機：カウントダウン後に発射状態へ遷移
            pShot->param_i[1]--;
            if (pShot->param_i[1] <= 0) {
                pShot->param_i[0] = STATE_MUTATE_FIRE;
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->speed = 5.0 + GetRand(30) / 10.0; // 5.0 ~ 8.0 の高速
                pShot->kind = img_enemyShotMediumBall[3]; // 中玉・シアン(3)にして視認性向上
            }
        }
        else if (pShot->param_i[0] == STATE_MUTATE_FIRE) {
            // 変異弾：プレイヤーに向かって飛ぶ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 跳弾ビットとの当たり判定と反射
            sEnemyShot* pBit = pEnemyShotSet->pEnemyShotHead->next;
            while (pBit != pEnemyShotSet->pEnemyShotHead) {
                if (pBit->param_i[0] == STATE_BIT) {
                    double dx = pShot->x - pBit->x;
                    double dy = pShot->y - pBit->y;
                    // 中楕円と中玉の距離判定
                    if (dx * dx + dy * dy < 45.0 * 45.0 && pShot->param_i[2] == 0) { // 15.0の二乗
                        // 乱数でランダムな方向に反射
                        pShot->muki += DX_PI + (GetRand(60) - 30) / 180.0 * DX_PI;
                        pShot->param_i[2] = 1;
                    }
                }
                pBit = pBit->next;
            }
        }
        else if (pShot->param_i[0] == STATE_BIT) {
            // 跳弾ビット：5秒間は画面中央付近をウロウロし、その後画面外へ退場
            if (pEnemyShotSet->count > 3000) {
                // 画面外へ退場させる（Setの自然消滅を促す）
                pShot->muki = atan2(pShot->y - 240.0, pShot->x - 240.0);
                pShot->speed = 5.0;
            }
            else {
                // 中心に近づいたら反転してウロウロする
                double dx = pShot->x - 240.0;
                double dy = pShot->y - 240.0;
                if (dx * dx + dy * dy < 2500.0) { // 距離50以内
                    pShot->muki = atan2(-dy, -dx) + (GetRand(60) - 30) / 180.0 * DX_PI;
                }
                // 画面端で跳ね返る
                if (pShot->x < 20.0 || pShot->x > 460.0) pShot->muki = DX_PI - pShot->muki;
                if (pShot->y < 20.0 || pShot->y > 460.0) pShot->muki = -pShot->muki;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}


// 敵本体のパターン
void EnemyPat_TooChaotic_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 60フレーム経過後に弾幕Setを1つだけ生成（以後はSet内で完結）
    if (count % 1200 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGlitchCascade;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
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