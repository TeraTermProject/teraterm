# dump_code.pl

## 概要

- 0x00から0xFFを表示するプログラム。
- 端末の設定に合わせた文字を表示。
- C0(0x00-0x1F),C1(0x80-0x9F)は略号を表示。

## 実行

```
$ perl dump_code.pl
a   All
      all chars, 0x00...0xFF
      ISO/IEC 8859(=Unicode)
      ISO/IEC 2022 8bit
u   Unicode + UTF-8
      CES(character encoding scheme) = UTF-8
      U+0000...U+00FF
7   ISO/IEC 2022 7bit (jis)
s   Shift_JIS
e   Japanese/EUC (euc-jp)
j   JIS
q   quit
>
```
端末の設定に合わせて文字を入力すると文字表を表示。


例

JIS
```
> j
C0(0x00-0x1F) & GL(0x20-0x7F):
      | +0| +1| +2| +3| +4| +5| +6| +7| +8| +9| +a| +b| +c| +d| +e| +f|
------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---|
 0x0X |NUL|SOH|STX|ETX|EOT|ENQ|ACK|BEL|BS |HT |LF |VT |FF |CR |SO |SI |
 0x1X |DLE|DC1|DC2|DC3|DC4|NAK|SYN|ETB|CAN|EM |SUB|ESC|FS |GS |RS |US |
 0x2X |SP | ! | " | # | $ | % | & | ' | ( | ) | * | + | , | - | . | / |
 0x3X | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | : | ; | < | = | > | ? |
 0x4X | @ | A | B | C | D | E | F | G | H | I | J | K | L | M | N | O |
 0x5X | P | Q | R | S | T | U | V | W | X | Y | Z | [ | \ | ] | ^ | _ |
 0x6X | ` | a | b | c | d | e | f | g | h | i | j | k | l | m | n | o |
 0x7X | p | q | r | s | t | u | v | w | x | y | z | { | | | } | ~ |DEL|
C1(0x80-0x9F) & GR(0xA0-0xFF):
      | +0| +1| +2| +3| +4| +5| +6| +7| +8| +9| +a| +b| +c| +d| +e| +f|
------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---|
 0x8X |PAD|HOP|BPH|NBH|IND|NEL|SSA|ESA|HTS|HTJ|VTS|PLD|PLU|RI |SS2|SS3|
 0x9X |DCS|PU1|PU2|STS|CCH|MW |SPA|EPA|SOS|SGC|SCI|CSI|ST |OSC|PM |APC|
 0xaX |---| ｡ | ｢ | ｣ | ､ | ･ | ｦ | ｧ | ｨ | ｩ | ｪ | ｫ | ｬ | ｭ | ｮ | ｯ |
 0xbX | ｰ | ｱ | ｲ | ｳ | ｴ | ｵ | ｶ | ｷ | ｸ | ｹ | ｺ | ｻ | ｼ | ｽ | ｾ | ｿ |
 0xcX | ﾀ | ﾁ | ﾂ | ﾃ | ﾄ | ﾅ | ﾆ | ﾇ | ﾈ | ﾉ | ﾊ | ﾋ | ﾌ | ﾍ | ﾎ | ﾏ |
 0xdX | ﾐ | ﾑ | ﾒ | ﾓ | ﾔ | ﾕ | ﾖ | ﾗ | ﾘ | ﾙ | ﾚ | ﾛ | ﾜ | ﾝ | ﾞ | ﾟ |
 0xeX |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
 0xfX |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
```

SJIS
```
> s
      | +0| +1| +2| +3| +4| +5| +6| +7| +8| +9| +a| +b| +c| +d| +e| +f|
