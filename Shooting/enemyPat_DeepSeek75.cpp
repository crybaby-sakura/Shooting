// EnemyPat_Pentagram_DeepSeek.cpp
// 星鎖円舞（五芒星弾幕）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <cmath>

// 前方宣言
static void PentagramStar(sEnemyShotSet* pEnemyShotSet);
static void BossLaser(sEnemyShotSet* pEnemyShotSet);

//----------------------------------------------------
// ユーティリティ：リンクリストから弾を削除（プール返却）
//----------------------------------------------------
static void RemoveBullet(sEnemyShotSet* pSet, sEnemyShot* pShot) {
    pShot->prev->next = pShot->next;
    pShot->next->prev = pShot->prev;
    delete pShot;
}

//----------------------------------------------------
// 星の線分を構成する弾を生成
//----------------------------------------------------
static void SpawnLineBullets(sEnemyShotSet* pSet) {
    const double outerRadius = 100.0;           // 基本半径
    const int    numPerLine = 20;               // 1辺あたりの弾数
    const double rot = pSet->param_d[0];      // 現在の回転角
    const double scale = pSet->param_d[1];      // 現在の拡大率
    const double cx = pSet->x;
    const double cy = pSet->y;

    // 回転0度（正立）における外側の5頂点の角度
    // 五芒星は頂点が上を向くように 18°＋72°*k に配置
    auto outerAngle = [](int k) { return (18.0 + 72.0 * k) * DX_PI / 180.0; };

    // 外側頂点の座標（スケール1, 回転0）
    double outerX[5], outerY[5];
    for (int i = 0; i < 5; ++i) {
        double a = outerAngle(i);
        outerX[i] = outerRadius * cos(a);
        outerY[i] = outerRadius * sin(a);
    }

    // 5本の線分（i → i+2 mod5）を描く
    for (int line = 0; line < 5; ++line) {
        int idx0 = line;
        int idx1 = (line + 2) % 5;

        for (int j = 0; j <= numPerLine; ++j) {
            double t = (double)j / numPerLine;   // 0.0 ～ 1.0
            double lx = outerX[idx0] + t * (outerX[idx1] - outerX[idx0]);
            double ly = outerY[idx0] + t * (outerY[idx1] - outerY[idx0]);

            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotSmallBall[3];   // シアン小玉
            p->x = cx + scale * (lx * cos(rot) - ly * sin(rot));
            p->y = cy + scale * (lx * sin(rot) + ly * cos(rot));
            p->muki = 0.0;
            p->speed = 0.0;
            p->param_i[0] = line;                  // 線分インデックス
            p->param_d[0] = t;                     // 進行度 t
            p->param_d[2] = lx;                    // 基準座標 x
            p->param_d[3] = ly;                    // 基準座標 y
            p->param_i[1] = 0;                     // 種類：線分弾

            // リンク挿入
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
}

//----------------------------------------------------
// 破裂弾（固定放射）を生成
//----------------------------------------------------
static void SpawnBurstBullets(sEnemyShotSet* pSet) {
    const double cx = pSet->x;
    const double cy = pSet->y;
    const int    num = 36;          // 10度刻み
    const double speed = 4.0;

    for (int i = 0; i < num; ++i) {
        double rad = i * (10.0 * DX_PI / 180.0);
        sEnemyShot* p = new sEnemyShot;
        p->kind = img_enemyShotSmallBall[6];   // 白小玉
        p->x = cx;
        p->y = cy;
        p->muki = rad;
        p->speed = speed;
        p->param_i[1] = 2;                     // 種類：破裂弾

        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }
}

//----------------------------------------------------
// 星のパターン関数
//----------------------------------------------------
static void PentagramStar(sEnemyShotSet* pSet) {
    // 定数
    const int   EXPAND_TIME = 240;   // 拡大時間 (約4秒)
    const int   BURST_TIME = 20;    // 破裂時間
    const int   WAIT_TIME = 60;    // 再形成待機
    const double ANGLE_STEP = 0.0087266; // 毎フレーム 0.5度 (30度/秒)

    // 初回フレーム
    if (pSet->count == 1) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pSet->param_d[0] = 0.0;      // 回転角
        pSet->param_d[1] = 0.3;      // 現在のスケール
        pSet->param_i[0] = 0;        // フェーズ 0:拡大
        pSet->param_i[1] = 1;        // フェーズ開始カウント
        SpawnLineBullets(pSet);
        return;
    }

    // フェーズと経過時間の取得
    int phase = pSet->param_i[0];
    int phaseStart = pSet->param_i[1];
    int elapsed = pSet->count - phaseStart;

    double& rot = pSet->param_d[0];
    double& scale = pSet->param_d[1];
    double cx = pSet->x, cy = pSet->y;

    // 回転更新
    rot += ANGLE_STEP;

    switch (phase) {
    case 0: { // 拡大中
        double prog = (elapsed >= EXPAND_TIME) ? 1.0 : (double)elapsed / EXPAND_TIME;
        scale = 0.3 + (1.5 - 0.3) * prog;

        if (elapsed >= EXPAND_TIME) {
            // 破裂フェーズへ
            pSet->param_i[0] = 1;
            pSet->param_i[1] = pSet->count;
            elapsed = 0; // 以降 case 1 用
            // ※縮小アニメーションのためそのまま case 1 へ移行
        }
        break;
    }
    case 1: { // 収縮 → 破裂
        double prog = (elapsed >= BURST_TIME) ? 1.0 : (double)elapsed / BURST_TIME;
        scale = 1.5 * (1.0 - prog);

        if (elapsed == BURST_TIME) {
            // 線分弾をすべて削除
            sEnemyShot* p = pSet->pEnemyShotHead->next;
            while (p != pSet->pEnemyShotHead) {
                sEnemyShot* next = p->next;
                if (p->param_i[1] == 0) RemoveBullet(pSet, p);
                p = next;
            }
            // 破裂弾を発生
            SpawnBurstBullets(pSet);
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            // 待機フェーズへ
            pSet->param_i[0] = 2;
            pSet->param_i[1] = pSet->count;
            scale = 0.0;
        }
        break;
    }
    case 2: { // 再形成待機
        if (elapsed >= WAIT_TIME) {
            // 拡大フェーズに戻る
            pSet->param_i[0] = 0;
            pSet->param_i[1] = pSet->count;
            scale = 0.3;
            // 回転は維持したまま線分を再生成
            SpawnLineBullets(pSet);
        }
        break;
    }
    }

    // 頂点狙撃弾（常時一定間隔で発射）
    if (pSet->count % 30 == 0 && scale > 0.01) { // スケールがほぼ0なら出さない
        const double R = 100.0;
        for (int i = 0; i < 5; ++i) {
            double a = (18.0 + 72.0 * i) * DX_PI / 180.0 + rot;
            double vx = cx + scale * R * cos(a);
            double vy = cy + scale * R * sin(a);

            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotSmallBall[5];   // マゼンタ小玉（赤紫）
            p->x = vx;
            p->y = vy;
            p->muki = atan2(player.y - vy, player.x - vx);
            p->speed = 1.5;
            p->param_i[1] = 1;                     // 頂点狙撃弾

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 各弾の更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[1] == 0) {   // 線分弾 → 目標位置へ瞬間移動
            double dx = pShot->param_d[2];
            double dy = pShot->param_d[3];
            double cosR = cos(rot), sinR = sin(rot);
            pShot->x = cx + scale * (dx * cosR - dy * sinR);
            pShot->y = cy + scale * (dx * sinR + dy * cosR);

            double tx = cx + scale * (dx * cosR - dy * sinR);
            double ty = cy + scale * (dx * sinR + dy * cosR);
            double dist = sqrt((tx - pShot->x) * (tx - pShot->x) +
                (ty - pShot->y) * (ty - pShot->y));
            pShot->muki = atan2(ty - pShot->y, tx - pShot->x);
            pShot->speed = dist;   // 1フレームで追いつく
        }
        else { // 頂点狙撃 or 破裂弾 → 通常移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

//----------------------------------------------------
// ボスのレーザー攻撃パターン
//----------------------------------------------------
static void BossLaser(sEnemyShotSet* pSet) {
    if (pSet->count == 1) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* p = new sEnemyShot;
        p->kind = img_enemyShotLaser[1];   // 黄色レーザー（金色）
        p->x = pSet->x;
        p->y = pSet->y;
        p->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        p->speed = 8.0;
        p->param_i[1] = -1;   // 区別不要

        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }
    // レーザー弾は通常移動で画面外へ
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//----------------------------------------------------
// 敵本体パターン（エントリ関数）
//----------------------------------------------------
void EnemyPat_Pentagram_DeepSeek()
{
    static int muki = 1;          // 左右移動の向き
    static int shotTimer = 0;     // レーザー用タイマー

    // 初回フレーム
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotTimer = 0;

        // ★五芒星パターンを一度だけ生成（以降自動で繰り返す）
        sEnemyShotSet* pStarSet = new sEnemyShotSet;
        pStarSet->count = 0;
        pStarSet->patternFunc = PentagramStar;
        pStarSet->x = 240.0;   // 画面中央
        pStarSet->y = 240.0;
        pStarSet->muki = 0.0;

        pStarSet->pEnemyShotHead = new sEnemyShot;
        pStarSet->pEnemyShotHead->prev = pStarSet->pEnemyShotHead;
        pStarSet->pEnemyShotHead->next = pStarSet->pEnemyShotHead;

        pStarSet->prev = enemyShotSetHead.prev;
        pStarSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pStarSet;
        enemyShotSetHead.prev = pStarSet;
    }
    else {
        // 敵本体の左右移動
        enemy.x += 0.98 * muki;
        if (count % 120 == 60) muki *= -1;

        // ボスレーザー（周期的に発射）
        shotTimer++;
        if (shotTimer >= 50) {
            shotTimer = 0;

            sEnemyShotSet* pLaserSet = new sEnemyShotSet;
            pLaserSet->count = 0;
            pLaserSet->patternFunc = BossLaser;
            pLaserSet->x = enemy.x;
            pLaserSet->y = enemy.y;
            pLaserSet->muki = 0.0;

            pLaserSet->pEnemyShotHead = new sEnemyShot;
            pLaserSet->pEnemyShotHead->prev = pLaserSet->pEnemyShotHead;
            pLaserSet->pEnemyShotHead->next = pLaserSet->pEnemyShotHead;

            pLaserSet->prev = enemyShotSetHead.prev;
            pLaserSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pLaserSet;
            enemyShotSetHead.prev = pLaserSet;
        }
    }
}