import os
import random

# ================= 參數設定 =================
MIN_ELO = 1000
MAX_ELO = 3000
INTERVAL = 100
LINES_PER_FILE = 50000
FOLDER_NAME = f"rank_{LINES_PER_FILE}_{MIN_ELO}_{MAX_ELO}_{INTERVAL}interval"
# ==========================================


def process_file(in_file, cand_target, test_target):
    total_needed = cand_target + test_target

    with open(in_file, "r", encoding="utf-8") as file:
        lines = file.readlines()

        # 1. 總量檢查
        if len(lines) < total_needed:
            print(f"Warning: {in_file} total lines ({len(lines)}) < needed ({total_needed}). Taking all.")
            selected_lines = lines

            # 依比例分配
            split_idx = int(len(lines) * (cand_target / total_needed))
            if split_idx == 0 and len(lines) > 0:
                split_idx = 1

            cand_lines = selected_lines[:split_idx]
            test_lines = selected_lines[split_idx:]

        else:
            # 2. 分類：(Rapid/Classical)
            high_quality = []  # Rapid, Classical
            low_quality = []  # Blitz

            for line in lines:
                if "EV[Rapid]" in line or "EV[Classical]" in line:
                    high_quality.append(line)
                else:
                    low_quality.append(line)

            selected_lines = []

            # 3. 優先級抽樣邏輯
            if len(high_quality) >= total_needed:
                # 高品質資料夠多，只用高品質的
                # print(f"  [High Quality] {in_file}: {len(high_quality)} available, taking {total_needed}")
                selected_lines = random.sample(high_quality, total_needed)
            else:
                # 高品質不夠，先全拿，剩下用 Blitz 補
                shortage = total_needed - len(high_quality)
                # print(f"  [Mixed] {in_file}: Taking all {len(high_quality)} Rapid + {shortage} Blitz")

                # 補 Blitz
                if len(low_quality) >= shortage:
                    selected_lines = high_quality + random.sample(low_quality, shortage)
                else:
                    # 雖然總數夠，但邏輯上不應該跑到這，做個保險
                    selected_lines = high_quality + low_quality

            # 4. 打亂順序 (避免前半段都是 Rapid 後半段都是 Blitz)
            random.shuffle(selected_lines)

            # 5. 分配
            cand_lines = selected_lines[:cand_target]
            test_lines = selected_lines[cand_target:]

    # 寫入 cand
    target_dir = os.path.dirname(in_file).replace("test_origin", "cand")
    os.makedirs(target_dir, exist_ok=True)
    with open(in_file.replace("test_origin", "cand"), "w", encoding="utf-8") as file:
        file.writelines(cand_lines)

    # 寫入 test
    target_dir = os.path.dirname(in_file).replace("test_origin", "test")
    os.makedirs(target_dir, exist_ok=True)
    with open(in_file.replace("test_origin", "test"), "w", encoding="utf-8") as file:
        file.writelines(test_lines)

    print(f"{in_file} -> Cand:{len(cand_lines)} Test:{len(test_lines)} (HighQ Ratio: {len([x for x in selected_lines if 'Rapid' in x or 'Classical' in x])}/{len(selected_lines)})")


def process_folder(folder_path):
    walk_path = os.path.join(folder_path, "test_origin")
    if not os.path.exists(walk_path):
        print(f"Error: {walk_path} not found.")
        return

    for root, dirs, files in os.walk(walk_path):
        for file in files:
            if file.endswith(".txt"):
                folder_name = os.path.basename(root)
                if folder_name.startswith("sgf_"):
                    parts = folder_name.split("_")
                    if len(parts) == 3:
                        try:
                            x = int(parts[1])
                            # 邊界外少抽一點，正常區段多抽一點
                            if x < MIN_ELO or x >= MAX_ELO:
                                process_file(os.path.join(root, file), 5, 50)
                            else:
                                process_file(os.path.join(root, file), 20, 200)
                        except ValueError:
                            pass


def main():
    print(f"Processing: {FOLDER_NAME}")
    process_folder(FOLDER_NAME)


if __name__ == "__main__":
    main()
