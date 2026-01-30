#!/usr/bin/env python3
"""
Find "Critical" or "Flat" positions in chess engine logs.

Modes:
  1. high_variance (Default): High Entropy + High Score Spread.
     Finds sharp tactical moments where moves have vastly different outcomes.
  2. low_variance: High Entropy + Low Score Spread.
     Finds positional/quiet moments where many moves are viable and equal.

Usage:
    python analyze_positions.py -i results.txt --n 20 --mode high_variance
    python analyze_positions.py -i results.txt --n 20 --mode low_variance
"""

from __future__ import annotations

import argparse
import dataclasses
import math
import re
from typing import List, Optional

import chess

# --- Regex Patterns ---
GAME_RE = re.compile(r"^\s*Game\s+(\d+)\s*:\s*$")
POS_RE = re.compile(r"^\s*Position\s+(\d+)\s*,\s*Action\s*=\s*([a-h][1-8][a-h][1-8][qrbn]?)\s*$")
MOVE_RE = re.compile(
    r"^\s*([a-h][1-8][a-h][1-8][qrbn]?)\s*:.*?(?:policy=)?([+-]?\d+(?:\.\d+)?(?:e[+-]?\d+)?).*?(?:strength=)?([+-]?\d+(?:\.\d+)?(?:e[+-]?\d+)?)",
    re.IGNORECASE,
)

@dataclasses.dataclass
class CandidateMove:
    uci: str
    policy: float
    strength: float

@dataclasses.dataclass
class PositionRecord:
    game_idx: int
    pos_idx: int
    fen: str
    history: str
    action_uci: str
    candidates: List[CandidateMove]
    
    # Computed Metrics
    entropy: float = 0.0
    weighted_std_dev: float = 0.0
    
    # Sorting Scores
    high_var_score: float = 0.0  # Entropy * StdDev
    low_var_score: float = 0.0   # Entropy / (StdDev + epsilon)


def compute_metrics(candidates: List[CandidateMove]) -> tuple[float, float, float, float]:
    if not candidates:
        return 0.0, 0.0, 0.0, 0.0

    # 1. Normalize Probabilities
    raw_probs = [c.policy for c in candidates]
    total_p = sum(raw_probs)
    if total_p <= 0:
        return 0.0, 0.0, 0.0, 0.0 
    probs = [p / total_p for p in raw_probs]
    scores = [c.strength for c in candidates]

    # 2. Calculate Entropy (Still calculated for display, but not used for sorting)
    entropy = 0.0
    for p in probs:
        if p > 0:
            entropy -= p * math.log2(p)

    # 3. Calculate Weighted StdDev
    weighted_mean_score = sum(p * s for p, s in zip(probs, scores))
    weighted_variance = sum(p * ((s - weighted_mean_score) ** 2) for p, s in zip(probs, scores))
    weighted_std = math.sqrt(weighted_variance)

    # --- MODIFIED SECTION ---
    # 4. Calculate Sorting Scores (Pure Variance)
    high_var_score = weighted_std
    low_var_score = 1.0 / (weighted_std + 1e-9)
    # ------------------------

    return entropy, weighted_std, high_var_score, low_var_score


