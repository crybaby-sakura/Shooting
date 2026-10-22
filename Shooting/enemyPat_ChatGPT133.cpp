#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 共通：敵弾を追加
// ============================================================
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

    return pEnemyShot;
}

// ============================================================
// 共通：効果音
// ============================================================
static void PlayEnemySound(int sound)
{
    if (CheckSoundMem(sound)) StopSoundMem(sound);
    PlaySoundMem(sound, DX_PLAYTYPE_BACK);
}

// ============================================================
// 晴天の弾
// 青い小玉をプレイヤー方向へゆったり放つ
// ============================================================
static void ShotClearSky(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlayEnemySound(sound_enemyShot_light);

        double baseMuki = atan2(
            player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x
        );

        for (int i = 0; i < 11; i++) {
            pEnemyShot = AddShot(pEnemyShotSet);

            double offset = ((double)i - 5.0) * 0.075;
            double randomOffset = (GetRand(20) - 10) / 500.0;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 10.0;
            pEnemyShot->muki = baseMuki + offset + randomOffset;
            pEnemyShot->speed = 1.4 + GetRand(45) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[4];
        }

        // 空から落ちてくる薄い雲のような弾
        for (int i = 0; i < 4; i++) {
            pEnemyShot = AddShot(pEnemyShotSet);

            pEnemyShot->x = 70.0 + GetRand(340);
            pEnemyShot->y = -20.0 - GetRand(70);
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(20) - 10) / 500.0;
            pEnemyShot->speed = 0.8 + GetRand(35) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[3];
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 青天の霹靂：突然発生する雷
//
// param_i[0]
//   0 = 予告・衝撃波弾
//   1 = 雷光レーザー
//   2 = 落雷中心
//
// param_d[0], [1]
//   落雷地点
//
// param_d[2]
//   落雷開始位置X
// ============================================================
static void ShotLightning(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot, *pShot;

    // --------------------------------------------------------
    // 雷鳴前の予告
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        PlayEnemySound(sound_enemyCharge);

        // プレイヤーの現在位置を「落雷予定地点」として記録
        pEnemyShotSet->param_d[0] = player.x;
        pEnemyShotSet->param_d[1] = player.y;

        // 上空の雷雲から落とすための始点
        pEnemyShotSet->param_d[2] =
            pEnemyShotSet->param_d[0] + GetRand(360) - 180;

        if (pEnemyShotSet->param_d[2] < 30.0)
            pEnemyShotSet->param_d[2] = 30.0;
        if (pEnemyShotSet->param_d[2] > 450.0)
            pEnemyShotSet->param_d[2] = 450.0;

        // 雷が溜まっているように見せる青白い玉
        for (int i = 0; i < 8; i++) {
            pEnemyShot = AddShot(pEnemyShotSet);

            double muki = DX_PI * 2.0 * i / 8.0;

            pEnemyShot->x =
                pEnemyShotSet->x + cos(muki) * 22.0;
            pEnemyShot->y =
                pEnemyShotSet->y + sin(muki) * 22.0;
            pEnemyShot->muki = muki;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[3];
            pEnemyShot->param_i[0] = 0;
        }
    }

    // --------------------------------------------------------
    // 突然の落雷
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 34) {
        PlayEnemySound(sound_enemyShot_extreme);

        // 予告用の玉を四散させる
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                pShot->kind = img_enemyShotSmallBall[1];
                pShot->muki =
                    atan2(
                        pShot->y - pEnemyShotSet->y,
                        pShot->x - pEnemyShotSet->x
                    );
                pShot->speed = 4.5;
                pShot->param_i[0] = 0;
            }
            pShot = pShot->next;
        }

        // ----------------------------------------------------
        // 雷の本体
        // 上空から落雷地点までギザギザに伸ばす
        // ----------------------------------------------------
        const int segmentCount = 9;
        double nodeX[segmentCount + 1];
        double nodeY[segmentCount + 1];

        nodeX[0] = pEnemyShotSet->param_d[2];
        nodeY[0] = -20.0;

        for (int i = 1; i < segmentCount; i++) {
            double t = (double)i / (double)segmentCount;

            double straightX =
                nodeX[0] +
                (pEnemyShotSet->param_d[0] - nodeX[0]) * t;

            double straightY =
                nodeY[0] +
                (pEnemyShotSet->param_d[1] - nodeY[0]) * t;

            double offset = (i % 2 == 0) ? 26.0 : -26.0;

            // 少しだけランダム性を加えて雷らしい折れ方にする
            offset += GetRand(16) - 8;

            nodeX[i] = straightX + offset;
            nodeY[i] = straightY;
        }

        nodeX[segmentCount] = pEnemyShotSet->param_d[0];
        nodeY[segmentCount] = pEnemyShotSet->param_d[1];

        // 雷の各節
        for (int i = 0; i < segmentCount; i++) {
            double dx = nodeX[i + 1] - nodeX[i];
            double dy = nodeY[i + 1] - nodeY[i];

            double length = sqrt(dx * dx + dy * dy);
            double muki = atan2(dy, dx);

            pEnemyShot = AddShot(pEnemyShotSet);

            pEnemyShot->x =
                (nodeX[i] + nodeX[i + 1]) * 0.5;
            pEnemyShot->y =
                (nodeY[i] + nodeY[i + 1]) * 0.5;
            pEnemyShot->muki = muki;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[6];
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->param_d[0] = length;
        }

        // ----------------------------------------------------
        // 雷の折れ曲がり部分に火花を置く
        // ----------------------------------------------------
        for (int i = 1; i < segmentCount; i++) {
            pEnemyShot = AddShot(pEnemyShotSet);

            pEnemyShot->x = nodeX[i];
            pEnemyShot->y = nodeY[i];
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotDiamond[1];
            pEnemyShot->param_i[0] = 1;
        }

        // ----------------------------------------------------
        // 落雷地点そのもの
        // ----------------------------------------------------
        pEnemyShot = AddShot(pEnemyShotSet);

        pEnemyShot->x = pEnemyShotSet->param_d[0];
        pEnemyShot->y = pEnemyShotSet->param_d[1];
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotLargeBall[1];
        pEnemyShot->param_i[0] = 2;

        // ----------------------------------------------------
        // 落雷の衝撃波
        // ----------------------------------------------------
        for (int i = 0; i < 18; i++) {
            pEnemyShot = AddShot(pEnemyShotSet);

            double muki =
                DX_PI * 2.0 * i / 18.0
                + (GetRand(30) - 15) / 500.0;

            pEnemyShot->x = pEnemyShotSet->param_d[0];
            pEnemyShot->y = pEnemyShotSet->param_d[1];
            pEnemyShot->muki = muki;
            pEnemyShot->speed = 2.8 + GetRand(55) / 100.0;

            if (i % 3 == 0)
                pEnemyShot->kind = img_enemyShotSmallBall[6];
            else
                pEnemyShot->kind = img_enemyShotSmallBall[1];

            pEnemyShot->param_i[0] = 0;
        }

        // ----------------------------------------------------
        // 雷の枝分かれ
        // ----------------------------------------------------
        int branchIndex = 4;

        double branchX = nodeX[branchIndex];
        double branchY = nodeY[branchIndex];

        double branchAngle =
            atan2(
                pEnemyShotSet->param_d[1] - branchY,
                pEnemyShotSet->param_d[0] - branchX
            );

        for (int i = 0; i < 2; i++) {
            double angle =
                branchAngle
                + (i == 0 ? -0.75 : 0.75);

            pEnemyShot = AddShot(pEnemyShotSet);

            pEnemyShot->x = branchX + cos(angle) * 20.0;
            pEnemyShot->y = branchY + sin(angle) * 20.0;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[6];
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->param_d[0] = 45.0;
        }
    }

    // --------------------------------------------------------
    // 雷光が砕けて飛び散る
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 46) {
        pShot = pEnemyShotSet->pEnemyShotHead->next;

        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {
                double muki = pShot->muki;

                pShot->speed =
                    5.5 + GetRand(20) / 100.0;

                pShot->muki = muki;
            }

            if (pShot->param_i[0] == 2) {
                pShot->kind = img_enemyShotMediumBall[1];
                pShot->speed = 2.8;
                pShot->muki = GetRand(628) / 100.0 / 100.0 * DX_PI;
                pShot->param_i[0] = 0;
            }

            pShot = pShot->next;
        }
    }

    // --------------------------------------------------------
    // 弾の移動
    // --------------------------------------------------------
    pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_SeitenNoHekireki_ChatGPT()
{
    static int muki;
    static int skyShotCount;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        muki = 1;
        skyShotCount = 0;
    }
    else {
        enemy.x += 1.05 * (double)muki;

        if (enemy.x > 430.0) {
            enemy.x = 430.0;
            muki = -1;
        }
        else if (enemy.x < 50.0) {
            enemy.x = 50.0;
            muki = 1;
        }
    }

    // --------------------------------------------------------
    // 晴天の弾
    // --------------------------------------------------------
    if (count % 36 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotClearSky;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x
            );
        pEnemyShotSet->kind = skyShotCount++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // --------------------------------------------------------
    // 青天の霹靂
    // 定期的に突然の落雷を発生
    // --------------------------------------------------------
    if (count % 210 == 121) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLightning;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x
            );
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}