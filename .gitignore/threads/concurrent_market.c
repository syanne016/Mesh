#include "include/concurrency_layer.h"

int main(int argc, char * argv[]){

	/* Declaracion de variables. */
	pthread_t tid[16];
	stock_market market_madrid;
	int exit = 0;
	pthread_mutex_t exit_mutex;

	/* Inicializacion del mercado de valores y mecanismos de concurrencia. */
	init_market(&market_madrid, "stocks.txt");
	init_concurrency_mechanisms();
	pthread_mutex_init(&exit_mutex,NULL);
	
	/* Inicializacion de las estructuras necesarias para los hilos broker. */
	broker_info info_b1;
	strcpy(info_b1.batch_file, "batch_operations1.txt");
	info_b1.market = &market_madrid;

	broker_info info_b2;
	strcpy(info_b2.batch_file, "batch_operations2.txt");
	info_b2.market = &market_madrid;

	broker_info info_b3;
	strcpy(info_b3.batch_file, "batch_operations3.txt");
	info_b3.market = &market_madrid;

	broker_info info_b4;
	strcpy(info_b4.batch_file, "batch_operations4.txt");
	info_b4.market = &market_madrid;

	broker_info info_b5;
	strcpy(info_b5.batch_file, "batch_operations5.txt");
	info_b5.market = &market_madrid;
	
	/* Inicializacion de las estructuras necesarias para el hilo de operation executer. */
	exec_info info_ex1;
	info_ex1.market = &market_madrid;
	info_ex1.exit = &exit;
	info_ex1.exit_mutex = &exit_mutex;
	
	/* Inicializacion de las estructuras necesarias para los hilos reader. */
	reader_info info_re1;
	info_re1.market = &market_madrid;
	info_re1.exit = &exit;
	info_re1.exit_mutex = &exit_mutex;
	info_re1.frequency = 100000;

	reader_info info_re2;
	info_re2.market = &market_madrid;
	info_re2.exit = &exit;
	info_re2.exit_mutex = &exit_mutex;
	info_re2.frequency = 100000;

	reader_info info_re3;
	info_re3.market = &market_madrid;
	info_re3.exit = &exit;
	info_re3.exit_mutex = &exit_mutex;
	info_re3.frequency = 100000;

	reader_info info_re4;
	info_re4.market = &market_madrid;
	info_re4.exit = &exit;
	info_re4.exit_mutex = &exit_mutex;
	info_re4.frequency = 100000;

	reader_info info_re5;
	info_re5.market = &market_madrid;
	info_re5.exit = &exit;
	info_re5.exit_mutex = &exit_mutex;
	info_re5.frequency = 100000;

	reader_info info_re6;
	info_re6.market = &market_madrid;
	info_re6.exit = &exit;
	info_re6.exit_mutex = &exit_mutex;
	info_re6.frequency = 100000;

	reader_info info_re7;
	info_re7.market = &market_madrid;
	info_re7.exit = &exit;
	info_re7.exit_mutex = &exit_mutex;
	info_re7.frequency = 100000;

	reader_info info_re8;
	info_re8.market = &market_madrid;
	info_re8.exit = &exit;
	info_re8.exit_mutex = &exit_mutex;
	info_re8.frequency = 100000;

	reader_info info_re9;
	info_re9.market = &market_madrid;
	info_re9.exit = &exit;
	info_re9.exit_mutex = &exit_mutex;
	info_re9.frequency = 100000;

	reader_info info_re10;
	info_re10.market = &market_madrid;
	info_re10.exit = &exit;
	info_re10.exit_mutex = &exit_mutex;
	info_re10.frequency = 100000;
	
	/* Creacion de hilos broker. */
	pthread_create(&(tid[0]), NULL, &broker, (void*) &info_b1);
	pthread_create(&(tid[1]), NULL, &broker, (void*) &info_b2);
	pthread_create(&(tid[2]), NULL, &broker, (void*) &info_b3);
	pthread_create(&(tid[3]), NULL, &broker, (void*) &info_b4);
	pthread_create(&(tid[4]), NULL, &broker, (void*) &info_b5);

	/* Creacion de hilo operation executer. */
	pthread_create(&(tid[5]), NULL, &operation_executer, (void*) &info_ex1);

	/* Creacion de hilos reader. */
	pthread_create(&(tid[6]), NULL, &stats_reader, (void*) &info_re1);
	pthread_create(&(tid[7]), NULL, &stats_reader, (void*) &info_re2);
	pthread_create(&(tid[8]), NULL, &stats_reader, (void*) &info_re3);
	pthread_create(&(tid[9]), NULL, &stats_reader, (void*) &info_re4);
	pthread_create(&(tid[10]), NULL, &stats_reader, (void*) &info_re5);
	pthread_create(&(tid[11]), NULL, &stats_reader, (void*) &info_re6);
	pthread_create(&(tid[12]), NULL, &stats_reader, (void*) &info_re7);
	pthread_create(&(tid[13]), NULL, &stats_reader, (void*) &info_re8);
	pthread_create(&(tid[14]), NULL, &stats_reader, (void*) &info_re9);
	pthread_create(&(tid[15]), NULL, &stats_reader, (void*) &info_re10);

	/* Join de los hilos broker. */
	void * res;
	pthread_join(tid[0],&res);
	pthread_join(tid[1],&res);
	pthread_join(tid[2],&res);
	pthread_join(tid[3],&res);
	pthread_join(tid[4],&res);
	//printf("Join de los brokers.\n");
	
	/* Activamos la flag exit una vez los brokers han terminado. */
	pthread_mutex_lock(&exit_mutex);
	exit = 1;
	pthread_mutex_unlock(&exit_mutex);
	
	/* Join del hilo operation executer. */
	pthread_join(tid[5],&res);
	//printf("Join del ejecutor. \n");
	
	/* Join de los hilos de reader. */
	pthread_join(tid[6], &res);
	pthread_join(tid[7], &res);
	pthread_join(tid[8], &res);
	pthread_join(tid[9],&res);
	pthread_join(tid[10],&res);
	pthread_join(tid[11],&res);
	pthread_join(tid[12],&res);
	pthread_join(tid[13],&res);
	pthread_join(tid[14],&res);
	pthread_join(tid[15],&res);
	//printf("Join de los readers.\n");
	
	/* Imprimimos las estadisticas del mercado. */
	print_market_status(&market_madrid);
	
	/* Destruimos el mercado y todos los mecanismos de concurrencia creados. */
	delete_market(&market_madrid);
	destroy_concurrency_mechanisms();
	pthread_mutex_destroy(&exit_mutex);
  
	return 0;
}