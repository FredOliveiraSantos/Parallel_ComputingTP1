#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <omp.h>
#include <algorithm>

/**
 * Frederico Oliveira Costa Santos
 * Projeto 01 - Paralelização do K-Means para agrupar pontos cartesianos.
 * Algoritmo modificado a partir do código submetido pelo usuário aditya1601. (GitHub: https://github.com/aditya1601/kmeans-clustering-cpp)
 * 
 * 
 * Arquivo base de entrada (input.txt) contém aproximadamente 13.500 pontos aletórios para serem agrupados, entre -100.000 e 100.000.
 * Testes de performance foram feitos no servidor PARCODE com 100 clusters para o sequencial e paralelo.
 * 
 * TEMPO SEQUENCIAL: 
 * time ./kmeansSeq input.txt 100
 * real 0m15.130s  
 * 
 * 
 * TEMPO PARALELO 
 * time ./kmeansPar input.txt 100
 * 
 * real 0m9.317s (2 NÚCLEOS)
 * 
 * real 0m5.523s (4 NÚCLEOS)
 * 
 * real 0m6.647s (8 NÚCLEOS)
 * 
 */

using namespace std;

class Point{

private:
    int pointId, clusterId;
    int dimensions;
    vector<double> values;

public:
    Point(int id, string line){
        dimensions = 0;
        pointId = id;
        stringstream is(line);
        double val;
        while(is >> val){
            values.push_back(val);
            dimensions++;
        }
        clusterId = 0;
    }

    int getDimensions(){
        return dimensions;
    }

    int getCluster(){
        return clusterId;
    }

    int getID(){
        return pointId;
    }

    void setCluster(int val){
        clusterId = val;
    }

    double getVal(int pos){
        return values[pos];
    }
};

class Cluster{

private:
    int clusterId;
    vector<double> centroid;
    vector<Point> points;

public:
    Cluster(int clusterId, Point centroid){
        this->clusterId = clusterId;
        for(int i=0; i<centroid.getDimensions(); i++){
            this->centroid.push_back(centroid.getVal(i));
        }
        this->addPoint(centroid);
    }

    void addPoint(Point p){
        p.setCluster(this->clusterId);
        points.push_back(p);
    }

    bool removePoint(int pointId){
        int size = points.size();

        for(int i = 0; i < size; i++)
        {
            if(points[i].getID() == pointId)
            {
                points.erase(points.begin() + i);
                return true;
            }
        }
        return false;
    }

    int getId(){
        return clusterId;
    }

    Point getPoint(int pos){
        return points[pos];
    }

    int getSize(){
        return points.size();
    }

    double getCentroidByPos(int pos) {
        return centroid[pos];
    }

    void setCentroidByPos(int pos, double val){
        this->centroid[pos] = val;
    }
};

class KMeans{
private:
    int K, iters, dimensions, total_points;
    vector<Cluster> clusters;

    // Função muito custosa para paralelização (tanto utilizando sessões criticas quanto o padrão REDUCE). 
    // A paralelização da mesma faz com que as threads fiquem muito tempo em espera e aumenta o tempo de execução do código. (Ainda deixei comentado as diretrizes utilizadas)
    int getNearestClusterId(Point point){
        double sum = 0.0, min_dist;
        int NearestClusterId;

        // #pragma omp parallel for reduction(+:sum)
        for(int i = 0; i < dimensions; i++)
        {
            sum += pow(clusters[0].getCentroidByPos(i) - point.getVal(i), 2.0);
        }

        min_dist = sqrt(sum);
        NearestClusterId = clusters[0].getId();
        
        for(int i = 1; i < K; i++)
        {
            double dist;
            sum = 0.0;
            // #pragma omp parallel for reduction(+:sum)
            for(int j = 0; j < dimensions; j++)
            {
                sum += pow(clusters[i].getCentroidByPos(j) - point.getVal(j), 2.0);
            }

            dist = sqrt(sum);

            if(dist < min_dist)
            {
                min_dist = dist;
                NearestClusterId = clusters[i].getId();
            }
        }

        return NearestClusterId;
    }

public:
    KMeans(int K, int iterations){
        this->K = K;
        this->iters = iterations;
    }

    void run(vector<Point>& all_points){

        total_points = all_points.size();
        dimensions = all_points[0].getDimensions();

        vector<int> used_pointIds;
        for(int i=1; i<=K; i++)
        {
            while(true)
            {
                int index = rand() % total_points;

                if(find(used_pointIds.begin(), used_pointIds.end(), index) == used_pointIds.end())
                {
                    used_pointIds.push_back(index);
                    all_points[index].setCluster(i);
                    Cluster cluster(i, all_points[index]);
                    clusters.push_back(cluster);
                    break;
                }
            }
        }

        int iter = 1;
        while(true)
        {
            bool done = true;
            /**
             * Parte paralela do código: Adiciona os pontos nos respectivos clusters mais próximos.
             * Aqui, o Id do cluster mais próximo será buscado, e os pontos serão removidos do cluster anterior a qual pertenciam, e adicionados no cluster correto
             * As operações críticas dessa sessão são as de acesso ao vetor compartilhado de clusters, portanto estão protegidas pela diretriz #pragma omp critical
             */
            #pragma omp parallel for
            for(int i = 0; i < total_points; i++)
            {
                int currentClusterId = all_points[i].getCluster();
                int nearestClusterId = getNearestClusterId(all_points[i]);

                if(currentClusterId != nearestClusterId)
                {
                    if(currentClusterId != 0){
                        for(int j=0; j<K; j++){
                            if(clusters[j].getId() == currentClusterId){
                                #pragma omp critical
                                clusters[j].removePoint(all_points[i].getID());
                            }
                        }
                    }
                    for(int j=0; j<K; j++){
                        if(clusters[j].getId() == nearestClusterId){
                            #pragma omp critical
                            clusters[j].addPoint(all_points[i]);
                        }
                    }
                    #pragma omp critical
                    all_points[i].setCluster(nearestClusterId);
                    done = false;
                }
            }
            // Parte deixada sequencial para fins de performance, pois se repete poucas vezes (igual ao número de clusters K).
            // O overhead de sincronização das threads prejudica a performance paralela, e deixa a sequencial melhor
            for(int i = 0; i < K; i++)
            {
                int ClusterSize = clusters[i].getSize();

                for(int j = 0; j < dimensions; j++)
                {
                    double sum = 0.0;
                    if(ClusterSize > 0)
                    {
                        for(int p = 0; p < ClusterSize; p++)
                            sum += clusters[i].getPoint(p).getVal(j);
                        clusters[i].setCentroidByPos(j, sum / ClusterSize);
                    }
                }
            }

            if(done || iter >= iters)
            {
                break;
            }
            iter++;
        }
    }
};

int main(int argc, char **argv){

    // Setando numero de threads para o tamanho especificado
    omp_set_num_threads(8);

    if(argc != 3){
        return 1;
    }

    int K = atoi(argv[2]);

    string filename = argv[1];
    ifstream infile(filename.c_str());

    if(!infile.is_open()){
        return 1;
    }

    int pointId = 1;
    vector<Point> all_points;
    string line;
    while(getline(infile, line)){
        Point point(pointId, line);
        all_points.push_back(point);
        pointId++;
    }
    infile.close();

    if(all_points.size() < K){
        return 1;
    }

    int iters = 100;

    KMeans kmeans(K, iters);
    kmeans.run(all_points);

    return 0;
}