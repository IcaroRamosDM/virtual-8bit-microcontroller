# Como o Microcontrolador Virtual de 8 Bits Funciona

[Read this documentation in English.](HOW_IT_WORKS.md)

## Objetivo

Este projeto modela em software um pequeno microcontrolador de 8 bits. Ele possui duas partes relacionadas:

1. O **simulador** implementa a CPU, a memória, o decodificador de instruções, as flags e o laço de execução.
2. O **montador**, chamado `vm8asm`, traduz o código-fonte Assembly legível por humanos para a sequência de bytes compreendida pela CPU simulada.

A ideia central é:

```text
Código-fonte Assembly escrito por uma pessoa
                    |
                    v
           montador vm8asm
                    |
                    v
 Código de máquina contendo apenas bytes
                    |
                    v
     Simulador da CPU virtual de 8 bits
```

A CPU nunca lê palavras como `LDI`, `start` ou `memory_demo`. Essas palavras existem somente no arquivo-fonte e dentro do montador. O programa final apresentado à CPU contém apenas bytes numéricos.

## Limite atual da implementação

O simulador da CPU está funcional. Ele pode executar a demonstração embutida de 18 bytes armazenada em `src/program.c` ou um binário bruto compatível selecionado pela linha de comando. A demonstração Assembly atual gera um binário de 19 bytes porque inclui um byte de dado embutido depois de `HALT`. Qualquer uma dessas origens pode ser executada normalmente ou com um rastreamento de instruções legível por pessoas.

A CPU agora possui uma pilha descendente de 16 bytes. `PUSH A` e `POP A` transferem dados por meio dela, enquanto `CALL addr8` e `RET` a utilizam para salvar e restaurar endereços de retorno de 8 bits. Overflow e underflow da pilha são erros explícitos de execução compartilhados pelos dois tipos de operação.

A CPU também possui uma porta de entrada controlada pelo hospedeiro no endereço `0xEE` e um latch de saída em `0xEF`. Eles são acessados por instruções comuns `LDA` e `STA`, pois a CPU decodifica esses endereços como E/S mapeada em memória.

O simulador também oferece um monitor interativo de terminal. Ele pode executar o programa embutido ou um programa carregado de arquivo uma instrução por vez, inspecionar o estado da CPU e a memória mapeada, substituir o programa atual, gerenciar breakpoints no hospedeiro e ativar ou desativar o rastreamento de instruções sem reiniciar o processo.

O módulo compartilhado `instruction_set` é o responsável pelas definições de `Opcode` e por uma tabela de metadados somente para leitura que associa cada byte de opcode suportado ao seu mnemônico Assembly. A busca recebe um `uint8_t` bruto porque a memória pode conter qualquer byte; ela retorna um ponteiro para os metadados de um opcode reconhecido ou um ponteiro nulo para um valor desconhecido.

O montador atualmente implementa:

- validação dos argumentos da linha de comando;
- leitura limitada do arquivo-fonte;
- remoção de comentários e espaços ao redor da linha;
- entrega dos statements normalizados por meio de um callback;
- uma tabela de símbolos compartilhada por labels e constantes nomeadas;
- uma primeira passagem que registra labels e constantes e calcula o tamanho das instruções e dos dados brutos;
- interpretação de literais de 8 bits em decimal e hexadecimal;
- resolução de um operando como literal ou símbolo;
- um parser que separa nomes de instruções ou diretivas e operandos;
- um codificador que valida a semântica das instruções e emite opcodes e operandos;
- uma segunda passagem que ignora `.EQU`, resolve operandos de `.BYTE`, codifica instruções e acumula a sequência completa de bytes do programa;
- um gravador de binário que armazena os bytes brutos no arquivo de saída solicitado.

`make assemble` traduz `programs/demo.asm` para o arquivo de 19 bytes `build/demo.bin`. `make inspect` realiza essa montagem e depois exibe o tamanho e os bytes brutos gerados. `make run` executa o bytecode embutido de 18 bytes de `src/program.c`, enquanto `make run-bin` monta, carrega e executa `build/demo.bin`. As duas demonstrações são equivalentes no comportamento, mas não são idênticas byte por byte. `make trace` e `make trace-bin` selecionam essas mesmas duas origens de programa enquanto expõem cada instrução executada e o estado resultante da CPU.

## O que “8 bits” significa

Neste projeto, 8 bits descreve a largura natural dos dados da CPU:

- o registrador `A` armazena um valor de 8 bits;
- o registrador `B` armazena um valor de 8 bits;
- cada posição da memória armazena um valor de 8 bits;
- o contador de programa armazena um endereço de 8 bits;
- o ponteiro de pilha armazena um endereço de pilha de 8 bits ou o sentinela de pilha vazia;
- os resultados aritméticos mantidos pela CPU possuem 8 bits.

Um valor de 8 bits possui 256 combinações possíveis:

```text
Binário:     00000000 até 11111111
Hexadecimal: 0x00 até 0xFF
Decimal:     0 até 255
```

O contador de programa também possui 8 bits, portanto pode endereçar 256 posições de memória. Consequentemente, esta versão possui exatamente 256 bytes de memória, nos endereços de `0x00` até `0xFF`.

Oito bits **não** significa que toda instrução precisa ocupar um único byte. Uma instrução pode ser formada por vários valores consecutivos de 8 bits. A CPU busca esses valores um de cada vez.

Processadores reais de 8 bits também costumam possuir instruções e endereços maiores que 8 bits. “CPU de 8 bits” normalmente se refere principalmente à largura dos registradores e da unidade lógica e aritmética, e não a um limite universal para o tamanho das instruções.

## Estado da CPU

A estrutura `Cpu` contém todo o estado visível do processador virtual:

| Componente | Largura ou tipo | Finalidade |
| --- | --- | --- |
| Registrador `A` | 8 bits | Acumulador principal usado por instruções aritméticas e de transferência de memória. |
| Registrador `B` | 8 bits | Operando aritmético secundário. |
| Contador de programa (`PC`) | 8 bits | Endereço do próximo byte que será buscado. |
| Ponteiro de pilha (`SP`) | 8 bits | Endereço do byte mais novo da pilha, ou `0x00` quando a pilha está vazia. |
| Porta de entrada | 8 bits | Valor fornecido pelo hospedeiro e lido pelo software VM8 em `0xEE`. |
| Latch de saída | 8 bits | Valor lido ou gravado pelo software VM8 em `0xEF` e observado pelo hospedeiro. |
| Flag zero (`Z`) | Booleana | Indica que o resultado mais recente que atualiza flags foi zero. |
| Flag carry (`C`) | Booleana | Indica carry de saída na adição ou empréstimo na subtração. |
| Estado halted | Booleano | Impede novas execuções após uma parada, opcode inválido ou erro da pilha. |
| Contador de ciclos | 64 bits | Conta instruções tentadas no modelo simplificado de temporização. |
| Vetor de memória | 256 bytes | Sustenta código, dados comuns e pilha; a E/S mapeada é decodificada antes do acesso comum ao vetor. |

O estado inicial normal é completamente inicializado com zeros:

```c
Cpu cpu = {0};
```

O programa é então copiado para a memória começando no endereço `0x00`. Embora o espaço de endereços completo possua 256 posições, um programa carregado pode ocupar no máximo os primeiros 238 bytes. Os endereços `0xEE` e `0xEF` são reservados para E/S, e os 16 endereços finais são reservados para a pilha.

## Memória unificada de código e dados

O projeto usa um único espaço de endereços unificado de 8 bits. Instruções e dados comuns compartilham o vetor de memória, enquanto a CPU intercepta os dois endereços de E/S antes do acesso comum à memória. Os dados da pilha utilizam a parte final do mesmo vetor.

| Faixa de endereços | Tamanho | Uso |
| --- | ---: | --- |
| `0x00` até `0xED` | 238 bytes | Programa carregado e dados comuns selecionados pelo programa. |
| `0xEE` | 1 byte | Porta de entrada virtual somente para leitura. |
| `0xEF` | 1 byte | Latch de saída virtual para leitura e gravação. |
| `0xF0` até `0xFF` | 16 bytes | Pilha gerenciada pela CPU. |

Por exemplo, o programa de demonstração ocupa os endereços de `0x00` até `0x11`, enquanto usa o endereço `0x80` para armazenar dados. A instrução:

```asm
STA 0x80
```

grava o registrador `A` na posição de memória `0x80`.

Como código e dados compartilham o mesmo vetor, uma gravação direcionada a um endereço do programa poderia sobrescrever uma instrução. A demonstração atual coloca seus dados graváveis deliberadamente fora de seus bytes de instrução. O software ainda pode endereçar a região da pilha com instruções comuns de memória, mas fazer isso pode corromper o conteúdo da pilha. O montador e o carregador de programas garantem que o próprio binário carregado ocupe somente de `0x00` até `0xED`.

## Entrada e saída mapeadas em memória

E/S mapeada em memória significa que a CPU usa instruções comuns de memória para acessar periféricos. O decodificador de endereços decide se um endereço se refere ao vetor de memória ou a um registrador de dispositivo:

| Endereço | Comportamento de leitura | Comportamento de gravação |
| --- | --- | --- |
| `0xEE` | Retorna o valor atual de entrada fornecido pelo hospedeiro. | É ignorada, pois a porta é somente de entrada do ponto de vista do software VM8. |
| `0xEF` | Retorna o valor atual do latch de saída. | Substitui o valor do latch de saída pelo byte gravado. |
| Qualquer outro endereço | Lê o byte correspondente do vetor de memória. | Grava o byte correspondente no vetor de memória. |

