 COMANDO PARA COMPILAR O CÓDIGO:

 Versão sequencial: g++ kmeans_seq.cpp -fopenmp -o kmeansSeq

 Versão paralela: g++ kmeans.cpp -fopenmp -o kmeansPar


 COMANDO PARA EXECUTAR O CÓDIGO: 

 Versão sequencial: time ./kmeansSeq input.txt 100

 Versão paralela: time ./kmeansPar input.txt 100


 O primeiro argumento é o arquivo de entrada (também presenta na pasta, chamado input.txt)
 O segundo argumento é o número de clusters que o K-Means irá criar.

 ===========================================================================================
 
 * TEMPO SEQUENCIAL: 
 * time ./kmeansSeq input.txt 100
 * real 0m15.130s  
 * 
 * TEMPO PARALELO 
 * time ./kmeansPar input.txt 100
 * 
 * real 0m9.317s (2 NÚCLEOS)
 * 
 * real 0m5.523s (4 NÚCLEOS)
 * 
 * real 0m6.647s (8 NÚCLEOS)


Análise de performance: 
    Creio que o tempo de execução paralela caiu de 4 para 8 núcleos devido ao overhead de sincronização de threads para 8 núcleos, (maior que o de 4 núcleos).
    Se aumentássemos o tamanho do arquivo de entrada drasticamente, provavelmente o tempo de execução para 8 núcleos ficará menor que o de 4
    Isso ocorre, pois o peso do overhead no cálculo do speedup para uma entrada muito grande irá diminuir.

    Speedup do Sequencial para 2 núcleos: 1.66
    Speedup do Sequencial para 4 núcleos: 2.74
    Speedup de 2 Núcleos para 4 Núcleos: 1.67

