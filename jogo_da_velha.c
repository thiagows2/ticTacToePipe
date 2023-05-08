#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define CLEAR printf("\033[2J\033[1;1H");
#define MAXBUFF 512
#define CELULA_VAZIA '-'
#define JOGADOR_O 'O'
#define JOGADOR_X 'X'

int organiza_jogo(void);
int verifica_resultado(char buff[3][3]);
void *thread_mostra_tabuleiro(void *arg);
void mostra_tabuleiro(char buff[3][3]);
void zera_matriz(char buff[3][3]);
void recebe_jogada(char buff[3][3], char);
void joga_com_o(int, int, char buff[3][3]);
void joga_com_x(int, int, char buff[3][3]);
void cadastra_jogador(void);
void lista_jogadores(void);
void escolhe_jogadores(void);

typedef struct {
	char nome[40];
	int partidas;
	int vitorias;
} jogador;

jogador dados[10];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int linha, coluna, jogador1, jogador2, aux, cod, posicao = 0;

void *thread_mostra_tabuleiro(void* arg) {
	char (*matriz)[3][3] = (char (*)[3][3]) arg;

	while (1) {
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond, &mutex);
		CLEAR;
		printf("===== TABULEIRO COM THREAD =====");
		mostra_tabuleiro(*matriz);
		pthread_mutex_unlock(&mutex);
	}
}

void mostra_tabuleiro(char matriz[3][3]) {
	printf("\n\n");
	for(linha = 0; linha < 3; linha++) {
		printf("\n      %c | %c | %c ", matriz[linha][0], matriz[linha][1], matriz[linha][2]);
	}
	printf("\n\n");
}

void zera_matriz(char buff[3][3]) {
	for(coluna = 0; coluna < 3;coluna++) {
		for(linha = 0; linha < 3; linha++) {
				buff[linha][coluna] = '-';
		}
	}
}

void cadastra_jogador(void) {
	printf("\nDigite seu Nome: ");
	fflush(stdin);
	scanf("%s", dados[posicao].nome);

	posicao++;
	aux++;

	CLEAR
}

void lista_jogadores(void) {
	printf("\n\nLista de jogadores:\n");
	printf("==========================================\n");
	printf("| Codigo | Nome                           |\n");
	printf("|--------|--------------------------------|\n");
	for (posicao = 0; posicao < aux; posicao++) {
		cod = posicao + 1;
		printf("| %-6d | %-30s |\n", cod, dados[posicao].nome);
	}
	printf("==========================================\n");
}

void mostra_relatorio(void) {
	float valor;

	for (int i = 0; i < aux ; i ++) {
		cod = i + 1;

		printf("\nCodigo do Jogador: %d ", cod);
		printf("\nNome: %s",dados[i].nome);
		printf("\nVitorias: %d ",dados[i].vitorias);
		printf("\nPartidas: %d", dados[i].partidas);

		if (dados[i].partidas == 0) {
			printf("\nPorcentagem de Vitorias: 0%%\n\n");
		} else {
			valor = ((float)dados[i].vitorias / dados[i].partidas) * 100 ;
			printf("\nPorcetagem de Vitorias: %.2f%%\n\n", valor);
		}
	}
}

void escolhe_jogadores(void) {
	printf("\nDigite o codigo do primeiro jogador: ");
	fflush(stdin);
	scanf("%d", &jogador1);
	dados[jogador1-1].partidas++;
	printf("Jogador escolhido: %s", dados[jogador1-1].nome);
	printf("\nDigite o codigo do segundo jogador: ");
	fflush(stdin);
	scanf("%d", &jogador2);
	dados[jogador2-1].partidas++;
	printf("Jogador escolhido: %s", dados[jogador2-1].nome);
}

void recebe_jogada(char buff[3][3], char letra) {
	sleep(0.9); // Espera printar o tabuleiro
	int jogador_atual = letra == 'X' ? jogador1 : jogador2;

	printf("\n\n%s sua vez: ", dados[jogador_atual-1].nome);
	printf("\nDigite a linha: ");
	fflush(stdin);
	scanf("%d", &linha);
	printf("Digite a coluna: ");
	fflush(stdin);
	scanf("%d", &coluna);

	if (buff[linha][coluna] == CELULA_VAZIA) {
		buff[linha][coluna] = letra;
	} else {
		printf("Lugar já cupado");
	}
}

void joga_com_x(int readfd, int writefd, char buff[3][3]) {
	while (1) {
		recebe_jogada(buff, JOGADOR_X);
		write(writefd, buff, MAXBUFF);

		read(readfd, buff, MAXBUFF);
		if (verifica_resultado(buff) != 0) break;
		pthread_cond_signal(&cond); // Acorda thread para imprimir tabuleiro
	}
}

void joga_com_o(int readfd, int writefd, char buff[3][3]) {
	while (1) {
		read(readfd, buff, MAXBUFF);
		if (verifica_resultado(buff) != 0) break;
		pthread_cond_signal(&cond); // Acorda thread para imprimir tabuleiro

		recebe_jogada(buff, JOGADOR_O);
		write(writefd, buff, MAXBUFF);
	}
}

