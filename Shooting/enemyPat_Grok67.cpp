// enemyPat_Tmp.cpp
// 無限螺旋・時間歪曲弾幕（Infinity Spiral Time Distortion）
// 世界一難しい弾幕の実装サンプル
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン関数群
// ============================================================

// 螺旋弾の基本移動＋位相による挙動制御
// param_i[0] : 弾の種別フラグ (0:通常螺旋, 1:静止後再配置, 2:黒弾, 3:不可視予告, 4:誘導微調整)
// param_i[1] : 色インデックス (0-8)
// param_i[2] : 分裂カウンタ
// param_d[0] : 基準角度オフセット
// param_d[1] : 角速度
// param_d[2] : 初期半径 or 待機時間
// param_d[3] : 加速係数
static void ShotInfinitySpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 効果音（過負荷防止のため一定間隔）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 30 == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    // 弾生成フェーズ（継続的に発生）
    // --------------------------------------------------------
    // 位相0: 初期螺旋放射 (count 0〜180)
    // 位相1: 時間歪曲（静止＋再配置） (180〜480)
    // 位相2: 無限分裂・黒弾ループ (480〜)
    // 位相3: 不可視＋未来予測強化 (900〜)

    int phase = 0;
    if (pEnemyShotSet->count >= 900) phase = 3;
    else if (pEnemyShotSet->count >= 480) phase = 2;
    else if (pEnemyShotSet->count >= 180) phase = 1;

    // 螺旋の回転速度（時間経過で加速）
    double spiralRot = pEnemyShotSet->count * 0.035;
    if (phase >= 2) spiralRot *= 1.4;
    if (phase >= 3) spiralRot *= 1.6;

    // 生成間隔を位相で調整（後半ほど高密度）
    int emitInterval = 16;
    if (phase == 1) emitInterval = 12;
    if (phase == 2) emitInterval = 8;
    if (phase == 3) emitInterval = 4;

    if (pEnemyShotSet->count % emitInterval == 0) {
        int arms = 8;                       // 基本アーム数
        if (phase >= 2) arms = 12;
        if (phase >= 3) arms = 16;

        for (int a = 0; a < arms; a++) {
            // 時計回り・反時計回りの交互アーム
            double baseAng = spiralRot + (DX_PI * 2.0 * a) / arms;
            if (a % 2 == 1) baseAng = -spiralRot + (DX_PI * 2.0 * a) / arms;

            // 1アームあたり複数層
            int layers = 1;
            if (phase >= 1) layers = 2;
            if (phase >= 2) layers = 3;

            for (int L = 0; L < layers; L++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = baseAng + L * 0.12;
                pEnemyShot->speed = 1.2 + L * 0.35 + (phase * 0.25);

                // ランダム性を少し加える（再現性あり）
                pEnemyShot->speed += GetRand(30) / 100.0;
                pEnemyShot->muki += (GetRand(20) - 10) / 180.0 * DX_PI;

                // 弾種・色選択
                int colorIdx = (pEnemyShotSet->count / 15 + a + L) % 9; // 色循環
                if (phase >= 2 && (a + L) % 5 == 0) colorIdx = 7; // 黒弾を混ぜる

                // 使用素材：小玉（高密度用）・中玉・菱形・鱗弾・短レーザーを抜粋
                if (phase == 0) {
                    pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];
                }
                else if (phase == 1) {
                    if (L == 0) pEnemyShot->kind = img_enemyShotDiamond[colorIdx];
                    else        pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];
                }
                else if (phase == 2) {
                    if (colorIdx == 7) pEnemyShot->kind = img_enemyShotMediumBall[7]; // 黒中玉
                    else if (L == 2)   pEnemyShot->kind = img_enemyShotScale[colorIdx];
                    else               pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];
                }
                else { // phase 3
                    if ((a + L) % 4 == 0) pEnemyShot->kind = img_enemyShotLaser[colorIdx];
                    else if (colorIdx == 7) pEnemyShot->kind = img_enemyShotMediumBall[7];
                    else pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];
                }

                // パラメータ初期化
                pEnemyShot->param_i[0] = 0; // 通常螺旋
                pEnemyShot->param_i[1] = colorIdx;
                pEnemyShot->param_i[2] = 0; // 分裂カウンタ
                pEnemyShot->param_d[0] = baseAng;
                pEnemyShot->param_d[1] = 0.04 + phase * 0.01; // 角速度
                pEnemyShot->param_d[2] = 0.0; // 待機用
                pEnemyShot->param_d[3] = 0.015 + phase * 0.005; // 加速係数

                // 位相1以降は一部を「静止後再配置」タイプに
                if (phase >= 1 && (a + L) % 7 == 0) {
                    pEnemyShot->param_i[0] = 1;
                    pEnemyShot->param_d[2] = 40.0 + GetRand(30); // 静止フレーム数
                    pEnemyShot->speed *= 0.6;
                }
                // 位相2以降は黒弾候補
                if (phase >= 2 && colorIdx == 7) {
                    pEnemyShot->param_i[0] = 2;
                    pEnemyShot->param_d[2] = 60.0 + GetRand(40); // 爆発までの時間
                }
                // 位相3は誘導微調整フラグ
                if (phase >= 3 && (a % 3 == 0)) {
                    pEnemyShot->param_i[0] = 4;
                }

                // リスト接続
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // --------------------------------------------------------
    // 既存弾の更新
    // --------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int type = pShot->param_i[0];

        // 色変化（赤に近づくと加速）
        int life = pShot->count;
        if (life > 90 && pShot->param_i[1] != 0 && life % 20 == 0) {
            // 徐々に赤寄りへ（簡易）
            if (pShot->param_i[1] > 0) pShot->param_i[1]--;
            // kind を再設定（色変化）
            int c = pShot->param_i[1];
            if (pShot->kind == img_enemyShotSmallBall[c + 1] || pShot->kind == img_enemyShotSmallBall[c]) {
                pShot->kind = img_enemyShotSmallBall[c];
            }
        }

        if (type == 0) {
            // 通常螺旋：徐々に加速＋緩やかな回転
            pShot->speed += pShot->param_d[3];
            pShot->muki += pShot->param_d[1] * 0.3;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (type == 1) {
            // 時間歪曲：一定時間静止 → プレイヤー中心に再配置して再加速
            if (pShot->count < (int)pShot->param_d[2]) {
                // 静止（位置固定）
            }
            else if (pShot->count == (int)pShot->param_d[2]) {
                // 再配置：プレイヤー位置を中心に少しずらす
                double dx = player.x - pShot->x;
                double dy = player.y - pShot->y;
                double dist = sqrt(dx * dx + dy * dy);
                if (dist < 1.0) dist = 1.0;
                while (true) {
                    pShot->x = player.x + (GetRand(80) - 40);
                    pShot->y = player.y + (GetRand(80) - 40);
                    if (hypot(pShot->x - player.x, pShot->y - player.y) > 50) break;
                }
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x) + (GetRand(60) - 30) / 180.0 * DX_PI;
                pShot->speed = 2.5 + GetRand(20) / 10.0;
                // 小さな誘導弾を追加生成（密度アップ）
                for (int k = 0; k < 3; k++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    pNew->muki = pShot->muki + (k - 1) * 0.4;
                    pNew->speed = pShot->speed * 0.8;
                    pNew->kind = img_enemyShotSmallBall[pShot->param_i[1]];
                    pNew->param_i[0] = 0;
                    pNew->param_i[1] = pShot->param_i[1];
                    pNew->param_d[1] = 0.02;
                    pNew->param_d[3] = 0.02;
                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;
                }
            }
            else {
                pShot->speed += 0.03;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else if (type == 2) {
            // 黒弾：一定時間後に爆発分裂
            if (pShot->count < (int)pShot->param_d[2]) {
                // ゆっくり追尾気味に移動
                double targetMuki = atan2(player.y - pShot->y, player.x - pShot->x);
                double diff = targetMuki - pShot->muki;
                while (diff > DX_PI) diff -= DX_PI * 2.0;
                while (diff < -DX_PI) diff += DX_PI * 2.0;
                pShot->muki += diff * 0.04;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else if (pShot->count == (int)pShot->param_d[2]) {
                // 爆発：超高密度微細弾を螺旋放出
                if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

                for (int i = 0; i < 6; i++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    pNew->muki = (DX_PI * 2.0 * i) / 24.0 + pShot->count * 0.1;
                    pNew->speed = 1.8 + GetRand(40) / 20.0;
                    pNew->kind = img_enemyShotSmallBall[7]; // 黒小玉
                    pNew->param_i[0] = 0;
                    pNew->param_i[1] = 7;
                    pNew->param_d[1] = 0.05;
                    pNew->param_d[3] = 0.025;
                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;
                }
                // 本体は消すために画面外へ飛ばす（メインルーチンで消去）
                pShot->x = -9999.0;
                pShot->y = -9999.0;
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else if (type == 4) {
            // 未来予測風誘導：プレイヤーの現在速度を簡易推定して先読み
            // （簡易実装：現在位置＋少し先を狙う）
            double predX = player.x;
            double predY = player.y;
            // 簡易的に前回との差は取れないので、現在方向に少しオフセット
            double toPlayer = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->muki += (toPlayer - pShot->muki) * 0.08;
            pShot->speed += 0.02;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // フォールバック
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 追加の「過去軌跡弾」セット用（プレイヤーの移動履歴を簡易再現）
static void ShotTrailTrap(sEnemyShotSet* pEnemyShotSet)
{
    // このセットは一定間隔でプレイヤー位置に遅延弾を置く
    if (pEnemyShotSet->count % 8 == 0 && pEnemyShotSet->count < 600) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        // 少し遅れて出現させるため初期位置をプレイヤーの少し後ろに
        pEnemyShot->x = player.x + (GetRand(40) - 20);
        pEnemyShot->y = player.y + (GetRand(40) - 20);
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0; // 最初は静止
        pEnemyShot->kind = img_enemyShotDiamond[5]; // マゼンタ菱形
        pEnemyShot->param_i[0] = 1; // 静止後動き出すタイプとして流用
        pEnemyShot->param_d[2] = 25.0 + GetRand(15); // 出現後の待機
        pEnemyShot->param_d[3] = 0.04;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 更新（ShotInfinitySpiral の type==1 と似た処理を簡略化）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->count < (int)pShot->param_d[2]) {
            // 待機（ほぼ見えない状態で待機）
        }
        else if (pShot->count == (int)pShot->param_d[2]) {
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 3.2;
            pShot->kind = img_enemyShotSmallBall[0]; // 赤小玉に変化して加速
        }
        else {
            pShot->speed += pShot->param_d[3];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_TheHardest_Grok()
{
    static int phaseFlag = 0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        phaseFlag = 0;

        // メイン螺旋弾幕セットを生成
        sEnemyShotSet* pMain = new sEnemyShotSet;
        pMain->count = 0;
        pMain->patternFunc = ShotInfinitySpiral;
        pMain->x = enemy.x;
        pMain->y = enemy.y;
        pMain->muki = 0.0;
        pMain->kind = 0;
        pMain->pEnemyShotHead = new sEnemyShot;
        pMain->pEnemyShotHead->prev = pMain->pEnemyShotHead;
        pMain->pEnemyShotHead->next = pMain->pEnemyShotHead;
        pMain->prev = enemyShotSetHead.prev;
        pMain->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMain;
        enemyShotSetHead.prev = pMain;
    }
    else {
        // ボスはほぼ中央固定（わずかに左右に揺れる程度）
        enemy.x = 240.0 + sin(count * 0.02) * 30.0;
        enemy.y = 100.0 + cos(count * 0.015) * 10.0;

        // 途中から軌跡トラップセットを追加
        if (count == 300 && phaseFlag == 0) {
            phaseFlag = 1;
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            sEnemyShotSet* pTrail = new sEnemyShotSet;
            pTrail->count = 0;
            pTrail->patternFunc = ShotTrailTrap;
            pTrail->x = enemy.x;
            pTrail->y = enemy.y;
            pTrail->muki = 0.0;
            pTrail->kind = 1;
            pTrail->pEnemyShotHead = new sEnemyShot;
            pTrail->pEnemyShotHead->prev = pTrail->pEnemyShotHead;
            pTrail->pEnemyShotHead->next = pTrail->pEnemyShotHead;
            pTrail->prev = enemyShotSetHead.prev;
            pTrail->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pTrail;
            enemyShotSetHead.prev = pTrail;
        }
    }
}