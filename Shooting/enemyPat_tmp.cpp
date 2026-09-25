#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------------------------------------------------------
// 敵弾パターン関数：時間停止対応弾
// ---------------------------------------------------------------------------
static void ShotTimeStopPattern(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // pEnemyShotSet->count == 0 の時に新しい弾を生成（インクリメントはメインルーチン側で行われる）
    if (pEnemyShotSet->count == 0) {
        // 弾発射効果音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 1回の発射セットで放射状に配置する弾数（6方向）
        int way = 6;
        for (int i = 0; i < way; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (DX_PI * 2.0 / way) * i;
            pEnemyShot->speed = 2.0 + (GetRand(150) / 100.0); // 速度 2.0 ～ 3.5

            // ナイフや時間停止の演出に合った弾種（鱗弾・菱形弾・中楕円弾）を選択
            int color = pEnemyShotSet->kind % 8; // 0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒
            int bulletType = (pEnemyShotSet->kind / 8) % 3;

            switch (bulletType) {
            case 0:
                pEnemyShot->kind = img_enemyShotScale[color];      // 鱗弾（ナイフ風）
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotDiamond[color];    // 菱形弾
                break;
            case 2:
                pEnemyShot->kind = img_enemyShotMediumOval[color]; // 中楕円弾
                break;
            }

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 1周期240フレーム中、60～239フレーム間（3秒間）が時間停止状態
    int cycle = (count - 1) % 420;
    bool isTimeStopped = (cycle >= 60 && cycle < 240);

    // 時間停止中でない場合のみ弾の移動処理を行う（停止中は移動処理を行わずその場に停滞）
    if (!isTimeStopped) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
}

// ---------------------------------------------------------------------------
// 敵本体のパターン関数
// ---------------------------------------------------------------------------
void EnemyPat_Tmp()
{
    static double stop_player_x = 0.0;
    static double stop_player_y = 0.0;
    static int shot_count = 0;

    // 初期化処理
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;
        stop_player_x = player.x;
        stop_player_y = player.y;
        shot_count = 0;
    }

    // 1周期 = 240フレーム (4秒)
    //   0 ～  59 : 予告フェーズ (1秒 = 60フレーム)
    //  60 ～ 239 : 時間停止フェーズ (3秒 = 180フレーム)
    int cycle = (count - 1) % 420;
    bool isTimeStopped = (cycle >= 60 && cycle < 240);

    // 1. 周期開始時(cycle == 0)に予告音を再生
    if (cycle == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 2. 時間停止に入る直前(cycle == 59)の自機座標を保存
    if (cycle == 59) {
        stop_player_x = player.x;
        stop_player_y = player.y;
    }

    // 3. 時間停止状態中の制御
    if (isTimeStopped) {
        // (a) 自機を直前位置に固定して移動不能にする
        player.x = stop_player_x;
        player.y = stop_player_y;

        // (b) 自機ショットの y 座標を +15.0 してメインルーチンの -15.0 移動を相殺
        for (sPlayerShot* pShot = playerShotHead.next; pShot != &playerShotHead; pShot = pShot->next) {
            pShot->y += 15.0;
        }

        // (c) ボスがダイナミックに動き回る（8の字移動）
        double t = (double)(cycle - 60) / 180.0; // 時間停止内の進捗率 (0.0 ～ 1.0)
        enemy.x = 240.0 + 150.0 * sin(t * DX_PI * 4.0);
        enemy.y = 120.0 + 50.0 * cos(t * DX_PI * 2.0);

        // (d) ボスが移動しながら連続で敵弾を設置（3フレームごと）
        if (cycle % 3 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotTimeStopPattern;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            // 停止している自機への方向を軸にランダムな角度誤差を付与
            pEnemyShotSet->muki = atan2(stop_player_y - enemy.y, stop_player_x - enemy.x)
                + (GetRand(60) - 30) / 180.0 * DX_PI;
            pEnemyShotSet->kind = shot_count++;

            // ダミーヘッド初期化
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            // enemyShotSetHead リストへ追加
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    else {
        // 通常状態（予告中）：ボスは中央付近へ滑らかに移動して準備
        enemy.x += (240.0 - enemy.x) * 0.1;
        enemy.y += (120.0 - enemy.y) * 0.1;
    }
}