#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン定義：10年の壁
// ============================================================

// パターン1: 因果律の先回り (Predictive)
// プレイヤーの現在位置を「過去」として扱い、その周辺を冷徹な計算で埋める
static void Shot_Predictive(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // プレイヤーの現在位置を基準に、狭い角度で高密度にばら撒く
        double base_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        for (int i = 0; i < 16; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;

            // 狭い角度に集中させる (-10度 ~ +10度 のばらつき)
            pShot->muki = base_muki + (GetRand(20) - 10) / 180.0 * DX_PI;
            // 遅めの速度で、プレイヤーに「避けられる」と錯覚させつつ、密度で詰める
            pShot->speed = 1.2 + GetRand(80) / 100.0;

            // 菱形弾(シアン:3) で、冷徹な計算と予測を視覚的に表現
            pShot->kind = img_enemyShotDiamond[3];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// パターン2: 未来の残像 (Omen)
// プレイヤーの周囲に予兆を配置し、定石である「隙間での待機」を殺害する
static void Shot_Omen(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // プレイヤーの周囲円周上に「予兆」を配置
        for (int i = 0; i < 8; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = (i / 8.0) * 2.0 * DX_PI;
            pShot->x = player.x + cos(angle) * 70.0;
            pShot->y = player.y + sin(angle) * 70.0;
            pShot->muki = angle + DX_PI; // 中心(プレイヤー)に向かう
            pShot->speed = 0.4; // 最初は非常に遅く、じわじわと収縮する

            // 大玉(赤:0) で、直感的な「危険」を知らせるが、これは罠
            pShot->kind = img_enemyShotLargeBall[0];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // カウントが30に達したら、「予兆」が「殺意」を持って急加速する
        if (pEnemyShotSet->count == 30) {
            pShot->speed = 5.0;
            // 鱗弾(黒:7) に変化し、視覚的にも「飲み込まれる」印象を与える
            pShot->kind = img_enemyShotScale[7];
        }
        pShot = pShot->next;
    }
}

// パターン3: フラクタル・ノイズ (Fractal Noise)
// 数学的規則性により、滑らかな回避軌道を封じ、カオスな移動(ノイズ)を強制する
static void Shot_FractalNoise(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 黄金角 (約137.5度) を用いて、自然界のパターンを模倣した渦巻きを生成
        double golden_angle = 137.508 * DX_PI / 180.0;
        int num_shots = 32;

        for (int i = 0; i < num_shots; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;

            // 黄金角で角度をずらしながら発射
            pShot->muki = (i * golden_angle) + pEnemyShotSet->muki;
            // 速度に段階を持たせ、複数の「層」を作って滑らかな移動を封じる
            pShot->speed = 1.8 + (i % 4) * 0.6;

            // 小玉(白:6) と 中楕円弾(マゼンタ:5) を交互に配置し、視覚的ノイズを増幅
            if (i % 2 == 0) {
                pShot->kind = img_enemyShotSmallBall[6];
            }
            else {
                pShot->kind = img_enemyShotMediumOval[5];
            }

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
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
// 敵本体のパターン：10年の壁
// ============================================================
void EnemyPat_TenYears_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵の移動: 通常の往復に加え、下位ビットによる微細な「震え(ノイズ)」を加え、
        // プレイヤーの照準合わせをわずかに困難にする
        enemy.x += 0.95 * (double)muki + (sin(count * 0.5) * 0.3);

        // 画面端での折り返し
        if (enemy.x < 60.0 || enemy.x > 420.0) {
            muki *= -1;
        }
    }

    // HPに応じたフェーズ遷移と弾幕発射
    // フェーズ1: HP 200 ~ 134 (因果律の先回り)
    if (enemy.hp > 133) {
        if (count % 45 == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = Shot_Predictive;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 15.0;
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShotSet->kind = shot_count++;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    // フェーズ2: HP 133 ~ 67 (未来の残像)
    else if (enemy.hp > 66) {
        if (count % 60 == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = Shot_Omen;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 15.0;
            pEnemyShotSet->muki = 0.0; // 予兆はプレイヤー位置基準で生成するため、ここでは不使用
            pEnemyShotSet->kind = shot_count++;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    // フェーズ3: HP 66 ~ 1 (フラクタル・ノイズ)
    else {
        if (count % 30 == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = Shot_FractalNoise;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 15.0;
            // プレイヤー方向を基準に渦巻きの初期角度を定める
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShotSet->kind = shot_count++;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}