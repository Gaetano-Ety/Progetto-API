#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LENGTH_RIF 17 // Costante che segna la lunghezza di "+inserisci_inizio", il più lungo dei comandi
#define NUMERO_SIMBOLI 64 // Costante che indica il numero dei simboli che possono apparire in una parola (A-Z, a-z, -. _)

typedef enum {inizio, partita, inserimento, attesa} sts;

typedef struct el_diz{
	char c; // Carattere 
	struct el_diz *next; // Prossimo elemento
	struct el_diz *step; // Prossimo carattere della stringa
} el_diz;

typedef struct strList{
	char *str;
	struct strList *next;
} strList;

typedef struct charList{
	char c;
	struct charList *next;
} charList;

typedef struct vincolo{
	char esiste;
	char* dovuti; // Una stringa di pari dimensione delle parole, conserva i caratteri certi
	unsigned short presenze[NUMERO_SIMBOLI]; // Una cella per ogni carattere esistente: segna carattere per carattere quante volte compare
	char esatti[NUMERO_SIMBOLI]; // Gemmella dell'array presenze, affianca un valore booleano ad ogni numero n per dire se è un "almeno n" (false) o "esattamente n" (true)
	charList** nonDovuti; // Array di puntatori a liste, contiene per ogni posizione della parola quali caratteri non può avere
} vincolo;

// Il dizionario è una variabile globale, non ci possono essere più dizionari nel programma
el_diz *dizionario = NULL;

// Piccola funzione da chiamare in caso di errore
void errore(){
	printf("Errore\n");
	exit(1);
}

// Funzione che ritorna la cifra dell'array dei vincoli per ogni carattere
unsigned short myHash(char c){
	/*
	0-9 -> 0-9
	A-Z -> 10-35
	a-z -> 36-61
	- -> 62
	_ -> 63
	*/
	
	if(c == '-') return 62;
	if(c == '_') return 63;
	if(c < 58) return (int)(c-48);
	if(c < 91) return (int)(c-55);
	if(c < 123) return (int)(c-61);

	return 0;
}

// Funzione che data una lista di caratteri libera la memoria
void smontaLista(charList** p){
	if(*p == NULL) return;
	smontaLista(&((*p)->next));
	free(*p);
	*p = NULL;
}

// Funzione che aggiunge una stringa al dizionario
// L'aggiunta viene fatta in ordine, gli elementi minori vengono posizionati prima
void aggiungi(char* str){
	// Puntatore alla posizione corrente nel dizionario
	el_diz **p = &dizionario;
	
	// Variabili di lavoro
	unsigned short i = 0;
	char c;
	
	// Finché non leggo l'ultimo carattere...
	while ((c = str[i]) != '\0'){
		// Avanzo nella stringa aggiungendo carattere per carattere la parola al dizionario; eseguo una ricerca per capire dove posizionare ogni carattere
		while(*p != NULL){
			if ((*p)->c == c){
				// Se il carattere di riferimento è già inserito allora passo all'inserimento in step del prossimo carattere
				// Se il carattere di riferimento è l'ultimo della stringa allora non ha senso avanzare
				if(str[i+1] != '\0')
					p = &((*p)->step);
				break;
			}else if((*p)->c > c){
				// Se il carattere che sto cercando non è presente creo una cella per inserirlo in next nella sua posizione; finché non raggiungo la sua posizione vado avanti in next
				el_diz *temp = (el_diz*) malloc(sizeof(el_diz));
				
				temp->next = *p;
				(*p) = temp;
				(*p)->c = '\0';
				(*p)->step = NULL;
				break;
			}else p = &((*p)->next);
		}
		
		// Se raggiungo la fine della lista in next devo aggiungere un nuovo elemento
		if (*p == NULL){
			*p = (el_diz*) malloc(sizeof(el_diz));
			(*p)->c = '\0';
			(*p)->next = NULL;
			(*p)->step = NULL;
		}
		
		// Se ho dovuto creare un nuovo elemento, gli assegno il carattere dovuto e vado avanti in step con il prossimo carattere
		if((*p)->c == '\0'){
			(*p)->c = c;
			p = &((*p)->step);
		}
		
		// Avanzo di carattere
		i++;
	}
}

// Verifica se una stringa è presente o no nel dizionario
char cerca(char* str){
	int i = 0;
	el_diz* p = dizionario;
	
	// Scorro la parola e il dizionario finché non trovo la parola
	while(str[i] != '\0'){
		if(p == NULL || p->c > str[i]) return 0;
		
		while(p->c != str[i]){
			p = p->next;
			if(p == NULL || p->c > str[i]) return 0;
		}
		p = p->step;
		i++;
	}
	
	return 1;
}

