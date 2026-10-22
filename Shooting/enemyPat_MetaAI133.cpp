#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 青天の霹靂 - 晴天神鳴
// フェーズ 0: 青空漂い(0-150F)
// フェーズ 1: 停止と予兆フラッシュ(150-168F)
// フェーズ 2: 落雷本番(168F-)
// ============================================================
static void ShotSeitenHekireki(sEnemyShotSet* pSet)
{
    // 初期化
    if (pSet->count == 0) {
        pSet->param_i[0] = 0; // 0:青空 1:予兆 2:落雷後 3:終了
        pSet->param_d[0] = pSet->x; // 雷の起点X (敵位置を記憶)
        pSet->param_d[1] = pSet->y;
    }

    // 既存弾の移動と寿命処理
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[1] == 0) {
            // 通常漂い弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->muki += pShot->param_d[0]; // 微小な揺らぎ
        }
        else if (pShot->param_i[1] == 1) {
            // レーザー(稲妻)本体、フラッシュは停止
        }
        else if (pShot->param_i[1] == 2) {
            // 衝撃波
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->speed *= 0.985;
        }

        // 寿命が設定されている弾は時間で消去 (画面外消去はメイン側だが、演出用に自前で消す)
        if (pShot->param_i[0] > 0 && pShot->count > pShot->param_i[0]) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        pShot = pNext;
    }

    // フェーズ管理
    if (pSet->param_i[0] == 0) {
        // ---- フェーズ0: 青空 ----
        if (pSet->count % 3 == 0) {
            for (int k = 0; k < 2; k++) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = (double)GetRand(480);
                pNew->y = (double)GetRand(120) - 20.0;
                // ほぼ真下にゆっくり
                pNew->muki = DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
                pNew->speed = 0.3 + GetRand(50) / 100.0;
                pNew->param_d[0] = (GetRand(20) - 10) / 500.0; // 揺らぎ
                pNew->param_i[0] = 0; // 無限
                pNew->param_i[1] = 0; // 漂いタイプ
                // GetRand注意: 0-8の9色から青系のみ抜粋
                int col = (GetRand(1) == 0) ? 4 : 3; // 4:青 3:シアン
                pNew->kind = img_enemyShotSmallBall[col];

                pNew->prev = pSet->pEnemyShotHead->prev;
                pNew->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pNew;
                pSet->pEnemyShotHead->prev = pNew;
            }
        }
        // 稀にきらめき
        if (GetRand(35) == 0) {
            sEnemyShot* pNew = new sEnemyShot;
            pNew->x = (double)GetRand(480);
            pNew->y = (double)GetRand(300);
            pNew->muki = (GetRand(1) == 0) ? 0 : DX_PI;
            pNew->speed = 4.0 + GetRand(100) / 100.0;
            pNew->param_i[0] = 12;
            pNew->param_i[1] = 2;
            pNew->kind = img_enemyShotSmallBall[6]; // 白
            pNew->prev = pSet->pEnemyShotHead->prev;
            pNew->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pNew;
            pSet->pEnemyShotHead->prev = pNew;
        }

        if (pSet->count >= 150) {
            // 全弾停止でタメを作る
            sEnemyShot* pS = pSet->pEnemyShotHead->next;
            while (pS != pSet->pEnemyShotHead) {
                if (pS->param_i[1] == 0) pS->speed = 0;
                pS = pS->next;
            }
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            // 予兆フラッシュ (大玉白)
            sEnemyShot* pFlash = new sEnemyShot;
            pFlash->x = pSet->param_d[0];
            pFlash->y = pSet->param_d[1] + 20.0;
            pFlash->muki = 0;
            pFlash->speed = 0;
            pFlash->param_i[0] = 18; // 0.3秒だけ
            pFlash->param_i[1] = 1;
            pFlash->kind = img_enemyShotLargeBall[6]; // 白大玉 20x20
            pFlash->prev = pSet->pEnemyShotHead->prev;
            pFlash->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pFlash;
            pSet->pEnemyShotHead->prev = pFlash;

            pSet->param_i[0] = 1;
        }
    }
    else if (pSet->param_i[0] == 1) {
        // ---- フェーズ1: 予兆中 ----
        if (pSet->count >= 168) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            // ---- 稲妻生成 ----
            double curX = pSet->param_d[0];
            double curY = pSet->param_d[1] + 20.0;

            for (int seg = 0; seg < 7; seg++) {
                double nextY = curY + 65.0 + (double)GetRand(25);
                double nextX = curX + (double)GetRand(80) - 40.0;
                if (nextX < 30.0) nextX = 30.0;
                if (nextX > 450.0) nextX = 450.0;
                if (seg == 6) nextY = 520.0; // 画面外まで

                double dx = nextX - curX;
                double dy = nextY - curY;
                double ang = atan2(dy, dx);
                double dist = sqrt(dx * dx + dy * dy);
                int div = (int)(dist / 44.0) + 1;

                // 幹: 黄レーザー + 白レーザーを重ねて発光表現
                for (int j = 0; j < div; j++) {
                    double rate = (j + 0.5) / (double)div;
                    // 黄芯
                    {
                        sEnemyShot* pL = new sEnemyShot;
                        pL->x = curX + dx * rate;
                        pL->y = curY + dy * rate;
                        pL->muki = ang;
                        pL->speed = 0;
                        pL->param_i[0] = 38; // 0.6秒残留
                        pL->param_i[1] = 1;
                        pL->kind = img_enemyShotLaser[1]; // 黄 短レーザー 64x4
                        pL->prev = pSet->pEnemyShotHead->prev;
                        pL->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = pL;
                        pSet->pEnemyShotHead->prev = pL;
                    }
                    // 白芯を重ねる (細く光る)
                    {
                        sEnemyShot* pL = new sEnemyShot;
                        pL->x = curX + dx * rate;
                        pL->y = curY + dy * rate;
                        pL->muki = ang;
                        pL->speed = 0;
                        pL->param_i[0] = 32;
                        pL->param_i[1] = 1;
                        pL->kind = img_enemyShotLaser[6]; // 白
                        pL->prev = pSet->pEnemyShotHead->prev;
                        pL->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = pL;
                        pSet->pEnemyShotHead->prev = pL;
                    }
                }

                // 枝分かれ (2箇所)
                if (seg == 2 || seg == 4) {
                    double branchAng = ang + ((GetRand(1) == 0) ? DX_PI / 3.5 : -DX_PI / 3.5);
                    for (int b = 0; b < 2; b++) {
                        sEnemyShot* pB = new sEnemyShot;
                        pB->x = curX;
                        pB->y = curY;
                        pB->muki = branchAng;
                        pB->speed = 0;
                        pB->param_i[0] = 28;
                        pB->param_i[1] = 1;
                        pB->kind = img_enemyShotLaser[1];
                        // 長さは短いので位置はずらさず角度だけで表現、2本目で少し伸ばす
                        if (b == 1) {
                            pB->x += 30 * cos(branchAng);
                            pB->y += 30 * sin(branchAng);
                        }
                        pB->prev = pSet->pEnemyShotHead->prev;
                        pB->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = pB;
                        pSet->pEnemyShotHead->prev = pB;
                    }
                }

                // 折れ曲がり点で衝撃波 (青小玉を円形に)
                for (int s = 0; s < 10; s++) {
                    sEnemyShot* pW = new sEnemyShot;
                    pW->x = curX;
                    pW->y = curY;
                    pW->muki = (DX_PI * 2.0 / 10.0) * s + GetRand(20) / 180.0 * DX_PI;
                    pW->speed = 2.5 + GetRand(150) / 100.0;
                    pW->param_i[0] = 120;
                    pW->param_i[1] = 2;
                    pW->kind = img_enemyShotSmallBall[4]; // 青
                    pW->prev = pSet->pEnemyShotHead->prev;
                    pW->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pW;
                    pSet->pEnemyShotHead->prev = pW;
                }

                curX = nextX;
                curY = nextY;
                if (curY > 500.0) break;
            }
            pSet->param_i[0] = 2;
        }
    }
    else if (pSet->param_i[0] == 2) {
        // 終息、少し待って自然消滅へ
        if (pSet->count > 230) {
            pSet->param_i[0] = 3;
        }
    }
}

// 敵本体
void EnemyPat_SeitenNoHekireki_MetaAI()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆっくり左右に揺れるだけ (青空の余裕を演出)
        enemy.x += 0.45 * (double)muki;
        if (enemy.x > 320) muki = -1;
        if (enemy.x < 160) muki = 1;
    }

    // 240F周期で青天の霹靂を打ち出す
    if (count % 240 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSeitenHekireki;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = DX_PI / 2.0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}