Os valores de entrada e saída são campos do estado `Cpu`, e não bytes armazenados naqueles dois índices do vetor. O hospedeiro chama `cpu_set_input_port` antes da execução. O software VM8 lê a entrada por meio de `cpu_read_memory`, normalmente executando `LDA 0xEE`. O software altera a saída executando `STA 0xEF`, e o hospedeiro a observa por meio de `cpu_get_output_port`. Uma reinicialização completa da CPU zera os valores das duas portas.

Este programa copia a porta de entrada para o latch de saída:

```asm
.EQU INPUT_PORT, 0xEE
.EQU OUTPUT_PORT, 0xEF

start:
  LDA INPUT_PORT
  STA OUTPUT_PORT
  HALT
```

O montador resolve as constantes e emite cinco bytes:

```text
40 ee 41 ef 01
```

O mesmo programa em código de máquina pode ser criado diretamente para um experimento rápido:

```bash
make
printf '\x40\xEE\x41\xEF\x01' > build/io-demo.bin
./build/vm8 run build/io-demo.bin --input 0xA5
```

O estado final relevante é:

```text
Register A: 0xA5
Input port: 0xA5
Output port: 0xA5
Program counter: 5
Cycle count: 3
```

O mesmo valor pode ser escrito em decimal, e o rastreamento revela exatamente quando a saída muda:

```bash
./build/vm8 trace build/io-demo.bin --input 165
```

```text
Execution trace:
  ADDR=0x00 OP=0x40 MNEMONIC=LDA A=0xA5 B=0x00 SP=0x00 IN=0xA5 OUT=0x00 Z=0 C=0 NEXT=0x02 CYCLES=1 RESULT=ok
  ADDR=0x02 OP=0x41 MNEMONIC=STA A=0xA5 B=0x00 SP=0x00 IN=0xA5 OUT=0xA5 Z=0 C=0 NEXT=0x04 CYCLES=2 RESULT=ok
  ADDR=0x04 OP=0x01 MNEMONIC=HALT A=0xA5 B=0x00 SP=0x00 IN=0xA5 OUT=0xA5 Z=0 C=0 NEXT=0x05 CYCLES=3 RESULT=halted
```

A CLI aceita entrada decimal ou hexadecimal com prefixo `0x`, de 0 até 255. Sinais, espaços iniciais, texto restante, valores ausentes e valores maiores são rejeitados. Quando presente, `--input <byte>` precisa ser a opção final. Sem ela, a entrada assume `0x00`.

Os alvos do Make expõem a mesma opção por meio de `INPUT_VALUE`, por exemplo, `make run INPUT_VALUE=0xA5`. A demonstração embutida não lê `0xEE`, portanto seu latch de saída permanece zerado, embora o valor de entrada selecionado continue visível no estado final e no rastreamento.

## Pilha, `PUSH` e `POP`

Uma pilha é uma área de armazenamento de último a entrar, primeiro a sair. O byte inserido mais recentemente é o primeiro byte devolvido por uma retirada. A VM8 reserva os endereços de `0xF0` até `0xFF` para uma pilha que cresce para baixo, em direção a endereços menores.

`SP = 0x00` representa uma pilha vazia. Esse é um valor sentinela, e não um endereço atualmente ocupado por dados da pilha. Isso permite que a inicialização normal com zeros `Cpu cpu = {0};` crie uma pilha vazia válida sem exigir uma atribuição de inicialização separada.

`PUSH A` funciona assim:

1. Se a pilha estiver vazia, ajusta `SP` para `0xFF`.
2. Caso contrário, decrementa `SP` antes da gravação.
3. Armazena o registrador `A` em `memory[SP]`.

`POP A` realiza a operação inversa:

1. Lê `memory[SP]` para o registrador `A`.
2. Se o byte removido estava em `0xFF`, ajusta `SP` novamente para o sentinela vazio `0x00`.
3. Caso contrário, incrementa `SP` em direção a `0xFF`.

Por exemplo:

| Operação | `SP` resultante | Memória relevante ou resultado |
| --- | ---: | --- |
| Estado inicial vazio | `0x00` | Nenhum byte da pilha está ativo. |
| Insere `0xA5` | `0xFF` | `memory[0xFF] = 0xA5`. |
| Insere `0x5A` | `0xFE` | `memory[0xFE] = 0x5A`; `0xA5` permanece abaixo dele em `0xFF`. |
| Retira | `0xFF` | `A = 0x5A`. |
| Retira novamente | `0x00` | `A = 0xA5`; a pilha está vazia novamente. |

Depois de 16 inserções, todos os endereços de `0xFF` até `0xF0` estão ocupados e `SP` é igual a `0xF0`. Outro `PUSH A` informa overflow da pilha. Um `POP A` enquanto `SP` é `0x00` informa underflow da pilha. Qualquer um desses erros interrompe a execução e produz resultados distintos de passo e de execução sem alterar registradores, flags, memória da pilha ou `SP`; a busca do opcode tentado e a contagem do ciclo já ocorreram.

`PUSH A` preserva ambos os registradores e ambas as flags. `POP A` substitui `A`, atualiza `Z` conforme o byte retirado seja ou não zero, preserva `B` e `C` e não apaga o byte deixado na memória. É o avanço de `SP` que faz essa posição deixar de pertencer à pilha ativa.

## Sub-rotinas, `CALL` e `RET`

Uma sub-rotina é um bloco reutilizável de instruções que recebe temporariamente o controle e depois retorna para quem a chamou. Um salto comum modifica `PC`, mas não memoriza de onde veio a execução. `CALL addr8` realiza as duas tarefas: salva o endereço de retorno na pilha e depois salta para a sub-rotina. `RET` retira esse endereço da pilha e o coloca novamente em `PC`.

`CALL addr8` ocupa dois bytes: o opcode `0x52`, seguido por um byte de endereço absoluto. Sua ordem de execução é importante:

1. A busca normal da instrução lê o opcode de `CALL` e avança `PC`.
2. Uma segunda busca lê `addr8` e avança `PC` novamente.
3. O `PC` já avançado, que agora identifica a instrução posterior a `CALL`, é inserido na pilha como endereço de retorno.
4. O endereço de destino substitui `PC`, transferindo a execução para a sub-rotina.

Portanto, o endereço de retorno é calculado durante a execução; ele não é outro byte codificado no programa. `RET` ocupa um byte. Depois que seu opcode é buscado, ela retira o byte mais novo da pilha diretamente para `PC`. As duas instruções preservam os registradores `A` e `B` e as flags `Z` e `C`.

Considere este programa com chamadas aninhadas, cujos labels estão anotados com seus endereços de byte resultantes:

```asm
start:           ; endereço 0x00
  CALL first     ; bytes: 52 03
  HALT           ; endereço 0x02

first:           ; endereço 0x03
  CALL second    ; bytes: 52 06
  RET            ; endereço 0x05

second:          ; endereço 0x06
  RET
```

O montador resolve os labels e emite somente estes sete bytes:

```text
52 03 01 52 06 53 53
```

A sequência de execução é:

1. `CALL first` insere `0x02` em `memory[0xFF]`, deixa `SP = 0xFF` e ajusta `PC = 0x03`.
2. `CALL second` insere `0x05` em `memory[0xFE]`, deixa `SP = 0xFE` e ajusta `PC = 0x06`.
3. A instrução `RET` em `0x06` retira `0x05`, deixa `SP = 0xFF` e retoma a primeira sub-rotina em `0x05`.
4. A instrução `RET` em `0x05` retira `0x02`, restaura o sentinela vazio `SP = 0x00` e retoma o chamador em `0x02`.
5. `HALT` é executada em `0x02`. Cinco instruções foram tentadas, portanto a contagem simplificada de ciclos é cinco.

O último endereço de retorno inserido é o primeiro recuperado, exatamente o comportamento de último a entrar, primeiro a sair necessário para chamadas aninhadas.

Endereços de retorno e valores inseridos por `PUSH A` compartilham a mesma capacidade de 16 bytes. Dezesseis chamadas podem ser aninhadas somente quando não há outras entradas ativas na pilha. Cada `PUSH A` ativo consome uma posição que estaria disponível para uma chamada. O software também precisa manter as operações equilibradas: um `POP A` inesperado, um `PUSH A` adicional ou uma instrução `RET` no lugar errado pode consumir o byte incorreto, pois a pilha armazena bytes sem registrar se cada um representa dados ou um endereço de retorno.

Uma instrução `CALL` executada com a pilha cheia informa overflow depois que seu opcode e seu operando de destino já foram buscados, portanto `PC` já contém o endereço que seria utilizado para retorno; o salto não acontece. Uma instrução `RET` executada com a pilha vazia informa underflow depois da busca de seu opcode. Qualquer um desses erros interrompe a execução, contabiliza a instrução tentada e preserva registradores, flags, memória da pilha e `SP`.

## Busca, decodificação e execução

Cada passo da CPU segue três etapas conceituais:

1. **Busca:** lê o byte no endereço contido em `PC` e depois incrementa `PC`.
2. **Decodificação:** interpreta o byte buscado como um opcode.
3. **Execução:** realiza a operação e, quando necessário, busca mais um byte de operando.

A operação de busca é conceitualmente:

```text
endereço = PC
valor    = memória[endereço]
PC       = PC + 1
```

O opcode determina se outro byte precisa ser buscado. Uma instrução de um byte termina após o opcode. Uma instrução de dois bytes busca seu operando no endereço seguinte.

### Exemplo de um byte

```asm
ADD A, B
```

é codificada como:

```text
0x20
```

O próprio opcode identifica os dois registradores, portanto nenhum byte adicional de operando é necessário.

