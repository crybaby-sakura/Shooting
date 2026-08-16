// enemyPat_Tmp.cpp
// 北風と太陽をモチーフにした弾幕パターン
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 北風：高速・青系・突風のように押し付ける弾
// 使える素材（抜粋）
//   弾種: img_enemyShotSmallBall / MediumBall / Scale / Diamond
//   色  : 3=シアン, 4=青
//   効果音: sound_enemyShot_medium, sound_enemyShot_heavy
// ------------------------------------------------------------
static void ShotNorthWind(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 突風の間隔で発射（count はメイン側でインクリメントされる）
    if (pEnemyShotSet->count % 10 == 0 && pEnemyShotSet->count < 100) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 1回の突風で5〜7発、プレイヤー方向＋ランダム散らばり
        int num = 5 + GetRand(2);
        double baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            // 発射位置を少し散らして風の厚みを出す
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(40) - 20);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(20) - 10);

            // 高速＋小さな角度散らばり（北風らしい直線的な強風）
            pEnemyShot->muki = baseMuki + (i - (num - 1) / 2.0) * 0.12
                + (GetRand(30) - 15) / 180.0 * DX_PI;
            pEnemyShot->speed = 4.8 + GetRand(25) / 10.0;   // 4.8〜7.3

            // 青 or シアンの小玉・鱗弾（風っぽい軽さ）
            int col = (GetRand(1) == 0) ? 4 : 3;
            if (GetRand(2) == 0) {
                pEnemyShot->kind = img_enemyShotSmallBall[col];
            }
            else {
                pEnemyShot->kind = img_enemyShotScale[col];
            }

            // リスト連結
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既存弾の移動（画面外消去はメインルーチン側）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        // ごく弱い横風成分を足して「押しやる」感を追加
        pShot->x += 0.35 * sin(pShot->count * 0.08);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 太陽：低速・黄橙系・じわじわと画面を埋める弾
// 使える素材（抜粋）
//   弾種: img_enemyShotMediumBall / LargeBall / MediumOval / Diamond
//   色  : 1=黄, 8=橙
//   効果音: sound_enemyShot_light, sound_enemyCharge
// ------------------------------------------------------------
static void ShotSun(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ゆっくり放射（密度を上げて逃げ場を狭める）
    if (pEnemyShotSet->count % 10 == 0 && pEnemyShotSet->count < 140) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int num = 10 + GetRand(4);          // 10〜14方向
        double startAng = pEnemyShotSet->param_d[0];  // セット作成時に決めた基準角

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->muki = startAng + i * (2.0 * DX_PI / num)
                + (GetRand(12) - 6) * 0.008;  // わずかなゆらぎ
            pEnemyShot->speed = 1.4 + GetRand(12) / 10.0;  // 1.4〜2.6（遅い）

            // 黄 or 橙の中玉・中楕円
            int col = (GetRand(1) == 0) ? 1 : 8;
            if (GetRand(3) == 0) {
                pEnemyShot->kind = img_enemyShotMediumOval[col];
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumBall[col];
            }

            // 二次弾用パラメータ（後で使う）
            pEnemyShot->param_i[0] = 0;   // まだ二次弾を出していない
            pEnemyShot->param_d[0] = pEnemyShot->muki;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 次の放射の基準角を少し回す（螺旋っぽさ）
        pEnemyShotSet->param_d[0] += 0.17;
    }

    // 既存弾の移動 ＋ 一定時間後に小さな二次弾を放射（太陽がさらに広がるイメージ）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 弾が生まれてから45フレーム後に一度だけ小さな二次弾を3方向に
        if (pShot->count == 45 && pShot->param_i[0] == 0) {
            pShot->param_i[0] = 1;
            for (int k = -1; k <= 1; k++) {
                sEnemyShot* pSec = new sEnemyShot;
                pSec->x = pShot->x;
                pSec->y = pShot->y;
                pSec->muki = pShot->param_d[0] + k * 0.55;
                pSec->speed = 2.0 + GetRand(8) / 10.0;
                pSec->kind = img_enemyShotSmallBall[(GetRand(1) == 0) ? 1 : 8];
                pSec->param_i[0] = 1;
                pSec->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSec->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSec;
                pEnemyShotSet->pEnemyShotHead->prev = pSec;
            }
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// 名前は必ず void EnemyPat_NorthWindAndSun_Grok() にすること
// ------------------------------------------------------------
void EnemyPat_NorthWindAndSun_Grok()
{
    // 初期化（count はメイン側で毎フレーム+1）
    if (count == 1) {
        // ゲーム画面 480x480 の中央上部に配置
        enemy.x = 240.0;
        enemy.y = 70.0;
        //enemy.x2 = 240.0;   // 今回は1体扱いだがサンプル互換のため同座標
        //enemy.y2 = 70.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 緩やかな左右移動（太陽のように悠然と）
    enemy.x = 240.0 + 55.0 * sin(count * 0.015);
    //enemy.x2 = enemy.x;

    // サイクル管理（約 380 フレームで1周）
    // 0〜140 : 北風
    // 141〜170 : 移行（予告音）
    // 171〜340 : 太陽
    // 341〜380 : 小休止
    int cycle = count % 420;

    // ---------- 北風フェーズ ----------
    if (cycle < 70) {
        if (cycle == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotNorthWind;

            pEnemyShotSet->x = 40.0 + GetRand(30);   // 左寄り
            pEnemyShotSet->y = 30.0 + GetRand(40);
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

            // 弾リストのダミーヘッド
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            // グローバルリストへ連結
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    else if (cycle < 140) {
        if (cycle == 71) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotNorthWind;

            pEnemyShotSet->x = 440.0 - GetRand(30);  // 右寄り
            pEnemyShotSet->y = 30.0 + GetRand(40);
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

            // 弾リストのダミーヘッド
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            // グローバルリストへ連結
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    // ---------- 移行演出 ----------
    else if (cycle == 145) {
        // 予告音だけ鳴らす
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    // ---------- 太陽フェーズ ----------
    else if (cycle >= 171 && cycle < 340) {
        if (cycle == 171) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotSun;

            // ボス本体から放射
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 8.0;
            pEnemyShotSet->muki = 0.0;   // 使わないが初期化
            pEnemyShotSet->param_d[0] = GetRand(628) / 100.0;  // 初期角度（0〜2π相当）

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    // 休止区間は何もしない（弾が自然に消えていく）
}