# Xposed-SGameHook

王者正式服/体验服自定义名片 体验服自定义海报 QQ群：746855036

## 功能说明
- 正式服/体验服自定义游戏名片（jpg格式）
- 体验服自定义游戏海报（png格式）
- 通过 Xposed 模块实现
- 在放置自定义名片或自定义海报文件后，请手动将对应文件的权限设置为777，以确保游戏能够正常读取文件。否则会出现文件无法读取的情况。
- [Github Release](https://github.com/huajiqaq/Xposed-SGameHook/releases/) 下的发布版本 `SGameHook.sh` 为辅助sh脚本，可输入监听路径，检测新文件自动替换自定义名片/海报文件。

## 文件说明
在游戏目录 `/data/data/gamepackage/files/` 下：
- `customtexjpg` - 自定义名片（jpg格式）
- `customtexpng` - 自定义海报（png格式）

## 安装说明
1. 安装 Xposed框架 例如Lsposed
2. 安装本模块 勾选游戏
3. 启动游戏即可生效

## 其他说明
1. 本模块不支持模拟器 模拟器请使用 Zygisk 模块 [Zygisk-SGameHook](https://github.com/huajiqaq/Zygisk-SGameHook/)