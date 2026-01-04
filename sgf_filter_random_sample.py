import os
import random
import shutil
import re
from concurrent.futures import ThreadPoolExecutor
from tqdm import tqdm

# ================= 參數設定 =================
min_elo = 1000
max_elo = 3000
interval = 100
lines_per_file = 50000
# ==========================================


input_dir = "training_sgf"


def getRank(elo):
    if elo < min_elo or elo >= max_elo:
        return -1
    return (elo - min_elo) // interval


def process_file(file_name):
    # 這裡的 filtered_lines 結構改成：
    # filtered_lines[rank_index] = {'hq': [], 'lq': []}
    # hq: High Quality (Rapid/Classical)
    # lq: Low Quality (Blitz)

    total_ranks = (max_elo - min_elo) // interval
    filtered_data = []

    base_folder_name = f"rank_{lines_per_file}_{min_elo}_{max_elo}_{interval}interval"

    for i in range(total_ranks):
        filtered_data.append({'hq': [], 'lq': []})

    # 建立目錄結構
    for i in range(min_elo, max_elo, interval):
        os.makedirs(f"{base_folder_name}/train/sgf_{i}_{i + interval}", exist_ok=True)
        os.makedirs(f"{base_folder_name}/test_origin/sgf_{i}_{i + interval}", exist_ok=True)

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

                    if getRank(wr_rating) == getRank(br_rating):
                        rank_idx = getRank(wr_rating)
                        if 0 <= rank_idx < len(filtered_data):
                            # 【關鍵修改】在這裡就做分流
                            if "EV[Rapid]" in line or "EV[Classical]" in line:
                                filtered_data[rank_idx]['hq'].append(line)
                            else:
                                filtered_data[rank_idx]['lq'].append(line)

    # 開始抽樣與寫入
    for i in range(min_elo, max_elo, interval):
        rank_idx = getRank(i)
        hq_lines = filtered_data[rank_idx]['hq']
        lq_lines = filtered_data[rank_idx]['lq']
        total_available = len(hq_lines) + len(lq_lines)

        target_count = min(lines_per_file, total_available)

        final_samples = []

        # 【優先級邏輯】
        if len(hq_lines) >= target_count:
            # 爽！Rapid 夠多，全用 Rapid
            final_samples = random.sample(hq_lines, target_count)
            # print(f"Rank {i}: Pure High Quality ({target_count})")
        else:
            # Rapid 不夠，先全拿，剩下用 Blitz 補
            shortage = target_count - len(hq_lines)
            # print(f"Rank {i}: Mixed ({len(hq_lines)} HQ + {shortage} LQ)")

            if len(lq_lines) >= shortage:
                final_samples = hq_lines + random.sample(lq_lines, shortage)
            else:
                final_samples = hq_lines + lq_lines  # 全部梭哈

        # 打亂順序 (避免前面全是 Rapid 後面全是 Blitz，雖然訓練通常會 shuffle 但這樣比較保險)
        random.shuffle(final_samples)

        print(f"{file_name} Rank {i}~{i+interval}: {len(final_samples)} lines (HQ: {len([x for x in final_samples if 'Rapid' in x or 'Classical' in x])})")

        # 80% Train, 20% Test Origin
        sep = int(len(final_samples) * 0.8)

        output_file = os.path.join(f"{base_output_name}/train/sgf_{i}_{i + interval}", file_name)
        with open(output_file, "w", encoding="utf-8") as f_out:
            f_out.writelines(final_samples[:sep])

        output_file = os.path.join(f"{base_output_name}/test_origin/sgf_{i}_{i + interval}", file_name)
        with open(output_file, "w", encoding="utf-8") as f_out:
            f_out.writelines(final_samples[sep:])

    print(f"------finish {file_name}------")


if __name__ == "__main__":
    if not os.path.exists(input_dir):
        print(f"Error: {input_dir} not found.")
        exit(1)
    with ThreadPoolExecutor(max_workers=4) as executor:
        executor.map(process_file, [f for f in os.listdir(input_dir) if f.endswith(".txt")])
    print("complete!")
