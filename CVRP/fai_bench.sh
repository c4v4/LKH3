#!/bin/bash

VAIQUI="/nfsd/rop/qui_qsub"

for seed in 1 2 3 4
do
	CLASS=Belgium
	DATA=./INSTANCES/$CLASS
	for FILE in `cd ${DATA} && ls *.vrp` ; 
	do	
		NAME=$1${seed}_${FILE}
		echo "./run_bench_Belgium $CLASS ${FILE%%.vrp} 5000 1 ${seed} 0 > $NAME.log"
		$VAIQUI "./run_bench_Belgium $CLASS ${FILE%%.vrp} 5000 1 ${seed} 0" $NAME.log
	done

done

for seed in 1 2 3 4
do
	CLASS=Uchoa
	DATA=./INSTANCES/$CLASS
        for FILE in `cd ${DATA} && ls *.vrp` ;
        do
                NAME=$1${seed}_${FILE}
                echo "./run_bench_Uchoa $CLASS ${FILE%%.vrp} 10000 1 ${seed} 0 > $NAME.log"
                $VAIQUI "./run_bench_Uchoa $CLASS ${FILE%%.vrp} 10000 1 ${seed} 0" $NAME.log
        done
done

