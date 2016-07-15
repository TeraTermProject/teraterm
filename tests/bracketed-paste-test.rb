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

def getBracketedString
  bracket_type = nil
  rdata = ""
  begin
    return rawio(30) do
      pdata = nil

      c = STDIN.getc
      if c.ord == 3 || c.ord == 4
	return [:interrupt, rdata]
      end
      rdata << c

      # 一文字でも来たら残りのタイムアウトを 1 秒に変更する
      Timeout.timeout(1) do
	while (c = STDIN.getc)
	  if c.ord == 3 || c.ord == 4
	    return [:interrupt, rdata]
	  end
	  rdata << c
	  if /(?:\e\[|\x9b)20([01])~/ =~ rdata
	    pdata = $`
	    bracket_type = $1.to_i
	    break
	  end
	end

	case bracket_type
	when nil
	  # こないはずだけど
	  return [:interrupt, rdata]
	when 0
	  nil
	when 1
	  return [:nostart, pdata]
	else
	  # これもないはず
	  return [:invalid, bracket_type]
	end

	rdata = ""
	while (c = STDIN.getc)
	  if c.ord == 3 || c.ord == 4
	    return [:interrupt, rdata]
	  end
	  rdata << c
	  if /(\e\[|\x9b)201~/ =~ rdata
	    return [:ok, $`]
	  end
	end
      end
      [:noend, rdata]
    end
  rescue Timeout::Error
    case bracket_type
    when nil
      [:timeout, rdata]
    when 0
      [:noend, rdata]
    when 1
      [:nostart, rdata]
    else
      return [:invalid, bracket_type]
    end
  ensure
    print "\e[?2004l"
  end
end

def testBracketedPaste(msg, str, enableBracket)
  msgout msg
  setClipboard str
  print "\e[?2004h" if enableBracket
  msgout "貼り付け操作(マウス右ボタンクリックやAlt+v等)を行ってください。"
  result, data = getBracketedString

  case result
  when :ok
    if enableBracket
      if str == data
	msgout "結果: ○"
      else
	msgout "結果: × - 設定と応答が一致しません。受信文字列: \"#{data}\""
      end
    else
      if str == data
	msgout "結果: × - 非有効化なのに Bracket で囲まれています。"
      else
	msgout "結果: × - 非有効化なのに Bracket で囲まれています。設定と応答が一致しません。受信文字列: \"#{data}\""
      end
    end
  when :interrupt
    msgout "結果: × - 処理が中断されました。"
  when :invalid
    msgout "結果: × - 異常な状態です。"
  when :timeout
    if enableBracket
      if str == data
	msgout "結果: × - 応答が Bracket で囲まれていません。"
      else
	msgout "結果: × - タイムアウトしました。受信文字列: \"#{data}\""
      end
    else
      if str == data
	msgout "結果: ○"
      else
	msgout "結果: × - 設定と応答が一致しません。受信文字列: \"#{data}\""
      end
    end
  when :nostart
    msgout "結果: × - 開始 Bracket が来ないのに終了 Bracket が来ました。受信文字列: \"#{data}\""
  when :noend
    msgout "結果: × - 開始 Bracket は来ましたが終了 Bracket が来ませんでした。受信文字列: \"#{data}\""
  else
    msgout "結果: × - 異常な状態です。"
  end
  puts ""
end

check_kcode
msgout "Bracketed Paste Mode のテストを行います。"
msgout "「設定」⇒「その他の設定」⇒「制御シーケンス」にある"
msgout "「リモートからのクリップボードアクセス」を“書込のみ”または“読込/書込”にして下さい。"
msgout "準備が出来たらリターンキーを押して下さい。"
STDIN.gets

testBracketedPaste("テスト1: 通常", "Bracketed Paste Test", true)
testBracketedPaste("テスト2: 空文字列", "", true)
testBracketedPaste("テスト3: 非有効化", "Non-Bracketed Mode Test", false)
