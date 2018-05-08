#include "Ants.h"

// initializes the game_info structure on the very first turn
// function is not called after the game has started

void InitAntsData(char *data, game_info_t *game_info) {
    char *replace_data = data;
    while (*replace_data != '\0') {
        if (*replace_data == '\n')
            *replace_data = '\0';
        ++replace_data;
    }
    while (42) {
        char *value = data;
        while (*++value != ' ');
        ++value;
        int num_value = atoi(value);
	    switch (*data) {
            case 'l':
                game_info->loadtime = num_value;
                break;
            case 't':
                if (*(data + 4) == 't')
                    game_info->turntime = num_value;
                else
                    game_info->turns = num_value;
                break;
            case 'r':
                game_info->rows = num_value;
                break;
            case 'c':
                game_info->cols = num_value;
                break;
            case 'v':
                game_info->viewradius_sq = num_value;
                break;
            case 'a':
                game_info->attackradius_sq = num_value;
                break;
            case 's':
                if (*(data + 1) == 'p')
                    game_info->spawnradius_sq = num_value;
                else
                    game_info->seed = num_value;
                break;
        }
        data = value;
        while (*++data != '\0');
        ++data;
        if (strcmp(data, "ready") == 0)
            break;
    }
    if (DEBUG_GAMEINFO) {
        Print(DEBUG_GAMEINFO, outFile, "Game Info:\n");
        Print(DEBUG_GAMEINFO, outFile, "loadtime = %d\n", game_info->loadtime);
        Print(DEBUG_GAMEINFO, outFile, "turntime = %d\n", game_info->turntime);
        Print(DEBUG_GAMEINFO, outFile, "rows = %d\n", game_info->rows);
        Print(DEBUG_GAMEINFO, outFile, "cols = %d\n", game_info->cols);
        Print(DEBUG_GAMEINFO, outFile, "turns = %d\n", game_info->turns);
        Print(DEBUG_GAMEINFO, outFile, "viewradius_sq = %d\n", game_info->viewradius_sq);
        Print(DEBUG_GAMEINFO, outFile, "attackradius_sq = %d\n", game_info->attackradius_sq);
        Print(DEBUG_GAMEINFO, outFile, "spawnradius_sq = %d\n", game_info->spawnradius_sq);
        Print(DEBUG_GAMEINFO, outFile, "seed = %d\n", game_info->seed);
    }
}

