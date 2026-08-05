// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：崩星の重核
static void ShotHugeCore(sEnemyShotSet* pEnemyShotSet)
{
    // === 巨大弾の中心座標の管理 ===
    // 巨大弾の各パーツは個別に動くため、波紋弾の発生場所として中心座標を追跡します
    if (pEnemyShotSet->count < 120) {
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y + (double)pEnemyShotSet->count * 1.0;
    }
    else if (pEnemyShotSet->count == 120) {
        // 停止した時の座標を固定
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y + 120.0;
    }

    // === ① 圧迫と誘導（前半） ===
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 超巨大弾の外縁を形成（大玉をリング状に24個配置）
        int numRings = 24;
        double radius = 70.0; // 巨大弾の半径
        for (int i = 0; i < numRings; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double angle = (2.0 * DX_PI / numRings) * i;
            pEnemyShot->x = pEnemyShotSet->x + radius * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + radius * sin(angle);
            pEnemyShot->muki = DX_PI / 2.0; // 真下に落下
            pEnemyShot->speed = 1.0;        // ゆっくりとした速度
            pEnemyShot->kind = img_enemyShotLargeBall[8]; // 橙色の大玉(20.0x20.0)で巨大感を演出
            pEnemyShot->param_i[0] = 0;     // 0:巨大弾パーツ

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 巨大弾が落下している間、プレイヤーを誘導する針弾を発射
    if (pEnemyShotSet->count < 120 && pEnemyShotSet->count % 5 == 0) {
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(20) - 10;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x) + (GetRand(30) - 15) / 180.0 * DX_PI;
            pEnemyShot->speed = 4.0 + (GetRand(10) / 10.0);
            pEnemyShot->kind = img_enemyShotBullet[6]; // 白い銃弾(5.0x2.0)
            pEnemyShot->param_i[0] = 2; // 2:針弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // === ② 脈動と波紋（中盤） ===
    // 停止後に3回脈動し、同心円状の波紋を展開
    if (pEnemyShotSet->count >= 150 && pEnemyShotSet->count <= 210 && pEnemyShotSet->count % 30 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int numWave = 36;
        for (int i = 0; i < numWave; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            // 発生場所を巨大弾の「中心」にすることで、外縁の大玉をすり抜けていく
            pEnemyShot->x = pEnemyShotSet->param_d[0];
            pEnemyShot->y = pEnemyShotSet->param_d[1];
            pEnemyShot->muki = (2.0 * DX_PI / numWave) * i;
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアンの小玉(2.5x2.5)
            pEnemyShot->param_i[0] = 3; // 3:波紋弾
            pEnemyShot->param_d[0] = pEnemyShotSet->param_d[0]; // 吸引目標X
            pEnemyShot->param_d[1] = pEnemyShotSet->param_d[1]; // 吸引目標Y

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // === ③ 重力収縮（後半） ===
    if (pEnemyShotSet->count == 240) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 3) {
                pShot->param_i[0] = 4; // 4:吸引モードへ切り替え
                // 吸引目標座標をセット
                pShot->param_d[0] = pEnemyShotSet->param_d[0];
                pShot->param_d[1] = pEnemyShotSet->param_d[1];
            }
            pShot = pShot->next;
        }
    }

    // === 超爆散 ===
    if (pEnemyShotSet->count == 300) {
        // 巨大弾の外縁を構成していた大玉を消去（マージンをマイナスにしてメインルーチンに消させる）
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                pShot->margin = -1000.0;
            }
            pShot = pShot->next;
        }

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double cx = pEnemyShotSet->param_d[0];
        double cy = pEnemyShotSet->param_d[1];

        // 極太レーザーを3方向に放射（短レーザーを束ねて太くする）
        double laserAngles[3] = { DX_PI / 4.0, DX_PI / 2.0, DX_PI * 3.0 / 4.0 };
        for (int la = 0; la < 3; la++) {
            for (int i = -2; i <= 2; i++) { // 幅方向に5つ並べる
                for (int j = 0; j < 4; j++) { // 奥行き方向に4つ連結する
                    sEnemyShot* pEnemyShot = new sEnemyShot;
                    double perpAngle = laserAngles[la] + DX_PI / 2.0;
                    // 中心から直交方向へズラす
                    pEnemyShot->x = cx + i * 8.0 * cos(perpAngle) - j * 64.0 * cos(laserAngles[la]);
                    pEnemyShot->y = cy + i * 8.0 * sin(perpAngle) - j * 64.0 * sin(laserAngles[la]);
                    pEnemyShot->muki = laserAngles[la];
                    pEnemyShot->speed = 8.0;
                    pEnemyShot->kind = img_enemyShotLaser[0]; // 赤い短レーザー
                    pEnemyShot->param_i[0] = 5; // 5:爆散弾

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }

        // 星型に広がる高密度小弾幕（レーザーの隙間を埋める）
        int numStar = 48;
        for (int i = 0; i < numStar; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx;
            pEnemyShot->y = cy;
            pEnemyShot->muki = (2.0 * DX_PI / numStar) * i;
            pEnemyShot->speed = 3.5; // レーザーより遅くして後ろから迫るようにする
            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄色の中玉
            pEnemyShot->param_i[0] = 5; // 5:爆散弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // === 全弾の移動処理 ===
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case 0: // 巨大弾の外縁（120フレームで停止）
            if (pEnemyShotSet->count < 120) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            break;
        case 2: // 針弾
        case 3: // 波紋弾
        case 5: // 爆散弾（レーザー・星型）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        case 4: // 吸引モード
        {
            double dx = pShot->param_d[0] - pShot->x;
            double dy = pShot->param_d[1] - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > 5.0) {
                pShot->speed += 0.5; // 加速しながら引き寄せられる
                pShot->muki = atan2(dy, dx);
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                // 中心に到達したら消去
                pShot->margin = -1000.0;
            }
        }
        break;
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_HugeBullet_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 500フレームに1回、超巨大弾のセットを起動
    if (count % 400 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHugeCore;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;

        // 巨大弾の中心座標追跡用の初期値
        pEnemyShotSet->param_d[0] = enemy.x;
        pEnemyShotSet->param_d[1] = enemy.y + 10.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}