// enemyPat_Tmp.cpp (または enemyPat_sampleForAI.cpp に追記)

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：外歯車（固定軌跡）
static void ShotOuterGear(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 外歯車の初期化
        const int OUTER_TEETH = 48; // 外歯車の歯の数
        double R = 180.0 * 1.3;           // 外歯車の半径

        for (int i = 0; i < OUTER_TEETH; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double beta = i * (2.0 * DX_PI / OUTER_TEETH);

            pShot->x = 240.0 + R * cos(beta);
            pShot->y = 240.0 + R * sin(beta);
            pShot->muki = beta + DX_PI / 2.0; // 接線方向を向かせて歯っぽく見せる
            pShot->speed = 0.0;               // 速度0で固定
            pShot->kind = img_enemyShotMediumOval[4]; // 中楕円弾, 色:4(青)

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 既存の弾の移動処理（速度0なので位置は変わらないが、メインルーチンと同様のループ構造を維持）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 弾幕：スピログラフ（回転歯車の軌跡）
static void ShotSpirograph(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 予告音と発射音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // パラメータの初期化
        // param_d[0]: R (外歯車の半径)
        // param_d[1]: r (内歯車の半径)
        // param_d[2]: d (内歯車の中心から歯の先までの距離)
        // param_d[3]: d_theta (1フレームあたりの回転角)
        pEnemyShotSet->param_d[0] = 180.0 * 1.3;
        pEnemyShotSet->param_d[1] = 60.0 * 1.3;
        pEnemyShotSet->param_d[2] = 45.0 * 1.3;
        pEnemyShotSet->param_d[3] = 0.025 * 0.75;
    }

    double R = pEnemyShotSet->param_d[0];
    double r = pEnemyShotSet->param_d[1];
    double d = pEnemyShotSet->param_d[2];
    double d_theta = pEnemyShotSet->param_d[3];

    // 現在の回転角
    double theta = pEnemyShotSet->count * d_theta;

    // 内歯車の中心座標 (画面中央 240, 240 を基準)
    double cx = 240.0 + (R - r) * cos(theta);
    double cy = 240.0 + (R - r) * sin(theta);

    // 自転と公転の比率 (内側を転がるため負の方向)
    double ratio = (R - r) / r;

    // 内歯車の歯の数
    const int GEAR_TEETH = 8;

    // 内歯車の各「歯」の位置を計算し、弾を生成
    for (int k = 0; k < GEAR_TEETH; k++) {
        // 各歯の角度 (自転角度 + 位相オフセット)
        double alpha = -ratio * theta + k * (2.0 * DX_PI / GEAR_TEETH);

        // 歯の先の座標
        double gx = cx + d * cos(alpha);
        double gy = cy + d * sin(alpha);

        // 1. 内歯車の「歯」本体 (軌跡としてその場に残す)
        if (pEnemyShotSet->count % 1 == 0) {
            sEnemyShot* pShotGear = new sEnemyShot;
            pShotGear->x = gx;
            pShotGear->y = gy;
            pShotGear->muki = alpha;
            pShotGear->speed = 0.0; // 速度0でその場に残し、軌跡を形成
            pShotGear->kind = img_enemyShotMediumBall[0]; // 中玉, 色:0(赤)

            pShotGear->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShotGear->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShotGear;
            pEnemyShotSet->pEnemyShotHead->prev = pShotGear;
        }

        // 2. 歯の先から発射される弾幕 (外向きに飛翔)
        if (pEnemyShotSet->count % 5 == 0) {
            sEnemyShot* pShotBullet = new sEnemyShot;
            pShotBullet->x = gx;
            pShotBullet->y = gy;
            pShotBullet->muki = alpha; // 歯の外向き(法線方向)に発射
            pShotBullet->speed = 2.5;
            pShotBullet->kind = img_enemyShotSmallBall[6]; // 小玉, 色:6(白)

            pShotBullet->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShotBullet->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShotBullet;
            pEnemyShotSet->pEnemyShotHead->prev = pShotBullet;
        }
    }

    // 既存の弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        if (pShot->kind == img_enemyShotMediumBall[0] && pShot->count >= 240) pShot->margin = -9999;
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Spirograph_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480。中央に配置
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 60 * 20; // パターン観賞用として耐久を高く設定

        // ==========================================
        // 外歯車 (固定) 管理用ショットセット
        // ==========================================
        sEnemyShotSet* pOuterGearSet = new sEnemyShotSet;
        pOuterGearSet->count = 0;
        pOuterGearSet->patternFunc = ShotOuterGear; // 専用の関数を割り当て
        pOuterGearSet->x = 240.0;
        pOuterGearSet->y = 240.0;

        pOuterGearSet->pEnemyShotHead = new sEnemyShot;
        pOuterGearSet->pEnemyShotHead->prev = pOuterGearSet->pEnemyShotHead;
        pOuterGearSet->pEnemyShotHead->next = pOuterGearSet->pEnemyShotHead;

        pOuterGearSet->prev = enemyShotSetHead.prev;
        pOuterGearSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pOuterGearSet;
        enemyShotSetHead.prev = pOuterGearSet;

        // ==========================================
        // 内歯車・発射弾管理用ショットセット
        // ==========================================
        sEnemyShotSet* pInnerGearSet = new sEnemyShotSet;
        pInnerGearSet->count = 0;
        pInnerGearSet->patternFunc = ShotSpirograph;
        pInnerGearSet->x = 240.0;
        pInnerGearSet->y = 240.0;

        pInnerGearSet->pEnemyShotHead = new sEnemyShot;
        pInnerGearSet->pEnemyShotHead->prev = pInnerGearSet->pEnemyShotHead;
        pInnerGearSet->pEnemyShotHead->next = pInnerGearSet->pEnemyShotHead;

        pInnerGearSet->prev = enemyShotSetHead.prev;
        pInnerGearSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pInnerGearSet;
        enemyShotSetHead.prev = pInnerGearSet;
    }
    else {
        // 敵本体は中央に固定されたまま
        // 必要であればここで敵本体の点滅やHP管理などを行う
        enemy.hp--;
    }
}