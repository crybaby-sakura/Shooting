// enemyPat_Antlion.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// アリジゴク弾幕：巣の壁（左右から中央下に迫る）
// ============================================================
static void ShotAntlionWall(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 左壁と右壁を生成
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = (i == 0) ? 0.0 : 480.0;
            pShot->y = 0.0;
            pShot->kind = img_enemyShotLargeBall[8]; // 橙の大玉

            // 目標は画面中央下 (漏斗の底)
            pShot->param_d[0] = 240.0; // target X
            pShot->param_d[1] = 480.0; // target Y

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 目標に向かって減速移動 (線形補間) することで、漏斗に吸い込まれるような動きを表現
        pShot->x += (pShot->param_d[0] - pShot->x) * 0.015;
        pShot->y += (pShot->param_d[1] - pShot->y) * 0.015;
        pShot = pShot->next;
    }
}

// ============================================================
// アリジゴク弾幕：砂吹き（放物線を描いてプレイヤーに降りかかる）
// ============================================================
static void ShotAntlionSand(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 複数発の砂をばら撒く
        for (int i = 0; i < 9; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x + GetRand(60) - 30;
            pShot->y = pEnemyShotSet->y;
            pShot->kind = img_enemyShotScale[1]; // 黄の鱗弾

            // プレイヤー方向への初期速度成分
            double dx = player.x - pShot->x;
            double dy = player.y - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist == 0) dist = 1.0;

            // 一度上に打ち上げる成分を加える
            pShot->param_d[0] = (dx / dist) * 2.5 + (GetRand(40) - 20) / 10.0; // X方向速度 (多少のブレ)
            pShot->param_d[1] = -3.5 - GetRand(15) / 10.0;                     // 初期Y方向速度 (上向き)

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double px = pShot->x;
        double py = pShot->y;

        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];
        pShot->param_d[1] += 0.03; // 疑似重力を加えて放物線運動を表現

        pShot->muki = atan2(pShot->y - py, pShot->x - px);

        pShot = pShot->next;
    }
}

// ============================================================
// アリジゴク弾幕：砂崩れ（壁から斜め下に高速滑落）
// ============================================================
static void ShotAntlionAvalanche(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        double leftWallX = pEnemyShotSet->param_d[0];
        double rightWallX = pEnemyShotSet->param_d[1];
        double spawnY = pEnemyShotSet->y;

        // 左壁から滑落
        sEnemyShot* pShotL = new sEnemyShot;
        pShotL->x = leftWallX;
        pShotL->y = spawnY;
        pShotL->kind = img_enemyShotDiamond[8]; // 橙の菱形弾
        pShotL->param_d[0] = 1.8; // X方向速度 (右へ)
        pShotL->param_d[1] = 4.5; // Y方向速度 (下へ)
        pShotL->muki = atan2(pShotL->param_d[1], pShotL->param_d[0]);

        pShotL->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShotL->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShotL;
        pEnemyShotSet->pEnemyShotHead->prev = pShotL;

        // 右壁から滑落
        sEnemyShot* pShotR = new sEnemyShot;
        pShotR->x = rightWallX;
        pShotR->y = spawnY;
        pShotR->kind = img_enemyShotDiamond[8];
        pShotR->param_d[0] = -1.8; // X方向速度 (左へ)
        pShotR->param_d[1] = 4.5;  // Y方向速度 (下へ)
        pShotR->muki = atan2(pShotR->param_d[1], pShotR->param_d[0]);

        pShotR->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShotR->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShotR;
        pEnemyShotSet->pEnemyShotHead->prev = pShotR;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];
        pShot = pShot->next;
    }
}

