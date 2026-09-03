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

O simulador da CPU está funcional. Ele pode executar tanto a demonstração embutida de 18 bytes armazenada em `src/program.c` quanto um binário bruto compatível selecionado pela linha de comando.

O montador atualmente implementa:

- validação dos argumentos da linha de comando;
- leitura limitada do arquivo-fonte;
- remoção de comentários e espaços ao redor da linha;
- entrega dos statements normalizados por meio de um callback;
- uma tabela de símbolos;
- uma primeira passagem que registra labels e calcula o tamanho do programa;
- interpretação de literais de 8 bits em decimal e hexadecimal;
- resolução de um operando como literal ou símbolo;
- um parser de instruções que separa mnemônicos e operandos;
- um codificador que valida a semântica das instruções e emite opcodes e operandos;
- uma segunda passagem que resolve símbolos e acumula a sequência completa de bytes do programa;
- um gravador de binário que armazena os bytes brutos no arquivo de saída solicitado.

`make assemble` traduz `programs/demo.asm` para o arquivo de 18 bytes `build/demo.bin`. `make inspect` realiza essa montagem e depois exibe o tamanho e os bytes brutos gerados. `make run` executa o bytecode embutido equivalente de `src/program.c`, enquanto `make run-bin` monta, carrega e executa `build/demo.bin`.

## O que “8 bits” significa

Neste projeto, 8 bits descreve a largura natural dos dados da CPU:

- o registrador `A` armazena um valor de 8 bits;
- o registrador `B` armazena um valor de 8 bits;
- cada posição da memória armazena um valor de 8 bits;
- o contador de programa armazena um endereço de 8 bits;
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
| Flag zero (`Z`) | Booleana | Indica que o resultado mais recente que atualiza flags foi zero. |
| Flag carry (`C`) | Booleana | Indica carry de saída na adição ou empréstimo na subtração. |
| Estado halted | Booleano | Impede novas execuções após uma parada ou opcode inválido. |
| Contador de ciclos | 64 bits | Conta instruções tentadas no modelo simplificado de temporização. |
| Memória | 256 bytes | Armazena tanto os bytes do programa quanto os bytes de dados. |

O estado inicial normal é completamente inicializado com zeros:

```c
Cpu cpu = {0};
```

O programa é então copiado para a memória começando no endereço `0x00`.

## Memória unificada de código e dados

O projeto usa um modelo de memória unificada. As instruções e os dados comuns ocupam o mesmo vetor de 256 bytes.

Por exemplo, o programa de demonstração ocupa os endereços de `0x00` até `0x11`, enquanto usa o endereço `0x80` para armazenar dados. A instrução:

```asm
STA 0x80
```

grava o registrador `A` na posição de memória `0x80`.

Como código e dados compartilham o mesmo vetor, uma gravação direcionada a um endereço do programa poderia sobrescrever uma instrução. A demonstração atual coloca seus dados deliberadamente fora da região do programa.

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
| `JZ addr8` | `30 addr8` | 2 | Carrega `PC` com o endereço absoluto quando `Z` está ativa. |
| `JMP addr8` | `31 addr8` | 2 | Sempre carrega `PC` com o endereço absoluto. |
| `LDA addr8` | `40 addr8` | 2 | Carrega `A` a partir do endereço de memória; atualiza `Z`; preserva `C`. |
| `STA addr8` | `41 addr8` | 2 | Armazena `A` no endereço de memória; preserva registradores e flags. |

`imm8` e `addr8` possuem um byte cada. Portanto, podem representar valores de `0x00` até `0xFF`.

## Flags

### Flag zero

A flag zero é ativada quando uma instrução que atualiza flags produz zero. Atualmente ela é atualizada por:

- `LDI A, imm8`;
- `LDI B, imm8`;
- `ADD A, B`;
- `SUB A, B`;
- `LDA addr8`.

`JZ` lê a flag zero, mas não a modifica.

### Flag carry

A flag carry possui dois significados relacionados para operações sem sinal:

- após `ADD`, indica carry de saída além de `0xFF`;
- após `SUB`, indica que foi necessário um empréstimo porque o valor original de `A` era menor que `B`.

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

## Contador de programa e contador de ciclos

O contador de programa conta **endereços de bytes**, e não instruções. Portanto:

- uma instrução de um byte normalmente avança `PC` em um;
- uma instrução de dois bytes normalmente avança `PC` em dois;
- um salto realizado substitui `PC` pelo endereço de destino depois que o byte de destino foi buscado.

O contador de ciclos segue um modelo deliberadamente simplificado: toda instrução tentada conta como um ciclo, independentemente de a instrução ocupar um ou dois bytes.

