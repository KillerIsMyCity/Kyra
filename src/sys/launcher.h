#ifndef LAUNCHER_H
#define LAUNCHER_H
// #include "loader/ELF.h"
// #include "loader/SELF.h"
#include "../external/json.hpp"

typedef struct {
    std::string game_name;
    std::string game_version;
    std::string titleId;
    std::string contentId;
} appParams_t;

class appParams {
    public:
        appParams(const char* path, appParams_t *params);
    private:
        appParams_t params;
};


#endif // LAUNCHER_H