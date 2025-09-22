# Message parser for Oregon messages coming from CUx Stick

Example Oregon message:
```
omAAACCB532CD55532CAAD5352D4D55534B534D53334C819
```

## Compilation
(devcontainer included as an environment)

```
make all
```


## Running the program
```
vscode ➜ /workspaces/cux-oregon-message-parser $ ./oregon_parser omAAACCB532CD55532CAAD5352D4D55534B534D53334C819
--- Stage 1: Pre-processing CUL Message ---
Raw Input: omAAACCB532CD55532CAAD5352D4D55534B534D53334C819
Preprocessor Output: 501A2D40FB1023404642A5

--- Stage 2: Parsing Oregon Data ---
Received Message: 501A2D40FB1023404642A5
Parsing... Bits: 80, Sensor Type ID: 0x1a2d
Found Sensor Definition: THGR228N
Checksum OK.
--- Decoded Sensor: THGR228N_fb_4 ---
  - Type: temperature
    Value: 23.10 C
  - Type: humidity
    Value: 64.00 %
    State: comfortable
  - Type: battery_status
    State: ok
---------------------------------------
```