void InitDiffusionValues(game_info_t *game_info, game_state_t *game_state) {
    int row, col, idx, i;
    if (game_state->mapValues == 0) {
        game_state->mapValues = malloc(game_info->rows * game_info->cols * sizeof(square_info_t));
        memset(game_state->mapValues, 0, (game_info->rows * game_info->cols * sizeof(square_info_t)));
    }
    for (row = 0; row < game_info->rows; ++row) {
        for (col = 0; col < game_info->cols; ++col) {
            int offset = GetOffset(game_info, row, col);
            game_state->mapValues[offset].isVisible = 0;
            game_state->mapValues[offset].foodval = 0;
            game_state->mapValues[offset].hillval = 0;
            game_state->mapValues[offset].enemyval = 0;
            game_state->mapValues[offset].defenseval = 0;
            game_state->mapValues[offset].exploreval = 0;
            // age the entry per turn
            game_state->mapValues[offset].trailval -= 1;
            if (game_state->mapValues[offset].trailval < 0) game_state->mapValues[offset].trailval = 0;
            for (idx = 0; idx < game_state->my_count; ++idx) {
                if (DistanceEuclidianSquared(row, col, game_state->my_ants[idx].row, game_state->my_ants[idx].col, game_info) <= game_info->viewradius_sq) {
                    game_state->mapValues[offset].isVisible = 1;
                    game_state->mapValues[offset].lastSeen = game_info->currturn;
                }
            }
        }
    }
    for (row = 0; row < game_info->rows; ++row) {
        for (col = 0; col < game_info->cols; ++col) {
            int offset = GetOffset(game_info, row, col);
            char object = GetMapObjectByOffset(game_info, offset);
            if (object == '%') {
                game_state->mapValues[offset].foodval = 0;
                game_state->mapValues[offset].hillval = 0;
                game_state->mapValues[offset].enemyval = 0;
                game_state->mapValues[offset].exploreval = 0;
                game_state->mapValues[offset].trailval = 0;
                continue;
            }
//            if (object == '*') {
//                DiffuseValuesToMap(game_state, game_info, row, col, FoodVal, FOOD);
//            }
//            if (object > 'a' && object <= 'j') {
//                DiffuseValuesToMap(game_state, game_info, row, col, EnemyVal, ENEMYANT);
//            }
            if (object == '.') {
                int valexplore = 0;
                if (!game_state->mapValues[offset].isVisible) valexplore = ExploreVal + game_info->currturn - game_state->mapValues[offset].lastSeen;
                DiffuseValuesToMap(game_state, game_info, row, col, valexplore, EXPLORE);
            }
        }
    }
    for (i = 0; i < game_state->hill_count; ++i) {
        if (game_state->hills[i].razed) continue;
        row = game_state->hills[i].row;
        col = game_state->hills[i].col;
        if (game_state->hills[i].player == '0') {
            int offset;
            int nearest = MaxValue;
            int bestIdx[MAXDIR] = {-1, -1, -1, -1}; // defend against attacks in 4 directions
            for (idx = 0; idx < game_state->enemy_count; ++idx) {
                int dist = DistanceEuclidianSquared(row, col, game_state->enemy_ants[idx].row, game_state->enemy_ants[idx].col, game_info);
                if (dist < (game_info->viewradius_sq * 16) && dist < nearest) {
                    nearest = dist;
                    bestIdx[3] = bestIdx[2];
                    bestIdx[2] = bestIdx[1];
                    bestIdx[1] = bestIdx[0];
                    bestIdx[0] = idx;
                }
            }
            for (idx = 0; idx < MAXDIR; ++idx) {
                if (bestIdx[idx] != -1) {
                    DiffuseValuesToMap(game_state, game_info, game_state->enemy_ants[bestIdx[idx]].row, game_state->enemy_ants[bestIdx[idx]].col, DefenseVal, DEFENSE);
                }
            }

            // own hill square, score everything by zero
            offset = GetOffset(game_info, row, col);
            game_state->mapValues[offset].foodval = 0;
            game_state->mapValues[offset].hillval = 0;
            game_state->mapValues[offset].enemyval = 0;
            game_state->mapValues[offset].defenseval = 0;
            game_state->mapValues[offset].exploreval = 0;
        }
        if (game_state->hills[i].player > '0' && game_state->hills[i].player <= '9') {
            DiffuseValuesToMap(game_state, game_info, row, col, HillVal, ENEMYHILL);
        }
    }
    if (DEBUG_DIFFUSION) {
        Print(DEBUG_DIFFUSION, outFile, "\n\n");
        for (row = 0; row < game_info->rows; ++row) {
        for (col = 0; col < game_info->cols; ++col) {
                int offset = GetOffset(game_info, row, col);
                char object = GetMapObjectByOffset(game_info, offset);
                if (object == '%' || object == 'a' ) Print(DEBUG_DIFFUSION, outFile, "%c  ", object);
                else Print(DEBUG_DIFFUSION, outFile, "%d ", game_state->mapValues[offset].defenseval);
            }
            Print(DEBUG_DIFFUSION, outFile, "\n");
        }
        Print(DEBUG_DIFFUSION, outFile, "\n\n");
    }
}

// updates game data with locations of ants and food
// only the ids of your ants are preserved

