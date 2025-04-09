import subprocess
import re
import statistics
import numpy as np
import random
# 随机生成10个可解的二次方程组合
def generate_random_equation(type_of_roots):
    """根据指定的根类型生成随机二次方程系数
    type_of_roots: 'double' (判别式=0), 'real' (判别式>0), 'complex' (判别式<0)
    系数范围: 6位浮点数
    """
    # 确保a不为0
    a = random.uniform(1, 999999)  # 生成6位浮点数

    if type_of_roots == 'double':
        # 对于二重根，b^2 = 4ac
        # 先选择b，然后计算c
        b = random.uniform(-999999, 999999)  # 生成6位浮点数
        c = (b**2) / (4 * a)

        # 确保c在合理范围内
        attempts = 0
        while (abs(c) > 999999) and attempts < 50:
            b = random.uniform(-999999, 999999)
            c = (b**2) / (4 * a)
            attempts += 1

        # 如果找不到合适的值，则调整b使c在范围内
        if abs(c) > 999999:
            # 选择一个较小的b值
            b = random.uniform(-10000, 10000)
            c = (b**2) / (4 * a)

    elif type_of_roots == 'real':
        # 对于两个不同的实根，b^2 > 4ac
        b = random.uniform(-999999, 999999)
        # 确保判别式为正数且足够大以产生明显不同的根
        discriminant_target = random.uniform(1, 1000000)  # 目标判别式大小
        c = (b**2 - discriminant_target) / (4 * a)

        # 确保c在合理范围内
        if abs(c) > 999999:
            # 重新调整
            b = random.uniform(-100000, 100000)
            discriminant_target = random.uniform(1, 10000)
            c = (b**2 - discriminant_target) / (4 * a)

    else:  # 'complex'
        # 对于复根，b^2 < 4ac
        b = random.uniform(-100000, 100000)  # 使用较小范围以便找到合适的c
        # 确保判别式为负数
        discriminant_target = random.uniform(-1000000, -1)  # 目标判别式大小（负数）
        c = (b**2 - discriminant_target) / (4 * a)

        # 确保c在合理范围内
        if abs(c) > 999999 or c <= 0:  # c必须为正以确保判别式为负
            b = random.uniform(-10000, 10000)
            discriminant_target = random.uniform(-10000, -1)
            c = (b**2 - discriminant_target) / (4 * a)

            # 如果c仍然不合适，直接构造一个有效的c
            if c <= 0 or abs(c) > 999999:
                b = random.uniform(-1000, 1000)
                c = random.uniform(1, 10000)  # 确保c为正
                # 调整b以确保判别式为负
                discriminant = b**2 - 4*a*c
                if discriminant >= 0:
                    # 增大c使判别式为负
                    c = (b**2 + 1) / (3 * a)

    # 确保所有系数都在6位数范围内
    a = round(a, 6)
    b = round(b, 6)
    c = round(c, 6)

    return (a, b, c)

# 生成10个随机方程，包括不同类型的根
coefficients = []

# 生成3个二重根方程
for _ in range(3):
    coefficients.append(generate_random_equation('double'))

# 生成4个两个不同实根的方程
for _ in range(4):
    coefficients.append(generate_random_equation('real'))

# 生成3个复根方程
for _ in range(3):
    coefficients.append(generate_random_equation('complex'))

# 打乱顺序
random.shuffle(coefficients)

# 打印生成的方程系数及其类型
print("随机生成的10个二次方程组合:")
for a, b, c in coefficients:
    discriminant = b**2 - 4*a*c
    if abs(discriminant) < 1e-10:  # 浮点数比较需要容差
        root_type = "判别式为0，有一个二重根"
    elif discriminant > 0:
        root_type = "判别式大于0，有两个不同的实根"
    else:
        root_type = "判别式小于0，有两个共轭复根"
    print(f"({a:.6f}, {b:.6f}, {c:.6f}) - {root_type}")
# 要测试的程序及可执行文件名
implementations = {
    "顺序版": {
        "source": "quadratic_sequential.cpp",
        "binary": "quadratic_seq_exec",
        "coefficients": coefficients
    },
    "基本版": {
        "source": "quadratic_pthread.cpp",
        "binary": "quadratic_exec",
        "coefficients": coefficients
    }
}

