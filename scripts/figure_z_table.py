#!/usr/bin/env python3
"""
figure_z_table.py - Table de verite EXACTE de la promesse Z (Projet 5).

On force les 14 scenarios possibles du lien coupe (7 initiateurs x 2 feuilles coupees)
au lieu de les tirer au hasard, avec et sans Z, et on trace le resultat.

Usage, depuis la RACINE du depot :   python3 scripts/figure_z_table.py
Ecrit : results/z_truth_table.csv  et  report/figures/eval_6_table_verite_Z.png
Necessite : le programme compile (distributed-fault-tolerance a la racine), matplotlib.
"""
import csv
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import evaluate as e   # reutilise read_run et summarize
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = os.getcwd()
EXE = os.path.join(ROOT, "distributed-fault-tolerance")
OUT = os.path.join(ROOT, "results", "z_truth")
CONFIGS = [("Link_Down_Replication", "avec Z (diffuse-v3)"),
           ("Link_Down_Replication_NoZ", "sans Z (style v2)")]
LEAVES = (3, 4)       # feuille dont le lien avec son parent 1 est coupe
N = 7


def run_all():
    for c, _ in CONFIGS:
        for i in range(N):
            for l in LEAVES:
                d = os.path.join(OUT, "%s-%d-%d" % (c, i, l))
                subprocess.run([EXE, "-u", "Cmdenv", "-n", "..", "-c", c, "-r", "0",
                                "--**.initiatorId=%d" % i, "--**.partitionNodeId=%d" % l,
                                "--result-dir=" + d, "--cmdenv-express-mode=true", "omnetpp.ini"],
                               cwd=os.path.join(ROOT, "simulations"),
                               stdout=subprocess.DEVNULL, check=True)


def availability(c, i, l):
    path = os.path.join(OUT, "%s-%d-%d" % (c, i, l), "%s-#0.sca" % c)
    cfg, run, nodes = e.read_run(path)
    return e.summarize(cfg, run, nodes)["availability"]


def main():
    if not os.path.exists(EXE):
        sys.exit("Programme introuvable : %s (compile d'abord le projet)" % EXE)
    run_all()
    table = {c: [[availability(c, i, l) for l in LEAVES] for i in range(N)] for c, _ in CONFIGS}

    with open(os.path.join("results", "z_truth_table.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["config", "initiateur", "lien_coupe", "disponibilite_pct"])
        for c, _ in CONFIGS:
            for i in range(N):
                for k, l in enumerate(LEAVES):
                    w.writerow([c, i, "1-%d" % l, round(table[c][i][k], 1)])

    fig, axes = plt.subplots(1, 2, figsize=(10, 5.5))
    for ax, (c, label) in zip(axes, CONFIGS):
        fails = 0
        for i in range(N):
            for k in range(len(LEAVES)):
                a = table[c][i][k]
                bad = a < 100
                fails += bad
                ax.add_patch(plt.Rectangle((k, N - 1 - i), 1, 1,
                                           color="#d9534f" if bad else "#5cb85c"))
                ax.text(k + 0.5, N - 1 - i + 0.5, "%.1f%%" % a, ha="center", va="center",
                        color="white", fontweight="bold")
        ax.set_xlim(0, len(LEAVES))
        ax.set_ylim(0, N)
        ax.set_xticks([k + 0.5 for k in range(len(LEAVES))])
        ax.set_xticklabels(["lien 1-%d coupe" % l for l in LEAVES])
        ax.set_yticks([N - 1 - i + 0.5 for i in range(N)])
        ax.set_yticklabels(["initiateur %d" % i for i in range(N)])
        ax.set_title("%s\n%d echecs / %d cas = %.1f%%" % (label, fails, N * len(LEAVES),
                                                           100.0 * fails / (N * len(LEAVES))))
    fig.suptitle("Promesse Z : quand le lien de secours est neutralise (vert = tous informes)")
    os.makedirs(e.FIGS, exist_ok=True)
    plt.tight_layout()
    out = os.path.join(e.FIGS, "eval_6_table_verite_Z.png")
    plt.savefig(out, dpi=150)
    print("figure :", out)
    for c, label in CONFIGS:
        fails = sum(1 for row in table[c] for a in row if a < 100)
        print("%-28s echecs : %d / %d" % (c, fails, N * len(LEAVES)))


if __name__ == "__main__":
    main()
