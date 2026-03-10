/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#ifndef RDK_WINDOW_MANAGER_LINUX_KEYS_H
#define RDK_WINDOW_MANAGER_LINUX_KEYS_H

#include <stdint.h>
#include <string>

#define WAYLAND_KEY_RESERVED            0
#define WAYLAND_KEY_ESC                 1
#define WAYLAND_KEY_1                   2
#define WAYLAND_KEY_2                   3
#define WAYLAND_KEY_3                   4
#define WAYLAND_KEY_4                   5
#define WAYLAND_KEY_5                   6
#define WAYLAND_KEY_6                   7
#define WAYLAND_KEY_7                   8
#define WAYLAND_KEY_8                   9
#define WAYLAND_KEY_9                   10
#define WAYLAND_KEY_0                   11
#define WAYLAND_KEY_MINUS               12
#define WAYLAND_KEY_EQUAL               13
#define WAYLAND_KEY_BACKSPACE           14
#define WAYLAND_KEY_TAB                 15
#define WAYLAND_KEY_Q                   16
#define WAYLAND_KEY_W                   17
#define WAYLAND_KEY_E                   18
#define WAYLAND_KEY_R                   19
#define WAYLAND_KEY_T                   20
#define WAYLAND_KEY_Y                   21
#define WAYLAND_KEY_U                   22
#define WAYLAND_KEY_I                   23
#define WAYLAND_KEY_O                   24
#define WAYLAND_KEY_P                   25
#define WAYLAND_KEY_LEFTBRACE           26
#define WAYLAND_KEY_RIGHTBRACE          27
#define WAYLAND_KEY_ENTER               28
#define WAYLAND_KEY_LEFTCTRL            29
#define WAYLAND_KEY_A                   30
#define WAYLAND_KEY_S                   31
#define WAYLAND_KEY_D                   32
#define WAYLAND_KEY_F                   33
#define WAYLAND_KEY_G                   34
#define WAYLAND_KEY_H                   35
#define WAYLAND_KEY_J                   36
#define WAYLAND_KEY_K                   37
#define WAYLAND_KEY_L                   38
#define WAYLAND_KEY_SEMICOLON           39
#define WAYLAND_KEY_APOSTROPHE          40
#define WAYLAND_KEY_GRAVE               41
#define WAYLAND_KEY_LEFTSHIFT           42
#define WAYLAND_KEY_BACKSLASH           43
#define WAYLAND_KEY_Z                   44
#define WAYLAND_KEY_X                   45
#define WAYLAND_KEY_C                   46
#define WAYLAND_KEY_V                   47
#define WAYLAND_KEY_B                   48
#define WAYLAND_KEY_N                   49
#define WAYLAND_KEY_M                   50
#define WAYLAND_KEY_COMMA               51
#define WAYLAND_KEY_DOT                 52
#define WAYLAND_KEY_SLASH               53
#define WAYLAND_KEY_RIGHTSHIFT          54
#define WAYLAND_KEY_KPASTERISK          55
#define WAYLAND_KEY_LEFTALT             56
#define WAYLAND_KEY_SPACE               57
#define WAYLAND_KEY_CAPSLOCK            58
#define WAYLAND_KEY_F1                  59
#define WAYLAND_KEY_F2                  60
#define WAYLAND_KEY_F3                  61
#define WAYLAND_KEY_F4                  62
#define WAYLAND_KEY_F5                  63
#define WAYLAND_KEY_F6                  64
#define WAYLAND_KEY_F7                  65
#define WAYLAND_KEY_F8                  66
#define WAYLAND_KEY_F9                  67
#define WAYLAND_KEY_F10                 68
#define WAYLAND_KEY_NUMLOCK             69
#define WAYLAND_KEY_SCROLLLOCK          70
#define WAYLAND_KEY_KP7                 71
#define WAYLAND_KEY_KP8                 72
#define WAYLAND_KEY_KP9                 73
#define WAYLAND_KEY_KPMINUS             74
#define WAYLAND_KEY_KP4                 75
#define WAYLAND_KEY_KP5                 76
#define WAYLAND_KEY_KP6                 77
#define WAYLAND_KEY_KPPLUS              78
#define WAYLAND_KEY_KP1                 79
#define WAYLAND_KEY_KP2                 80
#define WAYLAND_KEY_KP3                 81
#define WAYLAND_KEY_KP0                 82
#define WAYLAND_KEY_KPDOT               83
#define WAYLAND_KEY_102ND               86
#define WAYLAND_KEY_F11                 87
#define WAYLAND_KEY_F12                 88
#define WAYLAND_KEY_KPENTER             96
#define WAYLAND_KEY_RIGHTCTRL           97
#define WAYLAND_KEY_KPSLASH             98
#define WAYLAND_KEY_RIGHTALT            100
#define WAYLAND_KEY_HOME                102
#define WAYLAND_KEY_UP                  103
#define WAYLAND_KEY_PAGEUP              104
#define WAYLAND_KEY_LEFT                105
#define WAYLAND_KEY_RIGHT               106
#define WAYLAND_KEY_END                 107
#define WAYLAND_KEY_DOWN                108
#define WAYLAND_KEY_PAGEDOWN            109
#define WAYLAND_KEY_INSERT              110
#define WAYLAND_KEY_DELETE              111
#define WAYLAND_KEY_MUTE                113
#define WAYLAND_KEY_VOLUME_DOWN         114
#define WAYLAND_KEY_VOLUME_UP           115
#define WAYLAND_KEY_KPEQUAL             117
#define WAYLAND_KEY_KPPLUSMINUS         118
#define WAYLAND_KEY_PAUSE               119
#define WAYLAND_KEY_KPCOMMA             121
#define WAYLAND_KEY_LEFTMETA            125
#define WAYLAND_KEY_RIGHTMETA           126
#ifndef KEY_YELLOW
#define WAYLAND_KEY_YELLOW              0x18e
#endif
#ifndef KEY_BLUE
#define WAYLAND_KEY_BLUE                0x18f
#endif
#define WAYLAND_KEY_PLAYPAUSE           164
#define WAYLAND_KEY_REWIND              168
#ifndef KEY_RED
#define WAYLAND_KEY_RED                 0x190
#endif
#ifndef KEY_GREEN
#define WAYLAND_KEY_GREEN               0x191
#endif
#define WAYLAND_KEY_PLAY                207
#define WAYLAND_KEY_FASTFORWARD         208
#define WAYLAND_KEY_PRINT               210     /* AC Print */
#define WAYLAND_KEY_BACK                158
#define WAYLAND_KEY_MENU                139
#define WAYLAND_KEY_HOMEPAGE            172
#define WAYLAND_KEY_F13                 183
#define WAYLAND_KEY_F14                 184
#define WAYLAND_KEY_F15                 185
#define WAYLAND_KEY_F16                 186
#define WAYLAND_KEY_F17                 187
#define WAYLAND_KEY_F18                 188
#define WAYLAND_KEY_F19                 189
#define WAYLAND_KEY_F20                 190
#define WAYLAND_KEY_F21                 191
#define WAYLAND_KEY_F22                 192
#define WAYLAND_KEY_F23                 193
#define WAYLAND_KEY_F24                 194


