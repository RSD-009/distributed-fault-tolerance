#!/usr/bin/env python3
"""
evaluate.py - Evaluation du Projet 5 (tache de Mankour).

Ce que le script fait, dans l'ordre :
  1. lit chaque fichier .sca (= un run) et garde les valeurs PAR NOEUD
     (initiateur, victime, informed, crashed, messages...) ;
  2. VALIDE le simulateur contre la theorie du cours (bornes 2m-n+1) ;
  3. compare chaque faute "sans / avec mecanisme" run par run (comparaison APPARIEE) ;
  4. mesure un delai de recovery apparie au Baseline (en ms) ;
  5. ecrit les CSV, un fichier de tableaux Markdown et 5 figures.

Usage, depuis la RACINE du depot :   python3 scripts/evaluate.py
Lit    : results/*.sca   et   simulations/omnetpp.ini
Ecrit  : results/runs_detail.csv, results/eval_summary.csv, results/eval_pairs.csv,
         report/evaluation_tables.md, report/figures/eval_*.png
Necessite : pandas, matplotlib
"""
import glob
import math
import os
import re
import sys

import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

RESULTS = "results"
FIGS = os.path.join("report", "figures")
INI = os.path.join("simulations", "omnetpp.ini")
NODE_RE = re.compile(r"node\[(\d+)\]")

# (n noeuds, m liens) de chaque topologie, pour la borne du cours 2m - n + 1
TOPO = {"Tree": (7, 6), "Ring": (7, 7), "Star": (7, 6), "Mesh": (11, 31)}

# (config SANS, config AVEC, titre court) : le delta est toujours AVEC - SANS
PAIRS = [
    ("Node_Reboot_Cold", "Node_Reboot_Checkpoint", "Reboot : Cold -> Checkpoint"),
    ("Node_Transient_NoRetry", "Node_Transient_Retransmit", "Transitoire : NoRetry -> Retransmit"),
    ("Link_MsgLoss_NoRetry", "Link_MsgLoss_Retransmit", "Perte 30% : NoRetry -> Retransmit"),
    ("Link_Down_Temporary_NoRetry", "Link_Down_Temporary", "Lien coupe court : NoRetry -> Retransmit"),
    ("Link_Down_Retransmit", "Link_Down_Replication", "Lien coupe : Retransmit -> Replication"),
    ("Link_Down_Replication", "Link_Down_Replication_NoZ", "Lien coupe : avec Z -> sans Z"),
    ("Mesh_Baseline", "Mesh_Baseline_NoZ", "Maillage : avec Z -> sans Z"),
]

T95 = {1: 12.706, 2: 4.303, 3: 3.182, 4: 2.776, 5: 2.571, 6: 2.447, 7: 2.365,
       8: 2.306, 9: 2.262, 14: 2.145, 19: 2.093, 29: 2.045}


def t95(df):
    ok = [k for k in T95 if k <= df]
    return T95[max(ok)] if ok else 1.96


# ----------------------------------------------------------------------------
# 1. LECTURE
# ----------------------------------------------------------------------------
def ini_info():
    """Noms des configs (dans l'ordre de l'ini) et valeur de repeat."""
    names, repeat = [], 1
    if os.path.exists(INI):
        with open(INI) as f:
            for line in f:
                m = re.match(r"\[Config\s+(\w+)\]", line.strip())
                if m:
                    names.append(m.group(1))
                m = re.match(r"repeat\s*=\s*(\d+)", line.strip())
                if m:
                    repeat = int(m.group(1))
    return names, repeat


def read_run(path):
    """Un .sca = un run. Renvoie (config, run, {indice_noeud: {scalaire: valeur}})."""
    config, run, nodes = None, None, {}
    with open(path) as f:
        for line in f:
            p = line.split()
            if len(p) < 3:
                continue
            if p[0] == "attr" and p[1] == "configname":
                config = p[2].strip('"')
            elif p[0] == "attr" and p[1] == "runnumber":
                run = int(p[2].strip('"'))
            elif p[0] == "scalar" and len(p) >= 4:
                m = NODE_RE.search(p[1])
                if m:
                    nodes.setdefault(int(m.group(1)), {})[p[2]] = float(p[3])
    return config, run, nodes


def topology_of(config):
    for pre in ("Ring", "Star", "Mesh"):
        if config.startswith(pre + "_"):
            return pre
    return "Tree"


