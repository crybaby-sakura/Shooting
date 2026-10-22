// enemyPat_RekiseiTekika.cpp
// 瀝青滴下（ピッチドロップ実験）モチーフの無限ループ4フェーズパターン
//
// フェーズ構成:
//   1) 漏斗充填    ：既存の小玉弾でロート/ビーカーの輪郭を組み上げる（橙）
//   2) 滴下前兆    ：小玉を鎖状に積み上げてネックを形成。二乗カーブでほぼ静止に見えたまま
//                    終盤だけ急に伸びる（実験の「見た目には動いていないのに実は成長している」
//                    性質を密度変化だけで表現）。待機中は左右交互に自機狙い3wayの火花が飛ぶ。
//   3) くびれ切断  ：白い菱形弾のリング警告フラッシュとともにネックが上へ急収縮、
//                    同時に滴（塊）が収束を始める
//   4) 着滴・飛沫拡散：塊が二乗カーブで加速落下→着弾で放射状バースト＋自機狙い5wayを発射、
//                    以後フェーズ1へループ
//
// 専用素材は使わず、既存の弾種（小玉・中玉・銃弾・菱形弾・中楕円弾）の組み合わせのみで構成。

namespace {

    // ---- 補間・イージング補助 ----
    inline double Lerp(double a, double b, double t) { return a + (b - a) * t; }
    inline double Clamp01(double t) { return t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t); }
    inline double EaseOutQuad(double t) { t = Clamp01(t); return 1.0 - (1.0 - t) * (1.0 - t); }
    inline double EaseInQuad(double t) { t = Clamp01(t); return t * t; }

    // ---- 装置の寸法定数（全フェーズで共有）----
    constexpr double APEX_X = 240.0;      // ロート出口＝ネック上端
    constexpr double APEX_Y = 190.0;
    constexpr double FUNNEL_TOP_Y = 70.0;
    constexpr double FUNNEL_LEFT_X = 150.0;
    constexpr double FUNNEL_RIGHT_X = 330.0;
    constexpr int    MAX_SEGMENTS = 56;   // ネックの最大節数
    constexpr double SEGMENT_SPACING = 3.5;
    constexpr double BREAK_Y = APEX_Y + MAX_SEGMENTS * SEGMENT_SPACING; // くびれ切断点
    constexpr double SPLASH_Y = 450.0;    // 受け皿の高さ

    // ============================================================
    // 共通：シューティングセット生成ヘルパー
    // ============================================================
    sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki = 0.0)
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = func;
        pSet->x = x;
        pSet->y = y;
        pSet->muki = muki;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
        return pSet;
    }

    sEnemyShot* AppendShot(sEnemyShotSet* pSet)
    {
        sEnemyShot* s = new sEnemyShot;
        s->margin = 480;
        s->prev = pSet->pEnemyShotHead->prev;
        s->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = s;
        pSet->pEnemyShotHead->prev = s;
        return s;
    }

    // ============================================================
    // フェーズ1：漏斗充填（ロート＋ビーカー縁の弾幕壁）
    // ============================================================
    void ShotFunnelWall(sEnemyShotSet* pSet)
    {
        const int N_WALL = 14;
        const int N_RIM = 12;

        if (pSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            // 左壁
            for (int i = 0; i < N_WALL; i++) {
                double t = i / (double)(N_WALL - 1);
                double tx = Lerp(FUNNEL_LEFT_X, APEX_X, t);
                double ty = Lerp(FUNNEL_TOP_Y, APEX_Y, t);
                sEnemyShot* s = AppendShot(pSet);
                s->kind = img_enemyShotSmallBall[8]; // 橙：瀝青の色
                s->param_d[0] = tx;
                s->param_d[1] = ty;
                s->param_d[2] = ty - 220.0; // 落下開始位置
                s->param_i[1] = i * 5;      // 出現ずらし
            }
            // 右壁
            for (int i = 0; i < N_WALL; i++) {
                double t = i / (double)(N_WALL - 1);
                double tx = Lerp(FUNNEL_RIGHT_X, APEX_X, t);
                double ty = Lerp(FUNNEL_TOP_Y, APEX_Y, t);
                sEnemyShot* s = AppendShot(pSet);
                s->kind = img_enemyShotSmallBall[8];
                s->param_d[0] = tx;
                s->param_d[1] = ty;
                s->param_d[2] = ty - 220.0;
                s->param_i[1] = i * 5;
            }
            // 上縁
            for (int j = 0; j < N_RIM; j++) {
                double t = j / (double)(N_RIM - 1);
                double tx = Lerp(FUNNEL_LEFT_X, FUNNEL_RIGHT_X, t);
                double ty = FUNNEL_TOP_Y;
                sEnemyShot* s = AppendShot(pSet);
                s->kind = img_enemyShotSmallBall[8];
                s->param_d[0] = tx;
                s->param_d[1] = ty;
                s->param_d[2] = ty - 220.0;
                s->param_i[1] = 70 + j * 4;
            }
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            double tx = pShot->param_d[0];
            double ty = pShot->param_d[1];
            double sy = pShot->param_d[2];
            int    delay = pShot->param_i[1];

            if (pSet->count < 900) {
                // 建て込みフェーズ：上から降ってきて所定位置に収まる
                double localCount = pSet->count - delay;
                double t = EaseOutQuad(localCount / 100.0);
                pShot->x = tx;
                pShot->y = Lerp(sy, ty, t);
            }
            else {
                // 撤収フェーズ：サイクル終端で上へ加速退避
                double rt = pSet->count - 900;
                pShot->x = tx;
                pShot->y = ty - rt * rt * 0.5;
            }
            pShot = pShot->next;
        }
    }

    // ============================================================
    // フェーズ2：滴下前兆（鎖状に伸びるネック）
    // ============================================================
    void ShotNeckChain(sEnemyShotSet* pSet)
    {
        const double GROW = 360.0;

        if (pSet->count == 0) {
            pSet->param_i[0] = 0; // 生成済み節数
        }

        if (pSet->count < 360) {
            double t = pSet->count / GROW;
            int target = (int)(MAX_SEGMENTS * EaseInQuad(t));
            if (target > MAX_SEGMENTS) target = MAX_SEGMENTS;
            while (pSet->param_i[0] < target) {
                int idx = pSet->param_i[0];
                sEnemyShot* s = AppendShot(pSet);
                s->kind = img_enemyShotSmallBall[8];
                s->param_i[0] = idx;
                pSet->param_i[0]++;
            }
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            int idx = pShot->param_i[0];
            double sway = 2.5 * sin(idx * 0.4 + count * 0.03); // 粘性を帯びたわずかな揺れ

            if (pSet->count < 360) {
                // 節ごとに固定位置で吊り下がる（末端だけが新しく増えていく）
                pShot->x = APEX_X + sway;
                pShot->y = APEX_Y + (idx + 1) * SEGMENT_SPACING;
            }
            else {
                // くびれ切断：ネック全体が加速的に上へ縮んで消える
                double rt = pSet->count - 360;
                pShot->x = APEX_X + sway;
                pShot->y = APEX_Y + (idx + 1) * SEGMENT_SPACING - rt * rt * 0.6;
            }
            pShot = pShot->next;
        }
    }

    // ============================================================
    // 待機中の警告弾（ネック脇から自機狙い3way、左右交互）
    // ============================================================
    void ShotWarningPulse(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            for (int i = -1; i <= 1; i++) {
                double muki = pSet->muki + i * 12.0 * DX_PI / 180.0;
                double speed = 1.8 + 0.25 * (i < 0 ? -i : i);
                sEnemyShot* s = AppendShot(pSet);
                s->kind = img_enemyShotBullet[1]; // 黄：火花
                s->muki = muki;
                s->param_d[0] = pSet->x;
                s->param_d[1] = pSet->y;
                s->param_d[2] = muki;
                s->param_d[3] = speed;
            }
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            double ox = pShot->param_d[0];
            double oy = pShot->param_d[1];
            double muki = pShot->param_d[2];
            double speed = pShot->param_d[3];
            pShot->x = ox + speed * pShot->count * cos(muki);
            pShot->y = oy + speed * pShot->count * sin(muki);
            pShot = pShot->next;
        }
    }

    // ============================================================
    // フェーズ3：くびれ切断の警告リング（白フラッシュ）
    // ============================================================
    void ShotRingWarning(sEnemyShotSet* pSet)
    {
        const int N_RING = 24;

        if (pSet->count == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            for (int i = 0; i < N_RING; i++) {
                double angle = i / (double)N_RING * 2.0 * DX_PI;
                sEnemyShot* s = AppendShot(pSet);
                s->kind = img_enemyShotDiamond[6]; // 白：亀裂の閃光
                s->muki = angle;
                s->param_d[0] = angle;
            }
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            double angle = pShot->param_d[0];
            double r = 0.02 * pShot->count * pShot->count; // 加速する衝撃波
            pShot->x = pSet->x + r * cos(angle);
            pShot->y = pSet->y + r * sin(angle);
            pShot = pShot->next;
        }
    }

    // ============================================================
    // フェーズ3→4：滴（塊）の収束と加速落下
    // ============================================================
    void ShotDroplet(sEnemyShotSet* pSet)
    {
        const int N_BLOB = 7;

        if (pSet->count == 0) {
            for (int i = 0; i < N_BLOB; i++) {
                double angle = (i == 0) ? 0.0 : (i - 1) / 6.0 * 2.0 * DX_PI;
                double baseR = (i == 0) ? 0.0 : 1.0;
                sEnemyShot* s = AppendShot(pSet);
                s->kind = img_enemyShotMediumBall[8]; // 橙：粘性の塊
                s->param_d[0] = angle;
                s->param_d[1] = baseR;
            }
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            double angle = pShot->param_d[0];
            double baseR = pShot->param_d[1];
            double cx, cy, radius;

            if (pSet->count < 30) {
                // 収束：ばらけた粒が塊へまとまる
                double t = EaseOutQuad(pSet->count / 30.0);
                radius = Lerp(20.0, 6.0, t) * baseR;
                cx = APEX_X; cy = BREAK_Y;
            }
            else if (pSet->count < 270) {
                // 落下：二乗カーブで加速
                double t = EaseInQuad((pSet->count - 30) / 240.0);
                radius = 6.0 * baseR;
                cx = APEX_X; cy = Lerp(BREAK_Y, SPLASH_Y, t);
            }
            else {
                // 着弾後は破裂弾に役目を譲って高速に消える
                double rt = pSet->count - 270;
                radius = 6.0 * baseR;
                cx = APEX_X; cy = SPLASH_Y + rt * rt * 0.5;
            }
            pShot->x = cx + radius * cos(angle);
            pShot->y = cy + radius * sin(angle);
            pShot = pShot->next;
        }
    }

    // ============================================================
    // フェーズ4：着滴・飛沫拡散
    // ============================================================
    void ShotSplashBurst(sEnemyShotSet* pSet)
    {
        const int N_RADIAL = 28;
        const int N_AIM = 5;

        if (pSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            for (int i = 0; i < N_RADIAL; i++) {
                double angle = i / (double)N_RADIAL * 2.0 * DX_PI;
                double mult = (90 + GetRand(20)) / 100.0;
                sEnemyShot* s = AppendShot(pSet);
                s->kind = (i % 2 == 0) ? img_enemyShotSmallBall[8] : img_enemyShotSmallBall[1];
                s->muki = angle;
                s->param_i[0] = 0; // 放射弾
                s->param_d[0] = angle;
                s->param_d[1] = mult;
            }

            double baseMuki = atan2(player.y - pSet->y, player.x - pSet->x);
            for (int i = 0; i < N_AIM; i++) {
                double muki = baseMuki + (i - 2) * 10.0 * DX_PI / 180.0;
                sEnemyShot* s = AppendShot(pSet);
                s->kind = img_enemyShotMediumOval[8];
                s->muki = muki;
                s->param_i[0] = 1; // 自機狙い弾
                s->param_d[0] = muki;
                s->param_d[1] = 2.2;
            }
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            double angle = pShot->param_d[0];
            double p1 = pShot->param_d[1];
            if (pShot->param_i[0] == 0) {
                double r = (1.0 * pShot->count + 0.02 * pShot->count * pShot->count) * p1;
                pShot->x = pSet->x + r * cos(angle);
                pShot->y = pSet->y + r * sin(angle);
            }
            else {
                pShot->x = pSet->x + p1 * pShot->count * cos(angle);
                pShot->y = pSet->y + p1 * pShot->count * sin(angle);
            }
            pShot = pShot->next;
        }
    }

} // namespace

