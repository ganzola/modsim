//SIMULADOR DE CAJA REGISTRADORA CON UNA FILA
//CÓDIGO REALIZADO CON BASE EN EL LIBRO DE SIMULACIÓN DE LAW

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "lib/lcgrand.h" /* Header file for the random-number-generator */

#define Q_LIMIT 100 /* Limit on the queue length */
#define BUSY 1		/* Mnemonics for register's being busy */
#define IDLE 0		/* and idle. */

int next_event_type, num_clients_served, num_total_clients, num_events, num_in_q, register_status;
float area_num_in_q, area_register_status, mean_interarrival, mean_service, sim_time, time_arrival[Q_LIMIT + 1], time_last_event, time_next_event[3], total_service_time;
FILE *infile, *outfile;

void initialize(void);
void timing(void);
void arrive(void);
void depart(void);
void report(void);
void update_time_avg_stats(void);
float expon(float mean);
float poisson(float mean);

int main(void)
{
	/* Archivos de entrada y salida */
	infile = fopen("mm1.9.in", "r");
	outfile = fopen("mm1.9.out", "w");

	/* Como son dos eventos, se establece en 2 */
	num_events = 2;

	/* Parámetros de entrada */
	fscanf(infile, "%f %f %d", &mean_interarrival, &mean_service, &num_total_clients);

	/* Mostrar valores de entrada */
	fprintf(outfile, "Sistema del supermercado\n\n");
	fprintf(outfile, "Tiempo promedio entre llegadas de %16.3f minutos\n\n", mean_interarrival);
	fprintf(outfile, "Tiempo medio de servicio de %16.3f minutos\n\n", mean_service);
	fprintf(outfile, "Número de clientes %14d\n\n", num_total_clients);

	/*Inicializar simulación */
	initialize();

	/* Correr la simulación mientras haya clientes */
	while (num_clients_served < num_total_clients)
	{
		/*Determinar el tiempo del siguiente evento */
		timing();

		/* Actualizar acumuladores estadísticos */
		update_time_avg_stats();

		/* Llamar a la rutina del evento, dependiendo de su tipo */
		switch (next_event_type)
		{
		case 1:
			arrive();
			break;
		case 2:
			depart();
			break;
		}
	}

	/* Invoke the report generator and end the simulation */
	report();

	fclose(infile);
	fclose(outfile);

	return 0;
}

void initialize(void)
{ /* Initialize function. */
	/* Initialize the simulation clock. */
	sim_time = 0;

	/* Initialize the state variables. */
	register_status = IDLE;
	num_in_q = 0;
	time_last_event = 0.0;

	/* Initialize the statistical counters. */
	num_clients_served = 0;
	total_service_time = 0.0;
	area_num_in_q = 0.0;
	area_register_status = 0.0;

	/* Initialize the event list. Since no customers are present, the departure (service completion) event is eliminated from consideration. */
	time_next_event[1] = sim_time + poisson(mean_interarrival);
	time_next_event[2] = 1.0e+30;
}

void timing(void)
{ /* Timing function. */
	int i;

	float min_time_next_event = 1.0e+29; //esto se declara así por una verificación de errores

	next_event_type = 0; //si no hay eventos, esto se quedará en 0 y arrojará error

	/* Determine the event type for the next event to occur. */
	for (i = 1; i <= num_events; ++i)
	{
		if (time_next_event[i] < min_time_next_event) //si el evento es válido
		{
			min_time_next_event = time_next_event[i];
			next_event_type = i;
		}
	}

	/* Check to see whether the event list is empty. */
	if (next_event_type == 0)
	{
		/* The event list is empty, so stop the simulation. */
		fprintf(outfile, "\nLista de eventos vacía en el tiempo %f", sim_time);
		exit(1);
	}

	/* The event list is not empty, so advance the simulation clock. */
	sim_time = min_time_next_event;
}

void arrive(void)
{ /* Arrival event function. */
	float delay;
	/* Schedule next arrival. */
	time_next_event[1] = sim_time + poisson(mean_interarrival);

	/* Check to see whether register is busy. */
	if (register_status == BUSY)
	{
		/* register is busy so increment the number of customers in the queue. */
		++num_in_q;

		/* Check to see whether an overflow condition exists. */
		if (num_in_q > Q_LIMIT)
		{
			/* The queue has overflowed, so stop the simulation. */
			fprintf(outfile, "\nLa cola se desbordó en el tiempo %f", sim_time);
			exit(2);
		}

		/* There is still room in the queue, so store the time of arrival of the arriving customer at the (new) end of time_arrival. */
		time_arrival[num_in_q] = sim_time;
	}

	else
	{
		/* register is idle, so arriving customer has a delay of zero. */
		delay = 0.0;
		total_service_time += delay;

		/* Increment the number of customers delayed, and make register busy. */
		++num_clients_served;
		register_status = BUSY;

		/* Schedule a departure (service completion). */
		time_next_event[2] = sim_time + expon(mean_service);
	}
}

void depart(void)
{ /*Departure event function. */
	int i;
	float delay;

	/* Check to see whether the queue is empty. */
	if (num_in_q == 0)
	{
		/* The queue is empty so make the register idle and eliminate the
		 * departure (service completion) event from consideration. */
		register_status = IDLE;
		time_next_event[2] = 1.0e+30;
	}
	else
	{
		/* The queue is nonempty, so decrement the number of customers in queue. */
		--num_in_q;

		/* Compute the delay of the customer who is beginning service
		 * and update the total delay of accumulator. */
		delay = sim_time - time_arrival[1];
		total_service_time += delay;

		/* Increment the number of customers delayed, and schedule departure. */
		++num_clients_served;
		time_next_event[2] = sim_time + expon(mean_service);

		/* Move each customer in queue (if any) up one place. */
		for (i = 1; i <= num_in_q; ++i)
		{
			time_arrival[i] = time_arrival[i + 1];
		}
	}
}

void report(void)
{ /*Report generator function */
	/* Compute and write estimates of desired measures of performance. */
	fprintf(outfile, "_________________________________________________________\n");
	fprintf(outfile, "Tiempo de espera en la cola promedio de %11.3f minutos\n\n", total_service_time / num_clients_served);
	fprintf(outfile, "Número promedio de clientes en la cola %10.3f\n\n", area_num_in_q / sim_time);
	fprintf(outfile, "Utilización de la caja registradora%15.3f\n\n", area_register_status / sim_time);
	fprintf(outfile, "La simulación termina en %12.3f minutos", sim_time);
}

void update_time_avg_stats(void)
{ /* Update area accumulators for time-average statistics. */
	float time_since_last_event;

	/* Compute time since last event, and update last-event-time- marker. */
	time_since_last_event = sim_time - time_last_event;
	time_last_event = sim_time;

	/* Update area under number-in-queue function */
	area_num_in_q += num_in_q * time_since_last_event;

	/* Update area under register-busy indicator function. */
	area_register_status += register_status * time_since_last_event;
}

float expon(float mean)
{ /* Exponential variate generation function. */
	/* Return an exponential random variate with mean "mean". */
	return -mean * log(lcgrand(1));
}

float poisson(float mean) /* Exponential variate generation function. */
{
	int f = rand() % 20;
	return -mean * log((lcgrand(f)));
}
