import os
import random

# ================= 參數設定 (必須同步) =================
MIN_ELO = 600
MAX_ELO = 3000
INTERVAL = 100
LINES_PER_FILE = 100000
FOLDER_NAME = f"rank_{LINES_PER_FILE}_{MIN_ELO}_{MAX_ELO}_{INTERVAL}interval"
# ====================================================


def process_file(in_file, cand_target, test_target):
    total_needed = cand_target + test_target

    with open(in_file, "r", encoding="utf-8") as file:
        lines = file.readlines()

        if len(lines) < total_needed:
            selected_lines = lines
            split_idx = int(len(lines) * (cand_target / total_needed))
            if split_idx == 0 and len(lines) > 0:
                split_idx = 1

            cand_lines = selected_lines[:split_idx]
            test_lines = selected_lines[split_idx:]
        else:
            selected_lines = random.sample(lines, total_needed)
            cand_lines = selected_lines[:cand_target]
            test_lines = selected_lines[cand_target:]

    target_dir = os.path.dirname(in_file).replace("test_origin", "cand")
    os.makedirs(target_dir, exist_ok=True)
    with open(in_file.replace("test_origin", "cand"), "w", encoding="utf-8") as file:
        file.writelines(cand_lines)

    target_dir = os.path.dirname(in_file).replace("test_origin", "test")
    os.makedirs(target_dir, exist_ok=True)
    with open(in_file.replace("test_origin", "test"), "w", encoding="utf-8") as file:
        file.writelines(test_lines)

    print(f"{in_file} -> Cand:{len(cand_lines)} Test:{len(test_lines)}")


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
