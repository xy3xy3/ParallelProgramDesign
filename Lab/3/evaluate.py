import subprocess
import re
import os

# 要测试的程序及可执行文件名
implementations = {
    "行划分": {
        "source": "row_pthread.cpp",
        "binary": "row_exec",
        "thread_counts": [1, 2, 4, 8, 16],
        "matrix_sizes": [128, 256, 512, 1024, 2048]
    },
    "块划分": {
        "source": "block_pthread.cpp",
        "binary": "block_exec",
        "thread_counts": [1, 2, 4, 8, 16],
        "matrix_sizes": [128, 256, 512, 1024, 2048]
    }
}

pattern = re.compile(r"矩阵乘法计算耗时：([\d\.]+) 秒")

def get_error_details(output, error):
    error_details = []
    if error:
        error_details.append("错误输出：")
        error_details.append(error)
    if output:
        error_details.append("输出信息：")
        error_details.append(output)
    return "\n".join(error_details)

# 执行每个版本的测试
for label, config in implementations.items():
    print(f"\n正在编译 {label}...")
    compile_command = f"g++ -pthread {config['source']} -o {config['binary']} -O2"
    try:
        subprocess.run(compile_command, shell=True, check=True, capture_output=True)
    except subprocess.CalledProcessError as e:
        print(f"编译失败：{e}")
        continue

    results = {}

    for t in config['thread_counts']:
        for size in config['matrix_sizes']:
            # 输入格式：线程数 m n k
            input_str = f"{t} {size} {size} {size}\n"
            command = f"./{config['binary']}"
            print(f"\n运行 {label}：线程数 = {t}, 矩阵规模 = {size} x {size}")
            try:
                result = subprocess.run(
                    command,
                    input=input_str,
                    text=True,
                    shell=True,
                    capture_output=True,
                    check=True
                )
                output = result.stdout
                match = pattern.search(output)
                if match:
                    time_consumed = match.group(1)
                else:
                    time_consumed = "N/A"
                results[(t, size)] = time_consumed
                print(f"测试结果：耗时 = {time_consumed} 秒")
            except subprocess.CalledProcessError as e:
                error_msg = f"进程退出码 {e.returncode}"
                error_details = get_error_details(e.stdout, e.stderr)
                print(f"运行出错：{error_msg}")
                if error_details:
                    print(error_details)
                results[(t, size)] = f"Error: {error_msg}"

    # 打印结果表格
    print(f"\n{label} - 测试结果表格（单位：秒）\n")
    header = "|线程数|" + "|".join(str(s) for s in config['matrix_sizes']) + "|"
    separator = "|" + " :-: |" * (len(config['matrix_sizes']) + 1)
    print(header)
    print(separator)
    for t in config['thread_counts']:
        row = f"|{t}|"
        for size in config['matrix_sizes']:
            row += f"{results[(t, size)]}|"
        print(row)
