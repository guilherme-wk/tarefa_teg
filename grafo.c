/*
 * grafo.c -- implementacao do TAD Grafo (matriz de adjacencias).
 * Tarefa 1_A -- TEG.
 */
#include "grafo.h"

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NUM_ATRIBUTOS 4 /* Sepal.length, Sepal.width, Petal.length, Petal.width */
#define MARCADOR_MATRIZ "matriz_adjacencias"
#define MAGICO_CSV "grafo_iris_csv"

struct Grafo {
    int n;
    int *adj; /* matriz n x n, linha a linha: adj[i*n + j] = nº de arestas i-j */
    GrafoInfo info;
};

#define A(g, i, j) ((g)->adj[(size_t)(i) * (size_t)(g)->n + (size_t)(j)])

/* ================================================================== */
/* Utilitarios                                                        */
/* ================================================================== */

static void set_erro(char *erro, size_t tam, const char *fmt, ...) {
    va_list ap;
    if (erro == NULL || tam == 0) return;
    va_start(ap, fmt);
    vsnprintf(erro, tam, fmt, ap);
    va_end(ap);
}

/* Remove \r e \n do final da linha. */
static void chomp(char *s) {
    size_t k = strlen(s);
    while (k > 0 && (s[k - 1] == '\n' || s[k - 1] == '\r')) s[--k] = '\0';
}

/* Retorna o proximo campo separado por ',' e avanca o cursor.
 * Retorna NULL quando nao ha mais campos. */
static char *proximo_campo(char **cursor) {
    char *ini = *cursor, *virg;
    if (ini == NULL) return NULL;
    virg = strchr(ini, ',');
    if (virg != NULL) {
        *virg = '\0';
        *cursor = virg + 1;
    } else {
        *cursor = NULL;
    }
    return ini;
}

static int parse_double(const char *s, double *out) {
    char *fim;
    double v;
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return -1;
    errno = 0;
    v = strtod(s, &fim);
    while (isspace((unsigned char)*fim)) fim++;
    if (*fim != '\0' || errno == ERANGE || !isfinite(v)) return -1;
    *out = v;
    return 0;
}

static int parse_int(const char *s, int *out) {
    char *fim;
    long v;
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return -1;
    errno = 0;
    v = strtol(s, &fim, 10);
    while (isspace((unsigned char)*fim)) fim++;
    if (*fim != '\0' || errno == ERANGE || v < 0 || v > 1000000000L) return -1;
    *out = (int)v;
    return 0;
}

/* "c12" -> 11 (indice 0-based). Retorna -1 se invalido. */
static int parse_vertice(const char *s, int n) {
    int k;
    if (s == NULL || s[0] != 'c') return -1;
    if (parse_int(s + 1, &k) != 0 || k < 1 || k > n) return -1;
    return k - 1;
}

static int cmp_componentes(const void *pa, const void *pb) {
    const Componente *a = (const Componente *)pa, *b = (const Componente *)pb;
    if (a->tamanho != b->tamanho) return b->tamanho - a->tamanho; /* maior primeiro */
    return a->vertice_inicial - b->vertice_inicial;
}

/* ================================================================== */
/* Construcao / operacoes basicas                                     */
/* ================================================================== */

Grafo *grafo_criar(int n) {
    Grafo *g;
    if (n < 1) return NULL;
    g = (Grafo *)calloc(1, sizeof(Grafo));
    if (g == NULL) return NULL;
    g->adj = (int *)calloc((size_t)n * (size_t)n, sizeof(int));
    if (g->adj == NULL) {
        free(g);
        return NULL;
    }
    g->n = n;
    g->info.total_vertices = n;
    g->info.limiar = LIMIAR_PADRAO;
    if (grafo_recalcular_estrutura(g) != 0) {
        grafo_destruir(g);
        return NULL;
    }
    return g;
}

void grafo_destruir(Grafo *g) {
    if (g == NULL) return;
    free(g->info.est.componentes);
    free(g->adj);
    free(g);
}

