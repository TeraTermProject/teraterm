#!/usr/bin/ruby

def bcetest(oscType, updateStr, setStr)
  print updateStr
  print "\e[J"
  print setStr
  puts "===== OSC #{oscType} test ====="
  print "=>"
  sleep 1

  print "\e[K EL"
  sleep 1

  print "\n\n"
  print updateStr
  puts "BCE Updated\n"
  print "=>"
  sleep 1

  print "\e[K EL"
  sleep 1

  print "\e[m\n\n"
end

print "\e[H\e[2J"

bcetest(4, "\e[48;5;17m", "\e]4;17;#550000\e\\")
bcetest(104, "\e[48;5;17m", "\e]104;17\e\\")
bcetest(11, "\e[m", "\e]11;#550000\e\\")
bcetest(111, "\e[m", "\e]111\e\\")

sleep 1
print "\e]104;200\e\\\e]111\e\\\e[m"
