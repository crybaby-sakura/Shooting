// enemyPat_Tmp.cpp
// 弾幕パターン：虚空乱舞（こくうらんぶ）
// ボスが瞬間移動しながら残像と複合攻撃を繰り出す

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 定数
static const int WARP_INTERVAL = 150;       // ワープ周期(フレーム)
static const int AFTERIMAGE_LIFE = 48;      // 残像寿命(0.8秒@60fps)
static const double MIN_WARP_DIST = 150.0;  // プレイヤーとの最低ワープ距離
static const double FIELD_W = 480.0;
static const double FIELD_H = 480.0;

// 残像の弾幕パターン
static void AfterimagePattern(sEnemyShotSet* pSet)
{
    // 出現直後：低速菱形弾（青）を24方向にばら撒く
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int WAY = 24;
        for (int i = 0; i < WAY; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = (2.0 * DX_PI) * i / WAY;
            pShot->speed = 1.5;  // 低速

            // 青（色4）の菱形弾
            pShot->kind = img_enemyShotDiamond[4];

            // 反射管理パラメータ
            pShot->param_i[0] = 0;  // 現在の反射回数
            pShot->param_i[1] = 2;  // 最大反射回数
            pShot->margin = 20.0;    // 反射処理のため、マージン無しで画面端検出

            // リストに追加
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 毎フレーム：菱形弾の移動と壁反射処理
    {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;

            // 移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 壁反射判定（画面サイズ 480x480）
            bool reflected = false;
            if (pShot->x < 0.0) {
                pShot->x = 0.0;
                pShot->muki = DX_PI - pShot->muki;
                reflected = true;
            }
            else if (pShot->x > FIELD_W) {
                pShot->x = FIELD_W;
                pShot->muki = DX_PI - pShot->muki;
                reflected = true;
            }
            if (pShot->y < 0.0) {
                pShot->y = 0.0;
                pShot->muki = -pShot->muki;
                reflected = true;
            }
            else if (pShot->y > FIELD_H) {
                pShot->y = FIELD_H;
                pShot->muki = -pShot->muki;
                reflected = true;
            }

            if (reflected) {
                pShot->param_i[0]++;
                if (pShot->param_i[0] >= pShot->param_i[1]) {
                    // 最大反射回数に達したら消去
                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                }
            }

            pShot = pNext;
        }
    }

    // 寿命終了時（残像爆散）：中速ランダム弾（黄の小玉）を16方向に放出
    if (pSet->count == AFTERIMAGE_LIFE) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int WAY = 16;
        for (int i = 0; i < WAY; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            // ランダム方向（0～360度をWAY分割＋ブレ）
            double base = (2.0 * DX_PI) * i / WAY;
            pShot->muki = base + (GetRand(200) - 100) / 1000.0; // 小さいブレ
            pShot->speed = 3.0 + GetRand(100) / 100.0;         // 中速

            // 黄（色1）の小玉
            pShot->kind = img_enemyShotSmallBall[1];

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
}

// ワープ先攻撃パターン（十字レーザー＋自機狙い高速弾）
static void WarpAttackPattern(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 十字レーザー（白の短レーザー）を4方向に発射
        double laserDirs[4] = { 0.0, DX_PI / 2.0, DX_PI, 3.0 * DX_PI / 2.0 };
        for (int i = 0; i < 4; i++) {
            sEnemyShot* pLaser = new sEnemyShot;
            pLaser->x = pSet->x;
            pLaser->y = pSet->y;
            pLaser->muki = laserDirs[i];
            pLaser->speed = 0.0;  // その場に留まる
            pLaser->kind = img_enemyShotLaser[6]; // 白色短レーザー
            pLaser->param_i[0] = 1; // レーザー識別用フラグ

            pLaser->prev = pSet->pEnemyShotHead->prev;
            pLaser->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pLaser;
            pSet->pEnemyShotHead->prev = pLaser;
        }

        // 自機狙い高速弾（赤の銃弾）を12方向に発射
        const int WAY = 12;
        for (int i = 0; i < WAY; i++) {
            sEnemyShot* pBullet = new sEnemyShot;
            pBullet->x = pSet->x;
            pBullet->y = pSet->y;
            // 円形配置から自機狙いへ
            double angle = (2.0 * DX_PI) * i / WAY;
            pBullet->muki = atan2(player.y - pSet->y, player.x - pSet->x) + angle;
            pBullet->speed = 5.0; // 高速

            pBullet->kind = img_enemyShotBullet[0]; // 赤色銃弾

            pBullet->prev = pSet->pEnemyShotHead->prev;
            pBullet->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pBullet;
            pSet->pEnemyShotHead->prev = pBullet;
        }
    }

    // レーザーの寿命管理（1秒 = 60フレームで消去）
    {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;
            // レーザーは param_i[0] == 1 で識別、かつ生成から60フレーム経過で消去
            if (pShot->param_i[0] == 1 && pShot->count >= 60) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pNext;
        }
    }
}