int grafo_num_vertices(const Grafo *g) { return g->n; }

int grafo_adicionar_aresta(Grafo *g, int u, int v) {
    if (u < 0 || v < 0 || u >= g->n || v >= g->n) return -1;
    if (u == v) {
        A(g, u, u)++;
    } else {
        A(g, u, v)++;
        A(g, v, u)++;
    }
    return 0;
}

int grafo_multiplicidade(const Grafo *g, int u, int v) {
    if (u < 0 || v < 0 || u >= g->n || v >= g->n) return -1;
    return A(g, u, v);
}

int grafo_tem_aresta(const Grafo *g, int u, int v) {
    int m = grafo_multiplicidade(g, u, v);
    return m > 0;
}

int grafo_grau(const Grafo *g, int v) {
    int j, grau = 0;
    if (v < 0 || v >= g->n) return -1;
    for (j = 0; j < g->n; j++) grau += A(g, v, j);
    grau += A(g, v, v); /* laco contribui com 2 ao grau */
    return grau;
}

const GrafoInfo *grafo_info(const Grafo *g) { return &g->info; }

/* ================================================================== */
/* Propriedades estruturais                                           */
/* ================================================================== */

/* Calcula as propriedades estruturais de g em *out (aloca out->componentes).
 * Retorna 0 em sucesso, -1 se faltar memoria. */
static int calcular_estrutura(const Grafo *g, Estrutura *out) {
    int n = g->n, i, j, k;
    int *visitado, *pilha;
    Componente *comp;
    int ncomp = 0;

    memset(out, 0, sizeof(*out));
    out->grau_max = -1;
    out->grau_min = -1;

    /* graus, lacos, arestas multiplas */
    for (i = 0; i < n; i++) {
        int grau = grafo_grau(g, i);
        if (out->grau_max < 0 || grau > out->grau_max) {
            out->grau_max = grau;
            out->v_grau_max = i;
        }
        if (out->grau_min < 0 || grau < out->grau_min) {
            out->grau_min = grau;
            out->v_grau_min = i;
        }
        out->num_lacos += A(g, i, i);
        for (j = i + 1; j < n; j++) {
            int m = A(g, i, j);
            out->num_arestas += m;
            if (m > 1) out->num_arestas_multiplas += m - 1;
        }
    }
    out->num_arestas += out->num_lacos;
    out->simples = (out->num_lacos == 0 && out->num_arestas_multiplas == 0);

    /* componentes conexos: busca em profundidade iterativa */
    visitado = (int *)calloc((size_t)n, sizeof(int));
    pilha = (int *)malloc((size_t)n * sizeof(int));
    comp = (Componente *)malloc((size_t)n * sizeof(Componente));
    if (visitado == NULL || pilha == NULL || comp == NULL) {
        free(visitado);
        free(pilha);
        free(comp);
        return -1;
    }
    for (i = 0; i < n; i++) {
        int topo = 0, tam = 0;
        if (visitado[i]) continue;
        visitado[i] = 1;
        pilha[topo++] = i;
        while (topo > 0) {
            int u = pilha[--topo];
            tam++;
            for (k = 0; k < n; k++) {
                if (k != u && A(g, u, k) > 0 && !visitado[k]) {
                    visitado[k] = 1;
                    pilha[topo++] = k;
                }
            }
        }
        comp[ncomp].tamanho = tam;
        comp[ncomp].vertice_inicial = i; /* i e o menor vertice do componente */
        ncomp++;
    }
    qsort(comp, (size_t)ncomp, sizeof(Componente), cmp_componentes);
    out->num_componentes = ncomp;
    out->componentes = comp;
    free(visitado);
    free(pilha);
    return 0;
}

int grafo_recalcular_estrutura(Grafo *g) {
    Estrutura nova;
    if (calcular_estrutura(g, &nova) != 0) return -1;
    free(g->info.est.componentes);
    g->info.est = nova;
    return 0;
}

