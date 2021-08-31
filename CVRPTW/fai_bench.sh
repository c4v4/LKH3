#!/bin/bash

VAIQUI="/nfsd/rop/qui_qsub"

# Select 2 random instaces for each class/subclass
# for c in 1000 800 600 400 200; do
#     CLASS=Homberger_${c}
#     for f in C1 C2 R1 R2 RC1 RC2; do
#         for FILE in $(cd INSTANCES/ && ls $CLASS/$f* | shuf -n2); do
#             echo ${FILE%%.vrptw}
#         done
#     done
# done

dataset=(
    Homberger_1000/C1_10_4.1000.90
    Homberger_1000/C1_10_5.1000.100
    Homberger_1000/C2_10_3.1000.28
    Homberger_1000/C2_10_9.1000.29
    Homberger_1000/R1_10_4.1000.91
    Homberger_1000/R1_10_3.1000.91
    Homberger_1000/R2_10_8.1000.19
    Homberger_1000/R2_10_1.1000.19
    Homberger_1000/RC1_10_6.1000.90
    Homberger_1000/RC1_10_1.1000.90
    Homberger_1000/RC2_10_1.1000.20
    Homberger_1000/RC2_10_6.1000.18
    Homberger_800/C1_8_2.800.72
    Homberger_800/C1_8_5.800.80
    Homberger_800/C2_8_6.800.23
    Homberger_800/C2_8_7.800.24
    Homberger_800/R1_8_10.800.72
    Homberger_800/R1_8_4.800.72
    Homberger_800/R2_8_6.800.15
    Homberger_800/R2_8_3.800.15
    Homberger_800/RC1_8_3.800.72
    Homberger_800/RC1_8_9.800.72
    Homberger_800/RC2_8_3.800.15
    Homberger_800/RC2_8_2.800.16
    Homberger_600/C1_6_8.600.56
    Homberger_600/C1_6_4.600.56
    Homberger_600/C2_6_2.600.17
    Homberger_600/C2_6_7.600.18
    Homberger_600/R1_6_3.600.54
    Homberger_600/R1_6_9.600.54
    Homberger_600/R2_6_6.600.11
    Homberger_600/R2_6_3.600.11
    Homberger_600/RC1_6_4.600.55
    Homberger_600/RC1_6_10.600.55
    Homberger_600/RC2_6_4.600.11
    Homberger_600/RC2_6_3.600.11
    Homberger_400/C1_4_1.400.40
    Homberger_400/C1_4_5.400.40
    Homberger_400/C2_4_2.400.12
    Homberger_400/C2_4_5.400.12
    Homberger_400/R1_4_4.400.36
    Homberger_400/R1_4_3.400.36
    Homberger_400/R2_4_10.400.8
    Homberger_400/R2_4_9.400.8
    Homberger_400/RC1_4_5.400.36
    Homberger_400/RC1_4_4.400.36
    Homberger_400/RC2_4_1.400.11
    Homberger_400/RC2_4_7.400.8
    Homberger_200/C1_2_5.200.20
    Homberger_200/C1_2_6.200.20
    Homberger_200/C2_2_4.200.6
    Homberger_200/C2_2_2.200.6
    Homberger_200/R1_2_3.200.18
    Homberger_200/R1_2_9.200.18
    Homberger_200/R2_2_10.200.4
    Homberger_200/R2_2_8.200.4
    Homberger_200/RC1_2_6.200.18
    Homberger_200/RC1_2_5.200.18
    Homberger_200/RC2_2_1.200.6
    Homberger_200/RC2_2_4.200.4
)

for inst in ${dataset[@]}; do
    CLASS=${inst%%/*}
    FILE=${inst##*/}
    for seed in 1 2 3 4; do
        NAME=$1${seed}_${FILE}
        echo "./run_bench $CLASS $FILE 10000 1 $seed 0 > $NAME.log"
        $VAIQUI "./run_bench $CLASS $FILE 10000 1 $seed 0 " $NAME.log
    done
done
