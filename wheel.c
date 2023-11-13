
#include "wheel.h"
#include <memory.h>
#include <assert.h>
#include <unistd.h>

wheel_timer_t *
init_wheel_timer(int wheel_size, int clock_tick_interval){
	
	wheel_timer_t *wt = calloc(1, sizeof(wheel_timer_t) + sizeof(ll_t*) * wheel_size);

	assert(wt != NULL);

	wt->wheel_size = wheel_size;
	wt->clock_tick_interval = clock_tick_interval;
	wt->current_cycle_no = 0;

	memset(&(wt->wheel_thread), 0, sizeof(wheel_timer_t));

	for(int i = 0; i < wheel_size; i++)
		wt->slot[i] = init_singly_ll();
	
	return wt;
}

wheel_elem_t *
register_app_event(wheel_timer_t *wt,
		   app_call_back app_callback,
		   int timer_interval,
		   void* arg,
		   int arg_size,
		   char is_recursive){

	wheel_elem_t *elem = calloc(1, sizeof(wheel_elem_t));
	singly_ll_node_t *node = calloc(1, sizeof(singly_ll_node_t));

	assert(elem != NULL);


	elem->app_callback = app_callback;
	elem->timer_interval = timer_interval;
	elem->arg = arg;
	elem->arg_size = arg_size;
	elem->is_recursive = is_recursive;
	
	node->data = (void *)elem;

	int trigger_event_time = (TOTAL_CLOCK_TICKS + timer_interval);

	elem->execute_cycle_no = trigger_event_time / wt->wheel_size;

	singly_ll_add_node(wt->slot[trigger_event_time % wt->wheel_size], node);	
	
	return elem;
}

void
deregister_app_event(wheel_timer_t *wt, wheel_elem_t *we){
	
	int trigger_event_time = TOTAL_CLOCK_TICKS + we->timer_interval;
	
	singly_ll_node_t * node = singly_ll_get_node_by_data_ptr(wt->slot[trigger_event_time % wt->wheel_size], (void *)we);

	singly_ll_remove_node(wt->slot[trigger_event_time % wt->wheel_size], node);
	
	free(node);
	printf("Node removed!\n");
	display_all_nodes(wt);

	//singly_ll_delete_node_by_value(wt->slot[trigger_event_time % wt->wheel_size], (void *)we, sizeof(wheel_elem_t));
}

void
display_all_nodes(wheel_timer_t *wt){

	
	for(int i = 0; i < wt->wheel_size; i++){

		singly_ll_node_t *node = wt->slot[i]->head;

		while(node != NULL){


			printf("%p -> ", node);

			node = node->next;
		}

		printf("NULL\n");
	}


}

static void *
wheel_fn(void *arg){

	wheel_timer_t *wt = (wheel_timer_t *)arg;	
	while(1){
		
		wt->current_clock_tick++;
		

		if(wt->current_clock_tick == wt->wheel_size){
			wt->current_clock_tick = 0;
			wt->current_cycle_no++;
		}
			
		sleep(wt->clock_tick_interval);

		ll_t *slot_list = wt->slot[wt->current_clock_tick];
		
		singly_ll_node_t *node  = slot_list->head;
		
		display_all_nodes(wt);
	
		
		if(!node)
			printf("Wheel Timer Time = %d : \n", TOTAL_CLOCK_TICKS);
		else
			printf("Wheel Timer Time = %d : ", TOTAL_CLOCK_TICKS);
			


		while(node != NULL){
			
			//printf("%d\n", wt->current_clock_tick);	
			wheel_elem_t *elem = (wheel_elem_t *)node->data;
				
			if(elem->execute_cycle_no == wt->current_cycle_no){

				elem->app_callback(elem->arg, elem->arg_size);

				if(elem->is_recursive){
					singly_ll_remove_node(slot_list, node);
					int trigger_event_time = TOTAL_CLOCK_TICKS + elem->timer_interval;
					elem->execute_cycle_no = trigger_event_time / wt->wheel_size;
					singly_ll_add_node(wt->slot[trigger_event_time % wt->wheel_size], node);
				}
				else{
				    singly_ll_node_t *next_node = node->next;
					singly_ll_delete_node(slot_list, node);
					node = next_node;
					continue;
				}
				
			}
			
			display_all_nodes(wt);	
			node = node->next;
		}
		
	}
}

void
start_wheel_timer(wheel_timer_t *wt){
	pthread_t *thread = &(wt->wheel_thread);
	if(pthread_create(thread, NULL, wheel_fn, (void *)wt)){
		printf("Wheel Timer Thread initialization failed, exiting...\n");
		exit(0);
	}
}
