// EnemyPat_HugeBullet_DeepSeek.cpp
// 超巨大環状弾「縮檻 -シュクコウ-」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//--------------------------------------------------------------
// リング収縮パターン
//--------------------------------------------------------------
static void RingShrinkPattern(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }

    // --- 初回のみの初期化（リング弾生成） ---
    if (pSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // リング中心は出現時のプレイヤー座標
        pSet->param_d[0] = player.x;
        pSet->param_d[1] = player.y;

        const double R0 = 204.0;              // 初期半径（480*0.85/2）
        const double shrinkSpeed = (R0 - 10.0) / 360.0; // 6秒(360F)で最小半径10へ
        pSet->param_d[2] = R0;                // 現在半径
        pSet->param_d[3] = shrinkSpeed;       // 収縮速度（正の値、後で減算）
        pSet->param_d[4] = 0.0;               // 射出点回転オフセット
        pSet->param_i[0] = 12;                // 射出点の数（12方向）
        pSet->param_i[1] = 0;                 // 発射カウンタ（交互フラグ等に使用）
        pSet->param_i[2] = 72;                // リング弾の数（5°刻み）
        pSet->param_i[3] = 0;                 // 爆散済みフラグ (0:未, 1:爆散済)

        const double cx = pSet->param_d[0];
        const double cy = pSet->param_d[1];
        const double R = pSet->param_d[2];
        const int ringNum = pSet->param_i[2];

        // リングを構成する大玉（白）を円周上に配置
        for (int i = 0; i < ringNum; ++i) {
            sEnemyShot* p = new sEnemyShot;
            double angle = (2.0 * DX_PI * i) / ringNum;
            p->x = cx + R * cos(angle);
            p->y = cy + R * sin(angle);
            p->muki = angle;                  // 角度を保存（位置更新で使用）
            p->speed = 0.0;
            p->kind = img_enemyShotLargeBall[6]; // 白の大玉
            p->param_i[0] = 1;                // 役割: 1=リング弾
            p->param_d[0] = angle;            // 固定角度

            // リストに追加
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }

        // 最初の発射は少し間を置く（例：30F後）
        pSet->param_i[1] = -30; // 負の値で待機
    }

    // --- 爆散後は何もしない ---
    if (pSet->param_i[3] == 1) return;

    // --- 現在のパラメータ取得 ---
    double cx = pSet->param_d[0];
    double cy = pSet->param_d[1];
    double R = pSet->param_d[2];
    double shrink = pSet->param_d[3];
    double& offset = pSet->param_d[4];
    int shotNum = pSet->param_i[0];
    int& timer = pSet->param_i[1];

    // --- 収縮処理 ---
    R -= shrink;
    if (R < 10.0) {
        // 最小半径に達した → 爆散
        R = 10.0;

        // 爆散：まずリング弾をすべてリストから削除
        sEnemyShot* head = pSet->pEnemyShotHead;
        sEnemyShot* p = head->next;
        while (p != head) {
            sEnemyShot* next = p->next;
            if (p->param_i[0] == 1) { // リング弾
                // リストから外して削除
                p->prev->next = p->next;
                p->next->prev = p->prev;
                delete p;
            }
            p = next;
        }

        // 爆発四散する大粒弾（大玉・赤）を全方位に発射
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int blowNum = 36; // 36方向
        for (int i = 0; i < blowNum; ++i) {
            sEnemyShot* b = new sEnemyShot;
            double angle = (2.0 * DX_PI * i) / blowNum;
            b->x = cx + R * cos(angle);
            b->y = cy + R * sin(angle);
            b->muki = angle;
            b->speed = 3.5; // 中速
            b->kind = img_enemyShotLargeBall[0]; // 赤の大玉
            b->param_i[0] = 2; // 爆散弾

            b->prev = head->prev;
            b->next = head;
            head->prev->next = b;
            head->prev = b;
        }

        pSet->param_i[3] = 1; // 爆散済みフラグ
        return;
    }
    pSet->param_d[2] = R; // 更新

    // --- リング弾の位置を更新 ---
    {
        sEnemyShot* head = pSet->pEnemyShotHead;
        sEnemyShot* p = head->next;
        while (p != head) {
            if (p->param_i[0] == 1) { // リング弾
                double angle = p->param_d[0];
                p->x = cx + R * cos(angle);
                p->y = cy + R * sin(angle);
                // 見た目の角度情報はそのまま
            }
            p = p->next;
        }
    }

    // --- 射出点オフセットをゆっくり回転 ---
    offset += 0.015; // 約1.5°ずつ回転

    // --- 射出タイミング管理 ---
    timer++;
    if (timer < 0) return; // まだ待機中
    if (timer % 8 != 0) return; // 8フレームごとに発射
    // timerの値自体で交互を判定（偶数回目の発射か奇数回目か）
    int wave = (timer / 8) % 2; // 0: 自機狙い針弾, 1: 固定放射丸弾

    // 軽い発射音
    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

    // 各射出点から発射
    for (int i = 0; i < shotNum; ++i) {
        double baseAngle = (2.0 * DX_PI * i) / shotNum + offset;
        double sx = cx + R * cos(baseAngle);
        double sy = cy + R * sin(baseAngle);

        if (wave == 0) {
            // --- 自機狙い3way針弾（菱形弾・赤） ---
            double aim = atan2(player.y - sy, player.x - sx);
            for (int j = -1; j <= 1; ++j) {
                sEnemyShot* p = new sEnemyShot;
                p->x = sx;
                p->y = sy;
                p->muki = aim + j * (10.0 / 180.0 * DX_PI); // ±10度
                p->speed = 0.1;
                p->kind = img_enemyShotDiamond[0]; // 赤の菱形弾
                p->param_i[0] = 0; // 小型弾
                p->margin = 5;
                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
        }
        else {
            // --- 固定角度の奇数弾（放射状に中速丸弾・黄） ---
            sEnemyShot* p = new sEnemyShot;
            p->x = sx;
            p->y = sy;
            p->muki = baseAngle; // 射出点の外向き角度
            p->speed = 2.2;
            p->kind = img_enemyShotSmallBall[1]; // 黄の小玉
            p->param_i[0] = 0;
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
}


//--------------------------------------------------------------
// 敵本体パターン
//--------------------------------------------------------------
void EnemyPat_HugeBullet_DeepSeek()
{
    static int muki = 1;
    
    // 初回初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }

    // 敵の動き（サンプル通り左右往復）
    {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定タイミングで「縮檻」を一度だけ発動
    if (count % 600 == 80) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = RingShrinkPattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // パラメータ初期値（RingShrinkPattern内で上書きするが念のため）
        pSet->param_d[0] = 0.0;
        pSet->param_d[1] = 0.0;
        pSet->param_d[2] = 0.0;
        pSet->param_i[0] = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 全体リストに接続
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}