### Exemplo de dois bytes

```asm
LDI A, 0x2A
```

é codificada como dois bytes consecutivos:

| Endereço | Byte | Significado |
| ---: | ---: | --- |
| `0x00` | `0x10` | Opcode de `LDI A`. |
| `0x01` | `0x2A` | Valor imediato carregado em `A`. |

A CPU não precisa de um registrador de 16 bits para executar essa instrução. Primeiro ela busca `0x10`, reconhece que o opcode exige um operando e então busca `0x2A`. Cada busca transfere um valor de 8 bits.

## Conjunto de instruções

| Sintaxe Assembly | Bytes codificados | Tamanho | Efeito |
| --- | --- | ---: | --- |
| `NOP` | `00` | 1 | Não altera o estado além do avanço normal de `PC` e do ciclo. |
| `HALT` | `01` | 1 | Ativa o estado halted. |
| `LDI A, imm8` | `10 imm8` | 2 | Carrega um byte imediato em `A`; atualiza `Z`; preserva `C`. |
| `LDI B, imm8` | `11 imm8` | 2 | Carrega um byte imediato em `B`; atualiza `Z`; preserva `C`. |
| `ADD A, B` | `20` | 1 | Armazena em `A` os oito bits inferiores de `A + B`; atualiza `Z` e o carry da adição. |
| `SUB A, B` | `21` | 1 | Armazena em `A` o resultado de `A - B` com retorno circular; atualiza `Z` e o empréstimo da subtração. |
| `AND A, B` | `22` | 1 | Armazena em `A` o AND bit a bit; atualiza `Z`; limpa `C`. |
| `OR A, B` | `23` | 1 | Armazena em `A` o OR bit a bit; atualiza `Z`; limpa `C`. |
| `XOR A, B` | `24` | 1 | Armazena em `A` o OR exclusivo bit a bit; atualiza `Z`; limpa `C`. |
| `NOT A` | `25` | 1 | Inverte todos os bits de `A`; atualiza `Z`; limpa `C`. |
| `SHL A` | `26` | 1 | Desloca `A` para a esquerda preenchendo com zero; atualiza `Z`; armazena o bit 7 original em `C`. |
| `SHR A` | `27` | 1 | Desloca `A` para a direita preenchendo com zero; atualiza `Z`; armazena o bit 0 original em `C`. |
| `CMP A, B` | `28` | 1 | Compara `A` e `B` sem sinal; atualiza `Z` e o empréstimo; preserva ambos os registradores. |
| `JZ addr8` | `30 addr8` | 2 | Carrega `PC` com o endereço absoluto quando `Z` está ativa. |
| `JNZ addr8` | `32 addr8` | 2 | Carrega `PC` com o endereço absoluto quando `Z` está inativa. |
| `JC addr8` | `33 addr8` | 2 | Carrega `PC` com o endereço absoluto quando `C` está ativa. |
| `JMP addr8` | `31 addr8` | 2 | Sempre carrega `PC` com o endereço absoluto. |
| `LDA addr8` | `40 addr8` | 2 | Carrega `A` a partir do endereço de memória; atualiza `Z`; preserva `C`. |
| `STA addr8` | `41 addr8` | 2 | Armazena `A` no endereço de memória; preserva registradores e flags. |
| `PUSH A` | `50` | 1 | Insere `A` na pilha; preserva registradores e flags. |
| `POP A` | `51` | 1 | Retira o byte mais novo da pilha para `A`; atualiza `Z`; preserva `C`. |
| `CALL addr8` | `52 addr8` | 2 | Insere o endereço posterior ao operando e depois carrega `PC` com o destino absoluto; preserva registradores e flags. |
| `RET` | `53` | 1 | Retira o byte mais novo da pilha diretamente para `PC`; preserva registradores e flags. |

`imm8` e `addr8` possuem um byte cada. Portanto, podem representar valores de `0x00` até `0xFF`.
`PUSH A`, `POP A` e `RET` não precisam de um byte de operando codificado porque seus opcodes identificam suas operações completas. `CALL` exige um operando `addr8`, que pode ser escrito como literal, constante ou label.

## Operações lógicas e de bits

`AND`, `OR` e `XOR` comparam os bits correspondentes de `A` e `B`. O resultado substitui `A`, enquanto `B` permanece inalterado. `NOT` opera somente sobre `A` e troca cada zero por um e cada um por zero. Essas quatro instruções limpam `C` e ativam `Z` quando o resultado armazenado em `A` é zero.

Por exemplo, com `A = 0xCA` e `B = 0xAC`:

```text
AND: 11001010 & 10101100 = 10001000 = 0x88
OR:  11001010 | 10101100 = 11101110 = 0xEE
XOR: 11001010 ^ 10101100 = 01100110 = 0x66
```

Para uma inversão de 8 bits:

```text
NOT: ~00001111 = 11110000
     ~0x0F      = 0xF0
```

`SHL` e `SHR` movem todos os bits de `A` uma posição e inserem zero na nova posição aberta. O bit que sai do registrador não é perdido silenciosamente: ele é copiado para `C`.

```text
SHL: A = 10000001 -> A = 00000010, C = 1
SHR: A = 10000001 -> A = 01000000, C = 1
```

O exemplo Assembly equivalente para o deslocamento à esquerda é:

```asm
LDI A, 0x81
SHL A
```

Após `SHL A`, o registrador `A` contém `0x02`, carry está ativa porque o bit 7 original era um e zero está inativa porque o resultado não é zero. A notação binária acima é apenas explicativa; o montador atual aceita literais de byte decimais e hexadecimais, mas não literais binários com `0b`.

## Flags

### Flag zero

A flag zero é ativada quando uma instrução que atualiza flags produz zero. Atualmente ela é atualizada por:

- `LDI A, imm8`;
- `LDI B, imm8`;
- `ADD A, B`;
- `SUB A, B`;
- `AND A, B`;
- `OR A, B`;
- `XOR A, B`;
- `NOT A`;
- `SHL A`;
- `SHR A`;
- `CMP A, B`;
- `LDA addr8`;
- `POP A`.

`JZ` e `JNZ` leem a flag zero, mas não a modificam.

### Flag carry

A flag carry registra informações que não cabem no resultado de 8 bits:

- após `ADD`, indica carry de saída além de `0xFF`;
- após `SUB`, indica que foi necessário um empréstimo porque o valor original de `A` era menor que `B`;
- após `CMP`, indica que o valor sem sinal em `A` é menor que o valor em `B`;
- após `SHL`, recebe o bit 7 original;
- após `SHR`, recebe o bit 0 original;
- após `AND`, `OR`, `XOR` ou `NOT`, é limpa.

Por exemplo:

```text
0xFF + 0x01 = 0x100
A armazenado  = 0x00
zero          = ativa
carry         = ativa
```

Na subtração:

```text
0x00 - 0x01  = 0xFF após o retorno circular de 8 bits
zero          = inativa
carry/borrow  = ativa
```

`JC` lê a flag carry, mas não a modifica. `PUSH A` preserva ambas as flags, enquanto `POP A` atualiza zero e preserva carry.

## Comparação e saltos condicionais

`CMP A, B` se comporta como uma subtração sem sinal usada somente para tomar decisões. Internamente, a CPU calcula o valor de 8 bits com retorno circular de `A - B` e o empréstimo correspondente, utiliza-os para atualizar `Z` e `C` e então descarta o resultado da subtração. Os registradores `A` e `B` permanecem inalterados.

| Relação | `Z` após `CMP` | `C` após `CMP` | Significado |
| --- | ---: | ---: | --- |
| `A == B` | 1 | 0 | Os valores são iguais. |
| `A > B` | 0 | 0 | `A` sem sinal é maior que `B`. |
| `A < B` | 0 | 1 | A subtração exigiria um empréstimo. |

Isso torna os saltos condicionais imediatamente úteis depois de uma comparação:

- `JZ` salta quando os valores comparados são iguais;
- `JNZ` salta quando os valores comparados são diferentes;
- `JC` salta quando `A` sem sinal é menor que `B`.

Por exemplo, a igualdade pode ser testada sem destruir nenhum dos operandos:

```asm
LDI A, 0x2A
LDI B, 0x2A
CMP A, B
JZ equal_values
```

`JNZ` também pode repetir um laço enquanto um resultado aritmético permanecer diferente de zero:

```asm
LDI A, 3
LDI B, 1

loop:
  SUB A, B
  JNZ loop

HALT
```

O laço executa `SUB` três vezes. Os dois primeiros resultados mantêm `Z` inativa, portanto `JNZ` retorna para `loop`. O terceiro resultado é zero, então a execução continua até `HALT`.

Uma decisão de menor que sem sinal utiliza o empréstimo registrado em `C`:

```asm
LDI A, 0x10
LDI B, 0x20
CMP A, B
JC a_is_lower
```

Todo salto condicional ocupa dois bytes e sempre busca seu operando de endereço. Se a condição for verdadeira, o destino substitui `PC`; caso contrário, `PC` já aponta para a instrução seguinte. O próprio salto preserva ambos os registradores e ambas as flags. Como várias instruções podem atualizar carry, o significado de `JC` depende da instrução mais recente que produziu flags; sua interpretação de menor que ocorre especificamente após `CMP`.

## Contador de programa e contador de ciclos

O contador de programa conta **endereços de bytes**, e não instruções. Portanto:

- uma instrução de um byte normalmente avança `PC` em um;
- uma instrução de dois bytes normalmente avança `PC` em dois;
- um salto realizado substitui `PC` pelo endereço de destino depois que o byte de destino foi buscado.

