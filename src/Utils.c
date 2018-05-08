#include "Ants.h"

#define MAX(a,b) \
({ __typeof__ (a) _a = (a); \
__typeof__ (b) _b = (b); \
_a > _b ? _a : _b; })

#define MIN(a,b) \
({ __typeof__ (a) _a = (a); \
__typeof__ (b) _b = (b); \
_a < _b ? _a : _b; })

// returns the absolute value of a number; used in DistanceEuclidian function
int abs(int x) {
    if (x >= 0) return x;
    return -x;
}

// just a function that returns the string on a given line for i/o
// you don't need to worry about this

char *GetLine(char *text) {
    char *tmp_ptr = text;
    int len = 0;

    while (*tmp_ptr != '\n') {
        ++tmp_ptr;
        ++len;
    }

    char *return_str = malloc(len + 1);
    memset(return_str, 0, len + 1);

    int i = 0;
    for (; i < len; ++i) {
        return_str[i] = text[i];
    }

    return return_str;
}

/* returns time in milli-seconds */
uint64_t GetTime(void) {
    #if defined(_WIN32) || defined(_WIN64)
    static struct _timeb tv;
    _ftime(&tv);
    return(tv.time * 1000 + tv.millitm);
    #else
    static struct timeval tv;
    static struct timezone tz;
    gettimeofday (&tv, &tz);
    return(tv.tv_sec * 1000 + (tv.tv_usec / 1000));
    #endif
}

void Print(int enabled, FILE *file, char *fmt, ...) {
    if (enabled) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(file, fmt, ap);
        fflush(file);
        va_end(ap);
    }
}

// returns the DistanceEuclidian between two items on the grid accounting for map wrapping
int DistanceEuclidian(int row1, int col1, int row2, int col2, game_info_t *Info) {
    int dr, dc;
    int abs1, abs2;
    abs1 = abs(row1 - row2);
    abs2 = Info->rows - abs(row1 - row2);
    if (abs1 > abs2) dr = abs2;
    else dr = abs1;
    abs1 = abs(col1 - col2);
    abs2 = Info->cols - abs(col1 - col2);
    if (abs1 > abs2) dc = abs2;
    else dc = abs1;
    return sqrt(pow(dr, 2) + pow(dc, 2));
}

int DistanceTaxicab(int row1, int col1, int row2, int col2, game_info_t *Info) {
    int dr, dc;
    int abs1, abs2;
    abs1 = abs(row1 - row2);
    abs2 = Info->rows - abs(row1 - row2);
    if (abs1 > abs2) dr = abs2;
    else dr = abs1;
    abs1 = abs(col1 - col2);
    abs2 = Info->cols - abs(col1 - col2);
    if (abs1 > abs2) dc = abs2;
    else dc = abs1;
    return dr + dc;
}

int DistanceEuclidianSquared(int row1, int col1, int row2, int col2, game_info_t *Info) {
    int dr, dc;
    int abs1, abs2;
    abs1 = abs(row1 - row2);
    abs2 = Info->rows - abs(row1 - row2);
    if (abs1 > abs2) dr = abs2;
    else dr = abs1;
    abs1 = abs(col1 - col2);
    abs2 = Info->cols - abs(col1 - col2);
    if (abs1 > abs2) dc = abs2;
    else dc = abs1;
    return (dr * dr) + (dc * dc);
}

int GetOffset(game_info_t *Info, int row, int col) {
    return row*Info->cols + col;
}

int GetRowByOffset(game_info_t *Info, int row, int offset) {
    return (Info->rows + row + (offset % Info->rows)) % Info->rows;
}

int GetColByOffset(game_info_t *Info, int col, int offset) {
    return (Info->cols + col + (offset % Info->cols)) % Info->cols;
}

int GetOffsetByDir(game_info_t *Info, int row, int col, int dir) {
    assert(dir < MAXDIR);
    //Print(DEBUG_PRINT_FLAG2, outFile, "%s: orig row = %d, orig col = %d, offset = %d\n", __func__, row, col, row*Info->cols + col);
    switch (dir) {
        case NORTH: row = (Info->rows + row - 1) % Info->rows; break;
        case SOUTH: row = (Info->rows + row + 1) % Info->rows; break;
        case EAST: col = (Info->cols + col + 1) % Info->cols; break;
        case WEST: col = (Info->cols + col - 1) % Info->cols; break;
    }
    //Print(DEBUG_PRINT_FLAG2, outFile, "%s: updated row = %d, updated col = %d, offset = %d\n", __func__, row, col, row*Info->cols + col);
    return GetOffset(Info, row, col);
}

