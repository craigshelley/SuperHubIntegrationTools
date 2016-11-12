all:	extractmemorydump unpacksettings repacksettings disassemblesettings assemblesettings

extractmemorydump: extractmemorydump.c
	gcc extractmemorydump.c -o extractmemorydump

unpacksettings: unpacksettings.c
	gcc unpacksettings.c -o unpacksettings

repacksettings: repacksettings.c
	gcc repacksettings.c -o repacksettings

disassemblesettings: disassemblesettings.c
	gcc disassemblesettings.c -o disassemblesettings

assemblesettings: assemblesettings.c
	gcc assemblesettings.c -o assemblesettings

clean:
	rm assemblesettings disassemblesettings repacksettings unpacksettings extractmemorydump
