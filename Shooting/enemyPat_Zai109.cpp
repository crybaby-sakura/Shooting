// enemyPat_Tmp.cpp
// 弾幕「薔薇の輪舞(ロンド)」
// バラ曲線 r = a·cos(kθ) をモチーフにした3段階弾幕
//  第1段階(形成): 弾が中心から広がり、バラの花の輪郭を描く
//  第2段階(開花): 花全体がゆっくり回転する
//  第3段階(散華): 花びらの先端の弾だけがプレイヤーへ飛来し、
//                 残りは回転しながらゆっくり広がって消えていく

// ---- この弾幕専用の定数 ----
namespace Rose {
    const int    FORM_FRAMES = 60;    // 花の形成にかかるフレーム数
    const int    ROTATE_FRAMES = 120;   // 開花(回転)するフレーム数
    const int    TOTAL_FRAMES = FORM_FRAMES + ROTATE_FRAMES;
    const int    PER_FRAME = 6;     // 1フレームあたりの弾の生成数
    const double RADIUS = 150.0; // 花の大きさ a
    const double SPIN = 0.02/2;  // 開花時の回転角速度(ラジアン/フレーム)

    // sEnemyShotSet のパラメータ使用箇所
    //   param_i[0]    : 散華(放出)済みフラグ
    //   param_d[0/1]  : 花の中心座標(生成時に固定)
    //   param_d[2]    : 回転角速度(符号で回転方向を切替)
    // sEnemyShot のパラメータ使用箇所
    //   param_i[0]    : 花びらの先端フラグ(1なら散華時にプレイヤーへ飛ぶ)
    //   param_d[0/1]  : 花の中心から見た弾の相対座標
}

// 弾幕：薔薇曲線
static void ShotRose(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    const double cx = pEnemyShotSet->param_d[0];
    const double cy = pEnemyShotSet->param_d[1];

    // kind の偶奇で花びらの数を変更(奇数kならk枚、偶数kなら2k枚)
    const int k = (pEnemyShotSet->kind % 2 == 0) ? 5 : 4;

    // ---- 第1段階: 弾をバラ曲線に沿って配置していく ----
    if (pEnemyShotSet->count <= Rose::FORM_FRAMES) {
        if (pEnemyShotSet->count == 1) {
            // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        const int total = Rose::FORM_FRAMES * Rose::PER_FRAME;
        for (int j = 0; j < Rose::PER_FRAME; j++) {
            // GetRand(x) は 0 から x までの x+1 種類の整数をランダムに返す関数なので注意！
            const int    p = (pEnemyShotSet->count - 1) * Rose::PER_FRAME + j;
            const double theta = (double)p / total * 2.0 * DX_PI;
            const double r = Rose::RADIUS * cos(k * theta);

            pEnemyShot = new sEnemyShot;

            // 花の中心から見た相対座標(rが負の場合、自然と反対側の点になる)
            const double px = r * cos(theta);
            const double py = r * sin(theta);

            // 弾は中心から出て、形成終了フレームにちょうど曲線上へ到着する
            pEnemyShot->x = cx;
            pEnemyShot->y = cy;
            pEnemyShot->muki = atan2(py, px);
            int remaining = Rose::FORM_FRAMES - pEnemyShotSet->count;
            if (remaining < 1) remaining = 1;
            pEnemyShot->speed = sqrt(px * px + py * py) / remaining;
            pEnemyShot->margin = 480;

            // 花びらの先端(曲線の極大点付近)かどうかを記録
            pEnemyShot->param_i[0] = (fabs(cos(k * theta)) > 0.95) ? 1 : 0;
            pEnemyShot->param_d[0] = px;
            pEnemyShot->param_d[1] = py;

            if (pEnemyShot->param_i[0]) {
                pEnemyShot->kind = img_enemyShotMediumBall[0]; // 先端: 赤い中玉
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[5];  // 花びら: マゼンタの小玉
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // ---- 第2段階の開始: 開花の効果音 ----
    else if (pEnemyShotSet->count == Rose::FORM_FRAMES + 1) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // ---- 第3段階の開始: 散華(先端の弾だけプレイヤーへ放つ) ----
    if (pEnemyShotSet->count == Rose::TOTAL_FRAMES + 1) {
        pEnemyShotSet->param_i[0] = 1;

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0]) {
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x)
                    + (GetRand(30) - 15) / 180.0 * DX_PI; // 15度のばらつき
                pShot->speed = 5.0 + GetRand(150) / 100.0;        // 5.0〜6.5
            }
            pShot = pShot->next;
        }
    }

    // ---- 弾の移動 ----
    const double omega = pEnemyShotSet->param_d[2];
    const double c = cos(omega);
    const double s = sin(omega);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        if (pEnemyShotSet->count <= Rose::FORM_FRAMES) {
            // 形成中: 曲線上の目標点へ直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[0] && pEnemyShotSet->param_i[0]) {
            // 散華後の先端弾: プレイヤーへ向かって直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 開花中の花びら・散華後の残り弾: 花の中心の周りを回転する
            double nx = pShot->param_d[0] * c - pShot->param_d[1] * s;
            double ny = pShot->param_d[0] * s + pShot->param_d[1] * c;

            if (pEnemyShotSet->param_i[0]) {
                nx *= 1.01; // 散華後はゆっくり広がり、画面外へ出て消える
                ny *= 1.01;
            }

            pShot->param_d[0] = nx;
            pShot->param_d[1] = ny;
            pShot->x = cx + nx;
            pShot->y = cy + ny;
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_RoseCurve_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 240フレームごとにバラを一輪咲かせる
    // (形成60 + 開花120 + 散華後の余裕 で大体1周する)
    if ((count - 1) % 240 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRose;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 40.0;

        // 花の中心は生成時の位置に固定(敵が動いても花はその場に咲く)
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;

        // 回転方向は交互に変える
        pEnemyShotSet->param_d[2] = (shot_count % 2 == 0) ? Rose::SPIN : -Rose::SPIN;

        // kind の偶奇で花びらの数(5枚/8枚)が切り替わる
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