char GetMapObjectByDir(game_info_t *Info, int row, int col, int dir) {
    assert(dir < MAXDIR);
    return Info->map[GetOffsetByDir(Info, row, col, dir)];
}

char GetMapObjectByOffset(game_info_t *Info, int offset) {
    assert(offset < (Info->cols*Info->rows));
    return Info->map[offset];
}

// from --> to, returns direction
int MoveToDirectionSrcDest(int row1, int col1, int row2, int col2) {
    if (row1 > row2) {
        if (col1 > col2) return WEST;
        else if (col1 < col2) return EAST;
        else return NORTH;
    } else if (row1 < row2) {
        if (col1 > col2) return WEST;
        else if (col1 < col2) return EAST;
        else return SOUTH;
    } else {
        if (col1 > col2) return WEST;
        else if (col1 < col2) return EAST;
    }
    return rand() & 3;
}

// from <-- to, returns direction
int moveAwayDirection(int row1, int col1, int row2, int col2) {
    if (row1 > row2) {
        if (col1 > col2) return WEST;
        else if (col1 < col2) return EAST;
        else return SOUTH;
    } else if (row1 < row2) {
        if (col1 > col2) return WEST;
        else if (col1 < col2) return EAST;
        else return NORTH;
    } else {
        if (col1 > col2) return WEST;
        else if (col1 < col2) return EAST;
    }
    return rand() & 3;
}

void GetNewCoordsByDir(int* r1, int* c1, const int row, const int col, const int dir, game_info_t *Info) {
    *r1 = row;
    *c1 = col;
    switch (dir) {
        case NORTH: *r1 = (Info->rows + row - 1) % Info->rows; break;
        case SOUTH: *r1 = (Info->rows + row + 1) % Info->rows; break;
        case EAST: *c1 = (Info->cols + col + 1) % Info->cols; break;
        case WEST: *c1 = (Info->cols + col - 1) % Info->cols; break;
    }
}

int GetNearestOwnHillIndex(game_state_t *Game, game_info_t *Info, int row, int col) {
    int i, nearestDist = MaxValue;
    int nearestIdx = -1;
    Print(DEBUG_DISTANCESCORE, outFile, "Game->hill_count = %d\n", Game->hill_count);
    for (i = 0; i < Game->hill_count; ++i) {
        Print(DEBUG_DISTANCESCORE, outFile, "Game->hills[%d].razed = %d\n", i, Game->hills[i].razed);
        if (Game->hills[i].razed) continue;
        Print(DEBUG_DISTANCESCORE, outFile, "Game->hills[%d].player = %c\n", i, Game->hills[i].player);
        if (Game->hills[i].player != '0') continue;
        int DistanceEuclidian = DistanceEuclidianSquared(row, col, Game->hills[i].row, Game->hills[i].col, Info);
        if (DistanceEuclidian < nearestDist) {
             nearestIdx = i;
             nearestDist = DistanceEuclidian;
        }
    }
    return nearestIdx;
}

int GetBestDirByLowestVal(int table[]) {
    int bestScore = MaxValue;
    int i, dir = -1;
    for (i = 0; i < MAXDIR; ++i) {
        if (table[i] < bestScore) {
            bestScore = table[i];
            dir = i;
        }
    }
    if (dir != -1) table[dir] = MaxValue; // flag score as used so that it wouldn't be used again
    return dir;
}

int GetBestDirByHighestVal(int table[], int* dir) {
    int bestScore = -MaxValue;
    int i;
    *dir = -1;
    for (i = 0; i < MAXDIR; ++i) {
        if (table[i] > bestScore) {
            bestScore = table[i];
            *dir = i;
        }
    }
    if (*dir != -1) table[*dir] = -MaxValue; // flag score as used so that it wouldn't be used again
    return bestScore;
}

