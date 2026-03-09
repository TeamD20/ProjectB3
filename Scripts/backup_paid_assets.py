#!/usr/bin/env python
# -*- coding: utf-8 -*-
""".gitignore의 '# Begin Fab 에셋 폴더' ~ '# End Fab 에셋 폴더' 사이 경로들을 백업합니다."""

from __future__ import annotations

import argparse
import shutil
from datetime import datetime
from pathlib import Path

SECTION_BEGIN = "# Begin Fab 에셋 폴더"
SECTION_END = "# End Fab 에셋 폴더"


def parse_paid_asset_paths(gitignore_path: Path) -> list[str]:
    lines = gitignore_path.read_text(encoding="utf-8").splitlines()
    try:
        start_index = lines.index(SECTION_BEGIN)
    except ValueError:
        raise ValueError(f"시작 마커를 찾을 수 없습니다: {SECTION_BEGIN}")

    try:
        end_index = lines.index(SECTION_END)
    except ValueError:
        raise ValueError(f"종료 마커를 찾을 수 없습니다: {SECTION_END}")

    if end_index <= start_index:
        raise ValueError("종료 마커가 시작 마커보다 앞에 있습니다.")

    paths: list[str] = []
    for line in lines[start_index + 1 : end_index]:
        stripped = line.strip()
        if not stripped:
            continue
        if stripped.startswith("#"):
            continue
        paths.append(stripped)

    if not paths:
        raise ValueError("백업할 경로가 비어 있습니다.")

    return paths


def copy_path(src: Path, dst: Path) -> None:
    if src.is_dir():
        shutil.copytree(src, dst, dirs_exist_ok=True)
        return
    if src.is_file():
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        return
    print(f"[SKIP] 경로 없음: {src}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description=".gitignore의 '# Begin Fab 에셋 폴더' ~ '# End Fab 에셋 폴더' 사이 경로들을 백업합니다.",
    )
    parser.add_argument(
        "--repo",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="프로젝트 루트 경로 (기본값: 스크립트의 상위 폴더)",
    )
    parser.add_argument(
        "--gitignore",
        type=Path,
        default=None,
        help=".gitignore 경로 (기본값: <repo>/.gitignore)",
    )
    parser.add_argument(
        "--backup",
        type=Path,
        default=None,
        help="백업 대상 루트 폴더 (기본값: <repo>/Backup/paid-assets-YYYYmmdd-HHMMSS)",
    )

    args = parser.parse_args()
    repo_root = args.repo.resolve()
    gitignore_path = args.gitignore.resolve() if args.gitignore else repo_root / ".gitignore"

    if not gitignore_path.exists():
        raise FileNotFoundError(f".gitignore를 찾을 수 없습니다: {gitignore_path}")

    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    backup_root = (
        args.backup.resolve()
        if args.backup
        else repo_root / "Backup" / f"paid-assets-{timestamp}"
    )
    backup_root.mkdir(parents=True, exist_ok=True)

    rel_paths = parse_paid_asset_paths(gitignore_path)

    for rel in rel_paths:
        src = (repo_root / rel).resolve()
        dst = (backup_root / rel).resolve()
        copy_path(src, dst)

    print(f"백업 완료: {backup_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
