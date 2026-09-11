// enemyPat_Tmp.cpp
// 弾幕コンセプト：【雨下雷撃（うからいげき）】

#include <cmath>

// -----------------------------------------------------------
// 弾幕：雨のカーテン
// -----------------------------------------------------------
static void ShotRain(sEnemyShotSet* pEnemyShotSet)
{
    // 3フレームに1回、数発の雨粒を生成
    if (pEnemyShotSet->count % 3 == 0) {
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            // 画面の幅(480)に対して少し余裕を持たせる
            pEnemyShot->x = GetRand(520) - 20;
            pEnemyShot->y = -10.0;
            pEnemyShot->muki = DX_PI / 2.0; // 真下
            pEnemyShot->speed = 3.0 + GetRand(200) / 100.0; // 3.0 〜 5.0

            // 色はシアン(3)と青(4)の小玉をランダム
            pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotSmallBall[3] : img_enemyShotSmallBall[4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// -----------------------------------------------------------
// 弾幕：雷光と水沫（メイン攻撃）
// -----------------------------------------------------------
static void ShotThunder(sEnemyShotSet* pEnemyShotSet)
{
    int charge_time = 60; // 予兆から落雷までの時間

    // 予兆マーカーの設置
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->param_d[0];
        pEnemyShot->y = 20.0 + 20;
        pEnemyShot->muki = DX_PI / 2.0;
        pEnemyShot->speed = 0.0; // まだ動かない
        pEnemyShot->kind = img_enemyShotDiamond[1]; // 黄色の菱形弾
        pEnemyShot->param_i[0] = 1; // マーカーフラグ

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 落雷
    if (pEnemyShotSet->count == charge_time) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 黄色の短レーザーを連ねて一本の長い雷光を表現
        for (int i = 0; i < 24; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->param_d[0];
            pEnemyShot->y = -i * 32.0; // 短レーザー(長さ64)を半分重なるように配置
            pEnemyShot->muki = DX_PI / 2.0;
            pEnemyShot->speed = 18.0; // 高速で落下
            pEnemyShot->kind = img_enemyShotLaser[1]; // 黄色
            pEnemyShot->param_i[0] = 0; // 通常弾
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 着弾時の飛散（水沫）
    // 速度18で落下し、先頭の弾がy=450付近に到達する時間を計算
    // マーカー(y=20)がスタート地点なので、(450 - 20) / 18 ≒ 24フレーム後
    if (pEnemyShotSet->count == charge_time + 24) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int way = 16;
        for (int i = 0; i < way; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->param_d[0];
            pEnemyShot->y = 450.0;
            pEnemyShot->muki = (DX_PI * 2.0 / way) * i;
            pEnemyShot->speed = 1.5 + GetRand(150) / 100.0; // 1.5 〜 3.0
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白色の小玉
            pEnemyShot->param_i[0] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // マーカーなら落雷と同時に落下開始
        if (pShot->param_i[0] == 1) {
            if (pEnemyShotSet->count == charge_time) {
                pShot->speed = 18.0;
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// -----------------------------------------------------------
// 敵本体のパターン
// -----------------------------------------------------------
void EnemyPat_ThunderInRain_Gemini()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 弾幕を見せるため少し硬めに設定
        muki = 1;

        // 【ベース展開】雨のカーテンのセットを登録
        // このセットは消滅せず、毎フレーム雨を降らせ続ける
        sEnemyShotSet* pSetRain = new sEnemyShotSet;
        pSetRain->count = 0;
        pSetRain->patternFunc = ShotRain;
        pSetRain->x = enemy.x;
        pSetRain->y = enemy.y;

        pSetRain->pEnemyShotHead = new sEnemyShot;
        pSetRain->pEnemyShotHead->prev = pSetRain->pEnemyShotHead;
        pSetRain->pEnemyShotHead->next = pSetRain->pEnemyShotHead;

        pSetRain->prev = enemyShotSetHead.prev;
        pSetRain->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetRain;
        enemyShotSetHead.prev = pSetRain;
    }
    else {
        // 敵はゆっくり左右に揺れる
        enemy.x += 0.8 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 【メイン攻撃】一定周期で雷を落とすセットを登録
    if (count > 60 && count % 80 == 0) {
        sEnemyShotSet* pSetThunder = new sEnemyShotSet;
        pSetThunder->count = 0;
        pSetThunder->patternFunc = ShotThunder;

        // プレイヤーのX座標付近を狙うが、完全にエイムしないようバラけさせる
        double targetX = player.x + (GetRand(100) - 50);
        // 画面外にならないようクランプ
        if (targetX < 20.0) targetX = 20.0;
        if (targetX > 460.0) targetX = 460.0;

        pSetThunder->param_d[0] = targetX; // 落下地点を保存

        pSetThunder->pEnemyShotHead = new sEnemyShot;
        pSetThunder->pEnemyShotHead->prev = pSetThunder->pEnemyShotHead;
        pSetThunder->pEnemyShotHead->next = pSetThunder->pEnemyShotHead;

        pSetThunder->prev = enemyShotSetHead.prev;
        pSetThunder->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetThunder;
        enemyShotSetHead.prev = pSetThunder;
    }
}