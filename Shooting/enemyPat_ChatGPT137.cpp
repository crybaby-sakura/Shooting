#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：「十年越しの解答」
//
// ・直近のプレイヤー軌跡をショットセット内に記録
// ・少し遅れた軌跡を安全地帯の中心として再生
// ・水平／垂直のレーザー回廊を交互に生成
// ・回廊の外側には追尾気味の小玉・軌跡再生弾を追加
// ・終盤ほど回廊が狭くなり、切り替えも速くなる
//
// count / pEnemyShotSet->count / pEnemyShot->count の加算、
// 画面外の弾の削除はメインルーチン側で行う仕様。
// ============================================================

static void AddShot(sEnemyShotSet* pEnemyShotSet,
    double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static double Clamp(double value, double minValue, double maxValue)
{
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

// 8点の履歴から、少し過去のプレイヤー位置をなめらかに補間する。
static void GetHistoryPoint(sEnemyShotSet* pEnemyShotSet, int historyIndex, double* x, double* y)
{
    historyIndex &= 7;
    *x = pEnemyShotSet->param_d[historyIndex];
    *y = pEnemyShotSet->param_d[8 + historyIndex];
}

static void ShotTenYear(sEnemyShotSet* pEnemyShotSet)
{
    // param_i[0] : 現在フェーズ
    // param_i[1] : 回廊の向き 0=水平 1=垂直
    // param_i[2] : 回廊切り替え時点
    // param_i[3] : 記録カーソル
    // param_i[4] : 最後に再生した履歴番号
    // param_i[5] : 回廊の通過回数
    // param_i[6] : フリーズ状態
    // param_i[7] : フリーズ開始時刻
    //
    // param_d[0..7]  : 8個の履歴X
    // param_d[8..15] : 8個の履歴Y

    if (pEnemyShotSet->count == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0;
        pEnemyShotSet->param_i[2] = 480;
        pEnemyShotSet->param_i[3] = 0;
        pEnemyShotSet->param_i[4] = 0;
        pEnemyShotSet->param_i[5] = 0;
        pEnemyShotSet->param_i[6] = 0;
        pEnemyShotSet->param_i[7] = 0;

        //for (int i = 0; i < 8; i++) {
        //    pEnemyShotSet->param_d[i] = player.x;
        //    pEnemyShotSet->param_d[8 + i] = player.y;
        //}

        // パターンを存続させるための最初の弾。
        AddShot(pEnemyShotSet, enemy.x, enemy.y + 14.0, DX_PI * 0.5, 0.0, img_enemyShotMediumBall[6]);
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->param_i[0] = 0;
        pShot->param_i[1] = 0;
        pShot->param_d[0] = 0.0;
    }

    // --------------------------------------------------------
    // プレイヤー軌跡を常時記録。
    // --------------------------------------------------------
    if (pEnemyShotSet->count % 6 == 0) {
        int cursor = pEnemyShotSet->param_i[3];
        cursor = (cursor + 1) & 7;
        pEnemyShotSet->param_i[3] = cursor;
        pEnemyShotSet->param_d[cursor] = player.x;
        pEnemyShotSet->param_d[8 + cursor] = player.y;
    }

    int phase = pEnemyShotSet->param_i[0];
    if (pEnemyShotSet->count >= 480 && phase == 0) {
        phase = 1;
        pEnemyShotSet->param_i[0] = 1;
        pEnemyShotSet->param_i[1] = 0;
        pEnemyShotSet->param_i[2] = pEnemyShotSet->count + 54;
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    if (pEnemyShotSet->count >= 960 && phase == 1) {
        phase = 2;
        pEnemyShotSet->param_i[1] = 0;
        pEnemyShotSet->param_i[2] = pEnemyShotSet->count + 42;
     //   if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
     //   PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    // 最初の記録フェーズ：プレイヤーを直接狙う弾幕。
    // 後半で再生されるため、「無意識の移動癖」も記録される。
    // --------------------------------------------------------
    if (phase == 0) {
        if (pEnemyShotSet->count % 12 == 1) {
            double base = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            int fan = 9;
            for (int i = 0; i < fan; i++) {
                double angle = base + (i - (fan - 1) * 0.5) * (DX_PI * 0.028);
                AddShot(pEnemyShotSet,
                    pEnemyShotSet->x,
                    pEnemyShotSet->y,
                    angle,
                    2.6 + 0.18 * (i & 1),
                    img_enemyShotSmallBall[(i + pEnemyShotSet->count / 12) % 3]);
            }
        }

        // 記録した軌跡の周囲に遅れて中玉を置き、後半への伏線を作る。
        if (pEnemyShotSet->count % 30 == 1) {
            for (int i = 0; i < 8; i++) {
                double hx, hy;
                GetHistoryPoint(pEnemyShotSet, i, &hx, &hy);
                double angle = atan2(hy - 240.0, hx - 240.0) + DX_PI * 0.5;
                AddShot(pEnemyShotSet, hx, hy, angle, 0.65, img_enemyShotDiamond[(i + 1) % 3]);
                sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->param_i[0] = 2;
                pShot->param_i[1] = i;
            }
        }
    }

    // --------------------------------------------------------
    // 回廊を構成するレーザーを毎フレーム再配置。
    // 短レーザーを横一列／縦一列に敷き詰め、画面端まで壁にする。
    // --------------------------------------------------------
    double routeX = 240.0;
    double routeY = 240.0;
    int routeIndex = (pEnemyShotSet->count / 6) & 7;
    int delayedIndex = (routeIndex + 4) & 7;
    GetHistoryPoint(pEnemyShotSet, delayedIndex, &routeX, &routeY);
    routeX = Clamp(routeX, 52.0, 428.0);
    routeY = Clamp(routeY, 52.0, 428.0);

    if (phase >= 1) {
        int interval = (phase == 1) ? 54 : 42;
        if (pEnemyShotSet->count >= pEnemyShotSet->param_i[2]) {
            pEnemyShotSet->param_i[1] ^= 1;
            pEnemyShotSet->param_i[2] = pEnemyShotSet->count + interval;
            pEnemyShotSet->param_i[5]++;

            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        double gap = (phase == 1) ? 34.0 : 23.0;
        int orientation = pEnemyShotSet->param_i[1];

        // 回廊用レーザーは既存弾を「レーザー・セグメント」として再利用する。
        int laserCount = 8;
        int existingLasers = 0;
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 3) existingLasers++;
            pShot = pShot->next;
        }

        while (existingLasers < laserCount * 2) {
            int segment = existingLasers % laserCount;
            int side = existingLasers / laserCount;
            if (orientation == 0) {
                double x = 32.0 + segment * 64.0;
                double y = routeY + (side == 0 ? -gap : gap);
                AddShot(pEnemyShotSet, x, y, 0.0, 0.0, img_enemyShotLaser[5]);
            }
            else {
                double x = routeX + (side == 0 ? -gap : gap);
                double y = 32.0 + segment * 64.0;
                AddShot(pEnemyShotSet, x, y, DX_PI * 0.5, 0.0, img_enemyShotLaser[5]);
            }
            sEnemyShot* pNewShot = pEnemyShotSet->pEnemyShotHead->prev;
            pNewShot->param_i[0] = 3;
            pNewShot->param_i[1] = orientation;
            pNewShot->param_i[2] = segment;
            pNewShot->param_i[3] = side;
            existingLasers++;
        }

        // 既存のレーザーを回廊に追従させる。
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;
            if (pShot->param_i[0] == 3) {
                int segment = pShot->param_i[2];
                int side = pShot->param_i[3];
                if (orientation == 0) {
                    pShot->x = 32.0 + segment * 64.0;
                    pShot->y = routeY + (side == 0 ? -gap : gap);
                    pShot->muki = 0.0;
                }
                else {
                    pShot->x = routeX + (side == 0 ? -gap : gap);
                    pShot->y = 32.0 + segment * 64.0;
                    pShot->muki = DX_PI * 0.5;
                }
                pShot->speed = 0.0;
            }
            pShot = pNext;
        }

        // 軌跡再生弾。過去の移動方向をなぞるように外へ流す。
        if (pEnemyShotSet->count % ((phase == 1) ? 24 : 18) == 1) {
            int history = (pEnemyShotSet->param_i[4] + 3) & 7;
            int historyPrev = (history + 7) & 7;
            double hx, hy, px, py;
            GetHistoryPoint(pEnemyShotSet, history, &hx, &hy);
            GetHistoryPoint(pEnemyShotSet, historyPrev, &px, &py);
            double tangent = atan2(hy - py, hx - px);
            if (fabs(hx - px) + fabs(hy - py) < 4.0) {
                tangent += (pEnemyShotSet->param_i[5] & 1) ? DX_PI * 0.5 : -DX_PI * 0.5;
            }
            for (int side = -1; side <= 1; side += 2) {
                double angle = tangent + side * DX_PI * 0.5;
                if (player.x != hx || player.y != hy) {
                    AddShot(pEnemyShotSet, hx, hy, angle, phase == 1 ? 1.8 : 2.2,
                        img_enemyShotMediumOval[(history + side + 3) % 3]);
                }
                sEnemyShot* pNewShot = pEnemyShotSet->pEnemyShotHead->prev;
                pNewShot->param_i[0] = 2;
                pNewShot->param_i[1] = history;
            }
            pEnemyShotSet->param_i[4] = (history + 1) & 7;
        }

        // 回廊の中心へ向かって飛ぶ菱形弾。回廊内でも横断するため、
        // 「安全地帯に入っただけ」では終わらない。
        if (pEnemyShotSet->count % ((phase == 1) ? 36 : 24) == 7) {
            for (int i = 0; i < 7; i++) {
                double startX;
                double startY;
                if (orientation == 0) {
                    startX = 34.0 + i * 68.0;
                    startY = (i & 1) ? 478.0 : 2.0;
                }
                else {
                    startX = (i & 1) ? 478.0 : 2.0;
                    startY = 34.0 + i * 68.0;
                }
                double angle = atan2(routeY - startY, routeX - startX);
                if (orientation == 1)
                    angle = atan2(routeY - startY, routeX - startX) + ((i & 1) ? 0.06 : -0.06);
                AddShot(pEnemyShotSet, startX, startY, angle, phase == 1 ? 2.0 : 2.35,
                    img_enemyShotDiamond[(i + pEnemyShotSet->param_i[5]) % 3]);
            }
        }
    }

    // --------------------------------------------------------
    // 終盤：追従レーザーに加えてプレイヤー狙いの密度を上げる。
    // --------------------------------------------------------
    if (phase == 2) {
        if (!pEnemyShotSet->param_i[6] && pEnemyShotSet->count % 10 == 1) {
            double base = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            for (int i = -2; i <= 2; i++) {
                double angle = base + i * DX_PI * 0.035;
                AddShot(pEnemyShotSet,
                    pEnemyShotSet->x,
                    pEnemyShotSet->y,
                    angle,
                    3.05 + 0.12 * (i & 1 ? 1 : 0),
                    img_enemyShotSmallBall[(pEnemyShotSet->count / 10 + i + 9) % 3]);
            }
        }

        // 過去の軌跡を「未来の安全位置」としてもう一度使う高速再生。
        if (!pEnemyShotSet->param_i[6] && pEnemyShotSet->count % 30 == 3) {
            int index = (pEnemyShotSet->count / 30 + 2) & 7;
            double hx, hy;
            GetHistoryPoint(pEnemyShotSet, index, &hx, &hy);
            for (int i = 0; i < 6; i++) {
                double angle = i * DX_PI / 3.0 + pEnemyShotSet->count * 0.008;
                AddShot(pEnemyShotSet, hx, hy, angle, 2.5,
                    img_enemyShotScale[(i + index) % 3]);
            }
        }

        // 一定周期だけ全体が静止する「解答時間」。
        // その直前までに作られた軌跡が、画面上の解答用紙になる。
        int local = pEnemyShotSet->count % 300;
        if (local == 1) {
            pEnemyShotSet->param_i[6] = 1;
            pEnemyShotSet->param_i[7] = pEnemyShotSet->count;
            for (sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
                pShot != pEnemyShotSet->pEnemyShotHead; pShot = pShot->next) {
                pShot->param_d[15] = pShot->speed;
                pShot->speed = 0.0;
            }
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
        if (pEnemyShotSet->param_i[6] &&
            pEnemyShotSet->count - pEnemyShotSet->param_i[7] >= 18) {
            pEnemyShotSet->param_i[6] = 0;
            for (sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
                pShot != pEnemyShotSet->pEnemyShotHead; pShot = pShot->next) {
                pShot->speed = pShot->param_d[15];
            }
        }
    }

    // --------------------------------------------------------
    // 最後に各弾を更新。
    // --------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 0) {
            // 通常小玉。一定時間ごとにわずかに蛇行する。
            pShot->muki += sin(pShot->count * 0.055 + pShot->param_d[0]) * 0.00055;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[0] == 2) {
            // 軌跡再生弾。muki は履歴上の接線方向。
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            // 楕円弾は進行方向を向ける。
            if (pShot->kind >= img_enemyShotMediumOval[0] &&
                pShot->kind <= img_enemyShotMediumOval[8]) {
                pShot->muki = pShot->muki;
            }
        }
        else if (pShot->param_i[0] == 3) {
            // レーザーは絶対座標で回廊位置を更新するため移動しない。
            pShot->speed = 0.0;
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_TenYears_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 48.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTenYear;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 14.0;
        pEnemyShotSet->muki = DX_PI * 0.5;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // ボスはゆっくり左右に揺れ続け、記録される移動履歴にも影響を与える。
        enemy.x += 0.72 * muki;
        if (enemy.x < 96.0 || enemy.x > 384.0)
            muki *= -1;

        sEnemyShotSet* pEnemyShotSet = enemyShotSetHead.next;
        while (pEnemyShotSet != &enemyShotSetHead) {
            if (pEnemyShotSet->patternFunc == ShotTenYear) {
                pEnemyShotSet->x = enemy.x;
                pEnemyShotSet->y = enemy.y + 14.0;
                break;
            }
            pEnemyShotSet = pEnemyShotSet->next;
        }
    }
}
