# Tarefa 1_A — TEG: grafo da base Iris (matriz de adjacências em C)

## Arquivos
| Arquivo | Conteúdo |
|---|---|
| `grafo.h` / `grafo.c` | TAD `Grafo` (tipo opaco, matriz de adjacências) |
| `main.c` | programa de demonstração (`carregar` / `recarregar`) |
| `Makefile` | `make` compila; `make teste` roda carga + recarga + verificação |
| `visualizar_grafo.py` | visualização 3D interativa (Plotly) |
| `grafo_iris.csv` | grafo persistido gerado com limiar 0,3 (exemplo) |
| `grafo_iris_3d.html` | visualização 3D já gerada (abrir no navegador; requer internet p/ carregar plotly.js) |

## Uso
```bash
make
./grafo_iris carregar original_IrisDataset.csv grafo_iris.csv        # carga primária + persistência
./grafo_iris recarregar grafo_iris.csv --verificar                   # recarga SEM reler a base / recalcular DEN
pip install plotly numpy
python3 visualizar_grafo.py                                          # abre no navegador (gire com o mouse)
```
Limiar opcional: `./grafo_iris carregar base.csv saida.csv 0.15`.

## Pipeline da carga primária (observação a do enunciado)
CSV → (ignora coluna 1, lê as 4 medidas) → tabela DE (distância euclideana, todos os pares)
→ tabela DEN = (DE − min DE) / (max DE − min DE), min/max sobre os pares i≠j
→ aresta (vi,vj) sse DEN ≤ 0,3 e i<j (sem laços, sem arestas múltiplas) → matriz de adjacências.

## Formato do CSV de persistência
Cabeçalho em linhas `chave,valor[,...]` (itens i–viii do enunciado), seguido da linha
`matriz_adjacencias` e das n linhas da matriz. Vértices aparecem como `c1…c150`.
```
total_vertices,150            maior_DE,<valor>,c14,c119     menor_DE,<valor>,c102,c143
maior_DEN,...                 menor_DEN,...                 grau_maximo,93,c78 / grau_minimo,30,c119
tipo_grafo,simples            numero_lacos,0                numero_arestas_multiplas,0
numero_componentes,1          componente,1,tamanho,150,vertice_inicial,c1
```

## Decisões de projeto
* **Empates** em maior/menor DE/DEN: vale o primeiro par encontrado na ordem i<j.
* **Menor DE = 0** (c102 e c143 são observações idênticas na base) — por isso a menor DEN também é 0.
* **Visualização 3D**: as observações têm 4 medidas; por padrão são projetadas nos 3 componentes
  principais (PCA, ~99,5% da variância). Alternativa: `--eixos atributos --attrs 0 2 3`.
* **Fonte do script Python**: escrito com base na documentação do Plotly
  (https://plotly.com/python/3d-scatter-plots/ e https://plotly.com/python/3d-line-plots/).
  Se usar também o script do Moodle, cite-o junto.

## Validação feita
Matriz, graus, componentes e extremos conferidos contra uma implementação independente em Python/numpy
(limiares 0,3 / 0,15 / 0,1 / 0,05); sem erros no AddressSanitizer/UBSan; arquivos inválidos
(colunas faltando, valor não numérico, matriz truncada/assimétrica) são rejeitados com mensagem.
