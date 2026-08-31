// enemyPat_Tmp.cpp
// 近接弾幕パターン：『裁縫師の刃縫い（スレッド・スラッシュ）』

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：裁縫師の刃縫い（スレッド・スラッシュ）
static void ShotThreadSlash(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;

    // 初回実行時
    if (t == 0) {
        // 突入時の効果音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 【フェーズ1】斬撃の軌跡を残す（約2秒間）
    if (t < 120) {
        // 5フレームごとに斬撃（弾）を生成
        if (t % 1 == 0) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // ボスの現在位置を基準に、少しブレさせて「軌跡」感を演出
            pEnemyShot->x = enemy.x;
            pEnemyShot->y = enemy.y;

            // プレイヤー方向を基本とし、±20度ほどばらけさせる
            double baseMuki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            pEnemyShot->muki = baseMuki;

            // 最初は速度0でその場に留まり、「斬撃の残像（糸）」として機能させる
            pEnemyShot->speed = 0.0;

            // 弾の種類：短レーザー（マゼンタ）で「刃の軌跡」を表現
            pEnemyShot->kind = img_enemyShotLaser[5];
            pEnemyShot->margin = 480;

            // 連結処理
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 【フェーズ2】糸の収束（フィニッシュ）
    else if (t == 120) {
        // 収束開始時の効果音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // すでに存在するすべての弾をプレイヤーに向かって収束させる
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 向きを現在のプレイヤー位置へ再計算
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);

            // 速度を上げて収束させる（個体差をつけて少しばらけさせる）
            pShot->speed = 3.0 - hypot(player.x - pShot->x, player.y - pShot->y) * 0.013;

            // 弾の種類を「銃弾」に変更し、「糸が針になって飛んでくる」演出と当たり判定の最適化を図る
            pShot->kind = img_enemyShotBullet[5];

            pShot = pShot->next;
        }
    }

    // 弾の移動処理（フェーズ1ではspeed=0なので動かず、フェーズ2で動く）
    sEnemyShot* pShotMove = pEnemyShotSet->pEnemyShotHead->next;
    while (pShotMove != pEnemyShotSet->pEnemyShotHead) {
        pShotMove->x += pShotMove->speed * cos(pShotMove->muki);
        pShotMove->y += pShotMove->speed * sin(pShotMove->muki);

        pShotMove = pShotMove->next;
    }
}

// 敵本体のパターン
void EnemyPat_CloseCombat_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // やや硬めに設定
    }

    const int T = 300;
    int countT = count % T;

    // 【移動ロジック】
    if (countT < 60) {
        // 待機動作：画面中央上部でゆっくり左右に揺れる
        enemy.x = 240.0 + sin(countT / 30.0) * 100.0;
        enemy.y = 80.0 + cos(countT / 40.0) * 10.0;
    }
    else if (countT == 60) {
        // 攻撃予告
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else if (countT >= 120 && countT < 240) {
        // 近接周回動作：プレイヤーの周囲を狭い半径で高速周回する
        // 半径は時間とともに縮小し、最大で半径70まで接近する（圧迫感）
        double radius = 170.0 - (countT - 120) * 0.8;
        if (radius < 70.0) radius = 70.0;

        double angle = (countT - 120) * 0.15; // 周回速度

        enemy.x = player.x + cos(angle) * radius;
        enemy.y = player.y + sin(angle) * radius;

        // 攻撃開始トリガー（1回のみ実行）
        if (countT == 120) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotThreadSlash;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = 0.0;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    else if (countT >= 240) {
        // 攻撃終了後の退避動作：初期位置へゆっくり戻る
        enemy.x += (240.0 - enemy.x) * 0.05;
        enemy.y += (80.0 - enemy.y) * 0.05;        
    }
}