O contador de ciclos segue um modelo deliberadamente simplificado: toda instrução tentada conta como um ciclo, independentemente de a instrução ocupar um ou dois bytes.

Portanto, o tamanho do programa e a quantidade de instruções executadas são medidas diferentes. Tanto a demonstração embutida de 18 bytes quanto a demonstração montada de 19 bytes executam somente nove instruções e terminam com:

```text
Program counter: 18
Cycle count: 9
```

Isso é uma abstração educacional. Um processador físico pode precisar de quantidades diferentes de ciclos de clock para diferentes instruções e acessos à memória.

## Código-fonte Assembly, labels e símbolos

O código-fonte Assembly é um texto processado pelo `vm8asm`, que é executado como um programa comum no computador hospedeiro. A CPU virtual não interpreta esse texto.

Uma declaração de label associa um nome ao endereço de saída atual:

```asm
start:
```

Os dois-pontos declaram o label, mas não fazem parte do nome armazenado. Uma instrução posterior pode referenciá-lo:

```asm
JMP start
```

Um **símbolo** é um nome associado a um valor de 8 bits conhecido pelo montador. Na linguagem atual, um símbolo pode ser o endereço de um label ou uma constante nomeada. Os dois tipos compartilham uma tabela de símbolos e um mesmo espaço de nomes. A tabela criada para a demonstração atual inclui entradas como:

```text
start            -> 0x00  (endereço de label)
memory_demo      -> 0x09  (endereço de label)
initial_data     -> 0x12  (endereço de label)
COMPARISON_VALUE -> 0x2A  (constante nomeada)
DATA_ADDRESS     -> 0x80  (constante nomeada)
```

Os nomes podem conter vários caracteres porque são armazenados e processados pelo montador executado no computador hospedeiro. Quando um símbolo é usado como operando, somente seu valor resolvido de 8 bits é gravado no código de máquina. O nome em si nunca entra na memória da CPU.

Labels e constantes diferenciam letras maiúsculas e minúsculas, devem começar com uma letra ou sublinhado e podem continuar com letras, números ou sublinhados. Mnemônicos de instruções e nomes de registradores são reservados e não podem ser usados como símbolos. Um label e uma constante também não podem reutilizar o mesmo nome porque compartilham um único espaço de nomes.

### Constantes nomeadas e dados brutos

`.EQU NAME, value` associa `NAME` a um literal decimal ou hexadecimal com prefixo `0x` entre 0 e 255. Ela não emite nenhum byte:

```asm
.EQU STORED_VALUE, 0x5A
```

`.BYTE value` emite exatamente um byte bruto. Seu operando pode ser um literal, uma constante nomeada ou um label. Isso permite que dados ocupem a mesma imagem de memória que as instruções executáveis:

```asm
.EQU STORED_VALUE, 0x5A

  LDA initial_data
  HALT

initial_data:
  .BYTE STORED_VALUE
```

Esse exemplo gera `40 03 01 5A`. O operando de `LDA` é `0x03`, o endereço registrado para `initial_data`; o byte nesse endereço é `0x5A`. Nenhum dos dois nomes simbólicos é armazenado no binário.

## Montagem no hospedeiro e máquinas com pouca memória

O `vm8asm` é executado no Ubuntu hospedeiro, portanto seu código executável, seus buffers de texto-fonte e sua tabela de símbolos não consomem nenhum dos 256 bytes de memória da CPU VM8. Somente os bytes de código de máquina gerados são copiados para a CPU virtual.

Escrever esses bytes manualmente em hexadecimal não tornaria o programa final menor. Por exemplo, inserir manualmente `10 2A` produz os mesmos dois bytes que montar `LDI A, 0x2A`; hexadecimal é apenas uma notação legível para os padrões de bits.

Em uma máquina histórica isolada, sem outro computador para o desenvolvimento, os programadores tinham várias opções:

- traduzir as instruções manualmente e inserir os valores de código de máquina por interruptores, cartões ou fita;
- inserir um carregador bootstrap muito pequeno que pudesse carregar uma ferramenta maior;
- carregar temporariamente um montador na mesma memória que seria posteriormente usada pelo programa;
- manter um monitor ou montador pequeno permanentemente em uma região separada de ROM.

Quando um montador ocupava a própria memória limitada da máquina, o código-fonte e a saída podiam ser transmitidos por mídias externas. Depois da montagem, o montador podia ser sobrescrito e sua memória reutilizada pelo programa gerado. Um montador com várias passagens trocava passagens adicionais sobre a entrada e tempo por um conjunto de trabalho menor na memória.

## Por que o montador usa duas passagens

Um salto pode referenciar um label que aparece mais adiante no código-fonte:

```asm
  JZ finished
  LDI A, 0xFF

finished:
  HALT
```

Quando o montador encontra `JZ finished` pela primeira vez, o endereço final de `finished` ainda não é conhecido. Duas passagens resolvem isso de maneira simples.

### Leitura e normalização do código-fonte

Antes de qualquer passagem, o leitor do código-fonte:

1. lê uma linha física por vez;
2. rejeita uma linha com mais de 255 caracteres;
3. remove o texto iniciado por `;`, pois ele é um comentário;
4. remove os espaços ao redor do conteúdo;
5. ignora resultados vazios;
6. envia cada statement restante, seu número físico de linha e o caminho do arquivo para um callback.

Por exemplo:

```asm
    LDI A, 0x2A   ; Load the initial value.
```

torna-se:

```text
LDI A, 0x2A
```

O número original da linha física é preservado para as mensagens de diagnóstico.

### Primeira passagem

A primeira passagem mantém o tamanho atual do programa, que também funciona como o endereço do próximo byte que seria emitido.

- Um label é inserido na tabela de símbolos com o endereço atual e emite zero bytes.
- `.EQU` insere seu nome e valor literal na mesma tabela de símbolos e emite zero bytes.
- `.BYTE` avança o tamanho em um porque emitirá um byte durante a segunda passagem.
- Uma instrução de um byte avança o tamanho em um.
- Uma instrução de dois bytes avança o tamanho em dois.
- Nomes de símbolos duplicados são rejeitados mesmo quando uma declaração é um label e a outra é uma constante.
- Nomes malformados ou reservados, diretivas inválidas, mnemônicos desconhecidos e programas maiores que a memória são rejeitados.

O literal de `.EQU` precisa ser validado durante essa passagem porque seu valor é armazenado imediatamente. Os operandos simbólicos das instruções e de `.BYTE` podem esperar pela segunda passagem; a primeira passagem precisa apenas dos respectivos tamanhos de saída para continuar calculando os endereços.

### Resolução de literais e símbolos

O interpretador de literais aceita valores estritos em decimal ou hexadecimal com prefixo `0x`:

```text
42   -> 0x2A
0x2A -> 0x2A
255  -> 0xFF
```

Sinais, espaços internos, dígitos malformados e valores acima de 255 são rejeitados.

O resolvedor de operandos de byte primeiro tenta interpretar o texto como literal. Se o texto não for um literal, mas for um nome de símbolo válido, ele consulta a tabela de símbolos compartilhada. Portanto, o mesmo resolvedor pode retornar um literal, o valor de uma constante ou o endereço de um label:

```text
0x80             -> byte literal 0x80
DATA_ADDRESS     -> valor de constante 0x80
memory_demo      -> endereço de label 0x09
initial_data     -> endereço de label 0x12
missing          -> erro de símbolo indefinido
```

### Segunda passagem

O parser separa cada statement normalizado em um nome inicial e até dois operandos. Ele valida a estrutura sintática, como espaços, vírgulas, operandos ausentes e operandos em excesso, mas não decide se uma instrução, diretiva ou registrador é suportado.

O codificador de instruções então valida o significado dos campos interpretados. Ele reconhece o conjunto atual de instruções, verifica a quantidade de operandos e a ordem dos registradores, resolve literais de byte ou símbolos e emite uma instrução codificada de um ou dois bytes. `AND`, `OR`, `XOR` e `CMP` exigem o par de registradores exato `A, B`; `NOT`, `SHL` e `SHR` exigem somente o registrador `A`; e `PUSH` e `POP` exigem `A`. `JZ`, `JNZ`, `JC`, `JMP`, `LDA`, `STA` e `CALL` aceitam um endereço de byte literal ou simbólico. `NOP`, `HALT` e `RET` não aceitam operandos. Uma falha de codificação deixa inalterado o objeto de saída fornecido pelo chamador.

A segunda passagem lê novamente os statements normalizados e trata cada tipo explicitamente:

- declarações de labels são ignoradas porque seus endereços já estão na tabela de símbolos;
- `.EQU` é ignorada porque já definiu um valor e não emite bytes;
- `.BYTE` resolve seu único operando e acrescenta exatamente um byte;
- instruções comuns passam pela validação semântica e pela codificação.

Cada byte emitido é acrescentado a um buffer limitado do programa. A passagem informa diagnósticos com o caminho original do arquivo e o número da linha e rejeita qualquer gravação que ultrapassaria a capacidade de saída fornecida. Somente instruções da CPU codificadas aumentam a contagem de instruções, portanto `.BYTE` muda a quantidade de bytes, mas não a quantidade de instruções.

Por exemplo:

```asm
JZ memory_demo
```

torna-se:

```text
30 09
```

Depois que as duas passagens concordam sobre o tamanho de 19 bytes do programa, o gravador de binário abre o caminho solicitado em modo binário, grava exatamente essa quantidade de bytes e verifica tanto a gravação quanto o fechamento final do arquivo. Portanto, `make assemble` cria `build/demo.bin` como código de máquina bruto.

## Codificação completa da demonstração

A demonstração Assembly atual é:

