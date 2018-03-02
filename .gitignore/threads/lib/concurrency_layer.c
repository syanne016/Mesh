#include "../include/concurrency_layer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>


pthread_mutex_t mutex_global;	/* Variable de tipo mutex global. */
pthread_cond_t cond_llena;		/* Variable condicional para cuando la cola esta llena. */
pthread_cond_t cond_vacia;		/* Variable condicional para cuando la cola esta vacia. */
int numeroLectores;				/* Numero de lectores actuales. */


/* Metodo que inicializa los mutex, las condiciones y las variables de control. */
void init_concurrency_mechanisms(){ 
	pthread_mutex_init(&mutex_global,NULL);
	pthread_cond_init(&cond_llena, NULL);
	pthread_cond_init(&cond_vacia, NULL);
	numeroLectores = 0;
}


/* Metodo que destruye los mutex, las condiciones y las variables de control. */
void destroy_concurrency_mechanisms(){
	pthread_mutex_destroy(&mutex_global);
	pthread_cond_destroy(&cond_llena);
	pthread_cond_destroy(&cond_vacia);
}


/*
  Metodo que contiene el codigo que usan los hilos de broker.
  Inserta las operaciones que recoje de un fichero batch en la cola de operaciones.
*/
void* broker(void * args){
	/* Declaramos las variables necesarias para el correcto funcionamiento del método. */
	struct broker_info * b = (broker_info *) args;
	char * batch = b->batch_file;
	struct stock_market * market = b->market;
	struct iterator * it = new_iterator(batch);
	struct operation * op = malloc(sizeof(operation));
	operations_queue * queue = market->stock_operations;

	/* Variables auxiliares. */
	int i = 1;
	int aux_enq;

	/* Mientras haya entradas en el fichero. */
	while((i = next_operation(it, op->id, &op->type, &op->num_shares, &op->share_price)) >= 0){		
		/* Lock del mutex global */
		pthread_mutex_lock(&mutex_global);

		/* Creamos la operacion a partir de los datos obtenidos. */
		new_operation(op, op->id, op->type, op->num_shares, op->share_price);

		/* Mientras la cola esta llena esperamos hasta que llege una señal de que no lo esta. */
		while(operations_queue_full(queue)){
			//printf("Broker waiting\n");
			pthread_cond_wait(&cond_llena, &mutex_global);
		} 

		/* Encolamos la operacion y comprobamos que no ha habido ningun error. */
		aux_enq = enqueue_operation(queue, op);
		/* Controlamos error al encolar */
		if(aux_enq == -1){
			//perror("Error al encolar la operación.");
			break;
		}

		pthread_cond_signal(&cond_vacia);	

		/* Desbloqueamos el mutex. */
		pthread_mutex_unlock(&mutex_global);
		
	}

	destroy_iterator(it);
	free(op);
	return EXIT_SUCCESS;
}


