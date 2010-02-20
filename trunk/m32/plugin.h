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
void bindPlugin(uint8_t key, const char *plugin);
void unBindKey(uint8_t key);

// Defined in automatically generated plugins.c
extern SPluginEntry plugins[] PROGMEM;

enum { EEPROM_BIND_ADDR = 0 }; // Starting address for EEPROM plugin binds

#endif