Assim, um programa de 18 bytes pode executar somente nove instruções e terminar com:

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

Um **símbolo** é um nome associado a uma informação conhecida pelo montador. Na linguagem atual, os símbolos são labels associados a endereços de bytes. A tabela de símbolos poderia conter:

```text
start       -> 0x00
memory_demo -> 0x09
```

Os nomes podem conter vários caracteres porque são armazenados e processados pelo montador executado no computador hospedeiro. Somente o endereço resolvido de 8 bits é gravado no código de máquina. O nome em si nunca entra na memória da CPU.

Os labels diferenciam letras maiúsculas e minúsculas, devem começar com uma letra ou sublinhado e podem continuar com letras, números ou sublinhados. Os mnemônicos das instruções e os nomes dos registradores são palavras reservadas e não podem ser usados como labels.

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
- Uma instrução de um byte avança o tamanho em um.
- Uma instrução de dois bytes avança o tamanho em dois.
- Labels duplicados, malformados, reservados ou fora da memória são rejeitados.
- Mnemônicos desconhecidos e programas maiores que a memória são rejeitados.

A sintaxe dos operandos não é totalmente interpretada durante essa passagem. Apenas o mnemônico e o tamanho codificado da instrução são necessários para calcular os endereços.

### Resolução de literais e símbolos

O interpretador de literais aceita valores estritos em decimal ou hexadecimal com prefixo `0x`:

```text
42   -> 0x2A
0x2A -> 0x2A
255  -> 0xFF
```

Sinais, espaços internos, dígitos malformados e valores acima de 255 são rejeitados.

O resolvedor de operandos de byte primeiro tenta interpretar o texto como literal. Se o texto não for um literal, mas for um nome de símbolo válido, ele consulta a tabela de símbolos:

```text
0x80        -> byte literal 0x80
memory_demo -> endereço de símbolo 0x09
missing     -> erro de símbolo indefinido
```

### Segunda passagem

O parser de instruções separa cada statement normalizado em um mnemônico e até dois operandos. Ele valida a estrutura sintática, como espaços, vírgulas, operandos ausentes e operandos em excesso, mas não decide se um mnemônico ou registrador é suportado.

O codificador de instruções então valida o significado dos campos interpretados. Ele reconhece o conjunto atual de instruções, verifica a quantidade de operandos e a ordem dos registradores, resolve literais de byte ou símbolos e emite uma instrução codificada de um ou dois bytes. Uma falha de codificação deixa inalterado o objeto de saída fornecido pelo chamador.

A segunda passagem lê novamente os statements normalizados, ignora as declarações de labels, executa o parser e o codificador e acrescenta cada codificação bem-sucedida a um buffer limitado do programa. Ela informa diagnósticos com o caminho original do arquivo e o número da linha, conta as instruções codificadas e rejeita qualquer gravação que ultrapassaria a capacidade de saída fornecida.

Por exemplo:

```asm
JZ memory_demo
```

torna-se:

```text
30 09
```

Depois que as duas passagens concordam sobre o tamanho de 18 bytes do programa, o gravador de binário abre o caminho solicitado em modo binário, grava exatamente essa quantidade de bytes e verifica tanto a gravação quanto o fechamento final do arquivo. Portanto, `make assemble` cria `build/demo.bin` como código de máquina bruto.

## Codificação completa da demonstração

A demonstração Assembly atual é:

```asm
; Demonstrates arithmetic, branching, and memory transfer.

start:
  LDI A, 0x2A
  LDI B, 0x2A
  SUB A, B
  JZ memory_demo
  LDI A, 0xFF

memory_demo:
  LDI A, 0x5A
  STA 0x80
  LDI A, 0x00
  LDA 0x80
  HALT
```

O cálculo dos endereços e a codificação gerada são:

| Endereço | Statement do código-fonte | Bytes emitidos | Explicação |
| ---: | --- | --- | --- |
| `0x00` | `start:` | — | Registra `start = 0x00`; não emite nada. |
| `0x00` | `LDI A, 0x2A` | `10 2A` | Ocupa `0x00` e `0x01`. |
| `0x02` | `LDI B, 0x2A` | `11 2A` | Ocupa `0x02` e `0x03`. |
| `0x04` | `SUB A, B` | `21` | Instrução de um byte. |
| `0x05` | `JZ memory_demo` | `30 09` | Resolve `memory_demo` como `0x09`. |
| `0x07` | `LDI A, 0xFF` | `10 FF` | Ignorada quando o salto é realizado. |
| `0x09` | `memory_demo:` | — | Registra `memory_demo = 0x09`; não emite nada. |
| `0x09` | `LDI A, 0x5A` | `10 5A` | Ocupa `0x09` e `0x0A`. |
| `0x0B` | `STA 0x80` | `41 80` | Armazena `A` na memória de dados. |
| `0x0D` | `LDI A, 0x00` | `10 00` | Limpa `A`. |
| `0x0F` | `LDA 0x80` | `40 80` | Recarrega o valor armazenado. |
| `0x11` | `HALT` | `01` | Para após buscar o byte. |