void InitGameData(game_info_t *game_info, game_state_t *game_state) {
    int map_len = game_info->rows*game_info->cols;
    int my_count = 0;
    int enemy_count = 0;
    int food_count = 0;
    int dead_count = 0;
    int hill_count = 0;
    int i, j, k;
    hill_t tempHill[20];
    int tempHillCnt = 0;

    for (i = 0; i < map_len; ++i) {
        char current = game_info->map[i];

        if (current == '?' || current == '.' || current == '%')
            continue;
        else if (current == '*')
            ++food_count;
        else if (current == 'a')
            ++my_count;
        else if (current == '!')
            ++dead_count;
        else if (current >= 48 && current < 58)
            ++hill_count;
        else if (current >=  'b' && current <=  'j')
            ++enemy_count;
    }

    game_state->my_count = my_count;
    game_state->enemy_count = enemy_count;
    game_state->food_count = food_count;
    game_state->dead_count = dead_count;

    my_ant_t *my_old = 0;;
    int my_old_count = game_state->my_count;
    if (game_state->my_ants != 0) my_old = game_state->my_ants;

    if (game_state->enemy_ants != 0) free(game_state->enemy_ants);
    if (game_state->food != 0) free(game_state->food);
    if (game_state->dead_ants != 0) free(game_state->dead_ants);

    if (my_count > 0) game_state->my_ants = malloc(my_count*sizeof(my_ant_t));
    else game_state->my_ants = 0;

    if (enemy_count > 0) game_state->enemy_ants = malloc(enemy_count*sizeof(basic_ant_t));
    else game_state->enemy_ants = 0;

    if (dead_count > 0) game_state->dead_ants = malloc(dead_count*sizeof(basic_ant_t));
    else game_state->dead_ants = 0;

    if (food_count > 0) game_state->food = malloc(food_count*sizeof(food_t));
    else game_state->food = 0;

    for (i = 0; i < game_info->rows; ++i) {
        for (j = 0; j < game_info->cols; ++j) {
            char current = game_info->map[game_info->cols*i + j];
            if (current == '?' || current == '.' || current == '%')
                 continue;

            if (current == '*') {
                --food_count;

                game_state->food[food_count].row = i;
                game_state->food[food_count].col = j;
            } else if (current == 'a') {
                --my_count;

                int keep_id = -1;
                int k = 0;

                if (my_old != 0) {
                    for (; k < my_old_count; ++k) {
                        if (my_old[k].row == i && my_old[k].col == j) {
                            keep_id = my_old[k].id;
                            break;
                        }
                    }
                }
                if (keep_id == -1) game_state->my_ants[my_count].id = ++game_state->my_ant_index;
                else game_state->my_ants[my_count].id = keep_id;

                game_state->my_ants[my_count].row = i;
                game_state->my_ants[my_count].col = j;
                game_state->my_ants[my_count].moved = 0;;
            } else if (current == '!') {
                --dead_count;

                game_state->dead_ants[dead_count].row = i;
                game_state->dead_ants[dead_count].col = j;
                game_state->dead_ants[dead_count].player = current;
            } else if (current >= 'b' && current <= 'j') {
                --enemy_count;

                game_state->enemy_ants[enemy_count].row = i;
                game_state->enemy_ants[enemy_count].col = j;
                game_state->enemy_ants[enemy_count].player = current;
            } else if (current >= '0' && current <= '9') {
                int offset = i * game_info->rows + j;
                for (k = 0; k < game_state->hill_count; ++k) {
                    int offset2 = game_state->hills[k].row * game_info->rows + game_state->hills[k].col;
                    if (offset == offset2) break;
                }
                if (k >= game_state->hill_count) { // insert new found hill
                    game_state->hills[game_state->hill_count].row = i;
                    game_state->hills[game_state->hill_count].col = j;
                    game_state->hills[game_state->hill_count].razed = 0;
                    game_state->hills[game_state->hill_count].player = current;
                    ++game_state->hill_count;
                }
                tempHill[tempHillCnt].row = i;
                tempHill[tempHillCnt].col = j;
                tempHill[tempHillCnt].razed = 0;
                tempHill[tempHillCnt].player = current;
                ++tempHillCnt;
            }
        }
    }
    // find razed hill here
    for (i = 0; i < game_state->hill_count; ++i) {
        int hillIsNearMyAnts = 0;
        int hillIsVisibleInGame = 0;
        for (j = 0; j < game_state->my_count; ++j) {
            if (DistanceEuclidian(game_state->hills[i].row, game_state->hills[i].col, game_state->my_ants[j].row, game_state->my_ants[j].col, game_info) <= 5) {
                hillIsNearMyAnts = 1;
                break;
            }
        }
        for (j = 0; j < tempHillCnt; ++j) {
            if (tempHill[j].row == game_state->hills[i].row && tempHill[j].col == game_state->hills[i].col) {
                hillIsVisibleInGame = 1;
                break;
            }
        }
        if (hillIsVisibleInGame) {
            game_state->hills[i].razed = 0;
        } else {
            if (game_state->hills[i].player == '0') continue; // HACK: do not assign razed to own hill
            if (hillIsNearMyAnts) {
                game_state->hills[i].razed = 1;
            } // else retain whatever value it has
        }

        Print(DEBUG_HILL_RAZED, outFile, "Hill[%d] razed = %d\n", i, game_state->hills[i].razed);
    }
    game_state->my_hill_count = 0;
    for (i = 0; i < game_state->hill_count; ++i) {
        if (game_state->hills[i].razed) continue;
        if (game_state->hills[i].player == '0') ++game_state->my_hill_count;
    }
    if (my_old != 0) free(my_old);

    if (game_state->moveToTable == 0) {
        game_state->moveToTable = malloc(game_info->rows * game_info->cols * sizeof(int));
    }
    memset(game_state->moveToTable, 0, (game_info->rows * game_info->cols * sizeof(int)));

    if (game_state->enemyAttacksTable == 0) {
        game_state->enemyAttacksTable = malloc(game_info->rows * game_info->cols * sizeof(int));
    }
    memset(game_state->enemyAttacksTable, 0, (game_info->rows * game_info->cols * sizeof(int)));

    if (game_state->ownAttacksTable == 0) {
        game_state->ownAttacksTable = malloc(game_info->rows * game_info->cols * sizeof(int));
    }
    memset(game_state->ownAttacksTable, 0, (game_info->rows * game_info->cols * sizeof(int)));

    if (game_state->enemyIndexTable == 0) {
        game_state->enemyIndexTable = malloc(game_info->rows * game_info->cols * sizeof(int));
    }
    memset(game_state->enemyIndexTable, -1, (game_info->rows * game_info->cols * sizeof(int)));

    if (game_state->ownIndexTable == 0) {
        game_state->ownIndexTable = malloc(game_info->rows * game_info->cols * sizeof(int));
    }
    memset(game_state->ownIndexTable, -1, (game_info->rows * game_info->cols * sizeof(int)));

    for (i = 0; i < game_state->enemy_count; ++i) {
        FillTableWithPossibleEnemyAttacks(game_state, game_info, game_state->enemy_ants[i].row, game_state->enemy_ants[i].col);
        int offset = GetOffset(game_info, game_state->enemy_ants[i].row, game_state->enemy_ants[i].col);
        game_state->enemyIndexTable[offset] = i;
    }

    for (i = 0; i < game_state->my_count; ++i) {
        FillTableWithPossibleOwnAttacks(game_state, game_info, game_state->my_ants[i].row, game_state->my_ants[i].col);
        int offset = GetOffset(game_info, game_state->my_ants[i].row, game_state->my_ants[i].col);
        game_state->ownIndexTable[offset] = i;
    }

    if (DEBUG_ENEMYATTACKSTAB) {
        Print(DEBUG_ENEMYATTACKSTAB, outFile, "\n\n");
        for (i = 0; i < game_info->rows; ++i) {
            for (j = 0; j < game_info->cols; ++j) {
                int offset = GetOffset(game_info, i, j);
                int object = GetMapObjectByOffset(game_info, offset);
                if (object >= 'b' && object <= 'j') Print(DEBUG_ENEMYATTACKSTAB, outFile, "%c", object);
                else if (!game_state->enemyAttacksTable[offset]) Print(DEBUG_ENEMYATTACKSTAB, outFile, "%c", object);
                else Print(DEBUG_ENEMYATTACKSTAB, outFile, "%d", game_state->enemyAttacksTable[offset]);
            }
            Print(DEBUG_ENEMYATTACKSTAB, outFile, "\n");
        }
        Print(DEBUG_ENEMYATTACKSTAB, outFile, "\n\n");
    }
}

