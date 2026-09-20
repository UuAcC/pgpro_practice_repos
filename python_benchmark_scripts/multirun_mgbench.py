import csv
import re
import subprocess
import time
from pathlib import Path
import sys

# =============================================
# Настройки
# =============================================

DB = "pgbench_test"
USED_VERSION = sys.argv[1]
TIME = 60
THREADS = 2
BUFFERS_SIZES = ["512MB", "1GB"]
CLIENTS = [1, 2, 4, 8, 16, 32, 64, 128]
REPEATS = 3

HOME = Path.home()
DATA_DIR = HOME / "postgres" / "data" / USED_VERSION / "debug"
LOG_DIR = HOME / "postgres" / "log" / USED_VERSION / "debug"
INSTALL_DIR = HOME / "postgres" / "install" / USED_VERSION / "debug"

PG_CTL = INSTALL_DIR / "bin" / "pg_ctl"
PGBENCH = INSTALL_DIR / "bin" / "pgbench"
PSQL = INSTALL_DIR / "bin" / "psql"

RESULT_FILE = Path(f"results_{USED_VERSION}_unknown.csv")
TEMP_DIR = Path("temp_results")
CONF_FILE = DATA_DIR / "postgresql.conf"
PG_LOG = LOG_DIR / "postgres.log"

RE_TPS = re.compile(r"tps\s*=\s*([\d.]+)")
RE_LAT_AVG = re.compile(r"latency average\s*=\s*([\d.]+)")
RE_LAT_STDDEV = re.compile(r"latency stddev\s*=\s*([\d.]+)")


# =============================================
# Вспомогательные функции
# =============================================

def run_cmd(cmd):
    """Запустить команду и дождаться завершения. Возвращает CompletedProcess."""
    return subprocess.run(cmd, capture_output=True, text=True)


def extract(regex, text):
    """Получить первое совпавшее с регулярным выражением, иначе ошибка."""
    m = regex.search(text)
    return m.group(1) if m else "ERROR"


# =============================================
# Функция для изменения shared_buffers
# =============================================

def set_shared_buffers(size):
    print("=" * 41)
    print(f"Устанавливаем shared_buffers = {size}")
    print("=" * 41)

    original = CONF_FILE.read_text(encoding="utf-8")
    backup = CONF_FILE.with_name(CONF_FILE.name + ".bak")
    backup.write_text(original, encoding="utf-8")

    new_text = re.sub(
        r"^shared_buffers =.*$",
        f"shared_buffers = {size}",
        original,
        flags=re.MULTILINE,
    )
    CONF_FILE.write_text(new_text, encoding="utf-8")

    run_cmd([str(PG_CTL), "-D", str(DATA_DIR), "-l", str(PG_LOG), "restart"])
    time.sleep(2)

    if USED_VERSION == "master":
        cp = run_cmd([str(PSQL), "-d", "postgres", "-t", "-c", "SHOW shared_buffers;"])
    else:
        cp = run_cmd([str(PSQL), "-d", "postgres", "-t", "-c", "-U", "postgres", "SHOW shared_buffers;"])
    actual = cp.stdout.strip().replace(" ", "")
    print(f"Проверка: shared_buffers = {actual}")


# =============================================
# Функция для запуска одного теста и извлечения результатов
# =============================================

def run_and_extract(buffers, clients, run_num):
    temp_log_file = TEMP_DIR / f"bench_{buffers}_c{clients}_run{run_num}.log"
    print(f"Запуск: buffers={buffers}, clients={clients}, run={run_num}")

    with temp_log_file.open("w", encoding="utf-8") as temp_log:
        if USED_VERSION == "master":
            subprocess.run(
                [
                    str(PGBENCH),
                    "-c", str(clients),
                    "-j", str(THREADS),
                    "-T", str(TIME),
                    "-P", "100000",
                    "-d", DB
                ],
                stdout=temp_log,
                stderr=subprocess.STDOUT,
            )
        else:
            subprocess.run(
                [
                    str(PGBENCH),
                    "-c", str(clients),
                    "-j", str(THREADS),
                    "-T", str(TIME),
                    "-U", "postgres",
                    "-P", "100000",
                    "-d", DB
                ],
                stdout=temp_log,
                stderr=subprocess.STDOUT,
            )

    output = temp_log_file.read_text(encoding="utf-8", errors="replace")

    tps = extract(RE_TPS, output)
    latency_avg = extract(RE_LAT_AVG, output)
    latency_stddev = extract(RE_LAT_STDDEV, output)

    print(f" -> TPS: {tps}, latency: {latency_avg} ms")

    return [buffers, clients, run_num, tps, latency_avg, latency_stddev]


# =============================================
# Основной цикл
# =============================================

def main():
    TEMP_DIR.mkdir(exist_ok=True)

    with RESULT_FILE.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["buffers", "clients", "run",
                         "tps", "latency_avg", "latency_stddev"])

        print("Начинаем нагрузочное тестирование")
        print(f"Результаты будут сохранены в {RESULT_FILE}")
        print()

        for buffers in BUFFERS_SIZES:
            set_shared_buffers(buffers)
            for clients in CLIENTS:
                for run_num in range(1, REPEATS + 1):
                    row = run_and_extract(buffers, clients, run_num)
                    writer.writerow(row)
                    f.flush()

    # =============================================
    # Завершение
    # =============================================
    
    print()
    print("=" * 41)
    print("Все тесты завершены!")
    print(f"Результаты сохранены в {RESULT_FILE}")
    print("=" * 41)
    print()
    print("Первые 10 строк результата:")
    with RESULT_FILE.open(encoding="utf-8") as f:
        for i, line in enumerate(f):
            if i >= 10:
                break
            print(line, end="")




if __name__ == "__main__":
    main()
