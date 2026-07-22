#include "tools/cmd.h"
// #include "sys/loader/ELF.h"
// #include "sys/loader/SELF.h"
#include "sys/loader/ELFV2.h"
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
void elfloader(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: load <path_to_elf>\n");
        return;
    }

    const char *elf_path = argv[1];
    ELFV2Loader V2ELF;
    LOGI("MAIN", "Loading ELF file: %s", elf_path);
    if (V2ELF.setPath(elf_path) != 0) {
        LOGE("MAIN", "Failed to set zELF file path: %s", elf_path);
        return;
    }
    LOGI("MAIN", "ELF file type: %s", V2ELF.isELF() ? "ELF" : (V2ELF.isSELF() ? "SELF" : "Unknown"));
    V2ELF.debugInfo();
    V2ELF.load();
    return;
}
// void selfloader(int argc, char *argv[])
// {
//     if (argc < 2)
//     {
//         printf("Usage: SELF_Test <path_to_self>\n");
//         return;
//     }
//     const char *self_path = argv[1];
//     SELFLoader loader;
//     LOGI("SELF", "Loading SELF file: %s", std::filesystem::path(self_path).filename().string().c_str());
//     loader.loadSELF(self_path);
// }
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

int main(int argc, char* argv[]) {
    LOGI("MAIN", "Starting Kyra - PS5 Translation Layer");
    cmd.registerCommand("help", "Displays this help message", helpHandler);
    // cmd.registerCommand("ELF_Test", "Test ELF loading", elfloader);
    // cmd.registerCommand("SELF_Test", "Test SELF loading", selfloader);
    cmd.registerCommand("load", "Load an ELF file", elfloader);
    cmd.registerCommand("APP_Data", "Get's the game's parameters", appdata);
    
    g_vmem = vmem_init();
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    cmd.argParse(argc, argv);
    // std::vector<std::string> args;
    // cmd.getArguments(argc, argv, args);
    // if (args.size() < 2) {

    // Pause for 10 seconds
    // sleep(10);
    LOGI("MAIN", "Exiting Kyra");
    vmem_free(g_vmem);
    return 0;
}
