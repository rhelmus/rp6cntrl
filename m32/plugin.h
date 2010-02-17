#ifndef PLUGIN_H
#define PLUGIN_H

typedef struct
{
    void (*start)(void);
    void (*stop)(void);
    void (*think)(void);
    const char *name;
} SPluginEntry;

// Defined in automatically generated plugins.c
extern SPluginEntry plugins[] PROGMEM;

#endif
