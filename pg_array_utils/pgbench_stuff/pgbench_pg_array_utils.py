import csv
import re
import subprocess
from pathlib import Path
import sys
import shutil

# =============================================
# Настройки
# =============================================

DB             = "pgbench_test"
EXTENSION_NAME = "pg_array_utils"

USED_VERSION   = sys.argv[1]

HOME        = Path.home()
INSTALL_DIR = HOME / "postgres" / "install" / USED_VERSION / "debug"

PGBENCH = INSTALL_DIR / "bin" / "pgbench"
PSQL    = INSTALL_DIR / "bin" / "psql"

BASELINE_DIR    = Path("baseline")
PERF_TESTS_DIR  = Path("perf_tests")
CSV_RESULTS_DIR = Path(f"csv_results_{USED_VERSION}")
TEMP_DIR        = Path("temp_results")

RE_TPS        = re.compile(r"tps\s*=\s*([\d.]+)")
RE_LAT_AVG    = re.compile(r"latency average\s*=\s*([\d.]+)")
RE_LAT_STDDEV = re.compile(r"latency stddev\s*=\s*([\d.]+)")

ALL_FUNCTIONS = [
    "array_flatten",
    "array_chunk",
    "array_zip",
    "array_compact",
    "array_pad",
    "array_slice",
    "array_repeat",
]

if len(sys.argv) > 2:
    FUNCTIONS = sys.argv[2:]
    invalid = [f for f in FUNCTIONS if f not in ALL_FUNCTIONS]
    if invalid:
        print(f"Ошибка: неизвестные функции {invalid}")
        print(f"Доступные: {ALL_FUNCTIONS}")
        raise SystemExit(1)
else:
    FUNCTIONS = ALL_FUNCTIONS

# ---------------------------------------------
# Прогон каждой функции
TIME           = 20
THREADS        = 2
CLIENTS        = [1, 2, 4, 8, 16]  
REPEATS        = 3
# ---------------------------------------------

# =============================================
# Вспомогательные функции
# =============================================

def extract(regex, text):
    m = regex.search(text)
    return m.group(1) if m else "ERROR"


def _psql(dbname, sql):
    cmd = [str(PSQL), "-d", dbname, "-v", "ON_ERROR_STOP=1", "-c", sql]
    if USED_VERSION != "master":
        cmd = [str(PSQL), "-U", "postgres", "-d", dbname,
               "-v", "ON_ERROR_STOP=1", "-c", sql]
    cp = subprocess.run(cmd, capture_output=True, text=True)
    if cp.returncode != 0:
        print(cp.stderr)
        raise SystemExit(1)

# =============================================
# Жизненный цикл БД и расширения
# =============================================

def create_database():
    _psql("postgres", f'DROP DATABASE IF EXISTS "{DB}"')
    _psql("postgres", f'CREATE DATABASE "{DB}"')


def create_extension():
    _psql(DB, f'CREATE EXTENSION IF NOT EXISTS {EXTENSION_NAME}')


def drop_extension():
    _psql(DB, f'DROP EXTENSION IF EXISTS {EXTENSION_NAME} CASCADE')


def drop_database():
    _psql("postgres",
          f"SELECT pg_terminate_backend(pid) FROM pg_stat_activity "
          f"WHERE datname = '{DB}' AND pid <> pg_backend_pid()")
    _psql("postgres", f'DROP DATABASE IF EXISTS "{DB}"')


# =============================================
# Запуск pgbench и разбор вывода
# =============================================

def parse_summary(output):
    tps = RE_TPS.search(output)
    lat = RE_LAT_AVG.search(output)
    std = RE_LAT_STDDEV.search(output)
    if not (tps and lat and std):
        return None
    return (float(tps.group(1)),
            float(lat.group(1)),
            float(std.group(1)))

def run_pgbench(script_file, clients, run_num):
    log = TEMP_DIR / f"bench_{script_file.stem}_c{clients}_run{run_num}.log"
    print(f"[{script_file.stem}] c={clients} run={run_num}")

    cmd = [str(PGBENCH),
           "-c", str(clients), "-j", str(THREADS),
           "-T", str(TIME), "-P", "100000",
           "-n", "-M", "prepared",
           "-f", str(script_file), "-d", DB]
    if USED_VERSION != "master":
        cmd[-2:-2] = ["-U", "postgres"]

    with log.open("w") as f:
        subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT)

    out = log.read_text(encoding="utf-8", errors="replace")
    result = parse_summary(out)
    if result is None:
        print(f" -> FAILED (см. {log})")
    return result

# =============================================
# Основной цикл
# =============================================

def main():
    print("=" * 41)
    print("Начало работы")
    print("=" * 41)
    print()

    if TEMP_DIR.exists():
        shutil.rmtree(TEMP_DIR)
    TEMP_DIR.mkdir(parents=True)
    CSV_RESULTS_DIR.mkdir(exist_ok=True)

    for func_name in FUNCTIONS:
        for f in (PERF_TESTS_DIR / f"perf_{func_name}.sql", BASELINE_DIR / f"bsln_{func_name}.sql"):
            if not f.exists():
                print(f"Нет файла {f}")
                raise SystemExit(1)

    try:
        create_database()
        create_extension()
        print("=" * 41)
        print("БД и расширение созданы")
        print("=" * 41)

        for func_name in FUNCTIONS:
            script_file   = PERF_TESTS_DIR / f"perf_{func_name}.sql"
            baseline_file = BASELINE_DIR   / f"bsln_{func_name}.sql"
            result_file   = CSV_RESULTS_DIR / f"results_{USED_VERSION}_{func_name}.csv"

            print(f"\n{'=' * 41}")
            print(f"Функция: {func_name}")
            print(f"{'=' * 41}")

            with result_file.open("w", newline="") as ft:
                wt = csv.writer(ft)
                wt.writerow(["clients", "run", "ext_tps", "ext_latency", "base_latency", "net_latency", "ext_std"])
                broken = False
                for clients in CLIENTS:
                    for r in range(1, REPEATS + 1):
                        ext  = run_pgbench(script_file,   clients, r)
                        base = run_pgbench(baseline_file, clients, r)

                        if ext is None or base is None: 
                            broken = True
                            break

                        ext_tps, ext_lat, ext_std = ext
                        wt.writerow([clients, r, 
                                    f"{ext_tps:.3f}",
                                    f"{ext_lat:.3f}",
                                    f"{base[1]:.3f}",
                                    f"{ext_lat - base[1]:.3f}",
                                    f"{ext_std:.3f}"])
                        ft.flush()
                    if broken:
                        break

        # =============================================
        # Завершение
        # =============================================

        print()
        print("=" * 41)
        print("Все тесты завершены!")
        print(f"Результаты сохранены в {CSV_RESULTS_DIR}/")
        print("=" * 41)

    finally:
        drop_extension()
        drop_database()
        print()
        print("=" * 41)
        print("БД и расширение удалены")
        print("=" * 41)


if __name__ == "__main__":
    main()