/* ================================================================== */
/* Carga primaria                                                     */
/* ================================================================== */

/* Le o CSV Iris. Devolve vetor plano com n*4 valores (malloc). */
static double *ler_observacoes(const char *caminho, int *n_out,
                               char *erro, size_t erro_tam) {
    FILE *f = fopen(caminho, "r");
    char linha[1024];
    double *obs = NULL;
    int n = 0, cap = 0, numlinha = 0, primeira_util = 1;

    if (f == NULL) {
        set_erro(erro, erro_tam, "nao foi possivel abrir '%s': %s", caminho,
                 strerror(errno));
        return NULL;
    }
    while (fgets(linha, sizeof(linha), f) != NULL) {
        char *p = linha, *campos[NUM_ATRIBUTOS + 1];
        double v[NUM_ATRIBUTOS];
        int k, ok = 1;

        numlinha++;
        if (strchr(linha, '\n') == NULL && !feof(f)) {
            set_erro(erro, erro_tam, "linha %d muito longa", numlinha);
            goto falha;
        }
        if (numlinha == 1 && (unsigned char)p[0] == 0xEF &&
            (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF)
            p += 3; /* descarta BOM UTF-8 */
        chomp(p);
        if (p[0] == '\0') continue; /* linha em branco */

        /* separa em exatamente 1 + 4 campos */
        for (k = 0; k < NUM_ATRIBUTOS + 1; k++) {
            campos[k] = proximo_campo(&p);
            if (campos[k] == NULL) {
                ok = 0;
                break;
            }
        }
        if (ok && p != NULL) ok = 0; /* sobraram campos */
        if (!ok) {
            set_erro(erro, erro_tam,
                     "linha %d: esperado 5 colunas (especie + 4 medidas)",
                     numlinha);
            goto falha;
        }
        /* campos[0] = especie -> ignorado (item b) */
        for (k = 0; k < NUM_ATRIBUTOS; k++)
            if (parse_double(campos[k + 1], &v[k]) != 0) break;
        if (k < NUM_ATRIBUTOS) {
            if (primeira_util) { /* primeira linha util nao numerica = cabecalho */
                primeira_util = 0;
                continue;
            }
            set_erro(erro, erro_tam, "linha %d: valor numerico invalido '%s'",
                     numlinha, campos[k + 1]);
            goto falha;
        }
        primeira_util = 0;

        if (n == cap) {
            int ncap = cap ? cap * 2 : 256;
            double *t = (double *)realloc(obs, (size_t)ncap * NUM_ATRIBUTOS * sizeof(double));
            if (t == NULL) {
                set_erro(erro, erro_tam, "memoria insuficiente");
                goto falha;
            }
            obs = t;
            cap = ncap;
        }
        for (k = 0; k < NUM_ATRIBUTOS; k++) obs[(size_t)n * NUM_ATRIBUTOS + k] = v[k];
        n++;
    }
    fclose(f);
    if (n < 2) {
        set_erro(erro, erro_tam, "a base precisa ter ao menos 2 observacoes (lidas: %d)", n);
        free(obs);
        return NULL;
    }
    *n_out = n;
    return obs;

falha:
    fclose(f);
    free(obs);
    return NULL;
}

Grafo *grafo_carregar_iris_csv(const char *caminho, double limiar,
                               char *erro, size_t erro_tam) {
    int n, i, j, k;
    double *obs, *de = NULL, *den = NULL;
    Grafo *g = NULL;
    GrafoInfo *info;
    int primeiro = 1;

    obs = ler_observacoes(caminho, &n, erro, erro_tam);
    if (obs == NULL) return NULL;

    /* ---- tabela de distancias euclideanas DE (todos os pares) ---- */
    de = (double *)calloc((size_t)n * (size_t)n, sizeof(double));
    den = (double *)calloc((size_t)n * (size_t)n, sizeof(double));
    g = grafo_criar(n);
    if (de == NULL || den == NULL || g == NULL) {
        set_erro(erro, erro_tam, "memoria insuficiente");
        goto falha;
    }
    info = &g->info;
    info->limiar = limiar;

    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            double soma = 0.0, d;
            for (k = 0; k < NUM_ATRIBUTOS; k++) {
                double dif = obs[(size_t)i * NUM_ATRIBUTOS + k] -
                             obs[(size_t)j * NUM_ATRIBUTOS + k];
                soma += dif * dif;
            }
            d = sqrt(soma);
            de[(size_t)i * n + j] = de[(size_t)j * n + i] = d;
            /* empates: prevalece o primeiro par encontrado (ordem i<j) */
            if (primeiro || d > info->maior_de.valor) {
                info->maior_de.valor = d;
                info->maior_de.par = (Par){i, j};
            }
            if (primeiro || d < info->menor_de.valor) {
                info->menor_de.valor = d;
                info->menor_de.par = (Par){i, j};
            }
            primeiro = 0;
        }
    }

    /* ---- normalizacao min-max da tabela: DEN = (DE-min)/(max-min) ---- */
    {
        double min = info->menor_de.valor, amp = info->maior_de.valor - min;
        primeiro = 1;
        for (i = 0; i < n; i++) {
            for (j = i + 1; j < n; j++) {
                double d = (amp > 0.0) ? (de[(size_t)i * n + j] - min) / amp : 0.0;
                den[(size_t)i * n + j] = den[(size_t)j * n + i] = d;
                if (primeiro || d > info->maior_den.valor) {
                    info->maior_den.valor = d;
                    info->maior_den.par = (Par){i, j};
                }
                if (primeiro || d < info->menor_den.valor) {
                    info->menor_den.valor = d;
                    info->menor_den.par = (Par){i, j};
                }
                primeiro = 0;
            }
        }
    }

    /* ---- limiar: aresta (vi,vj) sse DEN <= limiar (sem lacos, i<j) ---- */
    for (i = 0; i < n; i++)
        for (j = i + 1; j < n; j++)
            if (den[(size_t)i * n + j] <= limiar) grafo_adicionar_aresta(g, i, j);

    if (grafo_recalcular_estrutura(g) != 0) {
        set_erro(erro, erro_tam, "memoria insuficiente");
        goto falha;
    }
    free(obs);
    free(de);
    free(den);
    return g;

