#!/usr/bin/perl

$ESC=sprintf("%c", 0x1b);
$ST = $ESC . '(0';
$ED = $ESC . '(B';

for ($i=95;$i<=126;$i++) {
#	print $ESC.'(0'.$i."\n";
}
print $ST;
print "_\\abcdefghijklmnopqrstuvwxyz{|}~\n";
print "\n";
print "lqqqwqqqk\n";
print "x { x } x\n";
print "tqqqnqqqu\n";
print "x" . $ED . " A " . $ST . "x" . $ED . " % " . $ST . "x\n";
print "mqqqvqqqj\n";
print $ED;
