// enemyPat_sampleForAI.cpp 用の追加実装
// 既存の ShotScatter や EnemyPat_SampleForAI は削除またはコメントアウトし、
// 以下のコードに置き換えて使用してください。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：フーリエの断頭台（確率雲と波動干渉）
static void ShotFourierGuillotine(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 予告音：確率雲の生成
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 確率雲の生成: 画面広範囲に位相を持った弾を配置
        for (int i = 0; i < 48; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            // 画面外や広範囲から出現させる
            double angle = (double)i / 48.0 * DX_PI * 2.0;
            double radius = 300.0 + GetRand(100); // GetRand(100)は0~100を返す
            pShot->x = pEnemyShotSet->x + cos(angle) * radius;
            pShot->y = pEnemyShotSet->y + sin(angle) * radius;

            // 初期は自機方向へ非常にゆっくり進む（確率雲）
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 0.8;

            // 位相パラメータ (0.0 ~ 2π)
            // GetRand(628)は0~628を返すので、100.0で割って 0.00~6.28 に正規化
            pShot->param_d[0] = (double)GetRand(628) / 100.0;

            // 確率雲は白の小さな弾 (色6:白)
            pShot->kind = img_enemyShotSmallBall[6];
            pShot->param_i[0] = 0;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double dx = player.x - pShot->x;
        double dy = player.y - pShot->y;
        double dist = sqrt(dx * dx + dy * dy);

        // 観測効果：自機が一定距離(150px)以内に入ると弾が「確定」し、挙動が変化する
        if (dist < 150.0 && pShot->param_i[0] == 0) {
            pShot->speed = 2.5; // 確定して加速
            pShot->param_i[0] = 1;
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 移動処理
        if (pShot->param_i[0] == 0) {
            // 確率雲状態なら、自機への誘導を徐々に強める
            if (pShot->speed < 2.0) {
                double target_muki = atan2(dy, dx);
                double diff = target_muki - pShot->muki;
                while (diff > DX_PI) diff -= DX_PI * 2.0;
                while (diff < -DX_PI) diff += DX_PI * 2.0;
                pShot->muki += diff * 0.02;
            }
        }
        else if (pShot->param_i[0] == 2) {
            // リストから削除してメモリプールへ返却
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            sEnemyShot* nextShot = pShot->next;
            delete pShot;
            pShot = nextShot;
            continue; // 削除したので次の要素へ
        }
        else if (pShot->param_i[0] == 3) {
            // 自機を強く追尾する性質を与える（回避不可能な圧力）
            double target_muki = atan2(dy, dx);
            double diff = target_muki - pShot->muki;
            while (diff > DX_PI) diff -= DX_PI * 2.0;
            while (diff < -DX_PI) diff += DX_PI * 2.0;
            pShot->muki += diff * 0.15;
        }

        // 確定した弾は波動干渉の影響を受ける
        if (pShot->param_i[0] == 1) {
            // 干渉計算：自機との距離と固有位相による正弦波
            double interference = sin(dist * 0.15 + pShot->param_d[0]);

            if (interference < -0.95) {
                // 弱め合い：干渉により弾が相殺され消滅する（TASが目指す安全地帯）
                // 青い鱗弾に変化させてから消滅（視覚的フィードバック）
                pShot->kind = img_enemyShotScale[4]; // 色4:青
                pShot->speed = 0.0;
                pShot->param_i[0] = 2;
            }
            else if (interference > 0.95) {
                // 強め合い：干渉により高密度・即死判定の弾となる（TASが絶対に避けるべき領域）
                pShot->kind = img_enemyShotLargeBall[0]; // 色0:赤
                pShot->speed = 5.0; // 超高速化
                pShot->param_i[0] = 3;
            }
            else {
                // 中間：通常の確定弾（白の中玉）
                pShot->kind = img_enemyShotMediumBall[6];
                pShot->param_i[0] = 4;
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_TAS_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // TAS用にやや高めのHP
    }
    else {
        // ボス本体がリサージュ曲線を描いて移動し、波源そのものが動くことで
        // TASに「動的な定在波の節」の計算を強いる
        enemy.x = 240.0 + sin(count * 0.04) * 160.0;
        enemy.y = 60.0 + cos(count * 0.06) * 30.0;
    }

    // 一定間隔で確率雲を生成
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFourierGuillotine;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
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