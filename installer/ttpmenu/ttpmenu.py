
#
# Export Tera Term Menu registry to ini file.
#   with Python 2.7.6
#
# Usage:
# c:\>python ttpmenu.py > ttpmenu.ini
#

import _winreg
import sys
import struct

TTMREG = "Software\\ShinpeiTools\\TTermMenu";

write = sys.stdout.write

def enum_keys(key):
    try:
        i = 0
        while True:
            yield _winreg.EnumKey(key, i)
            i += 1
    except EnvironmentError:
        pass

def read_keys(key):
    values = []
    for name in enum_keys(key):
    	values.append(name)
    return values


def enum_values(key):
    try:
        i = 0
        while True:
            yield _winreg.EnumValue(key, i)
            i += 1
    except EnvironmentError:
        pass

def read_values(key):
    values = {}
    for name, value, type_ in enum_values(key):
    	write("%s=" % name)
    	if (type_ == _winreg.REG_DWORD):
    		write("%08x\n" % value)
    	
    	elif (type_ == _winreg.REG_BINARY):
    		for s in value:
    			bit = struct.unpack('B', s)[0]
    			write("%02x " % bit)
    		write("\n")
    	
    	else:
    		write("%s\n" % value)
    
    write("\n")
    return


def PrintSectionName(s):
	write("[%s]\n" % s)
	return

def ExportIniFile(regkey):
	key = _winreg.OpenKey(_winreg.HKEY_CURRENT_USER, regkey)
	read_values(key)
	_winreg.CloseKey(key)
	return

def main(s):
	PrintSectionName("TTermMenu")
	ExportIniFile(TTMREG)
	
	key = _winreg.OpenKey(_winreg.HKEY_CURRENT_USER, TTMREG)
	keylist = read_keys(key)
	_winreg.CloseKey(key)
#	print keylist

	for keystr in keylist:
		PrintSectionName(keystr)
		ExportIniFile(TTMREG + "\\" + keystr)
	
	return 0
	
if __name__ == '__main__':
	args = sys.argv
	if len(args) > 1:
		s = args[1:]
	else:
		s = ""
	main(s)


