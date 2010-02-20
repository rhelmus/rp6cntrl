#include "RP6ControlLib.h"
#include "plugin.h"

static SPluginEntry *activePlugin;

void startPlugin(const char *plugin)
{
    stopPlugin();
    
    uint8_t i;
    for (i=0; ; ++i)
    {
        if (!pgm_read_word(&plugins[i].name))
            break;
        
        if (!strcmp_P(plugin, (const char*)pgm_read_word(&plugins[i].name)))
        {
            activePlugin = &plugins[i];
            ((pluginFunc)pgm_read_word(&activePlugin->start))();
            return;
        }
    }

    writeString_P("No such plugin: ");
    writeString((char *)plugin);
    writeChar('\n');
}

void stopPlugin(void)
{
    if (activePlugin)
    {
        ((pluginFunc)pgm_read_word(&activePlugin->stop))();
        activePlugin = NULL;
    }
}

void pluginThink(void)
{
    if (activePlugin)
        ((pluginFunc)pgm_read_word(&activePlugin->think))();
}

void listPlugins(void)
{
    uint8_t i;
    for (i=0; ; ++i)
    {
        if (!pgm_read_word(&plugins[i].name))
            break;

        writeString_P("plugin: ");
        writeNStringP((const char *)pgm_read_word(&plugins[i].name));
        writeChar('\n');
    }
}
