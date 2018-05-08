#include "Ants.h"

void ScoreDiffusion(game_state_t *Game, game_info_t *Info, int scoreTable[], const int row, const int col, int type) {
    int dir;
    square_info_t *sqInfo;
    for (dir = 0; dir < MAXDIR; ++dir) {
        int r1 = row;
        int c1 = col;
        GetNewCoordsByDir(&r1, &c1, r1, c1, dir, Info);
        int offset = GetOffset(Info, r1, c1);
        sqInfo = &Game->mapValues[offset];
        if (GetMapObjectByOffset(Info, offset) == '%') continue;
        scoreTable[dir] = 0;
        switch (type) {
            case FOOD: scoreTable[dir] += sqInfo->foodval; break;
            case ENEMYHILL: scoreTable[dir] += sqInfo->hillval; break;
            case ENEMYANT: scoreTable[dir] += sqInfo->enemyval; break;
            case DEFENSE: scoreTable[dir] += sqInfo->defenseval; break;
            case EXPLORE: scoreTable[dir] += sqInfo->exploreval; break;
            case TRAIL: scoreTable[dir] += sqInfo->trailval; break;
            case RANDOM: scoreTable[dir] += sqInfo->foodval + sqInfo->hillval + sqInfo->enemyval + sqInfo->defenseval
                /*+ sqInfo->trailval*/ + sqInfo->exploreval; break; // play with commenting out trailval
        }
    }
}

// sends a move to the tournament engine and keeps track of ants new location
void MoveAnt(int index, int dir, game_state_t* Game, game_info_t* Info) {
    assert(dir < MAXDIR);

    static const char dirString[] = "NEWS";
    fprintf(stdout, "O %i %i %c\n", Game->my_ants[index].row, Game->my_ants[index].col, dirString[dir]);

    // update map here also to enable ants following another
    int offset = GetOffset(Info, Game->my_ants[index].row, Game->my_ants[index].col);
    Info->map[offset] = '.'; // assign passable land
    Game->ownIndexTable[offset] = -1;
    if (Game->mapValues[offset].trailval <= 0) Game->mapValues[offset].trailval = TrailVal;
    switch (dir) {
        case NORTH: Game->my_ants[index].row = (Info->rows + Game->my_ants[index].row - 1) % Info->rows; break;
        case SOUTH: Game->my_ants[index].row = (Info->rows + Game->my_ants[index].row + 1) % Info->rows; break;
        case EAST: Game->my_ants[index].col = (Info->cols + Game->my_ants[index].col + 1) % Info->cols; break;
        case WEST: Game->my_ants[index].col = (Info->cols + Game->my_ants[index].col - 1) % Info->cols; break;
    }
    offset = GetOffset(Info, Game->my_ants[index].row, Game->my_ants[index].col);
    Info->map[offset] = 'a'; // relocate own ant
    Game->ownIndexTable[offset] = index;
}

void MoveAntLocation(game_state_t *Game, game_info_t *Info, const int i, const int dir, const int type) {
    int offset = GetOffsetByDir(Info, Game->my_ants[i].row, Game->my_ants[i].col, dir);
    Print(DEBUG_PRINT_FLAG, outFile, "Ant --> %s: from = %d, to = %d, dir = %d\n", MoveTypeStr[type], Game->my_ants[i].row * Info->cols + Game->my_ants[i].col, offset, dir);
    MoveAnt(i, dir, Game, Info);
    Game->moveToTable[offset] = 1;
    Game->my_ants[i].moved = 1;
}

// TODO: make this more accurate
int IsAntSafe(game_state_t *Game, game_info_t *Info, int row, int col, int dir, int type) {
    int enemyAnts, myAnts;
    GetNewCoordsByDir(&row, &col, row, col, dir, Info);
    enemyAnts = Game->enemyAttacksTable[GetOffsetByDir(Info, row, col, dir)];
    GetNewCoordsByDir(&row, &col, row, col, dir, Info);
    GetNewCoordsByDir(&row, &col, row, col, dir, Info);
    myAnts = Game->ownAttacksTable[GetOffset(Info, row, col)];
    if (enemyAnts >= myAnts) return 0;
    return 1;
}