#define RDK_WINDOW_MANAGER_FLAGS_SHIFT        8
#define RDK_WINDOW_MANAGER_FLAGS_CONTROL      16
#define RDK_WINDOW_MANAGER_FLAGS_ALT          32
#define RDK_WINDOW_MANAGER_FLAGS_COMMAND      64


#define RDK_WINDOW_MANAGER_KEY_BACKSPACE 8
#define RDK_WINDOW_MANAGER_KEY_TAB 	9
#define RDK_WINDOW_MANAGER_KEY_ENTER 	13
#define RDK_WINDOW_MANAGER_KEY_SHIFT 	16
#define RDK_WINDOW_MANAGER_KEY_CTRL 	17
#define RDK_WINDOW_MANAGER_KEY_ALT 	18
#define RDK_WINDOW_MANAGER_KEY_PAUSE 	19
#define RDK_WINDOW_MANAGER_KEY_CAPSLOCK 	20
#define RDK_WINDOW_MANAGER_KEY_ESCAPE 	27
#define RDK_WINDOW_MANAGER_KEY_SPACE 	32
#define RDK_WINDOW_MANAGER_KEY_PAGEUP 	33
#define RDK_WINDOW_MANAGER_KEY_PAGEDOWN 	34
#define RDK_WINDOW_MANAGER_KEY_END 	35
#define RDK_WINDOW_MANAGER_KEY_HOME 	36
#define RDK_WINDOW_MANAGER_KEY_LEFT 	37
#define RDK_WINDOW_MANAGER_KEY_UP 	38
#define RDK_WINDOW_MANAGER_KEY_RIGHT 	39
#define RDK_WINDOW_MANAGER_KEY_DOWN 	40
#define RDK_WINDOW_MANAGER_KEY_INSERT 	45
#define RDK_WINDOW_MANAGER_KEY_DELETE 	46
#define RDK_WINDOW_MANAGER_KEY_ZERO 	48
#define RDK_WINDOW_MANAGER_KEY_ONE 	49
#define RDK_WINDOW_MANAGER_KEY_TWO 	50
#define RDK_WINDOW_MANAGER_KEY_THREE 	51
#define RDK_WINDOW_MANAGER_KEY_FOUR 	52
#define RDK_WINDOW_MANAGER_KEY_FIVE 	53
#define RDK_WINDOW_MANAGER_KEY_SIX 	54
#define RDK_WINDOW_MANAGER_KEY_SEVEN 	55
#define RDK_WINDOW_MANAGER_KEY_EIGHT 	56
#define RDK_WINDOW_MANAGER_KEY_NINE 	57
#define RDK_WINDOW_MANAGER_KEY_A 	65
#define RDK_WINDOW_MANAGER_KEY_B 	66
#define RDK_WINDOW_MANAGER_KEY_C 	67
#define RDK_WINDOW_MANAGER_KEY_D 	68
#define RDK_WINDOW_MANAGER_KEY_E 	69
#define RDK_WINDOW_MANAGER_KEY_F 	70
#define RDK_WINDOW_MANAGER_KEY_G 	71
#define RDK_WINDOW_MANAGER_KEY_H 	72
#define RDK_WINDOW_MANAGER_KEY_I 	73
#define RDK_WINDOW_MANAGER_KEY_J 	74
#define RDK_WINDOW_MANAGER_KEY_K 	75
#define RDK_WINDOW_MANAGER_KEY_L 	76
#define RDK_WINDOW_MANAGER_KEY_M 	77
#define RDK_WINDOW_MANAGER_KEY_N 	78
#define RDK_WINDOW_MANAGER_KEY_O 	79
#define RDK_WINDOW_MANAGER_KEY_P 	80
#define RDK_WINDOW_MANAGER_KEY_Q 	81
#define RDK_WINDOW_MANAGER_KEY_R 	82
#define RDK_WINDOW_MANAGER_KEY_S 	83
#define RDK_WINDOW_MANAGER_KEY_T 	84
#define RDK_WINDOW_MANAGER_KEY_U 	85
#define RDK_WINDOW_MANAGER_KEY_V 	86
#define RDK_WINDOW_MANAGER_KEY_W 	87
#define RDK_WINDOW_MANAGER_KEY_X 	88
#define RDK_WINDOW_MANAGER_KEY_Y 	89
#define RDK_WINDOW_MANAGER_KEY_Z 	90
#define RDK_WINDOW_MANAGER_KEY_WINDOWKEY_LEFT	91
#define RDK_WINDOW_MANAGER_KEY_WINDOWKEY_RIGHT 	92
#define RDK_WINDOW_MANAGER_KEY_SELECT 	93
#define RDK_WINDOW_MANAGER_KEY_NUMPAD0 	96
#define RDK_WINDOW_MANAGER_KEY_NUMPAD1 	97
#define RDK_WINDOW_MANAGER_KEY_NUMPAD2 	98
#define RDK_WINDOW_MANAGER_KEY_NUMPAD3 	99
#define RDK_WINDOW_MANAGER_KEY_NUMPAD4 	100
#define RDK_WINDOW_MANAGER_KEY_NUMPAD5 	101
#define RDK_WINDOW_MANAGER_KEY_NUMPAD6 	102
#define RDK_WINDOW_MANAGER_KEY_NUMPAD7 	103
#define RDK_WINDOW_MANAGER_KEY_NUMPAD8 	104
#define RDK_WINDOW_MANAGER_KEY_NUMPAD9 	105
#define RDK_WINDOW_MANAGER_KEY_MULTIPLY 	106
#define RDK_WINDOW_MANAGER_KEY_ADD 	107
#define RDK_WINDOW_MANAGER_KEY_SUBTRACT 	109
#define RDK_WINDOW_MANAGER_KEY_DECIMAL 	110
#define RDK_WINDOW_MANAGER_KEY_DIVIDE 	111
#define RDK_WINDOW_MANAGER_KEY_F1 	112
#define RDK_WINDOW_MANAGER_KEY_F2 	113
#define RDK_WINDOW_MANAGER_KEY_F3 	114
#define RDK_WINDOW_MANAGER_KEY_F4 	115
#define RDK_WINDOW_MANAGER_KEY_F5 	116
#define RDK_WINDOW_MANAGER_KEY_F6 	117
#define RDK_WINDOW_MANAGER_KEY_F7 	118
#define RDK_WINDOW_MANAGER_KEY_F8 	119
#define RDK_WINDOW_MANAGER_KEY_F9 	120
#define RDK_WINDOW_MANAGER_KEY_F10 	121
#define RDK_WINDOW_MANAGER_KEY_F11 	122
#define RDK_WINDOW_MANAGER_KEY_F12 	123
#define RDK_WINDOW_MANAGER_KEY_NUMLOCK 	144
#define RDK_WINDOW_MANAGER_KEY_SCROLLLOCK 	145
#define RDK_WINDOW_MANAGER_KEY_SEMICOLON 	186
#define RDK_WINDOW_MANAGER_KEY_EQUALS 	187
#define RDK_WINDOW_MANAGER_KEY_COMMA 	188
#define RDK_WINDOW_MANAGER_KEY_DASH 	189
#define RDK_WINDOW_MANAGER_KEY_PERIOD 	190
#define RDK_WINDOW_MANAGER_KEY_FORWARDSLASH 	191
#define RDK_WINDOW_MANAGER_KEY_GRAVEACCENT 	192
#define RDK_WINDOW_MANAGER_KEY_OPENBRACKET 	219
#define RDK_WINDOW_MANAGER_KEY_BACKSLASH 	220
#define RDK_WINDOW_MANAGER_KEY_CLOSEBRACKET 	221
#define RDK_WINDOW_MANAGER_KEY_SINGLEQUOTE 	222
#define RDK_WINDOW_MANAGER_KEY_PRINTSCREEN 	44
#define RDK_WINDOW_MANAGER_KEY_FASTFORWARD 	223
#define RDK_WINDOW_MANAGER_KEY_REWIND 	224
#define RDK_WINDOW_MANAGER_KEY_PLAY 	226
#define RDK_WINDOW_MANAGER_KEY_PLAYPAUSE 	227

