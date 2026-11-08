import csv
import re
import subprocess
from pathlib import Path
import sys

# =============================================
# Настройки
# =============================================

DB             = "pgbench_test"
EXTENSION_NAME = "pg_array_utils"
USED_VERSION   = sys.argv[1]
TIME           = 60
THREADS        = 2
CLIENTS        = [1, 2, 4, 8, 16, 32, 64, 128]
REPEATS        = 3

HOME        = Path.home()
INSTALL_DIR = HOME / "postgres" / "install" / USED_VERSION / "debug"

PGBENCH = INSTALL_DIR / "bin" / "pgbench"
PSQL    = INSTALL_DIR / "bin" / "psql"

SCRIPT_FILE   = Path("perf_tests.sql")
BASELINE_FILE = Path("baseline.sql")

RESULT_FILE = Path(f"results_{USED_VERSION}.csv")
TEMP_DIR    = Path("temp_results")

RE_TPS        = re.compile(r"tps\s*=\s*([\d.]+)")
RE_LAT_AVG    = re.compile(r"latency average\s*=\s*([\d.]+)")
RE_LAT_STDDEV = re.compile(r"latency stddev\s*=\s*([\d.]+)")
RE_STMT = re.compile(r"^\s*([\d.]+)\s+([\d.]+)\s+(.*)$")


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

def parse_statements(output):
    stmts = []
    in_block = False
    for line in output.splitlines():
        if "statement latencies in milliseconds" in line:
            in_block = True
            continue
        if in_block:
            m = RE_STMT.match(line)
            if m:
                stmts.append((float(m.group(1)),
                              float(m.group(2)),
                              m.group(3).strip()))
    return stmts


def run_pgbench(script_file, clients, run_num):
    log = TEMP_DIR / f"bench_{script_file.stem}_c{clients}_run{run_num}.log"
    print(f"[{script_file.stem}] c={clients} run={run_num}")

    cmd = [str(PGBENCH),
           "-c", str(clients), "-j", str(THREADS),
           "-T", str(TIME), "-P", "100000",
           "-n", "-M", "prepared", "-r",
           "-f", str(script_file), "-d", DB]
    if USED_VERSION != "master":
        cmd[-2:-2] = ["-U", "postgres"]

    with log.open("w") as f:
        subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT)

    out = log.read_text(encoding="utf-8", errors="replace")
    stmts = parse_statements(out)
    print(f" -> statements={len(stmts)}")
    return stmts


# =============================================
# Основной цикл
# =============================================

def main():
    print("=" * 41)
    print("Начало работы")
    print("=" * 41)
    print()
    TEMP_DIR.mkdir(exist_ok=True)
    for f in (SCRIPT_FILE, BASELINE_FILE):
        if not f.exists():
            print(f"Нет файла {f}")
            raise SystemExit(1)

    try:
        create_database()
        create_extension()
        print("=" * 41)
        print("БД и расширение созданы")
        print("=" * 41)
        with RESULT_FILE.open("w", newline="") as ft:
            wt = csv.writer(ft)
            wt.writerow(["clients", "run", "stmt_idx",
                         "ext_latency", "base_latency", "net_latency", "stmt_text"])

            for clients in CLIENTS:
                for r in range(1, REPEATS + 1):
                    ext_stmts  = run_pgbench(SCRIPT_FILE,   clients, r)
                    base_stmts = run_pgbench(BASELINE_FILE, clients, r)

                    for i, (ext_lat, _std, ext_txt) in enumerate(ext_stmts):
                        if i >= len(base_stmts):
                            continue
                        base_lat = base_stmts[i][0]
                        wt.writerow([clients, r, i,
                                     f"{ext_lat:.3f}",
                                     f"{base_lat:.3f}",
                                     f"{ext_lat - base_lat:.3f}",
                                     ext_txt])
                    ft.flush()
        
        # =============================================
        # Завершение
        # =============================================
                    
        print()
        print("=" * 41)
        print("Все тесты завершены!")
        print(f"Результаты сохранены в {RESULT_FILE}")
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