int FindBestDirectionForAnt(game_state_t *Game, game_info_t *Info, int i, int type, int* dir) {
    int scoreTable[MAXDIR] = {0, 0, 0, 0};
    ScoreDiffusion(Game, Info, scoreTable, Game->my_ants[i].row, Game->my_ants[i].col, type);
    Print(DEBUG_PRINT_FLAG, outFile, "scoreTable %s: %d %d %d %d\n", MoveTypeStr[type], scoreTable[0], scoreTable[1], scoreTable[2], scoreTable[3]);
    *dir = -1;
    if (!DiffusionScoreIsGood(scoreTable)) return -MaxValue;
    int tries;
    for (tries = 0; tries < MAXDIR; ++tries) {
        int score = GetBestDirByHighestVal(scoreTable, dir);
        if (*dir == -1) continue;
        int offset = GetOffsetByDir(Info, Game->my_ants[i].row, Game->my_ants[i].col, *dir);
        char object = GetMapObjectByOffset(Info, offset);
        if ((object == '.' || (object >= '0' && object <= '9')) && !Game->moveToTable[offset]
        && IsAntSafe(Game, Info, Game->my_ants[i].row, Game->my_ants[i].col, *dir, type)) return score;
    }
    return -MaxValue;
}


void FindAndMoveNearestAnt(game_state_t *Game, game_info_t *Info, int type) {
    int bestIndex, bestDir, j, farthest;
    farthest = -MaxValue;
    bestIndex = -1;
    bestDir = 0;
    for (j = 0; j < Game->my_count; ++j) {
        int dist;
        int tempdir;
        if (Game->my_ants[j].moved) continue;
        dist = FindBestDirectionForAnt(Game, Info, j, type, &tempdir);
        if (dist > farthest) {
            farthest = dist;
            bestDir = tempdir;
            bestIndex = j;
            Print(DEBUG_PRINT_FLAG, outFile, "Reachable %s: bestIndex = %d, nearest = %d, bestDir = %d\n", MoveTypeStr[type], bestIndex, farthest, bestDir);
        }
    }
    if (bestIndex != -1) {
        MoveAntLocation(Game, Info, bestIndex, bestDir, type);
    }
}

void AttackEnemyAnts(game_state_t *Game, game_info_t *Info, int i, int type) {
    int row = Game->enemy_ants[i].row;
    int col = Game->enemy_ants[i].col;
    int found = 0;
    int dir = NORTH;
    int antsToSend = 0;
    int enemyAntsAtt;
    int j;
    int antOffset[13];

    for (dir = 0; dir < MAXDIR; ++dir) {
        int r1 = row, c1 = col;
        if (CountEnemyAntsInAttackRadius(Game, Info, r1, c1, dir)) continue;
        antsToSend = CountMyAntsInAttackRadius(Game, Info, r1, c1, antOffset, dir);
        GetNewCoordsByDir(&r1, &c1, r1, c1, dir, Info);
        GetNewCoordsByDir(&r1, &c1, r1, c1, dir, Info);
        GetNewCoordsByDir(&r1, &c1, r1, c1, dir, Info);
        enemyAntsAtt = Game->enemyAttacksTable[GetOffset(Info, r1, c1)];
        if (enemyAntsAtt >= antsToSend) continue;
        found = 1;
        break;
    }
    if (found) {
        // send nearest ants to attack enemy ant
        for (j = 0; j < antsToSend; ++j) {
            int bestIndex = Game->ownIndexTable[antOffset[j]];
            int bestDir = MoveToDirectionSrcDest(Game->my_ants[bestIndex].row, Game->my_ants[bestIndex].col, row, col);
            MoveAntLocation(Game, Info, bestIndex, bestDir, type);
        }
    }
    else {
        if (Game->ownAttacksTable[GetOffset(Info, Game->enemy_ants[i].row, Game->enemy_ants[i].col)] < 1) return;
        DiffuseValuesToMap(Game, Info, Game->enemy_ants[i].row, Game->enemy_ants[i].col, EnemyVal, type);
        FindAndMoveNearestAnt(Game, Info, type);
        UnDiffuseValuesToMap(Game, Info, Game->enemy_ants[i].row, Game->enemy_ants[i].col, EnemyVal, type);
    }
}