#define RDK_WINDOW_MANAGER_LEFTBUTTON       1
#define RDK_WINDOW_MANAGER_MIDDLEBUTTON     2
#define RDK_WINDOW_MANAGER_RIGHTBUTTON      4

#define RDK_WINDOW_MANAGER_MOD_SHIFT        8
#define RDK_WINDOW_MANAGER_MOD_CONTROL      16
#define RDK_WINDOW_MANAGER_MOD_ALT          32
#define RDK_WINDOW_MANAGER_MOD_COMMAND      64

#define RDK_WINDOW_MANAGER_KEYDOWN_REPEAT   128

#define RDK_WINDOW_MANAGER_KEY_YELLOW       403
#define RDK_WINDOW_MANAGER_KEY_BLUE         404
#define RDK_WINDOW_MANAGER_KEY_RED          405
#define RDK_WINDOW_MANAGER_KEY_GREEN        406

#define RDK_WINDOW_MANAGER_KEY_BACK         407
#define RDK_WINDOW_MANAGER_KEY_MENU         408
#define RDK_WINDOW_MANAGER_KEY_HOMEPAGE     409

#define RDK_WINDOW_MANAGER_KEY_MUTE         173
#define RDK_WINDOW_MANAGER_KEY_VOLUME_DOWN  174
#define RDK_WINDOW_MANAGER_KEY_VOLUME_UP    175

