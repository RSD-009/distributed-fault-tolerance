## Resume par config

| config | runs | avail | avail_sd | avail_alive | msgs | msgs_sd | unreach | ckpt | t_ms | overhead_pct |
|---|---|---|---|---|---|---|---|---|---|---|
| Baseline | 10 | 100.0 | 0.0 | 100.0 | 12.0 | 0.0 | 0.0 | 0.0 | 17.5 | 0.0 |
| Node_Crash_Timeout | 10 | 84.3 | 25.6 | 91.7 | 12.2 | 3.0 | 0.6 | 0.0 | 15.5 | 1.7 |
| Node_Reboot_Cold | 10 | 94.3 | 7.4 | 94.3 | 13.3 | 1.3 | 0.0 | 0.0 | 82.5 | 10.8 |
| Node_Reboot_Checkpoint | 10 | 100.0 | 0.0 | 100.0 | 14.1 | 1.4 | 0.0 | 13.0 | 82.5 | 17.5 |
| Node_Transient_Retransmit | 10 | 100.0 | 0.0 | 100.0 | 14.7 | 2.0 | 0.0 | 0.0 | 91.5 | 22.5 |
| Node_Transient_NoRetry | 10 | 82.9 | 14.8 | 82.9 | 10.4 | 1.8 | 1.2 | 0.0 | 15.5 | -13.3 |
| Link_MsgLoss_Retransmit | 10 | 100.0 | 0.0 | 100.0 | 13.6 | 1.3 | 0.4 | 0.0 | 111.5 | 13.3 |
| Link_Down_Retransmit | 10 | 78.6 | 22.6 | 78.6 | 9.0 | 3.2 | 1.0 | 0.0 | 15.5 | -25.0 |
| Link_Down_Temporary | 10 | 100.0 | 0.0 | 100.0 | 12.1 | 0.3 | 0.0 | 0.0 | 104.5 | 0.8 |
| Link_Down_Replication | 10 | 98.6 | 4.5 | 98.6 | 13.6 | 1.3 | 1.0 | 0.0 | 11.0 | 13.3 |
| Ring_Baseline | 10 | 100.0 | 0.0 | 100.0 | 16.0 | 0.0 | 0.0 | 0.0 | 15.0 | 0.0 |
| Ring_Node_Crash_Timeout | 10 | 95.7 | 6.9 | 100.0 | 16.6 | 1.0 | 0.6 | 0.0 | 16.0 | 3.8 |
| Star_Baseline | 10 | 100.0 | 0.0 | 100.0 | 12.0 | 0.0 | 0.0 | 0.0 | 9.5 | 0.0 |
| Star_Node_Crash_Timeout | 10 | 92.9 | 7.5 | 100.0 | 13.0 | 1.1 | 0.5 | 0.0 | 9.5 | 8.3 |
| Mesh_Baseline | 10 | 100.0 | 0.0 | 100.0 | 37.8 | 3.7 | 0.0 | 0.0 | 13.0 | 0.0 |
| Mesh_Node_Crash_Timeout | 10 | 96.4 | 4.7 | 100.0 | 38.0 | 5.6 | 0.7 | 0.0 | 13.0 | 0.5 |
| Link_Down_Replication_NoZ | 10 | 100.0 | 0.0 | 100.0 | 16.0 | 0.0 | 2.0 | 0.0 | 11.5 | 33.3 |
| Mesh_Baseline_NoZ | 10 | 100.0 | 0.0 | 100.0 | 104.0 | 0.0 | 0.0 | 0.0 | 13.0 | 175.1 |
| Link_MsgLoss_NoRetry | 10 | 61.4 | 30.2 | 61.4 | 5.5 | 3.8 | 2.4 | 0.0 | 11.5 | -54.2 |
| Link_Down_Temporary_NoRetry | 10 | 87.1 | 4.5 | 87.1 | 10.1 | 0.3 | 1.0 | 0.0 | 17.5 | -15.8 |

## Comparaisons appariees (AVEC - SANS)

| titre | metric | n | mean_delta | sd | ic95 | wins | ties | losses |
|---|---|---|---|---|---|---|---|---|
| Reboot : Cold -> Checkpoint | availability | 10 | 5.7 | 7.4 | 5.3 | 4 | 6 | 0 |
| Reboot : Cold -> Checkpoint | messages | 10 | 0.8 | 1.7 | 1.2 | 2 | 8 | 0 |
| Transitoire : NoRetry -> Retransmit | availability | 10 | 17.1 | 14.8 | 10.6 | 8 | 2 | 0 |
| Transitoire : NoRetry -> Retransmit | messages | 10 | 4.3 | 2.2 | 1.6 | 10 | 0 | 0 |
| Perte 30% : NoRetry -> Retransmit | availability | 10 | 38.6 | 30.1 | 21.6 | 9 | 1 | 0 |
| Perte 30% : NoRetry -> Retransmit | messages | 10 | 8.1 | 3.8 | 2.7 | 10 | 0 | 0 |
| Lien coupe court : NoRetry -> Retransmit | availability | 10 | 12.9 | 4.5 | 3.2 | 9 | 1 | 0 |
| Lien coupe court : NoRetry -> Retransmit | messages | 10 | 2.0 | 0.0 | 0.0 | 10 | 0 | 0 |
| Lien coupe : Retransmit -> Replication | availability | 10 | 20.0 | 18.1 | 12.9 | 10 | 0 | 0 |
| Lien coupe : Retransmit -> Replication | messages | 10 | 4.6 | 1.9 | 1.4 | 10 | 0 | 0 |
| Lien coupe : avec Z -> sans Z | availability | 10 | 1.4 | 4.5 | 3.2 | 1 | 9 | 0 |
| Lien coupe : avec Z -> sans Z | messages | 10 | 2.4 | 1.3 | 0.9 | 10 | 0 | 0 |
| Maillage : avec Z -> sans Z | availability | 10 | 0.0 | 0.0 | 0.0 | 0 | 10 | 0 |
| Maillage : avec Z -> sans Z | messages | 10 | 66.2 | 3.7 | 2.6 | 10 | 0 | 0 |

## Delai de recovery apparie

| config | runs_valides | runs_impactes | delai_moyen_ms | delai_moyen_impactes_ms | delai_max_ms |
|---|---|---|---|---|---|
| Node_Transient_Retransmit | 10 | 8 | 74.0 | 92.5 | 100.0 |
| Link_Down_Temporary | 10 | 9 | 87.0 | 96.7 | 100.0 |
| Link_MsgLoss_Retransmit | 10 | 9 | 94.0 | 104.4 | 150.0 |