// Funzione che resetta i vincoli
void resetVincoli(vincolo* v, unsigned short len){
	if(v->esiste){
		for(short j=0; j<len; j++){
			v->dovuti[j] = '\0';
			smontaLista(&(v->nonDovuti[j]));
		}
		for(short j=0; j<NUMERO_SIMBOLI; j++){
			v->presenze[j] = 0;
			v->esatti[j] = 0;
		}		
		v->esiste = 0;
	}
}

unsigned short getNextPointer(el_diz** p, unsigned short pos, el_diz** prev, unsigned short* contate){
	el_diz* act = *p;
	char endNo = 1;
	
	if(act->next != NULL){
		*p = act->next;	
	}else{
		while(pos){
			pos--;
			act = prev[pos];
			contate[myHash(act->c)]--;
			if(act->next != NULL){
				*p = act->next;
				endNo = 0;
				break;
			}
		}
		if(endNo) return -1; // Se sto puntando alla prima posizione e non sono andato avanti allora sono finite le parole da esaminare
	}
	
	return pos;
}

unsigned int contaCompatibili(vincolo* v, unsigned short len){
	el_diz* p = dizionario; // Puntatore all'elemento corrente
	// Un dizionario vuoto non stampa niente
	if(p == NULL) return 0;
	
	unsigned int conteggio = 0; // Variabile di output
	unsigned short contate[NUMERO_SIMBOLI] = {0}; // Array che per ogni carattere andrà a segnare quante volte è stato contato per parola
	el_diz* prev[len-1]; // Array tale per cui ogni nodo di carattere punti al nodo del carattere precedente (tranne il primo ovviamente)
	unsigned short pos = 0; // Vaiabile che uso per tenere l'indice dell'array
	unsigned short idx;
	charList* nd;
	char end = 0;	
	
	
	do{
		// Posizione del carattere nell'array dei vincoli
		idx = myHash(p->c);
		
		/* Verifico compatibilità un carattere */
		
		// Verifico che l'attuale carattere possa esserci
		if(!(v->presenze[idx]) && v->esatti[idx]){
			pos = getNextPointer(&p, pos, prev, contate);
			if(pos == (unsigned short) -1) break;
			continue;
		}
		
		// Verifica se è dovuta
		if(v->dovuti[pos] != p->c){
			// Se non è dovuto...
			// Se il carattere dovuto è un altro è inutile procedere per questa strada
			if(v->dovuti[pos] != '\0'){
				pos = getNextPointer(&p, pos, prev, contate);
				if(pos == (unsigned short) -1) break;
				continue;
			}
			
			// Verifico che non sia non dovuto
			nd = v->nonDovuti[pos];
			while(nd != NULL){
				if(p->c == nd->c){
					pos = getNextPointer(&p, pos, prev, contate);
					if(pos == (unsigned short) -1) end = 1;
					break;
				}
				nd = nd->next;
			}
			if(end) break;
			if(nd != NULL) continue; // Se ho beccato un non dovuto, ho interrotto prima che nd si aggiornasse eventualmente a NULL
		}
		
		// Verifico se ho già raggiunto il numero esatto di ricorrenze di quel carattere
		if((contate[idx] >= v->presenze[idx]) && v->esatti[idx]){
			pos = getNextPointer(&p, pos, prev, contate);
			if(pos == (unsigned short) -1) break;
			continue;
		}
		// Conto di aver letto questo carattere
		contate[idx]++;
				
		// Vado avanti in step
		if(p->step != NULL){
			prev[pos] = p;
			p = p->step;
			pos++;
			continue;
		}else{
			// Se sono all'ultima parola verifico di aver contato almeno quanto dovevo
			for(short j=0; j<NUMERO_SIMBOLI; j++){
				if(contate[j] < v->presenze[j]){
					conteggio--;
					break;
				}
			}
			conteggio++;
			
			contate[idx]--;
			pos = getNextPointer(&p, pos, prev, contate);
			if(pos == (unsigned short) -1) break;
		}
	}while(1);
	
	return conteggio;
}

