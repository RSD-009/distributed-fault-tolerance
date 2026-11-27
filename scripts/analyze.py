"""
analyze.py - turns OMNeT++ .sca files into a summary table and 3 charts.

Run from the repo root:   python3 scripts/analyze.py
Needs: pandas, matplotlib   (pip install pandas matplotlib)
Reads: results/*.sca        Writes: results/summary.csv and report/figures/*.png
"""
import glob
import os
import re
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

RESULTS = "results"
FIGURES = os.path.join("report", "figures")
N_NODES = 7
THEORY_INFORM = 6            # course formula 2m - n + 1 for the 7-node tree (m = 6)

def configs_from_ini():
    """Config names in the order they appear in simulations/omnetpp.ini."""
    names = []
    path = os.path.join("simulations", "omnetpp.ini")
    if os.path.exists(path):
        with open(path) as f:
            for line in f:
                m = re.match(r"\[Config\s+(\w+)\]", line.strip())
                if m:
                    names.append(m.group(1))
    return names


ORDER = configs_from_ini()


def read_sca(path):
    """One .sca file = one run. Returns (config, run, {scalar name: [values per node]})."""
    config, run, values = None, None, {}
    with open(path) as f:
        for line in f:
            p = line.split()
            if not p:
                continue
            if p[0] == "attr" and p[1] == "configname":
                config = p[2].strip('"')
            elif p[0] == "attr" and p[1] == "runnumber":
                run = int(p[2].strip('"'))
            elif p[0] == "scalar" and len(p) >= 4:
                values.setdefault(p[2], []).append(float(p[3]))
    return config, run, values


def main():
    rows = []
    files = sorted(glob.glob(os.path.join(RESULTS, "*.sca")))
    if not files:
        raise SystemExit("No .sca files in results/. Run the simulations first.")
    for path in files:
        config, run, v = read_sca(path)
        if config is None:
            continue
        informed_at = [t for t in v.get("informedAt", []) if t >= 0]
        n_nodes = len(v.get("informed", [])) or N_NODES      # Mesh has 11 nodes, the others 7
        rows.append({
            "config": config,
            "run": run,
            "informed_nodes": sum(v.get("informed", [])),
            "availability_pct": 100.0 * sum(v.get("informed", [])) / n_nodes,
            "messages_sent": sum(v.get("messagesSent", [])),
            "messages_dropped": sum(v.get("messagesDropped", [])),
            "messages_cut": sum(v.get("messagesCut", [])),
            "messages_ignored": sum(v.get("messagesIgnored", [])),
            "checkpoints_saved": sum(v.get("checkpointsSaved", [])),
            "unreachable_reports": sum(v.get("unreachableNeighbors", [])),
            "completion_time_ms": 1000.0 * max(informed_at) if informed_at else None,
        })
    runs = pd.DataFrame(rows)
    runs.to_csv(os.path.join(RESULTS, "runs.csv"), index=False)

    summary = runs.groupby("config").agg(
        runs=("run", "count"),
        availability_pct=("availability_pct", "mean"),
        availability_std=("availability_pct", "std"),
        messages_sent=("messages_sent", "mean"),
        messages_std=("messages_sent", "std"),
        messages_dropped=("messages_dropped", "mean"),
        checkpoints_saved=("checkpoints_saved", "mean"),
        completion_time_ms=("completion_time_ms", "mean"),
    ).fillna(0)
    ordered = [c for c in ORDER if c in summary.index] + sorted(c for c in summary.index if c not in ORDER)
    summary = summary.reindex(ordered)

    def baseline_of(cfg):
        for pre in ("Ring_", "Star_", "Mesh_"):
            if cfg.startswith(pre):
                return pre + "Baseline"
        return "Baseline"

    # message overhead is measured against the baseline of the SAME topology
    summary["overhead_vs_baseline_pct"] = [
        100.0 * (row.messages_sent - summary.loc[baseline_of(cfg), "messages_sent"]) / summary.loc[baseline_of(cfg), "messages_sent"]
        if baseline_of(cfg) in summary.index and summary.loc[baseline_of(cfg), "messages_sent"] > 0 else float("nan")
        for cfg, row in summary.iterrows()]
    base = summary.loc["Baseline", "messages_sent"] if "Baseline" in summary.index else None
    summary.round(2).to_csv(os.path.join(RESULTS, "summary.csv"))
    print(summary.round(2).to_string())
    print("\nCourse theory for diffuse-v3: %d INFORM messages (2m-n+1); with ACKs: %d total."
          % (THEORY_INFORM, 2 * THEORY_INFORM))

    os.makedirs(FIGURES, exist_ok=True)
    labels = list(summary.index)
    x = range(len(labels))

    # Chart 1: availability
    plt.figure(figsize=(14, 6))
    plt.bar(x, summary["availability_pct"], yerr=summary["availability_std"], capsize=3, color="#3b7dd8")
    plt.axhline(100, color="gray", linestyle="--", linewidth=0.8)
    plt.xticks(x, labels, rotation=60, ha="right")
    plt.ylabel("Availability (% of nodes informed)")
    plt.title("Availability per experiment (mean of runs)")
    plt.tight_layout()
    plt.savefig(os.path.join(FIGURES, "fig1_availability.png"), dpi=150)
    plt.close()

    # Chart 2: messages sent
    plt.figure(figsize=(14, 6))
    plt.bar(x, summary["messages_sent"], yerr=summary["messages_std"], capsize=3, color="#e08a2c")
    if base:
        plt.axhline(base, color="gray", linestyle="--", linewidth=0.8, label="Baseline (%d)" % base)
        plt.legend()
    plt.xticks(x, labels, rotation=60, ha="right")
    plt.ylabel("Messages sent (INFORM + ACK + retries)")
    plt.title("Message overhead per experiment (mean of runs)")
    plt.tight_layout()
    plt.savefig(os.path.join(FIGURES, "fig2_messages.png"), dpi=150)
    plt.close()

    # Chart 3: time to inform every node that was reached (only where it is defined)
    t = summary[summary["completion_time_ms"] > 0]
    plt.figure(figsize=(14, 6))
    plt.bar(range(len(t)), t["completion_time_ms"], color="#4aa36b")
    plt.xticks(range(len(t)), list(t.index), rotation=60, ha="right")
    plt.ylabel("Time of the last INFORMED node (ms)")
    plt.title("Completion time per experiment (mean of runs)")
    plt.tight_layout()
    plt.savefig(os.path.join(FIGURES, "fig3_completion_time.png"), dpi=150)
    plt.close()
    print("\nWrote results/summary.csv, results/runs.csv and 3 PNG files in", FIGURES)


if __name__ == "__main__":
    main()