A sequência completa gerada de 18 bytes é:

```text
10 2A 11 2A 21 30 09 10 FF 10 5A 41 80 10 00 40 80 01
```

## Passo a passo da execução da demonstração

A CPU começa com registradores zerados, flags inativas, `PC = 0x00` e contador de ciclos igual a zero.

| Passo | Endereço | Instrução | Estado importante após a execução |
| ---: | ---: | --- | --- |
| 1 | `0x00` | `LDI A, 0x2A` | `A = 0x2A`, `Z = 0`, `PC = 0x02`, ciclos = 1. |
| 2 | `0x02` | `LDI B, 0x2A` | `B = 0x2A`, `Z = 0`, `PC = 0x04`, ciclos = 2. |
| 3 | `0x04` | `SUB A, B` | `A = 0x00`, `Z = 1`, sem empréstimo, `PC = 0x05`, ciclos = 3. |
| 4 | `0x05` | `JZ 0x09` | O operando de destino é buscado e a flag zero ativa muda `PC` para `0x09`; ciclos = 4. |
| 5 | `0x09` | `LDI A, 0x5A` | `A = 0x5A`, `Z = 0`, `PC = 0x0B`, ciclos = 5. |
| 6 | `0x0B` | `STA 0x80` | `memory[0x80] = 0x5A`, `PC = 0x0D`, ciclos = 6. |
| 7 | `0x0D` | `LDI A, 0x00` | `A = 0x00`, `Z = 1`, `PC = 0x0F`, ciclos = 7. |
| 8 | `0x0F` | `LDA 0x80` | `A = 0x5A`, `Z = 0`, `PC = 0x11`, ciclos = 8. |
| 9 | `0x11` | `HALT` | CPU parada, `PC = 0x12` (decimal 18), ciclos = 9. |

A instrução no endereço `0x07` nunca é executada porque `SUB A, B` produziu zero e `JZ` saltou diretamente para `0x09`.

O estado final visível é:

```text
Execution result: halted
Register A: 0x5A
Register B: 0x2A
Zero flag: clear
Carry flag: clear
Program counter: 18
Cycle count: 9
```

## Carregamento e execução limitada

`cpu_load_program` verifica se a sequência de bytes cabe na memória e rejeita um ponteiro nulo para um programa não vazio. Ele copia o programa a partir do endereço zero, mas não reinicializa a CPU automaticamente.

`cpu_run` recebe um limite de instruções. Isso impede que um laço incondicional como `JMP 0x00` continue para sempre sem devolver o controle ao chamador. A execução informa um de três resultados:

- parada normal;
- opcode inválido;
- limite de instruções atingido.

Um opcode inválido também para a CPU para que a execução não continue silenciosamente sobre dados desconhecidos.

## Leitura e execução de um binário externo

O leitor binário do simulador abre o arquivo selecionado em modo binário e lê seu conteúdo para um buffer fornecido pelo chamador. `main.c` fornece um buffer cuja capacidade é exatamente `CPU_MEMORY_SIZE`, portanto o leitor não consegue gravar além da capacidade de programa de 256 bytes da máquina virtual.

Depois de preencher o buffer, o leitor tenta buscar mais um byte. Essa leitura adicional diferencia dois casos que, sem ela, produziriam igualmente um buffer cheio:

- se a leitura adicional alcançar o fim do arquivo, o programa possui exatamente 256 bytes e é válido;
- se existir outro byte, o arquivo é grande demais e será rejeitado.

O leitor informa a quantidade de bytes realmente lida somente depois que tanto a leitura quanto o fechamento do arquivo terminam corretamente. Um arquivo vazio é um arquivo binário válido do ponto de vista estrito de entrada e saída do leitor, mas `main.c` o rejeita como programa executável. Essa separação mantém o transporte do arquivo separado da política do simulador.

A CLI oferece dois caminhos de execução:

```text
make run
  -> vetor de bytes embutido de src/program.c

make run-bin
  -> programs/demo.asm
  -> vm8asm
  -> build/demo.bin
  -> binary_reader_read
  -> cpu_load_program
  -> cpu_run
```

A forma direta `./build/vm8 run <program.bin>` utiliza o mesmo caminho de binário externo sem executar primeiro o montador. O simulador não sabe se esse arquivo veio de `vm8asm`, de outra ferramenta ou da inserção manual de bytes; ele enxerga somente os bytes.