falha:
    free(obs);
    free(de);
    free(den);
    grafo_destruir(g);
    return NULL;
}

/* ================================================================== */
/* Persistencia                                                       */
/* ================================================================== */

int grafo_salvar_csv(const Grafo *g, const char *caminho,
                     char *erro, size_t erro_tam) {
    const GrafoInfo *in = &g->info;
    const Estrutura *e = &in->est;
    FILE *f = fopen(caminho, "w");
    int i, j;

    if (f == NULL) {
        set_erro(erro, erro_tam, "nao foi possivel criar '%s': %s", caminho,
                 strerror(errno));
        return -1;
    }
    /* ---- cabecalho (enunciado, item e) ---- */
    fprintf(f, "%s,versao,1\n", MAGICO_CSV);
    fprintf(f, "total_vertices,%d\n", in->total_vertices);                      /* i    */
    fprintf(f, "limiar_DEN,%.10f\n", in->limiar);
    fprintf(f, "maior_DE,%.10f,c%d,c%d\n", in->maior_de.valor,                 /* ii   */
            in->maior_de.par.u + 1, in->maior_de.par.v + 1);
    fprintf(f, "menor_DE,%.10f,c%d,c%d\n", in->menor_de.valor,                 /* iii  */
            in->menor_de.par.u + 1, in->menor_de.par.v + 1);
    fprintf(f, "maior_DEN,%.10f,c%d,c%d\n", in->maior_den.valor,               /* iv   */
            in->maior_den.par.u + 1, in->maior_den.par.v + 1);
    fprintf(f, "menor_DEN,%.10f,c%d,c%d\n", in->menor_den.valor,               /* v    */
            in->menor_den.par.u + 1, in->menor_den.par.v + 1);
    fprintf(f, "grau_maximo,%d,c%d\n", e->grau_max, e->v_grau_max + 1);        /* vi   */
    fprintf(f, "grau_minimo,%d,c%d\n", e->grau_min, e->v_grau_min + 1);
    fprintf(f, "tipo_grafo,%s\n", e->simples ? "simples" : "multigrafo");      /* vii  */
    fprintf(f, "numero_lacos,%d\n", e->num_lacos);
    fprintf(f, "numero_arestas_multiplas,%d\n", e->num_arestas_multiplas);
    fprintf(f, "numero_arestas,%d\n", e->num_arestas);
    fprintf(f, "numero_componentes,%d\n", e->num_componentes);                 /* viii */
    for (i = 0; i < e->num_componentes; i++)
        fprintf(f, "componente,%d,tamanho,%d,vertice_inicial,c%d\n", i + 1,
                e->componentes[i].tamanho, e->componentes[i].vertice_inicial + 1);

    /* ---- corpo: matriz de adjacencias ---- */
    fprintf(f, "%s\n", MARCADOR_MATRIZ);
    for (i = 0; i < g->n; i++) {
        for (j = 0; j < g->n; j++) fprintf(f, j ? ",%d" : "%d", A(g, i, j));
        fputc('\n', f);
    }
    if (ferror(f) || fclose(f) != 0) {
        set_erro(erro, erro_tam, "erro de escrita em '%s'", caminho);
        return -1;
    }
    return 0;
}

