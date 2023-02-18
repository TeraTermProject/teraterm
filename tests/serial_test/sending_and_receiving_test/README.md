# Send/Receive test

Investigation losing serial data.

## Prepare

- testdata.bin
  - Run `perl create_testdata.pl` to create testdata.bin
  - size is 32770 bytes
  - SHA1
    - 66329d509a5f9331039d93abc5cf226c646ad582
    - `sha1sum testdata.bin`

## test1, Tera Term <-> Tera Term

- 2 serial ports connect using cross cable.
- Run Tera Term and save log file.
- Other Tera Term sends testdata.bin.
- compare log with testdata.bin.
  - with sha1sum or cmp

### checkpoint

- check send/receive for PC and TeraTerm
- check parameaters (speed, stopbit, parity, etc..)
- check flow control (RTS/CTS)

### tested environment

```
Tera Term --- FT232RL ---- FT232RL ---- Tera Term
```
[FT232RLx2.jpg](FT232RLx2.jpg)


## test2, Tera Term -> device

- Tera Term sends testdata.bin
- Device receives datas.
  - Device only receive and check received data.
  - See program example.
- If logic analyzer is available
  - Set trigger RTS and check each lines.

## test3, device -> Tera Term

- Tera Term receives data and save file.
- Device sends datas.
- If logic analyzer is available
  - Set trigger CTS and check each lines.

# Program Examples on device

## receive on device

    int i;
    int j;
    int c;
    for(i = 0; i < 32*1024/10; i++) {
        for(j = 0; j < 10; j++) {
            c = receive_1_byte();
            if (c != '0'+j) {
                // ERROR!
                assert(0);
            }
        }
    }
    // OK!

## send on device

    int i;
    int j;
    int c;
    for(i = 0; i < 32*1024/10; i++) {
        for(j = 0; j < 10; j++) {
            c = '0' + j;
            send_1_byte(c);
        }
    }
