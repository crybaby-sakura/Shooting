#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：バブルソート・スワップウェーブ
// 画面上部に並んだ弾がバブルソートのように隣同士を比較・交換しながら
// 徐々に整列し、整列完了後に一斉落下するパターン
// ============================================================

// 使える弾画像（サンプルより抜粋）
// 小玉・中玉・大玉・銃弾・鱗弾・菱形弾・中楕円弾・短レーザー
// 色: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
// 効果音: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy,
//         sound_enemyShot_extreme, sound_enemyCharge

static const int BUBBLE_N = 7;              // 配列サイズ（弾の数）
static const double BUBBLE_Y = 70.0;        // 初期配置Y
static const double BUBBLE_SPACING = 48.0;  // 弾間の間隔
static const double BUBBLE_CENTER_X = 240.0;
static const int SWAP_FRAMES = 18/2;          // スワップアニメーションのフレーム数
static const int COMPARE_INTERVAL = 8/2;      // 比較間隔（フレーム）

// 弾の「値」に応じた色を返す（値が大きいほど暖色寄り）
static int ValueToColor(int value)
{
    // 0〜6 を 0:赤,1:橙,2:黄,3:緑,4:シアン,5:青,6:マゼンタ にマッピング
    static const int colorTable[7] = { 0, 8, 1, 2, 3, 4, 5 };
    if (value < 0) value = 0;
    if (value > 6) value = 6;
    return colorTable[value];
}

