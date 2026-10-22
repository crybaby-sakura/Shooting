#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：青天の霹靂
static void ShotSeitenNoHekireki(sEnemyShotSet* pEnemyShotSet)
{
    int c = pEnemyShotSet->count;

    // ==========================================
    // フェーズ1: 【青天】 (0 ~ 119)
    // 青い中円弾がゆっくりと螺旋状に広がり、油断を誘う
    // ==========================================
    if (c >= 0 && c < 120) {
        if (c == 0) {
            // 静かなる予告音
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        if (c % 4 == 0) {
            for (int i = 0; i < 4; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                // 螺旋状に広がる角度計算
                double angle = (c / 8.0) * 0.4 + (i * DX_PI / 2.0);
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = angle;
                pShot->speed = 1.2 + (c / 120.0) * 1.0; // 徐々に速くなるが、基本は遅い

                // 4:青 の中円弾を使用
                pShot->kind = img_enemyShotMediumBall[4];

                // 弾リストに追加
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }
    // ==========================================
    // フェーズ2: 【霹靂】 (120 ~ 149)
    // 唐突に自機狙いの超高速白い短レーザーが、ジグザグに軌道を歪めながら貫く
    // ==========================================
    else if (c >= 120 && c < 150) {
        if (c == 120) {
            // 雷鳴のような極大効果音で不意打ちを強調
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        // 125, 135, 145 フレーム目に稲妻を発射
        if (c == 125 || c == 135 || c == 145) {
            double base_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

            for (int i = 0; i < 3; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;

                // GetRand(20)は0~20を返す。-10して100.0で割ることで -0.1 ~ +0.1 ラジアンのばらつきを与え、
                // 複数本の稲妻が分岐したように見せる
                pShot->muki = base_muki + (GetRand(20) - 10) / 100.0;
                pShot->speed = 14.0-4; // 超高速

                // 6:白 の短レーザーを使用（長さがあり貫通感が出る）
                pShot->kind = img_enemyShotLaser[6];

                // ジグザグ移動用パラメータの初期化
                // GetRand(1)は0か1を返す。*2-1することで -1 または 1 のいずれかになる
                pShot->param_i[0] = GetRand(1) * 2 - 1;
                pShot->param_d[0] = 0.06; // 1フレームあたりの角度変化量

                // 弾リストに追加
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }
    // ==========================================
    // フェーズ3: 【残響】 (150 ~ 199)
    // 雷が落ちた衝撃で、自機周辺に黄色い小円弾が爆発的に広がる
    // ==========================================
    else if (c >= 150 && c < 200) {
        if (c == 150) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        if (c == 150) {
            for (int i = 0; i < 20; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                // 自機周辺に発生（着弾の衝撃波を表現）
                pShot->muki = (i / 20.0) * 2.0 * DX_PI;
                pShot->x = player.x + GetRand(60) - 30 + 50 * cos(pShot->muki);
                pShot->y = player.y + GetRand(60) - 30 + 50 * sin(pShot->muki);
                pShot->speed = 2.5 + GetRand(30) / 10.0;

                // 1:黄 の小円弾を使用
                pShot->kind = img_enemyShotSmallBall[1];

                // 弾リストに追加
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // ==========================================
    // 全弾の移動処理
    // ==========================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 白い短レーザー（稲妻）の場合、進行方向をランダムに揺らしてジグザグ表現にする
        if (pShot->kind >= img_enemyShotLaser[0] && pShot->kind <= img_enemyShotLaser[8]) {
            if (GetRand(2) == 0) { // 1/3の確率で向きの反転方向を切り替え
                pShot->param_i[0] *= -1;
            }
            pShot->muki += pShot->param_i[0] * pShot->param_d[0];
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_SeitenNoHekireki_Qwen()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // 敵は左右にゆっくりと往復移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 300フレーム周期で「青天の霹靂」パターンを発動
    if (count % 300 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSeitenNoHekireki;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 弾リストのヘッダ初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}