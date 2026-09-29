// 弾幕：連鎖衝突『ニュートン・クレイドル』
//
// 【概要】
//  画面上部に5つの振り子(支持バー・糸・錘)を並べ、球の衝突に合わせて弾をばら撒く。
//  糸と錘とバーは専用素材が無いので、既存の敵弾を組み合わせて表現している。
//    支持バー … 菱形弾を横一列に並べたもの
//    糸       … 銃弾(muki を糸の向きに合わせる)を支点から錘まで一列に並べたもの
//    錘       … 大玉
//
//  フェーズ構成(1ラウンド ≒ 1800フレーム。ラウンドが終わったら最初から繰り返す)
//    第1段階 : 1個振り
//    第2段階 : 2個振り(衝突弾の扇が2重になる)
//    第3段階 : 3個振り(支持バーが左右に揺れ、衝突時に中玉リングが加わる)
//    最終段階 : 位相ウェーブ → 糸切れ(糸とバーが弾になって四散し、錘が自機へ飛ぶ)
//
//  実装方針
//    ・すべての弾は1つの ShotSet(ShotCradle)にまとめ、param_i[0] の役割(role)で動きを切り替える。
//      糸・錘・バーの位置は毎フレーム「時刻tだけから決まる関数」で計算して直接配置する。
//      (弾を削除されても位置が狂わない/リプレイ再現性が高い)
//    ・乱数は GetRand のみ使用(シード値がリプレイに保存されるので再現性あり)。
//    ・count 系のインクリメントと画面外の弾の消去はメインルーチンに任せる。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {

    // ============================================================
    //  定数(短い名前は Windows SDK と衝突するので NCR_ を付ける)
    // ============================================================
    const double NCR_CX = 240.0;   // 支持バーの中心x
    const double NCR_BAR_Y = 64.0-30;    // 支持バーのy
    const double NCR_LEN = 180.0;   // 糸の長さ(支点〜錘の中心)
    const double NCR_BALL_R = 20.0;    // 錘(大玉)の半径
    const double NCR_SPACE0 = 40.0;    // 通常時の支点間隔(球どうしが接する間隔)
    const double NCR_WAVE_OPEN = 20.0;    // ウェーブ時に支点間隔を広げる量
    const double NCR_TWO_PI = 2.0 * DX_PI;

    const int    NCR_BALLS = 5;       // 振り子の数
    const int    NCR_STR_N = 16;      // 1本の糸を構成する銃弾の数
    const double NCR_STR_R0 = 6.0;     // 糸の先頭の弾の、支点からの距離
    const double NCR_STR_PITCH = 9.8;     // 糸の弾の間隔(銃弾の全長 10 とほぼ同じ)
    const int    NCR_BAR_N = 34;      // 支持バーを構成する菱形弾の数

    // --- 通常フェーズ(第1〜第3段階) ---
    const int    NCR_PHASES = 3;
    const int    NCR_K[NCR_PHASES] = { 1, 2, 3 };          // 同時に振れる球の数
    const int    NCR_TQ[NCR_PHASES] = { 40, 36, 34 };       // 振り下ろし/跳ね上がり1回のフレーム数
    const double NCR_THETA[NCR_PHASES] = { 45.0 * DX_PI / 180.0, 50.0 * DX_PI / 180.0, 55.0 * DX_PI / 180.0 };
    const int    NCR_HITS[NCR_PHASES] = { 4, 4, 5 };          // 衝突回数(最後の衝突で振り子は静止する)
    const int    NCR_LIFT_LEN = 60;   // 最初の持ち上げ(予告)のフレーム数
    const int    NCR_REST_LEN = 40;   // フェーズ間のインターバル
    const double NCR_SWAY_AMP = 12.0; // 第3段階で支持バーが左右に揺れる幅

    // 衝突弾(扇)
    const int    NCR_FAN_LAYER = 3;                   // 速度の異なる扇の段数
    const int    NCR_FAN_N[NCR_PHASES] = { 20, 22, 24 };    // 扇1段あたりの弾数
    const int    NCR_FAN_SETS[NCR_PHASES] = { 1, 1, 1 };       // 扇の重ね枚数
    const int    NCR_FAN_COL[4] = { 1, 8, 0, 5 };    // 段ごとの色(黄・橙・赤・マゼンタ)
    const int    NCR_RING_N[NCR_PHASES] = { 0, 0, 32-8 };      // 衝突時の中玉リングの弾数

    // 振り上げ頂点からの自機狙い弾
    const int    NCR_APEX_WAY[NCR_PHASES] = { 7, 5, 5 };       // 1個の球あたりのway数
    const int    NCR_APEX_COL[NCR_PHASES] = { 0, 8, 5 };       // 鱗弾の色(赤・橙・マゼンタ)

    // --- 最終段階 ---
    const double NCR_WAVE_T = 100.0;                       // ウェーブ1周期のフレーム数
    const int    NCR_WAVE_LEN = 420;                         // ウェーブ全体の長さ
    const int    NCR_WAVE_IN = 50;                          // 振幅が立ち上がるフレーム数
    const int    NCR_WAVE_OUT = 40;                          // 振幅が収まるフレーム数
    const double NCR_WAVE_THETA = 35.0 * DX_PI / 180.0;        // ウェーブの最大振幅
    const int    NCR_WAVE_COL[NCR_BALLS] = { 0, 1, 2, 3, 4 };  // 球ごとの色(赤・黄・緑・シアン・青)
    const int    NCR_CHARGE_LEN = 70;                          // 糸切れ前の予告(点滅・震え)のフレーム数
    const int    NCR_AFTER_LEN = 150;                         // 糸切れ後、次のラウンドが始まるまでのフレーム数

    // --- 弾の役割(param_i[0])。0 を FREE にしておくと設定漏れがあっても安全 ---
    const int NCR_ROLE_FREE = 0;   // 通常の弾(速度と向きで動く)
    const int NCR_ROLE_BAR = 1;   // 支持バー(param_i[1]: バー内の番号)
    const int NCR_ROLE_STRING = 2;   // 糸(param_i[1]: 振り子の番号, param_i[2]: 糸内の番号)
    const int NCR_ROLE_BALL = 3;   // 錘(param_i[1]: 振り子の番号)

    // ストロークの種類
    const int NCR_LIFT = 0;   // 最初の持ち上げ
    const int NCR_DOWN = 1;   // 振り下ろし
    const int NCR_UP = 2;   // 跳ね上がり
    const int NCR_REST = 3;   // 静止

    // ============================================================
    //  スケジュール(ラウンド内の時刻表。定数だけから決まるので毎回同じ)
    // ============================================================
    struct sNcrStroke {
        int start;      // 開始フレーム(ラウンド先頭からの相対)
        int len;        // 継続フレーム数
        int type;       // NCR_LIFT / NCR_DOWN / NCR_UP / NCR_REST
        int side;       // -1: 左側の球が動く  +1: 右側の球が動く
        int phase;      // 第何段階か(0〜2)
        int finalHit;   // 1: この振り下ろしが最後の衝突(以降は静止)
    };

    const int NCR_MAX_STROKE = 64;
    sNcrStroke g_ncrStroke[NCR_MAX_STROKE];
    int  g_ncrStrokeN = 0;
    int  g_ncrPhaseStart[NCR_PHASES];
    int  g_ncrPhaseEnd[NCR_PHASES];
    int  g_ncrTWave = 0;     // ウェーブ開始
    int  g_ncrTCharge = 0;   // 糸切れ予告開始
    int  g_ncrTCut = 0;      // 糸切れ
    int  g_ncrTEnd = 0;      // ラウンド終了(次のラウンド開始)
    bool g_ncrBuilt = false;

    void NcrAddStroke(int& t, int len, int type, int side, int phase, int finalHit)
    {
        sNcrStroke& s = g_ncrStroke[g_ncrStrokeN++];
        s.start = t;
        s.len = len;
        s.type = type;
        s.side = side;
        s.phase = phase;
        s.finalHit = finalHit;
        t += len;
    }

    void NcrBuild()
    {
        if (g_ncrBuilt) return;
        g_ncrBuilt = true;
        g_ncrStrokeN = 0;
        int t = 0;
        NcrAddStroke(t, 30, NCR_REST, 0, 0, 0);   // 開始直後の静止
        for (int p = 0; p < NCR_PHASES; p++) {
            g_ncrPhaseStart[p] = t;
            int side = -1;   // 最初は左端から持ち上げる
            NcrAddStroke(t, NCR_LIFT_LEN, NCR_LIFT, side, p, 0);
            for (int h = 0; h < NCR_HITS[p]; h++) {
                int fin = (h == NCR_HITS[p] - 1) ? 1 : 0;
                NcrAddStroke(t, NCR_TQ[p], NCR_DOWN, side, p, fin);
                if (!fin) {
                    side = -side;   // 反対側の球が跳ね上がる
                    NcrAddStroke(t, NCR_TQ[p], NCR_UP, side, p, 0);
                }
            }
            g_ncrPhaseEnd[p] = t;
            NcrAddStroke(t, NCR_REST_LEN, NCR_REST, 0, p, 0);
        }
        g_ncrTWave = t;
        g_ncrTCharge = g_ncrTWave + NCR_WAVE_LEN;
        g_ncrTCut = g_ncrTCharge + NCR_CHARGE_LEN;
        g_ncrTEnd = g_ncrTCut + NCR_AFTER_LEN;
    }

    // ============================================================
    //  補助関数
    // ============================================================
    double NcrSmooth(double u)
    {
        if (u < 0.0) u = 0.0;
        if (u > 1.0) u = 1.0;
        return u * u * (3.0 - 2.0 * u);
    }

    void NcrPlaySound(int handle)
    {
        if (CheckSoundMem(handle)) StopSoundMem(handle);
        PlaySoundMem(handle, DX_PLAYTYPE_BACK);
    }

    // 真下(π/2)から±0.7rad の範囲に向きを収める
    double NcrClampDown(double ang)
    {
        double d = ang - DX_PI * 0.5;
        while (d > DX_PI) d -= NCR_TWO_PI;
        while (d < -DX_PI) d += NCR_TWO_PI;
        if (d > 0.7) d = 0.7;
        if (d < -0.7) d = -0.7;
        return DX_PI * 0.5 + d;
    }

    // 支持バーの中心x(第3段階だけ左右にゆっくり揺れる)。敵本体の動きとも共用する。
    double NcrSway(int rt)
    {
        int s = g_ncrPhaseStart[NCR_PHASES - 1];
        int e = g_ncrPhaseEnd[NCR_PHASES - 1];
        if (rt < s || rt > e) return 0.0;
        double env = 1.0;
        double a = NcrSmooth((double)(rt - s) / 60.0);
        double b = NcrSmooth((double)(e - rt) / 60.0);
        if (a < env) env = a;
        if (b < env) env = b;
        return NCR_SWAY_AMP * env * sin(NCR_TWO_PI * (double)(rt - s) / 200.0);
    }

    // ウェーブ中の、球iの位相
    double NcrWaveU(int w, int i)
    {
        return NCR_TWO_PI * (double)w / NCR_WAVE_T - (double)i * (NCR_TWO_PI / NCR_BALLS);
    }

    // ============================================================
    //  ラウンド内時刻tにおける振り子全体の姿勢(tだけから決まる)
    // ============================================================
    struct sNcrPose {
        double theta[NCR_BALLS];    // 各振り子の角度(右向きが正)
        double spacing;             // 支点間隔
        double waveEnv;             // ウェーブの振幅係数(0〜1)
        int    mode;                // 0:通常  1:ウェーブ  2:糸切れ予告
        int    moving[NCR_BALLS];   // 1: いま振れている球(色分け用)
        int    flash[NCR_BALLS];    // 1: 衝撃が伝わって光っている球
    };

    void NcrCalcPose(int t, sNcrPose* pose)
    {
        for (int i = 0; i < NCR_BALLS; i++) {
            pose->theta[i] = 0.0;
            pose->moving[i] = 0;
            pose->flash[i] = 0;
        }
        pose->spacing = NCR_SPACE0;
        pose->waveEnv = 0.0;
        pose->mode = 0;

        // ---- 通常フェーズ ----
        if (t < g_ncrTWave) {
            int lastHitT = -1;
            int lastHitSide = 0;
            for (int n = 0; n < g_ncrStrokeN; n++) {
                const sNcrStroke& s = g_ncrStroke[n];
                int endT = s.start + s.len;
                if (s.type == NCR_DOWN && endT <= t) {
                    lastHitT = endT;
                    lastHitSide = s.side;
                }
                if (t >= s.start && t < endT && s.type != NCR_REST) {
                    double u = (double)(t - s.start) / (double)s.len;
                    double mag = 0.0;
                    if (s.type == NCR_LIFT)      mag = 0.5 - 0.5 * cos(DX_PI * u);   // ゆっくり持ち上げる
                    else if (s.type == NCR_DOWN) mag = cos(DX_PI * 0.5 * u);         // 最高点から最下点へ
                    else                         mag = sin(DX_PI * 0.5 * u);         // 最下点から最高点へ
                    double th = NCR_THETA[s.phase] * mag * (double)s.side;
                    int k = NCR_K[s.phase];
                    for (int j = 0; j < k; j++) {
                        int i = (s.side < 0) ? j : (NCR_BALLS - 1 - j);
                        pose->theta[i] = th;
                        pose->moving[i] = 1;
                    }
                }
            }
            // 衝突の衝撃が、衝突した側から順に球を伝わっていく
            if (lastHitT >= 0) {
                for (int i = 0; i < NCR_BALLS; i++) {
                    int p = (lastHitSide < 0) ? i : (NCR_BALLS - 1 - i);
                    int age = t - lastHitT - 2 * p;
                    if (age >= 0 && age < 7) pose->flash[i] = 1;
                }
            }
            return;
        }

        // ---- ウェーブ ----
        int w = t - g_ncrTWave;
        if (t < g_ncrTCharge) {
            pose->mode = 1;
            double env = 1.0;
            if (w < NCR_WAVE_IN) env = NcrSmooth((double)w / (double)NCR_WAVE_IN);
            int rem = NCR_WAVE_LEN - w;
            if (rem < NCR_WAVE_OUT) {
                double e2 = NcrSmooth((double)rem / (double)NCR_WAVE_OUT);
                if (e2 < env) env = e2;
            }
            pose->waveEnv = env;
            pose->spacing = NCR_SPACE0 + NCR_WAVE_OPEN * env;
            for (int i = 0; i < NCR_BALLS; i++) {
                pose->theta[i] = env * NCR_WAVE_THETA * sin(NcrWaveU(w, i));
                pose->moving[i] = 1;
            }
            return;
        }

        // ---- 糸切れ予告(だんだん強く震える) ----
        pose->mode = 2;
        double ramp = (double)(t - g_ncrTCharge) / (double)NCR_CHARGE_LEN;
        for (int i = 0; i < NCR_BALLS; i++) {
            pose->theta[i] = 0.02 * ramp * sin(0.9 * (double)(t - g_ncrTCharge) + (double)i * 1.7);
        }
    }

    // 球iの中心座標
    void NcrBallPos(const sNcrPose& pose, double cx, int i, double* px, double* py)
    {
        *px = cx + (double)(i - 2) * pose.spacing + NCR_LEN * sin(pose.theta[i]);
        *py = NCR_BAR_Y + NCR_LEN * cos(pose.theta[i]);
    }

    // ============================================================
    //  弾の生成
    // ============================================================
    sEnemyShot* NcrAddShot(sEnemyShotSet* pEnemyShotSet, int kind, double x, double y, double muki, double speed, int role)
    {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = x;
        pEnemyShot->y = y;
        pEnemyShot->muki = muki;
        pEnemyShot->speed = speed;
        pEnemyShot->kind = kind;
        pEnemyShot->param_i[0] = role;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        return pEnemyShot;
    }

    // 通常の弾。param_d[0]: 加速度(毎フレーム speed に加算)  param_d[1]: 最高速度  param_d[2]: 重力(毎フレーム下向きに加算)
    // muki は常に進行方向と一致させる。
    void NcrAddFree(sEnemyShotSet* pEnemyShotSet, int kind, double x, double y, double muki, double speed,
        double acc, double maxSpeed, double gravity)
    {
        sEnemyShot* p = NcrAddShot(pEnemyShotSet, kind, x, y, muki, speed, NCR_ROLE_FREE);
        p->param_d[0] = acc;
        p->param_d[1] = maxSpeed;
        p->param_d[2] = gravity;
    }

    // 最初に、支持バー・糸・錘の弾を並べる(位置はこの直後の更新で確定する)
    void NcrCreateBullets(sEnemyShotSet* pEnemyShotSet)
    {
        for (int j = 0; j < NCR_BAR_N; j++) {
            sEnemyShot* p = NcrAddShot(pEnemyShotSet, img_enemyShotDiamond[6], NCR_CX, NCR_BAR_Y, 0.0, 0.0, NCR_ROLE_BAR);
            p->param_i[1] = j;
        }
        for (int i = 0; i < NCR_BALLS; i++) {
            for (int j = 0; j < NCR_STR_N; j++) {
                sEnemyShot* p = NcrAddShot(pEnemyShotSet, img_enemyShotBullet[6], NCR_CX, NCR_BAR_Y, DX_PI * 0.5, 0.0, NCR_ROLE_STRING);
                p->param_i[1] = i;
                p->param_i[2] = j;
            }
        }
        for (int i = 0; i < NCR_BALLS; i++) {
            sEnemyShot* p = NcrAddShot(pEnemyShotSet, img_enemyShotLargeBall[6], NCR_CX, NCR_BAR_Y + NCR_LEN, DX_PI * 0.5, 0.0, NCR_ROLE_BALL);
            p->param_i[1] = i;
        }
    }

    // ============================================================
    //  イベント弾
    // ============================================================
    // 衝突：衝突点から下向きの扇(＋第3段階は中玉リング)
    void NcrCollision(sEnemyShotSet* pEnemyShotSet, const sNcrStroke& s, double cx)
    {
        int k = NCR_K[s.phase];
        double idx = (s.side < 0) ? (double)((k - 1) - 2) : (double)((NCR_BALLS - k) - 2);
        double hx = cx + idx * NCR_SPACE0 + ((s.side < 0) ? NCR_BALL_R : -NCR_BALL_R);
        double hy = NCR_BAR_Y + NCR_LEN;

        NcrPlaySound(s.finalHit ? sound_enemyShot_extreme : sound_enemyShot_heavy);

        // 扇：自機方向(真下から±40°に制限)を中心に、速度違いの段を重ねる
        double center = NcrClampDown(atan2(player.y - hy, player.x - hx));
        int n = NCR_FAN_N[s.phase];
        int fans = NCR_FAN_SETS[s.phase];
        double step = (NCR_TWO_PI / 3.0) / (double)(n - 1);   // 120°の範囲
        for (int f = 0; f < fans; f++) {
            for (int l = 0; l < NCR_FAN_LAYER; l++) {
                for (int w = 0; w < n; w++) {
                    double a = center + ((double)w - (double)(n - 1) * 0.5) * step + (double)f * step * 0.5;
                    double sp = 1.8 + 0.4 * (double)l + 0.3 * (double)f;
                    NcrAddFree(pEnemyShotSet, img_enemyShotBullet[NCR_FAN_COL[(l + f) % 4]], hx, hy, a, sp, 0.0, 0.0, 0.0);
                }
            }
        }

        // 衝撃波：ゆっくり広がってから加速する中玉リング
        int rn = NCR_RING_N[s.phase];
        if (rn > 0) {
            double base = GetRand(359) * DX_PI / 180.0;
            for (int r = 0; r < rn; r++) {
                double a = base + NCR_TWO_PI * (double)r / (double)rn;
                NcrAddFree(pEnemyShotSet, img_enemyShotMediumBall[3], hx, hy, a, 1.2, 0.03, 3.2, 0.0);
            }
        }
    }

    // 振り上げ頂点(振り下ろしの開始直後)：動いている球それぞれから、自機狙いのn-way鱗弾を3連射
    void NcrVolley(sEnemyShotSet* pEnemyShotSet, const sNcrStroke& s, const sNcrPose& pose, double cx, int v)
    {
        int k = NCR_K[s.phase];
        int way = NCR_APEX_WAY[s.phase];
        if (v == 0) NcrPlaySound(sound_enemyShot_medium);
        for (int j = 0; j < k; j++) {
            int i = (s.side < 0) ? j : (NCR_BALLS - 1 - j);
            double bx, by;
            NcrBallPos(pose, cx, i, &bx, &by);
            double aim = atan2(player.y - by, player.x - bx);
            for (int w = 0; w < way; w++) {
                double a = aim + ((double)w - (double)(way - 1) * 0.5) * 0.14;
                NcrAddFree(pEnemyShotSet, img_enemyShotScale[NCR_APEX_COL[s.phase]], bx, by, a, 3.0 + 0.35 * (double)v, 0.0, 0.0, 0.0);
            }
        }
    }

    // ウェーブ：最下点を通過するたびに自機狙い7way、最大振幅に達するたびに小玉リング
    void NcrWaveEvents(sEnemyShotSet* pEnemyShotSet, int t, const sNcrPose& pose, double cx)
    {
        int w = t - g_ncrTWave;
        if (w < 1 || pose.mode != 1 || pose.waveEnv < 0.7) return;
        bool played = false;
        for (int i = 0; i < NCR_BALLS; i++) {
            double u = NcrWaveU(w, i);
            double u0 = NcrWaveU(w - 1, i);
            double bx, by;
            NcrBallPos(pose, cx, i, &bx, &by);
            if (floor(u / DX_PI) != floor(u0 / DX_PI)) {
                double aim = atan2(player.y - by, player.x - bx);
                for (int k = 0; k < 7; k++) {
                    NcrAddFree(pEnemyShotSet, img_enemyShotScale[6], bx, by, aim + (double)(k - 3) * 0.16, 2.6, 0.0, 0.0, 0.0);
                }
                played = true;
            }
            if (floor((u - DX_PI * 0.5) / DX_PI) != floor((u0 - DX_PI * 0.5) / DX_PI)) {
                double base = GetRand(359) * DX_PI / 180.0;
                for (int k = 0; k < 12; k++) {
                    double a = base + NCR_TWO_PI * (double)k / 12.0;
                    NcrAddFree(pEnemyShotSet, img_enemyShotSmallBall[NCR_WAVE_COL[i]], bx, by, a, 1.3, 0.02, 2.6-0.8, 0.0);
                }
                played = true;
            }
        }
        if (played) NcrPlaySound(sound_enemyShot_light);
    }

    void NcrEvents(sEnemyShotSet* pEnemyShotSet, int t, const sNcrPose& pose, double cx)
    {
        for (int n = 0; n < g_ncrStrokeN; n++) {
            const sNcrStroke& s = g_ncrStroke[n];
            if (s.type == NCR_LIFT && t == s.start) NcrPlaySound(sound_enemyCharge);   // 持ち上げの予告音
            if (s.type == NCR_DOWN) {
                int local = t - s.start;
                if (local == 0 || local == 6 || local == 12) NcrVolley(pEnemyShotSet, s, pose, cx, local / 6);
                if (t == s.start + s.len) NcrCollision(pEnemyShotSet, s, cx);
            }
        }
        NcrWaveEvents(pEnemyShotSet, t, pose, cx);
        if (t == g_ncrTCharge) NcrPlaySound(sound_enemyCharge);   // 糸切れの予告音
    }

    // ============================================================
    //  弾の更新
    // ============================================================
    int NcrBlink(int t)
    {
        return ((t / 4) % 2) ? 0 : 6;   // 赤/白の点滅
    }

    int NcrStringColor(const sNcrPose& pose, int i, int t)
    {
        if (pose.mode == 1) return NCR_WAVE_COL[i];
        if (pose.mode == 2) return NcrBlink(t);
        return pose.moving[i] ? 1 : 6;
    }

    int NcrBallColor(const sNcrPose& pose, int i, int t)
    {
        if (pose.mode == 1) return NCR_WAVE_COL[i];
        if (pose.mode == 2) return NcrBlink(t);
        if (pose.flash[i]) return 3;
        return pose.moving[i] ? 8 : 6;
    }

    // 支持バー・糸・錘の位置と向きと色を、姿勢から直接決める
    void NcrUpdateStatic(sEnemyShot* p, int t, const sNcrPose& pose, const sNcrPose& prev, double cx)
    {
        int role = p->param_i[0];
        int idx = p->param_i[1];
        if (role == NCR_ROLE_BAR) {
            double halfW = 2.0 * pose.spacing + NCR_BALL_R;
            p->x = cx + ((double)idx / (double)(NCR_BAR_N - 1) - 0.5) * 2.0 * halfW;
            p->y = NCR_BAR_Y;
            p->muki = 0.0;   // 横向き
            p->kind = img_enemyShotDiamond[(pose.mode == 2) ? NcrBlink(t) : 6];
        }
        else if (role == NCR_ROLE_STRING) {
            double th = pose.theta[idx];
            double r = NCR_STR_R0 + (double)p->param_i[2] * NCR_STR_PITCH;
            p->x = cx + (double)(idx - 2) * pose.spacing + r * sin(th);
            p->y = NCR_BAR_Y + r * cos(th);
            p->muki = DX_PI * 0.5 - th;   // 糸の向き(支点→錘)
            p->kind = img_enemyShotBullet[NcrStringColor(pose, idx, t)];
        }
        else if (role == NCR_ROLE_BALL) {
            double th = pose.theta[idx];
            p->x = cx + (double)(idx - 2) * pose.spacing + NCR_LEN * sin(th);
            p->y = NCR_BAR_Y + NCR_LEN * cos(th);
            double dth = th - prev.theta[idx];
            if (fabs(dth) > 1e-6) {
                double sg = (dth > 0.0) ? 1.0 : -1.0;
                p->muki = atan2(-sin(th) * sg, cos(th) * sg);   // 振れている向き(円弧の接線方向)
            }
            else {
                p->muki = DX_PI * 0.5;
            }
            p->kind = img_enemyShotLargeBall[NcrBallColor(pose, idx, t)];
        }
    }

    void NcrMoveFree(sEnemyShot* p)
    {
        double acc = p->param_d[0];
        if (acc != 0.0) {
            p->speed += acc;
            if (p->speed > p->param_d[1]) p->speed = p->param_d[1];
        }
        double g = p->param_d[2];
        if (g != 0.0) {
            double vx = p->speed * cos(p->muki);
            double vy = p->speed * sin(p->muki) + g;
            p->speed = sqrt(vx * vx + vy * vy);
            p->muki = atan2(vy, vx);   // 重力で曲がった向きに muki も追従させる
        }
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
    }

    // 糸切れ：バー・糸・錘を通常の弾に変える
    void NcrCut(sEnemyShotSet* pEnemyShotSet, sEnemyShot* p)
    {
        int role = p->param_i[0];
        if (role == NCR_ROLE_BAR) {
            // バーは崩れ落ちる
            p->param_i[0] = NCR_ROLE_FREE;
            p->kind = img_enemyShotDiamond[6];
            p->muki = DX_PI * 0.5 + (GetRand(60) - 30) / 180.0 * DX_PI;
            p->speed = 0.3 + GetRand(100) / 100.0;
            p->param_d[0] = 0.0;
            p->param_d[1] = 0.0;
            p->param_d[2] = 0.05;
        }
        else if (role == NCR_ROLE_STRING) {
            // 糸は向きと直交する方向へ弾け、火花(小玉)を2つ飛ばす
            double sgn = (GetRand(1) == 0) ? 1.0 : -1.0;
            p->param_i[0] = NCR_ROLE_FREE;
            p->kind = img_enemyShotBullet[8];
            p->muki = p->muki + sgn * DX_PI * 0.5 + (GetRand(40) - 20) / 180.0 * DX_PI;
            p->speed = 1.0 + GetRand(200) / 100.0;
            p->param_d[0] = 0.0;
            p->param_d[1] = 0.0;
            p->param_d[2] = 0.03;
            for (int e = 0; e < 2; e++) {
                double a = GetRand(359) * DX_PI / 180.0;
                double sp = 0.8 + GetRand(150) / 100.0;
                NcrAddFree(pEnemyShotSet, img_enemyShotSmallBall[e == 0 ? 6 : 1], p->x, p->y, a, sp, 0.0, 0.0, 0.0);
            }
        }
        else if (role == NCR_ROLE_BALL) {
            // 錘は少しずつ角度を変えて、ゆっくり動き出してから自機へ加速
            double aim = atan2(player.y - p->y, player.x - p->x) + (double)(p->param_i[1] - 2) * 0.22;
            p->param_i[0] = NCR_ROLE_FREE;
            p->kind = img_enemyShotLargeBall[0];
            p->muki = aim;
            p->speed = 0.8;
            p->param_d[0] = 0.05;
            p->param_d[1] = 3.8;
            p->param_d[2] = 0.0;
        }
    }

    // ============================================================
    //  弾幕本体(ShotSet 1つで、振り子と飛んでいく弾のすべてを管理)
    // ============================================================
    void ShotCradle(sEnemyShotSet* pEnemyShotSet)
    {
        NcrBuild();
        const int t = pEnemyShotSet->count;
        const double cx = NCR_CX + NcrSway(t);
        const bool alive = (t < g_ncrTCut);   // 糸切れ前か
        sNcrPose pose = {};
        sNcrPose prev = {};

        if (t == 0) NcrCreateBullets(pEnemyShotSet);

        if (alive) {
            NcrCalcPose(t, &pose);
            NcrCalcPose((t > 0) ? (t - 1) : 0, &prev);
            NcrEvents(pEnemyShotSet, t, pose, cx);
        }
        else if (t == g_ncrTCut) {
            NcrPlaySound(sound_enemyShot_extreme);
        }

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (t == g_ncrTCut && pShot->param_i[0] != NCR_ROLE_FREE) NcrCut(pEnemyShotSet, pShot);
            if (pShot->param_i[0] == NCR_ROLE_FREE) NcrMoveFree(pShot);
            else NcrUpdateStatic(pShot, t, pose, prev, cx);
            pShot = pShot->next;
        }
    }

}  // namespace

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_NewtonsCradle_Claude()
{
    NcrBuild();
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = NCR_CX;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 300;   // 200から変更しないこと
    }

    // ラウンド内の時刻(糸切れまで終わったら最初から繰り返す)
    int rt = (count - 1) % g_ncrTEnd;
    if (rt == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCradle;
        pEnemyShotSet->x = NCR_CX;
        pEnemyShotSet->y = NCR_BAR_Y;
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

    // 敵本体は支持バーの真上で、バーと同じ揺れ方をする
    enemy.x = NCR_CX + NcrSway(rt);
}