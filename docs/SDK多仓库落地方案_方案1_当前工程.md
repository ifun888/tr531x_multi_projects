# SDK 多仓库落地方案（方案1，适配当前工程）

## 1. 目标与约束

当前工程：

```text
tr531x_proj/
├─ sdk_root_dir/            # SDK 源码（当前主源）
├─ common/                  # 公共代码
├─ projects/                # 项目层
├─ patches/                 # SDK patch
├─ tools/                   # 构建脚本
└─ docs/
```

目标：

1. SDK 与项目层分离；
2. 工程切换脚本化；
3. 默认构建参数可配置；
4. SDK 改动走 patch，可重放。

## 2. 分层规则

- L0 `sdk_root_dir/`：SDK 层，尽量不长期裸改。
- L1 `patches/sdk/sdk_root_dir/<patch_set>/`：补丁层。
- L2 `common/`：公共复用层。
- L3 `projects/<project>/`：项目业务层。
- L4 `tools/tr531x/`：构建编排层。

## 3. 关键机制

1. `application/samples/custom` 由 `switch_project.sh` 软链到项目 overlay。
2. 项目 config 注入 SDK：
   `build/config/target_config/<chip>/menuconfig/acore/<target_underline>.config`
3. 一键脚本完成 patch + switch + build + export + post。

## 4. 当前默认参数

- 默认目标：`standard-tr5310-s`
- 默认 patch-set：`default`
- 默认项目：`demo_sle_uart`
