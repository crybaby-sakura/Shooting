// enemyPat_tmp.cpp
// 三色団子・串輪舞 (Three-Color Dumpling Skewer Wheel Dance)

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//---------------------------------------------------------
// 弾幕パターン関数（shot set に紐づく）
//---------------------------------------------------------
static void ShotDangoSkewer(sEnemyShotSet* pEnemyShotSet)
{
    // ----- 定数 -----
    const int    SKEWERS = 18;                     // 串の本数
    const int    COLORS = 3;                     // 色数（ピンク・白・緑）
    const double RADIUS[3] = { 40.0, 80.0, 120.0 }; // 中心からの距離
    const double BASE_SPEED = 0.025;                 // 基準角速度（rad/フレーム）
    const double SPEED_OFFSET[3] = { -0.008, 0.0, 0.008 }; // 色ごとの速度差（ピンクが遅く、緑が速い）
    const int    ROTATE_FRAMES = 120;                   // 回転フェーズの長さ

    // ----- shot set 内の状態変数（param_i/param_d を利用） -----
    int& phase = pEnemyShotSet->param_i[0]; // 0:回転, 1:飛散
    int& loopCount = pEnemyShotSet->param_i[1]; // ループ回数
    int& pelletSpawned = pEnemyShotSet->param_i[2]; // 飛散後に発生させた粒弾の数
    int& pelletArrived = pEnemyShotSet->param_i[3]; // 中心に戻った粒弾の数
    double& centerX = pEnemyShotSet->param_d[0]; // 弾幕の中心X
    double& centerY = pEnemyShotSet->param_d[1]; // 弾幕の中心Y
    double& rotationStart = pEnemyShotSet->param_d[3]; // 回転開始時のカウント

    // ----- 初期化 -----
    if (pEnemyShotSet->count == 0) {
        phase = 0;
        loopCount = 0;
        pelletSpawned = 0;
        pelletArrived = 0;
        centerX = pEnemyShotSet->x;
        centerY = pEnemyShotSet->y;
        rotationStart = 0.0;

        // 効果音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double baseBaseAngle = GetRand(359) / 360.0 * DX_PI * 2;

        // 6本の串×3色＝18個の団子弾を生成
        for (int i = 0; i < SKEWERS; ++i) {
            double baseAngle = i * 2.0 * DX_PI / SKEWERS + baseBaseAngle;
            for (int j = 0; j < COLORS; ++j) {
                sEnemyShot* p = new sEnemyShot;

                p->x = centerX + RADIUS[j] * cos(baseAngle);
                p->y = centerY + RADIUS[j] * sin(baseAngle);
                p->muki = 0.0;
                p->speed = 0.0;
                // 色：0→ピンク(マゼンタ), 1→白, 2→緑
                p->kind = (j == 0) ? img_enemyShotMediumBall[5] :
                    (j == 1) ? img_enemyShotMediumBall[6] :
                    img_enemyShotMediumBall[2];
                p->margin = 20.0;

                // 弾固有のパラメータ
                p->param_i[0] = j;                // 色番号
                p->param_i[1] = i;                // 串番号
                p->param_i[2] = 0;                // 状態 0:回転中
                p->param_d[0] = baseAngle;        // 基準角度
                p->param_d[1] = SPEED_OFFSET[j];  // 角速度オフセット
                p->param_d[2] = RADIUS[j];        // 中心からの距離

                // リストに追加
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
        return;
    }

    // ----- 毎フレーム処理：全弾の状態を更新 -----
    double elapsed = pEnemyShotSet->count - rotationStart; // 回転経過時間

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next; // 削除の可能性に備えて次を保持
        int state = pShot->param_i[2];

        // ---- 状態：回転中 ----
        if (state == 0 && phase == 0) {
            double angle = pShot->param_d[0] + (BASE_SPEED + pShot->param_d[1]) * elapsed;
            pShot->x = centerX + pShot->param_d[2] * cos(angle);
            pShot->y = centerY + pShot->param_d[2] * sin(angle);
            pShot->speed = 0.0;
        }
        // ---- 状態：飛散中 ----
        else if (state == 1) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 画面端に到達したら停止し、粒弾を吐き出す
            if (pShot->x < 20.0 || pShot->x > 460.0 ||
                pShot->y < 20.0 || pShot->y > 460.0) {
                pShot->speed = 0.0;
                pShot->param_i[2] = 2;   // 状態：端到達済み
                pShot->margin = 9999.0;  // 自動消去を防ぐ

                // あんこ玉（赤い小粒弾）を生成
                sEnemyShot* pPellet = new sEnemyShot;
                pPellet->x = pShot->x;
                pPellet->y = pShot->y;
                pPellet->muki = atan2(centerY - pPellet->y, centerX - pPellet->x);
                pPellet->speed = 2.0;
                pPellet->kind = img_enemyShotSmallBall[0]; // 赤
                pPellet->margin = 9999.0;
                pPellet->param_i[2] = 3; // 状態：粒弾（帰還中）

                // リストに追加
                pPellet->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pPellet->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pPellet;
                pEnemyShotSet->pEnemyShotHead->prev = pPellet;

                ++pelletSpawned;
            }
        }
        // ---- 状態：粒弾（中心へ帰還中） ----
        else if (state == 3) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            double dist = hypot(pShot->x - centerX, pShot->y - centerY);
            if (dist < 5.0) {
                pShot->speed = 0.0;
                pShot->param_i[2] = 4; // 状態：帰還完了
                ++pelletArrived;
            }
        }
        // その他の状態 (2:端で停止中, 4:帰還済み) は何もしない

        pShot = nextShot;
    }

    // ----- フェーズ遷移：回転 → 飛散 -----
    if (phase == 0 && elapsed >= ROTATE_FRAMES) {
        phase = 1;
        // 全回転弾を飛散状態に切り替え
        sEnemyShot* pTemp = pEnemyShotSet->pEnemyShotHead->next;
        while (pTemp != pEnemyShotSet->pEnemyShotHead) {
            if (pTemp->param_i[2] == 0) { // まだ回転中なら
                double angle = pTemp->param_d[0] + (BASE_SPEED + pTemp->param_d[1]) * elapsed;
                // 接線方向（反時計回り）へ飛ばす
                pTemp->muki = angle + DX_PI / 2.0;
                pTemp->speed = 3.0 + pTemp->param_i[0] * 0.6; // 色でわずかに速度差
                pTemp->param_i[2] = 1; // 飛散中
                pTemp->margin = 20.0;
            }
            pTemp = pTemp->next;
        }

        // 串が折れる音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // ----- 粒弾が全て中心に戻ったら次のループへ -----
    if (phase == 1 &&
        pelletSpawned == SKEWERS * COLORS &&
        pelletArrived == pelletSpawned) {

        // 現在の全弾を削除（リストを空にする）
        //sEnemyShot* pDel = pEnemyShotSet->pEnemyShotHead->next;
        //while (pDel != pEnemyShotSet->pEnemyShotHead) {
        //    sEnemyShot* nxt = pDel->next;
        //    pDel->prev->next = pDel->next;
        //    pDel->next->prev = pDel->prev;
        //    delete pDel;
        //    pDel = nxt;
        //}
        //pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        //pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        ++loopCount;
        if (loopCount < 999) {   // 2～3回ループ（ここでは3回）
            // 次のループの準備
            phase = 0;
            pelletSpawned = 0;
            pelletArrived = 0;
            rotationStart = (double)pEnemyShotSet->count; // 新しい回転開始時刻

            double baseBaseAngle = GetRand(359) / 360.0 * DX_PI * 2;

            // 再び18個の団子弾を生成（初期化と同じ）
            for (int i = 0; i < SKEWERS; ++i) {
                double baseAngle = i * 2.0 * DX_PI / SKEWERS + baseBaseAngle;
                for (int j = 0; j < COLORS; ++j) {
                    sEnemyShot* p = new sEnemyShot;
                    p->x = centerX + RADIUS[j] * cos(baseAngle);
                    p->y = centerY + RADIUS[j] * sin(baseAngle);
                    p->muki = 0.0;
                    p->speed = 0.0;
                    p->kind = (j == 0) ? img_enemyShotMediumBall[5] :
                        (j == 1) ? img_enemyShotMediumBall[6] :
                        img_enemyShotMediumBall[2];
                    p->margin = 20.0;
                    p->param_i[0] = j;
                    p->param_i[1] = i;
                    p->param_i[2] = 0;
                    p->param_d[0] = baseAngle;
                    p->param_d[1] = SPEED_OFFSET[j];
                    p->param_d[2] = RADIUS[j];

                    p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    p->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = p;
                    pEnemyShotSet->pEnemyShotHead->prev = p;
                }
            }

            // 再生開始音
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        // loopCount >= 3 なら終了（何もしない）
    }
}

//---------------------------------------------------------
// 敵本体パターン
//---------------------------------------------------------
void EnemyPat_TricolorDango_DeepSeek()
{
    // count はグローバルフレームカウンタ（毎フレーム自動+1）
    if (count == 1) {
        // 敵を画面中央に固定
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;
    }
    // 敵は動かない

    // 弾幕セットを1回だけ生成（全フェーズを含む）
    if (count == 30) { // 少し間をおいてから開始
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotDangoSkewer;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // 空の弾リストヘッダを用意
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 全体の shot set リストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}