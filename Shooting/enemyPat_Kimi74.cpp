// enemyPat_TarakoPasta.cpp
// たらこスパゲッティ弾幕「たらこクリームの大盛りスパゲッティ」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 使用素材一覧
// ------------------------------------------------------------
// 【効果音】
//   sound_enemyCharge    … フェーズ開始の予告音
//   sound_enemyShot_light… たらこ飛散音
//   sound_enemyShot_medium… パスト（レーザー）展開音
//   sound_enemyShot_heavy… 吸引（大盛り）音
//   sound_enemyShot_extreme… 爆散（盛り付け）音
//
// 【弾グラフィック】
//   img_enemyShotSmallBall[1]  … 小玉・黄  → バター粒
//   img_enemyShotScale[0]      … 鱗弾・赤  → たらこ
//   img_enemyShotLaser[6]      … 短レーザー・白 → パスタ主軸
//   img_enemyShotSmallBall[6]  … 小玉・白  → 麺（スパゲッティの粒）
//
// 【弾幕構成】
//   フェーズ0 (count 0  )：バター粒 12発  ※画面中心へ低速収束
//   フェーズ1 (count 90 )：たらこ   20発  ※全方位ランダム飛散
//   フェーズ2 (count 210)：パスタ主軸4本 + 麺弾約88発 ※螺旋回転＆跳ね返り
//   フェーズ3 (count 390)：吸引             ※全弾を生成点へ引き寄せ
//   フェーズ4 (count 450)：爆散             ※全方位へ高速拡散
// ------------------------------------------------------------

static void ShotTarakoPasta(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    sEnemyShot* pShot;
    sEnemyShot* pNext;

    // ============================================================
    // フェーズ0：バター粒（画面中心へゆっくり集まる）
    // ============================================================
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = 0;          // 現在フェーズ
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x; // 生成時X（吸引中心）
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y; // 生成時Y（吸引中心）

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12 * 5; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = GetRand(360) / 180.0 * DX_PI;
            // 生成点周辺に少し散らして出現
            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * 20.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * 20.0;
            // 画面中心(240,240)へ向かう
            pEnemyShot->muki = atan2(240.0 - pEnemyShot->y, 240.0 - pEnemyShot->x);
            pEnemyShot->speed = (50 + GetRand(80)) / 100.0; // 0.5 ～ 1.3
            pEnemyShot->kind = img_enemyShotSmallBall[1];   // 小玉・黄
            pEnemyShot->param_i[0] = 0; // type: butter

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ============================================================
    // フェーズ1：たらこ（全方位へランダム飛散）
    // ============================================================
    else if (pEnemyShotSet->count == 90) {
        pEnemyShotSet->param_i[0] = 1;

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 20 * 5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
            pEnemyShot->speed = (120 + GetRand(180)) / 100.0; // 1.2 ～ 3.0
            pEnemyShot->kind = img_enemyShotScale[0]; // 鱗弾・赤
            pEnemyShot->param_i[0] = 1; // type: tarako

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ============================================================
    // フェーズ2：パスタ（4本の白レーザーが螺旋回転＋麺弾追加）
    // ============================================================
    else if (pEnemyShotSet->count >= 210 && pEnemyShotSet->count <= 250 && pEnemyShotSet->count % 5 == 0) {
        pEnemyShotSet->param_i[0] = 2;

        if (pEnemyShotSet->count == 210) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 4本の主レーザー（十字方向から開始、回転しながら伸びる）
        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            double baseAngle = i * DX_PI / 2.0;
            pEnemyShot->muki = baseAngle;
            pEnemyShot->speed = 5.2;
            pEnemyShot->kind = img_enemyShotLaser[6]; // 短レーザー・白
            pEnemyShot->param_i[0] = 2; // type: pasta main
            pEnemyShot->param_d[0] = baseAngle; // 現在角度（回転管理用）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ============================================================
    // フェーズ3：吸引（全弾を生成点へ引き寄せる）
    // ============================================================
    else if (pEnemyShotSet->count == 390) {
        pEnemyShotSet->param_i[0] = 3;

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // ============================================================
    // フェーズ4：爆散（全方位へ高速拡散）
    // ============================================================
    else if (pEnemyShotSet->count == 450) {
        pEnemyShotSet->param_i[0] = 4;

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 既存の全弾を「生成点から外へ向かう」方向に書き換え
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            double dx = pShot->x - pEnemyShotSet->param_d[0];
            double dy = pShot->y - pEnemyShotSet->param_d[1];
            pShot->muki = atan2(dy, dx);
            pShot->speed = 3.0 + GetRand(300) / 100.0; // 3.0 ～ 6.0
            pShot->param_i[0] = 4; // type: burst
            pShot = pShot->next;
        }
    }

    // ============================================================
    // フェーズ2中：麺弾（小玉・白）を螺旋状に追加生成
    // ============================================================
    if (pEnemyShotSet->param_i[0] == 2 &&
        pEnemyShotSet->count >= 210 && pEnemyShotSet->count < 390) {
        if ((pEnemyShotSet->count - 210) % 2 == 0) {
            for (int i = 0; i < 4; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                // 主レーザーと同じ回転則で角度を計算
                double baseAngle = i * DX_PI / 2.0 + (pEnemyShotSet->count - 210) * 0.025;
                pEnemyShot->muki = baseAngle;
                pEnemyShot->speed = 2.2;
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 小玉・白
                pEnemyShot->param_i[0] = 3; // type: pasta noodle

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ============================================================
    // 毎フレーム更新
    // ============================================================
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pNext = pShot->next;

        int type = pShot->param_i[0];
        double cx = pEnemyShotSet->param_d[0];
        double cy = pEnemyShotSet->param_d[1];

        // --- タイプ別の基本移動 ---
        if (type == 0) {
            // バター：中心へ向かう低速直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (type == 1) {
            // たらこ：ランダム方向へ直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (type == 2) {
            // パスタ主レーザー：回転しながら伸び、画面端で跳ね返る
            pShot->param_d[0] += 0.025;
            pShot->muki = pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            if (pShot->x < 10.0 || pShot->x > 470.0) {
                pShot->muki = DX_PI - pShot->muki;
                pShot->param_d[0] = pShot->muki;
            }
            if (pShot->y < 10.0 || pShot->y > 470.0) {
                pShot->muki = -pShot->muki;
                pShot->param_d[0] = pShot->muki;
            }
        }
        else if (type == 3) {
            // 麺弾：生成時の角度のまま直進（螺旋の軌跡を描く）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (type == 4) {
            // 爆散：全方位高速直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        // --- フェーズ3（吸引）：全タイプの弾を生成点へ引き寄せ ---
        if (pEnemyShotSet->param_i[0] == 3) {
            double dx = cx - pShot->x;
            double dy = cy - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > 5.0) {
                pShot->x += (dx / dist) * 4.0;
                pShot->y += (dy / dist) * 4.0;
            }
        }

        pShot = pNext;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_TarakoSpaghetti_Kimi()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 1.2 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 約10秒（600フレーム）ごとに弾幕セットを生成
    if (count % 600 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTarakoPasta;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
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