#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------------------------------------
// 弾幕：青天（静かで穏やかな青い波）
// ---------------------------------------------------------
static void ShotSeiten(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射フレームのみ処理
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 5〜8wayの扇状にばら撒く
        int way = 5 + GetRand(3);
        double base_muki = pEnemyShotSet->muki;

        for (int i = 0; i < way; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 扇状に均等にばらけさせつつ、わずかにランダムなブレを加える
            pEnemyShot->muki = base_muki - 0.4 + (0.8 / (way - 1)) * i + (GetRand(20) - 10) / 100.0;
            pEnemyShot->speed = 1.0 + GetRand(150) / 100.0; // 速度 1.0 〜 2.5 (遅め)

            // 青色の小玉をメインに、たまにシアン色の中玉を混ぜる
            if (GetRand(4) == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン(3)・中玉
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青(4)・小玉
            }

            pEnemyShot->param_i[0] = 0; // 通常弾フラグ

            // リストの末尾に追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理（全て直進）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// ---------------------------------------------------------
// 弾幕：霹靂（前兆の予告線 → 超高速の雷撃 → 散る火花）
// ---------------------------------------------------------
static void ShotHekireki(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 【1. 霹靂の前兆】（発動フレーム）
    if (pEnemyShotSet->count == 0) {
        // 自機への角度を計算して保存しておく（本命の雷撃で使うため）
        pEnemyShotSet->param_d[0] = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 予告線として極細レーザーを超高速で1発だけ撃つ
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->param_d[0];
        pEnemyShot->speed = 30.0; // 一瞬で画面を抜ける超高速
        pEnemyShot->kind = img_enemyShotLaser[0]; // 赤(0)・短レーザー
        pEnemyShot->param_i[0] = 0;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 【2. 霹靂の着弾】（前兆から約0.5秒後）
    if (pEnemyShotSet->count == 30) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double base_muki = pEnemyShotSet->param_d[0];

        // 2-A. 雷撃本体（白の菱形弾と黄色の鱗弾を、速度を僅かにずらして一本の太い線に見せる）
        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_muki;
            pEnemyShot->speed = 22.0 + i * 1.0;
            pEnemyShot->kind = img_enemyShotDiamond[6]; // 白(6)・菱形
            pEnemyShot->param_i[0] = 0; // 通常弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_muki;
            pEnemyShot->speed = 22.5 + i * 1.0;
            pEnemyShot->kind = img_enemyShotScale[1]; // 黄(1)・鱗弾
            pEnemyShot->param_i[0] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 2-B. 雷撃の余波・火花（雷の軌跡から弾けるように散る小玉）
        for (int i = 0; i < 40*3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_muki; // 初期角度は雷撃と同じ
            pEnemyShot->speed = 20.0 + GetRand(60) / 10.0; // 20.0 〜 26.0

            if (GetRand(1) == 0) pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄(1)・小玉
            else pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙(8)・小玉

            // ★重要：弾が生成されてから何フレーム目に起爆して横に散るかのタイマー（2〜20フレーム）
            pEnemyShot->param_i[0] = 2 + GetRand(18);

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 火花が散るパチパチとした音の演出
    if (pEnemyShotSet->count > 30 && pEnemyShotSet->count < 55 && pEnemyShotSet->count % 5 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 弾の移動処理・火花の起爆判定
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // 火花弾の起爆（指定フレームに達したら、向きを変えて急減速）
        if (pShot->param_i[0] > 0 && pShot->count == pShot->param_i[0]) {
            // 雷撃の軌道から左右約90度（±20度ほどのブレ）の方向へ散らす
            double angleDiff = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
            if (GetRand(1) == 0) angleDiff *= -1.0; // 左右ランダム

            pShot->muki += angleDiff;
            pShot->speed = 1.0 + GetRand(200) / 100.0; // 速度を 1.0 〜 3.0 に急減速
            pShot->param_i[0] = 0; // 起爆済みとする
        }

        // 座標更新
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// ---------------------------------------------------------
// 敵本体のパターン（青天の霹靂 統括）
// ---------------------------------------------------------
void EnemyPat_SeitenNoHekireki_Gemini()
{
    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 敵の移動（定位置を中心に、ゆったりと左右に揺れる）
    enemy.x = 240.0 + 60.0 * sin(count / 70.0);

    // 300フレーム（5秒）で1ループとする周期管理
    int phase = count % 300;

    // ---------------------------------------------------------
    // 1. 青天フェーズ (0 〜 150フレーム)
    // ---------------------------------------------------------
    // 10フレーム毎（1秒間に6回）に青い波を降らせる
    if (phase < 150 && phase % 10 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSeiten;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = DX_PI / 2.0; // 基本は真下方向

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ---------------------------------------------------------
    // 2. 霹靂フェーズ (180フレーム目に発動開始)
    // ---------------------------------------------------------
    // 静寂（150〜179フレーム）を挟み、予告・雷撃・余波のSetを登録
    if (phase == 180) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHekireki;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}