```asm
; Demonstrates constants, embedded data, arithmetic, branching, and memory transfer.

.EQU COMPARISON_VALUE, 0x2A
.EQU FALLTHROUGH_VALUE, 0xFF
.EQU STORED_VALUE, 0x5A
.EQU CLEARED_VALUE, 0x00
.EQU DATA_ADDRESS, 0x80

start:
  LDI A, COMPARISON_VALUE
  LDI B, COMPARISON_VALUE
  SUB A, B
  JZ memory_demo
  LDI A, FALLTHROUGH_VALUE

memory_demo:
  LDA initial_data
  STA DATA_ADDRESS
  LDI A, CLEARED_VALUE
  LDA DATA_ADDRESS
  HALT

initial_data:
  .BYTE STORED_VALUE
```

O cálculo dos endereços e a codificação gerada são:

| Endereço | Statement do código-fonte | Bytes emitidos | Explicação |
| ---: | --- | --- | --- |
| — | `.EQU COMPARISON_VALUE, 0x2A` | — | Define uma constante; não emite nada. |
| — | `.EQU FALLTHROUGH_VALUE, 0xFF` | — | Define uma constante; não emite nada. |
| — | `.EQU STORED_VALUE, 0x5A` | — | Define uma constante; não emite nada. |
| — | `.EQU CLEARED_VALUE, 0x00` | — | Define uma constante; não emite nada. |
| — | `.EQU DATA_ADDRESS, 0x80` | — | Define uma constante; não emite nada. |
| `0x00` | `start:` | — | Registra `start = 0x00`; não emite nada. |
| `0x00` | `LDI A, COMPARISON_VALUE` | `10 2A` | Resolve a constante e ocupa `0x00` e `0x01`. |
| `0x02` | `LDI B, COMPARISON_VALUE` | `11 2A` | Resolve a mesma constante e ocupa `0x02` e `0x03`. |
| `0x04` | `SUB A, B` | `21` | Instrução de um byte. |
| `0x05` | `JZ memory_demo` | `30 09` | Resolve `memory_demo` como `0x09`. |
| `0x07` | `LDI A, FALLTHROUGH_VALUE` | `10 FF` | Resolve a constante; ignorada quando o salto é realizado. |
| `0x09` | `memory_demo:` | — | Registra `memory_demo = 0x09`; não emite nada. |
| `0x09` | `LDA initial_data` | `40 12` | Resolve o label como `0x12` e carrega seu byte. |
| `0x0B` | `STA DATA_ADDRESS` | `41 80` | Resolve a constante e armazena `A` em `0x80`. |
| `0x0D` | `LDI A, CLEARED_VALUE` | `10 00` | Resolve a constante e limpa `A`. |
| `0x0F` | `LDA DATA_ADDRESS` | `40 80` | Resolve a constante e recarrega o valor armazenado. |
| `0x11` | `HALT` | `01` | Para após buscar o byte. |
| `0x12` | `initial_data:` | — | Registra o endereço do dado embutido; não emite nada. |
| `0x12` | `.BYTE STORED_VALUE` | `5A` | Resolve a constante e emite um byte de dado. |

A sequência completa gerada de 19 bytes é:

```text
10 2A 11 2A 21 30 09 10 FF 40 12 41 80 10 00 40 80 01 5A
```

Os primeiros 18 bytes contêm código executável até `HALT`. O último byte, no endereço `0x12`, é um dado. Ele faz parte da imagem carregada na memória, mas é lido por `LDA`, e não buscado como opcode.

## Passo a passo da execução da demonstração

A CPU começa com registradores zerados, flags inativas, `PC = 0x00` e contador de ciclos igual a zero.

| Passo | Endereço | Instrução | Estado importante após a execução |
| ---: | ---: | --- | --- |
| 1 | `0x00` | `LDI A, 0x2A` | `A = 0x2A`, `Z = 0`, `PC = 0x02`, ciclos = 1. |
| 2 | `0x02` | `LDI B, 0x2A` | `B = 0x2A`, `Z = 0`, `PC = 0x04`, ciclos = 2. |
| 3 | `0x04` | `SUB A, B` | `A = 0x00`, `Z = 1`, sem empréstimo, `PC = 0x05`, ciclos = 3. |
| 4 | `0x05` | `JZ 0x09` | O operando de destino é buscado e a flag zero ativa muda `PC` para `0x09`; ciclos = 4. |
| 5 | `0x09` | `LDA 0x12` | Lê o byte embutido, portanto `A = 0x5A`, `Z = 0`, `PC = 0x0B`, ciclos = 5. |
| 6 | `0x0B` | `STA 0x80` | `memory[0x80] = 0x5A`, `PC = 0x0D`, ciclos = 6. |
| 7 | `0x0D` | `LDI A, 0x00` | `A = 0x00`, `Z = 1`, `PC = 0x0F`, ciclos = 7. |
| 8 | `0x0F` | `LDA 0x80` | `A = 0x5A`, `Z = 0`, `PC = 0x11`, ciclos = 8. |
| 9 | `0x11` | `HALT` | CPU parada, `PC = 0x12` (decimal 18), ciclos = 9. |

A instrução no endereço `0x07` nunca é executada porque `SUB A, B` produziu zero e `JZ` saltou diretamente para `0x09`.

O byte de dado em `0x12` nunca é executado porque `HALT` para a CPU depois de avançar `PC` de `0x11` para `0x12`. Mesmo assim, seu endereço é válido para a instrução anterior `LDA 0x12`.

O estado final visível é:

```text
Execution result: halted
Register A: 0x5A
Register B: 0x2A
Stack pointer: 0x00
Input port: 0x00
Output port: 0x00
Zero flag: clear
Carry flag: clear
Program counter: 18
Cycle count: 9
```

## Carregamento e execução limitada

`cpu_load_program` verifica se a sequência de bytes cabe na região de programa de 238 bytes e rejeita um ponteiro nulo para um programa não vazio. Ele copia o programa a partir do endereço zero, mas não reinicializa a CPU automaticamente nem grava nas regiões reservadas para E/S e pilha.

`cpu_run` recebe um limite de instruções. Isso impede que um laço incondicional como `JMP 0x00` continue para sempre sem devolver o controle ao chamador. A execução informa um de cinco resultados:

- parada normal;
- opcode inválido;
- limite de instruções atingido;
- overflow da pilha;
- underflow da pilha.

Um opcode inválido ou erro da pilha também para a CPU para que a execução não continue silenciosamente depois de uma transição de estado inválida.

## Observador de instruções e rastreamento da execução

`cpu_run_with_observer` separa a execução das instruções de qualquer recurso que observe essa execução. Ela recebe a mesma CPU e o mesmo limite de instruções de `cpu_run`, além de dois valores opcionais:

- um callback do tipo `CpuStepObserver`;
- um ponteiro de contexto opaco, repassado sem alteração ao callback.

Antes de chamar `cpu_step`, o laço de execução registra o valor atual do contador de programa e lê o opcode armazenado nesse endereço. Depois que `cpu_step` termina, o observador recebe o endereço original da instrução, o opcode, o estado atualizado da CPU, o resultado do passo e o ponteiro de contexto. O momento dessa chamada é importante: `ADDR` e `OP` identificam a instrução que acabou de ser executada, enquanto os registradores, as flags, `NEXT` e `CYCLES` descrevem o estado depois de sua execução.

A função original `cpu_run` permanece como um invólucro de conveniência. Ela chama `cpu_run_with_observer` com ponteiros nulos para o observador e o contexto, de modo que os chamadores existentes preservam o mesmo comportamento sem produzir uma saída de rastreamento.

O módulo de rastreamento fornece um observador que interpreta seu contexto como `FILE *` e grava uma linha por instrução tentada. Por exemplo, a primeira instrução da demonstração produz:

```text
Execution trace:
  ADDR=0x00 OP=0x10 MNEMONIC=LDI A=0x2A B=0x00 SP=0x00 IN=0x00 OUT=0x00 Z=0 C=0 NEXT=0x02 CYCLES=1 RESULT=ok
```

Os campos significam:

- `ADDR`: endereço do qual o opcode foi buscado;
- `OP`: byte bruto do opcode;
- `MNEMONIC`: nome da operação obtido dos metadados compartilhados do conjunto de instruções, ou `UNKNOWN` quando nenhum opcode corresponde;
- `A` e `B`: valores dos registradores depois da execução;
- `SP`: ponteiro de pilha depois da execução;
- `IN`: valor da porta de entrada mapeada em memória depois da execução;
- `OUT`: valor do latch de saída mapeado em memória depois da execução;
- `Z` e `C`: flags zero e carry depois da execução;
- `NEXT`: contador de programa depois da execução, incluindo qualquer salto realizado;
- `CYCLES`: total de instruções tentadas após esse passo;
- `RESULT`: `ok`, `halted`, `invalid-opcode`, `stack-overflow` ou `stack-underflow`.

A instrução final da demonstração, portanto, é exibida assim:

```text
  ADDR=0x11 OP=0x01 MNEMONIC=HALT A=0x5A B=0x2A SP=0x00 IN=0x00 OUT=0x00 Z=0 C=0 NEXT=0x12 CYCLES=9 RESULT=halted
```

Esse projeto de observador mantém a CPU independente da apresentação. Um depurador, registrador ou interface gráfica futura poderá fornecer outro callback sem inserir código de saída para terminal dentro de `cpu.c`.

## Leitura e execução de um binário externo

O leitor binário do simulador abre o arquivo selecionado em modo binário e lê seu conteúdo para um buffer fornecido pelo chamador. `main.c` fornece um buffer cuja capacidade é exatamente `CPU_PROGRAM_MEMORY_SIZE`, portanto o leitor não consegue gravar além da região de programa de 238 bytes da máquina virtual.