def parse_results(path: str, start_fen: str) -> List[PositionRecord]:
    records: List[PositionRecord] = []
    board = chess.Board(start_fen)
    game_idx = -1

    # State
    cur_pos_idx: Optional[int] = None
    cur_action: Optional[str] = None
    cur_fen: Optional[str] = None
    cur_candidates: List[CandidateMove] = []

    def finalize_position():
        nonlocal board, cur_pos_idx, cur_action, cur_fen, cur_candidates, records

        if cur_pos_idx is None or cur_action is None or cur_fen is None:
            return

        ent, std, h_score, l_score = compute_metrics(cur_candidates)

        # Generate History String (PGN format)
        temp_board = chess.Board(start_fen)
        try:
            temp_board = chess.Board(start_fen)
            # Do NOT manually push moves here. variation_san does it internally.
            history_san = temp_board.variation_san(board.move_stack)
        except Exception as e:
            history_san = f"Error: {e}"

        records.append(
            PositionRecord(
                game_idx=game_idx,
                pos_idx=cur_pos_idx,
                fen=cur_fen,
                history=history_san,
                action_uci=cur_action,
                candidates=cur_candidates[:],
                entropy=ent,
                weighted_std_dev=std,
                high_var_score=h_score,
                low_var_score=l_score
            )
        )

        # Advance board
        try:
            move = chess.Move.from_uci(cur_action)
            board.push(move)
        except Exception:
            pass # Ignore illegal moves

        cur_pos_idx = None
        cur_action = None
        cur_fen = None
        cur_candidates = []

    with open(path, "r", encoding="utf-8") as f:
        for raw_line in f:
            line = raw_line.rstrip("\n")

            m = GAME_RE.match(line)
            if m:
                if cur_pos_idx is not None: finalize_position()
                game_idx = int(m.group(1))
                board = chess.Board(start_fen)
                cur_pos_idx = None
                continue

            m = POS_RE.match(line)
            if m:
                if cur_pos_idx is not None: finalize_position()
                cur_pos_idx = int(m.group(1))
                cur_action = m.group(2)
                cur_fen = board.fen()
                cur_candidates = []
                continue

            m = MOVE_RE.match(line)
            if m and cur_pos_idx is not None:
                cur_candidates.append(CandidateMove(m.group(1), float(m.group(2)), float(m.group(3))))
                continue

            if (not line.strip() or "var =" in line) and cur_pos_idx is not None:
                finalize_position()
                continue
        
        if cur_pos_idx is not None:
            finalize_position()

    return records


def main() -> None:
    ap = argparse.ArgumentParser(description="Analyze chess positions by Entropy and Score Variance.")
    ap.add_argument("--input", "-i", required=True, help="Path to results file")
    ap.add_argument("--n", type=int, default=10, help="Top N positions to print")
    ap.add_argument("--mode", choices=['high_variance', 'low_variance'], default='high_variance',
                    help="Sort by 'high_variance' (sharp/tactical) or 'low_variance' (flat/positional)")
    ap.add_argument("--start-fen", default=chess.STARTING_FEN, help="Starting FEN")
    
    args = ap.parse_args()

    records = parse_results(args.input, args.start_fen)
    
    # Sort based on mode
    if args.mode == 'high_variance':
        # Descending: Highest complexity first
        records.sort(key=lambda r: r.high_var_score, reverse=True)
        score_label = "Complexity (Ent * Std)"
    else:
        # Descending: Highest 'Flatness' (High Entropy / Low Std)
        records.sort(key=lambda r: r.low_var_score, reverse=True)
        score_label = "Flatness (Ent / Std)"

    top = records[: max(args.n, 0)]
    
    print(f"Found {len(records)} positions. Showing top {len(top)} | Mode: {args.mode}")
    print("=" * 100)
    
    for rank, r in enumerate(top, 1):
        # Pick the score to display based on mode
        display_score = r.high_var_score if args.mode == 'high_variance' else r.low_var_score
        
        print(f"#{rank} [{score_label}: {display_score:.4f}] "
              f"Entropy: {r.entropy:.4f} | StdDev: {r.weighted_std_dev:.4f}")
        print(f"    Game {r.game_idx} | Pos {r.pos_idx} | Action: {r.action_uci}")
        print(f"    History: {r.history}")
        print(f"    FEN: {r.fen}")

        top_moves = sorted(r.candidates, key=lambda c: c.policy, reverse=True)[:10]
        print("    Top Candidates:")
        for m in top_moves:
            print(f"      {m.uci}: policy={m.policy:.4f}, strength={m.strength:.4f}")
        print("-" * 100)


if __name__ == "__main__":
    main()