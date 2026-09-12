// enemyPat_Tmp_maze.cpp
// 速度0の敵弾で作る格子状迷路ステージ
// 自機:左上固定 / 敵(ボス):右下固定 / 迷路を抜くまでボスに攻撃できない


// ============================================================
//  迷路の定義
// ============================================================
static const int MZ_W = 8;    // 迷路の横セル数 (480 / 60)
static const int MZ_H = 8;    // 迷路の縦セル数
static const int CELL = 60;   // 1セルのピクセル数

// 壁データ true=壁あり
static bool wallH[MZ_W][MZ_H + 1];        // セル(x,y)の上側の壁 (y=0,8は外周)
static bool wallV[MZ_W + 1][MZ_H];        // セル(x,y)の左側の壁 (x=0,8は外周)
static bool mazeDone = false;           // 壁弾の配置が完了したか
static int  mazeDoneAt = 0;               // 迷路が完成したフレーム

// 方向: 0:上 1:右 2:下 3:左
static const int dirX[4] = { 0, 1, 0, -1 };
static const int dirY[4] = { -1, 0, 1, 0 };

static double CellCX(int cx) { return cx * CELL + CELL / 2.0; }
static double CellCY(int cy) { return cy * CELL + CELL / 2.0; }

static int ClampCell(int c, int max)
{
    if (c < 0) return 0;
    if (c > max) return max;
    return c;
}
static int PlayerCellX() { return ClampCell((int)(player.x / CELL), MZ_W - 1); }
static int PlayerCellY() { return ClampCell((int)(player.y / CELL), MZ_H - 1); }

// セル(cx,cy)から方向dへ移動できるか(壁の有無で判定)
static bool CanMove(int cx, int cy, int d)
{
    int nx = cx + dirX[d], ny = cy + dirY[d];
    if (nx < 0 || nx >= MZ_W || ny < 0 || ny >= MZ_H) return false;
    switch (d) {
    case 0:  return !wallH[cx][cy];
    case 1:  return !wallV[cx + 1][cy];
    case 2:  return !wallH[cx][cy + 1];
    default: return !wallV[cx][cy];
    }
}

static void RemoveWall(int cx, int cy, int d)
{
    switch (d) {
    case 0:  wallH[cx][cy] = false;     break;
    case 1:  wallV[cx + 1][cy] = false; break;
    case 2:  wallH[cx][cy + 1] = false; break;
    default: wallV[cx][cy] = false;     break;
    }
}

// 穴掘り法で迷路を生成(スタートからゴールへの道は必ず通る)
static void GenerateMaze()
{
    bool visited[MZ_W][MZ_H] = {};
    int stkX[MZ_W * MZ_H], stkY[MZ_W * MZ_H], sp = 0;

    for (int x = 0; x < MZ_W; x++)
        for (int y = 0; y < MZ_H; y++) {
            visited[x][y] = false;
            wallH[x][y] = wallH[x][y + 1] = true;
            wallV[x][y] = wallV[x + 1][y] = true;
        }

    visited[0][0] = true;
    stkX[sp] = 0; stkY[sp] = 0; sp++;

    while (sp > 0) {
        int cx = stkX[sp - 1], cy = stkY[sp - 1];
        int cand[4], n = 0;
        for (int d = 0; d < 4; d++) {
            int nx = cx + dirX[d], ny = cy + dirY[d];
            if (nx < 0 || nx >= MZ_W || ny < 0 || ny >= MZ_H) continue;
            if (visited[nx][ny]) continue;
            cand[n++] = d;
        }
        if (n == 0) { sp--; continue; }          // 行き止まり→一つ戻る
        int d = cand[GetRand(n - 1)];            // GetRand(x)は0～xのx+1種なので-1する
        RemoveWall(cx, cy, d);
        int nx = cx + dirX[d], ny = cy + dirY[d];
        visited[nx][ny] = true;
        stkX[sp] = nx; stkY[sp] = ny; sp++;
    }

    // 完全な一本道にならないよう、内壁を数か所追加で開けて複数ルートを作る
    for (int i = 0; i < 6; i++) {
        int x = GetRand(MZ_W - 1), y = GetRand(MZ_H - 1), d = GetRand(3);
        int nx = x + dirX[d], ny = y + dirY[d];
        if (nx < 0 || nx >= MZ_W || ny < 0 || ny >= MZ_H) { i--; continue; }
        RemoveWall(x, y, d);
    }
}

// ============================================================
//  弾・弾セットの生成ヘルパ
// ============================================================
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;  p->y = y;
    p->muki = 0.0;  p->speed = speed;  p->kind = kind;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

static sEnemyShotSet* AddShotSet(sEnemyShotSet::PatternFunc func, int kind)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = func;
    p->x = enemy.x;  p->y = enemy.y;  p->kind = kind;

    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;

    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
    return p;
}