/* Le um extremo "chave,valor,cU,cV" ja separado em campos. */
static int ler_extremo(char *cursor, int n, Extremo *out) {
    char *val = proximo_campo(&cursor), *u = proximo_campo(&cursor),
         *v = proximo_campo(&cursor);
    if (val == NULL || u == NULL || v == NULL || cursor != NULL) return -1;
    if (parse_double(val, &out->valor) != 0) return -1;
    out->par.u = parse_vertice(u, n);
    out->par.v = parse_vertice(v, n);
    return (out->par.u < 0 || out->par.v < 0) ? -1 : 0;
}

/* Le "chave,k,cV" (grau + vertice). */
static int ler_grau(char *cursor, int n, int *grau, int *vert) {
    char *val = proximo_campo(&cursor), *v = proximo_campo(&cursor);
    if (val == NULL || v == NULL || cursor != NULL) return -1;
    if (parse_int(val, grau) != 0) return -1;
    *vert = parse_vertice(v, n);
    return (*vert < 0) ? -1 : 0;
}

Grafo *grafo_carregar_csv(const char *caminho, char *erro, size_t erro_tam) {
    FILE *f = fopen(caminho, "r");
    char *linha = NULL;
    size_t cap = 16384;
    Grafo *g = NULL;
    GrafoInfo *in = NULL;
    int n = 0, numlinha = 0, achou_matriz = 0, ncomp_lidos = 0, i, j;
    unsigned vistos = 0; /* bitmask das chaves obrigatorias ja lidas */
    enum { K_TOTAL = 1, K_MAIOR_DE = 2, K_MENOR_DE = 4, K_MAIOR_DEN = 8,
           K_MENOR_DEN = 16, K_GMAX = 32, K_GMIN = 64, K_TIPO = 128,
           K_LACOS = 256, K_MULT = 512, K_ARESTAS = 1024, K_NCOMP = 2048,
           K_LIMIAR = 4096, K_TODAS = 8191 };

    if (f == NULL) {
        set_erro(erro, erro_tam, "nao foi possivel abrir '%s': %s", caminho,
                 strerror(errno));
        return NULL;
    }
    linha = (char *)malloc(cap);
    if (linha == NULL) {
        set_erro(erro, erro_tam, "memoria insuficiente");
        fclose(f);
        return NULL;
    }

    /* ---------------- cabecalho ---------------- */
    while (!achou_matriz && fgets(linha, (int)cap, f) != NULL) {
        char *cur, *chave, *val;
        numlinha++;
        chomp(linha);
        if (linha[0] == '\0') continue;
        if (strcmp(linha, MARCADOR_MATRIZ) == 0) {
            achou_matriz = 1;
            break;
        }
        cur = linha;
        chave = proximo_campo(&cur);

        if (numlinha == 1) {
            if (strcmp(chave, MAGICO_CSV) != 0) {
                set_erro(erro, erro_tam, "'%s' nao e um arquivo de grafo valido", caminho);
                goto falha;
            }
            continue;
        }
        if (strcmp(chave, "total_vertices") == 0) {
            val = proximo_campo(&cur);
            if (val == NULL || parse_int(val, &n) != 0 || n < 1) {
                set_erro(erro, erro_tam, "linha %d: total_vertices invalido", numlinha);
                goto falha;
            }
            g = grafo_criar(n);
            if (g == NULL) {
                set_erro(erro, erro_tam, "memoria insuficiente");
                goto falha;
            }
            in = &g->info;
            free(linha);
            cap = (size_t)n * 12 + 64; /* cabe uma linha da matriz */
            if (cap < 4096) cap = 4096;
            linha = (char *)malloc(cap);
            if (linha == NULL) {
                set_erro(erro, erro_tam, "memoria insuficiente");
                goto falha;
            }
            vistos |= K_TOTAL;
            continue;
        }
        if (g == NULL) {
            set_erro(erro, erro_tam, "linha %d: 'total_vertices' deve ser a primeira chave", numlinha);
            goto falha;
        }
        if (strcmp(chave, "limiar_DEN") == 0) {
            val = proximo_campo(&cur);
            if (val == NULL || parse_double(val, &in->limiar) != 0) goto linha_ruim;
            vistos |= K_LIMIAR;
        } else if (strcmp(chave, "maior_DE") == 0) {
            if (ler_extremo(cur, n, &in->maior_de) != 0) goto linha_ruim;
            vistos |= K_MAIOR_DE;
        } else if (strcmp(chave, "menor_DE") == 0) {
            if (ler_extremo(cur, n, &in->menor_de) != 0) goto linha_ruim;
            vistos |= K_MENOR_DE;
        } else if (strcmp(chave, "maior_DEN") == 0) {
            if (ler_extremo(cur, n, &in->maior_den) != 0) goto linha_ruim;
            vistos |= K_MAIOR_DEN;
        } else if (strcmp(chave, "menor_DEN") == 0) {
            if (ler_extremo(cur, n, &in->menor_den) != 0) goto linha_ruim;
            vistos |= K_MENOR_DEN;
        } else if (strcmp(chave, "grau_maximo") == 0) {
            if (ler_grau(cur, n, &in->est.grau_max, &in->est.v_grau_max) != 0) goto linha_ruim;
            vistos |= K_GMAX;
        } else if (strcmp(chave, "grau_minimo") == 0) {
            if (ler_grau(cur, n, &in->est.grau_min, &in->est.v_grau_min) != 0) goto linha_ruim;
            vistos |= K_GMIN;
        } else if (strcmp(chave, "tipo_grafo") == 0) {
            val = proximo_campo(&cur);
            if (val == NULL) goto linha_ruim;
            if (strcmp(val, "simples") == 0) in->est.simples = 1;
            else if (strcmp(val, "multigrafo") == 0) in->est.simples = 0;
            else goto linha_ruim;
            vistos |= K_TIPO;
        } else if (strcmp(chave, "numero_lacos") == 0) {
            val = proximo_campo(&cur);
            if (val == NULL || parse_int(val, &in->est.num_lacos) != 0) goto linha_ruim;
            vistos |= K_LACOS;
        } else if (strcmp(chave, "numero_arestas_multiplas") == 0) {
            val = proximo_campo(&cur);
            if (val == NULL || parse_int(val, &in->est.num_arestas_multiplas) != 0) goto linha_ruim;
            vistos |= K_MULT;
        } else if (strcmp(chave, "numero_arestas") == 0) {
            val = proximo_campo(&cur);
            if (val == NULL || parse_int(val, &in->est.num_arestas) != 0) goto linha_ruim;
            vistos |= K_ARESTAS;
        } else if (strcmp(chave, "numero_componentes") == 0) {
            int k;
            val = proximo_campo(&cur);
            if (val == NULL || parse_int(val, &k) != 0 || k < 1 || k > n) goto linha_ruim;
            free(in->est.componentes);
            in->est.componentes = (Componente *)calloc((size_t)k, sizeof(Componente));
            if (in->est.componentes == NULL) {
                set_erro(erro, erro_tam, "memoria insuficiente");
                goto falha;
            }
            in->est.num_componentes = k;
            vistos |= K_NCOMP;
        } else if (strcmp(chave, "componente") == 0) {
            /* componente,<id>,tamanho,<t>,vertice_inicial,c<v> */
            char *id = proximo_campo(&cur), *r1 = proximo_campo(&cur),
                 *tam = proximo_campo(&cur), *r2 = proximo_campo(&cur),
                 *vi = proximo_campo(&cur);
            int idc, t, v;
            if (!(vistos & K_NCOMP) || id == NULL || r1 == NULL || tam == NULL ||
                r2 == NULL || vi == NULL || cur != NULL ||
                strcmp(r1, "tamanho") != 0 || strcmp(r2, "vertice_inicial") != 0 ||
                parse_int(id, &idc) != 0 || idc != ncomp_lidos + 1 ||
                idc > in->est.num_componentes || parse_int(tam, &t) != 0 || t < 1 ||
                (v = parse_vertice(vi, n)) < 0)
                goto linha_ruim;
            in->est.componentes[ncomp_lidos].tamanho = t;
            in->est.componentes[ncomp_lidos].vertice_inicial = v;
            ncomp_lidos++;
        } else {
            set_erro(erro, erro_tam, "linha %d: chave desconhecida '%s'", numlinha, chave);
            goto falha;
        }
        continue;

    linha_ruim:
        set_erro(erro, erro_tam, "linha %d: formato invalido (%s)", numlinha, chave);
        goto falha;
    }

    if (!achou_matriz || (vistos & K_TODAS) != K_TODAS ||
        ncomp_lidos != in->est.num_componentes) {
        set_erro(erro, erro_tam, "cabecalho incompleto em '%s'", caminho);
        goto falha;
    }
    in->total_vertices = n;

    /* ---------------- matriz de adjacencias ---------------- */
    for (i = 0; i < n; i++) {
        char *cur;
        if (fgets(linha, (int)cap, f) == NULL) {
            set_erro(erro, erro_tam, "matriz truncada: faltam linhas (lidas %d de %d)", i, n);
            goto falha;
        }
        numlinha++;
        chomp(linha);
        cur = linha;
        for (j = 0; j < n; j++) {
            char *c = proximo_campo(&cur);
            int m;
            if (c == NULL || parse_int(c, &m) != 0) {
                set_erro(erro, erro_tam, "linha %d: matriz com valor ausente/invalido na coluna %d",
                         numlinha, j + 1);
                goto falha;
            }
            A(g, i, j) = m;
        }
        if (cur != NULL) {
            set_erro(erro, erro_tam, "linha %d: colunas em excesso na matriz", numlinha);
            goto falha;
        }
    }
    for (i = 0; i < n; i++)
        for (j = i + 1; j < n; j++)
            if (A(g, i, j) != A(g, j, i)) {
                set_erro(erro, erro_tam, "matriz nao simetrica em (c%d,c%d)", i + 1, j + 1);
                goto falha;
            }

    free(linha);
    fclose(f);
    return g;

falha:
    free(linha);
    fclose(f);
    grafo_destruir(g);
    return NULL;
}

