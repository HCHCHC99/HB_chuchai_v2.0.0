#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
C/C++ 代码行数统计工具
用法：
    1. 拖拽文件夹到此脚本图标上
    2. 或在命令行: python count_code_lines.py <文件夹路径>
    3. 直接双击运行，然后粘贴文件夹路径

统计指定文件夹下所有 .c 和 .h 文件的行数。
包含：总行数、非空行数、注释行数估算。
"""

import sys
import os
from pathlib import Path

# 解决 Windows CMD 中文/emoji 输出乱码问题
if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass


def count_lines(filepath: Path) -> tuple:
    """
    统计单个文件的行数。
    返回: (total_lines, non_empty_lines, comment_lines)
    """
    try:
        with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
            lines = f.readlines()
    except Exception as e:
        print(f"  [!] 无法读取: {filepath} - {e}")
        return (0, 0, 0)

    total = len(lines)
    non_empty = 0
    comments = 0
    in_block_comment = False

    for line in lines:
        stripped = line.strip()

        # 空行跳过
        if not stripped:
            continue

        non_empty += 1

        # 块注释追踪
        if in_block_comment:
            comments += 1
            if "*/" in stripped:
                in_block_comment = False
            continue

        # 块注释开始
        if stripped.startswith("/*"):
            comments += 1
            if "*/" not in stripped:
                in_block_comment = True
            continue

        # 单行注释
        if stripped.startswith("//"):
            comments += 1
            continue

        # 行尾注释（有实际代码 + 注释）
        if "//" in stripped or "/*" in stripped:
            comments += 1

    return (total, non_empty, comments)


def scan_folder(folder: Path) -> dict:
    """
    扫描文件夹，按子目录分组统计。
    返回: {dir_name: {files: [(name, rel, total, non_empty, comments)], totals: (total, non_empty, comments)}}
    """
    result = {}

    c_files = sorted(folder.rglob("*.c"))
    h_files = sorted(folder.rglob("*.h"))
    all_files = c_files + h_files

    for f in all_files:
        try:
            rel = f.relative_to(folder)
        except ValueError:
            rel = f
        parts = rel.parts
        if len(parts) == 1:
            group = "(根目录)"
        else:
            group = parts[0]

        if group not in result:
            result[group] = {"files": [], "totals": (0, 0, 0)}

        t, ne, cm = count_lines(f)
        result[group]["files"].append((f.name, rel, t, ne, cm))
        prev = result[group]["totals"]
        result[group]["totals"] = (prev[0] + t, prev[1] + ne, prev[2] + cm)

    return result


def format_num(n: int) -> str:
    """格式化数字，加千分位"""
    return f"{n:,}"


def main(folder: Path):
    folder = folder.resolve()

    if not folder.exists():
        print(f"[X] 文件夹不存在: {folder}")
        input("\n按回车键退出...")
        sys.exit(1)
    if not folder.is_dir():
        print(f"[X] 不是文件夹: {folder}")
        input("\n按回车键退出...")
        sys.exit(1)

    print("=" * 72)
    print(f"  C/C++ 代码行数统计")
    print(f"  路径: {folder}")
    print("=" * 72)

    data = scan_folder(folder)

    if not data:
        print("\n  未找到任何 .c 或 .h 文件。")
        input("\n按回车键退出...")
        return

    # 按目录打印
    grand_total = 0
    grand_non_empty = 0
    grand_comments = 0
    total_files = 0

    for group in sorted(data.keys()):
        info = data[group]
        t, ne, cm = info["totals"]
        grand_total += t
        grand_non_empty += ne
        grand_comments += cm
        total_files += len(info["files"])

        print(f"\n{'─' * 72}")
        print(f"  [{group}/]  ({len(info['files'])} 个文件, {format_num(t)} 行)")
        print(f"{'─' * 72}")
        for name, rel, ft, fne, fcm in info["files"]:
            bar = "#" * min(int(ft / 100) + 1, 30)
            print(f"  {bar}")
            print(f"  {name:40s} {format_num(ft):>8s} 行  (有效 {format_num(fne):>6s}, 注释 ~{fcm:>4d})")

    # 总计
    code_lines = grand_non_empty - grand_comments if grand_non_empty > grand_comments else grand_non_empty
    comment_pct = grand_comments * 100 // grand_total if grand_total > 0 else 0

    print(f"\n{'=' * 72}")
    print(f"  [汇总]")
    print(f"{'=' * 72}")
    print(f"  文件夹:          {folder.name}")
    print(f"  .c / .h 文件:    {total_files} 个")
    print(f"  总行数:          {format_num(grand_total)}")
    print(f"  非空行:          {format_num(grand_non_empty)}")
    print(f"  注释估算:        ~{format_num(grand_comments)}  ({comment_pct}%)")
    print(f"  有效代码行(估):  ~{format_num(code_lines)}")
    print(f"{'=' * 72}")

    # 最大的文件 TOP 10
    print(f"\n  [TOP 10] 行数最多的文件:")
    print(f"  {'─' * 58}")
    all_files_flat = []
    for g, info in data.items():
        for name, rel, t, ne, cm in info["files"]:
            all_files_flat.append((name, str(rel), t, ne, cm))
    all_files_flat.sort(key=lambda x: x[2], reverse=True)
    for i, (name, rel, t, ne, cm) in enumerate(all_files_flat[:10], 1):
        print(f"  {i:2d}. {name:45s} {format_num(t):>8s} 行  ({rel})")

    # 按文件类型统计
    c_total = sum(1 for f in folder.rglob("*.c"))
    h_total = sum(1 for f in folder.rglob("*.h"))
    print(f"\n  文件类型分布: .c = {c_total} 个, .h = {h_total} 个")

    print()
    input("按回车键退出...")


if __name__ == "__main__":
    print("C/C++ 代码行数统计工具")
    print("支持: .c / .h 文件")
    print()

    if len(sys.argv) > 1:
        folder_path = sys.argv[1]
        # 处理拖拽时可能带引号的情况
        folder_path = folder_path.strip('"').strip("'")
    else:
        folder_path = input("请输入文件夹路径（或拖拽文件夹到此处）: ").strip('"').strip("'")

    if not folder_path:
        # 默认使用当前脚本所在目录
        folder_path = os.path.dirname(os.path.abspath(__file__))
        print(f"未输入路径，使用当前目录: {folder_path}")

    main(Path(folder_path))
