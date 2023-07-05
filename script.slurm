#!/bin/bash
#####definir particao/fila de uso. Checar as filas disponiveis com sinfo
#SBATCH --partition=cpushort
#####definir tempo maximo de execucao. Nao deve ultrapassar o maximo da fila
#SBATCH --time=00:05:00
#####defr. quant. tarefas. Necessario para usar mais de processo/thread por no (OpenMP)
#SBATCH --hint=nomultithread
#SBATCH --ntasks=28
#####nome que aparece na fila de execucao do slurm
#SBATCH --job-name=tccgreen
#####usa de forma exclusiva (sozinho) o no ou nao
#SBATCH --exclusive
cd $SLURM_SUBMIT_DIR
#### Prepare o ambiente com os devidos modulos #########
#### Exemplo: carregando modulo GCC: ml GCC
#### Exporte as variaveis relevantes para o job ########3
#### Variavel abaixo eh usado em OpenMP para definir a quant. de threads execucao
export OMP_NUM_THREADS=16
#### Execucao do job ####
#### Lista no em execucao
echo $SLURM_JOB_NODELIST

module load GCC
time ./tccgreen
