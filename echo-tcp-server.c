// Practica Tema 6: Salvador Peñacoba, Javier

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <signal.h>

//Macro assert para comprobar + mensaje de error.
#define ASSERT(_bool, ...) if (!(_bool)) { fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE); }

// Longitud máxima de la cadena a recibir
#define MAX_LENGHT 80

void process_string(char* msg);
void child_function(int clientfd, struct sockaddr_in client_addr);
void signal_handler(int signum);

int sockfd;


int main(int argc, char **argv)
{
    signal(SIGINT, signal_handler);

    int aux;
    int clientfd;
    pid_t childpid;
    uint16_t puerto = htons(5);
    unsigned int client_legth = 0;

    struct sockaddr_in client_addr; //Dirección del cliente.
    struct sockaddr_in my_addr; // Dirección local del servidor.

    memset(&client_addr,0,sizeof(client_addr));
    memset(&my_addr,0,sizeof(my_addr));

    ASSERT((argc == 1 || argc == 3), "Uso: %s [-p puerto_servidor]\n", argv[0]);

    if (argc == 3)
    {
        ASSERT((strcmp(argv[1], "-p") == 0), "Uso: %s [-p puerto_servidor]\n", argv[0]);
        sscanf(argv[2], "%i", &aux);
        ASSERT((aux > 0 && aux <= 65535), "Error: puerto_servidor no es un puerto válido\n");
        puerto = htons(aux);
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = puerto;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //Creamos un socket UDP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT(sockfd != -1, "Error creando socket: %s\n", strerror(errno));

    //Enlazamos el socket a una IP y puerto locales
    aux = bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    ASSERT(aux == 0, "Error vinculando socket: %s\n", strerror(errno));

    //Marcamos el socket como pasivo para aceptar conexiones, con una cola de conexiones pendientes de 10.
    aux = listen(sockfd,10);
    ASSERT(aux==0, "Eror creando el socket pasivo %s\n",strerror(errno))

    printf("Servidor Echo TCP en  %s escuchando puerto %i\n", inet_ntoa(my_addr.sin_addr), ntohs(puerto));
    fflush(stdout);

    while(1){
        //Aceptamos la conexión.
        clientfd = accept(sockfd,(struct sockaddr*)&client_addr,&client_legth);
        ASSERT(clientfd != -1 && errno != EBADF, "Eror creando conexión con cliente %s\n",strerror(errno));

        childpid = fork();
        ASSERT(childpid != -1, "Error creando proceso hijo %s\n", strerror(errno));
        if(childpid == 0){
            //El hijo ejecutará esta función, cierra el socket y termina.
            child_function(clientfd,client_addr);
            shutdown(clientfd,SHUT_RDWR);
            close(clientfd);
            exit(EXIT_SUCCESS);
        }
    }
    shutdown(sockfd,SHUT_RDWR);
    close(sockfd);

    return 0;
}

//Maneja la señal SIGINT, cierra el socket cuando el proceso se termina con ^C
void signal_handler(int signum){
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    exit(0);
}

//Codigo que ejecuta cada hijo que atiende una petición.
void child_function(int clientfd,struct sockaddr_in client_addr){
    int aux;
    char* msg = (char*)calloc(MAX_LENGHT+1,sizeof(char));
    ASSERT(msg != NULL,"Error malloc\n");

    aux = recv(clientfd,msg,MAX_LENGHT+1,0);
    ASSERT(aux != -1, "Error recibiendo mensaje%s\n",strerror(errno));

    printf("Recibida string: %s; desde %s\n",msg,inet_ntoa(client_addr.sin_addr));
    process_string(msg);

    aux = send(clientfd,msg,MAX_LENGHT+1,0);
    ASSERT(aux != -1, "Error enviando el mensaje %s\n",strerror(errno));

    free(msg);
}

//Cambia las mayúsculas por minúsculas y viceversa. Ignora el resto de caracteres.
void process_string(char* msg)
{
    int i;
    for(i = 0; i < strlen(msg); i++){
        if(msg[i] > 64 && msg[i] < 91) msg[i]+=32;
        else if(msg[i]>96 && msg[i]<123) msg[i]-=32;
    }
}