// Updates the map.
//
//    %   = Walls       (the official spec calls this water,
//                      in either case it's simply space that is occupied)
//    .   = Land        (territory that you can walk on)
//    a   = Your Ant
// [b..z] = Enemy Ants
// [A..Z] = Dead Ants   (disappear after one turn)
//    *   = Food
//    ?   = Unknown     (not used in latest engine version, unknowns are assumed to be land)

void InitMapData(char *data, game_info_t *game_info, game_state_t *game_state) {
    if (game_info->map == 0) {
        game_info->map = malloc(game_info->rows*game_info->cols);
        memset(game_info->map, '.', game_info->rows*game_info->cols);
    }

    int map_len = game_info->rows*game_info->cols;
    int i = 0;

    game_info->currturn += 1;
    Print(DEBUG_PRINT_FLAG, outFile, "Game Turn # = %d\n\n", game_info->currturn);

    for (; i < map_len; ++i) {
        if (game_info->map[i] != '%') {
            game_info->map[i] = '.';
        }
    }

    while (*data != 0) {
        char *tmp_data = data;

        //fprintf(outFile, "data: %s\n", GetLine(data));
        int arg = 0;

        while (*tmp_data != '\n') {
            if (*tmp_data == ' ') {
                *tmp_data = '\0';
                ++arg;
            }

            ++tmp_data;
        }

        char *tmp_ptr = tmp_data;
        tmp_data = data;

        tmp_data += 2;
        int jump = strlen(tmp_data) + 1;

        int row = atoi(tmp_data);
        int col = atoi(tmp_data + jump);
        char var3 = -1;

        if (arg > 2) {
            jump += strlen(tmp_data + jump) + 1;
            var3 = *(tmp_data + jump);
        }
        //fprintf(outFile, "var3: %d\n", var3);
        int offset = row*game_info->cols + col;

        switch (*data) {
            case 'w':
                game_info->map[offset] = '%';

                for (i = 0; i < map_len; ++i) {
                    char object = GetMapObjectByOffset(game_info, i);
                    if (object == '%') continue;
                }
                break;
            case 'a':
                game_info->map[offset] = var3 + 49;
                break;
            case 'd':
                game_info->map[offset] = '!';
                break;
            case 'f':
                game_info->map[offset] = '*';
                break;
            case 'h':
                game_info->map[offset] = var3;
                break;
        }

        data = tmp_ptr + 1;
    }
}