// Funzione che confronta due stringhe e ne estrae il vincolo
void traduzione(char* str, char* rif, int len, vincolo* v){
	char dest[len+1];
	dest[len] = '\0';
	unsigned short nNonCorretti[NUMERO_SIMBOLI] = {0};
	unsigned short presenze[NUMERO_SIMBOLI] = {0};
	short idxTemp;
	charList** p;	

	for(unsigned short i=0; i<len; i++){
		idxTemp = myHash(rif[i]);		
		
		if(str[i] == rif[i]){
			dest[i] = '+';
			v->dovuti[i] = rif[i];
			presenze[idxTemp]++;
			
			// Se so che in una certa posizione X vi è uno specifico carattere Y, allora automaticamente so tutti quelli che non vi sono
			smontaLista(&(v->nonDovuti[i]));
		}else{
			// So che il carattere che sto osservando compare in una posizione sbagliata; tengo conto di ciò
			nNonCorretti[idxTemp]++;
			
			// Aggiunta di un non-dovuto al vincolo
			// Se so già che vi è un carattere specifico è inutile specificare cosa non c'è
			if(v->dovuti[i] == '\0'){
				p = &(v->nonDovuti[i]);
				while((*p) != NULL){
					if((*p)->c == str[i]) break;
					p = &((*p)->next);
				}
				if((*p) == NULL){
					*p = (charList*) malloc(sizeof(charList));
					(*p)->c = str[i];
				}
			}
		}
	}
	
	for(unsigned short i=0; i<len; i++){
		// Se il carattere è in posizione corretta ci ho già pensato
		if(str[i] == rif[i]) continue;
		
		idxTemp = myHash(str[i]);
		if(nNonCorretti[idxTemp] > 0){
			dest[i] = '|';
			nNonCorretti[idxTemp]--;
			presenze[idxTemp]++;
		}else dest[i] = '/';		
	}
	
	printf("%s\n", dest);
	
	// Il vincolo dovrà avere quanti caratteri ci sono "almeno", pertanto prenderò il maggiore per ogni carattere
	for(short j=0; j<NUMERO_SIMBOLI; j++)
		if(presenze[j] > v->presenze[j])
			v->presenze[j] = presenze[j];
	
	// Verifica l'esattezza del numero di alcuni caratteri
		// Tutti i caratteri presenti in p per cui c'è almeno una '/' nella loro posizione hanno segnato il numero esatto di caratteri
	for(unsigned short j=0; j<len; j++)
		if(dest[j] == '/')
			v->esatti[myHash(str[j])] = 1;
}

void stampa_filtrate(vincolo* v, unsigned short len){
	el_diz* p = dizionario; // Puntatore all'elemento corrente
	
	// Un dizionario vuoto non stampa niente
	if(p == NULL) return;
	
	char out[len+1]; // Stringa di output
	out[len] = '\0';
	unsigned short contate[NUMERO_SIMBOLI] = {0}; // Array che per ogni carattere andrà a segnare quante volte è stato contato per parola
	el_diz* prev[len-1]; // Array tale per cui ogni nodo di carattere punti al nodo del carattere precedente (tranne il primo ovviamente)
	unsigned short pos = 0; // Vaiabile che uso per tenere l'indice dell'array
	unsigned short idx;
	charList* nd;
	char end = 0;	
	
	do{
		// Posizione del carattere nell'array dei vincoli
		idx = myHash(p->c);
		
		/* Verifico compatibilità un carattere */
		
		// Verifico che l'attuale carattere possa esserci
		if(!(v->presenze[idx]) && v->esatti[idx]){
			pos = getNextPointer(&p, pos, prev, contate);
			if(pos == (unsigned short) -1) break;
			continue;
		}
		
		// Verifica se è dovuta
		if(v->dovuti[pos] != p->c){
			// Se non è dovuto...
			// Se il carattere dovuto è un altro è inutile procedere per questa strada
			if(v->dovuti[pos] != '\0'){
				pos = getNextPointer(&p, pos, prev, contate);
				if(pos == (unsigned short) -1) break;
				continue;
			}
			
			// Verifico che non sia non dovuto
			nd = v->nonDovuti[pos];
			while(nd != NULL){
				if(p->c == nd->c){
					pos = getNextPointer(&p, pos, prev, contate);
					if(pos == (unsigned short) -1) end = 1;
					break;
				}
				nd = nd->next;
			}
			if(end) break;
			if(nd != NULL) continue; // Se ho beccato un non dovuto, ho interrotto prima che nd si aggiornasse eventualmente a NULL
		}
		
		// Verifico se ho già raggiunto il numero esatto di ricorrenze di quel carattere
		if((contate[idx] >= v->presenze[idx]) && v->esatti[idx]){
			pos = getNextPointer(&p, pos, prev, contate);
			if(pos == (unsigned short) -1) break;
			continue;
		}
		// Conto di aver letto questo carattere
		contate[idx]++;
				
		// Vado avanti in step o mando in stampa la parola
		out[pos] = p->c;
		if(p->step != NULL){
			prev[pos] = p;
			p = p->step;
			pos++;
			continue;
		}else{
			char stampare = 1;
			// Se sono all'ultima parola verifico di aver contato almeno quanto dovevo
			for(short j=0; j<NUMERO_SIMBOLI; j++){
				if(contate[j] < v->presenze[j]){
					stampare = 0;
					break;
				}
			}
			
			// Se arrivo alla fine rispettando tutti i vincoli mando in stampa la parola
			if(stampare) printf("%s\n", out);
			
			// Vado alla prossima parola
			contate[idx]--;
			pos = getNextPointer(&p, pos, prev, contate);
			if(pos == (unsigned short) -1) break;
		}
	}while(1);
}

