// enemyPat_rose.cpp

// バラ曲線を描く弾
static void ShotRose(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // pShot->count だけから軌道を決めることで、速度積分による誤差蓄積を避ける。
        double theta = pShot->param_d[0] + pShot->param_d[1] * pShot->count;

        // 5枚花弁のバラ曲線
        // r = A * cos(5 * theta)
        double r = pShot->param_d[2] * cos(5.0 * theta);

        // 同じバラ曲線の上を、少しずつ外側へ成長させる。
        double growth = 0.72 + 0.28 * sin(pShot->param_d[1] * pShot->count * 0.42 + pShot->param_d[3]);

        pShot->x = pEnemyShotSet->x + r * growth * cos(theta);
        pShot->y = pEnemyShotSet->y + r * growth * sin(theta) + pEnemyShotSet->count * 3.0;

        // 弾そのものも軌道の接線方向へ向ける。
        double dr = -5.0 * pShot->param_d[2] * sin(5.0 * theta);
        double tx = dr * cos(theta) - r * sin(theta);
        double ty = dr * sin(theta) + r * cos(theta);
        pShot->muki = atan2(ty, tx);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_RoseCurve_ChatGPT()
{
    static int phase;
    static int shotCount;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        shotCount = 0;
    }
    else {
        // 上部をゆるく左右移動
        enemy.x = 240.0 + 150.0 * sin(count * 0.010);
        enemy.y = 60.0 + 20.0 * sin(count * 0.017);
    }

    // 花を描くレイヤーを周期的に追加
    if (count % 48 == 1) {
        //if (count % 144 == 1) {
        //    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        //    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        //}

        if (count % 48 == 1) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotRose;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0;
        pSet->muki = 0.0;
        pSet->kind = shotCount++;

        // 花ごとの回転・大きさ・成長位相
        pSet->param_d[0] = (phase % 8) * (DX_PI / 20.0);
        pSet->param_d[1] = 0.020 + (phase % 5) * 0.002;
        pSet->param_d[2] = 70.0 + (phase % 4) * 18.0;
        pSet->param_d[3] = (phase % 6) * (DX_PI / 6.0);

        // 交互に花の向きを変える
        if (phase % 2 == 1) {
            pSet->param_d[1] *= -1.0;
        }

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 1つのバラ曲線を多数の弾で埋める。
        const int bulletNum = 58;

        for (int i = 0; i < bulletNum; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            double startTheta = (double)i / bulletNum * DX_PI * 2.0;
            pShot->param_d[0] = startTheta + pSet->param_d[0];
            pShot->param_d[1] = pSet->param_d[1];
            pShot->param_d[2] = pSet->param_d[2];
            pShot->param_d[3] = pSet->param_d[3];

            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->speed = 0.0;

            // 華やかさと見やすさを両立するため、鱗弾と中楕円弾を使用。
            // 前半後半で色を変えて花びらの重なりを見やすくする。
            if ((i + phase) % 3 == 0) {
                pShot->kind = img_enemyShotMediumOval[(i + phase) % 8];
            }
            else {
                pShot->kind = img_enemyShotScale[(i + phase + 5) % 8];
            }

            pShot->margin = 80.0;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        phase++;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}