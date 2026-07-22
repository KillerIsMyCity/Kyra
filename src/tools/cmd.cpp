#include "cmd.h"



void CMD::registerCommand(const char *name,
                          const char *description,
                          void (*handler)(int argc, char *argv[]))
{
    registered_cmd_t *cmd = new registered_cmd_t;

    cmd->name = name;
    cmd->description = description;
    cmd->handler = handler;
    cmd->next = nullptr;

    if (!cmd_list)
    {
        cmd_list = cmd;
        return;
    }

    registered_cmd_t *cur = cmd_list;

    while (cur->next)
        cur = cur->next;

    cur->next = cmd;
}

registered_cmd_t *CMD::findCommand(const char *name)
{
    registered_cmd_t *cur = cmd_list;

    while (cur)
    {
        if (strcmp(cur->name, name) == 0)
            return cur;

        cur = cur->next;
    }

    return nullptr;
}

void CMD::argParse(int argc, char *argv[])
{
    if (argc < 2)
    {
        return;
    }

    registered_cmd_t *cmd = findCommand(argv[1]);

    if (!cmd)
    {
        std::cerr << "Unknown command: " << argv[1] << "\n\n";
        return;
    }

    cmd->handler(argc - 1, &argv[1]);
}

void CMD::getArguments(int argc, char *argv[], std::vector<std::string> &args)
{
    for (int i = 0; i < argc; ++i)
    {
        args.push_back(std::string(argv[i]));
    }
}

CMD::CMD()
{
    cmd_list = nullptr;
}