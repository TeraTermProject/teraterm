#!/usr/bin/env ruby
# encoding: ASCII-8BIT

Encoding.default_external = "ASCII-8BIT" if RUBY_VERSION >= "1.9.0"

require 'timeout'
require 'kconv'

def rawio(t=nil, &blk)
  saved_mode = nil
  open("|stty -g") do |stty|
    saved_mode = stty.gets
  end
  begin
    system("stty raw -echo")
    if t
      return Timeout.timeout(t, &blk) 
    else
      return blk.call
    end
  ensure
    system("stty #{saved_mode}")
  end
end

def check_kcode
  begin
    rawio(1) do
      print "\r\xe6\xa4\xa3\xe6\x8e\xa7\e[6n\e[1K\r"
      buff = ""
      while c = STDIN.getc
	buff << c.chr
	if /(\x9c|\x1b\[)(\d+);(\d+)R/ =~ buff
	  case $3.to_i
	  when 5
	    $out_code = :toutf8
	  when 6
	    $out_code = :toeuc
	  when 7
	    $out_code = :tosjis
	  end
	  break
 	end
      end
    end
  rescue Timeout::Error
    $out_code = nil
  end
end

def msgout(msg)
  if $out_code
    puts msg.to_s.method($out_code).call
  else
    puts msg.to_s
  end
end

def getClipboard
  begin
    return rawio(1) do
      rdata = ""
      cbnum = ""

      print "\e]52;c;?\e\\"

      while (c = STDIN.getc)
	break if c.ord == 3 || c.ord == 4
	rdata << c
	if /(\e\]|\x9d)52;([cps0-7]+);/ =~ rdata
	  cbnum = $2
	  break
	end
      end

      rdata = ""
      if (cbnum != "")
	while (c = STDIN.getc)
	  break if c.ord == 3 || c.ord == 4
	  rdata << c
	  if /(\x9c|\x1b\\)/ =~ rdata
	    return $`.unpack("m")[0]
	    break
	  end
	end
      end
      nil
    end
  rescue Timeout::Error
    nil
  end
end

def setClipboard(data)
  printf "\e]52;c;#{[data].pack("m").chomp}\e\\"
end

def testClipboard
  def rwClipboard(data)
    msgout "クリップボードに \"#{data}\" をセットします。"
    setClipboard data

    msgout "クリップボードの内容を取得します。"
    cbdata = getClipboard

    if cbdata == data
      msgout "クリップボードへ設定した文字列と読み込んだ文字列が一致しました。"
      return :ok
    elsif cbdata == nil
      msgout "クリップボードの内容取得で、タイムアウトが発生しました。"
      msgout "クリップボードへのアクセスが許可されているか確認して下さい。"
      return :timeout
    else
      msgout "クリップボードへ設定した文字列と読み込んだ文字列が異なります。"
      msgout "set: #{data.inspect}"
      msgout "get: #{cbdata.inspect}"
      return :error
    end
  end

  check_kcode
  msgout "端末のクリップボードアクセスのテストを行います。"
  msgout "「設定」⇒「その他の設定」⇒「制御シーケンス」にある"
  msgout "「リモートからのクリップボードアクセス」を“読込/書込”にして下さい。"
  msgout "準備が出来たらリターンキーを押して下さい。"
  STDIN.gets

  rwClipboard "Clipboard Test"
  puts ""
  rwClipboard ""
end

case ARGV.shift
when "--set"
  setClipboard(ARGV.join(" "))
when "--get"
  if (cbdata = getClipboard)
    p cbdata
  else
    check_kcode
    msgout "クリップボードの内容取得で、タイムアウトが発生しました。"
  end
when "--test"
  testClipboard
else
  check_kcode
  msgout "使い方: #$0 [ --set | --get | --test ] [ パラメータ ]"
  msgout "  --set     パラメータに指定した文字列をクリップボードに設定する"
  msgout "  --get     クリップボードの内容を取得する"
  msgout "  --test    端末のクリップボードアクセス機能のテストを行う"
end
