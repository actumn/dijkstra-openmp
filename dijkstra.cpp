#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

using namespace std;

#define MAX 100000

int NODE = 0;
int** graph;

void parse_cxv(char* filename) {
	FILE* input_file = fopen(filename, "r");

	char curr;
	char* curr_field = (char*)malloc(64);
	
	// parse header
	for(curr = fgetc(input_file); curr != '\n'; curr = fgetc(input_file)) {
		if (curr == ',') {
			NODE++;
		}
	}
	graph = (int**) malloc(NODE * NODE * sizeof(int));
	for (int i = 0; i < NODE; i++) {
		graph[i] = (int *) malloc(NODE * sizeof(int));
	}

	// parse body
	int i = 1, j = 0; // i: current column, j: current row
	int curr_field_iter = 0;
	for (curr = fgetc(input_file); !feof(input_file); curr = fgetc(input_file)) {
		if (j == 0) {
			do { curr = fgetc(input_file); } while (curr != ',');
			j += 1;
			continue;
		}

		if (curr == ',') {
			if (memcmp(curr_field, "MAX", 3) == 0) {
				graph[i-1][j-1] = MAX;
			} else {
				graph[i-1][j-1] = atoi(curr_field);
			}

			j += 1;
			free(curr_field);
			curr_field = (char*)malloc(64);
			curr_field_iter = 0;
			continue;
		}

		if (curr == '\n') {
			if (memcmp(curr_field, "MAX", 3) == 0) {	// \r\n: widnows new line
				graph[i-1][j-1] = MAX;
			} else {
				graph[i-1][j-1] = atoi(curr_field);
			}

			i += 1;
			j = 0;
			free(curr_field);
			curr_field = (char*)malloc(64);
			curr_field_iter = 0;

			continue;
		}

		curr_field[curr_field_iter] = curr;
		curr_field_iter += 1;
		if (i == j && j == NODE) {
			break;
		}
	}

	free(curr_field);
	fclose(input_file);
}

int dijsktra_serial(const int source, const int target, char** path, int* pass_through, const int)
{
	// ** init **
	int* dist = (int*)malloc(NODE * sizeof(int));
	int* prev = (int*)malloc(NODE * sizeof(int));
	for (int i = 0; i < NODE; i++) {
		dist[i] = graph[source][i];
		prev[i] = graph[source][i] == 0 || graph[source][i] == MAX ? -1 : source; 
	}
	int* selected = (int*)calloc(NODE, sizeof(int));

	// ** algorithm **
	int start = source;
	selected[start] = 1;
	dist[start] = 0;

	while (selected[target] == 0) {
		int min = MAX + 1; // for excaping loop
		int waypoint = 0;

		for (int i = 0; i< NODE; i++) {
			int distance = dist[start] + graph[start][i];
			if (distance < dist[i] && selected[i] == 0) {
				dist[i] = distance;
				prev[i] = start;
			}
			
			if (min > dist[i] && selected[i] == 0) {
				min = dist[i];
				waypoint = i;
			}
		}
		start = waypoint;
		selected[start] = 1;
	}


	// ** end **
	start = target;
	while (start != -1) {
		char temp[6];
		sprintf(temp, "%d", start);

		strcpy(path[*pass_through],
			start < 10 ? "n000" :
			start < 100 ? "n00" :
			start < 1000 ? "n0" : "n");
		strcat(path[*pass_through], temp);
		*pass_through += 1;
		start = prev[start];
	}

	for (int i = *pass_through - 1; i >= 0; i--) {
		printf("%s ", path[i]);
	}

	int result = dist[target];
	free(dist);
	free(prev);
	free(selected);
	return result;
}

int dijsktra(const int source, const int target, char** path, int* pass_through, const int thread_count)
{
	// ** init **
	omp_set_num_threads(thread_count);

	int* dist = (int*)malloc(NODE * sizeof(int));
	int* prev = (int*)malloc(NODE * sizeof(int));
	for (int i = 0; i < NODE; i++) {
		dist[i] = graph[source][i];
		prev[i] = graph[source][i] == 0 || graph[source][i] == MAX ? -1 : source; 
	}
	int* selected = (int*)calloc(NODE, sizeof(int));
	selected[source] = 1;


	// ** algorithm **
	int* min_array = (int*) malloc(thread_count * sizeof(int));
	int* waypoint_array = (int*) calloc(thread_count, sizeof(int));
	for (int i = 0; i < thread_count; i++) {
		min_array[i] = MAX + 2;
	}
	int min; // for excaping loop
	int waypoint = 0;
	while(selected[target] == 0) {
		min = MAX + 1;
		for (int i = 0; i < thread_count; i++) {
			min_array[i] = MAX + 2;
		}

		#pragma omp parallel 
		{
			int thread_id = omp_get_thread_num();
			#pragma omp for 
			for (int i = 0; i < NODE; i++) {
				if (selected[i] == 0 && dist[i] < min_array[thread_id]) {
					min_array[thread_id] = dist[i];
					waypoint_array[thread_id] = i;
				}
			}
			#pragma omp single
			{
				for(int i = 0; i < thread_count; i++) {
					if (min_array[i] < min) {
						min = min_array[i];
						waypoint = waypoint_array[i];
					}
				}
				selected[waypoint] = 1;
			}
			
			#pragma omp for
			for (int i = 0; i < NODE; i++) {
				int distance = min + graph[waypoint][i];
				if (selected[i] == 0 && distance < dist[i]) {
					dist[i] = distance;
					prev[i] = waypoint;
				}
			}
		}
	}
	free(min_array);
	free(waypoint_array);



	// ** end **
	int start = target;
	while (start != -1) {
		char temp[6];
		sprintf(temp, "%d", start);

		strcpy(path[*pass_through],
			start < 10 ? "n000" :
			start < 100 ? "n00" :
			start < 1000 ? "n0" : "n");
		strcat(path[*pass_through], temp);
		*pass_through += 1;
		start = prev[start];
	}

	int result = dist[target];
	free(dist);
	free(prev);
	free(selected);
	return result;
}

int main(int argc, char* argv[]) {
	if(argc < 6){
		 printf("Invalid arguments.\nUsage: dijkstra <filename> <thread_count> <print_path> <source> <dest>.\n");
		 return EXIT_FAILURE;
	}

	char* filename = argv[1];
	int thread_count = atoi(argv[2]);
	int print_path_flag = atoi(argv[3]);
	int start_node = atoi(argv[4]);
	int target_node = atoi(argv[5]);

	parse_cxv(filename);

	char** path = (char**)malloc((NODE + 1) * sizeof(char*));
	for (int i = 0; i < NODE + 1; i++) 
		path[i] = (char*)malloc(6 * sizeof(char));
	int pass_through = 0;

	double start_time = omp_get_wtime();
	int co = dijsktra(start_node, target_node, path, &pass_through, thread_count);
	double end_time = omp_get_wtime();

	FILE* output = fopen("output.txt", "a");
	fprintf(output, "param: %s %d %d %d %d\n", filename, thread_count, print_path_flag, start_node, target_node);

	if (print_path_flag) {
		for (int i = pass_through - 1; i >= 0; i--) {
			fprintf(output, "%s ", path[i]);
		}
		fprintf(output, "\n");
	}

	fprintf(output, "Shortest Path: %d Compute time: %.5lf ms\n\n", co, (end_time - start_time) * 1000.);

	for (int i = 0; i < NODE + 1; i++) 
		free(path[i]);
	free(path);
	for (int i = 0; i < NODE; i++)
		free(graph[i]);
	free(graph);
	fclose(output);
}
 
