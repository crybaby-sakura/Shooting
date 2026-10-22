#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <vector>

static const int INPUT_N = 5;
static const int HIDDEN_N = 8;

// ------------------------------------------------------------
// ヘルパー: ShotSetに1発追加
// type: 0=入力ニューロン, 1=隠れニューロン, 2=出力ニューロン
// 10=シナプス 入力->隠れ, 11=シナプス 隠れ->出力
// 20=信号 入力->隠れ, 21=信号 隠れ->出力, 22=信号 出力->自機
// ------------------------------------------------------------
static sEnemyShot* AddShot(sEnemyShotSet* set, double x, double y, double muki, double speed, int kindImg,
    int type, int idx1 = 0, int idx2 = 0, double tX = 0.0, double tY = 0.0)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kindImg;
    p->count = 0;
    p->margin = 30.0;
    for (int i = 0; i < 16; i++) { p->param_i[i] = 0; p->param_d[i] = 0.0; }
    p->param_i[0] = type;
    p->param_i[1] = idx1;
    p->param_i[2] = idx2;
    p->param_d[0] = tX;
    p->param_d[1] = tY;

    p->prev = set->pEnemyShotHead->prev;
    p->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = p;
    set->pEnemyShotHead->prev = p;
    return p;
}

// ------------------------------------------------------------
// 弾幕本体: Synaptic Cascade
// ------------------------------------------------------------
static void Shot_SynapticCascade(sEnemyShotSet* pSet)
{
    // 固定座標
    static const double inY = 80.0;
    static const double hidCenterX = 240.0;
    static const double hidCenterY = 180.0;
    static const double hidR = 70.0;
    static const double outX = 240.0;
    static const double outY = 300.0;

    double inX[INPUT_N];
    double hidX[HIDDEN_N];
    double hidY[HIDDEN_N];
    for (int i = 0; i < INPUT_N; i++) inX[i] = 480.0 * (i + 1) / (INPUT_N + 1); // 80,160,240,320,400
    for (int j = 0; j < HIDDEN_N; j++) {
        double ang = 2.0 * DX_PI * j / HIDDEN_N - DX_PI * 0.5;
        hidX[j] = hidCenterX + hidR * cos(ang);
        hidY[j] = hidCenterY + hidR * sin(ang);
    }

    // 初回: ネットワーク構造を生成
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 16; i++) { pSet->param_i[i] = 0; pSet->param_d[i] = 0.0; }
        pSet->param_i[8] = 0; // 強化された隠れインデックス
        pSet->param_i[9] = 0; // 出力到達カウント
        pSet->param_i[10] = 0;

        // シナプス: Laserを線として2分割配置で表現
        for (int i = 0; i < INPUT_N; i++) {
            for (int j = 0; j < HIDDEN_N; j++) {
                double dx = hidX[j] - inX[i];
                double dy = hidY[j] - inY;
                double ang = atan2(dy, dx);
                double dist = hypot(dx, dy);
                for (int k = 0; k < 2; k++) {
                    double r = (k + 0.5) / 2.0;
                    double x = inX[i] + dx * r;
                    double y = inY + dy * r;
                    // 通常シナプスは白レーザーで薄く見せる
                    AddShot(pSet, x, y, ang, 0, img_enemyShotLaser[6], 10, i, j);
                }
            }
        }
        for (int j = 0; j < HIDDEN_N; j++) {
            double dx = outX - hidX[j];
            double dy = outY - hidY[j];
            double ang = atan2(dy, dx);
            for (int k = 0; k < 2; k++) {
                double r = (k + 0.5) / 2.0;
                double x = hidX[j] + dx * r;
                double y = hidY[j] + dy * r;
                AddShot(pSet, x, y, ang, 0, img_enemyShotLaser[6], 11, j, 0);
            }
        }

        // ニューロン本体: LargeBall 固定・無害に見せるため speed=0
        for (int i = 0; i < INPUT_N; i++) {
            sEnemyShot* p = AddShot(pSet, inX[i], inY, 0, 0, img_enemyShotLargeBall[4], 0, i);
            p->param_i[3] = 0; // 発光タイマー
        }
        for (int j = 0; j < HIDDEN_N; j++) {
            sEnemyShot* p = AddShot(pSet, hidX[j], hidY[j], 0, 0, img_enemyShotLargeBall[2], 1, j);
            p->param_i[3] = 0;
        }
        {
            sEnemyShot* p = AddShot(pSet, outX, outY, 0, 0, img_enemyShotLargeBall[0], 2, 0);
            p->param_i[3] = 0;
        }
        
        return;
    }

    // --- 毎フレーム更新 ---

    // 1. ニューロンの発光処理
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 0 || p->param_i[0] == 1 || p->param_i[0] == 2) {
            if (p->param_i[3] > 0) {
                p->param_i[3]--;
                p->kind = img_enemyShotLargeBall[6]; // 発火中は白
            }
            else {
                if (p->param_i[0] == 0) p->kind = img_enemyShotLargeBall[4]; // 青
                if (p->param_i[0] == 1) p->kind = img_enemyShotLargeBall[2]; // 緑
                if (p->param_i[0] == 2) p->kind = img_enemyShotLargeBall[0]; // 赤
            }
        }
        p = p->next;
    }

    // 2. 学習: プレイヤーに最も近い隠れニューロンを強化
    if (pSet->count % 180 == 0) {
        int nearest = 0;
        double best = 1e9;
        for (int j = 0; j < HIDDEN_N; j++) {
            double d = fabs(hidX[j] - player.x) + fabs(hidY[j] - player.y) * 0.3;
            if (d < best) { best = d; nearest = j; }
        }
        pSet->param_i[8] = nearest;

        // シナプス色更新
        sEnemyShot* s = pSet->pEnemyShotHead->next;
        while (s != pSet->pEnemyShotHead) {
            if (s->param_i[0] == 11) { // 隠れ->出力
                if (s->param_i[1] == nearest) s->kind = img_enemyShotLaser[0]; // 赤で強調
                else s->kind = img_enemyShotLaser[6]; // 白に戻す
            }
            s = s->next;
        }
    }

    // 3. 入力層の発火: 60F毎にランダムな入力ニューロンが全隠れへ信号を送る
    struct SpawnInfo { double x, y, muki, speed, tx, ty; int kind; int type, idx1, idx2; };
    std::vector<SpawnInfo> spawnQueue;

    if (pSet->count % 60 == 30) {
        int inputIdx = GetRand(INPUT_N - 1); // 0..4 : GetRandは 0～x inclusive なので N-1 を渡す
        // 入力ニューロン発光
        sEnemyShot* q = pSet->pEnemyShotHead->next;
        while (q != pSet->pEnemyShotHead) {
            if (q->param_i[0] == 0 && q->param_i[1] == inputIdx) q->param_i[3] = 20;
            q = q->next;
        }
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int j = 0; j < HIDDEN_N; j++) {
            double ang = atan2(hidY[j] - inY, hidX[j] - inX[inputIdx]);
            double spd = 3.2 + GetRand(40) / 100.0; // 3.2～3.6
            SpawnInfo si;
            si.x = inX[inputIdx]; si.y = inY; si.muki = ang; si.speed = spd;
            si.tx = hidX[j]; si.ty = hidY[j];
            si.kind = img_enemyShotSmallBall[3]; // シアン小玉
            si.type = 20; si.idx1 = j; si.idx2 = inputIdx;
            spawnQueue.push_back(si);
        }
    }

    // 4. 信号弾の移動と到達判定
    sEnemyShot* cur = pSet->pEnemyShotHead->next;
    while (cur != pSet->pEnemyShotHead) {
        sEnemyShot* nxt = cur->next;
        int tp = cur->param_i[0];
        if (tp == 20 || tp == 21 || tp == 22) {
            cur->x += cur->speed * cos(cur->muki);
            cur->y += cur->speed * sin(cur->muki);

            bool arrived = false;
            if (tp == 20 || tp == 21) {
                double dx = cur->param_d[0] - cur->x;
                double dy = cur->param_d[1] - cur->y;
                if (hypot(dx, dy) < cur->speed + 6.0) arrived = true;
            }

            if (arrived) {
                if (tp == 20) {
                    int hidIdx = cur->param_i[1];
                    pSet->param_i[hidIdx]++; // 隠れ到達カウント

                    // 閾値 2で隠れ発火
                    if (pSet->param_i[hidIdx] >= 2) {
                        pSet->param_i[hidIdx] = 0;
                        // 隠れ発光
                        sEnemyShot* h = pSet->pEnemyShotHead->next;
                        while (h != pSet->pEnemyShotHead) {
                            if (h->param_i[0] == 1 && h->param_i[1] == hidIdx) h->param_i[3] = 18;
                            h = h->next;
                        }
                        // 隠れ->出力 信号生成
                        int reinforced = pSet->param_i[8];
                        int ways = (hidIdx == reinforced) ? 3 : 1;
                        for (int w = 0; w < ways; w++) {
                            double base = atan2(outY - hidY[hidIdx], outX - hidX[hidIdx]);
                            double ang = base + (ways == 3 ? (w - 1) * 0.18 : 0.0);
                            SpawnInfo si;
                            si.x = hidX[hidIdx]; si.y = hidY[hidIdx];
                            si.muki = ang; si.speed = 4.0;
                            si.tx = outX; si.ty = outY;
                            si.kind = img_enemyShotMediumBall[1]; // 黄中玉
                            si.type = 21; si.idx1 = 0; si.idx2 = hidIdx;
                            spawnQueue.push_back(si);
                        }
                    }
                }
                else if (tp == 21) {
                    pSet->param_i[9]++; // 出力到達
                    if (pSet->param_i[9] >= 3) {
                        pSet->param_i[9] = 0;
                        // 出力発光
                        sEnemyShot* o = pSet->pEnemyShotHead->next;
                        while (o != pSet->pEnemyShotHead) {
                            if (o->param_i[0] == 2) o->param_i[3] = 30;
                            o = o->next;
                        }
                        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                        int n = 18 + GetRand(6*4); // 18～24 way
                        double base = atan2(player.y - outY, player.x - outX) + GetRand(10000);
                        for (int i = 0; i < n; i++) {
                            double ang = base + 2.0 * DX_PI * i / n + (GetRand(20) - 10) / 100.0 * 0.15;
                            double spd = 2.6 + GetRand(80) / 100.0;
                            SpawnInfo si;
                            si.x = outX; si.y = outY; si.muki = ang; si.speed = spd;
                            si.tx = 0; si.ty = 0;
                            si.kind = (i % 2 == 0) ? img_enemyShotMediumBall[0] : img_enemyShotSmallBall[8];
                            si.type = 22;
                            spawnQueue.push_back(si);
                        }
                    }
                }
                // 到達した信号弾を削除 (プールに返却)
                cur->prev->next = cur->next;
                cur->next->prev = cur->prev;
                delete cur;
            }
        }
        cur = nxt;
    }

    // 遅延生成した弾を追加
    for (auto& si : spawnQueue) {
        AddShot(pSet, si.x, si.y, si.muki, si.speed, si.kind, si.type, si.idx1, si.idx2, si.tx, si.ty);
    }
}

// 敵本体
void EnemyPat_NeuralNetwork_MetaAI()
{
    static bool netCreated = false;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        netCreated = false;
    }

    // 敵は中央でほぼ固定、視認性のため微左右
    enemy.x = 240.0 + sin(count * 0.015) * 10.0;

    if (!netCreated) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Shot_SynapticCascade;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0;
        pSet->kind = 0;
        for (int i = 0; i < 16; i++) { pSet->param_i[i] = 0; pSet->param_d[i] = 0.0; }

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        netCreated = true;
    }
}