#ifndef CMD_H
#define CMD_H

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
typedef struct registered_cmd {
    const char *name;
    const char *description;
    void (*handler)(int argc, char *argv[]);
    struct registered_cmd *next;
} registered_cmd_t;

class CMD {
public:
    CMD();

    void argParse(int argc, char *argv[]);
    void registerCommand(const char *name,
                         const char *description,
                         void (*handler)(int argc, char *argv[]));

    void getArguments(int argc, char *argv[], std::vector<std::string> &args);
    
    registered_cmd_t *cmd_list;
private:
    

    

    char *p_getCmdOption(char **begin, char **end,
                         const std::string &option);
    bool p_cmdOptionExists(char **begin, char **end,
                           const std::string &option);

    registered_cmd_t *findCommand(const char *name);
};
#endif // CMD_H
