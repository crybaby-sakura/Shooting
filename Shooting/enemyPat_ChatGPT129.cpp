#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {
    // 1周期:
    // 予告 1秒 → 時間停止 3秒 → 通常 3秒
    constexpr int TIME_STOP_WARNING = 60;
    constexpr int TIME_STOP_ACTIVE = 180;
    constexpr int TIME_STOP_NORMAL = 180;
    constexpr int TIME_STOP_CYCLE = TIME_STOP_WARNING + TIME_STOP_ACTIVE + TIME_STOP_NORMAL;

    // 現在の時間停止状態
    static bool g_timeStopActive = false;
    static double g_lockedPlayerX = 240.0;
    static double g_lockedPlayerY = 420.0;

    // 弾生成用の通し番号
    static int g_shotCount = 0;

    // 時間停止中でも弾そのものの状態は維持し、
    // 移動だけを止める。
    static void ShotTimeStop(sEnemyShotSet* pEnemyShotSet)
    {
        sEnemyShot* pEnemyShot;

        if (pEnemyShotSet->count == 0) {
            // 1回の射撃で放射状に大量の弾を生成する。
            // 停止中に生成された弾も、その場に留まって解除後に動き出す。
            const int burstType = pEnemyShotSet->kind % 3;

            int shotNum = 0;
            if (burstType == 0) {
                shotNum = 18;
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            }
            else if (burstType == 1) {
                shotNum = 12;
            }
            else {
                shotNum = 20;
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }

            const double baseMuki =
                atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

            for (int i = 0; i < shotNum; ++i) {
                pEnemyShot = new sEnemyShot;

                double muki = 0.0;
                double speed = 0.0;

                if (burstType == 0) {
                    // シアンの小玉を均等放射。
                    muki = 2.0 * DX_PI * i / shotNum;
                    speed = 2.8;
                    pEnemyShot->kind = img_enemyShotSmallBall[3];
                }
                else if (burstType == 1) {
                    // 白い菱形弾を少し狭い扇状にして自機方向へ。
                    const double spread = 0.75 * DX_PI;
                    muki = baseMuki - spread * 0.5 + spread * i / (shotNum - 1.0);
                    speed = 3.4;
                    pEnemyShot->kind = img_enemyShotDiamond[6];
                }
                else {
                    // 青い中楕円弾を回転しながら放射。
                    muki = 2.0 * DX_PI * i / shotNum + pEnemyShotSet->param_d[0];
                    speed = 2.2;
                    pEnemyShot->kind = img_enemyShotMediumOval[4];
                }

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = muki;
                pEnemyShot->speed = speed;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 時間停止中は敵弾を一切移動させない。
        // 通常状態へ戻ると、保持していた速度で再び移動する。
        if (!g_timeStopActive) {
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                pShot = pShot->next;
            }
        }
    }

    // ボス移動。
    // 時間停止中もボスだけは動き続ける。
    static void UpdateBossPosition()
    {
        // 画面内を滑らかに大きく動き回る。
        const double t = (double)count * 0.020;
        enemy.x = 240.0 + 185.0 * sin(t * 0.73);
        enemy.y = 125.0 + 90.0 * sin(t * 1.11 + 0.8);
    }

    static void FireShotSet(int patternType, double phaseOffset)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTimeStop;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = patternType + g_shotCount++;
        pEnemyShotSet->param_d[0] = phaseOffset;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}

// 敵本体のパターン
void EnemyPat_TimeStop_ChatGPT()
{
    static int phase = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 125.0;
        enemy.maxHp = enemy.hp = 200;

        phase = 0;
        g_timeStopActive = false;
        g_lockedPlayerX = player.x;
        g_lockedPlayerY = player.y;
        g_shotCount = 0;
    }
    else {
        phase = (count - 1) % TIME_STOP_CYCLE;
    }

    // ボスは常に動き回る。
    UpdateBossPosition();

    // 周期開始時に時間停止の予告音。
    if (phase == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 予告から1秒後に時間停止へ移行。
    if (phase == TIME_STOP_WARNING) {
        g_timeStopActive = true;
        g_lockedPlayerX = player.x;
        g_lockedPlayerY = player.y;

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 時間停止中:
    // ・自機を停止位置へ戻す
    // ・自機ショットのy移動(-15.0)を+15.0で相殺
    // ・ボスは移動を継続
    // ・敵弾はShotTimeStop側で移動しない
    if (g_timeStopActive) {
        player.x = g_lockedPlayerX;
        player.y = g_lockedPlayerY;

        sPlayerShot* pPlayerShot = playerShotHead.next;
        while (pPlayerShot != &playerShotHead) {
            pPlayerShot->y += 15.0;
            pPlayerShot = pPlayerShot->next;
        }
    }

    // 予告中は時間停止後の本格攻撃に備えた少数射撃。
    if (phase < TIME_STOP_WARNING) {
        if (count % 20 == 1) {
            FireShotSet(2, 0.0);
        }
    }
    // 時間停止中は弾を大量に生成する。
    // 弾はその場に凍り付き、解除時に一斉に動き始める。
    else if (phase < TIME_STOP_WARNING + TIME_STOP_ACTIVE) {
        if (count % 8 == 1) {
            const int burstType = (count / 8) % 3;
            const double phaseOffset = ((count / 8) % 2) ? DX_PI / 18.0 : 0.0;
            FireShotSet(burstType, phaseOffset);
        }
    }
    // 通常状態でも攻撃を続け、次の停止へつなげる。
    else {
        if (count % 16 == 1) {
            const int burstType = (count / 16) % 3;
            const double phaseOffset = ((count / 16) % 2) ? DX_PI / 18.0 : 0.0;
            FireShotSet(burstType, phaseOffset);
        }
    }

    // 時間停止終了。
    if (phase == TIME_STOP_WARNING + TIME_STOP_ACTIVE) {
        g_timeStopActive = false;
    }
}