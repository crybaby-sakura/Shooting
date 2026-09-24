#include <cmath> // std::sin, std::cos, std::abs, std::atan2 用

// --------------------------------------------------------
// 弾幕：華旋『ローゼン・クロックワーク』
// --------------------------------------------------------
static void ShotRosenClockwork(sEnemyShotSet* pEnemyShotSet)
{
    // === パラメータ設定 ===
    const int    K = 5;       // 花弁の数 (奇数を指定するとK枚になる)
    const double OMEGA = 0.006/5;   // 旋回角速度
    const double V = 0.7;     // 膨張速度
    const double A_0 = 0.0;     // 初期半径オフセット
    const double R_MIN = 25.0;    // 最小半径 (くびれ部分の安地幅)
    const int    NUM_SHOTS = 350-150;     // 弾の総数 (バラの輪郭を形作る)
    const int    PHASE_STOP = 200;     // 膨張が停止するフレーム数
    const int    PHASE_FIRE = 260;     // 散華 (一斉射出) するフレーム数

    // 1. 結実（弾の初期生成）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < NUM_SHOTS; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 弾に割り当てる初期角度 (0 ～ 2π)
            double theta = (double)i * (2.0 * DX_PI / (double)NUM_SHOTS);

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青色小玉
            pEnemyShot->margin = 80;

            pEnemyShot->param_d[0] = theta; // 初期角度を保持しておく
            pEnemyShot->param_i[0] = 0;     // 状態フラグ (0:バラ曲線軌道, 1:直進軌道)

            // リストへの追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 2. 散華の効果音再生
    if (pEnemyShotSet->count == PHASE_FIRE) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 3. 各弾の軌道計算と更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        if (pShot->param_i[0] == 0) {
            // --- 【開花と旋回フェーズ】バラ曲線の方程式に従って座標を強制更新 ---

            double t = (double)pEnemyShotSet->count;
            // 膨張用時間は途中でストップさせる（一瞬停止する演出のため）
            double t_exp = (t > PHASE_STOP) ? PHASE_STOP : t;
            // 旋回用時間は進み続ける
            double t_rot = t;

            double theta = pShot->param_d[0];

            // 極座標方程式: r = (a_0 + v*t) * |sin(k*(θ - ω*t))| + r_min
            double r = (A_0 + V * t_exp) * std::abs(std::sin(K * (theta - OMEGA * t_rot))) + R_MIN;

            // 中心座標(セットの生成位置)からの相対座標に変換
            pShot->x = pEnemyShotSet->x + r * std::cos(theta);
            pShot->y = pEnemyShotSet->y + r * std::sin(theta);

            // 規定フレームに達したら散華フェーズへ移行
            if (pEnemyShotSet->count == PHASE_FIRE) {
                pShot->param_i[0] = 1; // 直進モードへフラグ切替
                pShot->kind = img_enemyShotLaser[0]; // 赤色短レーザーへ変化

                // バラの形を保ったまま放射状に弾けるため、中心から外側への角度を設定
                pShot->muki = std::atan2(pShot->y - pEnemyShotSet->y, pShot->x - pEnemyShotSet->x);
                pShot->speed = 3.5; // 速度
            }
        }
        else {
            // --- 【散華フェーズ】設定された角度と速度で直進 ---
            pShot->x += pShot->speed * std::cos(pShot->muki);
            pShot->y += pShot->speed * std::sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// --------------------------------------------------------
// 敵本体のパターン関数
// --------------------------------------------------------
void EnemyPat_RoseCurve_Gemini()
{
    // 初期化処理
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 120.0;               // 画面中央上部に配置
        enemy.maxHp = enemy.hp = 200; // ボスを想定し少し硬めに設定
    }

    // ボス本体はバラ曲線の中心として機能するため、8の字に優雅に揺らして美しさを強調する
    enemy.x = 240.0 + 40.0 * std::sin(count * 0.015);
    enemy.y = 120.0 + 15.0 * std::sin(count * 0.030);

    // 400フレーム周期でバラ曲線弾幕を展開
    // (弾幕が完全に画面外に消える前に次を展開して圧迫感を持たせる)
    if (count % 400 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRosenClockwork;

        // 展開の中心点をその瞬間のボス座標に固定する
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストへの追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}