def summarize(config, run, nodes):
    """Resume un run : une ligne de runs_detail.csv."""
    n = len(nodes)
    val = lambda k: [v.get(k, 0.0) for v in nodes.values()]
    alive = [v for v in nodes.values() if v.get("crashed", 0) == 0]
    init = [i for i, v in nodes.items() if v.get("wasInitiator", 0) == 1]
    vict = [i for i, v in nodes.items() if v.get("wasCrashTarget", 0) == 1]
    times = [v["informedAt"] for v in nodes.values() if v.get("informedAt", -1) >= 0]
    informed = sum(val("informed"))
    rec_ms = float("nan")
    if vict:
        v = nodes[vict[0]]
        if (v.get("crashed", 0) == 0 and v.get("recoveredAt", -1) >= 0
                and v.get("crashedAt", -1) >= 0 and v.get("informed", 0) == 1):
            # de nouveau operationnel = reboot fait ET de nouveau informe
            rec_ms = 1000.0 * (max(v["recoveredAt"], v.get("informedAt", -1)) - v["crashedAt"])
    det = [1000.0 * (v["unreachableAt"] - v["informedAt"]) for v in nodes.values()
           if v.get("unreachableAt", -1) >= 0 and v.get("informedAt", -1) >= 0]
    det_ms = sum(det) / len(det) if det else float("nan")
    return {
        "config": config, "run": run, "topo": topology_of(config), "n": n,
        "initiator": init[0] if init else -1,
        "victim": vict[0] if vict else -1,
        "victim_informed": nodes[vict[0]].get("informed", 0) if vict else float("nan"),
        "informed": informed,
        "availability": 100.0 * informed / n,
        # disponibilite parmi les noeuds NON crashes en fin de run
        "availability_alive": (100.0 * sum(v.get("informed", 0) for v in alive) / len(alive))
                              if alive else float("nan"),
        "crashed_nodes": sum(val("crashed")),
        "messages": sum(val("messagesSent")),
        "dropped": sum(val("messagesDropped")),
        "cut": sum(val("messagesCut")),
        "ignored": sum(val("messagesIgnored")),
        "checkpoints": sum(val("checkpointsSaved")),
        "unreachable": sum(val("unreachableNeighbors")),
        "completion_ms": 1000.0 * max(times) if times else float("nan"),
        "recovery_ms": rec_ms,
        "detection_ms": det_ms,
    }


# ----------------------------------------------------------------------------
# 2. STATISTIQUES
# ----------------------------------------------------------------------------
def summary_table(runs, order):
    g = runs.groupby("config")
    s = g.agg(runs=("run", "count"),
              avail=("availability", "mean"), avail_sd=("availability", "std"),
              avail_alive=("availability_alive", "mean"), alive_sd=("availability_alive", "std"),
              msgs=("messages", "mean"), msgs_sd=("messages", "std"),
              unreach=("unreachable", "mean"), ckpt=("checkpoints", "mean"),
              t_ms=("completion_ms", "mean")).fillna(0)
    s = s.reindex([c for c in order if c in s.index] + sorted(c for c in s.index if c not in order))

    def base(c):
        b = (topology_of(c) + "_Baseline") if topology_of(c) != "Tree" else "Baseline"
        return b if b in s.index else None
    s["overhead_pct"] = [100.0 * (r.msgs - s.loc[base(c), "msgs"]) / s.loc[base(c), "msgs"]
                         if base(c) and s.loc[base(c), "msgs"] > 0 else float("nan")
                         for c, r in s.iterrows()]
    return s


def paired_stats(runs, a, b, metric):
    """Delta (b - a) run par run. Verifie que l'initiateur est le meme (appariement valide)."""
    A = runs[runs.config == a].set_index("run")
    B = runs[runs.config == b].set_index("run")
    common = A.index.intersection(B.index)
    if len(common) == 0:
        return None
    pairing_ok = bool((A.loc[common, "initiator"] == B.loc[common, "initiator"]).all())
    d = (B.loc[common, metric] - A.loc[common, metric]).dropna()
    n = len(d)
    sd = d.std() if n > 1 else 0.0
    return {"metric": metric, "n": n, "mean_delta": d.mean(), "sd": sd,
            "ic95": t95(n - 1) * sd / math.sqrt(n) if n > 1 else 0.0,
            "wins": int((d > 0).sum()), "ties": int((d == 0).sum()), "losses": int((d < 0).sum()),
            "pairing_ok": pairing_ok}