/* ================================================================== */
/* Relatorios                                                         */
/* ================================================================== */

void grafo_imprimir_resumo(const Grafo *g, FILE *out) {
    const GrafoInfo *in = &g->info;
    const Estrutura *e = &in->est;
    int i, max_listados = 20;

    fprintf(out, "=== Resumo do grafo ===\n");
    fprintf(out, "Total de vertices lidos : %d\n", in->total_vertices);
    fprintf(out, "Limiar aplicado (DEN)   : %.4f\n", in->limiar);
    fprintf(out, "Maior DE                : %.6f  (c%d, c%d)\n", in->maior_de.valor,
            in->maior_de.par.u + 1, in->maior_de.par.v + 1);
    fprintf(out, "Menor DE                : %.6f  (c%d, c%d)\n", in->menor_de.valor,
            in->menor_de.par.u + 1, in->menor_de.par.v + 1);
    fprintf(out, "Maior DEN               : %.6f  (c%d, c%d)\n", in->maior_den.valor,
            in->maior_den.par.u + 1, in->maior_den.par.v + 1);
    fprintf(out, "Menor DEN               : %.6f  (c%d, c%d)\n", in->menor_den.valor,
            in->menor_den.par.u + 1, in->menor_den.par.v + 1);
    fprintf(out, "Grau maximo             : %d  (ex.: c%d)\n", e->grau_max, e->v_grau_max + 1);
    fprintf(out, "Grau minimo             : %d  (ex.: c%d)\n", e->grau_min, e->v_grau_min + 1);
    fprintf(out, "Tipo                    : %s\n",
            e->simples ? "grafo simples" : "multigrafo");
    fprintf(out, "Lacos                   : %d\n", e->num_lacos);
    fprintf(out, "Arestas multiplas       : %d\n", e->num_arestas_multiplas);
    fprintf(out, "Numero de arestas       : %d\n", e->num_arestas);
    fprintf(out, "Componentes conexos     : %d\n", e->num_componentes);
    for (i = 0; i < e->num_componentes && i < max_listados; i++)
        fprintf(out, "  componente %2d: %3d vertices (a partir de c%d)\n", i + 1,
                e->componentes[i].tamanho, e->componentes[i].vertice_inicial + 1);
    if (e->num_componentes > max_listados)
        fprintf(out, "  ... (+%d componentes; lista completa no CSV)\n",
                e->num_componentes - max_listados);
}

