#include "RP6ControlLib.h"
#include "plugin.h"

// UNDONE: Doesn't work! needs AVR prgm stuff
static SPluginEntry *activePlugin;

void stopPlugin(void);

void startPlugin(const char *plugin)
{
    stopPlugin();
    
    uint8_t i;
    for (i=0; plugins[i].name; ++i)
    {
        if (!strcmp(plugins[i].name, plugin))
        {
            activePlugin = &plugins[i];
            activePlugin->start();
            return;
        }
    }

    writeString_P("No such plugin: ");
    writeString(plugin);
    writeChar('\n');
}

void stopPlugin(void)
{
    if (activePlugin)
    {
        activePlugin->stop();
        activePlugin = NULL;
    }
}

void pluginThink(void)
{
    if (activePlugin)
        activePlugin->think();
}
