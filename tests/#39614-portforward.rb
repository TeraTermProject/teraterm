#!/usr/bin/ruby
# coding: UTF-8
#
# Ticket: #39614 のテストスクリプト
#
# Tera Term で /ssh-L8000:ttssh2.osdn.jp:80 を指定してサーバに接続後、
# このスクリプトを実行する。
# 再現しない場合は sleep の時間を適宜調整する
#

require 'socket'

s = TCPSocket.open("localhost", 8000)

puts "Send request."
s.write "GET /tmp/ticket-39614-test.txt HTTP/1.0\r\nHost: ttssh2.osdn.jp\r\n\r\n"

puts "Wait 5 seconds."
sleep 5

puts "Skip HTTP header"

while s.gets
  break if /^\r\n$/ =~ $_
end

count = 0

puts "Reading body part..."
while data = s.read(4096)
  count += data.size
  if ((count / 4096) % 64) == 0
    puts "#{count} bytes read"
    sleep 1
  end
end

s.close

result = ""
if count == 13229000
  result = "data OK."
elsif count < 13229000
  result = "data corrupted."
else
  result = "unknown data."
end

puts "END: #{count} bytes read. #{result}"
