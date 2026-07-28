// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// 弾幕：落とし穴の形成（斜面弾幕）
static void ShotPitfall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double base_angle = DX_PI * 2.0 / 30.0;
        for (int ring = 0; ring < 3; ring++) {
            int num_shots = 18 + ring * 6; // 18, 24, 30
            double offset_angle = ring * (base_angle / 2.0);
            // すり鉢状に見せるため、最初から半径をずらして配置する
            double start_dist = 20.0 + ring * 30.0;

            for (int i = 0; i < num_shots; i++) {
                pEnemyShot = new sEnemyShot;
                double angle = base_angle * i + offset_angle;
                pEnemyShot->x = pEnemyShotSet->x + start_dist * cos(angle);
                pEnemyShot->y = pEnemyShotSet->y + start_dist * sin(angle);
                pEnemyShot->muki = angle;
                pEnemyShot->speed = 0.8;

                // 橙(8)の中玉で砂の斜面を表現
                pEnemyShot->kind = img_enemyShotMediumBall[8];

                pEnemyShot->margin = 480;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // アリジゴクの落とし穴に滑り落ちる演出（プレイヤーを中心に引き寄せる）
    if (pEnemyShotSet->count < 300) {
        double dx = (pEnemyShotSet->x - player.x) * 0.008;
        double dy = (pEnemyShotSet->y - player.y) * 0.005;
        player.x += dx;
        player.y += dy;
        spawnForceParticles(player.x, player.y, dx, dy);
        if (player.x < 10.0) player.x = 10.0;
        if (player.x > 470.0) player.x = 470.0;
        if (player.y < 10.0) player.y = 10.0;
        if (player.y > 470.0) player.y = 470.0;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 弾幕：砂かけ（狙い撃ち）
static void ShotSandThrow(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    // 60フレームの間、3フレームに1回発射
    if (pEnemyShotSet->count < 60 && pEnemyShotSet->count % 3 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double aim = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(20) - 10;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;
            // GetRand(60) - 30 で -30 ～ 30 の61種類のばらつき
            pEnemyShot->muki = aim + (GetRand(60) - 30) / 180.0 * DX_PI;
            // GetRand(30) で 0 ～ 30 のため、6.0 ～ 9.0 の速度
            pEnemyShot->speed = 6.0 + GetRand(30) / 10.0;

            // 白(6)の小玉で砂粒を表現
            pEnemyShot->kind = img_enemyShotSmallBall[6];

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

// 弾幕：大顎の捕食（挟み撃ち弾幕）
static void ShotMandibles(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 予告音

        double aim = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double left_base = aim - 0.5;
        double right_base = aim + 0.5;

        // 左顎生成（赤の菱形弾を20個連結）
        for (int i = 0; i < 20; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->param_d[0] = 15.0 + i * 8.0; // 中心からの距離
            pEnemyShot->param_d[1] = left_base;       // ベース角度
            pEnemyShot->param_d[2] = 1.0;             // 回転方向（左：正転）
            pEnemyShot->speed = 0.0;

            // 赤(0)の菱形弾で鋭角な大顎を表現
            pEnemyShot->kind = img_enemyShotDiamond[0];

            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 右顎生成
        for (int i = 0; i < 20; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->param_d[0] = 15.0 + i * 8.0;
            pEnemyShot->param_d[1] = right_base;
            pEnemyShot->param_d[2] = -1.0;            // 回転方向（右：逆転）
            pEnemyShot->speed = 0.0;

            pEnemyShot->kind = img_enemyShotDiamond[0];

            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        pEnemyShotSet->param_d[0] = 0.0; // 回転オフセットの初期化
    }

    // 閉じる瞬間の重い音
    if (pEnemyShotSet->count == 30) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count < 100) {
        // 閉じる動作（剛体回転）
        pEnemyShotSet->param_d[0] += 0.005;

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            double angle = pShot->param_d[1] + pEnemyShotSet->param_d[0] * pShot->param_d[2];
            pShot->x = pEnemyShotSet->x + pShot->param_d[0] * cos(angle);
            pShot->y = pEnemyShotSet->y + pShot->param_d[0] * sin(angle);
            pShot->muki = angle;
            pShot = pShot->next;
        }
    }
    else {
        // 閉じきった後、バラバラに飛んで消える
        if (pEnemyShotSet->count == 100) {
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                // 先端ほど速く飛ばすことで弧を描いて散る演出に
                pShot->speed = 3.0 + (pShot->param_d[0] / 175.0) * 2.0;
                pShot = pShot->next;
            }
        }

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Antlion_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 落とし穴の底としてゆっくり左右に揺れる
        enemy.x = 240.0 + sin(count * 0.02) * 50.0;
    }

    // 300フレーム（5秒）で1サイクル
    int cycle = count % 300;

    // 第1フェーズ：落とし穴の形成
    if (cycle == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPitfall;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
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

    // 第2フェーズ：砂かけ
    if (cycle == 121) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSandThrow;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
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

    // 第3フェーズ：大顎の捕食
    if (cycle == 181) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMandibles;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
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
}