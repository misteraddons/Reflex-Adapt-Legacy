#!/usr/bin/env python3

import argparse
import hashlib
import json
import os
import re
import time
from pathlib import Path
from typing import Optional, TypedDict, Union
from zipfile import ZIP_DEFLATED, ZipFile

DB_FILE = "reflex-adapt-legacy.json"
DB_ID = "misteraddons/reflex-adapt-legacy"
SCRIPT_DIR = Path(__file__).resolve().parent
MAPPINGS_DIR = SCRIPT_DIR / "config" / "inputs"
CONFIGS_DIR = SCRIPT_DIR / "config"
MAPPING_SUFFIX = "_v3.map"
CONFIG_SUFFIX = ".cfg"
MISTER_INPUTS_DIR = "config/inputs"
MISTER_CONFIGS_DIR = "config"
RAW_BASE = "https://raw.githubusercontent.com/misteraddons/Reflex-Adapt-Legacy/{tag}/mister/"
UPDATER_URL = "https://github.com/misteraddons/Reflex-Adapt-Legacy/releases/download/{tag}/reflex_updater.sh"
TAG_PATTERN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$")


class RepoDbFilesItem(TypedDict):
    hash: str
    size: int
    url: Optional[str]
    overwrite: Optional[bool]
    reboot: Optional[bool]


RepoDbFiles = dict[str, RepoDbFilesItem]


class RepoDbFoldersItem(TypedDict):
    tags: Optional[list[Union[str, int]]]


RepoDbFolders = dict[str, RepoDbFoldersItem]


class RepoDb(TypedDict):
    db_id: str
    timestamp: int
    files: RepoDbFiles
    folders: RepoDbFolders
    base_files_url: Optional[str]


def md5_file(path: Path) -> str:
    digest = hashlib.md5(usedforsecurity=False)
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def create_repo_db(input_files: list[Path], tag: str, files_ref: str, updater_path: Path) -> RepoDb:
    folders: RepoDbFolders = {
        f"{MISTER_CONFIGS_DIR}/": RepoDbFoldersItem(tags=None),
        f"{MISTER_INPUTS_DIR}/": RepoDbFoldersItem(tags=None),
        "Scripts/": RepoDbFoldersItem(tags=None),
    }

    files: RepoDbFiles = {}
    for path in sorted(input_files):
        folder = MISTER_CONFIGS_DIR if path.suffix.lower() == CONFIG_SUFFIX else MISTER_INPUTS_DIR
        files[f"{folder}/{path.name}"] = RepoDbFilesItem(
            hash=md5_file(path),
            size=path.stat().st_size,
            url=None,
            overwrite=False,
            reboot=None,
        )

    files["Scripts/reflex_updater.sh"] = RepoDbFilesItem(
        hash=md5_file(updater_path),
        size=updater_path.stat().st_size,
        url=UPDATER_URL.format(tag=tag),
        overwrite=None,
        reboot=None,
    )

    return RepoDb(
        db_id=DB_ID,
        timestamp=int(time.time()),
        files=files,
        folders=folders,
        base_files_url=RAW_BASE.format(tag=files_ref),
    )


def remove_nulls(value: object) -> object:
    if isinstance(value, dict):
        return {key: remove_nulls(item) for key, item in value.items() if item is not None}
    return value


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate the MiSTer Downloader database.")
    parser.add_argument("tag", help="Exact Git tag containing the mapped configuration files")
    parser.add_argument("--files-ref", help="Git ref containing mapping/config files; defaults to tag")
    parser.add_argument("--updater", type=Path, required=True, help="Path to reflex_updater.sh")
    parser.add_argument("--output", type=Path, default=Path(DB_FILE), help="Output JSON path")
    args = parser.parse_args()
    if not TAG_PATTERN.fullmatch(args.tag):
        parser.error("tag must contain only letters, digits, periods, underscores, and hyphens")
    if args.files_ref is None:
        args.files_ref = args.tag
    if not TAG_PATTERN.fullmatch(args.files_ref):
        parser.error("files-ref must contain only letters, digits, periods, underscores, and hyphens")
    if not args.updater.is_file():
        parser.error(f"updater does not exist: {args.updater}")
    return args


def main() -> None:
    args = parse_args()
    input_files = list(MAPPINGS_DIR.rglob(f"*{MAPPING_SUFFIX}"))
    input_files.extend(CONFIGS_DIR.rglob(f"*{CONFIG_SUFFIX}"))
    repo_db = create_repo_db(input_files, args.tag, args.files_ref, args.updater)

    args.output.write_text(json.dumps(remove_nulls(repo_db), indent=4) + "\n", encoding="utf-8")
    zip_path = args.output.with_suffix(args.output.suffix + ".zip")
    with ZipFile(zip_path, "w", compression=ZIP_DEFLATED) as archive:
        archive.write(args.output, arcname=args.output.name)


if __name__ == "__main__":
    main()