"""
run_task.py - Parallel worker for KaiAlloyShielding Simulation on GitHub Actions Cluster
"""

import os
import sys
import time
import json
import shutil
import argparse
import subprocess
from pathlib import Path
import pandas as pd

try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")
except Exception:
    pass

try:
    import gdrive_helper
except ImportError:
    gdrive_helper = None


def parse_args():
    parser = argparse.ArgumentParser(description="KaiAlloyShielding Parallel Worker")
    parser.add_argument("--runner-id", type=int, required=True, help="ID của runner (1..10)")
    parser.add_argument("--primaries", type=int, default=1000000, help="Số hạt mỗi job")
    parser.add_argument("--gdrive-folder", type=str, default=None, help="Google Drive Folder ID")
    parser.add_argument("--is-rescue", type=str, default="false", help="Chế độ cứu hộ ('true'/'false')")
    parser.add_argument("--rescue-jobs", type=str, default="", help="Danh sách jobs cứu hộ")
    return parser.parse_args()


def find_executable() -> str:
    candidates = [
        "./build/KaiShielding", "./KaiShielding",
        "./build/Sim", "./Sim",
        "bin/KaiShielding"
    ]
    for c in candidates:
        if os.path.exists(c) and os.access(c, os.X_OK):
            return c
    
    # Tìm kiếm bất kỳ binary nào trong build
    if os.path.exists("build"):
        for f in Path("build").iterdir():
            if f.is_file() and os.access(f, os.X_OK) and not f.name.endswith(".so") and not f.name.endswith(".a"):
                return str(f)
    
    return "./build/KaiShielding"


def write_macro(material: str, thickness_mm: float, spectrum: str, n_events: int, macro_path: str = "_tmp_run.mac"):
    macro = f"""/run/numberOfThreads 2
/shield/material   {material}
/shield/thickness  {thickness_mm} mm
/source/spectrum   {spectrum}
/run/initialize
/run/printProgress {max(1, n_events // 5)}
/run/beamOn        {n_events}
"""
    with open(macro_path, "w", encoding="utf-8") as f:
        f.write(macro)


def main():
    args = parse_args()
    print("=" * 80)
    print(f"🚀 RUNNER #{args.runner_id} KHỞI ĐỘNG (Primaries: {args.primaries})")
    print(f"Chế độ Cứu hộ: {args.is_rescue.upper()}")
    print("=" * 80)

    exe_path = find_executable()
    print(f"Executable Geant4: {exe_path}")

    # Xác định danh sách jobs cần chạy
    assigned_jobs = []
    if args.is_rescue.lower() == "true" and args.rescue_jobs.strip():
        # Chế độ cứu hộ
        job_names = [j.strip() for j in args.rescue_jobs.split(",") if j.strip()]
        for jname in job_names:
            assigned_jobs.append({
                "job_name": jname,
                "material": jname.split("_")[0],
                "spectrum": "fission",
                "thickness_mm": 20.0
            })
    else:
        # Chế độ thường: đọc plan.json
        plan_file = Path("plan.json")
        if plan_file.exists():
            with open(plan_file, "r", encoding="utf-8") as f:
                plan = json.load(f)
            # Tìm account tương ứng hoặc runner id
            for acc, r_dict in plan.items():
                if str(args.runner_id) in r_dict:
                    assigned_jobs = r_dict[str(args.runner_id)]
                    if assigned_jobs:
                        break

    if not assigned_jobs:
        print(f"ℹ️ Không có job nào được phân bổ cho runner #{args.runner_id}. Hoàn tất!")
        return

    print(f"\n>>> Tổng số jobs runner #{args.runner_id} cần xử lý: {len(assigned_jobs)}")
    results_dir = Path("results")
    results_dir.mkdir(parents=True, exist_ok=True)

    results_data = []

    for idx, job in enumerate(assigned_jobs, 1):
        jname = job.get("job_name", f"job_{idx}")
        mat = job.get("material", "SS316L")
        th_mm = float(job.get("thickness_mm", 20.0))
        spec = job.get("spectrum", "fission")

        print(f"\n[{idx}/{len(assigned_jobs)}] Đang chạy mô phỏng: {jname} (Vật liệu: {mat}, Bề dày: {th_mm}mm)...")
        t0 = time.time()
        
        macro_file = f"_tmp_{args.runner_id}.mac"
        write_macro(mat, th_mm, spec, args.primaries, macro_file)

        # Chạy binary
        res = subprocess.run([exe_path, macro_file])
        elapsed = time.time() - t0

        if res.returncode == 0:
            print(f"  ✅ {jname} hoàn tất trong {elapsed:.1f}s")
            results_data.append({
                "job_name": jname,
                "material": mat,
                "thickness_mm": th_mm,
                "spectrum": spec,
                "primaries": args.primaries,
                "elapsed_sec": round(elapsed, 2),
                "status": "SUCCESS"
            })
        else:
            print(f"  ❌ {jname} thất bại (Return code: {res.returncode})")
            results_data.append({
                "job_name": jname,
                "material": mat,
                "status": "FAILED"
            })

        if os.path.exists(macro_file):
            os.remove(macro_file)

    # Xuất CSV kết quả của runner
    out_csv = results_dir / f"runner_{args.runner_id}_results.csv"
    df = pd.DataFrame(results_data)
    df.to_csv(out_csv, index=False)
    print(f"\n>>> Đã xuất kết quả runner: {out_csv}")

    # Đẩy lên Google Drive nếu có thông tin xác thực
    gdrive_folder = args.gdrive_folder or os.environ.get("GDRIVE_FOLDER_ID")
    if gdrive_helper and gdrive_folder and os.environ.get("GDRIVE_OAUTH_JSON"):
        try:
            print(f"\n>>> Đang stream {out_csv.name} lên Google Drive...")
            service = gdrive_helper.get_drive_service()
            subfolders = gdrive_helper.get_or_create_subfolders(service, gdrive_folder)
            flux_folder_id = subfolders.get("02_Flux_Data", gdrive_folder)
            gdrive_helper.upload_or_update_file(service, flux_folder_id, out_csv)
            print("  ✅ Stream lên Google Drive thành công!")
        except Exception as e:
            print(f"  [CẢNH BÁO] Không thể đẩy lên Google Drive: {e}")

    print(f"\n🎉 RUNNER #{args.runner_id} HOÀN TẤT NHIỆM VỤ!")


if __name__ == "__main__":
    main()
