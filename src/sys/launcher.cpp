#include "launcher.h"

using json = nlohmann::json;


appParams::appParams(const char* path, appParams_t *params) {
    if (std::filesystem::exists(path)){
        FILE *fd = fopen(path, "r");
        if (fd) {
            fseek(fd, 0, SEEK_END);
            size_t size = ftell(fd);
            fseek(fd, 0, SEEK_SET);

            char *buffer = new char[size + 1];
            fread(buffer, 1, size, fd);
            buffer[size] = '\0';

            json j = json::parse(buffer);
            // printf("DEbug JSON: %s\n", j.dump(4).c_str());
            params->game_name = j["localizedParameters"]["en-US"]["titleName"];
            params->game_version = j["contentVersion"];
            params->titleId = j["titleId"];
            params->contentId = j["contentId"];
            fclose(fd);
            delete[] buffer;
        }
        
    }
}