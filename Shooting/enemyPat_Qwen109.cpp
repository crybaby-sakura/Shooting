// enemyPat_sampleForAI.cpp

#include <cmath>

// 弾幕：バラ曲線（ローズカーブ）
static void ShotRoseCurve(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 発射時の効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int num_petals = 5;             // 5枚の花びら
        int bullets_per_petal = 12*3;     // 花びらあたりの弾数
        int total_bullets = num_petals * bullets_per_petal; // 合計60発

        for (int i = 0; i < total_bullets; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 角度を 0〜2π に均等配置
            double theta = (2.0 * DX_PI / total_bullets) * i;

            // プレイヤー方向を基準角度として加算
            double actual_theta = pEnemyShotSet->muki + theta;

            // バラ曲線の半径: r = base + amp * |cos(n * theta)|
            double r_base = 30.0;
            double r_amp = 110.0;
            double cos_val = std::cos(num_petals * theta);
            double r = r_base + r_amp * std::abs(cos_val);

            // 初期位置の設定
            pEnemyShot->x = pEnemyShotSet->x + r * std::cos(actual_theta);
            pEnemyShot->y = pEnemyShotSet->y + r * std::sin(actual_theta);

            // 速度設定：花びらの形状を維持しつつ回転拡大させる
            // 基本の放射速度
            double speed_radial = 2.0;
            // 花びらの先端(cos_valの絶対値が大きい)は少し遅く、谷は速くすると形状が崩れにくい
            speed_radial += (1.0 - std::abs(cos_val)) * 1.0;

            // 回転成分（反時計回りにゆっくり回転させる）
            double rotate_offset = 0.08;

            // 最終的な進行角度
            double move_angle = actual_theta + rotate_offset;

            pEnemyShot->muki = move_angle;
            pEnemyShot->speed = speed_radial;

            // 弾の種類と色分け（比率に応じて変化）
            double ratio = std::abs(cos_val);
            if (ratio > 0.7) {
                // 花びらの先端: 中楕円弾 マゼンタ(5)
                pEnemyShot->kind = img_enemyShotMediumOval[5];
            }
            else if (ratio > 0.3) {
                // 花びらの中間部: 鱗弾 赤(0)
                pEnemyShot->kind = img_enemyShotScale[0];
            }
            else {
                // 谷の部分: 小玉 白(6)
                pEnemyShot->kind = img_enemyShotSmallBall[6];
            }

            // 弾をリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * std::cos(pShot->muki);
        pShot->y += pShot->speed * std::sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_RoseCurve_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0; // 画面中央より少し上。バラ曲線が画面内に収まるように調整
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右往復移動
        enemy.x += 1.2 * (double)muki;
        if (enemy.x > 400.0 || enemy.x < 80.0) {
            muki *= -1;
        }
    }

    // 90フレーム(約1.5秒)ごとにバラ曲線弾幕を発射
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRoseCurve;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // 弾幕の基準向きをプレイヤー方向にする
        pEnemyShotSet->muki = std::atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}