import subprocess
import re

# 编译 C++ 源码
compile_command = "mpic++ main.cpp -o main"
print("正在编译 C++ 程序...")
subprocess.run(compile_command, shell=True, check=True)

# 定义测试参数：进程数和矩阵规模（假设 m=n=k，即正方矩阵）
process_counts = [1, 2, 4, 8, 16]
matrix_sizes = [128, 256, 512, 1024, 2048]

# 用于存放测试结果，键为 (进程数, 矩阵规模)，值为耗时（秒）
results = {}

# 正则表达式模式，用于提取形如 "矩阵乘法计算耗时：0.123456 秒" 中的数字部分
pattern = re.compile(r"矩阵乘法计算耗时：([\d\.]+) 秒")

# 遍历所有组合进行测试
for p in process_counts:
    for size in matrix_sizes:
        # 构造输入：三个整数，均为矩阵规模（m, n, k）
        input_str = f"{size} {size} {size}\n"
        # --oversubscribe用来防止There are not enough slots available in the system to satisfy the 16 slots that were requested by the application
        command = f"mpirun --oversubscribe -np {p} ./main"
        print(f"\n运行测试：进程数 = {p}, 矩阵规模 = {size} x {size}")
        try:
            # 运行命令，同时传入输入字符串，捕获标准输出
            result = subprocess.run(command, input=input_str, text=True, shell=True, capture_output=True, check=True)
            output = result.stdout
            # 使用正则表达式匹配计算耗时
            match = pattern.search(output)
            if match:
                time_consumed = match.group(1)
            else:
                time_consumed = "N/A"
            results[(p, size)] = time_consumed
            print(f"测试结果：耗时 = {time_consumed} 秒")
        except subprocess.CalledProcessError as e:
            print(f"运行过程中发生错误：{e}")
            results[(p, size)] = "Error"

# 构造 Markdown 格式的结果表格
print("\n测试结果表格：\n")
header = "|进程数|"
for size in matrix_sizes:
    header += f"{size}|"
separator = "|" + " :-: |" * (len(matrix_sizes) + 1)
print(header)
print(separator)
for p in process_counts:
    row = f"|{p}|"
    for size in matrix_sizes:
        row += f"{results[(p, size)]}|"
    print(row)
