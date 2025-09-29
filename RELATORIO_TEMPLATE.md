# Relatório: Mini-Projeto 1 - Quebra-Senhas Paralelo

**Aluno(s):** 
Beatriz Silva Nóbrega - RA 10435789
Eduardo Kenji - RA - 10439924
Gabriel Ferreira - RA 10442043
Pedro Henrique Saraiva Arruda - RA 10437747
---

## 1. Estratégia de Paralelização


**Como você dividiu o espaço de busca entre os workers?**

O nosso grupo resolveu essa divisão do espaço pelo particionamento estático em blocos, calculando o numero total de senhas(espaço de busca), que é C^L: Tamanho do Charset elevado ao tamanho da senha.
Após isso, aplicamos a divisão da quantidade de senhas pelo n° de workers, nos dando a chunk(pedaço) de trabalho para cada worker. Assim, cada um recebe um índice de start e um de end, que são convertidos para a senha inicial e final.

**Código relevante:** Cole aqui a parte do coordinator.c onde você calcula a divisão:
```c
// Cole seu código de divisão aqui
```
// Cole seu código de divisão aqui
	long long passwords_per_worker = total_space / num_workers;
	long long remaining = total_space % num_workers;
	
	// Arrays para armazenar PIDs dos workers
	pid_t workers[MAX_WORKERS];
	
	// TODO 3: Criar os processos workers usando fork()
	printf("Iniciando workers...\n");
	
	long long current_start_index = 0;
	// IMPLEMENTE AQUI: Loop para criar workers
	for (int i = 0; i < num_workers; i++) {
		// TODO: Calcular intervalo de senhas para este worker
		long long chunk_size = passwords_per_worker + (i < remaining ? 1 : 0);
		if (chunk_size == 0) continue;
		long long end_index = current_start_index + chunk_size - 1;

		// TODO: Converter indices para senhas de inicio e fim
		char start_password[MAX_PASSWORD_LEN + 1];
		char end_password[MAX_PASSWORD_LEN + 1];
		index_to_password(current_start_index, charset, charset_len, password_len, start_password);
		index_to_password(end_index, charset, charset_len, password_len, end_password);
		
		// TODO 4: Usar fork() para criar processo filho
		pid_t pid = fork();

		if (pid < 0) {
			// TODO 7: Tratar erros de fork() e execl()
			perror("fork falhou");
			exit(EXIT_FAILURE);
		}

		if (pid == 0) {
			// TODO 6: No processo filho: usar execl() para executar worker
			char len_str[4];
			char id_str[4];
			snprintf(len_str, sizeof(len_str), "%d", password_len);
			snprintf(id_str, sizeof(id_str), "%d", i);
			execl("./worker", "worker", target_hash, start_password, end_password, charset, len_str, id_str, NULL);
			
			// TODO 7: Tratar erros de fork() e execl()
			perror("execl falhou");
			exit(EXIT_FAILURE);
		} else {
			// TODO 5: No processo pai: armazenar PID
			workers[i] = pid;
		}
		current_start_index += chunk_size;
	}
---

## 2. Implementação das System Calls

**Descreva como você usou fork(), execl() e wait() no coordinator:**

O coordinator usa um loop para criar o número específico de workers. A cada Iteração, a Syscall Fork é usada para duplicar o processo coordinator. No processo filho, Execl é utilizado para substituir a imagem do processo atual pelo programa worker. Wait é usado para aguardar o fim de cada processo filho que foi criado, evitando Processos Zumbis

