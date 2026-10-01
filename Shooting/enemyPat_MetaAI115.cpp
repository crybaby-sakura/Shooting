// EnemyPat_Inbachi_MetaAI.cpp - 陰蜂 檻収縮+反射パターン

// ---------- 檻の共有状態 ----------
static double g_cageCX = 240.0;
static double g_cageCY = 240.0;
static double g_cageR = 220.0;
static int g_cageState = 0; // 0:縮小中 1:タメ 2:収束
static int g_cageStateCount = 0;

// 六角形頂点を求める
static void GetHexVertex(int idx, double cx, double cy, double r, double* outX, double* outY) {
    double ang = DX_PI / 180.0 * (60.0 * idx - 90.0); // -90で上向き頂点
    *outX = cx + r * cos(ang);
    *outY = cy + r * sin(ang);
}

// -------------------------------------------------
// 檻の壁本体。速度0で毎フレーム座標を直接書き換える
// -------------------------------------------------
static void ShotCageWall(sEnemyShotSet* pSet) {
    static int g_openEdge = 0;

    const int BULLETS_PER_EDGE = 14; // 14*6=84発で壁を作る
    if (pSet->count == 0) {
        g_cageCX = pSet->param_d[0];
        g_cageCY = pSet->param_d[1];
        g_cageR = 220.0;
        g_cageState = 0;
        g_cageStateCount = 0;
        pSet->param_d[2] = g_cageR;

        for (int edge = 0; edge < 6; edge++) {
            double x0, y0, x1, y1;
            GetHexVertex(edge, g_cageCX, g_cageCY, g_cageR, &x0, &y0);
            GetHexVertex((edge + 1) % 6, g_cageCX, g_cageCY, g_cageR, &x1, &y1);
            for (int j = 0; j < BULLETS_PER_EDGE; j++) {
                double t = (double)j / (double)BULLETS_PER_EDGE;
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = x0 * (1.0 - t) + x1 * t;
                pShot->y = y0 * (1.0 - t) + y1 * t;
                pShot->muki = 0;
                pShot->speed = 0;
                pShot->kind = img_enemyShotMediumOval[7]; // 黒・中楕円
                pShot->margin = 200.0; // 画面外削除させない
                pShot->param_d[0] = (double)edge;
                pShot->param_d[1] = t;
                pShot->param_i[0] = 0; // 0:壁として配置 1:収束移動開始済み
                pShot->param_i[1] = 0;

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // 状態更新
        if (g_cageState == 0) {
            g_cageR -= 0.65; // 縮小速度
            if (g_cageR < 38.0) {
                g_cageR = 38.0;
                g_cageState = 1;
                g_openEdge = GetRand(5); // 0〜5のいずれかの辺を抜け道にする
                g_cageStateCount = pSet->count;
            }
        }
        else if (g_cageState == 1) {
            if (pSet->count - g_cageStateCount > 35) { // タメ0.5秒
                g_cageState = 2;
            }
        }
        pSet->param_d[2] = g_cageR;
        g_cageCX = pSet->param_d[0];
        g_cageCY = pSet->param_d[1];

        // 壁弾の位置更新
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;
            if (g_cageState < 2) {
                int edge = (int)pShot->param_d[0];
                double t = pShot->param_d[1];
                double x0, y0, x1, y1;
                GetHexVertex(edge, g_cageCX, g_cageCY, g_cageR, &x0, &y0);
                GetHexVertex((edge + 1) % 6, g_cageCX, g_cageCY, g_cageR, &x1, &y1);
                pShot->x = x0 * (1.0 - t) + x1 * t;
                pShot->y = y0 * (1.0 - t) + y1 * t;
            }
            else {
                int edge = (int)pShot->param_d[0];
				if (edge == g_openEdge) { // 抜け道の辺は収束させず消す
					pShot->prev->next = pShot->next;
					pShot->next->prev = pShot->prev;
					delete pShot;
                    pShot = pNext;
					continue;
				}

                // 全弾収束：中心へ加速
                if (pShot->param_i[0] == 0) {
                    pShot->muki = atan2(g_cageCY - pShot->y, g_cageCX - pShot->x);
                    pShot->speed = 0.2;
                    pShot->param_i[0] = 1;
                }
                pShot->speed += 0.12;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);

                // 中心に到達したら消す(メインに任せず自前で消去)
                double dx = pShot->x - g_cageCX;
                double dy = pShot->y - g_cageCY;
                if (dx * dx + dy * dy < 25.0) {
                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                }
            }
            pShot = pNext;
        }
    }
}