Depois de preencher o buffer, o leitor tenta buscar mais um byte. Essa leitura adicional diferencia dois casos que, sem ela, produziriam igualmente um buffer cheio:

- se a leitura adicional alcançar o fim do arquivo, o programa possui exatamente 238 bytes e é válido;
- se existir outro byte, o arquivo é grande demais e será rejeitado.

O leitor informa a quantidade de bytes realmente lida somente depois que tanto a leitura quanto o fechamento do arquivo terminam corretamente. Um arquivo vazio é um arquivo binário válido do ponto de vista estrito de entrada e saída do leitor, mas `main.c` o rejeita como programa executável. Essa separação mantém o transporte do arquivo separado da política do simulador.

A CLI oferece duas origens de programa e um rastreamento opcional para qualquer uma delas:

```text
make run
  -> vetor de bytes embutido de src/program.c
  -> a entrada assume 0x00 ou vem de INPUT_VALUE

make trace
  -> vetor de bytes embutido de src/program.c
  -> a entrada assume 0x00 ou vem de INPUT_VALUE
  -> rastreamento da execução ativado

make run-bin
  -> programs/demo.asm
  -> vm8asm
  -> build/demo.bin
  -> binary_reader_read
  -> cpu_load_program
  -> entrada do hospedeiro aplicada à CPU
  -> cpu_run_with_observer

make trace-bin
  -> mesmo caminho do binário montado
  -> rastreamento da execução ativado
```

As formas diretas `./build/vm8 run <program.bin>` e `./build/vm8 trace <program.bin>` utilizam o mesmo caminho de binário externo sem executar primeiro o montador. As duas aceitam uma opção final `--input <byte>`. O simulador não sabe se esse arquivo veio de `vm8asm`, de outra ferramenta ou da inserção manual de bytes; ele enxerga somente os bytes. A diferença é se `main.c` fornece um observador de rastreamento ao laço de execução da CPU.

O teste de processo em Bash exercita essa interface pública em vez de chamar diretamente as funções C. Ele verifica a execução normal de um binário externo, o rastreamento da demonstração embutida, o rastreamento do binário externo, a transferência da entrada para a saída, uma sessão interativa com breakpoint e execução passo a passo, as duas formas numéricas da entrada e cinco falhas esperadas: valor de entrada inválido, arquivo inexistente, arquivo vazio, arquivo de 239 bytes e arquivo contendo o opcode inválido `0xFF`. Cada falha precisa retornar um status de processo diferente de zero e colocar o diagnóstico esperado em `stderr`; a execução bem-sucedida precisa colocar o estado esperado da CPU ou a entrada esperada do rastreamento em `stdout`.

## Monitor interativo de terminal

Os modos `run` e `trace`, que executam uma única tarefa e encerram, são úteis para automação. O monitor acrescenta um laço de comandos no hospedeiro para inspecionar e controlar um processo VM8 em execução:

```bash
make monitor
make monitor-bin
./build/vm8 monitor path/to/program.bin
```

`make monitor` inicia com a demonstração embutida. `make monitor-bin` primeiro monta `programs/demo.asm` e inicia com `build/demo.bin`. O comando direto aceita qualquer binário bruto compatível. Em todos os casos, `main.c` seleciona e carrega o programa inicial antes de entregar o controle ao módulo dedicado do monitor.

Os comandos do monitor são:

| Comando | Efeito |
| --- | --- |
| `help` | Exibe a lista completa de comandos do monitor. |
| `registers` | Exibe registradores, `SP`, portas, flags, `PC` e o contador de ciclos. |
| `step` | Tenta executar exatamente uma instrução e exibe o resultado e o estado produzido da CPU. |
| `run` | Continua até `HALT`, opcode inválido, erro de pilha, limite de instruções ou breakpoint. |
| `reset` | Zera o estado da CPU e recarrega a imagem atual do programa no endereço zero. |
| `input <byte>` | Define a porta de entrada virtual usando um byte decimal ou hexadecimal estrito com prefixo `0x`. |
| `memory <address> [count]` | Exibe bytes a partir de um endereço de 8 bits; omitir `count` exibe 16 bytes. |
| `load <program.bin>` | Lê um novo binário não vazio, torna-o o programa atual, reinicia a CPU e o carrega. |
| `trace` | Informa se o rastreamento do monitor está ativado ou desativado. |
| `trace on` / `trace off` | Ativa ou desativa uma entrada de rastreamento para cada instrução tentada por `run` ou `step`. |
| `breakpoint add <address>` | Adiciona um endereço de parada mantido pelo hospedeiro. |
| `breakpoint remove <address>` | Remove um endereço de parada. |
| `breakpoint list` | Lista os endereços de parada ativos em ordem crescente. |
| `breakpoint clear` | Remove todos os endereços de parada. |
| `quit` | Encerra a sessão do monitor com sucesso. |

### Imagem atual do programa e reset

O `Program` entregue ao monitor pode apontar para dados pertencentes a outro módulo ou para um buffer temporário de `main.c`. Por isso, o monitor copia os bytes iniciais para seu próprio vetor de capacidade fixa. Um `reset` zera a CPU, recarrega essa imagem salva e devolve `PC` e o contador de ciclos a zero.

`load` utiliza primeiro um buffer separado para o candidato. Somente uma leitura bem-sucedida, não vazia e dentro da capacidade substitui a imagem salva. Se o caminho não existir, o arquivo estiver vazio ou o binário for grande demais, o monitor informa o erro e preserva o programa que já estava carregado. Esse é um pequeno padrão transacional: validar a substituição proposta antes de alterar o estado atual.

Reiniciar ou carregar zera o estado pertencente à CPU, como registradores, flags, portas, estado da pilha e contador de ciclos. Isso não limpa a opção de rastreamento nem a tabela de breakpoints do monitor, pois essas são configurações de depuração do hospedeiro, e não estado da CPU.

### Inspeção de memória

`memory` interpreta seu endereço com o mesmo conversor estrito de bytes usado pela interface de linha de comando. O `count` opcional precisa estar entre 1 e 255. As leituras passam por `cpu_read_memory`, então os endereços `0xEE` e `0xEF` mostram as portas atuais de entrada e saída em vez de bytes ocultos da RAM.

A exibição nunca dá a volta de `0xFF` para `0x00`. Por exemplo, `memory 0xFE 4` pode exibir somente os dois endereços existentes `0xFE` e `0xFF`. Limitar a contagem no fim do espaço de endereços impede que um comando conveniente de inspeção apresente por engano memória que deu a volta como se fosse um trecho contínuo.

### Breakpoints, execução passo a passo e rastreamento

Os breakpoints são armazenados em uma tabela de 256 valores booleanos no processo hospedeiro. O índice `0x04`, por exemplo, responde se a execução deve parar quando `PC == 0x04`. A tabela não consome nenhum byte da memória VM8 e não pode ser sobrescrita por uma instrução VM8 `STA`.

`run` verifica um breakpoint antes de executar a instrução atual e novamente depois que cada instrução bem-sucedida avança ou altera `PC`. Portanto, alcançar um breakpoint significa que a instrução marcada ainda não foi executada. `step` ignora breakpoints de propósito, permitindo executar essa instrução pendente sem remover primeiro o breakpoint. O breakpoint continua disponível se a execução voltar depois ao mesmo endereço.

A demonstração embutida mostra essa diferença porque sua instrução `SUB A, B` começa em `0x04`:

```text
vm8> breakpoint add 0x04
Breakpoint added at 0x04.
vm8> trace on
Trace enabled.
vm8> run
Execution trace:
  ADDR=0x00 OP=0x10 MNEMONIC=LDI ...
  ADDR=0x02 OP=0x11 MNEMONIC=LDI ...
Breakpoint reached at 0x04.
...
Program counter: 4
Cycle count: 2
vm8> step
Execution trace:
  ADDR=0x04 OP=0x21 MNEMONIC=SUB ...
Step result: ok
```

As duas instruções `LDI` foram executadas, então `PC` chegou ao valor decimal 4 depois de dois ciclos. `SUB` só foi executada após `step`. Com o rastreamento ativado, `run` exibe um cabeçalho seguido por todas as instruções tentadas naquela execução, enquanto `step` exibe um cabeçalho e sua única instrução tentada. Um `step` solicitado depois de `HALT` ainda informa o estado halted, mas não produz uma entrada falsa de rastreamento, pois nenhuma instrução foi tentada.

O monitor mantém a mesma garantia de execução limitada do executor que encerra após uma tarefa. Um programa que nunca para e nunca alcança um breakpoint devolve o controle depois de, no máximo, `CPU_MEMORY_SIZE` instruções tentadas, em vez de prender o usuário em um comando infinito.

## Firmware final de popcount

`programs/popcount.asm` é o firmware final de demonstração não trivial. Ele lê um byte da porta de entrada `0xEE`, conta os bits cujo valor é um e grava essa contagem, de `0` até `8`, na porta de saída `0xEF`. Essa operação costuma ser chamada de contagem populacional, ou popcount.

Por exemplo, `0xA5` é `10100101` em binário. Quatro posições contêm um, portanto o firmware produz `0x04`:

```text
entrada = 0xA5 = 10100101
saída   = 0x04 = quatro bits ligados
```

O código-fonte completo combina constantes, memória, um laço, uma sub-rotina, dados explícitos na pilha, lógica bit a bit, um deslocamento e E/S mapeada em memória:

```asm
; Counts the set bits in the virtual input byte and writes the result to output.

.EQU INPUT_PORT, 0xEE
.EQU OUTPUT_PORT, 0xEF

.EQU WORK_VALUE_ADDRESS, 0xD0
.EQU BIT_COUNT_ADDRESS, 0xD1
.EQU BITS_REMAINING_ADDRESS, 0xD2

.EQU ZERO, 0x00
.EQU ONE, 0x01
.EQU BITS_IN_BYTE, 0x08

start:
  LDA INPUT_PORT
  STA WORK_VALUE_ADDRESS

  LDI A, ZERO
  STA BIT_COUNT_ADDRESS

  LDI A, BITS_IN_BYTE
  STA BITS_REMAINING_ADDRESS

  LDI B, ONE

count_loop:
  CALL count_low_bit_and_shift

  LDA BITS_REMAINING_ADDRESS
  SUB A, B
  STA BITS_REMAINING_ADDRESS
  JNZ count_loop

  LDA BIT_COUNT_ADDRESS
  STA OUTPUT_PORT
  HALT

count_low_bit_and_shift:
  LDA WORK_VALUE_ADDRESS
  PUSH A

  AND A, B
  JZ skip_increment

  LDA BIT_COUNT_ADDRESS
  ADD A, B
  STA BIT_COUNT_ADDRESS

skip_increment:
  POP A
  SHR A
  STA WORK_VALUE_ADDRESS
  RET
```

A imagem do programa ocupa os endereços de `0x00` até `0x2B`, totalizando 44 bytes. Seus três valores mutáveis são armazenados deliberadamente bem depois do programa e antes da E/S mapeada em memória:

| Endereço | Símbolo | Finalidade |
| --- | --- | --- |
| `0xD0` | `WORK_VALUE_ADDRESS` | Cópia da entrada, deslocada uma posição para a direita a cada iteração. |
| `0xD1` | `BIT_COUNT_ADDRESS` | Quantidade de bits um encontrados até o momento. |
| `0xD2` | `BITS_REMAINING_ADDRESS` | Contador do laço, iniciado em oito e decrementado até zero. |

A inicialização copia a entrada para `0xD0`, zera o resultado em `0xD1`, armazena oito em `0xD2` e mantém a constante um no registrador `B`. O laço não termina quando o valor de trabalho se torna zero; ele sempre realiza exatamente oito iterações, portanto os zeros à esquerda são tratados de maneira consistente.

Cada iteração chama a sub-rotina em `0x1C`. Primeiro, `CALL` coloca o endereço de retorno `0x10` no endereço `0xFF` da pilha. A sub-rotina carrega o valor de trabalho e `PUSH A` armazena esse byte em `0xFE`, abaixo do endereço de retorno. Em seguida, `AND A, B` isola o bit 0 porque `B` contém `0x01`:

```text
valor de trabalho 10100101
máscara            00000001
resultado do AND   00000001
```

Quando o resultado é zero, `JZ` pula o incremento. Quando é um, as três instruções `LDA`, `ADD` e `STA` aumentam a contagem armazenada. `POP A` restaura o valor de trabalho sem a máscara, `SHR A` move seu próximo bit para a posição 0 e `STA` o salva para a iteração seguinte. Por fim, `RET` remove `0x10` da pilha e retoma o chamador. Assim, a pilha volta a ficar vazia depois de cada iteração.

Após a sub-rotina, o laço principal subtrai um do contador de bits restantes. `JNZ` volta para `count_loop` enquanto esse contador não for zero. Depois da oitava iteração, o programa carrega a contagem concluída, armazena-a no latch de saída e executa `HALT` em `0x1B`. Buscar esse opcode final avança `PC` para `0x1C`, valor decimal 28.

Os bytes gerados são:

```text
 40 ee 41 d0 10 00 41 d1 10 08 41 d2 11 01 52 1c
 40 d2 21 41 d2 32 0e 40 d1 41 ef 01 40 d0 50 22
 30 27 40 d1 20 41 d1 51 27 41 d0 53
```

O contador simplificado de ciclos pode ser deduzido, em vez de decorado:

- a inicialização usa 7 instruções;
- cada uma das 8 iterações usa 13 instruções quando seu bit testado é zero;
- cada bit um acrescenta as 3 instruções de incremento do contador;
- a saída e `HALT` usam 3 instruções.

Portanto:

```text
ciclos = 7 + (8 * 13) + (3 * quantidade de bits um) + 3
ciclos = 114 + (3 * quantidade de bits um)
```

Os principais vetores de teste são:

| Entrada | Forma binária | Saída | Ciclos | Flag zero final |
| --- | --- | --- | ---: | --- |
| `0x00` | `00000000` | `0x00` | 114 | ativada |
| `0xA5` | `10100101` | `0x04` | 126 | desativada |
| `0xFF` | `11111111` | `0x08` | 138 | desativada |

As três execuções terminam com `B = 0x01`, `SP = 0x00`, carry desativada, `PC = 0x1C`, valor de trabalho igual a zero e contador de bits restantes igual a zero. O teste dedicado de integração também confere essas invariantes internas, em vez de conferir somente a saída visível.

O mesmo código-fonte e o binário gerado estão disponíveis por meio de alvos específicos do Make:

```bash
make assemble-popcount
make inspect-popcount
make run-popcount INPUT_VALUE=0xA5
make trace-popcount INPUT_VALUE=0xA5
make monitor-popcount
```

O monitor oferece uma maneira prática de inspecionar a fronteira entre o chamador e a sub-rotina:

```text
vm8> input 0xA5
vm8> breakpoint add 0x1C
vm8> run
Breakpoint reached at 0x1C.
vm8> registers
vm8> memory 0xD0 3
vm8> memory 0xFF 1
```

Nesse breakpoint, a inicialização terminou, `CALL` foi executada, `SP` vale `0xFF` e a memória `0xFF` contém o endereço de retorno `0x10`. Avançar depois pelas instruções até `PUSH A` move `SP` para `0xFE`, ilustrando como endereços de retorno de sub-rotinas e dados inseridos explicitamente compartilham a mesma pilha limitada.

## Responsabilidades atuais dos módulos

| Módulo | Responsabilidade |
| --- | --- |
| `include/instruction_set.h`, `src/instruction_set.c` | Definições compartilhadas dos opcodes e busca somente para leitura de um byte bruto de opcode para seus metadados. |
| `include/cpu.h`, `src/cpu.c` | Estado da CPU, memória comum, E/S mapeada em memória, operações de pilha, busca, decodificação, execução, carregamento limitado do programa, execução limitada e entrega opcional de cada passo a um observador. |
| `include/cpu_trace.h`, `src/cpu_trace.c` | Formatação legível dos estados da CPU após cada instrução, incluindo `SP`, `IN`, `OUT` e erros da pilha. |
| `include/cpu_state.h`, `src/cpu_state.c` | Formatação reutilizável de todo o estado visível da CPU para a execução normal e os comandos do monitor. |
| `include/byte_value.h`, `src/byte_value.c` | Conversão estrita de um argumento do hospedeiro em decimal ou hexadecimal com prefixo `0x` para um byte. |
| `include/monitor.h`, `src/monitor.c` | Laço interativo de comandos, imagem salva do programa, controle da CPU, inspeção de memória, substituição do binário, estado do rastreamento e breakpoints no hospedeiro. |
| `include/program.h`, `src/program.c` | Descritor imutável e bytecode atual da demonstração embutida. |
| `include/binary_reader.h`, `src/binary_reader.c` | Entrada limitada de binário bruto com validação de abertura, leitura, tamanho e fechamento. |
| `include/cli.h`, `src/cli.c` | Seleção da execução única ou pelo monitor com programa embutido ou externo, rastreamento e entrada opcionais e apresentação da ajuda. |
| `src/main.c` | Seleção da origem do programa, aplicação da entrada do hospedeiro, conexão opcional do observador ou do monitor, orquestração geral do simulador e apresentação do estado final. |
| `assembler/source_line.*` | Remoção de comentários e normalização de espaços. |
| `assembler/source_reader.*` | Leitura limitada do arquivo e entrega por callback com localização no código-fonte. |
| `assembler/symbol_table.*` | Associação dos nomes dos símbolos a endereços de labels ou valores constantes de 8 bits. |
| `assembler/first_pass.*` | Coleta dos símbolos, cálculo do tamanho das instruções e validação da capacidade de programa de 238 bytes. |
| `assembler/byte_literal.*` | Conversão estrita de texto decimal e hexadecimal para `uint8_t`. |
| `assembler/byte_operand.*` | Resolução de um literal ou símbolo para um byte. |
| `assembler/instruction_parser.*` | Separação e validação estrutural dos mnemônicos e operandos das instruções. |
| `assembler/instruction_encoder.*` | Validação semântica e conversão das instruções interpretadas em bytes de opcode e operando. |
| `assembler/second_pass.*` | Ignorar labels, codificar instruções, acumular bytes com limite e emitir diagnósticos com localização no código-fonte. |
| `assembler/binary_writer.*` | Saída exata dos bytes brutos com verificação de abertura, gravação e fechamento. |
| `assembler/main.c` | Tratamento de argumentos, orquestração das duas passagens, verificação da concordância entre elas e coordenação da saída binária. |
| `tests/` | Testes unitários e de integração em C, além de um teste de processo em Bash para o executável completo do simulador. |

Manter `main.c` concentrado na orquestração torna o comportamento reutilizável testável de forma independente.

## Comandos de compilação e execução

A partir da raiz do repositório dentro do Ubuntu:

```bash
make
```

Compila o simulador.

```bash
make run
```

Compila e executa o simulador com a demonstração de bytecode embutida.