**Código do fork/exec:**
```c
	for (int i = 0; i < num_workers; i++) {
		// TODO: Calcular intervalo de senhas para este worker
		long long chunk_size = passwords_per_worker + (i < remaining ? 1 : 0);
		if (chunk_size == 0) continue;
		long long end_index = current_start_index + chunk_size - 1;

		// TODO: Converter indices para senhas de inicio e fim
		char start_password[MAX_PASSWORD_LEN + 1];
		char end_password[MAX_PASSWORD_LEN + 1];
		index_to_password(current_start_index, charset, charset_len, password_len, start_password);
		index_to_password(end_index, charset, charset_len, password_len, end_password);
		
		// TODO 4: Usar fork() para criar processo filho
		pid_t pid = fork();

		if (pid < 0) {
			// TODO 7: Tratar erros de fork() e execl()
			perror("fork falhou");
			exit(EXIT_FAILURE);
		}

		if (pid == 0) {
			// TODO 6: No processo filho: usar execl() para executar worker
			char len_str[4];
			char id_str[4];
			snprintf(len_str, sizeof(len_str), "%d", password_len);
			snprintf(id_str, sizeof(id_str), "%d", i);
			execl("./worker", "worker", target_hash, start_password, end_password, charset, len_str, id_str, NULL);
			
			// TODO 7: Tratar erros de fork() e execl()
			perror("execl falhou");
			exit(EXIT_FAILURE);
		} else {
			// TODO 5: No processo pai: armazenar PID
			workers[i] = pid;
		}
		current_start_index += chunk_size;
	}
```

---

## 3. Comunicação Entre Processos

**Como você garantiu que apenas um worker escrevesse o resultado?**
Fazendo as flags "O_CREAT | O_EXCL" na Syscall Open, fazendo com que o SO crie um arquivo, mas falhe caso ele já exista

**Como o coordinator consegue ler o resultado?**

Usando o Wait, fazemos o coordinator aguardar todos os processos worker serem finalizados. Após isso, ele tenta abrir o arquivo de resultado em modo leituira. Caso a fopen retorne um ponteiro não nulo, o arquivo terá sido criado com êxito, e teeve sua senha encontrada; Caso fopen retorne nulo, o arquivo não existe e o coordinator entende que a senha não foi encontrada
---

## 4. Análise de Performance
Complete a tabela com tempos reais de execução:
O speedup é o tempo do teste com 1 worker dividido pelo tempo com 4 workers.

| Teste | 1 Worker | 2 Workers | 4 Workers | Speedup (4w) |
|-------|----------|-----------|-----------|--------------|
| Hash: 202cb962ac59075b964b07152d234b70<br>Charset: "0123456789"<br>Tamanho: 3<br>Senha: "123" | 0,014s | 0,011s | 0,009s | 1,56x |
| Hash: 5d41402abc4b2a76b9719d911017c592<br>Charset: "abcdefghijklmnopqrstuvwxyz"<br>Tamanho: 5<br>Senha: "hello" | 1,28s | 0,7s | 0,38s | 3,37x |

**O speedup foi linear? Por quê?**
Não. O motivo disso foi em grande parte por conta do Overheadassociado à criação de processos:
Houve um desbalanceamento de carga, onde no primeiro teste(senha 123) a senha está logo no início, desperdiçando tempo para criar os outros workers, explicando o motivo do baixo Speedup(1,56x)

---

## 5. Desafios e Aprendizados
**Qual foi o maior desafio técnico que você enfrentou?**
Ao discutir com meus amigos, chegamos à conclusão que a implementação de "increment_password" (localizado no worker.c) foi a mais trabalhosa, tendo em vista que não poderíamos usar loops que pudessen ser escalados. Tivemos que utilizar um lógica pareceida com o "vai um" na operação de soma: verifiamos o último caracterer da senha. Caso ele nãi seja o último caractere do charset, trocamos ele pelo próximo. Se ele for o último, ele será "resetado" para o primeiro caractere do charset, e nele será aplicada a lógica de incremento ao caractere anterior a ele. Esse processo é repetido até que o incremento seja realizado, ou até um overflow(senha teve seu início quebrado)
---

## Comandos de Teste Utilizados

```bash
# Teste básico
./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 2

# Teste de performance
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 1
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 4

# Teste com senha maior
time ./coordinator "5d41402abc4b2a76b9719d911017c592" 5 "abcdefghijklmnopqrstuvwxyz" 4
```
---

**Checklist de Entrega:**
- [ FEITO] Código compila sem erros
- [ FEITO] Todos os TODOs foram implementados
- [ FEITO] Testes passam no `./tests/simple_test.sh`
- [ FEITO] Relatório preenchido