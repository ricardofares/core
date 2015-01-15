/**
*			Copyright (C) 2008-2015 HPDCS Group
*			http://www.dis.uniroma1.it/~hpdcs
*
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
* 
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; either version 3 of the License, or (at your option) any later
* version.
* 
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
* 
* @file control.c
* @brief 
* @author 
*/


#include <stdbool.h>

#include <scheduler/scheduler.h>
#include <scheduler/process.h>
#include <core/core.h>
#include <communication/communication.h>

// TODO: serve solo per vedere la macro  HAVE_LINUX_KERNEL_MAP_MODULE, che poi verrà spostata in configure.ac
#include <mm/modules/ktblmgr/ktblmgr.h>
#include <arch/thread.h>


#ifdef HAVE_LINUX_KERNEL_MAP_MODULE
void unblock_synchronized_objects(unsigned int lid) {
	unsigned int i;
	msg_t control_msg;

	#ifdef FINE_GRAIN_DEBUG
	printf("[UNBLOCK] %d is unblocking remote objects:\n", lid);
	#endif

	for(i = 1; i <= LPS[lid]->ECS_index; i++) {
		bzero(&control_msg, sizeof(msg_t));
		control_msg.sender = LidToGid(lid);
		control_msg.receiver = LidToGid(LPS[lid]->ECS_synch_table[i]);
		control_msg.type = RENDEZVOUS_UNBLOCK;	
		control_msg.timestamp = lvt(lid);
		control_msg.send_time = lvt(lid);
		control_msg.is_antimessage = false;
		control_msg.rendezvous_mark = LPS[lid]->wait_on_rendezvous;
		
		#ifdef FINE_GRAIN_DEBUG
		printf("\t%d\t", LPS[lid]->ECS_synch_table[i]);
		#endif
		
		Send(&control_msg);
	}

	LPS[lid]->wait_on_rendezvous = 0;


	#ifdef FINE_GRAIN_DEBUG
	printf("\n");
	#endif
}
#endif

void rollback_control_message(unsigned int lid, simtime_t simtime) {
	msg_t control_antimessage;
	
	msg_t *msg, *msg_prev;

	if(list_empty(LPS[lid]->rendezvous_queue)) {
		return;
	}

	msg = list_tail(LPS[lid]->rendezvous_queue);
	while(msg != NULL && msg->timestamp > simtime) {
	

		#ifdef FINE_GRAIN_DEBUG
		printf("[RENDEZVOUS ROLLACK] sending a RENDEZVOUS ROLLBACK message to %d\n", msg->sender);
		#endif

		// antimessaggio di controllo
		bzero(&control_antimessage, sizeof(msg_t));
		control_antimessage.type = RENDEZVOUS_ROLLBACK;
                control_antimessage.sender = msg->receiver;
                control_antimessage.receiver = msg->sender;
                control_antimessage.timestamp = msg->timestamp;
                control_antimessage.send_time = msg->send_time;
		control_antimessage.rendezvous_mark = msg->rendezvous_mark;
                control_antimessage.is_antimessage = other;

                Send(&control_antimessage);

		msg_prev = list_prev(msg);
		list_delete_by_content(LPS[lid]->rendezvous_queue, msg);
		msg = msg_prev;
	}
}

// return false if the antimessage is recognized (and processed) as a control antimessage
bool anti_control_message(msg_t * msg) {
	msg_t *old_rendezvous ;


	#ifdef  HAVE_LINUX_KERNEL_MAP_MODULE
	if(msg->type == RENDEZVOUS_ROLLBACK) {

		unsigned int lid_receiver = msg->receiver;

		#ifdef FINE_GRAIN_DEBUG
		printf("[RENDEZVOUS ROLLACK] %d is undoing a rendezvous point\n", lid_receiver);
		#endif

		if(tid != LPS[lid_receiver]->worker_thread) {
			printf("ERRORE ERRORE ERRORE\n");
			abort();
		}
		old_rendezvous = list_tail(LPS[lid_receiver]->queue_in);
		while(old_rendezvous != NULL && old_rendezvous->rendezvous_mark != msg->rendezvous_mark) {
			old_rendezvous = list_prev(old_rendezvous);
		}

		if(old_rendezvous == NULL) {
//			rootsim_error(true, "I'm asked to annihilate a rendezvous point, but there is no such rendezvous mark: %llu\n", msg->rendezvous_mark);
			return false;
		}

		if(old_rendezvous->timestamp <= lvt(lid_receiver)) {
			LPS[lid_receiver]->bound = old_rendezvous;
	                while (LPS[lid_receiver]->bound != NULL && LPS[lid_receiver]->bound->timestamp >= old_rendezvous->timestamp) {
	                	LPS[lid_receiver]->bound = list_prev(LPS[lid_receiver]->bound);
	        	}
	                LPS[lid_receiver]->state = LP_STATE_ROLLBACK;
		}
		old_rendezvous->rendezvous_mark = 0;

		if(LPS[lid_receiver]->wait_on_rendezvous == msg->rendezvous_mark) {
			LPS[lid_receiver]->ECS_index = 0;
			LPS[lid_receiver]->wait_on_rendezvous = 0;
		}
		return false;
	}
	#endif

	return true;
}
// return true if the control message should be reprocessed during silent exceution
bool reprocess_control_msg(msg_t *msg) {


	if(msg->type < MIN_VALUE_CONTROL) {
		return true;
	}

	return false;
}


