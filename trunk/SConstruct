# Based on: http://www.wiki.elektronik-projekt.de/mikrocontroller/avr/scons_avr

import glob
import os

# Settings
# ---------------------------------------------
rp6lib = "../RP6Lib"
cpppath = [ rp6lib, rp6lib + "/RP6common" ]
cpppath_main = [ rp6lib + "/RP6base" ]
cpppath_control = [ rp6lib + "/RP6control" ]
opt = "-Os"

plugin_dir = "m32-plugins"

target_main = "main/main"
target_control = "m32/main-m32"

src_main = [
    "main/main.c",
]

src_control = [
    "m32/command.c",
    "m32/main-m32.c",
    "m32/interface.c",
    "m32-plugins/plugins.c",
]

src_main_lib = [
    rp6lib + "/RP6base/RP6RobotBaseLib.c",
    rp6lib + "/RP6common/RP6uart.c",
    rp6lib + "/RP6common/RP6I2CslaveTWI.c",
]

src_control_lib = [
    rp6lib + "/RP6control/RP6ControlLib.c",
    rp6lib + "/RP6common/RP6uart.c",
    rp6lib + "/RP6common/RP6I2CmasterTWI.c",
]

# ---------------------------------------------


# Internal settings
# ---------------------------------------------
mcu = "atmega32"
cc = "avr-gcc -mmcu=%s" % mcu
format = "ihex"
cflags = [
    "-std=gnu99",
    "-funsigned-char",
    "-funsigned-bitfields",
    "-fpack-struct",
    "-fshort-enums",
    "-Wall",
    "-Wstrict-prototypes",
    
    # Additional optims
    "-ffunction-sections",
    "-fdata-sections",
]
# Additional optims
ldflags = [ "-Wl,--gc-sections", "-Wl,--relax" ]
libs = [ "m" ]

# ---------------------------------------------

# Plugins
# ---------------------------------------------

src_control_plugins = [ ]
def collectPlugins(context):
    context.Message("Checking for m32 plugins...")
    # From http://bogdan.org.ua/2007/08/12/python-iterate-and-read-all-files-in-a-directory-folder.html
    for file in glob.glob(os.path.join(plugin_dir, "plugin_*.c")):
        src_control_plugins.append(file)

    f = open(plugin_dir + "/plugins.c", "w")
    
    f.write("""
/*  Automagicly created by SConstruct script.
    This file contains some glue code for the m32 plugins.
    Do not edit, any changes will be lost!
*/

#include <avr/pgmspace.h>

typedef struct
{
    void (*start)(void);
    void (*stop)(void);
    void (*think)(void);
    const char *name;
} SPluginEntry;

""")
    for file in src_control_plugins:
        name = os.path.splitext(os.path.basename(file))[0].replace("plugin_", "", 1)
        f.write("extern void %sStart(void);\n" % (name))
        f.write("extern void %sStop(void);\n" % (name))
        f.write("extern void %sThink(void);\n\n" % (name))

    f.write("""
SPluginEntry plugins[] PROGMEM =
{
""")
    
    for file in src_control_plugins:
        name = os.path.splitext(os.path.basename(file))[0].replace("plugin_", "", 1)
        f.write('   { &%sStart, &%sStop, &%sThink, "%s" }\n' % (name, name, name, name))

    f.write("};\n")
    
    f.close()
    
    context.Result(", ".join(src_control_plugins))
    return True
    

# ---------------------------------------------

# Build env
rp6 = Environment(ENV = {'PATH' : os.environ['PATH']})

# Apply settings
rp6['CC'] = cc
rp6.Append(CPPPATH = cpppath + cpppath_main + cpppath_control)
rp6.Append(CFLAGS = cflags + [ opt ])
rp6.Append(LINKFLAGS = ldflags)
rp6.Append(LIBS = libs)

# Collect plugins
if not rp6.GetOption('clean'):
    conf = Configure(rp6, custom_tests = { "collectPlugins" : collectPlugins })
    if not conf.collectPlugins():
        print "Failed to collect m32 plugins!"
        Exit(1)
    rp6 = conf.Finish()

# Build main
rp6.Program(target_main + ".elf", src_main + src_main_lib)

rp6.Command(target_main + ".hex", target_main + ".elf",
            "avr-objcopy -O %s -R .eeprom $SOURCE $TARGET" % format)

# Build control
rp6.Program(target_control + ".elf", src_control + src_control_plugins + src_control_lib)

rp6.Command(target_control + ".hex", target_control + ".elf",
            "avr-objcopy -O %s -R .eeprom $SOURCE $TARGET" % format)

# Sizes
rp6.Command(None, target_main + ".hex", "avr-size $SOURCE")
rp6.Command(None, target_control + ".hex", "avr-size $SOURCE")