------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---|
 0x0X |NUL|SOH|STX|ETX|EOT|ENQ|ACK|BEL|BS |HT |LF |VT |FF |CR |SO |SI |
 0x1X |DLE|DC1|DC2|DC3|DC4|NAK|SYN|ETB|CAN|EM |SUB|ESC|FS |GS |RS |US |
 0x2X |SP | ! | " | # | $ | % | & | ' | ( | ) | * | + | , | - | . | / |
 0x3X | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | : | ; | < | = | > | ? |
 0x4X | @ | A | B | C | D | E | F | G | H | I | J | K | L | M | N | O |
 0x5X | P | Q | R | S | T | U | V | W | X | Y | Z | [ | \ | ] | ^ | _ |
 0x6X | ` | a | b | c | d | e | f | g | h | i | j | k | l | m | n | o |
 0x7X | p | q | r | s | t | u | v | w | x | y | z | { | | | } | ~ |DEL|
 0x8X |PAD|HOP|BPH|NBH|IND|NEL|SSA|ESA|HTS|HTJ|VTS|PLD|PLU|RI |SS2|SS3|
 0x9X |DCS|PU1|PU2|STS|CCH|MW |SPA|EPA|SOS|SGC|SCI|CSI|ST |OSC|PM |APC|
 0xaX |---| ｡ | ｢ | ｣ | ､ | ･ | ｦ | ｧ | ｨ | ｩ | ｪ | ｫ | ｬ | ｭ | ｮ | ｯ |
 0xbX | ｰ | ｱ | ｲ | ｳ | ｴ | ｵ | ｶ | ｷ | ｸ | ｹ | ｺ | ｻ | ｼ | ｽ | ｾ | ｿ |
 0xcX | ﾀ | ﾁ | ﾂ | ﾃ | ﾄ | ﾅ | ﾆ | ﾇ | ﾈ | ﾉ | ﾊ | ﾋ | ﾌ | ﾍ | ﾎ | ﾏ |
 0xdX | ﾐ | ﾑ | ﾒ | ﾓ | ﾔ | ﾕ | ﾖ | ﾗ | ﾘ | ﾙ | ﾚ | ﾛ | ﾜ | ﾝ | ﾞ | ﾟ |
 0xeX |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
 0xfX |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
```

ISO8859-1
```
> a
      | +0| +1| +2| +3| +4| +5| +6| +7| +8| +9| +a| +b| +c| +d| +e| +f|
------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---|
 0x0X |NUL|SOH|STX|ETX|EOT|ENQ|ACK|BEL|BS |HT |LF |VT |FF |CR |SO |SI |
 0x1X |DLE|DC1|DC2|DC3|DC4|NAK|SYN|ETB|CAN|EM |SUB|ESC|FS |GS |RS |US |
 0x2X |SP | ! | " | # | $ | % | & | ' | ( | ) | * | + | , | - | . | / |
 0x3X | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | : | ; | < | = | > | ? |
 0x4X | @ | A | B | C | D | E | F | G | H | I | J | K | L | M | N | O |
 0x5X | P | Q | R | S | T | U | V | W | X | Y | Z | [ | \ | ] | ^ | _ |
 0x6X | ` | a | b | c | d | e | f | g | h | i | j | k | l | m | n | o |
 0x7X | p | q | r | s | t | u | v | w | x | y | z | { | | | } | ~ |DEL|
 0x8X |PAD|HOP|BPH|NBH|IND|NEL|SSA|ESA|HTS|HTJ|VTS|PLD|PLU|RI |SS2|SS3|
 0x9X |DCS|PU1|PU2|STS|CCH|MW |SPA|EPA|SOS|SGC|SCI|CSI|ST |OSC|PM |APC|
 0xaX |   | ¡ | ¢ | £ | ¤ | ¥ | ¦ | § | ¨ | © | ª | « | ¬ | ­ | ® | ¯ |
 0xbX | ° | ± | ² | ³ | ´ | µ | ¶ | · | ¸ | ¹ | º | » | ¼ | ½ | ¾ | ¿ |
 0xcX | À | Á | Â | Ã | Ä | Å | Æ | Ç | È | É | Ê | Ë | Ì | Í | Î | Ï |
 0xdX | Ð | Ñ | Ò | Ó | Ô | Õ | Ö | × | Ø | Ù | Ú | Û | Ü | Ý | Þ | ß |
 0xeX | à | á | â | ã | ä | å | æ | ç | è | é | ê | ë | ì | í | î | ï |
 0xfX | ð | ñ | ò | ó | ô | õ | ö | ÷ | ø | ù | ú | û | ü | ý | þ | ÿ |
```