/* 
   Metodo que contiene el codigo que usan los hilos de op_exec.
   Ejecuta la operaciones que estan en la cola de operaciones.
*/
void* operation_executer(void * args){
	/* Declaramos las variables necesarias para el correcto funcionamiento del metodo. */
	struct exec_info * exec = (exec_info *) args;
	struct stock_market * market = exec->market;
	struct operations_queue * queue = market->stock_operations;
	struct operation * op = malloc(sizeof(operation));

	/* 
	   Lock del mutex global para comprobar si la cola esta vacia.
	   Mientras la cola esta vacia esperamos hasta que nos llegue una señal de que no lo esta.
	*/
	pthread_mutex_lock(&mutex_global);

	while(operations_queue_empty(queue)){
		pthread_cond_wait(&cond_vacia, &mutex_global);
	} 

	/* Unlock del mutex global. */
	pthread_mutex_unlock(&mutex_global);

	/* Variables auxiliares. */
	int aux = 0;
	int i;
	int aux_deq;

	/* Le damos a aux el valor de la flag exit. */
	pthread_mutex_lock(exec->exit_mutex);
	aux = *(exec->exit);
	pthread_mutex_unlock(exec->exit_mutex);

	/* Mientras la flag exit no este activa. */
	while(aux==0){
		/* Lock del mutex global. */
		pthread_mutex_lock(&mutex_global);					
		
		/* Desencolamos la operacion y comprobamos que no ha habido ningun error. */
		aux_deq = dequeue_operation(queue, op);
		/* Controlamos error al desencolar */
		if(aux_deq==-1){
			//perror("No se ha podido desencolar. \n");
			break;
		}
		pthread_cond_signal(&cond_llena);

		/* Procesamos la operacion y comprobamos que no ha habido ningun error. */
		i = process_operation(market, op);
		/* Controlamos error al procesar */
		if(i==-1){
			//perror("No se ha podido procesar. \n");
			break;
		}

		/* Le damos a aux el valor de la flag exit. */
		pthread_mutex_lock(exec->exit_mutex);
		aux = *(exec->exit);
		pthread_mutex_unlock(exec->exit_mutex);

		/* Mientras la cola esta vacia y la flag de exit no esta activa, dejamos al ejecutor esperando la señal de que la cola ya no esta vacia. */
		while(operations_queue_empty(queue) && aux==0){
			//printf("Exec espera\n");
			pthread_cond_wait(&cond_vacia, &mutex_global);
		} 

		/* Unlock del mutex global. */
		pthread_mutex_unlock(&mutex_global);
		
	}

	/* Mientras queden operaciones por desencolar una vez que la flag exit este activa. */
	while(operations_queue_empty(queue)!=1){
		/* Lock del mutex global. */
		pthread_mutex_lock(&mutex_global);

		/* Desencolamos la operacion y comprobamos que no ha habido ningun error. */
		aux_deq = dequeue_operation(queue, op);
		/* Controlamos error al desencolar */
		if(aux_deq==-1){
			//perror("No se ha podido desencolar. \n");
			break;
		}

		/* Procesamos la operacion y comprobamos que no ha habido ningun error. */
		i = process_operation(market, op); //free malloc?¿
		/* Controlamos error al procesar */
		if(i==-1){
			//perror("No se ha podido procesar. \n");
			break;
		}
		/* Unlock del mutex global. */
		pthread_mutex_unlock(&mutex_global);
	}
	free(op);
	return EXIT_SUCCESS;
}


/* 
   Metodo que contiene el codigo que usan los hilos de reader. 
   Lee el estado del mercado y lo imprime por pantalla.
*/
void* stats_reader(void * args){
	/* Declaramos las variables necesarias para el correcto funcionamiento del metodo. */
	struct reader_info * read = (reader_info *) args;
	stock_market * market = read->market;
	unsigned int freq = read->frequency;

	/* Variable auxiliar. */
	int aux;

	/* Le damos a aux el valor de la flag exit. */
	pthread_mutex_lock(read->exit_mutex);
	aux = *(read->exit);
	pthread_mutex_unlock(read->exit_mutex);	

	/* Mientras la flag exit no este activa. */
	while(aux==0){
		/* Si no hay lectores activos ahora mismo, bloqueamos el mutex global. */
		if(numeroLectores == 0){
			pthread_mutex_lock(&mutex_global);
		}
		/* Incrementamos el numero de lectores e imprimimos el mercado por pantalla. */
		numeroLectores++;
		print_market_status(market);

		/* Decrementamos el numero de lectores por 1 y si no hay mas lectores activos desbloqueamos el mutex global. */
		numeroLectores--;
		if(numeroLectores == 0){
			pthread_mutex_unlock(&mutex_global);
		}
		/* Ponemos el lector a dormir y sobrescribimos el valor de eux con la flag exit. */
		usleep(freq);
		//printf("Reader sleeping\n");
		pthread_mutex_lock(read->exit_mutex);
		aux = *(read->exit);
		pthread_mutex_unlock(read->exit_mutex);	
		
	}
	return EXIT_SUCCESS;
}
			