// ============================================================
// 敵本体：瀝青滴下（ピッチドロップ実験）モチーフ、960フレーム周期の無限ループ
// ============================================================
void EnemyPat_PitchDrop_Claude()
{
    const int CYCLE = 960;
    int local = (count - 1) % CYCLE;

    if (count == 1) {
        // 装置は動かない＝気の長い実験そのものを体現する不動のボス
        enemy.x = APEX_X;
        enemy.y = 130.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // フェーズ1：漏斗充填
    if (local == 0) {
        CreateShotSet(ShotFunnelWall, APEX_X, FUNNEL_TOP_Y);
    }

    // フェーズ2：滴下前兆（ネック生成開始）
    if (local == 180) {
        CreateShotSet(ShotNeckChain, APEX_X, APEX_Y);
    }

    // 待機中の警告パルス：210〜510の間、60フレームごとに左右交互で6回
    if (local >= 210 && local < 540 && (local - 210) % 60 == 0) {
        bool leftSide = (((local - 210) / 60) % 2 == 0);
        double ox = leftSide ? 170.0 : 310.0;
        double oy = 150.0;
        CreateShotSet(ShotWarningPulse, ox, oy, atan2(player.y - oy, player.x - ox));
    }

    // フェーズ3：くびれ切断（警告リング＋滴の収束開始）
    if (local == 540) {
        CreateShotSet(ShotRingWarning, APEX_X, BREAK_Y);
        CreateShotSet(ShotDroplet, APEX_X, BREAK_Y);
    }

    // フェーズ4：着滴・飛沫拡散
    if (local == 810) {
        CreateShotSet(ShotSplashBurst, APEX_X, SPLASH_Y);
    }

    // local 900〜959：静寂の余韻（漏斗が撤収し、次サイクルの漏斗充填へ繋がる）
}
