/*
 * grafo.h -- TAD Grafo (matriz de adjacencias) com carga primaria a partir
 *            da base Iris em CSV.  Tarefa 1_A -- TEG.
 *
 * O tipo Grafo e opaco: o programa cliente so manipula o grafo atraves das
 * funcoes declaradas aqui.
 *
 * Convencoes:
 *   - Vertices sao numerados internamente de 0 a n-1 e exibidos como
 *     c1 ... cn (c1 = primeira observacao do CSV).
 *   - O grafo nao e direcionado: a matriz de adjacencias e simetrica.
 */
#ifndef GRAFO_H
#define GRAFO_H

#include <stddef.h>
#include <stdio.h>

#define LIMIAR_PADRAO 0.3 /* limiar aplicado sobre a DEN (enunciado, item d) */

typedef struct Grafo Grafo; /* tipo opaco */

/* Par de vertices (indices 0-based). */
typedef struct {
    int u, v;
} Par;

/* Um valor extremo (maior/menor) e o par de vertices que o determinou. */
typedef struct {
    double valor;
    Par par;
} Extremo;

/* Componente conexo: tamanho e menor vertice pertencente a ele. */
typedef struct {
    int tamanho;
    int vertice_inicial;
} Componente;

/* Propriedades estruturais (derivadas da matriz de adjacencias). */
typedef struct {
    int grau_max, v_grau_max; /* grau maximo e um vertice que o atinge */
    int grau_min, v_grau_min; /* grau minimo e um vertice que o atinge */
    int simples;              /* 1 = grafo simples, 0 = multigrafo     */
    int num_lacos;
    int num_arestas_multiplas; /* arestas "extras" alem da primeira     */
    int num_arestas;
    int num_componentes;
    Componente *componentes;   /* ordenados por tamanho (decrescente)   */
} Estrutura;

/* Metadados do grafo (cabecalho do CSV de persistencia, item e). */
typedef struct {
    int total_vertices;
    double limiar;
    Extremo maior_de, menor_de;   /* distancias euclideanas            */
    Extremo maior_den, menor_den; /* distancias euclideanas normalizadas */
    Estrutura est;
} GrafoInfo;

/* ------------------------------------------------------------------ */
/* Construcao / destruicao                                            */
/* ------------------------------------------------------------------ */

/* Cria grafo com n vertices e nenhuma aresta. Retorna NULL se falhar. */
Grafo *grafo_criar(int n);
void grafo_destruir(Grafo *g);

/* ------------------------------------------------------------------ */
/* Operacoes basicas                                                  */
/* ------------------------------------------------------------------ */

int grafo_num_vertices(const Grafo *g);

/* Adiciona uma aresta {u,v}. Se u == v adiciona um laco.
 * Retorna 0 em sucesso, -1 se algum indice for invalido.
 * Apos modificar o grafo, chame grafo_recalcular_estrutura(). */
int grafo_adicionar_aresta(Grafo *g, int u, int v);

/* Numero de arestas entre u e v (0 = nao adjacentes). -1 se invalido. */
int grafo_multiplicidade(const Grafo *g, int u, int v);
int grafo_tem_aresta(const Grafo *g, int u, int v);

/* Grau do vertice (laco conta 2). -1 se invalido. */
int grafo_grau(const Grafo *g, int v);

/* Recalcula graus, simplicidade e componentes a partir da matriz. */
int grafo_recalcular_estrutura(Grafo *g);

/* Metadados e propriedades do grafo (somente leitura). */
const GrafoInfo *grafo_info(const Grafo *g);

/* ------------------------------------------------------------------ */
/* Carga primaria (CSV Iris -> DE -> DEN -> limiar -> adjacencias)    */
/* ------------------------------------------------------------------ */

/* Le a base Iris (a 1a coluna, nome da especie, e ignorada; as 4 colunas
 * seguintes sao Sepal.length, Sepal.width, Petal.length, Petal.width),
 * monta a tabela de DE, normaliza para [0,1] (min-max sobre todos os pares
 * distintos) e cria aresta (vi,vj) sse DEN(vi,vj) <= limiar, i != j.
 * Em caso de erro retorna NULL e escreve a mensagem em `erro`. */
Grafo *grafo_carregar_iris_csv(const char *caminho, double limiar,
                               char *erro, size_t erro_tam);

/* ------------------------------------------------------------------ */
/* Persistencia                                                       */
/* ------------------------------------------------------------------ */

/* Grava o grafo em CSV: cabecalho com os metadados (item e, i..viii)
 * seguido da matriz de adjacencias. Retorna 0 em sucesso. */
int grafo_salvar_csv(const Grafo *g, const char *caminho,
                     char *erro, size_t erro_tam);

/* Recarrega um grafo gravado por grafo_salvar_csv(), SEM reler a base
 * original e SEM recalcular DE/DEN. Retorna NULL em caso de erro. */
Grafo *grafo_carregar_csv(const char *caminho, char *erro, size_t erro_tam);

/* ------------------------------------------------------------------ */
/* Relatorios                                                         */
/* ------------------------------------------------------------------ */

void grafo_imprimir_resumo(const Grafo *g, FILE *out);

/* Recalcula as propriedades estruturais a partir da matriz e compara com
 * os valores guardados nos metadados. Retorna 0 se coerentes. */
int grafo_verificar(const Grafo *g, FILE *out);

#endif /* GRAFO_H */