def recovery_table(runs):
    """Delai de recovery = fin de diffusion (faute) - fin de diffusion (Baseline), meme run.
    Valable seulement pour les mecanismes a retransmission, et seulement si tous les noeuds
    sont informes. Pour le reboot il faudrait un scalaire recoveredAt (absent du code)."""
    if "Baseline" not in set(runs.config):
        return pd.DataFrame()
    base = runs[runs.config == "Baseline"].set_index("run")
    out = []
    for cfg in ["Node_Transient_Retransmit", "Link_Down_Temporary", "Link_MsgLoss_Retransmit"]:
        r = runs[runs.config == cfg].set_index("run")
        d = []
        for k in r.index.intersection(base.index):
            if r.loc[k, "initiator"] == base.loc[k, "initiator"] and r.loc[k, "availability"] == 100:
                x = r.loc[k, "completion_ms"] - base.loc[k, "completion_ms"]
                if not math.isnan(x):
                    d.append(x)
        imp = [x for x in d if x > 0]
        out.append({"config": cfg, "runs_valides": len(d), "runs_impactes": len(imp),
                    "delai_moyen_ms": sum(d) / len(d) if d else float("nan"),
                    "delai_moyen_impactes_ms": sum(imp) / len(imp) if imp else float("nan"),
                    "delai_max_ms": max(d) if d else float("nan")})
    return pd.DataFrame(out)


# ----------------------------------------------------------------------------
# 3. VALIDATION CONTRE LA THEORIE
# ----------------------------------------------------------------------------
def run_checks(runs, nfiles, order, repeat, pair_rows):
    res = []
    add = lambda kind, ok, txt: res.append((kind, ok, txt))
    sub = lambda c: runs[runs.config == c] if c in set(runs.config) else None

    add("HARD", nfiles == len(order) * repeat,
        "nombre de .sca = %d (attendu %d configs x %d runs = %d)"
        % (nfiles, len(order), repeat, len(order) * repeat))

    # messagesSent = INFORM + ACK ; sans perte chaque INFORM a son ACK
    for cfg, expected in [("Baseline", 12), ("Star_Baseline", 12), ("Ring_Baseline", 16),
                          ("Mesh_Baseline_NoZ", 104)]:
        s = sub(cfg)
        if s is not None:
            add("HARD", bool((s.messages == expected).all() and (s.availability == 100).all()),
                "%s : 100%% informes et exactement %d messages a chaque run (vu : min=%d max=%d)"
                % (cfg, expected, s.messages.min(), s.messages.max()))
    s = sub("Mesh_Baseline")
    if s is not None:
        add("HARD", bool((s.availability == 100).all() and (s.messages <= 104).all()),
            "Mesh_Baseline : 100%% informes et <= 104 messages (moyenne vue : %.1f)" % s.messages.mean())

    s = sub("Baseline")
    if s is not None:
        add("HARD", s.initiator.nunique() >= 3,
            "initiateur aleatoire : %d valeurs distinctes sur %d runs" % (s.initiator.nunique(), len(s)))

    for cfg, txt in [("Node_Transient_Retransmit", "100% a chaque run (la sourdine finit avant la derniere retransmission)"),
                     ("Node_Reboot_Checkpoint", "100% a chaque run (etat restaure ou retransmission dans la fenetre de 200 ms)"),
                     ("Link_Down_Replication_NoZ", "100% a chaque run (flooding sans promesse Z)")]:
        s = sub(cfg)
        if s is not None:
            add("PRED", bool((s.availability == 100).all()),
                "%s : %s (vu : min=%.1f%%)" % (cfg, txt, s.availability.min()))

    cold, ck = sub("Node_Reboot_Cold"), sub("Node_Reboot_Checkpoint")
    if cold is not None and ck is not None:
        lost = int((cold.victim_informed == 0).sum())
        wins = [p for p in pair_rows if p["sans"] == "Node_Reboot_Cold" and p["metric"] == "availability"]
        if wins:
            add("PRED", wins[0]["wins"] == lost,
                "Reboot : nb de runs ou Checkpoint gagne (%d) = nb de runs ou Cold perd l'etat (%d)"
                % (wins[0]["wins"], lost))

    s = sub("Node_Crash_Timeout")
    if s is not None and s.detection_ms.notna().any():
        d = s.detection_ms.dropna()
        add("PRED", bool(((d - 200).abs() < 1).all()),
            "Node_Crash_Timeout : detection = (maxRetries+1) x timeout = 200 ms (vu : min=%.1f max=%.1f)"
            % (d.min(), d.max()))

    for p in pair_rows:
        if p["metric"] == "availability":
            add("HARD", p["pairing_ok"], "appariement valide (meme initiateur run par run) : %s" % p["titre"])
    return res


