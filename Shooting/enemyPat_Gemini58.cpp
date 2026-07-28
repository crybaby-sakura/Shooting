// enemyPat_antlion.cpp
// 弾幕名：『流砂の地獄陣（アントリオン・トラップ）』

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// --- 【すり鉢の砂流】(外周からの渦巻き収束弾) ---
static void ShotConverge(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int way = 16;
        for (int i = 0; i < way; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            // 画面外周付近(半径350)に円形に配置
            double r = 350.0;
            double angle = pEnemyShotSet->muki + (DX_PI * 2.0 / way) * i;
            pShot->x = pEnemyShotSet->x + r * cos(angle);
            pShot->y = pEnemyShotSet->y + r * sin(angle);

            // 初期角度はとりあえずボス方向に向ける（後で毎フレーム補正）
            pShot->muki = angle + DX_PI;
            pShot->speed = 1.0;

            // 砂粒を表現：黄色(1)の小玉
            pShot->kind = img_enemyShotSmallBall[1];

            pShot->margin = 480;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 中心(ボス)との距離
        double dist = sqrt((pEnemyShotSet->x - pShot->x) * (pEnemyShotSet->x - pShot->x) +
            (pEnemyShotSet->y - pShot->y) * (pEnemyShotSet->y - pShot->y));

        // ボスから15ピクセル以上離れている場合は渦を巻きながら吸い込まれる
        if (dist > 15.0) {
            double angleToBoss = atan2(pEnemyShotSet->y - pShot->y, pEnemyShotSet->x - pShot->x);
            // ボス方向から0.7ラジアンずらすことで螺旋状の軌道にする
            pShot->muki = angleToBoss + 0.7;
        }

        // 徐々に加速して流砂の勢いを表現
        if (pShot->speed < 3.5) pShot->speed += 0.015;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// --- 【砂かけ噴出】(中央からの反転拡散弾) ---
static void ShotSplash(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int way = 16;
        for (int i = 0; i < way; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = pEnemyShotSet->muki + (DX_PI * 2.0 / way) * i;
            pShot->speed = 1.0;

            // 砂の塊を表現：橙(8)の鱗弾
            pShot->kind = img_enemyShotScale[8];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 最初は遅く、吹き飛ばされた後に加速するような緩急
        if (pEnemyShotSet->count < 40) {
            pShot->speed += 0.05;
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// --- 【大顎の挟撃】(V字レーザー) ---
static void ShotPincer(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 左右からのV字挟撃 (2本)
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;

            // i=0なら右寄り、i=1なら左寄りにずらす
            double offset = (i == 0) ? 0.35 : -0.35;
            pShot->muki = pEnemyShotSet->muki + offset;
            pShot->speed = 8.0; // 大顎による高速な一撃

            // 鋭い顎を表現：赤(0)の短レーザー
            pShot->kind = img_enemyShotLaser[0];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// --- 敵本体のパターン ---
void EnemyPat_Antlion_Gemini()
{
    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 200.0; // すり鉢の中心となる位置に陣取る
        enemy.maxHp = enemy.hp = 200;
    }

    // 【流砂の引力】: 毎フレーム自機をボス方向に引き寄せる
    double distToEnemy = sqrt((enemy.x - player.x) * (enemy.x - player.x) + (enemy.y - player.y) * (enemy.y - player.y));
    if (distToEnemy > 15.0) { // 中心に近すぎると荒ぶるため安全マージンを取る
        double angleToEnemy = atan2(enemy.y - player.y, enemy.x - player.x);
        double pullSpeed = 1.0; // 引き寄せの強さ
        player.x += pullSpeed * cos(angleToEnemy);
        player.y += pullSpeed * sin(angleToEnemy);
        spawnForceParticles(player.x, player.y, pullSpeed * cos(angleToEnemy) * 3, pullSpeed * sin(angleToEnemy) * 3);
    }

    // 周期管理 (300フレーム＝約5秒で1ループ)
    int t = count % 300;

    // 【すり鉢の砂流】: 0〜200Fの間、10F間隔で外周から収束弾を発生
    if (t < 200 && t % 10 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotConverge;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        // 初期配置のためのランダムな基準角度
        pSet->muki = (GetRand(360) / 180.0) * DX_PI;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 【砂かけ噴出】: 0〜200Fの間、50F間隔で中央から拡散弾を発生
    if (t > 0 && t < 200 && t % 50 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSplash;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        // 放射状の基準角度。自機方向ベースで撃つ。
        pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 【大顎の挟撃】: 240Fでチャージ(予告)、270FでV字レーザー発射
    if (t == 240) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (t == 270) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPincer;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        // 発射時点での自機狙い
        pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}