// 残光曼荼羅 - Phantom Trail -
// 自機の軌跡が弾幕に変わるパターン
// 敵本体関数名: void EnemyPat_TheMostFun_DeepSeek()

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾の役割
enum EShotType {
    TYPE_AFTERIMAGE = 0, // 残光（自機の軌跡）
    TYPE_NEEDLE = 1,     // 自機狙い針弾
    TYPE_WAVE = 2        // 波紋弾
};

// 残光の生成間隔(フレーム)
const int AFTERIMAGE_INTERVAL = 27;   // 0.45秒(60fps)
// 残光の爆発までの時間(フレーム)
const int AFTERIMAGE_LIFETIME = 96;   // 1.6秒
// 警告色に変わるタイミング
const int AFTERIMAGE_WARN_TIME = 80;   // 1.33秒
// 遅延フレーム数（自機の過去位置を使用）
const int PLAYER_POS_DELAY = 5;

// 難易度切り替え(経過フレーム)
const int DIFFICULTY_CHANGE_COUNT = 600; // 10秒

// 弾を生成してリストに追加する
static void CreateShot_Tmp(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind, int type)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->param_i[0] = type;
    pShot->param_i[1] = 0; // 爆発フラグなどに使用
    pShot->count = 0;      // メインルーチンが毎フレーム加算

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 残光爆発時に自機狙い針弾を撒く
static void SpawnNeedles_Tmp(sEnemyShotSet* pSet, double ex, double ey)
{
    int way = (pSet->count < DIFFICULTY_CHANGE_COUNT) ? 3 : 5;
    double stepDeg = 12.0;
    double startDeg = -(way - 1) / 2.0 * stepDeg;
    double baseAngle = atan2(player.y - ey, player.x - ex);

    for (int i = 0; i < way; ++i) {
        double offDeg = startDeg + i * stepDeg;
        // 微少なランダムずらし（±2度）
        int jitterDeg = GetRand(4) - 2;
        double angle = baseAngle + (offDeg + jitterDeg) * DX_PI / 180.0;
        double speed = (50 + GetRand(20)) / 10.0 - 1; // 5.0～7.0
        CreateShot_Tmp(pSet, ex, ey, angle, speed, img_enemyShotBullet[3], TYPE_NEEDLE);
    }
}

// 残光爆発時に波紋弾を撒く
static void SpawnWaves_Tmp(sEnemyShotSet* pSet, double ex, double ey)
{
    int way = (pSet->count < DIFFICULTY_CHANGE_COUNT) ? 8 : 12;
    for (int i = 0; i < way; ++i) {
        double angle = i * 2.0 * DX_PI / way;
        // 微少なランダムずらし（±5度）
        angle += (GetRand(10) - 5) * DX_PI / 180.0;
        double speed = (15 + GetRand(10)) / 10.0; // 1.5～2.5
        CreateShot_Tmp(pSet, ex, ey, angle, speed, img_enemyShotScale[3], TYPE_WAVE);
    }
}

// 弾幕パターン本体
static void PatternPhantomTrail(sEnemyShotSet* pSet)
{
    // 自機の過去位置を管理するための変数（pSetのparamを利用）
    // param_i[0] : 現在の書き込みインデックス
    // param_i[1] : 履歴が5フレーム分溜まったかどうか（0:未満、1:十分）
    // param_d[0..4]   : 過去5フレームのX座標
    // param_d[5..9]   : 過去5フレームのY座標
    int histIndex = pSet->param_i[0];
    int histReady = pSet->param_i[1];

    // 過去位置の取得（5フレーム前）と書き込みの順番に注意
    double oldX = 0.0;
    double oldY = 0.0;
    bool hasOldPos = (histReady != 0);

    if (hasOldPos) {
        oldX = pSet->param_d[histIndex];
        oldY = pSet->param_d[5 + histIndex];
    }

    // 一定間隔で、5フレーム前の自機位置に残光を生成
    if (hasOldPos && pSet->count % AFTERIMAGE_INTERVAL == 0) {
        CreateShot_Tmp(pSet, oldX, oldY, 0.0, 0.0,
            img_enemyShotLargeBall[6], TYPE_AFTERIMAGE);
    }

    // 現在の自機位置を履歴に書き込む
    pSet->param_d[histIndex] = player.x;
    pSet->param_d[5 + histIndex] = player.y;
    histIndex = (histIndex + 1) % PLAYER_POS_DELAY;
    pSet->param_i[0] = histIndex;

    // 一周したら履歴が十分に溜まったとみなす
    if (histIndex == 0) {
        pSet->param_i[1] = 1;
    }

    // ---- 第1パス：弾の更新・衝突判定 ----
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;

        if (pShot->param_i[0] == TYPE_AFTERIMAGE) {
            // 残光は動かない。寿命が近いと赤く点滅
            if (pShot->count >= AFTERIMAGE_LIFETIME) {
                pShot->param_i[1] = 1; // 爆発フラグ
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
            else if (pShot->count >= AFTERIMAGE_WARN_TIME) {
                pShot->kind = img_enemyShotLargeBall[0]; // 赤に変化
            }
        }
        else if (pShot->param_i[0] == TYPE_NEEDLE) {
            // 針弾は等速直線運動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[0] == TYPE_WAVE) {
            // 波紋弾はゆっくり進む
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 波紋弾が残光に触れたら誘爆させる
            sEnemyShot* pAfter = pSet->pEnemyShotHead->next;
            while (pAfter != pSet->pEnemyShotHead) {
                if (pAfter->param_i[0] == TYPE_AFTERIMAGE &&
                    pAfter->param_i[1] == 0)
                {
                    double dx = pShot->x - pAfter->x;
                    double dy = pShot->y - pAfter->y;
                    double distSq = dx * dx + dy * dy;
                    if (distSq < 20.0 * 20.0) {
                        pAfter->param_i[1] = 1; // 誘爆フラグ
                        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                    }
                }
                pAfter = pAfter->next;
            }
        }

        pShot = next;
    }

    // ---- 第2パス：爆発する残光を削除し、針弾・波紋弾を生成 ----
    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;

        if (pShot->param_i[0] == TYPE_AFTERIMAGE && pShot->param_i[1] == 1) {
            double ex = pShot->x;
            double ey = pShot->y;

            // リストから外して削除
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;

            // 爆発で針弾と波紋弾を放出            
            SpawnNeedles_Tmp(pSet, ex, ey);
            SpawnWaves_Tmp(pSet, ex, ey);
        }

        pShot = next;
    }
}

// 敵本体パターン（新規関数名）
void EnemyPat_TheMostFun_DeepSeek()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        // 残光曼荼羅用のショットセットを1つだけ生成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = PatternPhantomTrail;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // 履歴管理用パラメータの初期化
        pSet->param_i[0] = 0; // histIndex
        pSet->param_i[1] = 0; // histReady

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 敵本体は基本的に動かさない（弾幕は自機の軌跡で変化する）
}