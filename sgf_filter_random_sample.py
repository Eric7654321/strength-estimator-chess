import os
import random
import shutil
import re
from concurrent.futures import ThreadPoolExecutor
from tqdm import tqdm

# ================= 參數設定 =================
min_elo = 600
max_elo = 3000
interval = 100
lines_per_file = 100000
# ==========================================

input_dir = "training_sgf"


def getRank(elo):
    if elo < min_elo or elo >= max_elo:
        return -1
    return (elo - min_elo) // interval


def process_file(file_name):
    filtered_lines = []
    total_ranks = (max_elo - min_elo) // interval

    # 根據參數動態生成資料夾名稱
    base_output_name = f"rank_{lines_per_file}_{min_elo}_{max_elo}_{interval}interval"

    for i in range(total_ranks):
        filtered_lines.append([])

    # 預先建立好所有資料夾
    for i in range(min_elo, max_elo, interval):
        os.makedirs(f"{base_output_name}/train/sgf_{i}_{i + interval}", exist_ok=True)
        os.makedirs(f"{base_output_name}/test_origin/sgf_{i}_{i + interval}", exist_ok=True)

    input_file = os.path.join(input_dir, file_name)
    print(f"------start {file_name}------")

    with open(input_file, "r", encoding="utf-8") as f_in:
        for line in tqdm(f_in):
            wr_match = re.search(r"WR\[(\d+)\]", line)
            br_match = re.search(r"BR\[(\d+)\]", line)
            if wr_match and br_match:
                wr_rating = int(wr_match.group(1))
                br_rating = int(br_match.group(1))
                if wr_rating >= min_elo and wr_rating < max_elo:
                    # 只收同 Rank 對局
                    if getRank(wr_rating) == getRank(br_rating):
                        rank_idx = getRank(wr_rating)
                        if 0 <= rank_idx < len(filtered_lines):
                            filtered_lines[rank_idx].append(line)

    for i in range(min_elo, max_elo, interval):
        rank_idx = getRank(i)
        current_lines = filtered_lines[rank_idx]
        print(f"{file_name} Rank {i}~{i+interval}: {len(current_lines)} lines")

        sample_count = min(lines_per_file, len(current_lines))
        random_lines = random.sample(current_lines, sample_count)

        sep = int(len(random_lines) * 0.8)

        output_file = os.path.join(f"{base_output_name}/train/sgf_{i}_{i + interval}", file_name)
        with open(output_file, "w", encoding="utf-8") as f_out:
            f_out.writelines(random_lines[:sep])

        output_file = os.path.join(f"{base_output_name}/test_origin/sgf_{i}_{i + interval}", file_name)
        with open(output_file, "w", encoding="utf-8") as f_out:
            f_out.writelines(random_lines[sep:])

    print(f"------finish {file_name}------")


if __name__ == "__main__":
    if not os.path.exists(input_dir):
        print(f"Error: {input_dir} not found.")
        exit(1)
    with ThreadPoolExecutor(max_workers=4) as executor:
        executor.map(process_file, [f for f in os.listdir(input_dir) if f.endswith(".txt")])
    print("complete!")