// TODO: replace hard coded data
void FillTableWithPossibleEnemyAttacks(game_state_t* Game, game_info_t* Info, const int row, const int col) {
    static const int colAdder[56] = {
        -1, 0, 1,
        -2, -1, 0, 1, 2,
        -3, -2, -1, 0, 1, 2, 3,
        -4, -3, -2, -1, 0, 1, 2, 3, 4,
        -4, -3, -2, -1, 1, 2, 3, 4,
        -4, -3, -2, -1, 0, 1, 2, 3, 4,
        -3, -2, -1, 0, 1, 2, 3,
        -2, -1, 0, 1, 2,
        -1, 0, 1};
    static const int rowAdder[56] = {
        -4, -4, -4,
        -3, -3, -3, -3, -3,
        -2, -2, -2, -2, -2, -2, -2,
        -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3,
        4, 4, 4};

    int i;
    for (i = 0; i < 56; ++i) {
        int newrow = (Info->rows + row + rowAdder[i]) % Info->rows;
        int newcol = (Info->cols + col + colAdder[i]) % Info->cols;
        int offset = GetOffset(Info, newrow, newcol);
        Game->enemyAttacksTable[offset]++;
    }
}

// TODO: replace hard coded data
void FillTableWithPossibleOwnAttacks(game_state_t* Game, game_info_t* Info, const int row, const int col) {
    static const int colAdder[56] = {
        -1, 0, 1,
        -2, -1, 0, 1, 2,
        -3, -2, -1, 0, 1, 2, 3,
        -4, -3, -2, -1, 0, 1, 2, 3, 4,
        -4, -3, -2, -1, 1, 2, 3, 4,
        -4, -3, -2, -1, 0, 1, 2, 3, 4,
        -3, -2, -1, 0, 1, 2, 3,
        -2, -1, 0, 1, 2,
        -1, 0, 1};
    static const int rowAdder[56] = {
        -4, -4, -4,
        -3, -3, -3, -3, -3,
        -2, -2, -2, -2, -2, -2, -2,
        -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3,
        4, 4, 4};
    int i;
    for (i = 0; i < 56; ++i) {
        int newrow = (Info->rows + row + rowAdder[i]) % Info->rows;
        int newcol = (Info->cols + col + colAdder[i]) % Info->cols;
        int offset = GetOffset(Info, newrow, newcol);
        Game->ownAttacksTable[offset]++;
    }
}

// TODO: replace hard coded data
int CountMyAntsInAttackRadius(game_state_t* Game, game_info_t* Info, const int row, const int col, int antOffset[], int dir) {
    static const int colAdder[MAXDIR][13] = {
        {-1,0,1,-2,2,-3,-2,2,3,-4,4,-4,4},
        {0,1,2,2,3,4,4,4,3,2,2,1,0},
        {0,-1,-2,-2,-3,-4,-4,-4,-3,-2,-2,-1,0},
        {-1,0,1,-2,2,-3,-2,2,3,-4,4,-4,4}};
    static const int rowAdder[MAXDIR][13] = {
        {-4,-4,-4,-3,-3,-2,-2,-2,-2,-1,-1,0,0},
        {-4,-4,-3,-2,-2,-1,0,1,2,2,3,4,4},
        {-4,-4,-3,-2,-2,-1,0,1,2,2,3,4,4},
         {4,4,4,3,3,2,2,2,2,1,1,0,0}};

    int i, count = 0;
    for (i = 0; i < 13; ++i) {
        int newrow = (Info->rows + row + rowAdder[dir][i]) % Info->rows;
        int newcol = (Info->cols + col + colAdder[dir][i]) % Info->cols;
        int offset = GetOffset(Info, newrow, newcol);
        int movetodir = MoveToDirectionSrcDest(newrow, newcol, row, col);
        int offset2 = GetOffsetByDir(Info, newrow, newcol, movetodir);
        char object = GetMapObjectByOffset(Info, offset2);
        if ((object == '.' || object == '*' || (object >= '0' && object <= '9')) && !Game->moveToTable[offset2]
        && Info->map[offset] == 'a' && !Game->my_ants[Game->ownIndexTable[offset]].moved) {
            antOffset[count++] = offset;
        }
    }
    return count;
}