```bash
make run INPUT_VALUE=0xA5
```

Executa a mesma demonstração com a entrada `0xA5` fornecida pelo hospedeiro. A entrada pode ser observada mesmo que o programa embutido não a consuma.

```bash
make run-bin
```

Compila o simulador e o montador, traduz `programs/demo.asm` para `build/demo.bin`, carrega esse binário e o executa.

```bash
make run-popcount INPUT_VALUE=0xA5
```

Monta e executa `programs/popcount.asm` com uma entrada virtual selecionada. A quantidade resultante de bits um aparece na porta de saída.

```bash
make trace
```

Compila e executa a demonstração embutida enquanto imprime uma entrada de rastreamento depois de cada instrução tentada.

```bash
make trace INPUT_VALUE=165
```

O rastreamento e a execução normal aceitam a mesma entrada nas formas decimal ou hexadecimal.

```bash
make trace-bin
```

Monta `programs/demo.asm`, carrega `build/demo.bin` e o executa com o mesmo formato de rastreamento.

```bash
make trace-popcount INPUT_VALUE=0xA5
```

Monta e rastreia o firmware de popcount, incluindo seu laço, chamadas de sub-rotina, operações de pilha e saída final.

Um binário compatível já existente pode ser executado diretamente:

```bash
./build/vm8 run path/to/program.bin
```

Forneça uma entrada a esse binário colocando a opção por último:

```bash
./build/vm8 run path/to/program.bin --input 0xA5
```

Rastreie diretamente um binário compatível já existente:

```bash
./build/vm8 trace path/to/program.bin
```

```bash
./build/vm8 trace path/to/program.bin --input 165
```

Abra o monitor de terminal com a demonstração embutida:

```bash
make monitor
```

Monte a demonstração Assembly e abra-a no monitor:

```bash
make monitor-bin
```

Monte o firmware de popcount e abra-o no monitor:

```bash
make monitor-popcount
```

Esse alvo abre o firmware de popcount no monitor. Defina sua entrada com o comando `input <byte>` do monitor antes de executá-lo.

Abra no monitor um binário compatível que já exista:

```bash
./build/vm8 monitor path/to/program.bin
```

Execute `help` no prompt `vm8>` para exibir toda a sintaxe interativa.

```bash
make help
```

Exibe os comandos do simulador e a referência das instruções.

```bash
make test
```

Monta os dois programas de demonstração e executa todos os testes unitários, de integração e de processo automatizados. Testes dedicados verificam todas as associações atuais entre opcode e mnemônico, a rejeição de um opcode desconhecido, a entrega ao observador, a formatação exata do rastreamento e do estado da CPU, os limites do mapa de memória, a direção das portas, a reinicialização das portas, a interpretação estrita de bytes do hospedeiro, a seleção pela CLI e o laço de comandos do monitor. Os testes do monitor cobrem execução passo a passo, execução limitada, reset, exibição da memória mapeada sem dar a volta nos endereços, substituição segura do binário, persistência dos breakpoints e do estado do rastreamento, argumentos inválidos, fim de arquivo e entrada longa demais. O teste original do programa montado verifica `build/demo.bin`. O teste de integração do popcount executa `build/popcount.bin` com as entradas `0x00`, `0xA5` e `0xFF`, depois confere resultados visíveis e invariantes internas. `tests/test_vm8_process.sh` inicia o executável real e também confere a execução do popcount e sua entrada final de rastreamento.

```bash
make assembler
```

Compila o executável independente `build/vm8asm`.

```bash
make assemble
```

Compila `vm8asm` quando necessário, executa as duas passagens sobre `programs/demo.asm` e grava os 19 bytes brutos resultantes em `build/demo.bin`.

O mesmo montador pode ser chamado diretamente com caminhos explícitos:

```bash
./build/vm8asm programs/demo.asm build/demo.bin
```

Confira o tamanho exato da saída:

```bash
wc -c build/demo.bin
```

Saída esperada:

```text
19 build/demo.bin
```

`wc -c` conta bytes, e não linhas ou palavras. Isso confirma que o binário contém os 19 bytes calculados pelas duas passagens do montador.

Exiba todos os bytes brutos em hexadecimal:

```bash
od -An -tx1 -v build/demo.bin
```

Saída esperada:

```text
 10 2a 11 2a 21 30 09 10 ff 40 12 41 80 10 00 40
 80 01 5a
```

As opções de `od` significam:

- `-An`: omite a coluna de endereços;
- `-tx1`: formata cada unidade de um byte em hexadecimal;
- `-v`: exibe todos os dados em vez de abreviar linhas repetidas.

Isso lê o binário como bytes. Executar `cat build/demo.bin` não é útil porque muitos valores de byte do código de máquina não são caracteres imprimíveis.

Monte a demonstração e execute as duas inspeções em uma etapa:

```bash
make inspect
```

Em vez disso, gere e inspecione o firmware final de popcount:

```bash
make assemble-popcount
make inspect-popcount
```

`build/popcount.bin` precisa conter 44 bytes. `make inspect-popcount` aplica as mesmas verificações com `wc -c` e `od -An -tx1 -v` mostradas acima, portanto confere tanto o tamanho quanto a codificação exata dos bytes sem tratar o binário como texto.

## Ideias principais a recordar

- Valores de 8 bits são buscados e processados um byte por vez; uma instrução pode conter vários bytes.
- O opcode informa à CPU quantos bytes adicionais buscar e como interpretá-los.
- As definições dos opcodes pertencem ao módulo do conjunto de instruções, e não à interface completa da CPU.
- A busca nos metadados recebe um byte bruto e retorna nulo quando esse byte não é um opcode suportado.
- Operações lógicas processam independentemente os bits correspondentes, enquanto os deslocamentos preservam na flag carry o bit descartado.
- `CMP` atualiza zero e empréstimo sem alterar seus operandos; saltos condicionais consultam as flags sem modificá-las.
- Labels e mnemônicos pertencem ao montador, e não à CPU.
- Um label não consome memória do programa; ele nomeia o endereço de byte atual.
- `.EQU` dá a um literal de 8 bits um nome simbólico reutilizável e não emite nenhum byte.
- `.BYTE` emite um byte bruto e pode colocar dados na mesma imagem de memória que o código.
- Labels e constantes compartilham um único espaço de nomes que diferencia letras maiúsculas e minúsculas.
- A primeira passagem descobre os endereços, e a segunda substitui referências simbólicas por bytes numéricos.
- O gravador binário armazena os valores gerados como bytes brutos, e não como texto hexadecimal.
- O leitor binário utiliza uma capacidade fornecida pelo chamador e verifica um byte adicional para rejeitar entradas grandes demais com segurança.
- A E/S mapeada em memória reutiliza `LDA` e `STA`; o endereço decodificado seleciona a memória comum, a porta de entrada ou o latch de saída.
- O hospedeiro controla a entrada `0xEE`, o software VM8 controla a saída `0xEF` e a reinicialização zera ambas.
- A CLI aceita uma opção final `--input` de 0 até 255 na forma decimal ou hexadecimal com prefixo `0x`.
- `wc -c` verifica a quantidade de bytes, enquanto `od -An -tx1 -v` revela os valores exatos.
- Programas embutidos e carregados de arquivo utilizam as mesmas funções de carregamento e execução da CPU.
- Um observador recebe o endereço e o opcode anteriores a um passo junto com o estado da CPU posterior a esse passo.
- O rastreamento é uma camada de apresentação sobre a execução da CPU; o núcleo da CPU não imprime nada por conta própria.
- O monitor também é software do hospedeiro; o texto dos comandos, a imagem salva do programa, a opção de rastreamento e a tabela de breakpoints não consomem memória VM8.
- `run` para antes de executar um endereço com breakpoint, enquanto `step` executa de propósito a instrução atual mesmo quando esse endereço continua marcado.
- `reset` e um `load` bem-sucedido no monitor substituem o estado da CPU, mas preservam as configurações de depuração; um `load` que falha também preserva a imagem atual do programa.
- A inspeção da memória utiliza a decodificação de endereços da CPU e para em `0xFF`, portanto revela as portas mapeadas sem dar a volta até o endereço zero.
- A CPU finalmente executa somente uma sequência de bytes, independentemente da origem desses bytes.
- Testes unitários validam funções isoladamente, enquanto o teste de processo em Bash valida o programa compilado por meio de sua interface pública de linha de comando.
- `PC` mede endereços de bytes, enquanto o contador simplificado de ciclos mede instruções tentadas.
- A memória unificada permite acesso tanto ao código quanto aos dados, portanto as instruções de armazenamento devem usar endereços com cuidado.
- O programa carregado é limitado de `0x00` até `0xED`; `0xEE` e `0xEF` fornecem E/S, enquanto a pilha reserva de `0xF0` até `0xFF` e cresce para baixo.
- `SP = 0x00` é um sentinela de pilha vazia; `PUSH` e `POP` movimentam bytes na ordem último a entrar, primeiro a sair e informam explicitamente os erros de limite.
- `CALL` insere o `PC` já avançado como endereço de retorno de 8 bits; `RET` retira esse byte novamente para `PC`.
- Dados e endereços de retorno compartilham a mesma pilha, portanto operações equilibradas e profundidade disponível da pilha são responsabilidades do software.
- O firmware de popcount reúne a arquitetura: lê a entrada mapeada, percorre oito bits em um laço, guarda estado mutável na memória, chama uma sub-rotina que usa a pilha e grava a saída mapeada.
- Um laço fixo de oito iterações trata qualquer byte de entrada; seu tempo simplificado de execução é `114 + 3 * popcount(entrada)` ciclos.
