import subprocess
import re
import os
import signal

# 要测试的程序及可执行文件名
implementations = {
    # "行划分": {
    #     "source": "row_distributed.cpp",
    #     "binary": "row_exec",
    #     "process_counts": [1, 2, 4, 8, 16],
    #     "matrix_sizes": [128, 256, 512, 1024, 2048]
    # },
    "块划分": {
        "source": "block_distributed.cpp",
        "binary": "block_exec",
        "process_counts": [1, 4, 16],
        "matrix_sizes": [128, 256, 512, 1024, 2048]
    }
}

pattern = re.compile(r"矩阵乘法计算耗时：([\d\.]+) 秒")

def get_error_details(output, error):
    """从输出中提取错误详情"""
    error_details = []
    if error:
        error_details.append("错误输出：")
        error_details.append(error)
    if output:
        # 查找MPI错误信息
        mpi_errors = re.findall(r"\[.*?\] .*?(?=\n|$)", output)
        if mpi_errors:
            error_details.append("MPI错误信息：")
            error_details.extend(mpi_errors)
    return "\n".join(error_details)

# 执行每个版本的测试
for label, config in implementations.items():
    print(f"\n正在编译 {label}...")
    compile_command = f"mpic++ {config['source']} -o {config['binary']}"
    try:
        subprocess.run(compile_command, shell=True, check=True)
    except subprocess.CalledProcessError as e:
        print(f"编译失败：{e}")
        continue

    results = {}

    for p in config['process_counts']:
        for size in config['matrix_sizes']:
            input_str = f"{size} {size} {size}\n"
            command = f"OMPI_ALLOW_RUN_AS_ROOT=1 OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1 mpirun --oversubscribe -np {p} ./{config['binary']}"
            print(f"\n运行 {label}：进程数 = {p}, 矩阵规模 = {size} x {size}")
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
                results[(p, size)] = time_consumed
                print(f"测试结果：耗时 = {time_consumed} 秒")
            except subprocess.CalledProcessError as e:
                error_msg = f"进程退出码 {e.returncode}"
                if e.returncode < 0:
                    error_msg += f" (信号 {abs(e.returncode)})"
                error_details = get_error_details(e.stdout, e.stderr)
                print(f"运行出错：{error_msg}")
                if error_details:
                    print(error_details)
                results[(p, size)] = f"Error: {error_msg}"
            except Exception as e:
                error_msg = f"未知错误：{str(e)}"
                print(error_msg)
                results[(p, size)] = error_msg

    # 打印结果表格
    print(f"\n{label} - 测试结果表格（单位：秒）\n")
    header = "|进程数|" + "|".join(str(s) for s in config['matrix_sizes']) + "|"
    separator = "|" + " :-: |" * (len(config['matrix_sizes']) + 1)
    print(header)
    print(separator)
    for p in config['process_counts']:
        row = f"|{p}|"
        for size in config['matrix_sizes']:
            row += f"{results[(p, size)]}|"
        print(row)
