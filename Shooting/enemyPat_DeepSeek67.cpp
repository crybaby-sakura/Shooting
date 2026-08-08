// enemyPat_Tmp.cpp
// 因果遡行「不可逆の残影」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <cmath>

// -----------------------------------------------------------
// 前方宣言
// -----------------------------------------------------------
static void ShotRotatingGears(sEnemyShotSet* pEnemyShotSet);
static void ShotPastChainLaser(sEnemyShotSet* pEnemyShotSet);
static void ShotStardustMine(sEnemyShotSet* pEnemyShotSet);

// -----------------------------------------------------------
// 補助：双方向リストから sEnemyShot を削除して delete する
// -----------------------------------------------------------
static void DeleteShot(sEnemyShot* pShot, sEnemyShotSet* pSet) {
    if (!pShot) return;
    pShot->prev->next = pShot->next;
    pShot->next->prev = pShot->prev;
    delete pShot;
}

// -----------------------------------------------------------
// パターン1: 廻天の歯車 (外殻回転弾幕)
// -----------------------------------------------------------
static void ShotRotatingGears(sEnemyShotSet* pEnemyShotSet) {
    const int LAYER_A = 36; // 外層
    const int LAYER_B = 36; // 内層
    const double START_RADIUS_A = 320.0;
    const double START_RADIUS_B = 290.0;
    const double SHRINK_SPEED = 0.6;   // 毎フレームの収縮量
    const double ROT_SPEED_BASE = 0.025; // 基本回転速度(ラジアン/フレーム)
    const int REVERSE_INTERVAL = 180;    // 反転周期

    if (pEnemyShotSet->count == 0) {
        // 初期化
        pEnemyShotSet->param_i[0] = 1;  // 回転方向フラグ 1 or -1
        pEnemyShotSet->param_i[1] = 0;  // 反転カウンタ
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 外層A
        for (int i = 0; i < LAYER_A; ++i) {
            sEnemyShot* p = new sEnemyShot;
            double angle = (2.0 * DX_PI / LAYER_A) * i;
            p->x = pEnemyShotSet->x + START_RADIUS_A * cos(angle);
            p->y = pEnemyShotSet->y + START_RADIUS_A * sin(angle);
            p->muki = angle + DX_PI / 2.0; // 進行方向（接線方向）
            p->speed = 0.0;
            p->kind = img_enemyShotDiamond[4]; // 青菱形弾 細長い楔に
            p->margin = 480;
            p->param_d[0] = START_RADIUS_A;   // 半径
            p->param_d[1] = angle;            // 角度
            p->param_i[0] = 0;                // A層フラグ
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
        // 内層B
        for (int i = 0; i < LAYER_B; ++i) {
            sEnemyShot* p = new sEnemyShot;
            double angle = (2.0 * DX_PI / LAYER_B) * i + (DX_PI / LAYER_B); // オフセット
            p->x = pEnemyShotSet->x + START_RADIUS_B * cos(angle);
            p->y = pEnemyShotSet->y + START_RADIUS_B * sin(angle);
            p->muki = angle + DX_PI / 2.0;
            p->speed = 0.0;
            p->kind = img_enemyShotScale[3]; // シアン鱗弾
            p->margin = 480;
            p->param_d[0] = START_RADIUS_B;
            p->param_d[1] = angle;
            p->param_i[0] = 1;                // B層フラグ
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 回転方向反転
    pEnemyShotSet->param_i[1]++;
    if (pEnemyShotSet->param_i[1] >= REVERSE_INTERVAL) {
        pEnemyShotSet->param_i[1] = 0;
        pEnemyShotSet->param_i[0] *= -1;
    }

    int dir = pEnemyShotSet->param_i[0];
    double cx = pEnemyShotSet->x;
    double cy = pEnemyShotSet->y;

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = p->next;
        double& radius = p->param_d[0];
        double& angle = p->param_d[1];

        radius -= SHRINK_SPEED;
        if (radius < 10.0) {
            // 中心付近で削除
            DeleteShot(p, pEnemyShotSet);
            p = next;
            continue;
        }
        double rotSpeed = (p->param_i[0] == 0) ? ROT_SPEED_BASE : ROT_SPEED_BASE * 1.3;
        angle += rotSpeed * dir;
        p->x = cx + radius * cos(angle);
        p->y = cy + radius * sin(angle);
        p = next;
    }
}

// -----------------------------------------------------------
// パターン2: 過去の鎖 (残像追尾レーザー)
// -----------------------------------------------------------
static void ShotPastChainLaser(sEnemyShotSet* pEnemyShotSet) {
    const int LIFE_FRAMES = 90; // 1.5秒 (60fps)

    if (pEnemyShotSet->count == 0) {
        // 生成時にすでに param_d[0],param_d[1] に過去座標が入っている想定
        double tx = pEnemyShotSet->param_d[0];
        double ty = pEnemyShotSet->param_d[1];
        double angle = atan2(ty - pEnemyShotSet->y, tx - pEnemyShotSet->x);

        sEnemyShot* p = new sEnemyShot;
        p->x = tx;
        p->y = ty;
        p->muki = angle;            // 敵から過去座標への方向にレーザーが伸びる
        p->speed = 0.0;
        p->kind = img_enemyShotLaser[0]; // 赤レーザー
        p->param_i[0] = LIFE_FRAMES;
        p->prev = pEnemyShotSet->pEnemyShotHead->prev;
        p->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = p;
        pEnemyShotSet->pEnemyShotHead->prev = p;

        if (count % 20 == 0) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
    }

    // 寿命管理と削除
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = p->next;
        p->param_i[0]--;
        if (p->param_i[0] <= 0) {
            DeleteShot(p, pEnemyShotSet);
        }
        p = next;
    }
}

// -----------------------------------------------------------
// パターン3: 今際の星霜 (確率散布機雷)
// -----------------------------------------------------------
static void ShotStardustMine(sEnemyShotSet* pEnemyShotSet) {
    const int MINE_COUNT = 120;
    const double DETECT_RANGE = 40.0;
    const double BULLET_SPEED = 4.5;
    const int RESET_INTERVAL = 60; // 1秒

    if (pEnemyShotSet->count == 0) {
        // 初回：画面全体に機雷をランダム配置
        for (int i = 0; i < MINE_COUNT; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = 40.0 + GetRand(400); // 画面端を避ける
            p->y = 40.0 + GetRand(400);
            p->speed = 0.0;
            p->muki = 0.0;
            p->kind = img_enemyShotSmallBall[8]; // 橙小玉
            p->param_i[0] = 0; // 0:未発射 1:発射済み
            p->param_d[0] = 0.0;
            p->param_d[1] = 0.0;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
        pEnemyShotSet->param_i[2] = 0; // リセット用カウンタ
    }

    // 1秒ごとの再配置（既存の未発射弾を全削除 → 再生成）
    pEnemyShotSet->param_i[2]++;
    if (pEnemyShotSet->param_i[2] >= RESET_INTERVAL) {
        pEnemyShotSet->param_i[2] = 0;
        // 未発射弾をすべて削除
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* next = p->next;
            if (p->param_i[0] == 0) {
                DeleteShot(p, pEnemyShotSet);
            }
            p = next;
        }
        // 再生成
        for (int i = 0; i < MINE_COUNT; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = 40.0 + GetRand(400);
            p->y = 40.0 + GetRand(400);
            p->speed = 0.0;
            p->muki = 0.0;
            p->kind = img_enemyShotSmallBall[8];
            p->param_i[0] = 0;
            p->param_d[0] = 0.0;
            p->param_d[1] = 0.0;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // プレイヤーの移動ベクトル（セット生成時に param_d に格納済み）
    double pvx = pEnemyShotSet->param_d[0];
    double pvy = pEnemyShotSet->param_d[1];

    // 接近判定と発射
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = p->next;
        if (p->param_i[0] == 0) {
            double dx = player.x - p->x;
            double dy = player.y - p->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist < DETECT_RANGE) {
                // 未来予測着弾点 (60フレーム後)
                double futureX = player.x + pvx * 60.0;
                double futureY = player.y + pvy * 60.0;
                double angle = atan2(futureY - p->y, futureX - p->x);
                p->muki = angle;
                p->speed = BULLET_SPEED;
                p->kind = img_enemyShotMediumBall[1]; // 黄色中玉に変化
                p->param_i[0] = 1; // 発射済み
            }
        }
        p = next;
    }

    // 発射済みの弾の移動
    p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (p->param_i[0] == 1) {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }
}

// -----------------------------------------------------------
// 敵本体パターン
// -----------------------------------------------------------
void EnemyPat_TheHardest_DeepSeek()
{
    static int muki;
    static int phase;               // 0:通常 1:反転
    static double prevPlayerX, prevPlayerY; // 前フレームのプレイヤー座標
    static double playerHistoryX[60]; // 60フレーム(1秒)のリングバッファ
    static double playerHistoryY[60];
    static int historyIndex;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
        prevPlayerX = player.x;
        prevPlayerY = player.y;
        historyIndex = 0;
        for (int i = 0; i < 60; ++i) {
            playerHistoryX[i] = player.x;
            playerHistoryY[i] = player.y;
        }
    }
    else {
        // 敵の移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // プレイヤーの移動ベクトル算出
    double playerVx = player.x - prevPlayerX;
    double playerVy = player.y - prevPlayerY;
    prevPlayerX = player.x;
    prevPlayerY = player.y;

    // プレイヤー位置履歴を更新（0.5秒=30フレーム用に最低限60あれば十分）
    playerHistoryX[historyIndex] = player.x;
    playerHistoryY[historyIndex] = player.y;
    historyIndex = (historyIndex + 1) % 60;

    // ----- フェーズ管理（20秒=1200フレームで反転） -----
    if (count == 1200) {
        phase = 1;
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 予告音
    }

    // ----- 1. 廻天の歯車（常時） -----
    if (count == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotRotatingGears;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    // 反転時の回転方向変更は ShotRotatingGears 内で自律制御（今回は未実装だが param_i で対応可能）

    // ----- 2. 過去の鎖（5フレームごとにレーザー生成） -----
    if (count % 5 == 0) {
        // 0.5秒前の座標（30フレーム前）
        int idx = (historyIndex - 30 + 60) % 60;
        double pastX = playerHistoryX[idx];
        double pastY = playerHistoryY[idx];

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPastChainLaser;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->param_d[0] = pastX; // 過去座標をパラメータで伝達
        pSet->param_d[1] = pastY;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ----- 3. 今際の星霜（初回のみ生成、内部で永続） -----
    if (count == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotStardustMine;
        pSet->x = 0.0;
        pSet->y = 0.0;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->param_d[0] = playerVx; // 移動ベクトルを渡す
        pSet->param_d[1] = playerVy;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        // 既存の星霜セットのプレイヤー速度情報を毎フレーム更新する（検索して更新）
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == ShotStardustMine) {
                pSet->param_d[0] = playerVx;
                pSet->param_d[1] = playerVy;
                break;
            }
            pSet = pSet->next;
        }
    }
}