// ============================================================
// アリジゴク弾幕：顎の捕食（左右から交差するレーザー）
// ============================================================
static void ShotAntlionJaw(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 左顎と右顎
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = (i == 0) ? 0.0 : 480.0;
            pShot->y = 480.0;
            pShot->kind = img_enemyShotLaser[0]; // 赤の短レーザー

            // 画面中央上 (240, 0) に向かう基本角度
            double targetX = 240.0;
            double targetY = 0.0;
            pShot->muki = atan2(targetY - pShot->y, targetX - pShot->x);

            // 交差させるため、左は右寄り、右は左寄りに角度をずらす
            // GetRand(30) は 0〜30 を返すので、/10.0 で 0.0〜3.0 ラジアンのズレ幅を持たせる
            double offset = (i == 0) ? (GetRand(30) / 30.0) : -(GetRand(30) / 30.0);
            pShot->muki += offset;

            pShot->speed = 5.0; // レーザーの移動速度

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
// 敵本体のパターン：アリジゴク
// ============================================================
void EnemyPat_Antlion_Qwen()
{
    static double wallLeftX = 0.0;
    static double wallRightX = 480.0;
    static double wallY = 0.0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 420.0; // 画面下部中央に潜伏
        enemy.maxHp = enemy.hp = 60 * 30;

        wallLeftX = 0.0;
        wallRightX = 480.0;
        wallY = 0.0;

        player.y = 240;
    }

    enemy.hp--;

    const int T = 510;

    // --------------------------------------------------------
    // フェーズ1: 巣の形成 (count 1 ~ 120)
    // --------------------------------------------------------
    if (count % T < 120) {
        if (count % T % 60 == 1) {
            sEnemyShotSet* pWallSet = new sEnemyShotSet;
            pWallSet->count = 0;
            pWallSet->patternFunc = ShotAntlionWall;

            pWallSet->pEnemyShotHead = new sEnemyShot;
            pWallSet->pEnemyShotHead->prev = pWallSet->pEnemyShotHead;
            pWallSet->pEnemyShotHead->next = pWallSet->pEnemyShotHead;

            pWallSet->prev = enemyShotSetHead.prev;
            pWallSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pWallSet;
            enemyShotSetHead.prev = pWallSet;
        }

        if (count % T % 15 == 1) {
            sEnemyShotSet* pSandSet = new sEnemyShotSet;
            pSandSet->count = 0;
            pSandSet->patternFunc = ShotAntlionSand;
            pSandSet->x = enemy.x;
            pSandSet->y = enemy.y;

            pSandSet->pEnemyShotHead = new sEnemyShot;
            pSandSet->pEnemyShotHead->prev = pSandSet->pEnemyShotHead;
            pSandSet->pEnemyShotHead->next = pSandSet->pEnemyShotHead;

            pSandSet->prev = enemyShotSetHead.prev;
            pSandSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSandSet;
            enemyShotSetHead.prev = pSandSet;
        }
    }
    // --------------------------------------------------------
    // フェーズ2: 砂崩れと圧縮 (count 121 ~ 300)
    // --------------------------------------------------------
    else if (count % T < 300) {
        // 壁を徐々に狭めていく (V字型に圧縮)
        if (count % T % 2 == 0) {
            if (wallLeftX < 160.0) wallLeftX += 0.4;
            if (wallRightX > 320.0) wallRightX -= 0.4;
            //if (wallY < 240.0) wallY += 0.4;
        }

        if (count % T % 60 == 1) {
            sEnemyShotSet* pWallSet = new sEnemyShotSet;
            pWallSet->count = 0;
            pWallSet->patternFunc = ShotAntlionWall;

            pWallSet->pEnemyShotHead = new sEnemyShot;
            pWallSet->pEnemyShotHead->prev = pWallSet->pEnemyShotHead;
            pWallSet->pEnemyShotHead->next = pWallSet->pEnemyShotHead;

            pWallSet->prev = enemyShotSetHead.prev;
            pWallSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pWallSet;
            enemyShotSetHead.prev = pWallSet;
        }

        if (count % T % 4 == 1) {
            sEnemyShotSet* pAvalancheSet = new sEnemyShotSet;
            pAvalancheSet->count = 0;
            pAvalancheSet->patternFunc = ShotAntlionAvalanche;
            pAvalancheSet->param_d[0] = wallLeftX;  // 左壁の現在位置を渡す
            pAvalancheSet->param_d[1] = wallRightX; // 右壁の現在位置を渡す
            pAvalancheSet->y = wallY;

            pAvalancheSet->pEnemyShotHead = new sEnemyShot;
            pAvalancheSet->pEnemyShotHead->prev = pAvalancheSet->pEnemyShotHead;
            pAvalancheSet->pEnemyShotHead->next = pAvalancheSet->pEnemyShotHead;

            pAvalancheSet->prev = enemyShotSetHead.prev;
            pAvalancheSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pAvalancheSet;
            enemyShotSetHead.prev = pAvalancheSet;
        }

        if (count % T % 20 == 1) {
            sEnemyShotSet* pSandSet = new sEnemyShotSet;
            pSandSet->count = 0;
            pSandSet->patternFunc = ShotAntlionSand;
            pSandSet->x = enemy.x;
            pSandSet->y = enemy.y;

            pSandSet->pEnemyShotHead = new sEnemyShot;
            pSandSet->pEnemyShotHead->prev = pSandSet->pEnemyShotHead;
            pSandSet->pEnemyShotHead->next = pSandSet->pEnemyShotHead;

            pSandSet->prev = enemyShotSetHead.prev;
            pSandSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSandSet;
            enemyShotSetHead.prev = pSandSet;
        }
    }
    // --------------------------------------------------------
    // フェーズ3: 顎の捕食 (count 301 ~ 450)
    // --------------------------------------------------------
    else if (count % T < 450) {
        // 予告音
        if (count % T == 301) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        if (count % T % 60 == 1) {
            sEnemyShotSet* pWallSet = new sEnemyShotSet;
            pWallSet->count = 0;
            pWallSet->patternFunc = ShotAntlionWall;

            pWallSet->pEnemyShotHead = new sEnemyShot;
            pWallSet->pEnemyShotHead->prev = pWallSet->pEnemyShotHead;
            pWallSet->pEnemyShotHead->next = pWallSet->pEnemyShotHead;

            pWallSet->prev = enemyShotSetHead.prev;
            pWallSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pWallSet;
            enemyShotSetHead.prev = pWallSet;
        }

        if (count % T % 12 == 1) {
            sEnemyShotSet* pJawSet = new sEnemyShotSet;
            pJawSet->count = 0;
            pJawSet->patternFunc = ShotAntlionJaw;

            pJawSet->pEnemyShotHead = new sEnemyShot;
            pJawSet->pEnemyShotHead->prev = pJawSet->pEnemyShotHead;
            pJawSet->pEnemyShotHead->next = pJawSet->pEnemyShotHead;

            pJawSet->prev = enemyShotSetHead.prev;
            pJawSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pJawSet;
            enemyShotSetHead.prev = pJawSet;
        }

        if (count % T % 10 == 1) {
            sEnemyShotSet* pSandSet = new sEnemyShotSet;
            pSandSet->count = 0;
            pSandSet->patternFunc = ShotAntlionSand;
            pSandSet->x = enemy.x;
            pSandSet->y = enemy.y;

            pSandSet->pEnemyShotHead = new sEnemyShot;
            pSandSet->pEnemyShotHead->prev = pSandSet->pEnemyShotHead;
            pSandSet->pEnemyShotHead->next = pSandSet->pEnemyShotHead;

            pSandSet->prev = enemyShotSetHead.prev;
            pSandSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSandSet;
            enemyShotSetHead.prev = pSandSet;
        }
    }
    // --------------------------------------------------------
    // パターン終了後の待機 (count >= 450)
    // --------------------------------------------------------
    else {
        // パターン完了。必要に応じて敵を消去するか、HPを0にしてメイン側で処理させる。
        // ここでは例として、敵を画面外へ退避させて事実上の終了状態にする。
        //enemy.y = 600.0;
    }
}