# ----------------------------------------------------------------------------
# 4. SORTIES
# ----------------------------------------------------------------------------
def fmt(v):
    if isinstance(v, float):
        return "-" if math.isnan(v) else "%.1f" % v
    return str(v)


def md_table(df):
    cols = list(df.columns)
    lines = ["| " + " | ".join(cols) + " |", "|" + "|".join("---" for _ in cols) + "|"]
    for _, r in df.iterrows():
        lines.append("| " + " | ".join(fmt(v) for v in r) + " |")
    return "\n".join(lines)


def savefig(name):
    os.makedirs(FIGS, exist_ok=True)
    plt.tight_layout()
    plt.savefig(os.path.join(FIGS, name), dpi=150)
    plt.close()
    print("  figure :", os.path.join(FIGS, name))


# FIGURES V2 : barres d'erreur bornees a 100 %, valeurs ecrites, legendes hors des barres
COL_A, COL_B = "#9aa0a6", "#3b7dd8"
NAMES_FR = {"Tree": "Arbre", "Ring": "Anneau", "Star": "Étoile", "Mesh": "Maillage (n=11)"}


def _clip_err(means, sds, top=100.0):
    # erreurs asymetriques : la barre haute s'arrete a 100 % (une disponibilite ne depasse pas 100)
    return [list(sds), [min(sd, max(top - m, 0.0)) for m, sd in zip(means, sds)]]


def _label(ax, xs, vals, fmt, ups=None):
    for i, (x, v) in enumerate(zip(xs, vals)):
        up = ups[i] if ups else 0.0
        ax.annotate(fmt % v, (x, v + up), xytext=(0, 3), textcoords="offset points",
                    ha="center", fontsize=8)


def _save(fig, name):
    os.makedirs(FIGS, exist_ok=True)
    path = os.path.join(FIGS, name)
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print("  figure :", path)


def fig_pairs(s, metric, sd, ylabel, title, name, pairs):
    avail = [(a, b, t) for a, b, t in pairs if a in s.index and b in s.index]
    if not avail:
        return
    pct = metric == "avail"
    fig, ax = plt.subplots(figsize=(13, 6))
    ticks = []
    for i, (a, b, t) in enumerate(avail):
        x = i * 3
        for dx, c, col, lab in ((0, a, COL_A, "avant la flèche (sans mécanisme, ou avec Z)"),
                                (1, b, COL_B, "après la flèche (avec mécanisme, ou sans Z)")):
            m, d = s.loc[c, metric], s.loc[c, sd]
            err = _clip_err([m], [d]) if pct else [[d], [d]]
            ax.bar(x + dx, m, yerr=err, capsize=3, color=col, label=lab if i == 0 else None)
            _label(ax, [x + dx], [m], "%.1f", ups=[err[1][0]])
        ticks.append((x + 0.5, t.replace(" : ", "\n")))
    top = max(s.loc[c, metric] + s.loc[c, sd] for a, b, _ in avail for c in (a, b))
    ax.set_ylim(0, 115 if pct else top * 1.12)
    ax.set_xticks([p for p, _ in ticks])
    ax.set_xticklabels([tt for _, tt in ticks], fontsize=9)
    ax.set_ylabel(ylabel)
    ax.set_title(title, pad=20)
    if pct:
        ax.axhline(100, color="gray", linestyle="--", linewidth=0.8)
        note = "Disponibilité = nœuds informés / n (nœud crashé compris). Barres d'erreur : écart-type sur 10 runs, bornées à 100 %."
    else:
        note = "messagesSent = INFORM + ACK + retransmissions effectivement envoyés : un run qui atteint moins de nœuds envoie aussi moins de messages."
    ax.text(0.5, 1.01, note, transform=ax.transAxes, ha="center", va="bottom", fontsize=8, color="dimgray")
    ax.legend(loc="upper center", bbox_to_anchor=(0.5, -0.14), ncol=2, frameon=False)
    _save(fig, name)