// TODO: replace hard coded data
int CountEnemyAntsInAttackRadius(game_state_t* Game, game_info_t* Info, const int row, const int col, int dir) {
    static const int colAdder[MAXDIR][13] = {
        {-1,0,1,-2,2,-3,-2,2,3,-4,4,-4,4},
        {0,1,2,2,3,4,4,4,3,2,2,1,0},
        {0,-1,-2,-2,-3,-4,-4,-4,-3,-2,-2,-1,0},
        {-1,0,1,-2,2,-3,-2,2,3,-4,4,-4,4}};
    static const int rowAdder[MAXDIR][13] = {
        {-4,-4,-4,-3,-3,-2,-2,-2,-2,-1,-1,0,0},
        {-4,-4,-3,-2,-2,-1,0,1,2,2,3,4,4},
        {-4,-4,-3,-2,-2,-1,0,1,2,2,3,4,4},
         {4,4,4,3,3,2,2,2,2,1,1,0,0}};

    int i, count = 0;
    for (i = 0; i < 13; ++i) {
        int newrow = (Info->rows + row + rowAdder[dir][i]) % Info->rows;
        int newcol = (Info->cols + col + colAdder[dir][i]) % Info->cols;
        int offset = GetOffset(Info, newrow, newcol);
        if (Info->map[offset] >= 'b' && Info->map[offset] <= 'j') {
            count++;
        }
    }
    return count;
}

// at least 1 score is valid
int DiffusionScoreIsGood(int scoreTable[]) {
    int dir;
    for (dir = 0; dir < MAXDIR; ++dir) {
        if (scoreTable[dir] == 0) continue;
        else return 1;
    }
    return 0;
}

void DiffuseValuesToMap(game_state_t *Game, game_info_t *Info, const int row, const int col, int newValue, int type) {
    int offset, r1, c1, dir;
	char object;

    if (newValue <= 0) return;

    offset = GetOffset(Info, row, col);
    object = GetMapObjectByOffset(Info, offset);
    if (object == '%') return;

    switch (type) {
    case FOOD:
        if (newValue > Game->mapValues[offset].foodval)
        Game->mapValues[offset].foodval = newValue;
        else return;
        break;
    case ENEMYHILL:
        if (newValue > Game->mapValues[offset].hillval)
        Game->mapValues[offset].hillval = newValue;
        else return;
        break;
    case ENEMYANT:
        if (newValue > Game->mapValues[offset].enemyval)
        Game->mapValues[offset].enemyval = newValue;
        else return;
        break;
    case DEFENSE:
        if (newValue > Game->mapValues[offset].defenseval)
        Game->mapValues[offset].defenseval = newValue;
        else return;
        break;
    case EXPLORE:
        if (newValue > Game->mapValues[offset].exploreval)
        Game->mapValues[offset].exploreval = newValue;
        else return;
        break;
    }

    for (dir = 0; dir < MAXDIR; ++dir) {
        GetNewCoordsByDir(&r1, &c1, row, col, dir, Info);
        DiffuseValuesToMap(Game, Info, r1, c1, newValue-1, type);
    }
    return;
}

void UnDiffuseValuesToMap(game_state_t *Game, game_info_t *Info, const int row, const int col, int newValue, int type) {
    int offset, r1, c1, dir;
	char object;

    if (newValue <= 0) return;

    offset = GetOffset(Info, row, col);
    object = GetMapObjectByOffset(Info, offset);
    if (object == '%') return;

    switch (type) {
    case FOOD:
        Game->mapValues[offset].foodval = 0;
        break;
    case ENEMYHILL:
        Game->mapValues[offset].hillval = 0;
        break;
    case ENEMYANT:
        Game->mapValues[offset].enemyval = 0;
        break;
    case DEFENSE:
        Game->mapValues[offset].defenseval = 0;
        break;
    case EXPLORE:
        Game->mapValues[offset].exploreval = 0;
        break;
    }

    for (dir = 0; dir < MAXDIR; ++dir) {
        GetNewCoordsByDir(&r1, &c1, row, col, dir, Info);
        DiffuseValuesToMap(Game, Info, r1, c1, newValue-1, type);
    }
    return;
}
