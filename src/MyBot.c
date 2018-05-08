#include "Ants.h"

// main, communicates with tournament engine

int main(int argc, char *argv[]) {
    int action = -1;

    game_info_t Info;
    game_state_t Game;
    Info.map = 0;

    memset(&Game, 0, sizeof(Game));
    memset(&Info, 0, sizeof(Info));

    outFile = fopen("debug.txt", "w+");

    while (42) {
        int initial_buffer = 100000;
        char *data = malloc(initial_buffer);
        memset(data, 0, initial_buffer);
        *data = '\n';
        char *ins_data = data + 1;
        int i = 0;
        while (1 > 0) {
            ++i;
            if (i >= initial_buffer) {
                initial_buffer *= 2;
                data = realloc(data, initial_buffer);
                ins_data = data + i;
                memset(ins_data, 0, initial_buffer - i);
            }

            *ins_data = getchar();

            if (*ins_data == '\n') {
                char *backup = ins_data;

                while (*(backup - 1) != '\n') {
                    --backup;
                }
                char *test_cmd = GetLine(backup);

                if (strcmp(test_cmd, "go") == 0) {
                    action = 0;
                    free(test_cmd);
                    break;
                }
                else if (strcmp(test_cmd, "ready") == 0) {
                    action = 1;
                    free(test_cmd);
                    break;
                }
                free(test_cmd);
            }
            ++ins_data;
        }

        if (action == 0) {
            char *skip_line = data + 1;
            while (*++skip_line != '\n');
            ++skip_line;

            uint64_t time = GetTime();
            Print(DEBUG_TIME, outFile, "Start Time:\n");
            InitMapData(skip_line, &Info, &Game);
            Print(DEBUG_TIME, outFile, "Time After Map: %d\n", GetTime() - time);
            InitGameData(&Info, &Game);
            InitDiffusionValues(&Info, &Game); // InitGameData should be called before this
            Print(DEBUG_TIME, outFile, "Time After Init: %d\n", GetTime() - time);
            DoTurn(&Game, &Info);
            Print(DEBUG_TIME, outFile, "Time After Turn: %d\n", GetTime() - time);
            fprintf(stdout, "go\n");
            fflush(stdout);
            fflush(outFile);
        }
        else if (action == 1) {
            InitAntsData(data + 1, &Info);
            Game.my_ant_index = -1;
            fprintf(stdout, "go\n");
            fflush(stdout);
        }

        free(data);
    }
    fclose(outFile);
}
