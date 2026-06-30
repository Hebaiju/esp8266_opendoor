#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ESP8266-01S 开门器项目 - 统一构建脚本
功能：编译、上传、串口监控、清理
用法：
    python build.py          编译 + 上传
    python build.py build    仅编译
    python build.py upload   仅上传
    python build.py monitor  串口监控
    python build.py clean    清理构建产物
    python build.py full     编译 + 上传 + 监控
"""

import sys
import os
import subprocess
import time

PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
LOG_FILE = os.path.join(PROJECT_DIR, "build.log")


def find_pio():
    import shutil
    pio_path = shutil.which("pio") or shutil.which("platformio")
    if pio_path:
        return [pio_path]

    candidate = os.path.join(
        os.path.expanduser("~"),
        ".platformio", "penv", "Scripts", "platformio.exe"
    )
    if os.path.isfile(candidate):
        return [candidate]

    print("错误: 未找到 PlatformIO (pio/platformio)，请先安装 PlatformIO")
    sys.exit(1)


PIO_CMD = find_pio()


def pio_run(args, desc=""):
    return run_cmd(PIO_CMD + args, desc)


def run_cmd(cmd, desc=""):
    print(f"\n{'='*60}")
    print(f"[执行] {desc}")
    print(f"[命令] {' '.join(cmd)}")
    print(f"{'='*60}\n")

    env = os.environ.copy()
    env["PLATFORMIO_FORCE_COLOR"] = "true"

    start_time = time.time()
    result = subprocess.run(cmd, cwd=PROJECT_DIR, env=env)
    elapsed = time.time() - start_time

    print(f"\n{'='*60}")
    print(f"[完成] {desc} - 耗时: {elapsed:.2f}s - 退出码: {result.returncode}")
    print(f"{'='*60}\n")

    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(f"\n{'='*60}\n")
        f.write(f"[时间] {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"[命令] {' '.join(cmd)}\n")
        f.write(f"[结果] 退出码={result.returncode}, 耗时={elapsed:.2f}s\n")
        f.write(f"{'='*60}\n")

    return result.returncode == 0


def build_only():
    return pio_run(["run"], "编译固件")


def upload_only():
    return pio_run(["run", "--target", "upload"], "上传固件")


def build_upload():
    if not build_only():
        return False
    return upload_only()


def monitor(timeout=30):
    print(f"\n启动串口监视器，{timeout}秒后自动退出...\n")
    import threading
    import signal

    stop_event = threading.Event()

    def signal_handler(sig, frame):
        stop_event.set()

    old_int_handler = signal.signal(signal.SIGINT, signal_handler)
    try:
        old_break_handler = signal.signal(signal.SIGBREAK, signal_handler)
    except (AttributeError, ValueError):
        old_break_handler = None

    try:
        proc = subprocess.Popen(
            PIO_CMD + ["device", "monitor"],
            cwd=PROJECT_DIR,
            env=dict(os.environ, PLATFORMIO_FORCE_COLOR="true")
        )

        import time
        start = time.time()
        while proc.poll() is None and (time.time() - start) < timeout:
            if stop_event.is_set():
                break
            time.sleep(0.5)

        if proc.poll() is None:
            print(f"\n\n[监视器超时 {timeout}秒，自动退出]\n")
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()
    finally:
        signal.signal(signal.SIGINT, old_int_handler)
        if old_break_handler is not None:
            signal.signal(signal.SIGBREAK, old_break_handler)

    return True


def clean():
    return pio_run(["run", "--target", "clean"], "清理构建产物")


def full():
    if not build_only():
        return False
    if not upload_only():
        return False
    return monitor()


def print_help():
    print(__doc__)


def main():
    if len(sys.argv) < 2:
        action = "build_upload"
    else:
        action = sys.argv[1].lower()

    actions = {
        "build": build_only,
        "upload": upload_only,
        "": build_upload,
        "build_upload": build_upload,
        "monitor": monitor,
        "clean": clean,
        "full": full,
        "-h": print_help,
        "--help": print_help,
        "help": print_help,
    }

    if action not in actions:
        print(f"错误: 未知命令 '{action}'")
        print_help()
        sys.exit(1)

    with open(LOG_FILE, "w", encoding="utf-8") as f:
        f.write(f"[启动] {time.strftime('%Y-%m-%d %H:%M:%S')} - action={action}\n")

    ok = actions[action]()

    if not ok and action not in ("monitor", "clean", "-h", "--help", "help"):
        print(f"\n❌ 操作失败，请查看 build.log 了解详情\n")
        sys.exit(1)

    if ok and action in ("build", "build_upload", "full", "upload"):
        print(f"\n✅ 操作成功！\n")


if __name__ == "__main__":
    main()