#define RDK_WINDOW_MANAGER_KEY_F13 	124
#define RDK_WINDOW_MANAGER_KEY_F14 	125
#define RDK_WINDOW_MANAGER_KEY_F15 	126
#define RDK_WINDOW_MANAGER_KEY_F16 	127
#define RDK_WINDOW_MANAGER_KEY_F17 	129
#define RDK_WINDOW_MANAGER_KEY_F18 	130
#define RDK_WINDOW_MANAGER_KEY_F19 	131
#define RDK_WINDOW_MANAGER_KEY_F20 	132
#define RDK_WINDOW_MANAGER_KEY_F21 	133
#define RDK_WINDOW_MANAGER_KEY_F22 	134
#define RDK_WINDOW_MANAGER_KEY_F23 	135
#define RDK_WINDOW_MANAGER_KEY_F24 	136


bool keyCodeFromWayland(uint32_t waylandKeyCode, uint32_t waylandFlags, uint32_t &mappedKeyCode, uint32_t &mappedFlags);
uint32_t keyCodeToWayland(uint32_t keyCode);
void mapNativeKeyCodes();
void mapVirtualKeyCodes();
bool keyCodeFromVirtual(std::string& virtualKey, uint32_t &mappedKeyCode, uint32_t &mappedFlags);
#endif //RDK_WINDOW_MANAGER_LINUX_KEYS_H
