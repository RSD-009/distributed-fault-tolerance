# Évaluation des performances — Projet 5 (tolérance aux pannes sur diffuse-v3)

> Squelette : les chiffres ci-dessous sont ceux de la campagne (20 configs × 10 runs = 200 fichiers `.sca`).
> Chaque bloc « À RÉDIGER » est à écrire avec tes propres mots. Ne garde que des phrases que tu peux défendre à l'oral.
> Source des tableaux complets : `report/evaluation_tables.md`, `results/eval_summary.csv`, `results/eval_pairs.csv`.

## 1. Méthode

**Chiffres à utiliser**
- 20 configurations, 10 runs chacune (graines 0 à 9), 7 nœuds (11 pour le maillage), délai de lien 5 ms, timeout 50 ms, 3 retransmissions.
- À chaque run : initiateur, victime et lien coupé sont tirés au hasard ; seule la victime applique les paramètres de panne.
- Comparaisons **appariées** : le run k de deux configs du même réseau a le même initiateur (vérifié pour les 7 paires par `evaluate.py`).
- Métriques : disponibilité = nœuds informés / n (nœud crashé compris) ; disponibilité sur les nœuds non crashés ; messages = `messagesSent` (INFORM + ACK + retransmissions effectivement envoyés) ; délai de recovery = fin de diffusion − fin de diffusion du Baseline du même run ; temps de détection = `unreachableAt − informedAt`.
- Intervalle de confiance à 95 % (loi de Student) sur le delta apparié.

**À RÉDIGER (5 à 8 lignes)** : pourquoi l'appariement est plus rigoureux qu'une comparaison de moyennes ; ce que `messagesSent` ne compte pas (messages bloqués `messagesCut` ou détruits `messagesDropped`, donc le surcoût de la retransmission est sous-estimé).

## 2. Validation du simulateur contre le cours

| Config (sans panne) | Messages mesurés | Borne 2 × (2m − n + 1) |
|---|---|---|
| Arbre (n=7, m=6) | 12 à chaque run | 12 |
| Étoile (n=7, m=6) | 12 à chaque run | 12 |
| Anneau (n=7, m=7) | 16 à chaque run | 16 |
| Maillage sans Z (n=11, m=31) | 104 à chaque run | 104 |
| Maillage avec Z | 37,8 en moyenne (σ = 3,7) | 104 |

