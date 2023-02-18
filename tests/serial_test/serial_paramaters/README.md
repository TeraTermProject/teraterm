# Serial paramater

Special serial parameters for flow control investigations.
Please refer [block_diagram_serial.png](block_diagram_serial.png).

```
[Tera Term]
Serial.AppCTSDSRFlow = 0
```

AppCTSDSRFlow = 0
 Same as before (make a request to the OS)

AppCTSDSRFlow = 1
 Tera Term check the RTS line to stop/resume requests to the OS.

```
[Tera Term]
Serial.RecieveBufferSize = 8192
Serial.SendBufferSize = 2048
```

Send and receive buffer size.
When the receive buffer is increased, data will not be lost.