// 敵本体パターン
void EnemyPat_Warp_DeepSeek()
{
    // 状態保持用の静的変数
    static double warpTargetX, warpTargetY;
    static int lastWarpCount = -WARP_INTERVAL; // 前回ワープ時のcount

    // 初回フレーム処理
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        lastWarpCount = -WARP_INTERVAL;
        warpTargetX = enemy.x;
        warpTargetY = enemy.y;
        return;
    }

    // ワープタイミング判定（WARP_INTERVALごと）
    if (count - lastWarpCount >= WARP_INTERVAL) {
        lastWarpCount = count;

        // 予兆音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 1. 現在位置に残像用ShotSetを生成
        sEnemyShotSet* pAfterimageSet = new sEnemyShotSet;
        pAfterimageSet->count = 0;
        pAfterimageSet->patternFunc = AfterimagePattern;
        pAfterimageSet->x = enemy.x;
        pAfterimageSet->y = enemy.y;
        pAfterimageSet->muki = 0.0;
        pAfterimageSet->kind = 0;

        pAfterimageSet->pEnemyShotHead = new sEnemyShot;
        pAfterimageSet->pEnemyShotHead->prev = pAfterimageSet->pEnemyShotHead;
        pAfterimageSet->pEnemyShotHead->next = pAfterimageSet->pEnemyShotHead;

        pAfterimageSet->prev = enemyShotSetHead.prev;
        pAfterimageSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pAfterimageSet;
        enemyShotSetHead.prev = pAfterimageSet;

        // 2. ワープ先を決定（プレイヤーから MIN_WARP_DIST 以上離れたランダム位置）
        do {
            warpTargetX = GetRand(479) * 1.0;  // 0～479
            warpTargetY = GetRand(479) * 1.0;
        } while (hypot(warpTargetX - player.x, warpTargetY - player.y) < MIN_WARP_DIST);

        // 3. ボスを瞬間移動
        enemy.x = warpTargetX;
        enemy.y = warpTargetY;

        // 4. ワープ先に攻撃用ShotSetを生成
        sEnemyShotSet* pWarpAttackSet = new sEnemyShotSet;
        pWarpAttackSet->count = 0;
        pWarpAttackSet->patternFunc = WarpAttackPattern;
        pWarpAttackSet->x = enemy.x;
        pWarpAttackSet->y = enemy.y;
        pWarpAttackSet->muki = 0.0;
        pWarpAttackSet->kind = 0;

        pWarpAttackSet->pEnemyShotHead = new sEnemyShot;
        pWarpAttackSet->pEnemyShotHead->prev = pWarpAttackSet->pEnemyShotHead;
        pWarpAttackSet->pEnemyShotHead->next = pWarpAttackSet->pEnemyShotHead;

        pWarpAttackSet->prev = enemyShotSetHead.prev;
        pWarpAttackSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pWarpAttackSet;
        enemyShotSetHead.prev = pWarpAttackSet;
    }
}