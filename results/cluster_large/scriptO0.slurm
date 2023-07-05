#!/bin/bash
#####definir particao/fila de uso. Checar as filas disponiveis com sinfo
#SBATCH --partition=cpulong
#####definir tempo maximo de execucao. Nao deve ultrapassar o maximo da fila
#SBATCH --time=05:00:00
#####defr. quant. tarefas. Necessario para usar mais de processo/thread por no (OpenMP)
######SBATCH --hint=nomultithread
#SBATCH --ntasks=32
#####nome que aparece na fila de execucao do slurm
#SBATCH --job-name=tccgreen
#####usa de forma exclusiva (sozinho) o no ou nao
#SBATCH --exclusive
cd $SLURM_SUBMIT_DIR
#### Prepare o ambiente com os devidos modulos #########
#### Exemplo: carregando modulo GCC: ml GCC
#### Exporte as variaveis relevantes para o job ########3
#### Variavel abaixo eh usado em OpenMP para definir a quant. de threads execucao
#export OMP_NUM_THREADS=1
#### Execucao do job ####
#### Lista no em execucao
echo $SLURM_JOB_NODELIST

module load GCC
#time ./tccgreen -s 


#THREADS=1
#ALGORITHM="m"
OPTIMIZATION="O0"
#NSIZE=40000
#ITERATIONS=31

#function compile {
#    g++ /home/paulo/Desktop/TCC-Green/*.cpp -o /home/paulo/Desktop/TCC-Green/tccgreen -$OPTIMIZATION -fopenmp -Wall
#}

function execute {
    export OMP_NUM_THREADS=$THREADS 
    ./tccgreenO0 -o $OPTIMIZATION-$ALGORITHM$THREADS.txt -n $NSIZE -i $ITERATIONS -c $THREADS -$ALGORITHM -$ALGORITHM -$ALGORITHM -$ALGORITHM -$ALGORITHM
}

function testThreads {
    THREADS=32
    execute
    THREADS=16
    execute
    THREADS=8
    execute
    THREADS=4
    execute
    THREADS=2
    execute
    THREADS=1
    execute
}

function test {
    NSIZE=40000
    ITERATIONS=31
    ALGORITHM="m"
    testThreads

    NSIZE=40000
    ITERATIONS=31
    ALGORITHM="r"
    testThreads

    ALGORITHM="s"
    NSIZE=28000
    ITERATIONS=63
    testThreads
}

time test