def fig_topologies(s):
    names, bound, seen = [], [], []
    for t, (n, m) in TOPO.items():
        c = "Baseline" if t == "Tree" else t + "_Baseline"
        if c in s.index:
            names.append(t)
            bound.append(2 * (2 * m - n + 1))
            seen.append(s.loc[c, "msgs"])
    if not names:
        return
    fig, ax = plt.subplots(1, 2, figsize=(12.5, 5.4))
    x = list(range(len(names)))
    ax[0].bar([i - 0.2 for i in x], seen, 0.4, color="#e08a2c", label="mesuré (INFORM + ACK)")
    ax[0].bar([i + 0.2 for i in x], bound, 0.4, color=COL_A, label="2 × (2m − n + 1)  [borne du cours]")
    _label(ax[0], [i - 0.2 for i in x], seen, "%.1f")
    _label(ax[0], [i + 0.2 for i in x], bound, "%.0f")
    ax[0].set_xticks(x)
    ax[0].set_xticklabels([NAMES_FR[t] for t in names])
    ax[0].set_ylabel("messages envoyés")
    ax[0].set_ylim(0, max(bound) * 1.15)
    ax[0].set_title("Sans panne : coût en messages par topologie")
    ax[0].legend(loc="upper left", frameon=False)

    cn, ca, cs = [], [], []
    for t in TOPO:
        c = "Node_Crash_Timeout" if t == "Tree" else t + "_Node_Crash_Timeout"
        if c in s.index:
            cn.append(NAMES_FR[t])
            ca.append(s.loc[c, "avail_alive"])
            cs.append(s.loc[c, "alive_sd"] if "alive_sd" in s.columns else 0.0)
    err = _clip_err(ca, cs)
    ax[1].bar(cn, ca, yerr=err, capsize=3, color=COL_B)
    _label(ax[1], list(range(len(cn))), ca, "%.1f %%", ups=err[1])
    ax[1].axhline(100, color="gray", linestyle="--", linewidth=0.8)
    ax[1].set_ylim(0, 115)
    ax[1].set_ylabel("nœuds non crashés informés (%)")
    ax[1].set_title("Crash d'un nœud aléatoire (10 runs) : effet de la topologie")
    _save(fig, "eval_3_topologies.png")


def fig_z(s):
    have_a = {"Link_Down_Replication", "Link_Down_Replication_NoZ"} <= set(s.index)
    have_b = {"Mesh_Baseline", "Mesh_Baseline_NoZ"} <= set(s.index)
    if not (have_a or have_b):
        return
    cols = ["#e08a2c", "#4aa36b"]
    fig, ax = plt.subplots(1, 2, figsize=(11.5, 5.4))
    csv_path = os.path.join(RESULTS, "z_truth_table.csv")
    if os.path.exists(csv_path):
        # table de verite exacte (figure_z_table.py) : plus parlante que 10 runs tires au hasard
        t = pd.read_csv(csv_path)
        rates = [100.0 * (t[t.config == c].disponibilite_pct < 100).mean()
                 for c in ("Link_Down_Replication", "Link_Down_Replication_NoZ")]
        ax[0].bar(["avec Z (v3)", "sans Z (v2)"], rates, color=cols)
        _label(ax[0], [0, 1], rates, "%.1f %%")
        ax[0].set_ylim(0, 40)
        ax[0].set_ylabel("cas en échec (%)")
        ax[0].set_title("Robustesse : lien coupé + liens de secours\n14 cas exacts (7 initiateurs × 2 liens)")
    elif have_a:
        a = ["Link_Down_Replication", "Link_Down_Replication_NoZ"]
        means = [s.loc[c, "avail"] for c in a]
        err = _clip_err(means, [s.loc[c, "avail_sd"] for c in a])
        ax[0].bar(["avec Z (v3)", "sans Z (v2)"], means, yerr=err, capsize=3, color=cols)
        _label(ax[0], [0, 1], means, "%.1f %%", ups=err[1])
        ax[0].set_ylim(0, 115)
        ax[0].set_ylabel("disponibilité (%)")
        ax[0].set_title("Robustesse : lien coupé + liens de secours (10 runs tirés)")
    if have_b:
        a = ["Mesh_Baseline", "Mesh_Baseline_NoZ"]
        vals = [s.loc[c, "msgs"] for c in a]
        sds = [s.loc[c, "msgs_sd"] for c in a]
        ax[1].bar(["avec Z (v3)", "sans Z (v2)"], vals, yerr=sds, capsize=3, color=cols)
        _label(ax[1], [0, 1], vals, "%.1f", ups=sds)
        ax[1].axhline(104, color="gray", linestyle="--", linewidth=0.8, label="borne v2 : 2 × 52 = 104")
        ax[1].set_ylim(0, 125)
        ax[1].set_ylabel("messages envoyés")
        ax[1].set_title("Coût : maillage sans panne")
        ax[1].legend(loc="upper left", frameon=False)
    fig.suptitle("Le prix de la robustesse : la promesse Z", y=1.02)
    _save(fig, "eval_4_prix_de_Z.png")


