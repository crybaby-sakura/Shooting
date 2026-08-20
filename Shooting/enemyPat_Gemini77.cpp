// enemyPat_Tmp.cpp
// ヨーヨー演舞『アラウンド・スリーパー』実装ソースコード

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// -----------------------------------------------------------------------------
// 弾幕パターン：ヨーヨー演舞『アラウンド・スリーパー』
// -----------------------------------------------------------------------------
// 【使用素材】
//  - ヨーヨー本体: img_enemyShotLargeBall[0] (赤大玉 / 20.0x20.0)
//  - 糸（ストリング）: img_enemyShotSmallBall[3] (シアン小玉 / 2.5x2.5)
//  - 空転（スリーパー）放電: img_enemyShotDiamond[1] (黄菱形弾 / 4.5x2.5)
//  - 弾け散る糸（リターン）: img_enemyShotBullet[0] (赤銃弾 / 5.0x2.0)
//  - SE: sound_enemyShot_heavy (キャスト/手応え), sound_enemyShot_light (空転放電), sound_enemyShot_medium (引き戻し/炸裂)
// -----------------------------------------------------------------------------
static void ShotYoYo(sEnemyShotSet* pEnemyShotSet)
{
    // 各フェーズの切り替えフレーム定義
    const int CAST_END = 25;   // 1. キャスト（糸を伸ばしきる）
    const int SLEEP_END = 95;   // 2. スリーパー（停滞＆周囲へ放電射出）
    const int AROUND_END = 195;  // 3. アラウンド・ザ・ワールド（糸ごと扇状旋回）
    const int RETURN_END = 230;  // 4. リターン（一気に巻き戻り＆糸が弾けて拡散）

    int c = pEnemyShotSet->count;

    // -------------------------------------------------------------------------
    // Phase 0: 初期化 (c == 0) -> ヨーヨー本体の生成と射出方向・距離の計算
    // -------------------------------------------------------------------------
    if (c == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // ボス位置を中心軸として保存
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x; // 旋回中心 X
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y; // 旋回中心 Y

        // 自機方向に向かって飛ばすベクトルと目標距離を設定
        double dx = player.x - pEnemyShotSet->x;
        double dy = player.y - pEnemyShotSet->y;
        double targetDist = sqrt(dx * dx + dy * dy);

        // 距離の範囲制限（近すぎず遠すぎず）
        if (targetDist < 140.0) targetDist = 140.0;
        if (targetDist > 300.0) targetDist = 300.0;

        double angle = atan2(dy, dx);
        pEnemyShotSet->param_d[2] = targetDist; // 糸の長さ R
        pEnemyShotSet->param_d[3] = angle;      // 初期射出角度 theta

        // ヨーヨー本体（赤大玉）を作成
        sEnemyShot* pBody = new sEnemyShot;
        pBody->x = pEnemyShotSet->x;
        pBody->y = pEnemyShotSet->y;
        pBody->muki = angle;
        pBody->speed = targetDist / CAST_END;   // CAST_END フレームで目標到達
        pBody->kind = img_enemyShotLargeBall[0]; // 赤の大玉
        pBody->param_i[0] = 1;                  // 種別 1: ヨーヨー本体
        pBody->param_d[0] = 0.0;                // 現在の半径 r
        pBody->param_d[1] = angle;              // 現在の角度 theta
        pBody->margin = 480;

        // リストに追加
        pBody->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pBody->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pBody;
        pEnemyShotSet->pEnemyShotHead->prev = pBody;
    }

    // -------------------------------------------------------------------------
    // Phase 1: キャスト中 (0 < c <= CAST_END) -> 軌跡に「糸（小弾）」を等間隔設置
    // -------------------------------------------------------------------------
    if (c > 0 && c <= CAST_END) {
        // 定期的（3フレーム毎）に糸を構成する小弾を追加
        if (c % 3 == 0 && c < CAST_END) {
            double angle = pEnemyShotSet->param_d[3];
            double maxR = pEnemyShotSet->param_d[2];
            double currentR = maxR * ((double)c / CAST_END);

            sEnemyShot* pString = new sEnemyShot;
            pString->x = pEnemyShotSet->param_d[0] + currentR * cos(angle);
            pString->y = pEnemyShotSet->param_d[1] + currentR * sin(angle);
            pString->muki = angle;
            pString->speed = 0.0;                     // キャスト時は静止
            pString->kind = img_enemyShotSmallBall[3]; // シアンの小玉（糸）
            pString->param_i[0] = 2;                  // 種別 2: 糸の小弾
            pString->param_d[0] = currentR;           // 中心からの距離 r
            pString->param_d[1] = angle;              // 中心からの角度 theta
            pString->margin = 480;

            pString->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pString->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pString;
            pEnemyShotSet->pEnemyShotHead->prev = pString;
        }
    }

    // -------------------------------------------------------------------------
    // 各弾の挙動・更新処理
    // -------------------------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 削除・移動時に備えて保持
        int type = pShot->param_i[0];

        // --- Phase 1: キャスト期 ---
        if (c <= CAST_END) {
            if (type == 1) { // ヨーヨー本体の直進
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                pShot->param_d[0] += pShot->speed; // 距離 r を増加
            }
        }
        // --- Phase 2: スリーパー期（空転＆放電） ---
        else if (c <= SLEEP_END) {
            if (type == 1) { // ヨーヨー本体：先端で停止し、周囲へ放電弾を発射
                pShot->speed = 0.0;

                // 8フレームごとに全方位リング（8way）の黄色菱形弾を放電
                if ((c - CAST_END) % 8 == 0) {
                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                    int way = 8;
                    double baseAngle = (c * 10) * DX_PI / 180.0; // 交互に角度を少しずらす
                    for (int k = 0; k < way; k++) {
                        sEnemyShot* pSpark = new sEnemyShot;
                        pSpark->x = pShot->x;
                        pSpark->y = pShot->y;
                        pSpark->muki = baseAngle + k * (2.0 * DX_PI / way);
                        pSpark->speed = 2.2;
                        pSpark->kind = img_enemyShotDiamond[1]; // 黄色の菱形弾（火花）
                        pSpark->param_i[0] = 3;                  // 種別 3: 独立直線移動弾
                        
                        pSpark->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pSpark->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pSpark;
                        pEnemyShotSet->pEnemyShotHead->prev = pSpark;
                    }
                }
            }
        }
        // --- Phase 3: アラウンド・ザ・ワールド期（旋回） ---
        else if (c <= AROUND_END) {
            if (type == 1 || type == 2) { // ヨーヨー本体および糸の弾を一括旋回
                double rotSpeed = 0.016;  // 旋回速度（ラジアン/フレーム）
                pShot->param_d[1] += rotSpeed; // 角度 theta を更新

                double centerX = pEnemyShotSet->param_d[0];
                double centerY = pEnemyShotSet->param_d[1];
                double r = pShot->param_d[0];
                double th = pShot->param_d[1];

                // 回転座標の反映
                pShot->x = centerX + r * cos(th);
                pShot->y = centerY + r * sin(th);
                pShot->muki = th + DX_PI / 2.0;
            }
        }
        // --- Phase 4: リターン期（引き戻し＆糸の弾け散り） ---
        else if (c <= RETURN_END) {
            if (type == 1) { // ヨーヨー本体：ボスへ急速引き戻し
                double centerX = pEnemyShotSet->param_d[0];
                double centerY = pEnemyShotSet->param_d[1];
                double dx = centerX - pShot->x;
                double dy = centerY - pShot->y;
                double dist = sqrt(dx * dx + dy * dy);

                pShot->muki = atan2(dy, dx);
                pShot->speed = dist / (RETURN_END - c + 1); // 残りフレームで丁度到達するように加速
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else if (type == 2) { // 糸の小弾：引き戻し開始時に一斉に弾け散る
                if (c == AROUND_END + 1) {
                    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                    // 0〜360度のランダム方向へ低速拡散
                    // GetRand(x) は 0〜x までの x+1 種類の整数を返す点に注意
                    pShot->muki = (GetRand(360) / 360.0) * 2.0 * DX_PI;
                    pShot->speed = 0.8 + (GetRand(100) / 100.0); // 0.8 〜 1.8
                    pShot->kind = img_enemyShotBullet[0];       // 赤色の銃弾へ変化
                    pShot->param_i[0] = 3;                       // 独立直線移動弾へ移行
                }
            }
        }
        // --- Phase 5: リターン完了後 ---
        else {
            if (type == 1) { // 戻りきったヨーヨー本体の削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                pShot = pNext;
                continue;
            }
        }

        // --- 種別 3（独立直線移動弾：スリーパー放電弾・分散後の糸弾）の共通移動処理 ---
        if (type == 3) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// -----------------------------------------------------------------------------
// 敵本体のパターン関数（メインルーチンから呼ばれる）
// -----------------------------------------------------------------------------
void EnemyPat_Yoyo_Gemini()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面: 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // 敵本体のゆっくりとした左右移動
        enemy.x += 0.6 * (double)muki;
        if (count % 160 == 80) {
            muki *= -1;
        }
    }

    // 270フレーム（約4.5秒）おきに『アラウンド・スリーパー』を発射
    if (count % 80 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotYoYo;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // ダミーヘッドノードの作成（循環ダブルリンクの初期化）
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルリスト `enemyShotSetHead` へ繋ぐ
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}