#
# Export TeraTerm Menu registry to ini file.
#   with Python 2 & 3
#
# Usage:
# c:\>python ttpmenu.py > ttpmenu.ini
#

try:
	import _winreg				# Python 2
except ImportError:
	import winreg as _winreg	# Python 3
import sys
import struct
import ctypes

TTMREG = "Software\\ShinpeiTools\\TTermMenu"
changed = 0

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
		if value is not None:
			if (type_ == _winreg.REG_DWORD):
				write("%d\n" % ctypes.c_int32(value).value)
			elif (type_ == _winreg.REG_BINARY):
				for s in bytearray(value):
					write("%02X " % s)
				write("\n")
			else:
				write("%s\n" % value)
		else:
			write("\n")
	write("\n")
	return

def PrintSectionName(s):
	global changed
	if ']' in s:
		org = s
		# ] を削除。] 削除によりセクション名が重複する場合がある。
		s = s.replace(']', '')
		write("[%s] ; <- [%s]\n" % (s, org))
		changed = 1
	else:
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

	if changed == 1:
		print("; ']' has been removed. check the section name.", file=sys.stderr)
		
	return 0

if __name__ == '__main__':
	args = sys.argv
	if len(args) > 1:
		s = args[1:]
	else:
		s = ""
	main(s)
