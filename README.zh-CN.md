# Zorix OS 0.7 Liquid Glass Live（实验版）

Zorix OS 0.7 是一个 x86-64、UEFI-only 的实验性 Live ISO。它继续使用 Debian/Linux 作为底层，同时加入 Zorix 自研的桌面、启动层、Swift/C 组件、Wine/GPU 管理工具，以及新的 Rust 源码层和大面积液态玻璃界面。

## ISO

- 文件名：`ZorixOS-0.7.iso`
- 大小：437,256,192 bytes（约 417 MiB）
- SHA-256：`18eee8b5137c6eabcfeaa6f526bd9613d7f9c1eda20faf774bb4cb3d08fca793`
- 架构：x86-64
- 固件：UEFI only
- Secure Boot：未验证；EFI loader 未签名
- Legacy BIOS：不支持
- 磁盘安装器：未包含
- 持久化：未包含

## 0.7 新增/加强

主要界面表面统一使用 Zorix 自己实现的 glass material：菜单栏、Dashboard、卡片、窗口、Dock、弹出提示与设置页面。加入多层透明与背景模糊、动态镜面高光与边缘折射感、指针跟随 optical lens、Aurora / Dawn / Midnight 动态背景、弹性窗口动画、Dock lift/bounce、Glass intensity 调节以及 Reduce Motion / Reduce Transparency / High Contrast / Large Text。

这是原创 CSS/JS 视觉实现，不包含 Apple 的 SwiftUI、UIKit、AppKit 或 Liquid Glass 框架/代码。

## Rust 状态

0.7 新增 `src/ZorixRuntime.rs`，实现无需第三方 crate 的 Linux /proc 读取与 JSON 输出。构建环境没有可用 `rustc`，所以最终 ISO 包含 Rust 源码和构建报告，但没有伪造“已编译 Rust 二进制”。

## 验证

构建时通过 50 项后台 API / 安全检查、35 项 Chromium 界面交互检查、7 条 rootfs 执行测试以及 34 项最终 ISO 独立结构与内容审计，总计 126 项。

这些检查不等于真实 UEFI 开机成功。真实开机、实体 GPU、Wi-Fi、声音、RTX、Wine 游戏与 Secure Boot 没有被算作通过。

## ISO 校验

```bash
sha256sum -c ZorixOS-0.7.iso.sha256
```

期望 SHA-256：

`18eee8b5137c6eabcfeaa6f526bd9613d7f9c1eda20faf774bb4cb3d08fca793`