// return true if the event must not be filtered here
bool receive_control_msg(msg_t *msg) {

	if(msg->type < MIN_VALUE_CONTROL || msg->type > MAX_VALUE_CONTROL) {
		return true;
	}

	#ifdef  HAVE_LINUX_KERNEL_MAP_MODULE
	switch(msg->type) {

		case RENDEZVOUS_START:
			#ifdef FINE_GRAIN_DEBUG
			printf("[SYNCH] %d has received a RENDEZVOUS START request from %d\n", msg->receiver, msg->sender);
			#endif
			return true;

		case RENDEZVOUS_ACK:
			if(LPS[msg->receiver]->state == LP_STATE_ROLLBACK) {
				#ifdef FINE_GRAIN_DEBUG
				printf("[SYNCH] %d is discarding a RENDEZVOUS ACK message because has to roll back\n", msg->receiver);
				#endif
				break;
			}

			#ifdef FINE_GRAIN_DEBUG
			printf("[SYNCH] %d has received a RENDEZVOUS ACK message and is passing in READY FOR SYNCH state\n", msg->receiver);
			#endif
			if(LPS[msg->receiver]->wait_on_rendezvous == msg->rendezvous_mark) {
				LPS[msg->receiver]->state = LP_STATE_READY_FOR_SYNCH;
			}
			
			break;

		case RENDEZVOUS_UNBLOCK:
			#ifdef FINE_GRAIN_DEBUG
			printf("[UNBLOCK] %d has received a RENDEZVOUS UNBLOCK message and is passing in READY state\n", msg->receiver);
			#endif
//			if(LPS[msg->receiver]-> state != LP_STATE_WAIT_FOR_UNBLOCK) {
			if(LPS[msg->receiver]->wait_on_rendezvous == msg->rendezvous_mark) {
				LPS[msg->receiver]->state = LP_STATE_READY;
				LPS[msg->receiver]->wait_on_rendezvous = 0;
			}
			current_lp = msg->receiver;
			current_lvt = msg->timestamp;
			force_LP_checkpoint(current_lp);
			LogState(current_lp);
			current_lvt = INFTY;
			current_lp = IDLE_PROCESS;

			break;

		case RENDEZVOUS_ROLLBACK:
			#ifdef FINE_GRAIN_DEBUG
			printf("[RENDEZVOUS ROLLBACK] %d has received a RENDEZVOUS ROLLBACK message\n", msg->receiver);
			#endif
			return true;

		default:
			rootsim_error(true, "Trying to handle a control message which is meaningless at receive time: %d\n", msg->type);

	}
	#endif

	return false;
}


// return true if must be passed to the LP
bool process_control_msg(msg_t *msg) {

	#ifdef HAVE_LINUX_KERNEL_MAP_MODULE
	msg_t control_msg;
	#endif


	if(msg->type < MIN_VALUE_CONTROL || msg->type > MAX_VALUE_CONTROL) {
		return true;
	}

	#ifdef HAVE_LINUX_KERNEL_MAP_MODULE
	switch(msg->type) {

		case RENDEZVOUS_START:
			list_insert(LPS[msg->receiver]->rendezvous_queue, timestamp, msg);
			// Place this into input queue
			LPS[msg->receiver]->wait_on_rendezvous = msg->rendezvous_mark;

			#ifdef FINE_GRAIN_DEBUG
			printf("[SYNCH] %d has processed the RENDEZVOUS START request from %d and is now in WAIT FOR UNBLOCK state. RENDEZVOUS ACK message sent back.\n", msg->receiver, msg->sender);
			#endif
			LPS[msg->receiver]->state = LP_STATE_WAIT_FOR_UNBLOCK;
			bzero(&control_msg, sizeof(msg_t));
			control_msg.sender = msg->receiver;
			control_msg.receiver = msg->sender;
			control_msg.type = RENDEZVOUS_ACK;	
			control_msg.timestamp = msg->timestamp;
			control_msg.send_time = msg->timestamp;
			control_msg.is_antimessage = false;
			control_msg.rendezvous_mark = msg->rendezvous_mark;
			Send(&control_msg);
			break;

/*		case RENDEZVOUS_ACK:
			#ifdef FINE_GRAIN_DEBUG
			printf("[SYNCH] %d has processed the RENDEZVOUS ACK message from %d and is now in READY state.\n", msg->receiver, msg->sender);
			#endif
			LPS[msg->receiver]->state = LP_STATE_READY_FOR_SYNCH;
			return true;
*/
/*		case RENDEZVOUS_UNBLOCK:
			#ifdef FINE_GRAIN_DEBUG
			printf("[SYNCH] %d has processed the RENDEZVOUS UNBLOCK request from %d and is now in READY state.\n", msg->receiver, msg->sender);
			#endif
			LPS[msg->receiver]->state = LP_STATE_READY;
			break;
*/
		default:
			rootsim_error(true, "Trying to handle a control message which is meaningless at schedule time: %d\n", msg->type);

	}
	#endif

	return false;
}

