#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "hash_utils.h"

/**
 * PROCESSO COORDENADOR - Mini-Projeto 1: Quebra de Senhas Paralelo
 * * Este programa coordena múltiplos workers para quebrar senhas MD5 em paralelo.
 * O MD5 JÁ ESTÁ IMPLEMENTADO - você deve focar na paralelização (fork/exec/wait).
 * * Uso: ./coordinator <hash_md5> <tamanho> <charset> <num_workers>
 * * Exemplo: ./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 4
 * * SEU TRABALHO: Implementar os TODOs marcados abaixo
 */

#define MAX_WORKERS 16
#define RESULT_FILE "password_found.txt"
#define MAX_PASSWORD_LEN 10

/**
 * Calcula o tamanho total do espaço de busca
 * * @param charset_len Tamanho do conjunto de caracteres
 * @param password_len Comprimento da senha
 * @return Número total de combinações possíveis
 */
long long calculate_search_space(int charset_len, int password_len) {
    long long total = 1;
    for (int i = 0; i < password_len; i++) {
        total *= charset_len;
    }
    return total;
}

/**
 * Converte um índice numérico para uma senha
 * Usado para definir os limites de cada worker
 * * @param index Índice numérico da senha
 * @param charset Conjunto de caracteres
 * @param charset_len Tamanho do conjunto
 * @param password_len Comprimento da senha
 * @param output Buffer para armazenar a senha gerada
 */
void index_to_password(long long index, const char *charset, int charset_len, 
                       int password_len, char *output) {
    for (int i = password_len - 1; i >= 0; i--) {
        output[i] = charset[index % charset_len];
        index /= charset_len;
    }
    output[password_len] = '\0';
}

/**
 * Função principal do coordenador
 */
int main(int argc, char *argv[]) {
    // TODO 1: Validar argumentos de entrada
    // Verificar se argc == 5 (programa + 4 argumentos)
    // Se não, imprimir mensagem de uso e sair com código 1
    
    // IMPLEMENTE AQUI: verificação de argc e mensagem de erro
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <hash_md5> <tamanho> <charset> <num_workers>\n", argv[0]);
        return 1;
    }
    
    // Parsing dos argumentos (após validação)
    const char *target_hash = argv[1];
    int password_len = atoi(argv[2]);
    const char *charset = argv[3];
    int num_workers = atoi(argv[4]);
    int charset_len = strlen(charset);
    
    // TODO: Adicionar validações dos parâmetros
    // - password_len deve estar entre 1 e 10
    // - num_workers deve estar entre 1 e MAX_WORKERS
    // - charset não pode ser vazio
    if (password_len <= 0 || password_len > MAX_PASSWORD_LEN) {
        fprintf(stderr, "Erro: O tamanho da senha deve estar entre 1 e %d.\n", MAX_PASSWORD_LEN);
        return 1;
    }
    if (num_workers <= 0 || num_workers > MAX_WORKERS) {
        fprintf(stderr, "Erro: O número de workers deve estar entre 1 e %d.\n", MAX_WORKERS);
        return 1;
    }
    if (charset_len == 0) {
        fprintf(stderr, "Erro: O charset não pode ser vazio.\n");
        return 1;
    }
    
    printf("=== Mini-Projeto 1: Quebra de Senhas Paralelo ===\n");
    printf("Hash MD5 alvo: %s\n", target_hash);
    printf("Tamanho da senha: %d\n", password_len);
    printf("Charset: %s (tamanho: %d)\n", charset, charset_len);
    printf("Número de workers: %d\n", num_workers);
    
    // Calcular espaço de busca total
    long long total_space = calculate_search_space(charset_len, password_len);
    printf("Espaço de busca total: %lld combinações\n\n", total_space);
    
    // Remover arquivo de resultado anterior se existir
    unlink(RESULT_FILE);
    
    // Registrar tempo de início
    time_t start_time = time(NULL);
    
    // TODO 2: Dividir o espaço de busca entre os workers
    // Calcular quantas senhas cada worker deve verificar
    // DICA: Use divisão inteira e distribua o resto entre os primeiros workers
    
    // IMPLEMENTE AQUI:
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
    
    printf("\nTodos os workers foram iniciados. Aguardando conclusão...\n");
    
    // TODO 8: Aguardar todos os workers terminarem usando wait()
    // IMPORTANTE: O pai deve aguardar TODOS os filhos para evitar zumbis
    
    // IMPLEMENTE AQUI:
    for (int i = 0; i < num_workers; i++) {
        waitpid(workers[i], NULL, 0);
    }
    
    // Registrar tempo de fim
    time_t end_time = time(NULL);
    double elapsed_time = difftime(end_time, start_time);
    
    printf("\n=== Resultado ===\n");
    
    // TODO 9: Verificar se algum worker encontrou a senha
    // Ler o arquivo password_found.txt se existir
    
    // IMPLEMENTE AQUI:
    FILE *result_file = fopen(RESULT_FILE, "r");
    if (result_file) {
        char line[256];
        if (fgets(line, sizeof(line), result_file)) {
            char *password = strchr(line, ':');
            if (password) {
                password++; // Pula o ':'
                password[strcspn(password, "\n")] = 0; // Remove a quebra de linha
                printf("SENHA ENCONTRADA: %s\n", password);
            }
        }
        fclose(result_file);
    } else {
        printf("Senha não foi encontrada.\n");
    }
    
    // Estatísticas finais (opcional)
    printf("Tempo total de busca: %.2f segundos.\n", elapsed_time);
    
    return 0;
}