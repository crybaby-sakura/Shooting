// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：彩り三色・団子の逆転螺旋（複雑ギミック版）
static void ShotDangoSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 予告音（発射60フレーム前に鳴らす）
    if (pEnemyShotSet->count == -60) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // カウントが0になってから発射開始
    if (pEnemyShotSet->count >= 0) {
        // 3フレームごとに発射
        if (pEnemyShotSet->count % 6 == 0) {
            if (pEnemyShotSet->count % 12 == 0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            // 回転速度のうねり（正弦波で自動的に加減速・反転を繰り返す）
            // 周期は 2*PI / 0.005 ≒ 1256フレームで1周
            double rot_speed = 0.045 * cos(pEnemyShotSet->count * 0.005);

            // 微小な高周波の揺らぎを追加し、プレイヤーの視線と回避を攪乱する
            double base_angle = pEnemyShotSet->count * rot_speed
                + sin(pEnemyShotSet->count * 0.023) * 0.8
                + cos(pEnemyShotSet->count * 0.017) * 0.5;

            double base_x = pEnemyShotSet->x;
            double base_y = pEnemyShotSet->y;

            // 2つのグループで位相をずらす（合計10方向の放射）
            for (int group = 0; group < 2; group++) {
                double group_offset = group * (DX_PI / 5.0);

                // 5方向に発射
                for (int i = 0; i < 5; i++) {
                    double m = base_angle + group_offset + i * (DX_PI * 2.0 / 5.0);

                    // ==========================================
                    // ピンク弾（一番手前、遅い）
                    // 初速2.3で前方に配置。白と緑に徐々に抜かれ、最後尾へ逆転する。
                    // ==========================================
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = base_x + 8.0 * cos(m);
                    pEnemyShot->y = base_y + 8.0 * sin(m);
                    pEnemyShot->muki = m;
                    pEnemyShot->speed = 2.3;
                    pEnemyShot->kind = img_enemyShotMediumBall[5]; // 5:マゼンタ(ピンク)
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

                    // ==========================================
                    // 白弾（真ん中、中速度）
                    // 初速2.5で中央に配置。発射直後は綺麗な団子の中心。
                    // ==========================================
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = base_x;
                    pEnemyShot->y = base_y;
                    pEnemyShot->muki = m;
                    pEnemyShot->speed = 2.5;
                    pEnemyShot->kind = img_enemyShotMediumBall[6]; // 6:白
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

                    // ==========================================
                    // 緑弾（一番奥、早い）
                    // 初速2.7で後方に配置。前方のピンクと白を追い抜き、先頭へ逆転する。
                    // ==========================================
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = base_x - 8.0 * cos(m);
                    pEnemyShot->y = base_y - 8.0 * sin(m);
                    pEnemyShot->muki = m;
                    pEnemyShot->speed = 2.7;
                    pEnemyShot->kind = img_enemyShotMediumBall[2]; // 2:緑
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_TricolorDango_Zai()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 出現と同時にセットし、60フレームの予告時間を設ける
    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = -60; // マイナス値で初期化（予告音用のタイマーとして機能）
        pEnemyShotSet->patternFunc = ShotDangoSpiral;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}