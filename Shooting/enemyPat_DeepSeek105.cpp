// EnemyPat_Suikawari_DeepSeek.cpp
// スイカ割りをモチーフにした弾幕パターン
// 敵本体関数: void EnemyPat_Suikawari_DeepSeek()

#include <cmath> // cos, sin, atan2

//-----------------------------------------------------------
// 弾幕パターン：スイカ割り
// 大きな赤い弾（スイカ本体）が飛び、一定時間後に
// 赤い中玉（果肉）・黒い小玉（種）・緑の小玉（皮）に分裂する
//-----------------------------------------------------------
static void ShotWatermelonSplit(sEnemyShotSet* pSet)
{
    const int SPLIT_FRAME = 60; // 分裂までのフレーム数

    // 初回のみスイカ本体を生成
    if (pSet->count == 0) {
        sEnemyShot* melon = new sEnemyShot;

        melon->x = pSet->x;
        melon->y = pSet->y;
        melon->muki = pSet->muki;          // 自機方向
        melon->speed = 1.2;                // ゆっくり飛ぶ
        melon->kind = img_enemyShotLargeBall[0]; // 赤・大玉
        melon->count = 0;                  // メインルーチンがインクリメント
        melon->param_i[0] = 0;             // 状態：スイカ

        // リスト先頭に挿入
        melon->prev = pSet->pEnemyShotHead;
        melon->next = pSet->pEnemyShotHead->next;
        pSet->pEnemyShotHead->next->prev = melon;
        pSet->pEnemyShotHead->next = melon;
        return;
    }

    // 毎フレーム：弾の移動と分裂処理
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* next = p->next; // 削除される可能性があるため保存

        // スイカ本体が分裂タイミングを迎えたか
        if (p->kind == img_enemyShotLargeBall[0] && p->count >= SPLIT_FRAME) {
            double splitX = p->x;
            double splitY = p->y;

            // スイカ本体をリストから削除
            p->prev->next = p->next;
            p->next->prev = p->prev;
            delete p;

            // 分裂音（重め）
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // --- 果肉：赤い中玉 12個を全方位に ---
            for (int i = 0; i < 12*7; ++i) {
                sEnemyShot* flesh = new sEnemyShot;
                flesh->x = splitX;
                flesh->y = splitY;
                double angle = (2.0 * DX_PI * i) / 12.0/5;
                flesh->muki = angle;
                flesh->speed = 2.0 + GetRand(100) / 100.0; // 2.0～3.0
                flesh->kind = img_enemyShotMediumBall[0];   // 赤・中玉
                flesh->param_i[0] = 1;                      // 果肉

                flesh->prev = pSet->pEnemyShotHead;
                flesh->next = pSet->pEnemyShotHead->next;
                pSet->pEnemyShotHead->next->prev = flesh;
                pSet->pEnemyShotHead->next = flesh;
            }

            // --- 種：黒い小玉 8個をランダム方向に ---
            for (int i = 0; i < 8*7; ++i) {
                sEnemyShot* seed = new sEnemyShot;
                seed->x = splitX + GetRand(20) - 10; // 少し散らす
                seed->y = splitY + GetRand(20) - 10;
                double angle = GetRand(360) / 180.0 * DX_PI; // 0～2π
                seed->muki = angle;
                seed->speed = 1.5 + GetRand(200) / 100.0; // 1.5～3.5
                seed->kind = img_enemyShotSmallBall[7];    // 黒・小玉
                seed->param_i[0] = 2;                      // 種

                seed->prev = pSet->pEnemyShotHead;
                seed->next = pSet->pEnemyShotHead->next;
                pSet->pEnemyShotHead->next->prev = seed;
                pSet->pEnemyShotHead->next = seed;
            }

            // --- 皮：緑の小玉 4個をランダム方向に（やや遅め） ---
            for (int i = 0; i < 4*7; ++i) {
                sEnemyShot* rind = new sEnemyShot;
                rind->x = splitX;
                rind->y = splitY;
                double angle = GetRand(360) / 180.0 * DX_PI;
                rind->muki = angle;
                rind->speed = 1.0 + GetRand(100) / 100.0; // 1.0～2.0
                rind->kind = img_enemyShotSmallBall[2];    // 緑・小玉
                rind->param_i[0] = 3;                      // 皮

                rind->prev = pSet->pEnemyShotHead;
                rind->next = pSet->pEnemyShotHead->next;
                pSet->pEnemyShotHead->next->prev = rind;
                pSet->pEnemyShotHead->next = rind;
            }
        }
        else {
            // 通常の直線移動
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }

        p = next;
    }
}

//-----------------------------------------------------------
// 敵本体パターン
// 左右に動きながら、一定間隔でスイカ割り弾幕を発射する
//-----------------------------------------------------------
void EnemyPat_Suikawari_DeepSeek()
{
    static int muki;
    static int shot_count;

    // 初期化（count == 1 のときのみ）
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 150フレームごとにスイカ割り弾幕セットを生成
    if (count % 150 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;                     // メインルーチンがインクリメント
        pSet->patternFunc = ShotWatermelonSplit;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = shot_count++;           // 弾の種類・色のバリエーション用

        // センチネルノードの初期化
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 敵弾セットリストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}