void DoTurn(game_state_t *Game, game_info_t *Info) {
    int i, j;
    uint64_t startTime = GetTime();

    Print(DEBUG_PRINT_FLAG, outFile, "Run: ants = %d, food = %d\n", Game->my_count, Game->food_count);

    // for all foods find nearest ant and move accordingly
    for (i = 0; i < Game->food_count; ++i) {
        DiffuseValuesToMap(Game, Info, Game->food[i].row, Game->food[i].col, FoodVal, FOOD);
        FindAndMoveNearestAnt(Game, Info, FOOD);
        UnDiffuseValuesToMap(Game, Info, Game->food[i].row, Game->food[i].col, FoodVal, FOOD);
    }
    Print(DEBUG_TIME, outFile, "Time After %s: %d\n", MoveTypeStr[FOOD], GetTime() - startTime);

    for (i = 0; i < Game->hill_count; ++i) {
        if (Game->hills[i].razed) continue;
        if (Game->hills[i].player == '0') { // own hill, attack nearest enemy, or if none, just defend the fort
            Print(DEBUG_DISTANCESCORE, outFile, "Game->hills[%d].player = %c", i, Game->hills[i].player);
            int antsToSend = Game->my_count * 2 / (Game->my_hill_count * 10); // 20% of ants for homeland security
            for (j = 0; j < antsToSend; ++j) {
                FindAndMoveNearestAnt(Game, Info, DEFENSE);
            }
            Print(DEBUG_TIME, outFile, "Time After %s: %d\n", MoveTypeStr[DEFENSE], GetTime() - startTime);
        }
    }

    for (i = 0; i < Game->hill_count; ++i) {
        if (Game->hills[i].razed) continue;
        if (Game->hills[i].player != '0') {
            Print(DEBUG_DISTANCESCORE, outFile, "Game->hills[%d].player = %c", i, Game->hills[i].player);
            int antsToSend = Game->my_count * 2 / 10; // 20% of ants for international peacekeeping
            for (j = 0; j < antsToSend; ++j) {
                FindAndMoveNearestAnt(Game, Info, ENEMYHILL);
            }
            Print(DEBUG_TIME, outFile, "Time After %s: %d\n", MoveTypeStr[ENEMYHILL], GetTime() - startTime);
        }
    }

    for (i = 0; i < Game->enemy_count; ++i) {
        AttackEnemyAnts(Game, Info, i, ENEMYANT);
    }
    Print(DEBUG_TIME, outFile, "Time After %s: %d\n", MoveTypeStr[ENEMYANT], GetTime() - startTime);

    for (i = 0; i < Game->my_count; ++i) {
        if (Game->my_ants[i].moved) continue;
        int bestDir;
        int score = FindBestDirectionForAnt(Game, Info, i, EXPLORE, &bestDir);
        if (score == -MaxValue) continue;
        MoveAntLocation(Game, Info, i, bestDir, EXPLORE);
    }
    Print(DEBUG_TIME, outFile, "Time After %s: %d\n", MoveTypeStr[EXPLORE], GetTime() - startTime);

    // remaining ants that didn't move just move randomly to safe square, as it may get killed when not moving
    for (i = 0; i < Game->my_count; ++i) {
        if (Game->my_ants[i].moved) continue;
        int bestDir;
        int score = FindBestDirectionForAnt(Game, Info, i, RANDOM, &bestDir);
        if (score == -MaxValue) continue;
        MoveAntLocation(Game, Info, i, bestDir, TRAIL);
    }
    Print(DEBUG_TIME, outFile, "Time After %s: %d\n", MoveTypeStr[RANDOM], GetTime() - startTime);

    Print(DEBUG_PRINT_FLAG, outFile, "\n\n");
}
