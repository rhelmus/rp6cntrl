#ifndef PLUGIN_H
#define PLUGIN_H

typedef void (*pluginFunc)(void);

typedef struct
{
    pluginFunc start;
    pluginFunc stop;
    pluginFunc think;
    const char *name;
} SPluginEntry;

void startPlugin(const char *plugin);
void stopPlugin(void);
void pluginThink(void);
void listPlugins(void);

// Defined in automatically generated plugins.c
extern SPluginEntry plugins[] PROGMEM;

#endif
