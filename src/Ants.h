#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#if defined(_WIN32) || defined(_WIN64)
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

// TODO:
// -replace all row and col with a coordinate structure with x and y, more terse and saves on parameter passing

FILE* outFile;
#define DEBUG_PRINT_FLAG 0
#define DEBUG_PRINT_FLAG2 0
#define DEBUG_HILL_RAZED 0
#define DEBUG_DISTANCESCORE 0
#define DEBUG_ENEMYATTACKSTAB 0
#define DEBUG_GAMEINFO 0
#define DEBUG_TIME 1
#define DEBUG_DIFFUSION 0

static const int MaxValue = 10000000;
// TODO: to be tuned
static const int FoodVal = 32;
static const int HillVal = 32;
static const int EnemyVal = 5;
static const int DefenseVal = 32;
static const int ExploreVal = 32;
static const int TrailVal = 64;

enum DirType {
    NORTH = 0,
    EAST,
    WEST,
    SOUTH,
    MAXDIR
};

enum MoveType {
    FOOD = 0,
    ENEMYHILL,
    ENEMYANT,
    DEFENSE,
    EXPLORE,
    TRAIL,
    RANDOM,
    MAXMOVETYPE
};

static const char* MoveTypeStr[] = {
    "FOOD",
    "ENEMYHILL",
    "ENEMYANT",
    "DEFENSE",
    "EXPLORE",
    "TRAIL",
    "RANDOM"
};

typedef struct {
	int loadtime;
	int turntime;
	int rows;
	int cols;
	int turns;
	int currturn;
	int viewradius_sq;
	int attackradius_sq;
	int spawnradius_sq;
    int seed;
	char *map;
} game_info_t;

typedef struct {
    int row;
    int col;
    char player;
} basic_ant_t;

typedef struct {
    int id;
    int row;
    int col;
    int moved;
} my_ant_t;

typedef struct {
    int row;
    int col;
} food_t;

typedef struct {
    int row;
    int col;
    int razed;
    char player;
} hill_t;

typedef struct {
    int foodval;
    int exploreval;
    int hillval;
    int enemyval;
    int defenseval;
    int trailval; // TODO: try adding enemy trail to add to this
    int isVisible;
    int lastSeen;
} square_info_t;

typedef struct {
    my_ant_t *my_ants;
    basic_ant_t *enemy_ants;
    food_t *food;
    basic_ant_t *dead_ants;
    hill_t hills[50];
    int* moveToTable; // TODO: integrate to square_info_t
    int* enemyAttacksTable; // TODO: integrate to square_info_t
    int* ownAttacksTable; // TODO: integrate to square_info_t
    int* enemyIndexTable;  // unused
    int* ownIndexTable;
    int* foodLocations;  // unused
    square_info_t* mapValues;

    int my_count;
    int enemy_count;
    int food_count;
    int dead_count;
    int hill_count;
    int my_hill_count;

    int my_ant_index;
} game_state_t;


extern char *GetLine(char *text);
extern uint64_t GetTime(void);
extern void Print(int enabled, FILE *file, char *fmt, ...);
extern int DistanceEuclidian(int row1, int col1, int row2, int col2, game_info_t *Info);
extern int DistanceTaxicab(int row1, int col1, int row2, int col2, game_info_t *Info);
extern int DistanceEuclidianSquared(int row1, int col1, int row2, int col2, game_info_t *Info);
extern int GetOffset(game_info_t *Info, int row, int col);
extern int GetRowByOffset(game_info_t *Info, int row, int offset);
extern int GetColByOffset(game_info_t *Info, int col, int offset);
extern int GetOffsetByDir(game_info_t *Info, int row, int col, int dir);
extern char GetMapObjectByDir(game_info_t *Info, int row, int col, int dir);
extern char GetMapObjectByOffset(game_info_t *Info, int offset);
extern int MoveToDirectionSrcDest(int row1, int col1, int row2, int col2);
extern int MoveAwayDirectionSrcDest(int row1, int col1, int row2, int col2);
extern void GetNewCoordsByDir(int* r1, int* c1, const int row, const int col, const int dir, game_info_t *Info);
extern int GetNearestOwnHillIndex(game_state_t *Game, game_info_t *Info, int row, int col);
extern int GetBestDirByLowestVal(int table[]);
extern int GetBestDirByHighestVal(int table[], int* dir);
extern void FillTableWithPossibleEnemyAttacks(game_state_t* Game, game_info_t* Info, const int row, const int col);
extern void FillTableWithPossibleOwnAttacks(game_state_t* Game, game_info_t* Info, const int row, const int col);
extern int CountMyAntsInAttackRadius(game_state_t* Game, game_info_t* Info, const int row, const int col, int antOffset[], int dir);
extern int CountEnemyAntsInAttackRadius(game_state_t* Game, game_info_t* Info, const int row, const int col, int dir);
extern int DiffusionScoreIsGood(int scoreTable[]);
extern int UnDiffusionScoreIsGood(int scoreTable[]);
extern void DiffuseValuesToMap(game_state_t *Game, game_info_t *Info, const int row, const int col, int newValue, int type);
extern void InitAntsData(char *data, game_info_t *game_info);
extern void InitDiffusionValues(game_info_t *game_info, game_state_t *game_state);
extern void InitGameData(game_info_t *game_info, game_state_t *game_state);
extern void InitMapData(char *data, game_info_t *game_info, game_state_t *game_state);
extern void DoTurn(game_state_t *Game, game_info_t *Info);
