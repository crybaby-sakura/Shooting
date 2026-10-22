#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：ニューラルネットワーク（隠れ層の発火）
// ============================================================
static void ShotNeuralNetwork(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 【初期化】ネットワーク構造の構築
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // ニューロン配置座標 (3列 x 4行)
        double neuronX[3] = { 120.0-40, 240.0, 360.0+40 };
        double neuronY[4] = { 120.0, 200.0, 280.0, 360.0 };
        double nx[12], ny[12];

        // 1. ニューロン（細胞体）と装飾の生成
        for (int c = 0; c < 3; c++) {
            for (int r = 0; r < 4; r++) {
                int idx = c * 4 + r;
                nx[idx] = neuronX[c];
                ny[idx] = neuronY[r];

                // 細胞体本体
                sEnemyShot* pNeuron = new sEnemyShot;
                pNeuron->x = nx[idx];
                pNeuron->y = ny[idx];
                pNeuron->muki = 0.0;
                pNeuron->speed = 0.0;
                pNeuron->kind = img_enemyShotMediumBall[4]; // 青い中玉
                pNeuron->param_i[0] = 0; // 0: ニューロン
                pNeuron->param_i[1] = 0; // 発火状態タイマー
                pNeuron->param_i[2] = idx; // 自身のID

                pNeuron->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNeuron->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNeuron;
                pEnemyShotSet->pEnemyShotHead->prev = pNeuron;

                // 細胞体装飾（回転する鱗弾）
                for (int k = 0; k < 4; k++) {
                    sEnemyShot* pOrbit = new sEnemyShot;
                    pOrbit->x = nx[idx];
                    pOrbit->y = ny[idx];
                    pOrbit->kind = img_enemyShotScale[3]; // シアンの鱗弾
                    pOrbit->param_i[0] = 5; // 5: ニューロン装飾
                    pOrbit->param_i[1] = idx; // 親ニューロンID
                    pOrbit->param_d[0] = (k * 90) / 180.0 * DX_PI; // 初期角度

                    pOrbit->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pOrbit->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pOrbit;
                    pEnemyShotSet->pEnemyShotHead->prev = pOrbit;
                }
            }
        }

        // 2. シナプス（神経線維）と活動電位の生成
        for (int c = 0; c < 2; c++) {
            for (int r1 = 0; r1 < 4; r1++) {
                int idx1 = c * 4 + r1;
                // 次の層のランダムな2つのニューロンに接続
                for (int i = 0; i < 2; i++) {
                    int r2 = GetRand(3); // 0〜3のランダム
                    int idx2 = (c + 1) * 4 + r2;

                    double dx = nx[idx2] - nx[idx1];
                    double dy = ny[idx2] - ny[idx1];
                    double dist = sqrt(dx * dx + dy * dy);
                    double muki = atan2(dy, dx);

                    // シナプス表現：銃弾を間隔16pxで並べて点線にする
                    int numBullets = (int)(dist / 16.0);
                    for (int j = 1; j < numBullets; j++) {
                        sEnemyShot* pSynapse = new sEnemyShot;
                        pSynapse->x = nx[idx1] + (dx / (double)numBullets) * j;
                        pSynapse->y = ny[idx1] + (dy / (double)numBullets) * j;
                        pSynapse->muki = muki;
                        pSynapse->speed = 0.0;
                        pSynapse->kind = img_enemyShotBullet[4]; // 青い銃弾
                        pSynapse->param_i[0] = 1; // 1: シナプス
                        pSynapse->param_i[1] = idx1; // 起点ニューロンID
                        pSynapse->param_i[2] = idx2; // 終点ニューロンID
                        pSynapse->param_i[3] = 0; // 0: 正常, 1: ドロップアウト(消滅済), 2: 重み更新済

                        // 終点座標を記録（活動電位の到達判定用）
                        pSynapse->param_d[0] = nx[idx2];
                        pSynapse->param_d[1] = ny[idx2];

                        pSynapse->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pSynapse->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pSynapse;
                        pEnemyShotSet->pEnemyShotHead->prev = pSynapse;
                    }

                    // 活動電位（信号）：起点から移動開始
                    sEnemyShot* pSignal = new sEnemyShot;
                    pSignal->x = nx[idx1];
                    pSignal->y = ny[idx1];
                    pSignal->muki = muki;
                    pSignal->speed = 3.0;
                    pSignal->kind = img_enemyShotSmallBall[6]; // 白い小玉
                    pSignal->param_i[0] = 2; // 2: 活動電位
                    pSignal->param_i[1] = idx1;
                    pSignal->param_i[2] = idx2;
                    pSignal->param_i[3] = 0; // 0: 正常伝播, 1: ノイズ化
                    pSignal->param_d[0] = nx[idx2]; // 終点X
                    pSignal->param_d[1] = ny[idx2]; // 終点Y

                    pSignal->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pSignal->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pSignal;
                    pEnemyShotSet->pEnemyShotHead->prev = pSignal;
                }
            }
        }
    }
    else {
        // 【毎フレーム更新】

        // ギミックA: ドロップアウト (約3秒ごとにランダムなシナプスを切断)
        if (pEnemyShotSet->count % 180 == 90) {
            sEnemyShot* pSyn = pEnemyShotSet->pEnemyShotHead->next;
            while (pSyn != pEnemyShotSet->pEnemyShotHead) {
                if (pSyn->param_i[0] == 1 && pSyn->param_i[3] == 0) {
                    if (GetRand(15) == 0) { // 約6%の確率でドロップアウト発動
                        pSyn->param_i[3] = 1; // 消滅マーク

                        // このシナプス上にある活動電位を「ノイズ弾」に変化させる
                        sEnemyShot* pSig = pEnemyShotSet->pEnemyShotHead->next;
                        while (pSig != pEnemyShotSet->pEnemyShotHead) {
                            if (pSig->param_i[0] == 2 && (int)pSig->param_i[1] == pSyn->param_i[1] && pSig->param_i[3] == 0) {
                                pSig->param_i[0] = 3; // 3: ノイズ弾化
                                pSig->kind = img_enemyShotDiamond[7]; // 黒い菱形弾
                                pSig->param_d[2] = (GetRand(360) / 180.0 * DX_PI); // ランダムな移動方向
                                pSig->speed = 2.0;
                            }
                            pSig = pSig->next;
                        }

                        // シナプス自体をリストから削除
                        pSyn->prev->next = pSyn->next;
                        pSyn->next->prev = pSyn->prev;
                        delete pSyn;

                        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                        break; // 1フレームで1つだけ処理
                    }
                }
                pSyn = pSyn->next;
            }
        }

        // ギミックB: 誤差逆伝播 (5秒後に出力層から入力層へ赤い信号が逆走)
        if (pEnemyShotSet->count == 300) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            int startR = GetRand(3);
            int startIdx = 2 * 4 + startR; // 出力層のランダムなニューロン

            sEnemyShot* pBackProp = new sEnemyShot;
            sEnemyShot* pNeuron = pEnemyShotSet->pEnemyShotHead->next;
            while (pNeuron != pEnemyShotSet->pEnemyShotHead) {
                if (pNeuron->param_i[0] == 0 && pNeuron->param_i[2] == startIdx) {
                    pBackProp->x = pNeuron->x;
                    pBackProp->y = pNeuron->y;
                    break;
                }
                pNeuron = pNeuron->next;
            }

            pBackProp->muki = DX_PI; // 左向きに逆走
            pBackProp->speed = 4.5;
            pBackProp->kind = img_enemyShotSmallBall[0]; // 赤い小玉
            pBackProp->param_i[0] = 4; // 4: 誤差逆伝播弾

            pBackProp->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBackProp->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBackProp;
            pEnemyShotSet->pEnemyShotHead->prev = pBackProp;
        }

        // 全弾の更新処理
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* nextShot = pShot->next; // 削除時に備えて退避

            if (pShot->param_i[0] == 0) {
                // ニューロン：発火処理
                if (pShot->param_i[1] > 0) {
                    if (pShot->param_i[1] == 1) {
                        // 発火瞬間：自機狙い弾を散布
                        for (int i = 0; i < 3; i++) {
                            sEnemyShot* pFire = new sEnemyShot;
                            pFire->x = pShot->x;
                            pFire->y = pShot->y;
                            pFire->muki = atan2(player.y - pShot->y, player.x - pShot->x) + (GetRand(40) - 20) / 180.0 * DX_PI;
                            pFire->speed = 2.5;
                            pFire->kind = img_enemyShotMediumOval[1]; // 黄色の中楕円弾
                            pFire->param_i[0] = 99; // 汎用移動弾

                            pFire->prev = pEnemyShotSet->pEnemyShotHead->prev;
                            pFire->next = pEnemyShotSet->pEnemyShotHead;
                            pEnemyShotSet->pEnemyShotHead->prev->next = pFire;
                            pEnemyShotSet->pEnemyShotHead->prev = pFire;
                        }
                        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                    }
                    pShot->param_i[1]++;
                    if (pShot->param_i[1] > 30) pShot->param_i[1] = 0; // 発火状態リセット
                }
            }
            else if (pShot->param_i[0] == 1) {
                // シナプス：ドロップアウトで消滅マークが付いていればリストから削除
                if (pShot->param_i[3] == 1) {
                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                }
            }
            else if (pShot->param_i[0] == 2) {
                // 活動電位：移動と到達判定
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);

                double dx = pShot->param_d[0] - pShot->x;
                double dy = pShot->param_d[1] - pShot->y;
                if (dx * dx + dy * dy < 100.0) { // 終点に到達（距離10px以内）
                    // 終点ニューロンを探して発火フラグを立てる
                    sEnemyShot* pTarget = pEnemyShotSet->pEnemyShotHead->next;
                    while (pTarget != pEnemyShotSet->pEnemyShotHead) {
                        if (pTarget->param_i[0] == 0 && pTarget->param_i[2] == (int)pShot->param_i[2]) {
                            pTarget->param_i[1] = 1; // 発火開始
                            break;
                        }
                        pTarget = pTarget->next;
                    }
                    // 活動電位は役目を終えたので消去
                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                }
            }
            else if (pShot->param_i[0] == 3) {
                // ノイズ弾（ドロップアウトにより暴走した信号）
                pShot->x += pShot->speed * cos(pShot->param_d[2]);
                pShot->y += pShot->speed * sin(pShot->param_d[2]);
                // 画面端でバウンド
                if (pShot->x < 10.0 || pShot->x > 470.0) pShot->param_d[2] = DX_PI - pShot->param_d[2];
                if (pShot->y < 10.0 || pShot->y > 470.0) pShot->param_d[2] = -pShot->param_d[2];
            }
            else if (pShot->param_i[0] == 4) {
                // 誤差逆伝播弾
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);

                // 経路上のシナプスの「重み（色）」を更新
                sEnemyShot* pSyn = pEnemyShotSet->pEnemyShotHead->next;
                while (pSyn != pEnemyShotSet->pEnemyShotHead) {
                    if (pSyn->param_i[0] == 1 && pSyn->param_i[3] == 0) {
                        double sdx = pSyn->x - pShot->x;
                        double sdy = pSyn->y - pShot->y;
                        if (sdx * sdx + sdy * sdy < 400.0) { // 距離20px以内で衝突判定
                            pSyn->kind = img_enemyShotBullet[0]; // 赤い銃弾へ変更（重み更新）
                            pSyn->param_i[3] = 2; // 更新済みフラグ

                            // 更新されたシナプスから、新しい信号（赤）を発生させて学習結果を可視化
                            sEnemyShot* pNewSignal = new sEnemyShot;
                            pNewSignal->x = pSyn->x;
                            pNewSignal->y = pSyn->y;
                            pNewSignal->muki = pSyn->muki;
                            pNewSignal->speed = 3.5;
                            pNewSignal->kind = img_enemyShotSmallBall[0]; // 赤い小玉
                            pNewSignal->param_i[0] = 2;
                            pNewSignal->param_i[1] = pSyn->param_i[1];
                            pNewSignal->param_i[2] = pSyn->param_i[2];
                            pNewSignal->param_d[0] = pSyn->param_d[0];
                            pNewSignal->param_d[1] = pSyn->param_d[1];

                            pNewSignal->prev = pEnemyShotSet->pEnemyShotHead->prev;
                            pNewSignal->next = pEnemyShotSet->pEnemyShotHead;
                            pEnemyShotSet->pEnemyShotHead->prev->next = pNewSignal;
                            pEnemyShotSet->pEnemyShotHead->prev = pNewSignal;
                        }
                    }
                    pSyn = pSyn->next;
                }

                // 画面外に出たら消去
                if (pShot->x < -50.0) {
                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                }
            }
            else if (pShot->param_i[0] == 5) {
                // ニューロン装飾：親の周りを回転
                double px = pShot->x, py = pShot->y;
                sEnemyShot* pParent = pEnemyShotSet->pEnemyShotHead->next;
                while (pParent != pEnemyShotSet->pEnemyShotHead) {
                    if (pParent->param_i[0] == 0 && pParent->param_i[2] == pShot->param_i[1]) {
                        px = pParent->x;
                        py = pParent->y;
                        break;
                    }
                    pParent = pParent->next;
                }
                pShot->param_d[0] += 0.08; // 回転速度
                pShot->x = px + cos(pShot->param_d[0]) * 14.0;
                pShot->y = py + sin(pShot->param_d[0]) * 14.0;
            }
            else if (pShot->param_i[0] == 99) {
                // 汎用移動弾（発火時に撒かれた弾など）
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }

            if (pEnemyShotSet->count >= 600) pShot->margin = -999;

            pShot = nextShot;
        }
    }
}

// ============================================================
// 敵本体のパターンエントリーポイント
// ============================================================
void EnemyPat_NeuralNetwork_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵の左右移動
        enemy.x += 0.8 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // 60フレーム目にネットワーク弾幕を初期化
    if (count % 600 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNeuralNetwork;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // パターンをループさせる場合のリセット処理（約10秒周期）
    //if (count > 660) {
    //    count = 59; // 60フレーム目に巻き戻して再構築
    //}
}