- 19 vérifications automatiques réussies (voir la sortie d'`evaluate.py`).
- Détection d'un crash : **200,0 ms** sur les quatre topologies (min = max), soit (maxRetries + 1) × timeout.

**À RÉDIGER** : relie ces chiffres au cours (chapitre 2, bornes de la diffusion) ; explique pourquoi la valeur du maillage avec Z est sous la borne (Z supprime des envois).

## 3. Fautes de nœud

| Paire (sans → avec) | Disponibilité | Delta apparié (points) | Gains / égalités / pertes |
|---|---|---|---|
| Reboot : Cold → Checkpoint | 94,3 % → 100 % | +5,71 ± 5,28 | 4 / 6 / 0 |
| Transitoire : NoRetry → Retransmit | 82,9 % → 100 % | +17,14 ± 10,55 | 8 / 2 / 0 |

- Reboot à froid : la victime perd l'état dans **4 runs sur 10** (cas B) ; dans les 6 autres une retransmission la rattrape (cas A). Les 4 gains du checkpoint sont exactement ces 4 runs.
- Recovery du nœud rebooté (crash → de nouveau informé) : Checkpoint 10 runs valides sur 10, moyenne 104,4 ms (de 53,2 à 162,3) ; Cold 6 runs valides sur 10 (les 4 autres : jamais récupéré), moyenne 116,6 ms.
- Coût du checkpoint : 13 sauvegardes par run en moyenne, +0,8 message.
- Crash permanent : le détecteur ne le récupère pas (84,3 % sur tous les nœuds, 91,7 % sur les nœuds non crashés pour l'arbre).

**À RÉDIGER** : (1) le checkpoint ne raccourcit pas le recovery, il le **rend possible** dans 100 % des cas au lieu de 60 % ; (2) pourquoi les moyennes 104,4 et 116,6 ne se comparent pas (runs différents) ; (3) la limite : en cas B le recovery vaut le délai de reboot imposé par l'ini.

## 4. Fautes de lien

| Paire (sans → avec) | Disponibilité | Delta apparié (points) | Messages (delta) |
|---|---|---|---|
| Perte 30 % : NoRetry → Retransmit | 61,4 % → 100 % | +38,57 ± 21,57 | +8,1 |
| Lien coupé court : NoRetry → Retransmit | 87,1 % → 100 % | +12,86 ± 3,23 | +2,0 (σ = 0) |
| Lien coupé permanent : Retransmit → Replication | 78,6 % → 98,6 % | +20,00 ± 12,93 (10 gains sur 10) | +4,6 |

- Fausses suspicions sous 30 % de perte : 0,4 voisin déclaré injoignable par run **sans aucun crash** (théorie : 6 liens × 0,51⁴ ≈ 0,41).
- Délais de recovery (fin de diffusion − Baseline) : valeurs observées 0, 50, 90, 100, 140, 150 ms ; maximum 150 ms, sous l'abandon à 200 ms. Moyennes sur les runs impactés : transitoire 92,5 ms (8 runs sur 10), lien coupé court 96,7 ms (9 sur 10), perte 104,4 ms (9 sur 10).

**À RÉDIGER** : (1) le +2,0 message de la coupure courte n'est pas un « surcoût » mais l'INFORM et l'ACK enfin livrés ; (2) pourquoi les délais se regroupent près de multiples du timeout (la retransmission arrive par pas de 50 ms, auxquels s'ajoutent quelques sauts de 5 ms) ; (3) les fausses suspicions et leur lien avec le modèle semi-synchrone.

## 5. Le compromis Z (diffuse-v3 contre diffuse-v2)

| | Avec Z (v3) | Sans Z (v2) |
|---|---|---|
| Maillage, sans panne : messages | 37,8 | 104,0 (−64 % avec Z) |
| Lien coupé + secours : cas en échec, table exacte (14 cas) | 4 / 14 = 28,6 % (= 2/7) | 0 / 14 |
| Lien coupé + secours, 50 runs tirés | 19 / 50 = 38 % (disponibilité 94,6 %) | 0 / 50 (100 %) |
| Lien coupé + secours : messages (10 runs) | 13,6 | 16,0 |
| Lien coupé + secours : voisins déclarés injoignables par run | 1,0 | 2,0 |

- Avec Z, l'échec survient **si et seulement si** l'initiateur est le nœud 1, ou la feuille dont le lien avec 1 est coupé. Les 19 échecs sur 50 s'expliquent ainsi : 10 (initiateur 1) + 6 (initiateur 3) + 3 (initiateur 4). Le taux de 38 % vient du tirage des initiateurs (l'initiateur 1 est sorti 10 fois au lieu de 7).
- Sans Z, deux nœuds se déclarent mutuellement injoignables alors que les deux sont vivants.
- Sur les 10 runs de la campagne, un seul run a échoué avec Z : ce n'est pas suffisant pour conclure, d'où la table exacte.

**À RÉDIGER** : (1) Z est une promesse valable seulement sous la fiabilité totale du cours ; (2) la table de vérité (figure `eval_6`) ; (3) le compromis coût contre robustesse ; (4) les pistes : alerte de promesse rompue, bascule vers v2 après la première détection.

## 6. Effet de la topologie (crash d'un nœud aléatoire)

| Topologie | Disponibilité, tous nœuds | Nœuds non crashés informés |
|---|---|---|
| Arbre | 84,3 % (σ = 25,6) | 91,7 % |
| Anneau | 95,7 % | 100 % |
| Étoile | 92,9 % | 100 % |
| Maillage | 96,4 % | 100 % |

**À RÉDIGER** : l'arbre perd un sous-arbre, l'anneau et le maillage contournent la panne (redondance de chemins). **Précaution** : l'étoile reste à 100 % mais le centre n'a pas été la victime dans ces 10 runs : sa fragilité est théorique, non observée (à tester avec `--**.crashNodeId=0` si tu as le temps).

## 7. Limites (à reprendre telles quelles, avec tes mots)

- 10 runs par config : des différences proches ne sont pas significatives ; les intervalles de confiance le montrent.
- Un surcoût négatif (par exemple −54 % pour la perte sans retry) n'est pas une économie : moins de nœuds sont atteints.
- Le recovery du reboot est dominé par le délai imposé dans l'ini ; seul le délai apparié est mesuré pour la retransmission.
- Détection à la demande (pas de heartbeat), pas de re-sondage, checkpoint en mémoire.
- Le maillage (11 nœuds) sort de la fourchette 5–10 de la fiche : cas bonus.
- Les prédictions théoriques ne sont pas toutes confirmées par un test : cite uniquement celles que la campagne a vérifiées.

## 8. Déclaration d'usage d'un LLM (obligatoire)

**À RÉDIGER avec Benhalima**, honnêtement : quel assistant, pour quoi (explication de concepts, aide à l'écriture des scripts d'analyse et des correctifs), et ce que l'équipe a vérifié elle-même. Le rapport doit dire que chaque membre comprend chaque fichier.
