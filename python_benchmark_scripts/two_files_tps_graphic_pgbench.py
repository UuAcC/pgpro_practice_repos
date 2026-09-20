import pandas as pd
import matplotlib.pyplot as plt
import sys

# =============================================
# 1. Настройки
# =============================================

USED_VERSION = sys.argv[1]
FILES = {
    "pg_stat_statements OFF": f"results_{USED_VERSION}_base.csv",
    "pg_stat_statements ON": f"results_{USED_VERSION}_exc.csv",
}

FILE_COLORS = {
    "pg_stat_statements OFF": "tab:blue",
    "pg_stat_statements ON": "tab:orange",
}

BUFFER_STYLES = {
    "512MB":  {"linestyle": "--", "marker": "o", "label_suffix": " (512MB)"},
    "1GB": {"linestyle": "-",  "marker": "s", "label_suffix": " (1GB)"},
}

# =============================================
# 2. Построение графика
# =============================================

fig, ax = plt.subplots(figsize=(9, 6))

for file_label, filename in FILES.items():
    df = pd.read_csv(filename)
    df["tps"] = pd.to_numeric(df["tps"], errors="coerce")
    df = df.dropna(subset=["tps"])
    avg = df.groupby(["buffers", "clients"], as_index=False)["tps"].mean()

    color = FILE_COLORS.get(file_label, None)

    for buffers_value, group in avg.groupby("buffers"):
        style = BUFFER_STYLES.get(
            buffers_value,
            {"linestyle": "-", "marker": "x", "label_suffix": f" ({buffers_value})"},
        )
        group = group.sort_values("clients")

        ax.plot(
            group["clients"],
            group["tps"],
            linestyle=style["linestyle"],
            marker=style["marker"],
            color=color,
            linewidth=1.8,
            markersize=6,
            label=f"{file_label}{style['label_suffix']}",
        )

# =============================================
# 3. Оформление
# =============================================

ax.set_xlabel("Число клиентов (clients)")
ax.set_ylabel("Среднее TPS")
ax.set_title(f"График TPS, версия {USED_VERSION}")

ax.set_xscale("log", base=2)
ax.set_xticks([1, 2, 4, 8, 16, 32, 64, 128])
ax.set_xticklabels([1, 2, 4, 8, 16, 32, 64, 128])

ax.grid(True, which="both", linestyle="--", alpha=0.5)
ax.legend()
plt.tight_layout()

# =============================================
# 4. Сохранение и показ
# =============================================

plt.savefig(f"tps_plot_{USED_VERSION}.png", dpi=200)
plt.show()
