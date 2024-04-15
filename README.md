# CS2 Cheat - C++

> Latest Update：2024.04.15 15:14:00

![external](./external.png)

## Features

- 方框透视
- 骨骼透视
- 玩家身体发光
- 地图扫描敌人雷达
- 实时显示剩余血条、玩家名字、玩家持有的武器
- 自瞄锁头并开枪射击
- 枪后坐力补偿
- 防闪光弹
- 连跳
- 跳越自动射击
- 玩家当前移动速度监控
- 玩家视野角度超广角
- 通过雷达实现敌人与墙体的遮挡可见性检测
- C4炸弹倒计时检测
- 绕过VAC反作弊系统(用户模式句柄劫持，非DMA和内核)

## Usage

See [Releases page](https://github.com/yinleiCoder/cs2-cheat-cpp/releases)  for decorated Changelogs. Reading the changelogs is a good way to keep up to date with the things `CS2CheatCPP` has to offer, and maybe will give you ideas of some features that you've been ignoring until now!

## Notices

- Change **LAUNCH OPTIONS**  with `-insecure` mode
- VisualStudio 2022
	- `Release` and Platform target `x64` because cs2 platform
	- Character Set `Use Multi-Byte Character Set`
	- Linker->Additional Dependencies `d3d11.lib`
	- Linker->System->SubSystem `Windows xxx`
	- VC++ Directories->Include Directories `dependencies/ImGui`
- Offsets:
	- [offsets](https://github.com/a2x/cs2-dumper/blob/main/output/offsets.hpp)
	- [client.dll](https://github.com/a2x/cs2-dumper/blob/main/output/client.dll.hpp)
- Packages:
	- [ImGui](https://github.com/ocornut/imgui)
	- [source2sdk](https://github.com/neverlosecc/source2sdk/tree/cs2/sdk)
- [Visual Key Code](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes)
- [unknowncheats](https://www.unknowncheats.me/forum/index.php)
- [句柄劫持绕过反作弊系统](https://github.com/Apxaey/Handle-Hijacking-Anti-Cheat-Bypass)
- and so on

## Thanks

![wechat](./wechat.jpg)
