#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import subprocess
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import re
import pandas as pd
from datetime import datetime

def compile_cpp():
    """自动编译 montecarlo_pthread.cpp 文件"""
    cpp_file = "montecarlo_pthread.cpp"
    binary = "montecarlo_pthread"

    if not os.path.exists(cpp_file):
        print(f"错误：{cpp_file} 文件不存在，请确认 C++ 源代码文件已放在当前目录下。")
        sys.exit(1)

    compile_command = f"g++ {cpp_file} -o {binary} -pthread -O2"
    print("正在编译 C++ 代码...")
    try:
        result = subprocess.run(compile_command, shell=True, capture_output=True, text=True, check=True)
        print("编译成功。")
    except subprocess.CalledProcessError as e:
        print("编译失败：")
        print("stdout:", e.stdout)
        print("stderr:", e.stderr)
        sys.exit(1)

def run_cpp(total_points, thread_count):
    """运行 C++ 可执行程序，并返回其标准输出"""
    binary = "./montecarlo_pthread"
    run_command = f"{binary} {total_points} {thread_count}"
    print(f"正在运行命令：{run_command}")
    try:
        result = subprocess.run(run_command, shell=True, capture_output=True, text=True, check=True)
        print("运行 C++ 程序输出：")
        print(result.stdout)
    except subprocess.CalledProcessError as e:
        print("运行 C++ 程序出错：")
        print("stdout:", e.stdout)
        print("stderr:", e.stderr)
        sys.exit(1)

def read_points(filename):
    """读取 C++ 程序输出的采样点数据
    每行格式: x y in_circle
    """
    inside_x = []
    inside_y = []
    outside_x = []
    outside_y = []
    try:
        with open(filename, 'r') as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) != 3:
                    continue
                x = float(parts[0])
                y = float(parts[1])
                in_circle = int(parts[2])
                if in_circle == 1:
                    inside_x.append(x)
                    inside_y.append(y)
                else:
                    outside_x.append(x)
                    outside_y.append(y)
    except Exception as ex:
        print(f"读取文件 {filename} 失败：{ex}")
        sys.exit(1)
    return inside_x, inside_y, outside_x, outside_y

def plot_points(inside_x, inside_y, outside_x, outside_y, total_points, pi_estimate, thread_count, save_path=None):
    """绘制散点图，红色表示圆内点，蓝色表示圆外点"""
    # 设置支持中文的字体
    try:
        # 尝试使用系统中可能存在的中文字体
        chinese_fonts = ['SimHei', 'Microsoft YaHei', 'WenQuanYi Micro Hei', 'Noto Sans CJK SC', 'Noto Sans CJK JP', 'Droid Sans Fallback']
        font_found = False

        for font in chinese_fonts:
            if any(font.lower() in f.lower() for f in fm.findSystemFonts()):
                plt.rcParams['font.sans-serif'] = [font, 'DejaVu Sans']
                plt.rcParams['axes.unicode_minus'] = False
                font_found = True
                break

        if not font_found:
            # 如果没有找到中文字体，使用英文替代
            print("警告：未找到支持中文的字体，将使用英文显示")
            # 将标题中的中文替换为英文
            global use_english
            use_english = True
    except Exception as e:
        print(f"设置字体时出错: {e}")

    plt.figure(figsize=(6,6))
    plt.scatter(inside_x, inside_y, s=5, c='red', alpha=0.5, label='Inside Circle')
    plt.scatter(outside_x, outside_y, s=5, c='blue', alpha=0.5, label='Outside Circle')

    plt.xlim(0, 1)
    plt.ylim(0, 1)
    plt.xlabel('X')
    plt.ylabel('Y')
    if use_english:
        plt.title(f"Monte Carlo Pi Approximation\nSample Points: {total_points}, Threads: {thread_count}\nPi Estimate: {pi_estimate:.6f}")
    else:
        plt.title(f"Monte Carlo Pi Approximation\n采样点数: {total_points}, 线程数: {thread_count}\n估算 pi: {pi_estimate:.6f}")
    plt.legend()
    plt.tight_layout()

    if save_path:
        plt.savefig(save_path)
        print(f"图像已保存到: {save_path}")
    else:
        plt.show()

