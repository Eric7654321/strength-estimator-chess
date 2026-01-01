#!/bin/bash

# cp ./scripts/data.py download_chess_game/
# cp ./scripts/board.py download_chess_game/
# cd download_chess_game/

# for year in 2024 2023; do
#     for month in 01 02 09 10 11 12; do
#         if [ "$year" = "2024" ] || ([ "$year" = "2023" ] && [ "$month" -ge 9 ]); then
#             mkdir -p "database${year}/${year}${month}/"
#             python3 data.py $year $month -u > "database${year}/${year}${month}/${year}-${month}-convert.txt"
#         fi
#     done
# done
# cd ../

# ==============================================================================
# 步驟 1: 收集分散的 SGF 檔案
# ==============================================================================
# echo "Collecting generated SGF files..."
# mkdir -p training_sgf

# # 迴圈遍歷所有可能的月份字串
# for year in 2024 2023; do
#     for month in 01 02 09 10 11 12; do
        
#         if [[ "$year" == "2024" && ("$month" == "01" || "$month" == "02") ]] || \
#            [[ "$year" == "2023" && ("$month" == "09" || "$month" == "10" || "$month" == "11" || "$month" == "12") ]]; then
            
#             SOURCE_FILE="download_chess_game/database${year}/${year}${month}/${year}-${month}-convert.txt"
            
#             # 再次檢查檔案是否存在，確保安全
#             if [ -f "$SOURCE_FILE" ]; then
#                 echo "Copying $SOURCE_FILE..."
#                 cp --reflink=auto "$SOURCE_FILE" training_sgf/
#             else
#                 echo "Warning: Target file $SOURCE_FILE should exist but was not found."
#             fi
#         fi
#     done
# done

# # ==============================================================================
# # 步驟 2: 執行過濾與抽樣 (Python Scripts)
# # ==============================================================================
echo "Running sgf_filter_random_sample.py (Splitting by Rank 600-3000, 100 interval)..."
cp ./scripts/sgf_filter_random_sample.py ./
python3 sgf_filter_random_sample.py

echo "Running random_sample.py (Creating Candidate/Test sets)..."
cp ./scripts/random_sample.py ./
python3 random_sample.py

# ==============================================================================
# 步驟 3: 合併檔案到最終目錄
# ==============================================================================
declare -A folder_map

# 資料夾名稱 (100000筆, 600-3000分, 100間距)
SOURCE_BASE="rank_100000_600_3000_100interval"

# 定義來源資料夾 -> 目標資料夾的對應關係
folder_map["$SOURCE_BASE/train"]="training_sgf_chess"
folder_map["$SOURCE_BASE/test"]="query_sgf_chess"
folder_map["$SOURCE_BASE/cand"]="candidate_sgf_chess"

for source_dir in "${!folder_map[@]}"; do
    target_dir="${folder_map[$source_dir]}"  
    
    mkdir -p "$target_dir"

    echo "Merging files from [$source_dir] -> [$target_dir]..."

    if [ -d "$source_dir" ]; then
        # 遍歷 sgf_xxxx_xxxx 資料夾
        for folder in "$source_dir"/sgf_*; do
            if [[ -d "$folder" ]]; then  
                base_name=$(basename "$folder")
                # 把 sgf_1000_1100 轉成 1000_1100.txt
                new_name="${base_name#sgf_}.txt"
                
                # 合併該資料夾下的所有 txt
                cat "$folder"/*.txt > "$target_dir/$new_name"
            fi
        done
    else
        echo "Error: Source directory '$source_dir' does not exist!"
        echo "Please check if python scripts ran successfully."
    fi
done

echo "========================================================"
echo "Preprocessing Complete!"
echo "Ready for Training."
echo "========================================================"