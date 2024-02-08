# CS2游戏外挂(C++)

> 最近更新：2024年2月8日

## Features

- 方框透视
- 骨骼透视
- 绕过VAC防作弊系统

## Notices

- Change **LAUNCH OPTIONS**  with `-insecure` mode
- VisualStudio 2022
	- `Release` and Platform target `x64` because cs2 platform
	- Character Set `Use Multi-Byte Character Set`
	- Linker->Additional Dependencies `d3d11.lib`
	- Linker->System->SubSystem `Windows xxx`
	- VC++ Directories->Include Directories `dependencies/ImGui`
- Offsets:
	- [offsets](https://github.com/a2x/cs2-dumper/blob/main/generated/offsets.hpp)
	- [client.dll](https://github.com/a2x/cs2-dumper/blob/main/generated/client.dll.hpp)
- Packages:
	- [ImGui](https://github.com/ocornut/imgui)
- [Visual Key Code](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes)
- and so on
