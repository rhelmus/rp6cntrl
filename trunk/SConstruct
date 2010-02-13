# Based on: http://www.wiki.elektronik-projekt.de/mikrocontroller/avr/scons_avr

import os

rp6 = Environment(ENV = {'PATH' : os.environ['PATH']})

# Settings
# ---------------------------------------------
rp6lib = "../RP6Lib"
cpppath = [ rp6lib, rp6lib + "/RP6common" ]
cpppath_main = [ rp6lib + "/RP6base" ]
cpppath_control = [ rp6lib + "/RP6control" ]
opt = "-Os"

target_main = "main/main"
target_control = "m32/main-m32"

src_main = [
    "main/main.c",
]

src_control = [
    "m32/command.c",
    "m32/main-m32.c",
    "m32/interface.c",
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

# Apply settings
rp6['CC'] = cc
rp6.Append(CPPPATH = cpppath + cpppath_main + cpppath_control)
rp6.Append(CFLAGS = cflags + [ opt ])
rp6.Append(LINKFLAGS = ldflags)
rp6.Append(LIBS = libs)

# Build main
rp6.Program(target_main + ".elf", src_main + src_main_lib)

rp6.Command(target_main + ".hex", target_main + ".elf",
            "avr-objcopy -O %s -R .eeprom $SOURCE $TARGET" % format)

# Build control
rp6.Program(target_control + ".elf", src_control + src_control_lib)

rp6.Command(target_control + ".hex", target_control + ".elf",
            "avr-objcopy -O %s -R .eeprom $SOURCE $TARGET" % format)

# Sizes
rp6.Command(None, target_main + ".hex", "avr-size $SOURCE")
rp6.Command(None, target_control + ".hex", "avr-size $SOURCE")
