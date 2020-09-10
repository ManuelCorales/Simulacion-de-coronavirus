// Con estos includes les deberia alcanzar
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <random>
#include <chrono>
#include <math.h>
#include <string.h>  
#include <atomic>
#include <tuple>

using namespace std;

tuple<int, int> simulate(int day,atomic<int> *sick, float threshold, int M, float pSuperSpreader);

void* share_mem(int size);


int main(int argc, char* argv[])
{

	int max_days = atoi(argv[1]); // cantidad máxima de días
	int M = atoi(argv[2]); // cantidad de personas por reunion 
	float threshold = stof(argv[3]); // número que determina la probabilidad de que se infecten las personas
	float pSuperSpreader = stof(argv[4]); // probabilidad de que surja un super spreader
 	int size = max_days;
	int pidProcesoPrimero = getpid(); 
	int cantidadInfectadosPdiaPProceso;
	atomic<int> *sick = (atomic<int> *) share_mem(size); // Arreglo de enteros. SIZE determina el tamaño del array
	sick[0] = 1;
  	
 	int day, nsick;


	for(day = 0; day < max_days; day++)
	{
		tuple<int,int> result = simulate(day, sick, threshold, M, pSuperSpreader);
		int pid = get<0>(result);
		nsick = get<1>(result);
		if(nsick != -1){
			if(pidProcesoPrimero == getpid()){
				cout << "Yo, spreader, " << getpid() << " infecté a " << nsick << " personas" << endl;
			} else {
				cout << "Yo, spreader, " << getpid() << " infecté a " << nsick << " personas y me infectó: " << getppid() << endl;
			}
			
			sick[day] += nsick;
			for(int i = 0; i < nsick; i++){
				wait(NULL);
			}
			break;
		}
	}
	if(getpid() == pidProcesoPrimero){
		vector<int> diasConMasInfectados;
		diasConMasInfectados.push_back(0);
		for(int i = 0; i < max_days; i++){
			if(diasConMasInfectados[0] < sick[i]){
				diasConMasInfectados.clear();
				diasConMasInfectados.push_back(i);

			} else if (diasConMasInfectados[0] == sick[i]) {
				diasConMasInfectados.push_back(i);
			}
		}
		if(diasConMasInfectados.size() > 1){
			cout << "Los dias con mayor cantidad de infectados fueron los números: ";
			for(int i = 0; i < diasConMasInfectados.size(); i++){
				cout << diasConMasInfectados[i] << " ";
			} 
			cout << "y " << diasConMasInfectados[diasConMasInfectados.size() - 1] << endl;
		} else {
			cout << "El día con más infectados fue el número: " << diasConMasInfectados[0] << endl;
		}
		int cantidadTotalDeInfectados = 0;
		float promedioInfectados = 0; 
		for(int i = 0; i < max_days; i++){
			cantidadTotalDeInfectados += sick[i];
			promedioInfectados = cantidadTotalDeInfectados / (i + 1);
			cout << "En el día " << (i) << " hubo un total de infectados de " << sick[i] << " personas, sumando un total de: " << cantidadTotalDeInfectados << " infectados. Con un promedio de infectados por día de " << promedioInfectados << endl;
		}
		cout << "Los infectados totales fueron " << cantidadTotalDeInfectados << endl;
	}
	free(sick);
	exit(0);
	
	return 0;

}


void* share_mem(int size)
{
    void * mem;
    if( MAP_FAILED == (mem = (atomic<int>*)mmap(NULL, sizeof(atomic<int>)*size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0) ) )
    {
        perror( "mmap failed" );
        exit( EXIT_FAILURE );
    }
    return mem;	
}

tuple<int, int> simulate(int day,atomic<int> *sick, float threshold, int M, float pSuperSpreader)
{
	int nsick = 0;
	int pid;
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator (seed);
 	uniform_real_distribution<double> distribution(0.0,1.0);
	float valorAleatorio;
	valorAleatorio = distribution(generator);
	if(valorAleatorio < pSuperSpreader){
		cout << "Noooo, el contagiado número: " << getpid() << " resultó ser un super spreader" << endl;
		for(int i = 0; i < M; i++){
			nsick++;
			pid = fork();
			if(pid == 0){
				break;
			}
		}
	} else {
		for(int i = 0; i < M; i++){
			valorAleatorio = distribution(generator);
			if (valorAleatorio < threshold){
				nsick++;
				pid = fork();
				if(pid == 0){
					break;
				}
			}
		}
	}
	if(pid == 0){
		return make_tuple(pid, -1); // El valor -1 indica que el que hace el return es un hijp
	}
	return make_tuple(pid,nsick);
}