// ============================================================
//  迷路壁の展開 (speed = 0 の弾を壁に見立てる)
// ============================================================
static void ShotMaze(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        GenerateMaze();
        // 展開の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 全壁セグメント数(縦壁と横壁のスロット総数)
    const int totalV = (MZ_W + 1) * MZ_H;      // wallV のスロット数
    const int total = totalV + MZ_W * (MZ_H + 1);

    // 1フレームに2セグメントずつ、左上から順に壁を配置していく
    int placed = 0;
    while (pSet->param_i[0] < total && placed < 2) {
        int idx = pSet->param_i[0]++;

        if (idx < totalV) {                          // 縦壁 wallV[x][y]
            int x = idx / MZ_H, y = idx % MZ_H;
            if (!wallV[x][y]) continue;
            for (int k = 0; k < 6; k++)
                AddShot(pSet, x * CELL, y * CELL + 5.0 + k * 10.0, 0.0, img_enemyShotMediumBall[4]);
        }
        else {                                       // 横壁 wallH[x][y]
            int idx2 = idx - totalV;
            int x = idx2 / (MZ_H + 1), y = idx2 % (MZ_H + 1);
            if (!wallH[x][y]) continue;
            for (int k = 0; k < 6; k++)
                AddShot(pSet, x * CELL + 5.0 + k * 10.0, y * CELL, 0.0, img_enemyShotMediumBall[4]);
        }
        placed++;
    }

    // 全セグメント走査完了 → 迷路完成
    if (pSet->param_i[0] >= total && !mazeDone) {
        mazeDone = true;
        mazeDoneAt = count;
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // speed = 0 のため移動処理は不要。壁弾は画面外に出ないので消去もされない
}

// ============================================================
//  通路を辿って自機を追う追尾弾
// ============================================================
// 次に進む方向を決める(自機のいるセルに近い方向を優先、たまに揺らぐ)
static int ChooseDir(int cx, int cy, int lastDir)
{
    int back = (lastDir + 2) % 4;
    int cand[4], n = 0;
    for (int d = 0; d < 4; d++)
        if (d != back && CanMove(cx, cy, d)) cand[n++] = d;
    if (n == 0) cand[n++] = back;                    // 行き止まりは引き返す

    int best = cand[0];
    double bestD = 1e9;
    for (int i = 0; i < n; i++) {
        int d = cand[i];
        double dx = CellCX(cx + dirX[d]) - player.x;
        double dy = CellCY(cy + dirY[d]) - player.y;
        double dist = dx * dx + dy * dy + GetRand(400);  // ランダム要素で動きに揺らぎ
        if (dist < bestD) { bestD = dist; best = d; }
    }
    return best;
}

static void MoveSeeker(sEnemyShot* pShot)
{
    double dx = pShot->param_d[0] - pShot->x;
    double dy = pShot->param_d[1] - pShot->y;
    double dist = sqrt(dx * dx + dy * dy);

    if (dist <= pShot->speed) {
        // 次のセルの中心に到達 → セルを更新して方向転換
        pShot->x = pShot->param_d[0];
        pShot->y = pShot->param_d[1];
        int cx = pShot->param_i[0] + dirX[pShot->param_i[2]];
        int cy = pShot->param_i[1] + dirY[pShot->param_i[2]];
        pShot->param_i[0] = cx;
        pShot->param_i[1] = cy;
        int d = ChooseDir(cx, cy, pShot->param_i[2]);
        pShot->param_i[2] = d;
        pShot->param_d[0] = CellCX(cx + dirX[d]);
        pShot->param_d[1] = CellCY(cy + dirY[d]);
    }
    else {
        pShot->x += pShot->speed * dx / dist;
        pShot->y += pShot->speed * dy / dist;
    }
}

// 3体1組の追尾弾セット
static void ShotSeekers(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = AddShot(pSet, enemy.x, enemy.y, 2.0 + i * 0.3, img_enemyShotLargeBall[0]);
            pShot->param_i[0] = MZ_W - 1;            // 現在セル(右下=敵の位置)
            pShot->param_i[1] = MZ_H - 1;
            int d = ChooseDir(MZ_W - 1, MZ_H - 1, -1);
            pShot->param_i[2] = d;                   // 進行方向
            pShot->param_d[0] = CellCX(MZ_W - 1 + dirX[d]);  // 目標座標(次のセル中心)
            pShot->param_d[1] = CellCY(MZ_H - 1 + dirY[d]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        MoveSeeker(pShot);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン(ボス・右下固定)
// ============================================================
void EnemyPat_FixedMaze_Zai()
{
    if (count == 1) {
        enemy.x = CellCX(MZ_W - 1);      // 450.0 右下で完全固定
        enemy.y = CellCY(MZ_H - 1);      // 450.0
        enemy.maxHp = enemy.hp = 30;    // 200で固定
        player.x = CellCX(0);
        player.y = CellCX(0);

        mazeDone = false;
        mazeDoneAt = 0;

        // 迷路壁の展開セット(1つだけ)
        AddShotSet(ShotMaze, 0);
    }

    // 敵は動かない。迷路が完成したら一定間隔で追尾弾の部隊を送り込む
    if (mazeDone && (count - mazeDoneAt) % 180 == 30) {
        AddShotSet(ShotSeekers, 1);
    }
}