int organiza_jogo(void) {
	pthread_t thread;
	int descritor, pipe1[2], pipe2[2];
	char buff[3][3];

	zera_matriz(buff);
	lista_jogadores();
	escolhe_jogadores();

	if (pipe(pipe1) < 0 || pipe(pipe2) < 0) {
		printf("Erro na chamada PIPE");
		exit(0);
	}
	if ((descritor = fork()) < 0) {
		printf("Erro na chamada FORK");
		exit(0);
	}

	pthread_create(&thread, NULL, thread_mostra_tabuleiro, (void *) buff);

	if (descritor > 0) {
		close(pipe1[0]);
		close(pipe2[1]);

		joga_com_x(pipe2[0], pipe1[1], buff);

		close(pipe1[1]);
		close(pipe2[0]);
		exit(0);

	} else {
		close(pipe1[1]);
		close(pipe2[0]);

		joga_com_o(pipe1[0], pipe2[1], buff);

		close(pipe1[0]);
		close(pipe2[1]);
		exit(0);
	}

	pthread_exit(NULL);

	return 1;
}

int verifica_resultado(char matriz[3][3]) {
	for(linha=0; linha<=2; linha++) {
		for(coluna=0; coluna<=2; coluna++) {
			if(matriz[0][coluna]==JOGADOR_X && matriz[1][coluna]==JOGADOR_X && matriz[2][coluna]==JOGADOR_X) {
				printf("\n\n%s e o(a) ganhador(a)!\n", dados[jogador1-1].nome);
				dados[jogador1-1].vitorias++;
				mostra_tabuleiro(matriz);
				return 1;
			}
			if(matriz[0][coluna]==JOGADOR_O && matriz[1][coluna]==JOGADOR_O && matriz[2][coluna]==JOGADOR_O) {
				printf("\n\n%s e o(a) ganhador(a)!\n", dados[jogador2-1].nome);
				dados[jogador2-1].vitorias++;
				mostra_tabuleiro(matriz);
				return 1;
			}
		}

		if(matriz[linha][0]==JOGADOR_X && matriz[linha][1]==JOGADOR_X && matriz[linha][2]==JOGADOR_X) {
			printf("\n\n%s e o(a) ganhador(a)!\n", dados[jogador1-1].nome);
			dados[jogador1-1].vitorias++;
			mostra_tabuleiro(matriz);
			return 1;
		} else if(matriz[linha][0]==JOGADOR_O && matriz[linha][1]==JOGADOR_O && matriz[linha][2]==JOGADOR_O) {
			printf("\n\n%s e o(a) ganhador(a)!\n", dados[jogador2-1].nome);
			dados[jogador2-1].vitorias++;
			mostra_tabuleiro(matriz);
			return 1;
		}
	}

	if((matriz[0][0]==JOGADOR_O && matriz[1][1]==JOGADOR_O && matriz[2][2]==JOGADOR_O) || (matriz[0][2]==JOGADOR_O && matriz[1][1]==JOGADOR_O && matriz[2][0]==JOGADOR_X)) {
			printf("\n\n%s e o(a) ganhador(a)!\n", dados[jogador2-1].nome);
			dados[jogador2-1].vitorias++;
			mostra_tabuleiro(matriz);
			return 1;
		} else if((matriz[0][0]==JOGADOR_X && matriz[1][1]==JOGADOR_X && matriz[2][2]==JOGADOR_X)|| (matriz[0][2]==JOGADOR_X && matriz[1][1]==JOGADOR_X && matriz[2][0]==JOGADOR_X)) {
			printf("\n\n%s e o(a) ganhador(a)!\n", dados[jogador1-1].nome);
			dados[jogador1-1].vitorias++;
			mostra_tabuleiro(matriz);
			return 1;
		} else {
			return 0;
		}
}

int main(void) {
	int opcao;
	char var;

	printf("+===============+\n");
	printf("| Jogo Da Velha |\n");
	printf("+===============+");

	do {
		printf("\n\nMenu de Opções\n");
		printf("==============\n");
		printf("[1] Realizar Partida\n");
		printf("[2] Cadastro de Jogadores\n");
		printf("[3] Lista de jogadores\n");
		printf("[4] Relatorio \n");
		printf("[5] Sair\n\n");
		printf("Escolha um opção: ");
		fflush(stdin);
		scanf("%d", &opcao);

		switch(opcao) {
			case 1:
				CLEAR;
				organiza_jogo();
				break;
			case 2:
				CLEAR;
				cadastra_jogador();
				break;
			case 3:
				CLEAR;
				lista_jogadores();
				break;
			case 4:
				CLEAR;
				mostra_relatorio();
				break;
			case 5:
				printf("\nEncerrando o programa... \n");
				break;
			default:
				printf("\nOpção inválida!!! Tente outro numero...\n");
				break;
		}
	}

	while(opcao != 5);
	return 0;
}