// バブルソート弾幕のパターン関数
static void ShotBubbleSort(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // -------------------------------------------------------
    // 初期化（count==0 のときだけ弾を生成）
    // -------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // セットのパラメータ初期化
        // param_i[0] : フェーズ (0=ソート中, 1=整列完了待機, 2=落下中)
        // param_i[1] : 現在の比較インデックス i
        // param_i[2] : 現在のパス (0〜N-2)
        // param_i[3] : アニメーション残りフレーム
        // param_i[4] : スワップ中フラグ (1なら2発が移動中)
        // param_i[5] : 次の比較までの待機カウンタ
        // param_i[6] : 整列完了後の待機フレーム
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0;
        pEnemyShotSet->param_i[2] = 0;
        pEnemyShotSet->param_i[3] = 0;
        pEnemyShotSet->param_i[4] = 0;
        pEnemyShotSet->param_i[5] = 20;     // 出現後少し待ってからソート開始
        pEnemyShotSet->param_i[6] = 0;

        // ランダムな順列を生成（0〜N-1 の重複なし）
        int values[BUBBLE_N];
        for (int i = 0; i < BUBBLE_N; i++) values[i] = i;
        // Fisher-Yates シャッフル
        for (int i = BUBBLE_N - 1; i > 0; i--) {
            int j = GetRand(i);             // 0〜i の乱数
            int tmp = values[i];
            values[i] = values[j];
            values[j] = tmp;
        }

        // 弾を横一列に生成
        for (int i = 0; i < BUBBLE_N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = BUBBLE_CENTER_X + (i - (BUBBLE_N - 1) / 2.0) * BUBBLE_SPACING;
            pEnemyShot->y = BUBBLE_Y;
            pEnemyShot->muki = DX_PI / 2.0; // 下向き（後で使う）
            pEnemyShot->speed = 0.0;        // ソート中は自前で動かす

            int col = ValueToColor(values[i]);
            // 中玉を使用（視認性と密度のバランスが良い）
            pEnemyShot->kind = img_enemyShotMediumBall[col];

            // 弾固有パラメータ
            // param_i[0] : 値（ソートキー）
            // param_i[1] : 現在の配列インデックス
            // param_i[2] : スワップ中フラグ
            // param_d[0] : スワップ開始時のx
            // param_d[1] : スワップ目標のx
            // param_d[2] : スワップ開始時のy
            // param_d[3] : 弧の高さ
            pEnemyShot->param_i[0] = values[i];
            pEnemyShot->param_i[1] = i;
            pEnemyShot->param_i[2] = 0;
            pEnemyShot->param_d[0] = pEnemyShot->x;
            pEnemyShot->param_d[1] = pEnemyShot->x;
            pEnemyShot->param_d[2] = pEnemyShot->y;
            pEnemyShot->param_d[3] = 0.0;

            // リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        return;
    }

    // -------------------------------------------------------
    // 毎フレーム処理
    // -------------------------------------------------------
    // 現在の弾をインデックス順に配列へ取り出す
    sEnemyShot* arr[BUBBLE_N] = {};
    int collected = 0;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead && collected < BUBBLE_N) {
        int idx = pShot->param_i[1];
        if (idx >= 0 && idx < BUBBLE_N) {
            arr[idx] = pShot;
            collected++;
        }
        pShot = pShot->next;
    }
    // 万が一数が足りない場合は何もしない
    if (collected < BUBBLE_N) return;

    int phase = pEnemyShotSet->param_i[0];

    // ========== フェーズ0：バブルソート実行 ==========
    if (phase == 0) {
        // スワップアニメーション中
        if (pEnemyShotSet->param_i[4] != 0) {
            int timer = pEnemyShotSet->param_i[3];
            double t = 1.0 - (double)timer / SWAP_FRAMES;   // 0→1
            // イージング（滑らかに）
            double ease = t * t * (3.0 - 2.0 * t);

            for (int k = 0; k < BUBBLE_N; k++) {
                if (arr[k]->param_i[2] != 0) {
                    double sx = arr[k]->param_d[0];
                    double tx = arr[k]->param_d[1];
                    double sy = arr[k]->param_d[2];
                    double arc = arr[k]->param_d[3];
                    arr[k]->x = sx + (tx - sx) * ease;
                    // 弧を描く（sinカーブ）
                    arr[k]->y = sy - sin(ease * DX_PI) * arc;
                }
            }

            pEnemyShotSet->param_i[3]--;
            if (pEnemyShotSet->param_i[3] <= 0) {
                // アニメ終了：位置を確定しフラグを下ろす
                for (int k = 0; k < BUBBLE_N; k++) {
                    if (arr[k]->param_i[2] != 0) {
                        arr[k]->x = arr[k]->param_d[1];
                        arr[k]->y = BUBBLE_Y;
                        arr[k]->param_i[2] = 0;
                    }
                }
                pEnemyShotSet->param_i[4] = 0;
                pEnemyShotSet->param_i[5] = COMPARE_INTERVAL; // 次の比較まで待機
            }
        }
        // 待機中 or 次の比較へ
        else {
            if (pEnemyShotSet->param_i[5] > 0) {
                pEnemyShotSet->param_i[5]--;
            }
            else {
                int i = pEnemyShotSet->param_i[1];
                int pass = pEnemyShotSet->param_i[2];

                // パス終了判定
                if (i >= BUBBLE_N - 1 - pass) {
                    // 次のパスへ
                    pEnemyShotSet->param_i[2]++;
                    pEnemyShotSet->param_i[1] = 0;
                    pass = pEnemyShotSet->param_i[2];

                    if (pass >= BUBBLE_N - 1) {
                        // ソート完了 → フェーズ1へ
                        pEnemyShotSet->param_i[0] = 1;
                        pEnemyShotSet->param_i[6] = 40; // 少し誇示してから落下
                        // 完了音
                        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                        return;
                    }
                    // パスが変わったら少し長めに待つ
                    pEnemyShotSet->param_i[5] = COMPARE_INTERVAL + 6;
                    return;
                }

                // 隣接比較
                sEnemyShot* left = arr[i];
                sEnemyShot* right = arr[i + 1];
                if (left->param_i[0] > right->param_i[0]) {
                    // スワップ発生
                    // インデックスを交換
                    left->param_i[1] = i + 1;
                    right->param_i[1] = i;

                    // アニメーションパラメータ設定
                    double leftTargetX = BUBBLE_CENTER_X + ((i + 1) - (BUBBLE_N - 1) / 2.0) * BUBBLE_SPACING;
                    double rightTargetX = BUBBLE_CENTER_X + (i - (BUBBLE_N - 1) / 2.0) * BUBBLE_SPACING;

                    left->param_d[0] = left->x;
                    left->param_d[1] = leftTargetX;
                    left->param_d[2] = left->y;
                    left->param_d[3] = 18.0 + GetRand(8);   // 弧の高さに少しランダム
                    left->param_i[2] = 1;

                    right->param_d[0] = right->x;
                    right->param_d[1] = rightTargetX;
                    right->param_d[2] = right->y;
                    right->param_d[3] = 12.0 + GetRand(6);
                    right->param_i[2] = 1;

                    pEnemyShotSet->param_i[3] = SWAP_FRAMES;
                    pEnemyShotSet->param_i[4] = 1;

                    // スワップ音
                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }
                else {
                    // 交換なし。次の比較へすぐ進むための短い待機
                    pEnemyShotSet->param_i[5] = COMPARE_INTERVAL / 2;
                }

                // 次のインデックスへ
                pEnemyShotSet->param_i[1]++;
            }
        }
    }
    // ========== フェーズ1：整列完了後の待機 ==========
    else if (phase == 1) {
        if (pEnemyShotSet->param_i[6] > 0) {
            pEnemyShotSet->param_i[6]--;
            // 待機中はわずかに上下に揺れる演出
            for (int k = 0; k < BUBBLE_N; k++) {
                arr[k]->y = BUBBLE_Y + sin((pEnemyShotSet->count + k * 8) * 0.15) * 3.0;
            }
        }
        else {
            // 落下開始
            pEnemyShotSet->param_i[0] = 2;
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            for (int k = 0; k < BUBBLE_N; k++) {
                // 少し外側に広がるように角度を付ける
                double spread = (k - (BUBBLE_N - 1) / 2.0) * 0.08;
                arr[k]->muki = DX_PI / 2.0 + spread;
                arr[k]->speed = 2.8 + GetRand(40) / 100.0;
                arr[k]->param_d[4] = 0.0; // サイン波用位相
            }
        }
    }
    // ========== フェーズ2：落下 ==========
    else if (phase == 2) {
        for (int k = 0; k < BUBBLE_N; k++) {
            // 基本移動
            arr[k]->x += arr[k]->speed * cos(arr[k]->muki);
            arr[k]->y += arr[k]->speed * sin(arr[k]->muki);

            // 微細なサイン波を加えて単調さを消す
            arr[k]->param_d[4] += 0.12;
            arr[k]->x += sin(arr[k]->param_d[4]) * 0.7;

            // 徐々に加速
            arr[k]->speed += 0.015;
        }
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_BubbleSort_Grok()
{
    static int muki;
    static int wave;          // 発生させたウェーブ数
    static int nextWaveTime;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        wave = 0;
        nextWaveTime = 30;    // 最初の弾幕発生タイミング
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;
    }

    // 一定間隔でバブルソート弾幕を発生（最大2ウェーブ程度）
    if (wave < 999 && count >= nextWaveTime) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBubbleSort;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = wave;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        wave++;
        nextWaveTime = count + 280; // 次のウェーブまでの間隔（ソート＋落下を考慮）
    }
}