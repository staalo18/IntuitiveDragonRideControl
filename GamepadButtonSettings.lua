Gamepad button mappings:
Button Name - keyCode (buttonEvent->GetIDCode()) - Skyrim DXScanCode - Default QUserEvent (buttonEvent->QUserEvent()
 
These QUserEvents are fixed in IDRC- cannot be re-assigned in IDRC MCM Config
DPAD_UP - 1 - 266 - Favorites
DPAD_DOWN - 2 - 267 - Favorites
DPAD_LEFT - 4 - 268 - Hotkey1
DPAD_RIGHT - 8 - 269 - Hotkey2


These QUserEvents can be re-assigned in IDRC MCM config (ie swapped between each other):
LT - 9 - 280 - Left Attack/Block
RT - 10 - 281- Right Attack/Block
Start - 16 - 270 - Journal
Back - 32 - 271 - Wait
LS - 64 - 272 - Sneak
RS - 128 - 273 - Toggle POV
LB - 256 - 274 - Sprint
RB - 512 - 275 - Shout
A - 4096 - 276 - Activate
B - 8192 - 277 - Tween Menu
X - 16384 - 278 - Ready Weapon
Y - 32768 - 279 - Jump


DXScanCodes (from Wiki):
0x10A  266  DPAD_UP
0x10B  267  DPAD_DOWN
0x10C  268  DPAD_LEFT
0x10D  269  DPAD_RIGHT
0x10E  270  START
0x10F  271  BACK
0x110  272  LEFT_THUMB
0x111  273  RIGHT_THUMB
0x112  274  LEFT_SHOULDER
0x113  275  RIGHT_SHOULDER
0x114  276  A
0x115  277  B
0x116  278  X
0x117  279  Y
0x118  280  LT
0x119  281  RT
