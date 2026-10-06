CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -pedantic -O2
LDLIBS  = -lm

grafo_iris: main.c grafo.c grafo.h
	$(CC) $(CFLAGS) -o $@ main.c grafo.c $(LDLIBS)

teste: grafo_iris
	./grafo_iris carregar original_IrisDataset.csv grafo_iris.csv
	./grafo_iris recarregar grafo_iris.csv --verificar

clean:
	rm -f grafo_iris
