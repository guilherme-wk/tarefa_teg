# Tarefa 1_A — TEG: grafo da base Iris (matriz de adjacências em C)

## Arquivos
| Arquivo | Conteúdo |
|---|---|
| `grafo.h` / `grafo.c` | TAD `Grafo` (tipo opaco, matriz de adjacências) |
| `main.c` | menu interativo no terminal |
| `Makefile` | `make` compila; `make teste` roda carga + salvar + recarga + verificação via menu |
| `visualizar_grafo.py` | visualização 3D interativa (Plotly), chamada pela opção 8 do menu |
| `grafo_iris.csv` | grafo persistido gerado com limiar 0,3 (exemplo) |

## Uso
```bash
pip install plotly numpy      # só para a visualização 3D
make
./grafo_iris                  # execute DENTRO da pasta do projeto
```
Menu (ENTER em qualquer pergunta usa o valor padrão entre colchetes; o limiar aceita `0.3` ou `0,3`):
```
1) Carga primária (ler CSV da base Iris)      5) Verificar cabeçalho x matriz
2) Salvar grafo em CSV                        6) Consultar vértice (grau e vizinhos)
3) Recarregar grafo de CSV                    7) Verificar adjacência entre dois vértices
4) Mostrar resumo do grafo                    8) Visualizar em 3D (script Python)
0) Sair
```
Fluxo típico: `1` → `2` → `8`. Numa próxima sessão: `3` (recarrega sem reler a base nem recalcular DEN) → `8`.
A opção 8 salva o grafo antes (se necessário) e executa `python3 visualizar_grafo.py` (`python` no Windows);
o navegador abre com o gráfico, que gira com o mouse.

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
Menu testado com entradas válidas/inválidas, EOF, sobrescrita e caminhos com espaço/aspas. Matriz, graus, componentes e extremos conferidos contra uma implementação independente em Python/numpy
(limiares 0,3 / 0,15 / 0,1 / 0,05); sem erros no AddressSanitizer/UBSan; arquivos inválidos
(colunas faltando, valor não numérico, matriz truncada/assimétrica) são rejeitados com mensagem.