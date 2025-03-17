#!/usr/bin/env python3
import subprocess
import re
import sys
m=1024
n=512
k=512
# 定义实验版本，每个实验包括版本号、描述、编译命令（如果有）和运行命令
experiments = [
    {
        "version": "1",
        "description": "Python",
        "compile": None,
        "run": f"python3 1.py {m} {n} {k}"
    },
    {
        "version": "2",
        "description": "C/C++（传统三重循环实现）",
        "compile": "g++ 2.cpp -O0 -o 2",
        "run": f"./2 {m} {n} {k}"
    },
    {
        "version": "3",
        "description": "调整循环顺序",
        "compile": "g++ 3.cpp -O0 -o 3",
        "run": f"./3 {m} {n} {k}"
    },
    {
        "version": "4",
        "description": "编译优化（-O1）",
        "compile": "g++ 3.cpp -O1 -o 4",
        "run": f"./4 {m} {n} {k}"
    },
    {
        "version": "5",
        "description": "循环展开（-O2）",
        "compile": "g++ 3.cpp -O2 -o 5",
        "run": f"./5 {m} {n} {k}"
    },
    {
        "version": "6",
        "description": "O3优化（-O3）",
        "compile": "g++ 3.cpp -O3 -o 6",
        "run": f"./6 {m} {n} {k}"
    },
    {
        "version": "7",
        "description": "OpenBLAS（4.cpp -lopenblas）",
        "compile": "g++ 4.cpp -O3 -o 7 -lopenblas",
        "run": f"./7 {m} {n} {k}"
    }
]

# 假设矩阵乘法需要的浮点运算次数
# 对于 m x k 和 k x n 的矩阵相乘，运算次数为 2*m*n*k
flops = 2 * m * n * k

device_peak_gflops = 843.8

def run_command(cmd):
    """
    执行命令，返回 (returncode, stdout, stderr)
    """
    try:
        result = subprocess.run(cmd, shell=True, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        return result.returncode, result.stdout, result.stderr
    except subprocess.CalledProcessError as e:
        return e.returncode, e.stdout, e.stderr

def extract_time(output):
    """
    从输出中提取 "Time taken: ... seconds" 的时间值
    """
    match = re.search(r"Time taken:\s*([\d\.]+)\s*seconds", output)
    if match:
        return float(match.group(1))
    else:
        return None

def main():
    results = []
    print("开始编译和运行各个版本……")
    for exp in experiments:
        print(f"\n=== 版本 {exp['version']}：{exp['description']} ===")
        # 如果有编译命令，则先编译
        if exp["compile"]:
            print("正在编译...")
            ret, out, err = run_command(exp["compile"])
            if ret != 0:
                print(f"编译失败，错误信息：\n{err}")
                sys.exit(1)
            else:
                print("编译成功。")
        # 运行命令
        print("正在运行...")
        ret, out, err = run_command(exp["run"])
        if ret != 0:
            print(f"运行失败，错误信息：\n{err}")
            sys.exit(1)
        # 打印完整输出（可选）
        print("程序输出：")
        print(out)
        # 提取运行时间
        t = extract_time(out)
        if t is None:
            print("未检测到 'Time taken' 输出。")
            t = 0.0
        else:
            print(f"检测到运行时间：{t} seconds")
        # 计算浮点性能（GFLOPS），如果运行时间有效则计算，否则为0
        if t > 0:
            gflops = flops / (t * 1e9)
        else:
            gflops = 0.0
        # 计算峰值性能百分比
        peak_percent = (gflops / device_peak_gflops) * 100 if device_peak_gflops > 0 else 0.0
        results.append({
            "version": exp["version"],
            "description": exp["description"],
            "time": t,
            "gflops": gflops,
            "peak_percent": peak_percent
        })

    # 找出最快的时间（非零值）
    valid_times = [r["time"] for r in results if r["time"] > 0]
    if not valid_times:
        print("没有检测到有效的运行时间数据。")
        sys.exit(1)
    fastest = min(valid_times)

    # 构造 Markdown 表格
    md_table = []
    header = "|版本|实现描述|运行时间(sec.)|相对加速比|绝对加速比|浮点性能(GFLOPS)|峰值性能百分比|"
    separator = "|---|---|---|---|---|---|---|"
    md_table.append(header)
    md_table.append(separator)

    # 获取版本1的运行时间（用于计算绝对加速比）
    version1_time = next((r["time"] for r in results if r["version"] == "1" and r["time"] > 0), None)

    for i, r in enumerate(results):
        # 计算相对加速比（相对于前一版本）
        rel_speedup = ""
        if r["time"] > 0:
            if i > 0 and results[i-1]["time"] > 0:  # 如果有前一个版本且时间有效
                rel_speedup = results[i-1]["time"] / r["time"]
            else:
                rel_speedup = 1.0  # 第一个版本的相对加速比为1

        # 计算绝对加速比（相对于版本1）
        abs_speedup = ""
        if r["time"] > 0 and version1_time is not None and version1_time > 0:
            abs_speedup = version1_time / r["time"]  # 使用版本1的时间计算绝对加速比

        # 格式化各项数据（时间和加速比保留6位小数，峰值百分比保留4位小数）
        time_str = f"{r['time']:.6f}" if r["time"] > 0 else ""
        rel_str = f"{rel_speedup:.6f}" if isinstance(rel_speedup, float) else ""
        abs_str = f"{abs_speedup:.6f}" if isinstance(abs_speedup, float) else ""
        gflops_str = f"{r['gflops']:.6f}" if r["time"] > 0 else ""
        peak_str = f"{r['peak_percent']:.4f}%" if r["time"] > 0 else ""
        row = f"|{r['version']}|{r['description']}|{time_str}|{rel_str}|{abs_str}|{gflops_str}|{peak_str}|"
        md_table.append(row)

    # 输出 Markdown 表格
    print("\n最终生成的 Markdown 表格：\n")
    md_result = "\n".join(md_table)
    print(md_result)

if __name__ == "__main__":
    main()