def fig_recovery(rec):
    # un point par run (pas seulement une moyenne) : on voit les delais par paliers de timeout
    path = os.path.join(RESULTS, "runs_detail.csv")
    if not os.path.exists(path):
        return
    runs = pd.read_csv(path)
    if "Baseline" not in set(runs.config):
        return
    base = runs[runs.config == "Baseline"].set_index("run")
    cfgs = [("Node_Transient_Retransmit", "Transitoire\n(nœud sourd)"),
            ("Link_Down_Temporary", "Lien coupé\n4 à 70 ms"),
            ("Link_MsgLoss_Retransmit", "Perte 30 %")]
    fig, ax = plt.subplots(figsize=(8.5, 5.6))
    shown = []
    for j, (cfg, lab) in enumerate(cfgs):
        r = runs[runs.config == cfg].set_index("run")
        d = []
        for k in r.index.intersection(base.index):
            if r.loc[k, "initiator"] == base.loc[k, "initiator"] and r.loc[k, "availability"] == 100:
                x = r.loc[k, "completion_ms"] - base.loc[k, "completion_ms"]
                if x == x:
                    d.append(float(x))
        if not d:
            continue
        d.sort()
        jit = [(i - (len(d) - 1) / 2) * 0.04 for i in range(len(d))]
        ax.scatter([j + e for e in jit], d, color="#4aa36b", edgecolor="black", s=45, zorder=3)
        mean = sum(d) / len(d)
        ax.hlines(mean, j - 0.3, j + 0.3, color="black", linewidth=2, zorder=4)
        ax.text(j + 0.32, mean, "moy. %.0f ms" % mean, va="center", fontsize=9)
        shown.append(lab)
    for k in (50, 100, 150):
        ax.axhline(k, linestyle="--", color="gray", linewidth=0.8)
        ax.text(len(cfgs) - 0.45, k + 2, "%d timeout(s) = %d ms" % (k // 50, k), fontsize=8, color="dimgray")
    ax.set_xticks(range(len(cfgs)))
    ax.set_xticklabels([lab for _, lab in cfgs])
    ax.set_ylim(-10, 180)
    ax.set_xlim(-0.5, len(cfgs) - 0.2)
    ax.set_ylabel("fin de diffusion − fin de diffusion du Baseline (ms)")
    ax.set_title("Délai de recovery par run (même run que le Baseline)", pad=18)
    ax.text(0.5, 1.01, "Un point = un run ; 0 ms = run non touché par la faute ; trait noir = moyenne",
            transform=ax.transAxes, ha="center", va="bottom", fontsize=8, color="dimgray")
    _save(fig, "eval_5_recovery.png")


# ----------------------------------------------------------------------------
def main():
    order, repeat = ini_info()
    files = sorted(glob.glob(os.path.join(RESULTS, "*.sca")))
    if not files:
        sys.exit("Aucun .sca dans results/. Lance d'abord la campagne.")
    rows = []
    for path in files:
        cfg, run, nodes = read_run(path)
        if cfg and nodes:
            rows.append(summarize(cfg, run, nodes))
    runs = pd.DataFrame(rows)
    runs.to_csv(os.path.join(RESULTS, "runs_detail.csv"), index=False)
    s = summary_table(runs, order)
    s.round(2).to_csv(os.path.join(RESULTS, "eval_summary.csv"))

    pair_rows = []
    for a, b, t in PAIRS:
        for metric in ("availability", "messages"):
            r = paired_stats(runs, a, b, metric)
            if r:
                r.update({"titre": t, "sans": a, "avec": b})
                pair_rows.append(r)
    pairs = pd.DataFrame(pair_rows)
    if not pairs.empty:
        pairs.round(2).to_csv(os.path.join(RESULTS, "eval_pairs.csv"), index=False)
    rec = recovery_table(runs)

    print("=" * 70 + "\nVALIDATION CONTRE LA THEORIE\n" + "=" * 70)
    checks = run_checks(runs, len(files), order, repeat, pair_rows)
    for kind, ok, txt in checks:
        print("[%s] %-4s %s" % ("PASS" if ok else "FAIL", kind, txt))
    nfail = sum(1 for _, ok, _ in checks if not ok)
    print("\n%d verification(s) en echec sur %d.  HARD = derive du code/cours ; PRED = prediction a confirmer." %
          (nfail, len(checks)))

    print("\n" + "=" * 70 + "\nRESUME PAR CONFIG (moyenne sur les runs)\n" + "=" * 70)
    print(s.round(1).to_string())
    if not pairs.empty:
        print("\n" + "=" * 70 + "\nCOMPARAISONS APPARIEES (delta = AVEC - SANS, run par run)\n" + "=" * 70)
        print(pairs[["titre", "metric", "n", "mean_delta", "sd", "ic95", "wins", "ties", "losses",
                     "pairing_ok"]].round(2).to_string(index=False))
    if not rec.empty:
        print("\n" + "=" * 70 + "\nDELAI DE RECOVERY APPARIE AU BASELINE\n" + "=" * 70)
        print(rec.round(1).to_string(index=False))
    for cfgs, title, col in [
            (["Node_Reboot_Cold", "Node_Reboot_Checkpoint"],
             "RECOVERY DU NOEUD REBOOTE (crash -> de nouveau informe), ms", "recovery_ms"),
            (["Node_Crash_Timeout", "Ring_Node_Crash_Timeout", "Star_Node_Crash_Timeout",
              "Mesh_Node_Crash_Timeout"],
             "TEMPS DE DETECTION DU CRASH (envoi -> UNREACHABLE), ms", "detection_ms")]:
        sel = runs[runs.config.isin(cfgs)]
        if len(sel) and sel[col].notna().any():
            print("\n" + "=" * 70 + "\n" + title + "\n" + "=" * 70)
            print(sel.groupby("config").agg(runs=("run", "count"), valides=(col, "count"),
                                            moyenne=(col, "mean"), min=(col, "min"),
                                            max=(col, "max")).round(1).to_string())

    cold = runs[runs.config == "Node_Reboot_Cold"]
    if len(cold):
        lost = int((cold.victim_informed == 0).sum())
        print("\nReboot a froid : la victime a PERDU l'etat (cas B) dans %d/%d runs ; "
              "dans les autres, une retransmission l'a rattrapee (cas A)." % (lost, len(cold)))

    with open(os.path.join("report", "evaluation_tables.md"), "w") as f:
        f.write("## Resume par config\n\n" + md_table(s.round(1).reset_index()) + "\n\n")
        if not pairs.empty:
            f.write("## Comparaisons appariees (AVEC - SANS)\n\n" +
                    md_table(pairs[["titre", "metric", "n", "mean_delta", "sd", "ic95",
                                    "wins", "ties", "losses"]].round(2)) + "\n\n")
        if not rec.empty:
            f.write("## Delai de recovery apparie\n\n" + md_table(rec.round(1)) + "\n")

    print("\nFigures :")
    fig_pairs(s, "avail", "avail_sd", "disponibilite (% de noeuds informes)",
              "Disponibilite : sans / avec mecanisme (moyenne +- ecart-type)", "eval_1_disponibilite.png", PAIRS)
    fig_pairs(s, "msgs", "msgs_sd", "messages envoyes",
              "Surcout en messages : sans / avec mecanisme", "eval_2_messages.png", PAIRS)
    fig_topologies(s)
    fig_z(s)
    fig_recovery(rec)
    print("\nFichiers : results/runs_detail.csv, results/eval_summary.csv, results/eval_pairs.csv, "
          "report/evaluation_tables.md")
    sys.exit(1 if nfail else 0)


if __name__ == "__main__":
    main()
