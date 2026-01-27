#!/usr/bin/env python3
"""
Find top-N positions with highest var from a results file like:
usage:
python top_var_positions.py -i results.txt --n 20 --per-position-moves 5
"""

from __future__ import annotations

import argparse
import dataclasses
import re
from typing import List, Optional, Tuple

import chess


GAME_RE = re.compile(r"^\s*Game\s+(\d+)\s*:\s*$")
POS_RE = re.compile(r"^\s*Position\s+(\d+)\s*,\s*Action\s*=\s*([a-h][1-8][a-h][1-8][qrbn]?)\s*$")
# move:policy, strength
MOVE_RE = re.compile(
    r"^\s*([a-h][1-8][a-h][1-8][qrbn]?)\s*:\s*([+-]?\d+(?:\.\d+)?(?:e[+-]?\d+)?)\s*,\s*([+-]?\d+(?:\.\d+)?(?:e[+-]?\d+)?)\s*$",
    re.IGNORECASE,
)
VAR_RE = re.compile(r"^\s*var\s*=\s*([+-]?\d+(?:\.\d+)?(?:e[+-]?\d+)?)\s*$", re.IGNORECASE)


@dataclasses.dataclass
class CandidateMove:
    uci: str
    policy: float
    strength: float


@dataclasses.dataclass
class PositionRecord:
    game_idx: int
    pos_idx: int
    fen: str               # fen BEFORE applying Action
    action_uci: str
    var: float
    candidates: List[CandidateMove]


def parse_results(path: str, start_fen: str) -> List[PositionRecord]:
    records: List[PositionRecord] = []

    board = chess.Board(start_fen)
    game_idx = -1

    # State for current position block
    cur_pos_idx: Optional[int] = None
    cur_action: Optional[str] = None
    cur_fen: Optional[str] = None
    cur_candidates: List[CandidateMove] = []

    def finalize_position(var_value: float) -> None:
        nonlocal board, cur_pos_idx, cur_action, cur_fen, cur_candidates, records

        if cur_pos_idx is None or cur_action is None or cur_fen is None:
            return  # nothing to finalize

        records.append(
            PositionRecord(
                game_idx=game_idx,
                pos_idx=cur_pos_idx,
                fen=cur_fen,
                action_uci=cur_action,
                var=var_value,
                candidates=cur_candidates[:],
            )
        )

        # Advance board by the chosen action
        try:
            move = chess.Move.from_uci(cur_action)
            if move not in board.legal_moves:
                # Try pushing anyway to surface a clearer error
                raise ValueError(f"Illegal move {cur_action} on FEN {board.fen()}")
            board.push(move)
        except Exception as e:
            raise RuntimeError(
                f"Failed to apply Action move for Game {game_idx}, Position {cur_pos_idx}: {cur_action}\n"
                f"Current FEN was: {board.fen()}\n"
                f"Error: {e}"
            ) from e

        # Reset position state
        cur_pos_idx = None
        cur_action = None
        cur_fen = None
        cur_candidates = []

    with open(path, "r", encoding="utf-8") as f:
        for raw_line in f:
            line = raw_line.rstrip("\n")

            m = GAME_RE.match(line)
            if m:
                # New game: reset board and state
                game_idx = int(m.group(1))
                board = chess.Board(start_fen)
                cur_pos_idx = None
                cur_action = None
                cur_fen = None
                cur_candidates = []
                continue

            m = POS_RE.match(line)
            if m:
                cur_pos_idx = int(m.group(1))
                cur_action = m.group(2)
                cur_fen = board.fen()  # position before action
                cur_candidates = []
                continue

            m = MOVE_RE.match(line)
            if m and cur_pos_idx is not None:
                uci = m.group(1)
                policy = float(m.group(2))
                strength = float(m.group(3))
                cur_candidates.append(CandidateMove(uci=uci, policy=policy, strength=strength))
                continue

            m = VAR_RE.match(line)
            if m and cur_pos_idx is not None:
                var_value = float(m.group(1))
                finalize_position(var_value)
                continue

    return records


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", "-i", required=True, help="Path to results.txt")
    ap.add_argument("--n", type=int, default=10, help="Top N positions to print (default: 10)")
    ap.add_argument(
        "--start-fen",
        default=chess.STARTING_FEN,
        help="Starting FEN for each game (default: standard initial position)",
    )
    ap.add_argument(
        "--per-position-moves",
        type=int,
        default=0,
        help="If >0, also print top K candidate moves (by policy) for each returned position",
    )
    args = ap.parse_args()

    records = parse_results(args.input, args.start_fen)
    records.sort(key=lambda r: r.var, reverse=True)

    top = records[: max(args.n, 0)]
    for rank, r in enumerate(top, 1):
        print(f"#{rank}  var={r.var:.6g}  Game {r.game_idx}  Position {r.pos_idx}  Action={r.action_uci}")
        print(f"FEN: {r.fen}")

        # Always list the candidate move UCIs (in file order).
        # Optionally also include policy/strength details for top-K by policy.
        move_ucis = [cm.uci for cm in r.candidates]
        print(f"Moves ({len(move_ucis)}): {' '.join(move_ucis) if move_ucis else '(none)'}")

        if args.per_position_moves and r.candidates:
            k = min(args.per_position_moves, len(r.candidates))
            topk = sorted(r.candidates, key=lambda cm: cm.policy, reverse=True)[:k]
            print("Top candidates by policy:")
            for cm in topk:
                print(f"  {cm.uci}: policy={cm.policy:.6g}, strength={cm.strength:.6g}")

        print("-" * 80)


if __name__ == "__main__":
    main()