int grafo_verificar(const Grafo *g, FILE *out) {
    Estrutura r;
    const Estrutura *e = &g->info.est;
    int i, divergencias = 0;

    if (calcular_estrutura(g, &r) != 0) {
        fprintf(out, "verificacao: memoria insuficiente\n");
        return -1;
    }
#define CONFERE(campo, nome)                                                   \
    if (r.campo != e->campo) {                                                 \
        fprintf(out, "  DIVERGENCIA em %s: cabecalho=%d, matriz=%d\n", nome,   \
                e->campo, r.campo);                                            \
        divergencias++;                                                        \
    }
    CONFERE(grau_max, "grau_maximo")
    CONFERE(grau_min, "grau_minimo")
    CONFERE(simples, "tipo_grafo")
    CONFERE(num_lacos, "numero_lacos")
    CONFERE(num_arestas_multiplas, "numero_arestas_multiplas")
    CONFERE(num_arestas, "numero_arestas")
    CONFERE(num_componentes, "numero_componentes")
#undef CONFERE
    if (r.num_componentes == e->num_componentes) {
        for (i = 0; i < r.num_componentes; i++)
            if (r.componentes[i].tamanho != e->componentes[i].tamanho ||
                r.componentes[i].vertice_inicial != e->componentes[i].vertice_inicial) {
                fprintf(out, "  DIVERGENCIA no componente %d\n", i + 1);
                divergencias++;
            }
    }
    free(r.componentes);
    fprintf(out, "Verificacao cabecalho x matriz: %s\n",
            divergencias == 0 ? "OK (coerente)" : "FALHOU");
    return divergencias == 0 ? 0 : 1;
}