def extract_info_from_output(output_str):
    """从 C++ 程序的输出中提取估算的 pi 值和执行时间
       假设输出行形如：'估算的pi值: 3.141600' 和 '求解耗时: 0.123456 秒'
    """
    pi_estimate = None
    time_consumed = None

    for line in output_str.splitlines():
        if "估算的pi值" in line:
            # 提取冒号后面的数字
            try:
                parts = line.split(":")
                if len(parts) >= 2:
                    pi_estimate = float(parts[1].strip())
            except:
                pass
        elif "求解耗时" in line:
            # 提取执行时间
            try:
                match = re.search(r'求解耗时:\s*([0-9.]+)\s*秒', line)
                if match:
                    time_consumed = float(match.group(1))
            except:
                pass

    return pi_estimate, time_consumed

# 全局变量，用于控制是否使用英文替代中文
use_english = False

def run_simulation(total_points, thread_count, save_image=False, img_dir="img"):
    """运行一次蒙特卡洛模拟，并返回执行时间和pi估计值"""
    # 运行 C++ 程序
    binary = "./montecarlo_pthread"
    run_command = f"{binary} {total_points} {thread_count}"
    try:
        result = subprocess.run(run_command, shell=True, capture_output=True, text=True, check=True)
        cpp_output = result.stdout
        print(f"\n运行配置: 采样点数={total_points}, 线程数={thread_count}")
        print("C++ 程序输出:")
        print(cpp_output)
    except subprocess.CalledProcessError as e:
        print("运行 C++ 程序时出现错误：")
        print("stdout:", e.stdout)
        print("stderr:", e.stderr)
        return None, None

    # 从 C++ 输出中提取估算的 pi 值和执行时间
    pi_estimate, time_consumed = extract_info_from_output(cpp_output)
    if pi_estimate is None:
        pi_estimate = 0.0
    if time_consumed is None:
        time_consumed = 0.0

    # 如果需要保存图像
    if save_image:
        # 确保目录存在
        if not os.path.exists(img_dir):
            os.makedirs(img_dir)

        # 读取生成的点数据
        points_file = "montecarlo_points.txt"
        inside_x, inside_y, outside_x, outside_y = read_points(points_file)

        # 生成图像文件名
        img_filename = f"montecarlo_points_{total_points}_threads_{thread_count}.png"
        img_path = os.path.join(img_dir, img_filename)

        # 绘图并保存
        plot_points(inside_x, inside_y, outside_x, outside_y, total_points, pi_estimate, thread_count, save_path=img_path)

    return pi_estimate, time_consumed

def generate_markdown_table(results):
    """生成包含不同配置执行时间的markdown表格"""
    # 创建DataFrame
    df = pd.DataFrame(results)

    # 生成Markdown表格
    markdown = "| 采样点数 | 线程数 | Pi估计值 | 执行时间 (s) |\n"
    markdown += "| ------------ | ----------- | ----------- | ------------------ |\n"

    for _, row in df.iterrows():
        # 使用整数格式显示采样点数和线程数
        markdown += f"| {int(row['sample_points'])} | {int(row['thread_count'])} | {row['pi_estimate']:.6f} | {row['time_consumed']:.6f} |\n"

    return markdown

def main():
    # 1. 编译 C++ 程序
    compile_cpp()

    # 定义要测试的采样点数和线程数
    sample_points_list = [1024, 4096, 16384, 65536]
    thread_counts = [2, 4, 8]

    # 存储结果
    results = []

    # 运行所有配置
    for points in sample_points_list:
        # 对每个采样点数，只保存一次图像（使用4线程）
        for threads in thread_counts:
            save_img = (threads == 4)  # 只在线程数为4时保存图像
            pi_est, time_cons = run_simulation(points, threads, save_image=save_img)

            if pi_est is not None and time_cons is not None:
                results.append({
                    'sample_points': points,
                    'thread_count': threads,
                    'pi_estimate': pi_est,
                    'time_consumed': time_cons
                })

    # 生成markdown表格
    markdown_table = generate_markdown_table(results)
    print("\n生成的Markdown表格:")
    print(markdown_table)

    # 保存markdown表格到文件
    with open("montecarlo_results.md", "w") as f:
        f.write(markdown_table)
    print("\n结果已保存到 montecarlo_results.md")

if __name__ == "__main__":
    main()
