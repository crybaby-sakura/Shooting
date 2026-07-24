// enemyPat_tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <cmath>

// ヘルパー：コア槍弾（シアンレーザー）の8方向生成
static void SpawnSpearBurst(sEnemyShotSet* pSet, double bx, double by)
{
    const double PI = 3.14159265358979;
    double base = atan2(player.y - by, player.x - bx);
    for (int i = 0; i < 8; ++i) {
        sEnemyShot* p = new sEnemyShot;
        p->kind = img_enemyShotLaser[3];          // シアンの短レーザー（槍状）
        p->x = bx;
        p->y = by;
        p->muki = base + i * (PI / 4.0);          // 8方向均等
        p->speed = 1.8 + GetRand(40) / 100.0;     // 1.8～2.2
        // param_d[1] にカーブ用の角速度を格納（すべて時計回り）
        p->param_d[1] = (1.5 + GetRand(10) * 0.1) * 0.01; // 0.015～0.025 rad/frame
        p->margin = 240;
        // 双方向リストへ追加
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }
}

// メインパターン関数
static void ChronosSpiralPattern(sEnemyShotSet* pSet)
{
    const double PI = 3.14159265358979;
    const double boss_x = pSet->x;  // 240
    const double boss_y = pSet->y;  // 240
    const int INTERVAL_SPEAR = 48;  // 0.8秒 (60fps想定)
    const int INTERVAL_SPIRAL = 4;  // 渦巻き弾の追加周期

    const int T0 = 300, T1 = 30, T2 = 240;

    // ----- 初回フレーム（count==0）で初期弾を大量投入し、完成した渦巻きを即座に描く -----
    if (pSet->count == 0) {
        // 開始音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 最初の槍弾バースト
        SpawnSpearBurst(pSet, boss_x, boss_y);

        // あらかじめ時間をさかのぼった渦巻き弾（マゼンタ / 黄金）を生成
        for (int layer = 0; layer < 2; ++layer) {
            int color = (layer == 0) ? 5 : 8; // マゼンタ / オレンジ
            int kind = (layer == 0) ? img_enemyShotDiamond[5] : img_enemyShotSmallBall[8];
            for (int k = 0; k < 15; ++k) {
                double base_angle = 2.0 * PI * k / 15.0;
                // age_offset = -240 ～ 0 まで 4フレーム刻みでさかのぼって配置
                for (int age_offset = -240; age_offset <= 0; age_offset += 4) {
                    sEnemyShot* p = new sEnemyShot;
                    p->kind = kind;
                    p->param_i[0] = k;           // 生成点インデックス
                    p->param_i[1] = layer;       // 0:マゼンタ, 1:黄金
                    p->param_i[2] = age_offset;  // 仮想の発生時刻（countからの相対値）
                    p->param_d[0] = base_angle;  // 初期角度
                    // リスト追加
                    p->prev = pSet->pEnemyShotHead->prev;
                    p->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = p;
                    pSet->pEnemyShotHead->prev = p;
                }
            }
        }
    }

    // ----- 定期的な槍弾バースト -----
    if (pSet->count % INTERVAL_SPEAR == 0 && pSet->count > 0 && pSet->count < T0) {
        SpawnSpearBurst(pSet, boss_x, boss_y);
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // ----- 渦巻き弾を毎フレーム少しずつ補給 -----
    if (pSet->count % INTERVAL_SPIRAL == 0) {
        for (int k = 0; k < 15; ++k) {
            double base_angle = 2.0 * PI * k / 15.0;
            // マゼンタ
            sEnemyShot* pM = new sEnemyShot;
            pM->kind = img_enemyShotDiamond[5];
            pM->param_i[0] = k; pM->param_i[1] = 0; pM->param_i[2] = 0;
            pM->param_d[0] = base_angle;
            pM->prev = pSet->pEnemyShotHead->prev;
            pM->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pM;
            pSet->pEnemyShotHead->prev = pM;
            // 黄金
            sEnemyShot* pG = new sEnemyShot;
            pG->kind = img_enemyShotSmallBall[8];
            pG->param_i[0] = k; pG->param_i[1] = 1; pG->param_i[2] = 0;
            pG->param_d[0] = base_angle;
            pG->prev = pSet->pEnemyShotHead->prev;
            pG->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pG;
            pSet->pEnemyShotHead->prev = pG;
        }
    }

    // ----- 全弾の運動更新 -----
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 槍弾（シアンレーザー）は速度＋曲率で移動
        if (pShot->kind == img_enemyShotLaser[3]) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->muki += pShot->param_d[1];   // 時計回りのカーブ
        }
        // 渦巻き弾（菱形・小玉）は数式に従って位置を直接計算
        else if (pShot->kind == img_enemyShotDiamond[5] || pShot->kind == img_enemyShotSmallBall[8]) {
            int layer = pShot->param_i[1];
            int age = pSet->count - pShot->param_i[2];   // 仮想の経過フレーム数
            if (age < 0) age = 0;

            double r0 = 120.0;                  // 渦の起点（円周半径）
            double b = 0.0661;                 // 対数螺旋の巻き係数（2.5巻き）
            double radial_speed, rot_period, rot_dir;
            if (layer == 0) {                   // マゼンタ螺旋
                radial_speed = 0.92;
                rot_period = 3.0;             // 3秒で1回転（時計回り）
                rot_dir = 1.0;
            }
            else {                            // 黄金螺旋
                radial_speed = 0.92 * 1.5;      // 1.5倍の速度
                rot_period = 2.0;             // 2秒で1回転（反時計回り）
                rot_dir = -1.0;
            }

            double r = r0 + radial_speed * age;
            if (r < r0) r = r0;

            // パターン全体の回転角（経過時間に比例）
            double pattern_rot = rot_dir * 2.0 * PI * (age / 60.0) / rot_period;
            // 対数螺旋による追加角度
            double spiral_theta = (1.0 / b) * log(r / r0);
            // 最終的な角度
            double angle = pShot->param_d[0] + pattern_rot + spiral_theta;

            pShot->x = boss_x + r * cos(angle);
            pShot->y = boss_y + r * sin(angle);
        }
        pShot = pShot->next;
    }

    // ----- 10秒ごとの「時空歪曲」エフェクト -----
    {
        int t = pSet->count;
        double factor = 0.0;   // 0のときは影響なし

        if (t >= T0 && t < T0 + T1) {                // 0.3秒間の吸い込み
            double progress = (double)(t - T0) / T1;
            factor = -0.03 * sin(progress * PI / 2.0);   // 最大30%引き寄せ
        }
        else if (t >= T0 + T1 && t < T0 + T1 + T2) {         // 続く0.5秒間の反動拡散
            double progress = (double)(t - T0 - T1) / T2;
            factor = 0.05 * (1.0 - progress);           // 減衰しながら外へ
        }

        if (factor != 0.0) {
            // 全弾をボス中心からの距離に応じて内外へオフセット
            sEnemyShot* p = pSet->pEnemyShotHead->next;
            while (p != pSet->pEnemyShotHead) {
                double dx = p->x - boss_x;
                double dy = p->y - boss_y;
                double len = sqrt(dx * dx + dy * dy);
                if (len > 0.0) {
                    double shift = len * factor;
                    p->x += dx / len * shift;
                    p->y += dy / len * shift;
                }
                p = p->next;
            }
        }

        // 歪曲開始時にチャージ音
        if (t == T0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }
}

// 敵本体のパターン（呼び出し名は固定）
void EnemyPat_ThumbnailFriendly_DeepSeek()
{
    if (count == 1) {
        // スクリーン中央に固定（サムネイル用の幾何学対称構図）
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // このパターンでは敵は移動しない（静止）

    if (count % 500 == 30) {
        // メインの弾幕セットをひとつだけ生成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ChronosSpiralPattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // 弾リストのダミーヘッド
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルリストへ接続
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}