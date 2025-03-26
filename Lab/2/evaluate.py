import subprocess
import re
import os

# 允许root执行
os.environ["OMPI_ALLOW_RUN_AS_ROOT"] = "1"
os.environ["OMPI_ALLOW_RUN_AS_ROOT_CONFIRM"] = "1"

# 测试配置
process_counts = [1, 2, 4, 8, 16]
matrix_sizes = [128, 256, 512, 1024, 2048]
pattern = re.compile(r"矩阵乘法计算耗时：([\d\.]+) 秒")

# 要测试的程序及可执行文件名
implementations = {
    # "集合通信版本": {
    #     "source": "mpi_collective_matrix_multiplication.cpp",
    #     "binary": "collective_exec"
    # },
    "点对点通信版本": {
        "source": "mpi_point_to_point_matrix_multiplication.cpp",
        "binary": "p2p_exec"
    }
}

# 执行每个版本的测试
for label, config in implementations.items():
    print(f"\n正在编译 {label}...")
    compile_command = f"mpic++ {config['source']} -o {config['binary']}"
    subprocess.run(compile_command, shell=True, check=True)

    results = {}

    for p in process_counts:
        for size in matrix_sizes:
            input_str = f"{size} {size} {size}\n"
            command = f"mpirun --oversubscribe -np {p} ./{config['binary']}"
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
                print(f"运行出错：{e}")
                results[(p, size)] = "Error"

    # 打印结果表格
    print(f"\n{label} - 测试结果表格（单位：秒）\n")
    header = "|进程数|" + "|".join(str(s) for s in matrix_sizes) + "|"
    separator = "|" + " :-: |" * (len(matrix_sizes) + 1)
    print(header)
    print(separator)
    for p in process_counts:
        row = f"|{p}|"
        for size in matrix_sizes:
            row += f"{results[(p, size)]}|"
        print(row)
