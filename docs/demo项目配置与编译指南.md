# Demo 项目配置与编译指南（demo_sle_uart）

## 1. 目录

```text
projects/demo_sle_uart/
├─ config/standard_tr5310_s.config
├─ sdk_overlay/custom/
│  ├─ CMakeLists.txt
│  ├─ Kconfig
│  └─ demo_sle_uart.c
└─ scripts/post_build.sh
```

## 2. 一键构建

```bash
cd /data/tr531x_proj
./tools/tr531x/build_project.sh --project demo_sle_uart --target standard-tr5310-s --patch-set default
```

## 3. 默认构建配置

```bash
./tools/tr531x/config_build_profile.sh
./tools/tr531x/build_project.sh
```

## 4. 产物位置

- SDK产物：`sdk_root_dir/output/tr5310/...`
- 项目导出产物：`projects/demo_sle_uart/output/standard-tr5310-s/`
