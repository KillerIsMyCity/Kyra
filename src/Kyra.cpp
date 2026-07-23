#include "tools/cmd.h"
#include "sys/loader/ELF.h"
#include "sys/launcher.h"
#include "sys/vmem.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

CMD cmd;

void helpHandler(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("Available commands:\n");
    registered_cmd_t *cur = cmd.cmd_list;

    while (cur)
    {
        printf("  %s: %s\n", cur->name, cur->description);
        cur = cur->next;
    }
}
void load(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: load <path_to_elf_or_self>\n");
        return;
    }

    LOGI("MAIN", "=== Kyra - PS5 Translation Layer ===");
    const char *file_path = argv[1];

    LOGI("MAIN", "Loading: %s", file_path);

    SELFLoader selfLoader;
    selfLoader.debugEnabled = true;
    elf_t* mainElf = nullptr;

    if (selfLoader.setPath(file_path) != 0) {
        LOGE("MAIN", "Failed to open file: %s", file_path);
        return;
    }

    if (selfLoader.isSELF()) {
        LOGI("MAIN", "File type: SELF");
        selfLoader.debugInfo();

        if (selfLoader.load() != 0) {
            LOGE("MAIN", "Failed to load SELF file");
            return;
        }
        LOGI("MAIN", "SELF loaded successfully");

        if (!selfLoader.self._elfs.empty()) {
            mainElf = &selfLoader.self._elfs.back();
        }
    } else {
        LOGI("MAIN", "File type: ELF (raw)");
        if (selfLoader.self.file.file) {
            fclose(selfLoader.self.file.file);
            selfLoader.self.file.file = nullptr;
        }

        ELFLoader elfLoader;
        elfLoader.debugEnabled = true;

        if (elfLoader.setPath(file_path) != 0) {
            LOGE("MAIN", "Failed to open ELF file: %s", file_path);
            return;
        }

        if (elfLoader.load() != 0) {
            LOGE("MAIN", "Failed to load ELF file");
            return;
        }
        LOGI("MAIN", "ELF loaded successfully");
        elfLoader.debugInfo();

        selfLoader.self._elfs.push_back(elfLoader.elf);
        mainElf = &selfLoader.self._elfs.back();
    }
    
    LOGI("MAIN", "Exiting Kyra");
    
    return;
}
void appdata(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: app_data <path_to_app_params>\n");
        return;
    }

    const char *app_params_path = argv[1];
    appParams_t params;
    appParams(app_params_path, &params);
    printf("Game Name: %s\n", params.game_name.c_str());
    printf("Game Version: %s\n", params.game_version.c_str());
    printf("Title ID: %s\n", params.titleId.c_str());
    printf("Content ID: %s\n", params.contentId.c_str());

}

void signalHandler(int signum) {
    (void)signum;
    printf("\n");
    if (g_vmem) {
        vmem_free(g_vmem);
        g_vmem = nullptr;
    }
    LOGI("MAIN", "Exiting Kyra due to signal %d", signum);
    _exit(EXIT_FAILURE);
}

void cmdReg(){
    cmd.registerCommand("help", "Displays this help message", helpHandler);
    cmd.registerCommand("load", "Load an ELF file", load);
    // cmd.registerCommand("APP_Data", "Get's the game's parameters", appdata);
    
}

int main(int argc, char* argv[]) {
    g_vmem = vmem_init();
    cmdReg();
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    cmd.argParse(argc, argv);
    
    // sleep(10);
    vmem_free(g_vmem);    
    return 0;
}


