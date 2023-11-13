#include <stdio.h>
#include <netdb.h>
#include "udp_init.h"
#include "wheel.h"
#include "LinkedListApi.h"

static wheel_timer_t *wt = NULL;
static wheel_elem_t *we = NULL;
static ll_t *rt_table = NULL;
static char data_buffer[1024];
static int sockfd = 0;

typedef struct key{
	char address[16];
	char mask;
  int event_counter;
}key;

typedef struct rt_table_entry_{
	char address[16];
	char mask;
	char gateway[16];
	char oif[32];
}rt_table_entry_t;


void refresh_data(void *arg, int arg_size);

static rt_table_entry_t * create_elem(rt_table_entry_t *temp_elem){

	rt_table_entry_t *new_elem = calloc(1, sizeof(rt_table_entry_t));
	
	*new_elem = *temp_elem;

	return new_elem;
}



int
delete_entry_by_key(char *key, char mask){
	
	singly_ll_node_t *node = rt_table->head;

	while(node != NULL){
		singly_ll_node_t *prev = node;
		node = node->next;
		
		rt_table_entry_t *elem = (rt_table_entry_t *)prev->data;

		if(strncmp(elem->address, key, sizeof(*elem->address)) == 0 && mask == elem->mask){
			singly_ll_delete_node(rt_table, prev);
			free(elem);
			return 1;
		}

	}

	return 0;
}

rt_table_entry_t *
is_entry_in_table(char *key, char mask){
	
	singly_ll_node_t *node = rt_table->head;

	while(node != NULL){
		singly_ll_node_t *prev = node;
		node = node->next;
		
		rt_table_entry_t *elem = (rt_table_entry_t *)prev->data;

		if(strncmp(elem->address, key, sizeof(*elem->address)) == 0 && mask == elem->mask){
			return elem; 
		}

	}

	return NULL;
}

void
check_recv_data(void * arg, int arg_size){

  key *k = (key *)arg;

	socklen_t len;
	
	struct sockaddr_in dest;

	dest.sin_family = AF_INET;
	dest.sin_port = 2000;
	dest.sin_addr.s_addr = INADDR_ANY;
	
	memset(data_buffer, 0, sizeof(data_buffer));

	if(recvfrom(sockfd, (void *)data_buffer, sizeof(data_buffer), MSG_DONTWAIT, (struct sockaddr *)&dest,&len) != -1){
		
		//printf("Entry received!\n");
		rt_table_entry_t *elem = is_entry_in_table(k->address, k->mask);

		if(elem){
			register_app_event(wt, refresh_data, 60, (void *)k, sizeof(*elem), 0);
		}
		else{
			elem = create_elem((rt_table_entry_t *)data_buffer);
				
			singly_ll_add_node_by_val(rt_table, (void *)elem);
			register_app_event(wt, refresh_data, 60, (void *)k, sizeof(*elem), 0);
        }
 }
 else if(k->event_counter < 3){
 	
	if(delete_entry_by_key(k->address, k->mask)){
		printf("Entry deleted!\n");
	}
	else{	
		printf("Entry could not be found!");
	}

        free(k);
 }
 else{
   sendto(sockfd, (void *)k, sizeof(*k),0 ,(struct sockaddr *)&dest, sizeof(struct sockaddr_in));
   register_app_event(wt, check_recv_data, 5, (void *)k, sizeof(*k),0);	
 }
 
 k->event_counter++;
}


void
refresh_data(void *arg, int arg_size){
	
	 key *k = (key *)arg;

	 k->event_counter = 0;

 	struct sockaddr_in dest;	 
	sendto(sockfd, (void *)k, sizeof(*k),0 ,(struct sockaddr *)&dest, sizeof(struct sockaddr_in));

        register_app_event(wt, check_recv_data, 5, (void *)k, sizeof(*k), 0);
}

int main(){
	
		
	sockfd = create_master_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	int choice = 0;
	
	wt = init_wheel_timer(10, 1);
	start_wheel_timer(wt);

	rt_table = init_singly_ll();
	

	do{

		printf("1. Get new entry\n");
		printf("2. Delete entry\n");
		printf("3. Display routing table\n");
		printf("4. exit\n");

		scanf("%d", &choice);

		switch(choice){


			case 1:{
				
   	    			key *k = calloc(1, sizeof(key));
            
        			struct sockaddr_in dest;

	      			dest.sin_family = AF_INET;
	      			dest.sin_port = 2000;
	      			dest.sin_addr.s_addr = INADDR_ANY;

        			printf("Address: ");			
				scanf("%s", k->address);
				scanf("%*[^\n]");	
				scanf("%*c");
				printf("Mask: ");
				scanf("%d", &k->mask);
				
        			k->event_counter = 0;
        
        			sendto(sockfd, (void *)k, sizeof(*k),0 ,(struct sockaddr *)&dest, sizeof(struct sockaddr_in));
        
				register_app_event(wt, check_recv_data, 5, (void *)k, sizeof(*k),0);	

				break;
			   }
		    case 2:{
				
			   	char addr[16];
				char mask;

				printf("Address: ");
				scanf("%s", addr);
				scanf("%*[^\n]");
				scanf("%*c");

				printf("Mask: ");
				scanf("%d", &mask);
				
				
				printf("%s\n", addr);
				printf("%d\n", mask);	
				delete_entry_by_key(addr, mask);	
			   	
				break;
			   }
		    case 3:{

			           singly_ll_node_t *current = rt_table->head;

				   while(current != NULL){

					   rt_table_entry_t *rt_entry = (rt_table_entry_t *) current->data;

					   printf("%16s %16s %32s\n", rt_entry->address, rt_entry->gateway, rt_entry->oif);
					   current = current->next;
				   }
				   break;
			}
		  default:{
				  break;
			}

		}
	}while(choice != 4);
	close(sockfd);

	return 0;
}
