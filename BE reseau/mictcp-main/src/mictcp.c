#include "mictcp.h"
#include "mictcp_core.h"

const int N = 10;
int t[10]={1,1,1,1,1,1,1,1,1,1};
int propdu=0;
mic_tcp_sock sock;
mic_tcp_sock_addr sock_addr;
unsigned int TE=0; //trame emis
unsigned int TA=0; //trame recu
double perte;
double perte_admis=0.3;
int lr=30;
/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");
   printf(__FUNCTION__); 
   printf("\n");
   result = initialize_components(sm);    /* Appel obligatoire */
   if (result!=-1){
   sock.fd=0 ;
   set_loss_rate(lr);

   return sock.fd ;
   }
   else return -1;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");
   printf(__FUNCTION__); 
   printf("\n");
   if (sock.fd==socket) {
        sock.addr=addr;
        return 0;
        }
     else
        {
        return -1;
        }
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: "); 
    printf(__FUNCTION__); 
    printf("\n");
    
    //mic_tcp_socket()
    //mic_tcp_bind()
   int timeout=0;
    
    // Attendre la réception d'une demande de connexion
    while (1) {
        if (sock.state == IDLE) {
            if (sock.addr.ip_addr_size > 0) {
                // Réception de la demande de connexion
                mic_tcp_pdu pdu_syn;
                mic_tcp_sock_addr addr_syn;
         
                IP_recv(&pdu_syn,&addr_syn,timeout);
                
                // Vérification du type de PDU
                if (pdu_syn.header.syn == 1 ) {
                    // Générer le PDU SYN-ACK
                    mic_tcp_pdu pdu_synack;
                    pdu_synack.header.ack = 1;
                    pdu_synack.header.seq_num = sock.fd;
                    pdu_synack.header.ack_num = pdu_syn.header.seq_num + 1;
                    pdu_synack.payload.size = 0;
                    pdu_synack.payload.data = NULL;
                    
                    // Envoi du PDU SYN ACK
                    IP_send(pdu_synack, *addr);
                    
                    // Attente de l'ACK
                    mic_tcp_pdu pdu_ack;
                    mic_tcp_sock_addr addr_ack;
                    IP_recv(&pdu_ack, &addr_ack, timeout) ;
                   
                    
                    // Vérification du type de PDU
                    if (pdu_ack.header.ack == 1 && pdu_ack.header.ack_num == sock.fd) {
                        sock.state = ESTABLISHED;
                        sock.addr = *addr;
                        return 0;
                    }
                }
            }
        }
    }
    return -1;


}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");
    printf(__FUNCTION__); 
    printf("\n");
    
    mic_tcp_pdu pdu_syn, pdu_syn_ack, pdu_ack;
    int nb_essais = 0;
    int timeout=0;

    // Initialisation des PDUs SYN, SYN-ACK et ACK
    pdu_syn.header.source_port = socket;
    pdu_syn.header.dest_port = addr.port;
    pdu_syn.header.seq_num = 0;
    pdu_syn.header.ack_num = 0;
    pdu_syn.header.syn = 1;
    pdu_syn.header.ack = 0;
    pdu_syn.payload.size = 0;

    pdu_syn_ack.header.source_port = socket;
    pdu_syn_ack.header.dest_port = addr.port;
    pdu_syn_ack.header.seq_num = 0;
    pdu_syn_ack.header.ack_num = 0;
    pdu_syn_ack.header.syn = 1;
    pdu_syn_ack.header.ack = 1;
    pdu_syn_ack.payload.size = 0;

    pdu_ack.header.source_port = socket;
    pdu_ack.header.dest_port = addr.port;
    pdu_ack.header.seq_num = 1;
    pdu_ack.header.ack_num = 1;
    pdu_ack.header.syn = 0;
    pdu_ack.header.ack = 1;
    pdu_ack.payload.size = 0;

    // Envoi du PDU SYN et attente de la réponse SYN-ACK
    while (nb_essais < lr) {
        mic_tcp_pdu *pdu_syn_ack_recv = NULL;
        mic_tcp_sock_addr addr_recv;
        if (IP_recv(pdu_syn_ack_recv, &addr_recv, timeout) != -1) {
            if (pdu_syn_ack_recv->header.syn && pdu_syn_ack_recv->header.ack &&
                pdu_syn_ack_recv->header.dest_port == socket && pdu_syn_ack_recv->header.source_port == addr.port) {
                // Réception d'un PDU SYN-ACK valide
                nb_essais = lr; // Sortie de la boucle while
                free(pdu_syn_ack_recv);
            }
        } else {
            // Pas de réponse avant le timeout
            nb_essais++;
            IP_send(pdu_syn, addr);
        }
    }

    if (nb_essais == lr) {
        // Échec de la connexion
        return -1;
    }

    // Envoi du PDU ACK et connexion établie
    IP_send(pdu_ack, addr);
    return 0;
}


    

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: ");   
    printf(__FUNCTION__); 
    printf("\n");
    mic_tcp_pdu pdu;
    sock_addr=sock.addr;
    mic_tcp_pdu ack_pdu;
    int i;
    int taille_pdu;
    int ack_recv=0; // 1 si ack recu , 0 sinon
    int pdu_recu=0;
    unsigned int timeout=100;
    if (sock.fd==mic_sock){
        //construction du pdu
        pdu.header.dest_port=sock_addr.port;
        pdu.header.seq_num=TE;
        pdu.header.ack=0; //init champ ack
        TE= ( TE+1 ) % 2 ; //mise a jour du PE
        
        pdu.payload.data=mesg;
        pdu.payload.size=mesg_size;
        //envoi
        if ((taille_pdu= IP_send(pdu,sock_addr))==-1) {
            printf("erreur envoi\n");
            exit(-1);
            }
        //attente du ack
        ack_pdu.payload.size=0;
        //boucle tant qu'on a pas recu de ack
        while (ack_recv==0) { 
            //verification (reception ack avant le timer expire) ET ( numero de sequence juste)
            if ((IP_recv(&(ack_pdu),&sock_addr,timeout)>=0)&&(ack_pdu.header.ack==1) && (ack_pdu.header.ack_num==TE)) {
                printf("ack recu avec TA = %d \n",TA);
                ack_recv=1;
                t[propdu]=1;
                propdu=(propdu+1)%N;
                }
            else if (IP_recv(&(ack_pdu),&sock_addr,timeout)<0) {
                printf("time out expire\n");
                // on n'a pas recu de pdu car pas de ack dans le timeout
                t[propdu]=0;
                pdu_recu=0;
                //calcul pourcentage de perte
                for (i=0;i<N;i++){
                    pdu_recu+=t[i];
                    }
                printf("nbre de pdu recu: %d\n",pdu_recu);
                perte=(double)(N-pdu_recu)/(double)N;
                printf("perte calculée: %f\n",perte*100.0);
                if (perte<=perte_admis) {
                    printf("perte respectée\n");
                    ack_recv=1;
                    propdu=(propdu+1)%N;
                    TE= (TE+1)%2;
                    }
                else {
                    printf("perte n'est pas respectée, renvoi du pdu \n");
                    if ((taille_pdu=IP_send(pdu,sock_addr))==-1){
                        printf("erreur envoi pdu\n");
                        exit(-1);
                        }
                    }
                }
            else { printf("reception du mauvais numero de seq\n");}
            }
        }
    return taille_pdu;
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: ");
    printf(__FUNCTION__); 
    printf("\n");
    int pdu_recu=-1;
    
    mic_tcp_payload pdu;
    pdu.data=mesg;
    pdu.size=max_mesg_size;
    if (sock.fd==socket)
        pdu_recu=app_buffer_get(pdu);
     
    
    return pdu_recu;
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  ");
    printf(__FUNCTION__); 
    printf("\n");
    int cl= close(socket);
    if (cl<0) { 
        printf("erreur close socket");
        return -1;
        }
    return 0    ;
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des 11221212numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");
    printf(__FUNCTION__); 
    printf("\n");
    mic_tcp_pdu ack_pdu;         //pour éviter les confusions avec .ack
    unsigned long timeout = 100; //100ms
    if (pdu.header.seq_num==TA)
        {
        app_buffer_put(pdu.payload);
        TA=(TA+1)%2;
        }
    else {
        printf("num sequence faux\n");
        //Remarque: en cas de mauvais numéro de séquence, PA ne change pas
        }
    ack_pdu.header.ack_num=TA;
    ack_pdu.header.ack=1;
    ack_pdu.payload.size=0;
    //SOLUTION AU PROBLEME: envoi de plusieurs ack selon le loss rate
    //selon les tests effectués, et pour réduire la probabilité de perte de(s) ack(s), on envoie X ack si le loss rate est fixé à 10*X
    //l'envoie de plusieurs PDUs peut entraîner l'affichage du messages plusieurs fois. En revanche, l'envoi de plusieurs ack
    //ayant tous les même numéro de séquence ne pose aucun problème au programme (si ce n'est l'affichage inutile d'un printf
    //disant qu'il s'agit d'un mauvais numéro de séquence mais cette situation est expliquée dans la ligne 224 du programme)
    //L'envoi de plusieurs ack est donc une bonne solution pour réduire la probabilité de perte de ack et remédie ainsi
    //au problème précédent
    for (int i=0;i<(int)(lr/10);i++){
        if (IP_send(ack_pdu,addr)==-1) {printf("erreur envoi\n");}
        }
       
}
