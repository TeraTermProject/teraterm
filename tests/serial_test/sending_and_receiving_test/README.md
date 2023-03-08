# Send/Receive test

Investigation losing serial data.

# Prepare

'testdata.bin' is reference data file.

- Run `perl create_testdata.pl` to create testdata.bin
- size is 32770 bytes
- SHA1
  - 66329d509a5f9331039d93abc5cf226c646ad582
   - `sha1sum testdata.bin`

# test1, Tera Term <-> Tera Term

- 2 serial ports connect using cross cable.
  - We can use the same PC or different PCs.
- Run Tera Term and save log file.
- Other Tera Term sends testdata.bin.
- compare log with testdata.bin.
  - with sha1sum or cmp

## checkpoint

- check send/receive for PC and TeraTerm
- check parameaters (speed, stopbit, parity, etc..)
- check flow control (RTS/CTS)

# test2, Tera Term <-> ttcomtester (test program)

`ttcomtester` is our test program.
This program can send back data as it is received.

- 2 serial ports connect using cross cable.
- start ttcomtester
  - `ttcomtester --device com2 -o "BuildComm=baud=57600 parity=N data=8 stop=1 octs=on rts=on" --verbose --test echo_rts`
- Start Tera Term and save log file.
- Send testdata.bin
- compare log with testdata.bin

## about ttcomtester

This test program is for sending/receiving tests.

### options

- `--device com2`
  - Specify com device
- BuildComm option (-o "BuildComm=baud=57600....")
  - BulidComm string is passed to BuildCommDCB() API.
  - See next URL about BuildCommDCB().
    - https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-buildcommdcba
- `--test echo`
  - simply echo test
- `--test echo_rts`
  - echo and RTS=0/1 every second

example:
- `ttcomtester --device com2 -o "BuildComm=baud=57600 parity=N data=8 stop=1 octs=on rts=on" --verbose --test echo`
- `ttcomtester --device com2 -o "BuildComm=baud=57600 parity=N data=8 stop=1 octs=on rts=on" --verbose --test echo_rts`
- `ttcomtester --device com2 -o "BuildComm=baud=57600 parity=N data=8 stop=1 octs=on rts=hs" --verbose --test echo`

### display

Show next charactors

- `r` or `R`
  - receive datas
- `s` or `S`
  - send datas
- `t`
  - RTS=0
- `T`
  - RTS=1

### key

`q` to quit.

# test3, device -> Tera Term

- Tera Term receives data and save log file.
- Device sends datas.
- If logic analyzer is available
  - Set trigger CTS(Tera term side) and check each lines.

# test4, Tera Term -> device

- Tera Term sends testdata.bin
- Device receives datas.
  - Device receive and check received data.
  - See program example.
- If logic analyzer is available
  - Set trigger RTS(Tera) and check each lines.

# Program Examples on device

This program is an example.
You need create send_1_bytes() and receive_1_byte(), etc.

## send on device

Sends the same data as 'testdata.bin'

    int i;
    int j;
    int c;
    for(i = 0; i < 32*1024/10; i++) {
        for(j = 0; j < 10; j++) {
            c = '0' + j;
            send_1_byte(c);
        }
    }

## receive on device

receive and check data.

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
