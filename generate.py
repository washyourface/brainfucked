
from pathlib import Path

tests = {
    "sub_basic": (
        "+++++--.",
        bytes([3]),
    ),
    "sub_underflow": (
        "-.",
        bytes([255]),
    ),
    "sub_wrap": (
        "---+++.",
        bytes([0]),
    ),
    "move_right": (
        ">+++++.",
        bytes([5]),
    ),
    "move_left": (
        ">+++++<+++.",
        bytes([3]),
    ),
    "move_multiple": (
        ">>>+++++<<<+++.>>>.",
        bytes([3, 5]),
    ),
    "cell_independence": (
        "++++>+++++<--.>-.",
        bytes([2, 4]),
    ),
    "input_basic": (
        ",.",
        b"A",
        b"A",
    ),
    "input_multiple": (
        ",.,.",
        b"AB",
        b"AB",
    ),
    "input_overwrite": (
        "+++++,.",
        b"B",
        b"B",
    ),
    "input_cells": (
        ",>,<.>.",
        b"XY",
        b"XY",
    ),
    "input_eof": (
        "+,.",
        b"\x00",
        b"",
    ),
    "input_eof_after_read": (
        ",,.",
        b"\x00",
        b"A",
    ),
    "input_binary": (
        ",.",
        bytes([255]),
        bytes([255]),
    ),
}

directory = Path("tests")
directory.mkdir(exist_ok=True)

for name, test in tests.items():
    source, expected, *stdin = test

    path = directory / f"{name}.bf"

    path.write_text(source + "\n", encoding="ascii")

    Path(f"{path}.expected").write_bytes(
        b"[exit]\n0\n[stdout]\n"
        + expected
        + b"\n[stderr]\n\n"
    )

    if stdin:
        Path(f"{path}.stdin").write_bytes(stdin[0])
