/* Anghel Ionela-Carmen 332CB */
#include "tema1.h"

double multiply(double nr, int exp) {
    double pp = nr;
    for (int i = 1; i < exp; i++) {
        pp *= nr;
    }  
    return pp;  
}

bool isPP (int n, int exp) {
    /* 0 or less is not consider pp for any exp */
    if (n <= 0)
        return false;
    /* 1 is consider pp for any exp */
    if (n == 1)
        return true;

    double low = 1;
    double high = n;
    double mid;

    /* find number that at power exp is n */
    while ((high - low) > 1e-6) {
        mid = (high + low) / 2.0;
        double pp = multiply(mid, exp);
        if (pp < n) {
            low = mid;
        }
        else {
            high = mid;
        }
    }

    /* check and rectify found number for error
        if given number is a pp, the algoritm will still have a small error,
        given the error, the actual base is either the integer part or the integer part + 1
        of the found number. checking is the integer part is in fact the base for our pp,
        if not then our pp is not a pp */
    int check = multiply((int)mid + 1, exp);
    if (check != n) {
        check = multiply((int)mid, exp);
        if (check != n) {
            /* not pp */
            return false;
        }
    }

    return true;
}

std::map<int, std::list<int>> mapper(char* fileName, int Nexp) {
    FILE *file = fopen( fileName, "r");
    int n;

    std::map<int, std::list<int>> map;
    std::list<int> numbers;
    
    fscanf(file, "%d", &n);
    for (int i = 0; i < n; i++) {
        int nr;
        fscanf(file, "%d", &nr);
        /* add all numbers in list to be proccesed after */
        numbers.push_back(nr);
    }
    fclose(file);

    /* check all numbers as pps for each exponent and create list to be added to 
        result map */
    for (int i = 2; i < Nexp + 2; i++) {
        std::list<int> expList;
        std::list<int>::iterator it;
        for (it = numbers.begin(); it != numbers.end(); it++) {
            if (isPP(*it, i)) {
                expList.push_back(*it);
            }
        }
        map.insert({i, expList});
    }

    return map;
}

void reducer(int exp, std::list<std::list<int>> partialLists) { 
    std::list<int> list;

    std::list<std::list<int>>::iterator it;
    for (it = partialLists.begin(); it != partialLists.end(); it++) {
        std::list<int>::iterator it2;
        for (it2 = (*it).begin(); it2 != (*it).end(); it2++) {
            bool found = (std::find(list.begin(), list.end(), *it2) != list.end());
            if (!found) {
                list.push_back(*it2);
            }
        }
    }

    char name[40];
    int size = list.size();
    sprintf(name, "out%d.txt", exp);
    FILE *file = fopen(name, "w");
    fprintf(file, "%d", size);
    fclose(file);
}

void *f (void *arguments) {
    struct arg_struct *args = (struct arg_struct *)arguments;
    int id = args -> id;
    std::queue<char*> *qfiles = (std::queue<char*> *)args->qfiles;
    std::map<int, std::list<std::list<int>>> *partialSols = (std::map<int, std::list<std::list<int>>>*)args->partialSols; 

    /* mappers */
    if (id < args->N_mapper) {
        /* if id is less than nr of mappers, then thread is a mapper */
        while ((*qfiles).front() != NULL) {
            /* lock in order to get a file */
            pthread_mutex_lock(args->mutex);
            bool hasFile = false;
            char* fileToRead = (*qfiles).front();
            char* fileName = (char*) malloc(100);
            if (fileToRead != NULL) {
                /* the thread managed to get the file */
                /* copy file name to be used */
                strcpy(fileName, fileToRead);
                hasFile = true;
                (*qfiles).pop();
            }
            pthread_mutex_unlock(args->mutex);
            
            /* if thread has a file, start mapping */
            std::map<int, std::list<int>> rezMap;
            if (hasFile) {
                rezMap = mapper(fileName, args->N_reducer);
                free(fileName);
            }

            /* add the partial lists from mapper to the partial solutions */
            pthread_mutex_lock(args->mutex);

            for (std::map<int, std::list<int>>::iterator iter = rezMap.begin(); iter != rezMap.end(); iter++) {
                std::map<int, std::list<std::list<int>>>::iterator pos = (*partialSols).find(iter->first);
                pos->second.push_back(iter->second);
            }

            pthread_mutex_unlock(args->mutex);
        }
    }

    pthread_barrier_wait(args->barrier);

    /* reducers */
    if (id < args->N_reducer) {
        /* if id is less than nr of mappers, then thread is a reducer */
        /* call reducer function with id + 2 cause exponents start from 2, and ids from 0 */
        reducer (id + 2, (*partialSols).find(id + 2)->second);
    }

    pthread_exit(NULL);
}

int max(int a, int b) {
    return (a < b) ? b : a;
}

int main(int argc, char **argv)
{
    /* get argumets */
    int N_mapper = atoi(argv[1]);
    int N_reducer = atoi(argv[2]);
    char* fileName = argv[3];

    /* read file and build file names queue */
    FILE *file = fopen( fileName, "r");
    char *buff; /* buffer for reading */
    int n_files;
    std::queue<char*> qfiles; /* queue to be sent to threds */
    fscanf(file, "%d", &n_files);
    
    for (int i = 0; i < n_files; i++) {
        buff = (char*)malloc(100);
        fscanf(file, "%s", buff);
        qfiles.push(buff);
    }

    fclose(file);

    /* create partial solutions map to be used bt reducers */
    std::map<int, std::list<std::list<int>>> partSolutions; 
    for (int i = 2; i < N_reducer + 2; i++) {
        std::list<std::list<int>> lists;
        partSolutions.insert({i, lists});
    }

    /* get number of threeds to be created */
    int N = max (N_mapper, N_reducer);
    /* list of argument structures for each thread */
    struct arg_struct **args = (struct arg_struct **)malloc((N) * sizeof(struct arg_struct));

    /* initialize barrier and mutex */
	pthread_t threads[N];
    pthread_barrier_t barrier;
    pthread_mutex_t mutex;
    pthread_barrier_init(&barrier, NULL, N);
    pthread_mutex_init(&mutex, NULL);


    /* create treads */
	for (int i = 0; i < N; i++) {
        /* create args structure for new thread */
        args[i] = (struct arg_struct *)malloc(sizeof(struct arg_struct));
        args[i]->id = i;
        args[i]->N_mapper = N_mapper;
        args[i]->N_reducer = N_reducer;
        args[i]->barrier = &barrier;
        args[i]->mutex = &mutex;
        args[i]->qfiles = (void*)&qfiles;
        args[i]->partialSols = (void*)&partSolutions;
    
		int r = pthread_create(&threads[i], NULL, f, (void *)args[i]);

		if (r) {
			printf("Eroare la crearea thread-ului %d\n", i);
			exit(-1);
		}
	}

    /* join threads */
	for (int i = 0; i < N; i++) {
		int r = pthread_join(threads[i], NULL);
        /* free arg struct */
        free (args[i]);

		if (r) {
			printf("Eroare la asteptarea thread-ului %d\n", i);
			exit(-1);
		}
	}


    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);
    /* free args list */
    free(args);

	return 0;
}