# 匹配输出中的耗时信息
pattern = re.compile(r"求解耗时：([\d\.]+) 秒")

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
    compile_command = f"g++ -pthread {config['source']} -o {config['binary']} -O0 -lm -lrt"
    try:
        subprocess.run(compile_command, shell=True, check=True, capture_output=True)
    except subprocess.CalledProcessError as e:
        print(f"编译失败：{e}")
        print(get_error_details(e.stdout, e.stderr))
        continue

    # 对每组系数进行多次测试以获得更准确的时间
    num_runs = 5
    config['results'] = {}  # 存储当前实现的结果

    for coef_idx, (a, b, c) in enumerate(config['coefficients']):
        times = []

        for run in range(num_runs):
            # 输入格式：a b c
            input_str = f"{a} {b} {c}\n"
            command = f"./{config['binary']}"

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
                    time_consumed = float(match.group(1))
                    times.append(time_consumed)

                    # 只在第一次运行时打印方程和解
                    if run == 0:
                        # 提取方程和解的信息
                        equation_lines = output.strip().split('\n')
                        equation_info = '\n'.join(line for line in equation_lines if "方程" in line or "根" in line)
                        print(f"\n测试 {label}：方程 {a:.6f}x² + {b:.6f}x + {c:.6f} = 0")
                        print(equation_info)

            except subprocess.CalledProcessError as e:
                error_msg = f"进程退出码 {e.returncode}"
                error_details = get_error_details(e.stdout, e.stderr)
                print(f"运行出错：{error_msg}")
                if error_details:
                    print(error_details)
                times = [float('nan')] * num_runs
                break

        # 计算平均时间和标准差
        if times:
            avg_time = statistics.mean(times)
            std_time = statistics.stdev(times) if len(times) > 1 else 0.0
            config['results'][(a, b, c)] = (avg_time, std_time)
            print(f"平均耗时：{avg_time:.6f} 秒，标准差：{std_time:.6f} 秒")
        else:
            config['results'][(a, b, c)] = (float('nan'), float('nan'))

    # 打印结果表格
    print(f"\n{label} - 测试结果表格（单位：秒）\n")
    header = "|方程系数 (a, b, c)|平均耗时|标准差|"
    separator = "| :--- | ---: | ---: |"
    print(header)
    print(separator)

    for (a, b, c), (avg_time, std_time) in config['results'].items():
        row = f"|({a:.6f}, {b:.6f}, {c:.6f})|{avg_time:.9f}|{std_time:.9f}|"
        print(row)

# 比较两个版本的性能
if len(implementations) > 1:
    print("\n\n性能比较\n")
    header = "|方程系数 (a, b, c)|顺序版耗时|基本版耗时|基本版/顺序版|"
    separator = "| :--- | ---: | ---: | ---: |"
    print(header)
    print(separator)

    seq_results = implementations["顺序版"]["coefficients"]

    for i in range(len(seq_results)):
        coef = seq_results[i]
        a, b, c = coef

        # 从各个实现中获取结果
        seq_time = implementations["顺序版"]["results"].get((a, b, c), (float('nan'), float('nan')))[0]
        basic_time = implementations["基本版"]["results"].get((a, b, c), (float('nan'), float('nan')))[0]

        # Format the sequential time with 9 decimal places to check if it displays as 0.000000000
        seq_time_formatted = f"{seq_time:.9f}"

        # If sequential time is displayed as 0.000000000 or very close to zero, always show N/A for the ratio
        if seq_time_formatted == "0.000000000" or np.isnan(seq_time) or seq_time <= 0.000000001:
            row = f"|({a:.6f}, {b:.6f}, {c:.6f})|{seq_time:.9f}|{basic_time:.9f}|N/A|"
        else:
            basic_ratio = basic_time / seq_time if not np.isnan(basic_time) and basic_time > 0 else float('nan')

            if np.isnan(basic_ratio):
                row = f"|({a:.6f}, {b:.6f}, {c:.6f})|{seq_time:.9f}|{basic_time:.9f}|N/A|"
            else:
                row = f"|({a:.6f}, {b:.6f}, {c:.6f})|{seq_time:.9f}|{basic_time:.9f}|{basic_ratio:.2f}|"

        print(row)