int main(){
	unsigned short k;
	int useless;
	
	// Variabile che indica lo stato del programma, cioè cosa sta facendo in quel momento
	sts status = inizio;
	sts prec = inizio; // Indica lo stato precedente, serve per lo switch attesa-inserimento-partita
	
	// Inserimento della dimensione delle stringhe
	useless = scanf("%hd", &k);
	if(!useless){
		errore();
	}
	
	// La stringa su cui viene scritto l'ingresso deve poter leggere tutte le stringhe in input, quindi deve avere almeno la dimensione del comando massimo
	unsigned short x;
	if(k < LENGTH_RIF) x = LENGTH_RIF;
	else x = k;
	char str[x];
	
	char rif[k]; // Parola di riferimento delle partite
	int max_p; // Massimo numero di parole per una partita
	vincolo vinc; // Struttura vincolo che si aggiornerà ad ogni partita
	int lastCompatibili = -1; // Ogni volta che conto il numero di parole compatibili, segna l'ultima contata
	
	// Inizializzazione dei vincoli
	vinc.dovuti = (char*) malloc(k);
	vinc.nonDovuti = (charList**) malloc(sizeof(charList*)*k);
	vinc.esiste = 1;
	resetVincoli(&vinc, k);
	
	do{
		useless = scanf("%s", str);
		if(useless == EOF) break;
		if(!useless) printf("Errore\n");
		
		if(str[0] == '+'){
			if(!strcmp("+nuova_partita", str)){
				// Appena inizia una nuova partita resetto i vincoli e imposto lo stato di partita
				status = partita;
				vinc.esiste = 1;
				resetVincoli(&vinc, k);
				lastCompatibili = -1;
				
				// Inserimento della stringa riferimento
				useless = scanf("%s", rif);
				if(!useless) errore();
				
				// Inserimento lunghezza massima partita
				useless = scanf("%d", &max_p);
				if(!useless) errore();
				
				// Se inserisco 0 come numero massimo di parole inseribili allora agiscso come se ho terminato le parole da inserire
				if(!max_p){
					printf("ko\n");
					status = attesa;
				}
			}else if(!strcmp("+stampa_filtrate", str)){
				// Stampo tutte le parole compatibili con i vincoli attuali; se ho una sola parola compatibile con i vincoli sarà per forza quella di riferimento
				if(lastCompatibili == 1)
					printf("%s\n", rif);
				else if(lastCompatibili != 0)
					stampa_filtrate(&vinc, k);
			}else if(!strcmp("+inserisci_inizio", str)){
				prec = status;
				status = inserimento;
			}else if(!strcmp("+inserisci_fine", str)){
				status = prec;
			}else errore();
		}else{
			// Se sono in attesa e leggo una parola è un errore *da ignorare*: lo stato di attesa si sblocca con "+nuova_partita" o "+inserisci_fine"
			if(status == attesa) errore();

			// Se sto iniziando a leggere le parole o se ho attivato il comando "+inserisci_inizio" devo aggiungere delle parole al dizionario
			if(status == inizio || status == inserimento)
				aggiungi(str);
			
			// Se ho iniziato una nuova partita...
			if(status == partita){
				// Mi assicuro che le parole inserite esistano nel dizionario
				if(!cerca(str))
					printf("not_exists\n");
				else{
					max_p--;
					// Se la stringa che sto guardando al momento è uguale a quella di riferimento mando in output "ok" e chiudo la partita
					if(!strcmp(rif, str)){
						printf("ok\n");
						status = attesa;
						resetVincoli(&vinc, k);
						lastCompatibili = -1;
					}else{
						// Se la stringa che sto guardando non è quella di riferimento ma esiste mando in output il vincolo
						traduzione(str, rif, k, &vinc);
						
						// Conto quante parole nella partita siano ancora compatibili con quella di riferimento; se già prima ho scoperto che ce n'era una sola ora non possono essercene di più
						if(lastCompatibili == 1)
							printf("1\n");
						else if(lastCompatibili == 0)
							printf("0\n");
						else{
							// Conto quante sono ancora le parole compatibili con i vincoli fin ora e mando in output
							lastCompatibili = contaCompatibili(&vinc, k);
							printf("%d\n", lastCompatibili);
						}
												
						// Se è anche l'ultima stringa possibile della partita mando in output "ko" e chiudo la partita
						if(!max_p){
							printf("ko\n");
							status = attesa;
							resetVincoli(&vinc, k);
							lastCompatibili = -1;
						}
					}
				}
			}
		}
	}while(1);
	
	return 0;
}
