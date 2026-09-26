#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：焼栗『爆裂天津弾』（ばくれつてんしんだん）
// 【挙動】
// 1. 加熱：敵から大玉（栗）と、その周りを回る小玉（熱気）が自機に向かって飛んでくる。
// 2. 亀裂：空中でスーッと減速して止まり、タメの後に大玉から菱形弾（実）が少しだけせり出す。
// 3. 破裂：大玉が消滅し、一気に高速の菱形弾（実）が扇状に弾け飛び、小玉（熱気）も四方に散る。

static void ShotKuri(sEnemyShotSet* pEnemyShotSet)
{
    // === 1. カウントに応じた弾の追加とSE再生 ===

    // (1) 加熱・投下：栗の生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 大円弾（栗の本体：橙）
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = pEnemyShotSet->muki;
        pShot->speed = 3.0; // 減速してちょうど60フレームで止まる初速
        pShot->kind = img_enemyShotLargeBall[8]; // 橙色の大玉
        pShot->param_i[0] = 0; // 役割0: 栗の本体
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;

        // 小円弾（熱気：赤）を4個
        for (int i = 0; i < 4; i++) {
            sEnemyShot* pSmall = new sEnemyShot;
            pSmall->x = pEnemyShotSet->x;
            pSmall->y = pEnemyShotSet->y;
            pSmall->muki = 0;
            pSmall->speed = 0;
            pSmall->kind = img_enemyShotSmallBall[0]; // 赤色の小玉
            pSmall->param_i[0] = 1; // 役割1: 熱気
            pSmall->param_d[0] = (DX_PI * 2.0 / 4.0) * i; // 各自の初期角度
            pSmall->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pSmall->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pSmall;
            pEnemyShotSet->pEnemyShotHead->prev = pSmall;
        }
    }

    // 大玉（栗の本体）の現在位置を取得する（基準点用）
    bool hasCore = false;
    double cx = 0, cy = 0;
    sEnemyShot* pNode = pEnemyShotSet->pEnemyShotHead->next;
    while (pNode != pEnemyShotSet->pEnemyShotHead) {
        if (pNode->param_i[0] == 0) {
            hasCore = true;
            cx = pNode->x;
            cy = pNode->y;
            break;
        }
        pNode = pNode->next;
    }

    // (2) 亀裂：はぜる直前に中身がチラ見えする
    if (pEnemyShotSet->count == 80 && hasCore) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - cy, player.x - cx); // 自機方向
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pKiretsu = new sEnemyShot;
            // 栗の輪郭付近からじわっと出させるため少し外側に配置
            double angle = baseAngle + (i - 1) * 0.2;
            pKiretsu->x = cx + 15.0 * cos(angle);
            pKiretsu->y = cy + 15.0 * sin(angle);
            pKiretsu->muki = angle;
            pKiretsu->speed = 0.5; // ゆっくりせり出す
            pKiretsu->kind = img_enemyShotDiamond[1]; // 黄色の菱形弾（実）
            pKiretsu->param_i[0] = 2; // 役割2: チラ見えする実
            pKiretsu->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pKiretsu->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pKiretsu;
            pEnemyShotSet->pEnemyShotHead->prev = pKiretsu;
        }
    }

    // (3) 破裂・拡散：はぜた瞬間
    if (pEnemyShotSet->count == 110 && hasCore) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - cy, player.x - cx);

        // メインの実（高速の扇状スプレッド弾：黄色の菱形）
        int num = 13;
        for (int i = 0; i < num; i++) {
            sEnemyShot* pMi = new sEnemyShot;
            pMi->x = cx;
            pMi->y = cy;
            // -45度 〜 +45度 に等間隔で配置して美しい扇状に
            double angleOffset = (num == 1) ? 0 : (-DX_PI / 4.0) + (DX_PI / 2.0) * i / (num - 1);
            pMi->muki = baseAngle + angleOffset;
            pMi->speed = 6.0+3;
            pMi->kind = img_enemyShotDiamond[1]; // 黄
            pMi->param_i[0] = 3; // 役割3: 弾け飛ぶ実
            pMi->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pMi->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pMi;
            pEnemyShotSet->pEnemyShotHead->prev = pMi;
        }

        // ちょっと遅れて広がるサブの実（皮の破片的なばらまき：橙色の鱗弾）
        for (int i = 0; i < 10*3; i++) {
            sEnemyShot* pMi2 = new sEnemyShot;
            pMi2->x = cx;
            pMi2->y = cy;
            pMi2->muki = baseAngle + (GetRand(120) - 60) / 180.0 * DX_PI; // -60度〜+60度
            pMi2->speed = 2.0 + GetRand(300) / 100.0;
            pMi2->kind = img_enemyShotScale[8]; // 橙
            pMi2->param_i[0] = 3;
            pMi2->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pMi2->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pMi2;
            pEnemyShotSet->pEnemyShotHead->prev = pMi2;
        }
    }

    // === 2. 各弾の移動と状態更新 ===
    pNode = pEnemyShotSet->pEnemyShotHead->next;
    while (pNode != pEnemyShotSet->pEnemyShotHead) {
        int kindRole = pNode->param_i[0];

        if (kindRole == 0) { // 栗の本体
            if (pEnemyShotSet->count < 60) {
                // 毎フレーム減速（0.05 × 60フレーム = 3.0減速し、綺麗に停止する）
                pNode->speed -= 0.05;
                if (pNode->speed < 0) pNode->speed = 0;
                pNode->x += pNode->speed * cos(pNode->muki);
                pNode->y += pNode->speed * sin(pNode->muki);
            }
            else if (pEnemyShotSet->count == 110) {
                // 破裂時に画面外へ飛ばして自動消去させる
                pNode->x = -9999;
                pNode->y = -9999;
            }
        }
        else if (kindRole == 1) { // 熱気（小玉）
            if (pEnemyShotSet->count < 110) {
                if (hasCore) {
                    // 大玉の周りを回転
                    pNode->param_d[0] += 0.1; // 角度更新
                    double r = 30.0; // 回転半径
                    pNode->x = cx + r * cos(pNode->param_d[0]);
                    pNode->y = cy + r * sin(pNode->param_d[0]);
                }
                else {
                    pNode->x = -9999;
                    pNode->y = -9999;
                }
            }
            else if (pEnemyShotSet->count == 110) {
                // 破裂と同時に四方へ散る
                pNode->muki = GetRand(360) / 180.0 * DX_PI;
                pNode->speed = 1.0 + GetRand(150) / 100.0;
                pNode->x += pNode->speed * cos(pNode->muki);
                pNode->y += pNode->speed * sin(pNode->muki);
            }
            else {
                // 散った後の直線移動
                pNode->x += pNode->speed * cos(pNode->muki);
                pNode->y += pNode->speed * sin(pNode->muki);
            }
        }
        else if (kindRole == 2) { // チラ見えする実
            if (pEnemyShotSet->count < 100) {
                // じわっと進む
                pNode->x += pNode->speed * cos(pNode->muki);
                pNode->y += pNode->speed * sin(pNode->muki);
            }
            if (pEnemyShotSet->count == 110) {
                // 本物の実が弾け飛ぶため用済みとして消去
                pNode->x = -9999;
                pNode->y = -9999;
            }
        }
        else if (kindRole == 3) { // 弾け飛んだ実
            pNode->x += pNode->speed * cos(pNode->muki);
            pNode->y += pNode->speed * sin(pNode->muki);
        }

        pNode = pNode->next;
    }
}

// 敵本体のパターン（エントリポイント）
void EnemyPat_RoastChestnut_Gemini()
{
    static int muki;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 150.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    // 移動（上部を左右にうろうろする）
    else {
        enemy.x += 1.0 * (double)muki;
        if (enemy.x > 380.0) muki = -1;
        if (enemy.x < 100.0) muki = 1;
    }

    // 100フレームごとに1回、栗を発射
    if (count % 100 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKuri;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 弾セットのリスト初期化・追加
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}