// 頂点から中心へ向かう収縮針
static void ShotConvergeNeedle(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 6; i++) {
            double vx, vy;
            GetHexVertex(i, g_cageCX, g_cageCY, g_cageR, &vx, &vy);
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = vx;
            pShot->y = vy;
            // 狙いは自機ではなく中心へ少しランダムを加える
            pShot->muki = atan2(g_cageCY - vy, g_cageCX - vx) + (GetRand(40) - 20) / 180.0 * DX_PI;
            pShot->speed = 2.2 + GetRand(100) / 100.0;
            pShot->kind = img_enemyShotSmallBall[1]; // 黄小玉
            pShot->margin = 20.0;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 過去の自機位置から湧いて外へ逃げ、壁で反射して金になる弾
static void ShotReflectNeedle(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        // param_d[0], [1] に60F前の自機位置が入っている
        double sx = pSet->param_d[0];
        double sy = pSet->param_d[1];
        for (int k = 0; k < 3; k++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = sx + GetRand(20) - 10;
            pShot->y = sy + GetRand(20) - 10;
            // 中心から外へ逃げる方向
            double angToCenter = atan2(g_cageCY - pShot->y, g_cageCX - pShot->x);
            pShot->muki = angToCenter + DX_PI + (GetRand(60) - 30) / 180.0 * DX_PI;
            pShot->speed = 0.9 + GetRand(40) / 100.0;
            pShot->kind = img_enemyShotScale[7]; // 黒鱗弾
            pShot->margin = 20.0;
            pShot->param_i[0] = 0; // 未反射
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        if (pShot->param_i[0] == 0) {
            double dx = pShot->x - g_cageCX;
            double dy = pShot->y - g_cageCY;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist >= g_cageR - 6.0 && g_cageState == 0) {
                // 円近似で反射
                double nx = dx / dist;
                double ny = dy / dist;
                double vx = pShot->speed * cos(pShot->muki);
                double vy = pShot->speed * sin(pShot->muki);
                double dot = vx * nx + vy * ny;
                double rvx = vx - 2.0 * dot * nx;
                double rvy = vy - 2.0 * dot * ny;
                pShot->muki = atan2(rvy, rvx);
                pShot->speed *= 1.9;
                pShot->param_i[0] = 1;
                pShot->kind = img_enemyShotScale[1]; // 黄に変色
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            }
        }
        pShot = pShot->next;
    }
}

// -------------------------------------------------
// 敵本体
// -------------------------------------------------
void EnemyPat_Inbachi_MetaAI()
{
    static int muki;
    static double histX[60];
    static double histY[60];
    static int histIdx;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        histIdx = 0;
        for (int i = 0; i < 60; i++) {
            histX[i] = player.x;
            histY[i] = player.y;
        }
        g_cageCX = 240.0;
        g_cageCY = 240.0;
        g_cageR = 220.0;
        g_cageState = 0;
    }
    else {
        // 本体はゆっくり横移動
        enemy.x += 0.35 * (double)muki;
        if (count % 240 == 120) muki *= -1;

        // 自機位置履歴更新 (60Fリングバッファ)
        histX[histIdx] = player.x;
        histY[histIdx] = player.y;
        histIdx = (histIdx + 1) % 60;
    }

    const int T = 360;
    int countT = count % T;

    // 60F目：檻生成（自機位置を中心に、画面端に寄りすぎないようクランプ）
    if (countT == 60-30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotCageWall;
        double cx = player.x;
        double cy = player.y;
        if (cx < 110.0) cx = 110.0; if (cx > 370.0) cx = 370.0;
        if (cy < 110.0) cy = 110.0; if (cy > 370.0) cy = 370.0;
        pSet->x = cx;
        pSet->y = cy;
        pSet->param_d[0] = cx;
        pSet->param_d[1] = cy;
        pSet->param_d[2] = 320.0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 80F～350F：収縮針を8F間隔で
    if (countT > 80-30 && countT < 350-30 && g_cageState == 0 && countT % 8 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotConvergeNeedle;
        pSet->x = g_cageCX;
        pSet->y = g_cageCY;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 90F～350F：反射弾を14F間隔で（60F前の位置から）
    if (countT > 90-30 && countT < 350-30 && g_cageState == 0 && countT % 14 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotReflectNeedle;
        int pastIdx = histIdx; // 最も古い = 60F前
        pSet->param_d[0] = histX[pastIdx];
        pSet->param_d[1] = histY[pastIdx];
        pSet->x = histX[pastIdx];
        pSet->y = histY[pastIdx];
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}