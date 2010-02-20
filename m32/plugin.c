#include "RP6ControlLib.h"
#include "plugin.h"

static SPluginEntry *activePlugin;

inline uint8_t getEEPROMBindAddress(uint8_t key)
{
    return EEPROM_BIND_ADDR + (key-1);
}

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
    // Check if bound button was pressed
    const uint8_t key = checkReleasedKeyEvent();
    if (key)
    {
        // Check if the button is bound
        const uint8_t eepb = SPI_EEPROM_readByte(getEEPROMBindAddress(key));
        if (eepb != 255)
        {
            // Got a hit

            // Safety check in case index points out of bounds
            uint8_t i;
            for (i=0; i<eepb; ++i)
            {
                if (!pgm_read_word(&plugins[i].name))
                    break;
            }

            if (i == eepb)
            {
                // Toggle plugin state
                if (activePlugin != &plugins[i])
                {
                    stopPlugin();                    
                    activePlugin = &plugins[i];
                    ((pluginFunc)pgm_read_word(&activePlugin->start))();
                }
                else
                    stopPlugin();                
            }
        }
    }
    
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

void bindPlugin(uint8_t key, const char *plugin)
{
    if ((key < 1) || (key > 5))
        return;

    uint8_t i;
    for (i=0; ; ++i)
    {
        if (!pgm_read_word(&plugins[i].name))
            break;

        if (!strcmp_P(plugin, (const char*)pgm_read_word(&plugins[i].name)))
        {
            // Found plugin. Write plugin index to EEPROM
            SPI_EEPROM_writeByte(getEEPROMBindAddress(key), i);
            return;
        }
    }
}

void unBindKey(uint8_t key)
{
    if ((key < 1) || (key > 5))
        return;

    SPI_EEPROM_writeByte(getEEPROMBindAddress(key), 255);
}