O teste de processo em Bash exercita essa interface pública em vez de chamar diretamente as funções C. Ele verifica um binário válido e quatro falhas esperadas: arquivo inexistente, arquivo vazio, arquivo de 257 bytes e arquivo contendo o opcode inválido `0xFF`. Cada falha precisa retornar um status de processo diferente de zero e colocar o diagnóstico esperado em `stderr`; a execução bem-sucedida precisa colocar o estado esperado da CPU em `stdout`.

## Responsabilidades atuais dos módulos

| Módulo | Responsabilidade |
| --- | --- |
| `include/cpu.h`, `src/cpu.c` | Estado da CPU, operações de memória, busca, decodificação, execução, carregamento do programa e execução limitada. |
| `include/program.h`, `src/program.c` | Descritor imutável e bytecode atual da demonstração embutida. |
| `include/binary_reader.h`, `src/binary_reader.c` | Entrada limitada de binário bruto com validação de abertura, leitura, tamanho e fechamento. |
| `include/cli.h`, `src/cli.c` | Seleção dos comandos de demonstração embutida, binário externo e ajuda, além da apresentação da ajuda. |
| `src/main.c` | Seleção da origem do programa, orquestração geral do simulador e apresentação do estado final. |
| `assembler/source_line.*` | Remoção de comentários e normalização de espaços. |
| `assembler/source_reader.*` | Leitura limitada do arquivo e entrega por callback com localização no código-fonte. |
| `assembler/symbol_table.*` | Associação dos nomes dos símbolos a endereços de 8 bits. |
| `assembler/first_pass.*` | Coleta dos labels, cálculo do tamanho das instruções e validação da capacidade da memória. |
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
make run-bin
```

Compila o simulador e o montador, traduz `programs/demo.asm` para `build/demo.bin`, carrega esse binário e o executa.

Um binário compatível já existente pode ser executado diretamente:

```bash
./build/vm8 run path/to/program.bin
```

```bash
make help
```

Exibe os comandos do simulador e a referência das instruções.

```bash
make test
```

Monta a demonstração e executa todos os testes unitários, de integração e de processo automatizados. O teste do programa montado lê `build/demo.bin`, carrega-o na memória da CPU, executa-o e verifica os registradores, as flags, o contador de programa, o contador de ciclos e o dado armazenado esperados. Em seguida, `tests/test_vm8_process.sh` inicia o executável real e verifica seu status de processo e seus fluxos de saída para entradas válidas e inválidas.

```bash
make assembler
```

Compila o executável independente `build/vm8asm`.

```bash
make assemble
```

Compila `vm8asm` quando necessário, executa as duas passagens sobre `programs/demo.asm` e grava os 18 bytes brutos resultantes em `build/demo.bin`.

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
18 build/demo.bin
```

`wc -c` conta bytes, e não linhas ou palavras. Isso confirma que o binário contém os 18 bytes calculados pelas duas passagens do montador.

Exiba todos os bytes brutos em hexadecimal:

```bash
od -An -tx1 -v build/demo.bin
```

Saída esperada:

```text
 10 2a 11 2a 21 30 09 10 ff 10 5a 41 80 10 00 40
 80 01
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

## Ideias principais a recordar

- Valores de 8 bits são buscados e processados um byte por vez; uma instrução pode conter vários bytes.
- O opcode informa à CPU quantos bytes adicionais buscar e como interpretá-los.
- Labels e mnemônicos pertencem ao montador, e não à CPU.
- Um label não consome memória do programa; ele nomeia o endereço de byte atual.
- A primeira passagem descobre os endereços, e a segunda substitui referências simbólicas por bytes numéricos.
- O gravador binário armazena os valores gerados como bytes brutos, e não como texto hexadecimal.
- O leitor binário utiliza uma capacidade fornecida pelo chamador e verifica um byte adicional para rejeitar entradas grandes demais com segurança.
- `wc -c` verifica a quantidade de bytes, enquanto `od -An -tx1 -v` revela os valores exatos.
- Programas embutidos e carregados de arquivo utilizam as mesmas funções de carregamento e execução da CPU.
- A CPU finalmente executa somente uma sequência de bytes, independentemente da origem desses bytes.
- Testes unitários validam funções isoladamente, enquanto o teste de processo em Bash valida o programa compilado por meio de sua interface pública de linha de comando.
- `PC` mede endereços de bytes, enquanto o contador simplificado de ciclos mede instruções tentadas.
- A memória unificada permite acesso tanto ao código quanto aos dados, portanto as instruções de armazenamento devem usar endereços com cuidado.
