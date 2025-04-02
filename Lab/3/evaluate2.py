import subprocess
import re

# 定义需要测试的程序及可执行文件名
implementations = {
    "Mutex聚合": {
        "source": "sum_pthread_mutex.cpp",
        "binary": "sum_mutex_exec",
        "thread_counts": [1, 2, 4, 8, 16],
        "array_sizes": [1000000, 4000000, 8000000, 16000000, 32000000, 64000000, 128000000]
    },
    "局部聚合": {
        "source": "sum_pthread_local.cpp",
        "binary": "sum_local_exec",
        "thread_counts": [1, 2, 4, 8, 16],
        "array_sizes": [1000000, 4000000, 8000000, 16000000, 32000000, 64000000, 128000000]
    }
}

# 匹配输出中的耗时信息
pattern = re.compile(r"计算耗时：([\d\.]+) 秒")

def get_error_details(output, error):
    error_details = []
    if error:
        error_details.append("错误输出：")
        error_details.append(error)
    if output:
        error_details.append("输出信息：")
        error_details.append(output)
    return "\n".join(error_details)

# 针对每种实现进行测试
for label, config in implementations.items():
    print(f"\n正在编译 {label} 版本...")
    compile_command = f"g++ -pthread {config['source']} -o {config['binary']} -O2"
    try:
        subprocess.run(compile_command, shell=True, check=True, capture_output=True)
    except subprocess.CalledProcessError as e:
        print(f"编译失败：{e}")
        continue

    results = {}

    for t in config['thread_counts']:
        for size in config['array_sizes']:
            # 输入格式：线程数 n
            input_str = f"{t} {size}\n"
            command = f"./{config['binary']}"
            print(f"\n运行 {label}：线程数 = {t}, 数组长度 = {size}")
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
    header = "|线程数|" + "|".join(str(size) for size in config['array_sizes']) + "|"
    separator = "|" + " :-: |" * (len(config['array_sizes']) + 1)
    print(header)
    print(separator)
    for t in config['thread_counts']:
        row = f"|{t}|"
        for size in config['array_sizes']:
            row += f"{results[(t, size)]}|"
        print(row)
