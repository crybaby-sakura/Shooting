// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 撃ち返し弾幕：瞬停分裂弾
static void ShotStopSplit(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 初期弾の発射音（軽めの音）
        if (enemy.hp % 2 == 1) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        sEnemyShot* pEnemyShot = new sEnemyShot;

        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        // プレイヤーの現在位置に向けて発射
        pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
        pEnemyShot->speed = 2.0; // ゆっくりとした速度

        // 素材選定：目立つように「大玉」の「マゼンタ(5)」を使用
        pEnemyShot->kind = img_enemyShotLargeBall[5];

        // param_i[0] を状態管理に使用 (0:飛行中, 1:停止中, 2:消滅待ち)
        pEnemyShot->param_i[0] = 0;
        pEnemyShot->param_i[1] = enemy.hp;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        if (pShot->param_i[0] == 0) {
            // 状態0：飛行中
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 20フレーム経過で停止状態へ遷移
            if (pShot->count >= 20) {
                pShot->param_i[0] = 1;
                // 停止位置を固定（浮動小数点の誤差で微妙に滑るのを防ぐ）
                pShot->param_d[0] = pShot->x;
                pShot->param_d[1] = pShot->y;
            }
        }
        else if (pShot->param_i[0] == 1) {
            // 状態1：停止中
            pShot->x = pShot->param_d[0];
            pShot->y = pShot->param_d[1];

            // 停止してから30フレーム後（発射から50フレーム後）に分裂
            if (pShot->count >= 50) {
                // 分裂時の発射音（中程度の音）
                if (pShot->param_i[1] % 2 == 1) {
                    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                }

                int num_ways = 7; // 7方向に分裂
                double base_angle = pShot->muki; // 元の進行方向を中心にする

                // 連続ヒット数に応じて変化した角度間隔を取得（初期値は DX_PI / 12.0）
                double angle_space = (pEnemyShotSet->param_d[0] != 0.0) ? pEnemyShotSet->param_d[0] : DX_PI / 12.0;

                for (int i = 0; i < num_ways; i++) {
                    sEnemyShot* pNewShot = new sEnemyShot;

                    pNewShot->x = pShot->x;
                    pNewShot->y = pShot->y;
                    pNewShot->muki = base_angle + (i - (num_ways - 1) / 2.0) * angle_space;
                    pNewShot->speed = 4.5; // 分裂後は少し速めに設定

                    // 素材選定：通常弾と被らないよう「中玉」の「橙(8)」を使用
                    pNewShot->kind = img_enemyShotMediumBall[8];

                    // 分裂弾は通常移動させるため、状態を -1 に設定
                    pNewShot->param_i[0] = -1;

                    // リストに追加
                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }

                // 親弾（大玉）は画面外へ飛ばしてメインルーチンの自動消去に任せる
                pShot->x = -1000.0;
                pShot->y = -1000.0;
                pShot->param_i[0] = 2; // 消滅待ち状態へ
            }
        }
        else if (pShot->param_i[0] == 2) {
            // 状態2：消滅待ち（画面外にいるため、メインルーチンで自動消去される）
        }
        else {
            // 状態-1：分裂後の通常弾の移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Counter_Zai()
{
    static int muki;
    static int prev_hp;               // 前フレームのHPを保持用
    static int return_shot_cooltime;  // 撃ち返しのクールダウン用
    static int combo_count;           // 連続ヒット回数
    static int combo_reset_timer;     // 連続ヒット途切れ検知用タイマー

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        prev_hp = enemy.hp;
        return_shot_cooltime = 0;
        combo_count = 0;
        combo_reset_timer = 0;
    }
    else {
        // サンプルと同じ左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;

        // クールダウンをカウントダウン
        if (return_shot_cooltime > 0) return_shot_cooltime--;

        // 連続ヒット途切れの監視
        if (combo_reset_timer > 0) {
            combo_reset_timer--;
            if (combo_reset_timer == 0) {
                combo_count = 0; // 一定時間ダメージが無ければ連続ヒットをリセット
            }
        }

        // ダメージを受けていないかチェック（初期化直後の誤発動を防ぐため count > 2）
        if (count > 2 && enemy.hp < prev_hp && return_shot_cooltime == 0) {

            combo_count++;
            combo_reset_timer = 30; // 60フレームダメージが無ければ連続ヒット解除

            // 角度間隔の計算 
            // 初期値 DX_PI/12.0 (15度) から、連続ヒットごとに DX_PI/60.0 (3度) ずつ狭める
            double current_angle_space = DX_PI / 12.0 - (combo_count - 1) * (DX_PI / 180.0 / 3);
            if (current_angle_space < DX_PI / 60.0) {
                current_angle_space = DX_PI / 60.0; // 最小値の保証(3度)
            }

            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotStopSplit;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;
            pEnemyShotSet->muki = 0.0; // ShotStopSplit内でプレイヤー方向を計算するためダミー値
            pEnemyShotSet->kind = 0;
            pEnemyShotSet->param_d[0] = current_angle_space; // 計算した間隔をセット

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;

            // 連続ヒットで画面が埋め尽くされないよう、30フレームのクールダウンを設定
            //return_shot_cooltime = 30;
        }

        // HPを記録
        prev_hp = enemy.hp;
    }
}